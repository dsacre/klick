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

#ifndef _METRONOME_JACK_H
#define _METRONOME_JACK_H

#include "audio.h"
#include "metronome.h"

/*
 * plays a click track following jack transport; no tempomap!
 */

class MetronomeJack
  : public Metronome
{
  public:
    MetronomeJack(AudioChunkPtr emphasis, AudioChunkPtr normal)
      : Metronome(emphasis, normal)
    { }
    virtual ~MetronomeJack() { }

    virtual bool running() const { return true; }

  protected:
    virtual void process_callback(sample_t *, nframes_t);
};


#endif // _METRONOME_JACK_H
