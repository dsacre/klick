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

#ifndef _METRONOME_H
#define _METRONOME_H

#include "audio_interface.h"
#include "audio_chunk.h"

#include <boost/noncopyable.hpp>

/*
 * abstract metronome base class
 */

class Metronome
  : protected AudioInterface::ProcessCallback,
    boost::noncopyable
{
  public:
    Metronome(AudioChunkConstPtr emphasis,
              AudioChunkConstPtr normal);
    virtual ~Metronome();

    virtual void start();
    virtual bool running() const = 0;

  protected:
    virtual void process_callback(sample_t *, nframes_t) = 0;

    void play_click(bool emphasis, nframes_t offset, float volume = 1.0f);

    AudioChunkConstPtr _click_emphasis;
    AudioChunkConstPtr _click_normal;
};


#endif // _METRONOME_H
