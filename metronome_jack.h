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
    MetronomeJack(AudioChunkConstPtr emphasis, AudioChunkConstPtr normal);
    virtual ~MetronomeJack();

    virtual bool running() const {
        return true;
    }

  protected:
    static const nframes_t MIN_FRAMES_DIFF = 64;

    virtual void process_callback(sample_t *, nframes_t);

  private:
    nframes_t _last_click_frame;
};


#endif // _METRONOME_JACK_H
