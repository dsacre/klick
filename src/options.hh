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

#ifndef _OPTIONS_HH
#define _OPTIONS_HH

#include <string>
#include <iostream>
#include <vector>
#include <stdexcept>

#include "util/string.hh"


class Options
{
  public:
    Options();

    void parse(int argc, char *argv[]);

    std::string client_name;
    std::vector<std::string> connect_ports;
    bool auto_connect;
    bool use_osc;
    std::string osc_port;
    std::string osc_return_port;
    bool interactive;
    bool follow_transport;
    std::string filename;
    std::string cmdline;

    static int const CLICK_SAMPLE_FROM_FILE = -1;
    int click_sample;
    std::string click_filename_emphasis;
    std::string click_filename_normal;

    enum {
        EMPHASIS_NORMAL,
        EMPHASIS_NONE,
        EMPHASIS_ALL
    } emphasis;

    float volume_emphasis, volume_normal;
    float pitch_emphasis, pitch_normal;
    bool transport_enabled;
    bool transport_master;
    float delay;

    static int const PREROLL_NONE = -1,
                     PREROLL_2_BEATS = 0;
    int preroll;
    std::string start_label;
    float tempo_multiplier;
    bool verbose;

  private:
    struct CmdlineError
      : public std::runtime_error
    {
        CmdlineError(std::string const & w)
          : std::runtime_error(w) { }
    };

    struct InvalidArgument
      : public std::runtime_error
    {
        InvalidArgument(char a, std::string const & w)
          : std::runtime_error(das::make_string() << "invalid argument to -" << a << " (" << w << ")")
        {
        }
    };

    void print_version(std::ostream & = std::cout);
    void print_usage(std::ostream & = std::cout);
};


#endif // _OPTIONS_HH
