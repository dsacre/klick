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

#ifndef KLICK_AUDIO_INTERFACE_HH
#define KLICK_AUDIO_INTERFACE_HH

#include "audio.hh"

#include <string>
#include <stdexcept>
#include <memory>
#include <array>
#include <functional>
#include <boost/noncopyable.hpp>


class AudioInterface
  : boost::noncopyable
{
  public:

    struct AudioError : public std::runtime_error {
        AudioError(std::string const & w) : std::runtime_error(w) { }
    };

    AudioInterface();
    virtual ~AudioInterface() { }

    typedef std::function<void (sample_t *, nframes_t)> ProcessCallback;

    virtual void set_process_callback(ProcessCallback cb);

    // get sample rate
    virtual nframes_t samplerate() const = 0;
    // check if backend is still running
    virtual bool is_shutdown() const = 0;

    // start playing audio chunk at offset into the current period
    void play(AudioChunkConstPtr chunk, nframes_t offset, float volume = 1.0);

    void set_volume(float v) { _volume = v; }
    float volume() const { return _volume; }

  protected:

    ProcessCallback _process_cb;

    void process_mix(sample_t *, nframes_t);

  private:

    void process_mix_samples(sample_t *dest, sample_t const * src, nframes_t length, float volume = 1.0);

    // maximum number of audio chunks that can be played simultaneously
    static int const MAX_PLAYING_CHUNKS = 4;

    struct PlayingChunk {
        AudioChunkConstPtr chunk;
        nframes_t offset;
        nframes_t pos;
        float volume;
    };

    typedef std::array<PlayingChunk, MAX_PLAYING_CHUNKS> ChunkArray;

    ChunkArray _chunks;
    int _next_chunk;
    float _volume;
};



class AudioInterfaceTransport
  : public AudioInterface
{
  public:

    typedef std::function<void (position_t *)> TimebaseCallback;

    virtual void set_timebase_callback(TimebaseCallback cb) = 0;


    virtual bool transport_rolling() const = 0;
    virtual position_t position() const = 0;
    virtual nframes_t frame() const = 0;
    virtual bool set_position(position_t const &) = 0;
    virtual bool set_frame(nframes_t) = 0;

  protected:

    TimebaseCallback _timebase_cb;
};


#endif // KLICK_AUDIO_INTERFACE_HH
