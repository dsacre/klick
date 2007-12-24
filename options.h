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

#ifndef _OPTIONS_H
#define _OPTIONS_H

#include <string>
#include <iostream>
#include <vector>
#include "util.h"


class Options
{
  public:
    Options();

    void parse(int argc, char *argv[]);

    std::string client_name;
    std::vector<std::string> connect_ports;
    bool auto_connect;

    bool follow_transport;
    std::string filename;
    std::string cmdline;

    static const int CLICK_SAMPLE_FROM_FILE = 0;
    int click_sample;
    std::string click_filename_emphasis;
    std::string click_filename_normal;

    enum { EMPHASIS_NORMAL,
           EMPHASIS_NONE,
           EMPHASIS_ALL } emphasis;
    float volume;
    float frequency;
    bool transport_enabled;
    bool transport_master;
    float delay;

    static const int PREROLL_NONE = -1,
                     PREROLL_2_BEATS = 0;
    int preroll;
    std::string start_label;
    float tempo_multiplier;
    bool verbose;

  private:
    struct CmdlineError : public das::exception {
        CmdlineError(const std::string & w)
          : das::exception(w) { }
    };

    struct InvalidArgument : public das::exception {
        InvalidArgument(const std::string & w)
          : das::exception("invalid argument (" + w + ")") { }
    };

    void print_version(std::ostream & = std::cout);
    void print_usage(std::ostream & = std::cout);

    std::vector<std::string> split_arguments(const std::string & str);
};


#endif // _OPTIONS_H
