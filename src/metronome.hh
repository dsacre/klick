/*
 * klick - an advanced metronome for jack
 *
 * Copyright (C) 2007-2009  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _METRONOME_HH
#define _METRONOME_HH

#include "audio_interface.hh"
#include "audio_chunk.hh"

#include <boost/noncopyable.hpp>

#include "util/disposable.hh"


/*
 * abstract metronome base class
 */
class Metronome
  : public das::disposable
  , boost::noncopyable
{
  public:

    Metronome(AudioInterface & audio);
    virtual ~Metronome() { }

    void register_process_callback();
    void register_timebase_callback();

    void set_sound(AudioChunkConstPtr emphasis, AudioChunkConstPtr normal);

    void set_active(bool b);
    void start() { set_active(true); }
    void stop() { set_active(false); }

    virtual void do_start() { }
    virtual void do_stop() { }

    bool active() const { return _active; }

    virtual bool running() const = 0;

  protected:

    virtual void process_callback(sample_t *, nframes_t) = 0;
    virtual void timebase_callback(position_t *) { }

    void play_click(bool emphasis, nframes_t offset, float volume = 1.0f);

    AudioInterface & _audio;

    AudioChunkConstPtr _click_emphasis;
    AudioChunkConstPtr _click_normal;

  private:

    bool _active;
};


#endif // _METRONOME_HH
