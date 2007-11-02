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

#ifndef _METRONOME_MAP_H
#define _METRONOME_MAP_H

#include "audio.h"
#include "metronome.h"
#include "tempomap.h"

#include <string>

/*
 * plays a click track using a predefined tempomap
 */

class MetronomeMap
  : public Metronome,
    protected AudioInterface::TimebaseCallback
{
  public:
    MetronomeMap(TempoMapConstPtr tempomap,
                 float tempo_multiplier,
                 bool transport, bool master,
                 int preroll, const std::string & start_label,
                 AudioChunkConstPtr emphasis, AudioChunkConstPtr normal);
    virtual ~MetronomeMap();

    void start();
    bool running() const;

  protected:
    static const double TICKS_PER_BEAT = 1920.0;

    typedef double float_frames_t;

    struct Tick {
        nframes_t frame;
        TempoMap::BeatType type;
        float volume;
    };

    /*
     * keeps track of the current position in the tempomap
     */
    class Position
    {
      public:
        Position(TempoMapConstPtr tempomap, float multiplier);

        void set_start_label(const std::string & start_label);
        void add_preroll(int nbars);

        // move to frame
        void locate(nframes_t frame);

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

        float_frames_t _frame;      // frame position of current tick
        int _entry, _bar, _beat;    // current position in tempomap
        int _bar_total;             // current bar number (including previous entries)
        bool _init, _end;

        TempoMapConstPtr _tempomap;
        float _multiplier;
    };

    virtual void process_callback(sample_t *, nframes_t);
    virtual void timebase_callback(jack_position_t *);

    // transport position
    nframes_t _current;

    // position in tempomap
    Position _pos;

    bool _transport_enabled;
    bool _transport_master;
};


#endif // _METRONOME_MAP_H
