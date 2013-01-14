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

#ifndef KLICK_POSITION_HH
#define KLICK_POSITION_HH

#include "audio.hh"
#include "tempomap.hh"

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

    Position(TempoMapConstPtr tempomap, float_frames_t samplerate, float multiplier);

    void set_start_label(std::string const & start_label);
    void add_preroll(int nbars);

    // move to frame
    void locate(nframes_t f);
    // move position one tick forward
    void advance();

    // distance from previous (current) tick to the next
    float_frames_t dist_to_next() const;
    // frame of next tick
    float_frames_t next_frame() const { return frame() + dist_to_next(); }

    // get current tick
    const Tick tick() const;
    // end of tempomap reached?
    bool end() const { return _end; }

    float_frames_t frame() const { return _frame; }
    int entry() const { return _entry; }
    int bar() const { return _bar; }
    int beat() const { return _beat; }
    int bar_total() const { return _bar_total; }
    int beat_total() const { return _beat_total; }

    // current tempomap entry
    TempoMap::Entry const & current_entry() const {
        return (*_tempomap)[_entry];
    }

    // total length of tempomap
    float_frames_t total_frames() const {
        return _start_frames.back();
    }

  private:
    // reset, locate at start of tempomap
    void reset();
    void calculate_entry_positions();

    // calculate length of entry or beat(s)
    float_frames_t frame_dist(TempoMap::Entry const & e, int start, int end) const;

    // frame position of current tick
    float_frames_t _frame;

    // current position in tempomap
    int _entry;
    int _bar;
    int _beat;

    // current bar/beat number (including previous entries)
    int _bar_total;
    int _beat_total;

    bool _init;
    bool _end;

    TempoMapConstPtr _tempomap;
    float_frames_t _samplerate;
    float _multiplier;

    // start of each entry, measured in frames, bars, and beats from the start of the tempomap
    std::vector<float_frames_t> _start_frames;
    std::vector<int> _start_bars;
    std::vector<int> _start_beats;
};


#endif // KLICK_POSITION_HH
