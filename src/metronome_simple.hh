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

#ifndef KLICK_METRONOME_SIMPLE_HH
#define KLICK_METRONOME_SIMPLE_HH

#include "audio.hh"
#include "metronome.hh"
#include "tempomap.hh"

#include <vector>
#include <deque>


class MetronomeSimple
  : public Metronome
{
  public:

    MetronomeSimple(AudioInterface & audio, TempoMap::Entry const * params = NULL);
    virtual ~MetronomeSimple();

    virtual bool running() const { return true; }

    void set_tempo(float);
    void set_tempo_increment(float);
    void set_tempo_start(float);
    void set_tempo_limit(float);
    void set_meter(int, int);
    void set_pattern(TempoMap::Pattern const &);

    void set_all(TempoMap::Entry const & params);

    void tap(double now);
    void tap();

    float tempo() const { return _tempo; }
    float tempo_increment() const { return _tempo_increment; }
    float tempo_start() const { return _tempo_start; }
    float tempo_limit() const { return _tempo_limit; }
    float current_tempo() const { return _current_tempo; }
    int beats() const { return _beats; }
    int denom() const { return _denom; }
    TempoMap::Pattern const & pattern() const { return _pattern; }

    virtual void do_start();
    virtual void do_stop();

    virtual void process_callback(sample_t *, nframes_t);

  private:

    static int const MAX_TAPS = 5;
    static float constexpr MAX_TAP_AGE = 3.0f;
    static float constexpr TAP_DIFF = 0.2f;

    float _tempo;
    float _tempo_increment;
    float _tempo_start;
    float _tempo_limit;
    float _current_tempo;
    int _beats;
    int _denom;
    TempoMap::Pattern _pattern;

    nframes_t _frame;
    nframes_t _next;
    int _beat;

    std::deque<double> _taps;
    nframes_t _prev;
    bool _tapped;
};


#endif // KLICK_METRONOME_SIMPLE_HH
