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
 * mono 32-bit audio sample
 */
class AudioChunk
{
  public:
    // loads sample from file, converting to the given samplerate if samplerate is non-zero
    AudioChunk(const std::string & filename, nframes_t samplerate = 0);
    // copy c'tor
    AudioChunk(const AudioChunk & in);
    // makes an AudioChunk object from raw samples
    AudioChunk(const sample_t * samples, nframes_t length, nframes_t samplerate);

    ~AudioChunk();

    void adjust_volume(float volume);
    void adjust_frequency(float factor);

    // resample to the given samplerate
    void resample(nframes_t samplerate);

    const sample_t * samples() const { return _samples; }
    nframes_t length() const { return _length; }
    nframes_t samplerate() const { return _samplerate; }

  protected:
    void resample(const sample_t *samples_in,
                  nframes_t length_in,
                  nframes_t samplerate_in,
                  sample_t *& samples_out,
                  nframes_t & length_out,
                  nframes_t samplerate_out);

    sample_t *_samples;
    nframes_t _length;
    nframes_t _samplerate;
};


#endif // _AUDIO_CHUNK_H
