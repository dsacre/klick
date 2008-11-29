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

#ifndef _KLICK_HH
#define _KLICK_HH

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>

#include "audio.hh"


class Options;
class AudioInterface;
class TempoMap;
class Metronome;
class OSCHandler;
class TerminalHandler;
namespace das { class garbage_collector; }


class Klick
  : boost::noncopyable
{
  public:

    Klick(int argc, char *argv[]);
    ~Klick();

    void run();
    void signal_quit();

    boost::shared_ptr<Metronome> metronome() const { return _metro; }

    enum MetronomeType {
        METRONOME_TYPE_SIMPLE,
        METRONOME_TYPE_MAP,
        METRONOME_TYPE_JACK
    };

    void set_metronome(MetronomeType type);

    void set_sound(int n);
    void set_sound_custom(std::string const &, std::string const &);
    int sound() const;
    std::string const & sound_custom_emphasis() const;
    std::string const & sound_custom_normal() const;

    void set_tempomap_filename(std::string const & filename);
    void set_tempomap_preroll(int bars);
    void set_tempomap_multiplier(float mult);
    std::string const & tempomap_filename() const;
    int tempomap_preroll() const;
    float tempomap_multiplier() const;


  private:

    void load_tempomap();
    void load_samples();

    boost::scoped_ptr<Options> _options;

    boost::scoped_ptr<das::garbage_collector> _gc;
    boost::shared_ptr<AudioInterface> _audio;

    AudioChunkPtr _click_emphasis;
    AudioChunkPtr _click_normal;

    boost::shared_ptr<TempoMap> _map;

    boost::shared_ptr<OSCHandler> _osc;
    boost::shared_ptr<TerminalHandler> _term;

    boost::shared_ptr<Metronome> _metro;

    volatile bool _quit;
};


#endif // _KLICK_HH
