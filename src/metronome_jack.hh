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

#ifndef _METRONOME_JACK_HH
#define _METRONOME_JACK_HH

#include "audio.hh"
#include "metronome.hh"

/*
 * plays a click track following jack transport; no tempomap!
 */
class MetronomeJack
  : public Metronome
{
  public:
    MetronomeJack(AudioInterface & audio);
    virtual ~MetronomeJack();

    virtual bool running() const {
        return true;
    }

  protected:
    virtual void process_callback(sample_t *, nframes_t);

  private:
    static nframes_t const MIN_FRAMES_DIFF = 64;

    nframes_t _last_click_frame;
};


#endif // _METRONOME_JACK_HH
