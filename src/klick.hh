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

#ifndef KLICK_KLICK_HH
#define KLICK_KLICK_HH

#include <string>
#include <csignal>
#include <memory>
#include <tuple>
#include <boost/noncopyable.hpp>

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

    std::shared_ptr<Metronome> metronome() const { return _metro; }

    void set_metronome(Options::MetronomeType type);

    void set_sound(int n);
    void set_sound_custom(std::string const &, std::string const &);
    void set_sound_volume(float, float);
    void set_sound_pitch(float, float);

    int sound() const {
        return _options->click_sample;
    }
    std::tuple<std::string, std::string> sound_custom() const {
        return std::make_tuple(_options->click_filename_emphasis, _options->click_filename_normal);
    }
    std::tuple<float, float> sound_volume() const {
        return std::make_tuple(_options->volume_emphasis, _options->volume_normal);
    }
    std::tuple<float, float> sound_pitch() const {
        return std::make_tuple(_options->pitch_emphasis, _options->pitch_normal);
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

    std::tuple<std::string, std::string> sample_filenames(int n, Options::EmphasisMode emphasis_mode);
    AudioChunkPtr load_sample(std::string const & filename, float volume, float pitch);

    void run_jack();
    void run_sndfile();


    std::unique_ptr<Options> _options;
    std::unique_ptr<das::garbage_collector> _gc;

    std::unique_ptr<AudioInterface> _audio;

    AudioChunkPtr _click_emphasis;
    AudioChunkPtr _click_normal;

    std::shared_ptr<TempoMap> _map;

    std::unique_ptr<OSCHandler> _osc;
    std::unique_ptr<TerminalHandler> _term;

    std::shared_ptr<Metronome> _metro;

    volatile std::sig_atomic_t _quit;
};


#endif // KLICK_KLICK_HH
