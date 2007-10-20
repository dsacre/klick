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
#include <boost/shared_ptr.hpp>

/*
 * plays a click track using a predefined tempomap
 */

class MetronomeMap
  : public Metronome,
    protected AudioInterface::TimebaseCallback
{
  public:
    MetronomeMap(boost::shared_ptr<const TempoMap> tempomap,
                float tempo_multiplier,
                bool transport, bool master,
                int preroll, const std::string & start_label,
                AudioDataPtr emphasis, AudioDataPtr normal);
    virtual ~MetronomeMap();

    void start();
    bool running() const { return _transport_enabled ? true : !_pos.end(); }

  protected:
    struct Click {
        nframes_t frame;
        TempoMap::BeatType type;
        float volume;
    };

    // keeps track of the current position in the tempomap
    class Position
    {
      public:
        Position(boost::shared_ptr<const TempoMap> tempomap, float multiplier);

        void set_start_label(const std::string & start_label);
        void add_preroll(int nbars);

        void locate(nframes_t);

        float dist_to_next() const;
        float next_frame() const { return frame() + dist_to_next(); }
        void advance();

        const Click click() const;
        bool end() const { return _end; }

        float frame() const { return _frame; }
        uint entry() const { return _entry; }
        uint bar() const { return _bar; }
        uint beat() const { return _beat; }
        uint bar_total() const { return _bar_total; }

        const TempoMap::Entry & map_entry() const { return (*_tempomap)[_entry]; }

      private:
        void reset();

        double _frame;
        uint _entry, _bar, _beat;
        uint _bar_total;
        bool _init, _end;

        boost::shared_ptr<const TempoMap> _tempomap;
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
