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

#ifndef _AUDIO_CHUNK_HH
#define _AUDIO_CHUNK_HH

#include "audio.hh"

#include <boost/shared_array.hpp>

#include "util/disposable.hh"


/*
 * mono 32-bit float audio sample
 */
class AudioChunk
  : public das::disposable
{
  public:
    // loads sample from file, converting to the given samplerate if samplerate is non-zero
    AudioChunk(std::string const & filename, nframes_t samplerate = 0);

    // create empty audio
    AudioChunk(nframes_t samplerate = 0)
      : _samples()
      , _length(0)
      , _samplerate(samplerate)
    {
    }

    void adjust_volume(float volume);
    void adjust_frequency(float factor);

    // resample to the given samplerate
    void resample(nframes_t samplerate);

    sample_t const * samples() const { return _samples.get(); }
    nframes_t length() const { return _length; }
    nframes_t samplerate() const { return _samplerate; }

  private:
    typedef boost::shared_array<sample_t> SamplePtr;

    static void resample(SamplePtr samples_in, nframes_t length_in, nframes_t samplerate_in,
                         SamplePtr & samples_out, nframes_t & length_out, nframes_t samplerate_out);

    SamplePtr _samples;
    nframes_t _length;
    nframes_t _samplerate;
};


#endif // _AUDIO_CHUNK_HH
