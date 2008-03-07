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

#include "options.hh"
#include "main.hh"

#include <string>
#include <iostream>
#include <boost/tokenizer.hpp>
#include <cstdlib>
#include <unistd.h>

using namespace std;
typedef boost::char_separator<char> char_sep;
typedef boost::tokenizer<char_sep> tokenizer;


Options::Options()
  : auto_connect(false),
    follow_transport(false),
    click_sample(0),
    emphasis(EMPHASIS_NORMAL),
    volume(1.0),
    frequency(1.0),
    transport_enabled(false),
    transport_master(false),
    delay(0.0f),
    preroll(PREROLL_NONE),
    tempo_multiplier(1.0),
    verbose(false)
{
}


void Options::print_version(ostream & out)
{
    out << "klick " VERSION << endl << endl;
}


void Options::print_usage(ostream & out)
{
    out << "Usage:\n"
        << "  klick [ options ] [meter] tempo[-tempo2/accel] [pattern]\n"
        << "  klick [ options ] -f filename\n"
        << "  klick [ options ] -j\n"
        << "\n"
        << "Options:\n"
        << "  -f filename       load tempomap from file\n"
        << "  -j                no tempomap, just follow jack transport\n"
        << "  -n name           set jack client name\n"
        << "  -p port,..        jack port(s) to connect to\n"
        << "  -P                automatically connect to hardware ports\n"
        << "  -s number         use built-in sounds:\n"
        << "                      0: square wave (default)\n"
        << "                      1: sine wave\n"
        << "                      2: noise\n"
        << "                      3: bell/click\n"
        << "  -S file[,file]    load sounds from file(s)\n"
        << "  -e                no emphasized beats\n"
        << "  -E                emphasized beats only\n"
        << "  -v multiplier     adjust playback volume\n"
        << "  -w multiplier     adjust playback pitch\n"
        << "  -t                enable jack transport\n"
        << "  -T                become transport master (implies -t)\n"
        << "  -d seconds        delay before starting playback\n"
        << "  -c bars           pre-roll. use -c 0 for 2 beats\n"
        << "  -l label          start playback at the given label\n"
        << "  -x multiplier     multiply tempo by the given factor\n"
        << "  -h                show this help\n"
        << "\n"
        << "Tempomap file format:\n"
        << "  [label:] bars [meter] tempo [pattern] [volume]\n"
        << "  ..." << endl;
}


void Options::parse(int argc, char *argv[])
{
    int c;
    char optstring[] = "+f:jn:p:Ps:S:eEv:w:tTd:c:l:x:Vh";
    char *end;

    if (argc < 2) {
        // run with no arguments, print usage
        print_version();
        print_usage();
        throw Exit(EXIT_SUCCESS);
    }

    while ((c = getopt(argc, argv, optstring)) != -1) {
        switch (c) {
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
              { char_sep sep(",");
                tokenizer tok(string(::optarg), sep);
                for (tokenizer::iterator i = tok.begin(); i != tok.end(); ++i) {
                    connect_ports.push_back(*i);
                }
              } break;

            case 'P':
                auto_connect = true;
                break;

            case 's':
                click_sample = strtoul(::optarg, &end, 10);
                if (*end != '\0' || click_sample < 0 || click_sample > 3) {
                    throw InvalidArgument("click sample");
                }
                break;

            case 'S':
              { char_sep sep(",");
                string str(::optarg);
                tokenizer tok(str, sep);
                tokenizer::iterator i = tok.begin();
                click_filename_emphasis = *i;
                switch (distance(tok.begin(), tok.end())) {
                  case 1:
                    click_filename_normal = click_filename_emphasis;
                    break;
                  case 2:
                    click_filename_normal = *(++i);
                    break;
                  default:
                    throw InvalidArgument("sample file names");
                }
                click_sample = CLICK_SAMPLE_FROM_FILE;
              } break;

            case 'e':
                emphasis = EMPHASIS_NONE;
                break;

            case 'E':
                emphasis = EMPHASIS_ALL;
                break;

            case 'v':
                volume = strtof(::optarg, &end);
                if (*end != '\0') throw InvalidArgument("volume");
                break;

            case 'w':
                frequency = strtof(::optarg, &end);
                if (*end != '\0') throw InvalidArgument("frequency");
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
                if (*end != '\0' || delay < 0.0f) throw InvalidArgument("delay");
                break;

            case 'c':
                preroll = strtoul(::optarg, &end, 10);
                if (*end != '\0') throw InvalidArgument("pre-roll");
                break;

            case 'l':
                start_label = string(::optarg);
                break;

            case 'x':
                tempo_multiplier = strtof(::optarg, &end);
                if (*end != '\0' || tempo_multiplier <= 0.0f) {
                    throw InvalidArgument("tempo multiplier");
                }
                break;

            case 'V':
                verbose = true;
                break;

            case 'h':
                print_version();
                print_usage();
                throw Exit(EXIT_SUCCESS);
                break;

            default:
                print_usage(cerr);
                throw Exit(EXIT_FAILURE);
                break;
        }
    }

    // all remaining arguments make up the "tempomap"
    for (int n = ::optind; n < argc; n++) {
        cmdline += string(argv[n]);
        if (n < argc - 1) cmdline += " ";
    }

    if (follow_transport && (filename.length() || cmdline.length())) {
        throw CmdlineError("can't use tempo/tempomap together with -j option");
    }

    if (filename.length() && cmdline.length()) {
        throw CmdlineError("can't use tempomap from file and command line at the same time");
    }

    if ((!follow_transport) && filename.empty() && cmdline.empty()) {
        throw CmdlineError("no tempo specified");
    }
}
