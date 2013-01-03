/*
 * klick - an advanced metronome for jack
 *
 * Copyright (C) 2007-2013  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef KLICK_OPTIONS_HH
#define KLICK_OPTIONS_HH

#include "audio.hh"

#include <string>
#include <vector>
#include <stdexcept>

#include "util/string.hh"


class Options
{
  public:
    Options();

    void parse(int argc, char *argv[]);

    // jack options
    std::string client_name;
    std::vector<std::string> connect_ports;
    bool auto_connect;

    // osc options
    bool use_osc;
    std::string osc_port;
    std::string osc_return_port;

    // mode options
    bool interactive;
    bool follow_transport;

    enum MetronomeType {
        METRONOME_TYPE_SIMPLE,
        METRONOME_TYPE_MAP,
        METRONOME_TYPE_JACK
    };
    MetronomeType type;

    // tempomap
    std::string filename;
    std::string cmdline;

    static int const PREROLL_NONE = -1;
    static int const PREROLL_2_BEATS = 0;
    int preroll;
    std::string start_label;
    float tempo_multiplier;

    // export settings
    std::string output_filename;
    nframes_t output_samplerate;

    // sound settings
    static int const CLICK_SAMPLE_FROM_FILE = -2;
    static int const CLICK_SAMPLE_SILENT = -1;
    int click_sample;
    std::string click_filename_emphasis;
    std::string click_filename_normal;

    enum EmphasisMode {
        EMPHASIS_MODE_NORMAL,
        EMPHASIS_MODE_NONE,
        EMPHASIS_MODE_ALL
    };
    EmphasisMode emphasis_mode;

    float volume_emphasis;
    float volume_normal;
    float pitch_emphasis;
    float pitch_normal;

    // jack transport options
    bool transport_enabled;
    bool transport_master;

    float delay;
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

    void print_version();
    void print_usage();
};


#endif // KLICK_OPTIONS_HH
