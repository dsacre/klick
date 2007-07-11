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
#include "audio.h"
#include "tempomap.h"
#include "metronome.h"
#include "click_data.h"
#include "util.h"

#include <string>
#include <iostream>
#include <sstream>
#include <cstring>
#include <cstdlib>

#include <unistd.h>

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
    _audio.reset(new AudioInterface(_options.client_name, _options.connect_ports));

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
    if (_options.filename.empty() && _options.settings.empty()) {
        throw "no tempo specified";
    } else {
        if (!_options.filename.empty()) {
            _map.reset(new TempoMapFile(_options.filename));
        } else {
            _map.reset(new TempoMapCmdline(_options.settings));
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
    if (_options.click_sample == 0) {
        logv << "loading samples from file:" << endl
               << "  emphasis: " << _options.click_filename_emphasis << endl
               << "  normal: " << _options.click_filename_normal << endl;
    }

    switch (_options.click_sample) {
      case 0:
        _click_normal.reset(new AudioData(_options.click_filename_normal, Audio->samplerate()));
        break;
      case 1:
        _click_normal.reset(new AudioData(CLICK_1_NORMAL_DATA, Audio->samplerate()));
        break;
      case 2:
        _click_normal.reset(new AudioData(CLICK_2_NORMAL_DATA, Audio->samplerate()));
        break;
    }

    if (!_options.no_emphasis) {
        switch (_options.click_sample) {
          case 0:
            _click_emphasis.reset(new AudioData(_options.click_filename_emphasis, Audio->samplerate()));
            break;
          case 1:
            _click_emphasis.reset(new AudioData(CLICK_1_EMPHASIS_DATA, Audio->samplerate()));
            break;
          case 2:
            _click_emphasis.reset(new AudioData(CLICK_2_EMPHASIS_DATA, Audio->samplerate()));
            break;
        }
    } else {
        _click_emphasis.reset(new AudioData(*_click_normal));
    }

    if (_options.volume != 1.0f) {
        _click_emphasis->adjust_volume(_options.volume);
        _click_normal->adjust_volume(_options.volume);
    }

    if (_options.frequency != 1.0) {
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


void Klick::Options::print_version(ostream & out)
{
    out << "klick " VERSION << endl << endl;
}

void Klick::Options::print_usage(ostream & out)
{
    out << "Usage:" << endl
        << "  klick [ options ] [meter] tempo[-tempo2/accel] [pattern]" << endl
        << "  klick [ options ] -f tempomap-file" << endl
        << "  klick [ options ] -j" << endl
        << endl
        << "Options:" << endl
        <<  "  -f <filename>        load tempomap from file" << endl
        <<  "  -j                   no tempomap, just follow jack transport" << endl
        <<  "  -n <name>            jack client name" << endl
        <<  "  -p <port,..>         jack port(s) to connect to" << endl
        <<  "  -s <number>          use built-in click sample 1 (default) or 2" << endl
        <<  "  -S <emphasis,normal> use the given files as click samples" << endl
        <<  "  -e                   no emphasized beats" << endl
        <<  "  -v <multiplier>      adjust playback volume" << endl
        <<  "  -w <multiplier>      adjust playback frequency" << endl
        <<  "  -t                   enable jack transport" << endl
        <<  "  -T                   become transport master (implies -t)" << endl
        <<  "  -d <seconds>         delay before starting playback" << endl
        <<  "  -c <bars>            pre-roll. use -c 0 for 2 beats" << endl
        <<  "  -l <label>           start playback at the given label" << endl
        <<  "  -x <multiplier>      multiply tempo by the given factor" << endl
        <<  "  -V                   verbose output" << endl
        <<  "  -h                   show this help" << endl
        << endl
        << "Tempomap file format:" << endl
        << "  [label:] bars [meter] tempo[-tempo2] [pattern] [volume]" << endl
        << "  ..." << endl;
}

void Klick::Options::parse(int argc, char *argv[])
{
    int c;
    char optstring[] = "-f:jn:p:s:S:ev:w:tTd:c:l:x:Vh";
    char *end;

    while ((c = getopt(argc, argv, optstring)) != -1)
    {
        switch (c)
        {
            case 1:
                if (!settings.empty()) settings += " ";
                settings += string(::optarg);
                break;
            case 'f':
                filename = string(::optarg);
                break;
            case 'j':
                follow_transport = true;
                break;
            case 'n':
                client_name = string(::optarg);
                break;
            case 'p':
              { char *p = strtok(::optarg, ",");  // yuck
                while (p) {
                    connect_ports.push_back(string(p));
                    p = strtok(NULL, ",");
                }
              } break;
            case 's':
                click_sample = strtoul(::optarg, &end, 10);
                if (*end != '\0' || click_sample < 1 || click_sample > 2) {
                    throw "invalid click sample number";
                }
                break;
            case 'S':
              { string s(::optarg);
                string::size_type comma = s.find(',');
                if (comma != string::npos && comma == s.rfind(',')) {
                    click_filename_emphasis = string(s, 0, comma);
                    click_filename_normal = string(s, comma + 1, s.length() - comma);
                } else {
                    throw "invalid click sample file names";
                }
                click_sample = 0;
              } break;
            case 'e':
                no_emphasis = true;
                break;
            case 'v':
                volume = strtof(::optarg, &end);
                if (*end != '\0') throw "invalid volume";
                break;
            case 'w':
                frequency = strtof(::optarg, &end);
                if (*end != '\0') throw "invalid frequency multiplier";
                break;
            case 't':
                transport_enabled = true;
                break;
            case 'T':
                transport_master = true;
                transport_enabled = true;
                break;
            case 'd':
                delay = strtod(::optarg, &end);
                if (*end != '\0' || delay < 0.0f) throw "invalid delay";
                break;
            case 'c':
                preroll = strtoul(::optarg, &end, 10);
                if (*end != '\0') throw "invalid pre-roll";
                break;
            case 'l':
                start_label = string(::optarg);
                break;
            case 'x':
                tempo_multiplier = strtof(::optarg, &end);
                if (*end != '\0') throw "invalid tempo multiplier";
                break;
            case 'V':
                verbose = true;
                break;
            case 'h':
                print_version(cout);
                print_usage(cout);
                exit(EXIT_SUCCESS);
                break;
            default:
                print_usage(cerr);
                exit(EXIT_FAILURE);
                break;
        }
    }
}
