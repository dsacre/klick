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

#include "audio.h"
#include "audio_data.h"

#include <boost/noncopyable.hpp>

/*
 * abstract metronome base class
 */

class Metronome
  : protected AudioInterface::ProcessCallback,
    boost::noncopyable
{
  public:
    Metronome(AudioDataPtr emphasis, AudioDataPtr normal);
    virtual ~Metronome();

    virtual void start();
    virtual bool running() const = 0;

  protected:
    virtual void process_callback(sample_t *, nframes_t) = 0;

    void start_click(sample_t *buffer, nframes_t nframes,
                            nframes_t offset, AudioDataPtr click, float volume = 1.0);
    void continue_click(sample_t *buffer, nframes_t nframes);
    bool playing_click() { return _play_click; }
    void reset_click() { _play_click.reset(); }

    void copy_audio(sample_t *dest, const sample_t *src, nframes_t length, float volume = 1.0);

    AudioDataPtr _click_emphasis;
    AudioDataPtr _click_normal;

  private:
    // curently playing click
    AudioDataPtr _play_click;   // sample
    float _play_volume;         // volume
    nframes_t _play_sample;     // offset in sample
};


#endif // _METRONOME_H
