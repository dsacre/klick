/*
 * klick - an advanced metronome for jack
 *
 * Copyright (C) 2007-2008  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "klick.hh"
#include "options.hh"
#include "audio_interface.hh"
#include "audio_chunk.hh"
#include "tempomap.hh"
#include "metronome_map.hh"
#include "metronome_jack.hh"

#include <string>
#include <iostream>
#include <fstream>
#include <unistd.h>

#include "util/string.hh"
#include "util/exception.hh"
#include "util/logstream.hh"

using namespace std;
using namespace das;


Klick::Klick(int argc, char *argv[])
  : _options(new Options),
    _quit(false)
{
    _options->parse(argc, argv);
    logv.enable(_options->verbose);

    // determine client name
    if (_options->client_name.empty()) {
        _options->client_name = "klick";
    }

    // setup jack
    _audio.reset(new AudioInterface(_options->client_name,
                                    _options->connect_ports,
                                    _options->auto_connect));

    logv << "jack client name: " << _audio->client_name() << endl;

    if (_options->follow_transport) {
        logv << "using follow transport, no tempomap" << endl;
    } else {
        load_tempomap();
    }

    if (_options->transport_master) {
        logv << "jack transport master enabled" << endl;
    } else if (_options->transport_enabled) {
        logv << "jack transport enabled" << endl;
    }

    load_samples();

    // create metronome object
    if (_options->follow_transport) {
        _metro.reset(new MetronomeJack(_click_emphasis, _click_normal));
    } else {
        _metro.reset(new MetronomeMap(_map, _options->tempo_multiplier,
                                      _options->transport_enabled,
                                      _options->transport_master,
                                      _options->preroll,
                                      _options->start_label,
                                      _click_emphasis, _click_normal));
    }
}


Klick::~Klick()
{
}


void Klick::load_tempomap()
{
    if (_options->filename.length()) {
        logv << "loading tempomap from file" << endl;
        _map = TempoMap::new_from_file(_options->filename);
    } else {
        logv << "loading tempomap from command line" << endl;
        _map = TempoMap::new_from_cmdline(_options->cmdline);
    }

    logv << "tempomap:" << endl
         << indent(_map->dump(), 2);

    // make sure the start label exists
    if (_options->start_label.length()) {
        if (_map->entry(_options->start_label)) {
            logv << "starting at label: " << _options->start_label << endl;
        } else {
            throw das::exception(make_string() << "label '" << _options->start_label << "' not found in tempomap");
        }
    }
}


void Klick::load_samples()
{
    if (_options->click_sample == Options::CLICK_SAMPLE_FROM_FILE) {
        logv << "loading samples from file:" << endl
               << "  emphasis: " << _options->click_filename_emphasis << endl
               << "  normal: " << _options->click_filename_normal << endl;
    }

    string emphasis, normal;

    switch (_options->click_sample) {
      case 0:
        emphasis = data_file("sounds/square_emphasis.flac");
        normal   = data_file("sounds/square_normal.flac");
        break;
      case 1:
        emphasis = data_file("sounds/sine_emphasis.flac");
        normal   = data_file("sounds/sine_normal.flac");
        break;
      case 2:
        emphasis = data_file("sounds/noise_emphasis.flac");
        normal   = data_file("sounds/noise_normal.flac");
        break;
      case 3:
        emphasis = data_file("sounds/click_emphasis.flac");
        normal   = data_file("sounds/click_normal.flac");
        break;
      case Options::CLICK_SAMPLE_FROM_FILE:
        emphasis = _options->click_filename_emphasis;
        normal   = _options->click_filename_normal;
        break;
    }

    _click_emphasis.reset(new AudioChunk(emphasis, Audio->samplerate()));
    _click_normal.reset(new AudioChunk(normal, Audio->samplerate()));

    switch (_options->emphasis) {
      case Options::EMPHASIS_NONE:
        _click_emphasis = _click_normal;
        break;
      case Options::EMPHASIS_ALL:
        _click_normal = _click_emphasis;
        break;
      default:
        break;
    }

    if (_options->volume != 1.0f) {
        _click_emphasis->adjust_volume(_options->volume);
        if (_click_normal != _click_emphasis) {
            _click_normal->adjust_volume(_options->volume);
        }
    }

    if (_options->frequency != 1.0f) {
        _click_emphasis->adjust_frequency(_options->frequency);
        if (_click_normal != _click_emphasis) {
            _click_normal->adjust_frequency(_options->frequency);
        }
    }
}


void Klick::run()
{
    if (!_options->transport_enabled && _options->delay) {
        logv << "waiting for " << _options->delay << " seconds..." << endl;
        usleep((unsigned long)(_options->delay * 1000000));
    }
    logv << "starting metronome..." << endl;
    _metro->start();

    while (true) {
        usleep(100000);

        if (_quit) {
            logv << "terminated" << endl;
            break;
        } else if (!_metro->running()) {
            logv << "end of tempomap reached" << endl;
            break;
        } else if (_audio->is_shutdown()) {
            throw das::exception("shut down by the jack server");
        }
    }
}


void Klick::signal_quit()
{
    _quit = true;
}


string Klick::data_file(const string & path)
{
    fstream f;
    f.open(path.c_str(), ios::in);
    if (f.is_open()) {
        f.close();
        return path;
    }
    return string(DATA_DIR"/") + path;
}