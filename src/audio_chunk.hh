/*
 * klick - an advanced metronome for jack
 *
 * Copyright (C) 2007-2013  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef KLICK_AUDIO_CHUNK_HH
#define KLICK_AUDIO_CHUNK_HH

#include "audio.hh"


/*
 * mono 32-bit float audio sample
 */
class AudioChunk
{
  public:
    // loads sample from file, converting to the given samplerate if samplerate is non-zero
    AudioChunk(std::string const & filename, nframes_t samplerate);

    // create empty audio
    AudioChunk(nframes_t samplerate)
      : _samples()
      , _length(0)
      , _samplerate(samplerate)
    {
    }

    void adjust_volume(float volume);
    void adjust_pitch(float factor);

    sample_t const * samples() const { return _samples.get(); }
    nframes_t length() const { return _length; }
    nframes_t samplerate() const { return _samplerate; }

  private:
    typedef std::unique_ptr<sample_t[]> SamplePtr;

    void resample(nframes_t samplerate);
#ifdef ENABLE_RUBBERBAND
    void pitch_shift(float factor);
#endif

    SamplePtr _samples;
    nframes_t _length;
    nframes_t _samplerate;
};


#endif // KLICK_AUDIO_CHUNK_HH
