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

#ifndef _KLICK_HH
#define _KLICK_HH

#include <string>
#include <csignal>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/tuple/tuple.hpp>

#include "audio.hh"
#include "options.hh"


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

    void set_metronome(Options::MetronomeType type);

    void set_sound(int n);
    void set_sound_custom(std::string const &, std::string const &);
    void set_sound_volume(float, float);
    void set_sound_pitch(float, float);

    int sound() const {
        return _options->click_sample;
    }
    boost::tuple<std::string, std::string> sound_custom() const {
        return boost::make_tuple(_options->click_filename_emphasis, _options->click_filename_normal);
    }
    boost::tuple<float, float> sound_volume() const {
        return boost::make_tuple(_options->volume_emphasis, _options->volume_normal);
    }
    boost::tuple<float, float> sound_pitch() const {
        return boost::make_tuple(_options->pitch_emphasis, _options->pitch_normal);
    }

    void set_tempomap_filename(std::string const & filename);
    void set_tempomap_preroll(int bars);
    void set_tempomap_multiplier(float mult);

    std::string const & tempomap_filename() const { return _options->filename; }
    int tempomap_preroll() const { return _options->preroll; }
    float tempomap_multiplier() const { return _options->tempo_multiplier; }


  private:

    void setup_jack();
    void setup_sndfile();
    void load_tempomap();
    void load_samples();
    void load_metronome();

    boost::tuple<std::string, std::string> sample_filenames(int n, Options::EmphasisMode emphasis_mode);
    AudioChunkPtr load_sample(std::string const & filename, float volume, float pitch);

    void run_jack();
    void run_sndfile();


    boost::scoped_ptr<Options> _options;
    boost::scoped_ptr<das::garbage_collector> _gc;

    boost::shared_ptr<AudioInterface> _audio;

    AudioChunkPtr _click_emphasis;
    AudioChunkPtr _click_normal;

    boost::shared_ptr<TempoMap> _map;

    boost::shared_ptr<OSCHandler> _osc;
    boost::shared_ptr<TerminalHandler> _term;

    boost::shared_ptr<Metronome> _metro;

    volatile std::sig_atomic_t _quit;
};


#endif // _KLICK_HH
