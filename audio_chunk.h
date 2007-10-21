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

#ifndef _AUDIO_CHUNK_H
#define _AUDIO_CHUNK_H

#include "audio.h"

#include <boost/shared_ptr.hpp>

/*
 * mono audio sample
 */
class AudioChunk
{
  public:
    // creates sample from plain audio data
    AudioChunk(const sample_t *samples, nframes_t length, nframes_t samplerate, float volume = 1.0f);
    // loads sample from file, converting to the given samplerate if samplerate is non-zero
    AudioChunk(const std::string & filename, nframes_t samplerate = 0);
    // makes a copy of another sample
    AudioChunk(const AudioChunk & in, nframes_t samplerate = 0);
    ~AudioChunk();

    const sample_t *samples() const { return _samples ? : _static_samples; }
    nframes_t length() const { return _length; }
    nframes_t samplerate() const { return _samplerate; }

    void adjust_volume(float);
    void adjust_frequency(float);

  protected:
    void resample(const sample_t *samples_in, nframes_t length_in, nframes_t samplerate_in,
                  sample_t **samples_out, nframes_t *length_out, nframes_t samplerate_out);

    const sample_t *_static_samples;
    sample_t *_samples;
    nframes_t _length;
    nframes_t _samplerate;
};


typedef boost::shared_ptr<AudioChunk> AudioChunkPtr;


#endif // _AUDIO_CHUNK_H
