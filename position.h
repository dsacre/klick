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

#ifndef _POSITION_H
#define _POSITION_H

#include "audio.h"
#include "tempomap.h"

#include <string>
#include <vector>


/*
 * keeps track of the position in the tempomap
 */

class Position
{
  public:
    typedef double float_frames_t;

    struct Tick {
        nframes_t frame;
        TempoMap::BeatType type;
        float volume;
    };

    Position(TempoMapConstPtr tempomap, float multiplier);

    void set_start_label(const std::string & start_label);
    void add_preroll(int nbars);

    // move to frame
    void locate(nframes_t f);

    // distance from previous (current) tick to the next
    float_frames_t dist_to_next() const;
    // frame of next tick
    float_frames_t next_frame() const { return frame() + dist_to_next(); }
    // move position one tick forward
    void advance();

    // get current tick
    const Tick tick() const;
    // end of tempomap reached?
    bool end() const { return _end; }

    float_frames_t frame() const { return _frame; }
    int entry() const { return _entry; }
    int bar() const { return _bar; }
    int beat() const { return _beat; }
    int bar_total() const { return _bar_total; }

    // current tempomap entry
    const TempoMap::Entry & map_entry() const {
        return (*_tempomap)[_entry];
    }

  private:
    // reset, locate at start of tempomap
    void reset();

    // calculate length of entry or beat(s)
    float_frames_t frame_dist(const TempoMap::Entry & e,
        int bar_start = 0, int beat_start = 0,
        int bar_end = -2 /* last bar */, int beat_end = 0) const;

    float_frames_t _frame;      // frame position of current tick
    int _entry, _bar, _beat;    // current position in tempomap
    int _bar_total;             // current bar number (including previous entries)
    bool _init, _end;

    TempoMapConstPtr _tempomap;
    float _multiplier;

    std::vector<float_frames_t> _start_frames;
    std::vector<int> _start_bars;
};


#endif // _POSITION_H
