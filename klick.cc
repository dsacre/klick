/*
 * klick - an advanced metronome for jack
 *
 * Copyright (C) 2007  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "klick.h"
#include "options.h"
#include "audio_interface.h"
#include "audio_chunk.h"
#include "tempomap.h"
#include "metronome_map.h"
#include "metronome_jack.h"
#include "click_data.h"

#include <string>
#include <iostream>
#include <sstream>

#include "util.h"

using namespace std;


Klick::Klick(int argc, char *argv[])
  : _quit(false)
{
    _options.parse(argc, argv);
    logv.enable(_options.verbose);

    if (!_options.follow_transport) {
        load_tempomap();
    }

    // determine client name (if not specified)
    if (_options.client_name.empty()) {
        ostringstream os;
        os << "klick-" << getpid();
        _options.client_name = os.str();
    }
    logv << "jack client name: " << _options.client_name << endl;

    // setup jack
    _audio.reset(new AudioInterface(_options.client_name, _options.connect_ports, _options.auto_connect));

    if (_options.transport_master) {
        logv << "jack transport master enabled" << endl;
    } else if (_options.transport_enabled) {
        logv << "jack transport enabled" << endl;
    }

    load_samples();

    // create metronome object
    if (_options.follow_transport) {
        _metro.reset(new MetronomeJack(_click_emphasis, _click_normal));
    } else {
        _metro.reset(new MetronomeMap(_map, _options.tempo_multiplier,
                _options.transport_enabled, _options.transport_master,
                _options.preroll, _options.start_label, _click_emphasis, _click_normal));
    }
}


Klick::~Klick()
{
}


void Klick::load_tempomap()
{
    if (_options.filename.empty() && _options.cmdline.empty()) {
        throw "no tempo specified";
    } else {
        if (!_options.filename.empty()) {
            _map = TempoMap::new_from_file(_options.filename);
        } else {
            _map = TempoMap::new_from_cmdline(_options.cmdline);
        }
    }

    logv << "tempomap:" << endl
           << indent(_map->dump(), 2);

    // make sure the start label exists
    if (!_options.start_label.empty()) {
        if (_map->entry(_options.start_label)) {
            logv << "starting at label: " << _options.start_label << endl;
        } else {
            ostringstream os;
            os << "label '" << _options.start_label << "' not found in tempomap";
            throw os.str();
        }
    }
}


void Klick::load_samples()
{
    if (_options.click_sample == Options::CLICK_SAMPLE_FROM_FILE) {
        logv << "loading samples from file:" << endl
               << "  emphasis: " << _options.click_filename_emphasis << endl
               << "  normal: " << _options.click_filename_normal << endl;
    }

    const AudioChunkStaticBase *data_emphasis, *data_normal;

    switch (_options.click_sample) {
      case 1:
        data_emphasis = &CLICK_1_EMPHASIS_DATA;
        data_normal = &CLICK_1_NORMAL_DATA;
        break;
      case 2:
        data_emphasis = &CLICK_2_EMPHASIS_DATA;
        data_normal = &CLICK_2_NORMAL_DATA;
        break;
      case 3:
        data_emphasis = &CLICK_3_EMPHASIS_DATA;
        data_normal = &CLICK_3_NORMAL_DATA;
        break;
      default:
        data_emphasis = NULL;
        data_normal = NULL;
        break;
    }

    if (data_normal) {
        _click_emphasis = data_emphasis->load();
        _click_normal = data_normal->load();
    } else {
        _click_emphasis.reset(new AudioChunk(_options.click_filename_emphasis, Audio->samplerate()));
        _click_normal.reset(new AudioChunk(_options.click_filename_normal, Audio->samplerate()));
    }

    switch (_options.emphasis) {
      case Options::EMPHASIS_NONE:
        _click_emphasis = _click_normal;
        break;
      case Options::EMPHASIS_ALL:
        _click_normal = _click_emphasis;
        break;
      default:
        break;
    }

    if (_options.volume != 1.0f) {
        _click_emphasis->adjust_volume(_options.volume);
        _click_normal->adjust_volume(_options.volume);
    }

    if (_options.frequency != 1.0f) {
        _click_emphasis->adjust_frequency(_options.frequency);
        _click_normal->adjust_frequency(_options.frequency);
    }
}


void Klick::run()
{
    if (!_options.transport_enabled && _options.delay) {
        logv << "waiting for " << _options.delay << " seconds..." << endl;
        usleep((unsigned long)(_options.delay * 1000000));
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
            throw "shut down by the jack server";
        }
    }
}


void Klick::signal_quit()
{
    _quit = true;
}
