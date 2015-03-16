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

#ifndef KLICK_METRONOME_MAP_HH
#define KLICK_METRONOME_MAP_HH

#include "metronome.hh"
#include "tempomap.hh"
#include "position.hh"

#include <string>

/*
 * plays a click track using a predefined tempomap
 */
class MetronomeMap
  : public Metronome
{
  public:
    MetronomeMap(
        AudioInterface & audio,
        TempoMapConstPtr tempomap,
        float tempo_multiplier,
        bool transport,
        int preroll,
        std::string const & start_label
    );
    virtual ~MetronomeMap();

    virtual void do_start();
    virtual void do_stop();

    bool running() const;

    nframes_t current_frame() const;
    nframes_t total_frames() const;

    virtual void process_callback(sample_t *, nframes_t);
    virtual void timebase_callback(position_t *);

  private:
    static int const TICKS_PER_BEAT = 1920;

    // transport position
    nframes_t _frame;

    // position in tempomap
    Position _pos;

    bool _transport_enabled;
};


#endif // KLICK_METRONOME_MAP_HH
