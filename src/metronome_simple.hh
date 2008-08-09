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

#ifndef _METRONOME_SIMPLE_HH
#define _METRONOME_SIMPLE_HH

#include "audio.hh"
#include "metronome.hh"
#include "tempomap.hh"

#include <vector>
#include <deque>


class MetronomeSimple
  : public Metronome
{
  public:

    MetronomeSimple(AudioInterface & audio);
    MetronomeSimple(AudioInterface & audio, TempoMap::Entry const & params);
    virtual ~MetronomeSimple();

    virtual bool running() const { return true; }

    void set_tempo(float);
    void set_meter(int, int);
    void set_pattern(TempoMap::Pattern const &);

    void set_all(TempoMap::Entry const & params);

    void tap(double now);
    void tap();

    float tempo() const { return _tempo; }
    int beats() const { return _beats; }
    int denom() const { return _denom; }

    virtual void set_active(bool);

  protected:

    virtual void process_callback(sample_t *, nframes_t);

  private:

    static int const MAX_TAPS = 5;
    static float const MAX_TAP_AGE = 3.0;
    static float const TAP_DIFF = 0.2;

    float _tempo;
    int _beats, _denom;
    TempoMap::Pattern _pattern;

    nframes_t _frame;
    nframes_t _next;
    int _beat;

    std::deque<double> _taps;
    nframes_t _prev;
    bool _tapped;
};


#endif // _METRONOME_SIMPLE_HH
