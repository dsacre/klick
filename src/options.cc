/*
 * klick - an advanced metronome for jack
 *
 * Copyright (C) 2007-2009  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "options.hh"
#include "main.hh"

#include <string>
#include <iostream>
#include <boost/tokenizer.hpp>
#include <cstdlib>
#include <unistd.h>

#include "util/lexical_cast.hh"

typedef boost::char_separator<char> char_sep;
typedef boost::tokenizer<char_sep> tokenizer;


Options::Options()
  : auto_connect(false),
    use_osc(false),
    interactive(false),
    follow_transport(false),
    preroll(PREROLL_NONE),
    tempo_multiplier(1.0),
    output_samplerate(48000),
    click_sample(0),
    emphasis_mode(EMPHASIS_MODE_NORMAL),
    volume_emphasis(1.0),
    volume_normal(1.0),
    pitch_emphasis(1.0),
    pitch_normal(1.0),
    transport_enabled(false),
    transport_master(false),
    delay(0.0f),
    verbose(false)
{
}


void Options::print_version()
{
    std::cout << "klick " VERSION << std::endl;
}


void Options::print_usage()
{
    std::cout
        << "Usage:\n"
        << "  klick [options] [bars] [meter] tempo[-tempo2/accel] [pattern]\n"
        << "  klick [options] -f filename\n"
#ifdef ENABLE_TERMINAL
        << "  klick [options] -i\n"
#endif
        << "  klick [options] -j\n"
        << "\n"
        << "Options:\n"
        << "  -f filename       load tempo map from file\n"
        << "  -j                no tempo map, just follow jack transport\n"
        << "  -n name           set jack client name\n"
        << "  -p port,..        jack port(s) to connect to\n"
        << "  -P                automatically connect to hardware ports\n"
#ifdef ENABLE_OSC
        << "  -o port           OSC port to listen on\n"
#endif
#ifdef ENABLE_TERMINAL
        << "  -i                interactive mode\n"
#endif
        << "  -W filename       export click track to audio file\n"
        << "  -r samplerate     sample rate of export (default: 48000)\n"
        << "  -s number         use built-in sounds:\n"
        << "                      0: square wave (default)\n"
        << "                      1: sine wave\n"
        << "                      2: noise\n"
        << "                      3: acoustic bell/click\n"
        << "  -S file[,file]    load sounds from file(s)\n"
        << "  -e                no emphasized beats\n"
        << "  -E                emphasized beats only\n"
        << "  -v mult[,mult]    adjust playback volume (default: 1.0)\n"
        << "  -w mult[,mult]    adjust playback pitch (default: 1.0)\n"
        << "  -t                enable jack transport\n"
        << "  -T                become transport master (implies -t)\n"
        << "  -d seconds        delay before starting playback\n"
        << "  -c bars           pre-roll. use -c 0 for 2 beats\n"
        << "  -l label          start playback at the given label\n"
        << "  -x multiplier     multiply tempo by the given factor\n"
        << "  -h                show this help\n"
        << "\n"
        << "Tempo Map File Format:\n"
        << "  [label:] bars [meter] tempo [pattern] [volume]\n"
        << "  ..." << std::endl;
}


void Options::parse(int argc, char *argv[])
{
    int c;
    char optstring[] = "+f:jn:p:Po:R:iW:r:s:S:eEv:w:tTd:c:l:x:LVh";

    if (argc < 2 || (argc == 2 && std::string(argv[1]) == "--help")) {
        // run with no arguments, print usage
        print_version();
        std::cout << std::endl;
        print_usage();
        throw Exit(EXIT_SUCCESS);
    }

    while ((c = ::getopt(argc, argv, optstring)) != -1) {
        switch (c) {
            case 'f':
                filename = std::string(::optarg);
                break;

            case 'j':
                follow_transport = true;
                break;

            case 'n':
                client_name = std::string(::optarg);
                break;

            case 'p':
              { std::string str(::optarg);
                char_sep sep(",");
                tokenizer tok(str, sep);
                for (tokenizer::iterator i = tok.begin(); i != tok.end(); ++i) {
                    connect_ports.push_back(*i);
                }
              } break;

            case 'P':
                auto_connect = true;
                break;

#ifdef ENABLE_OSC
            case 'o':
                use_osc = true;
                osc_port = ::optarg;
                break;

            case 'R':
                use_osc = true;
                osc_return_port = ::optarg;
                break;
#endif

#ifdef ENABLE_TERMINAL
            case 'i':
                interactive = true;
                break;
#endif

            case 'W':
                output_filename = ::optarg;
                break;

            case 'r':
                output_samplerate = das::lexical_cast<nframes_t>(::optarg, InvalidArgument(c, "samplerate"));
                break;

            case 's':
                click_sample = das::lexical_cast<int>(::optarg, -1);
                if (click_sample < 0 || click_sample > 3) {
                    throw InvalidArgument(c, "click sample");
                }
                break;

            case 'S':
              { std::string str(::optarg);
                char_sep sep(",");
                tokenizer tok(str, sep);
                tokenizer::iterator i = tok.begin();
                click_filename_emphasis = *i;
                if (++i == tok.end()) {
                    click_filename_normal = click_filename_emphasis;
                } else {
                    click_filename_normal = *i;
                    if (++i != tok.end()) throw InvalidArgument(c, "sample file names");
                }
                click_sample = CLICK_SAMPLE_FROM_FILE;
              } break;

            case 'e':
                emphasis_mode = EMPHASIS_MODE_NONE;
                break;

            case 'E':
                emphasis_mode = EMPHASIS_MODE_ALL;
                break;

            case 'v':
              { std::string str(::optarg);
                char_sep sep(",");
                tokenizer tok(str, sep);
                tokenizer::iterator i = tok.begin();
                volume_emphasis = das::lexical_cast<float>(*i, InvalidArgument(c, "volume"));
                i++;
                if (i == tok.end()) {
                    volume_normal = volume_emphasis;
                } else {
                    volume_normal = das::lexical_cast<float>(*i, InvalidArgument(c, "volume"));
                }
              } break;

            case 'w':
              { std::string str(::optarg);
                char_sep sep(",");
                tokenizer tok(str, sep);
                tokenizer::iterator i = tok.begin();
                pitch_emphasis = das::lexical_cast<float>(*i, InvalidArgument(c, "pitch"));
                i++;
                if (i == tok.end()) {
                    pitch_normal = pitch_emphasis;
                } else {
                    pitch_normal = das::lexical_cast<float>(*i, InvalidArgument(c, "pitch"));
                }
              } break;

            case 't':
                transport_enabled = true;
                break;

            case 'T':
                transport_master = true;
                transport_enabled = true;
                break;

            case 'd':
                delay = das::lexical_cast<float>(::optarg, InvalidArgument(c, "delay"));
                if (delay < 0.0f) throw InvalidArgument(c, "delay");
                break;

            case 'c':
                preroll = das::lexical_cast<int>(::optarg, InvalidArgument(c, "pre-roll"));
                if (preroll < 0) throw InvalidArgument(c, "pre-roll");
                break;

            case 'l':
                start_label = std::string(::optarg);
                break;

            case 'x':
                tempo_multiplier = das::lexical_cast<float>(::optarg, InvalidArgument(c, "tempo multiplier"));
                if (tempo_multiplier <= 0) throw InvalidArgument(c, "tempo multiplier");
                break;

            case 'L':
                verbose = true;
                break;

            case 'V':
                print_version();
                throw Exit(EXIT_SUCCESS);

            case 'h':
                print_version();
                std::cout << std::endl;
                print_usage();
                throw Exit(EXIT_SUCCESS);
                break;

            default:
                throw Exit(EXIT_FAILURE);
                break;
        }
    }

    // all remaining arguments make up the "tempomap"
    for (int n = ::optind; n < argc; ++n) {
        cmdline += std::string(argv[n]);
        if (n < argc - 1) cmdline += " ";
    }

    // catch some common command line errors...
    if (!output_filename.empty() && filename.empty() && cmdline.empty()) {
        throw CmdlineError("need a tempo map to export to audio file");
    }

    if (!use_osc) {
        if (follow_transport && (filename.length() || cmdline.length())) {
            throw CmdlineError("can't use explicit tempo or tempo map together with -j option");
        }

        if (filename.length() && cmdline.length()) {
            throw CmdlineError("can't use tempo map from file and command line at the same time");
        }

        if (!follow_transport && !interactive && filename.empty() && cmdline.empty()) {
            throw CmdlineError("no tempo specified");
        }
    }

    if (use_osc && interactive) {
        throw CmdlineError("can't enable OSC and terminal control at the same time, sorry");
    }

    if ((use_osc || interactive) && !output_filename.empty()) {
        throw CmdlineError("can't export to audio file when using OSC or interactive mode");
    }

    // determine metronome type
    type = output_filename.length() ? METRONOME_TYPE_MAP :
           interactive ? METRONOME_TYPE_SIMPLE :
           use_osc ? METRONOME_TYPE_SIMPLE :
           follow_transport ? METRONOME_TYPE_JACK :
           METRONOME_TYPE_MAP;
}
