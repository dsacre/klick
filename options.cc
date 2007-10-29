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

#include "options.h"

#include <string>
#include <iostream>
#include <boost/tokenizer.hpp>
#include <unistd.h>

#include "util.h"

using namespace std;
typedef boost::char_separator<char> char_sep;
typedef boost::tokenizer<char_sep> tokenizer;


Options::Options()
  : auto_connect(false),
    follow_transport(false),
    click_sample(1),
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
    out << "Usage:" << endl
        << "  klick [ options ] [meter] tempo[-tempo2/accel] [pattern]" << endl
        << "  klick [ options ] -f filename" << endl
        << "  klick [ options ] -j" << endl
        << endl
        << "Options:" << endl
        <<  "  -f filename       load tempomap from file" << endl
        <<  "  -j                no tempomap, just follow jack transport" << endl
        <<  "  -n name           set jack client name" << endl
        <<  "  -p port,..        jack port(s) to connect to" << endl
        <<  "  -P                automatically connect to hardware ports" << endl
        <<  "  -s number         use built-in sounds 1 (sine), 2 (noise) or 3 (bell/click)" << endl
        <<  "  -S file[,file]    load sounds from file(s)" << endl
        <<  "  -e                no emphasized beats" << endl
        <<  "  -E                emphasized beats only" << endl
        <<  "  -v multiplier     adjust playback volume" << endl
        <<  "  -w multiplier     adjust playback frequency" << endl
        <<  "  -t                enable jack transport" << endl
        <<  "  -T                become transport master (implies -t)" << endl
        <<  "  -d seconds        delay before starting playback" << endl
        <<  "  -c bars           pre-roll. use -c 0 for 2 beats" << endl
        <<  "  -l label          start playback at the given label" << endl
        <<  "  -x multiplier     multiply tempo by the given factor" << endl
        <<  "  -h                show this help" << endl
        << endl
        << "Tempomap file format:" << endl
        << "  [label:] bars [meter] tempo[-tempo2] [pattern] [volume]" << endl
        << "  ..." << endl;
}


void Options::parse(int argc, char *argv[])
{
    int c;
    char optstring[] = "+f:jn:p:Ps:S:eEv:w:tTd:c:l:x:Vh";
    char *end;

    while ((c = getopt(argc, argv, optstring)) != -1)
    {
        switch (c)
        {
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
                if (*end != '\0' || click_sample < 1 || click_sample > 3) {
                    throw "invalid click sample number";
                }
                break;

            case 'S':
              { char_sep sep(",");
                string str(::optarg);
                tokenizer tok(str, sep);
                tokenizer::iterator i = tok.begin();
                click_filename_emphasis = *i++;
                switch (count_iter(tok)) {
                  case 1:
                    click_filename_normal = click_filename_emphasis;
                    break;
                  case 2:
                    click_filename_normal = *i;
                    break;
                  default:
                    throw "too many sample file names";
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

    for (int n = ::optind; n < argc; n++) {
        cmdline += string(argv[n]);
        cmdline += " ";
    }
}
