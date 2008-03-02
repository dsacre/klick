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

#include "metronome.h"
#include "tempomap.h"
#include "position.h"

#include <string>

/*
 * plays a click track using a predefined tempomap
 */

class MetronomeMap
  : public Metronome,
    protected AudioInterface::TimebaseCallback
{
  public:
    MetronomeMap(
        TempoMapConstPtr tempomap,
        float tempo_multiplier,
        bool transport,
        bool master,
        int preroll,
        const std::string & start_label,
        AudioChunkConstPtr emphasis,
        AudioChunkConstPtr normal
    );
    virtual ~MetronomeMap();

    void start();
    bool running() const;

  protected:
    static const double TICKS_PER_BEAT = 1920.0;

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
