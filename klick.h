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

#ifndef _KLICK_H
#define _KLICK_H

#include "audio.h"

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>


class TempoMap;
class Metronome;


class Klick
  : boost::noncopyable
{
  public:
    class Options
    {
      public:
        Options()
          : auto_connect(false),
            follow_transport(false),
            click_sample(1),
            no_emphasis(false),
            volume(1.0),
            frequency(1.0),
            transport_enabled(false),
            transport_master(false),
            delay(0.0f),
            preroll(-1),
            tempo_multiplier(1.0),
            verbose(false)
        { }

        void parse(int argc, char *argv[]);
        static void print_version(std::ostream &);
        static void print_usage(std::ostream &);

        std::string client_name;
        std::vector<std::string> connect_ports;
        bool auto_connect;
        bool follow_transport;
        std::string filename;
        std::string settings;
        uint click_sample;
        std::string click_filename_emphasis;
        std::string click_filename_normal;
        bool no_emphasis;
        float volume;
        float frequency;
        bool transport_enabled;
        bool transport_master;
        float delay;
        int preroll;  // -1 := none, 0 := 2 beats
        std::string start_label;
        float tempo_multiplier;
        bool verbose;
    };

    Klick(int argc, char *argv[]);
    ~Klick();

    void load_tempomap();
    void load_samples();

    void run();
    void signal_quit();

  private:
    Options _options;
    AudioDataPtr _click_emphasis;
    AudioDataPtr _click_normal;

    boost::shared_ptr<TempoMap> _map;
    boost::shared_ptr<AudioInterface> _audio;
    boost::shared_ptr<Metronome> _metro;

    volatile bool _quit;
};


#endif // _KLICK_H
