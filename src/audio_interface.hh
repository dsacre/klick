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

#ifndef _AUDIO_INTERFACE_HH
#define _AUDIO_INTERFACE_HH

#include "audio.hh"
#include "audio_chunk.hh"

#include <string>
#include <vector>
#include <boost/array.hpp>
#include <stdexcept>
#include <boost/noncopyable.hpp>

#include <jack/transport.h>


class AudioInterface
  : boost::noncopyable
{
    // maximum number of audio chunks that can be played simultaneously
    static int const MAX_PLAYING_CHUNKS = 4;

  public:
    struct AudioError : public std::runtime_error {
        AudioError(std::string const & w)
          : std::runtime_error(w) { }
    };

    AudioInterface(std::string const & name);
    ~AudioInterface();

    // abstract base class for process callbacks
    class ProcessCallback {
      protected:
        ProcessCallback() { }
        virtual ~ProcessCallback() { }
        friend class AudioInterface;
        virtual void process_callback(sample_t *, nframes_t) = 0;
    };

    // abstract base class for timebase callbacks
    class TimebaseCallback {
      protected:
        TimebaseCallback() { }
        virtual ~TimebaseCallback() { }
        friend class AudioInterface;
        virtual void timebase_callback(jack_position_t *) = 0;
    };

    void set_process_callback(boost::shared_ptr<ProcessCallback>, bool mix = false);
    void set_timebase_callback(boost::shared_ptr<TimebaseCallback>);

    // get client name
    std::string client_name() const;
    // get sample rate
    nframes_t samplerate() const;

    pthread_t client_thread() const;

    void set_volume(float v) { _volume = v; }
    float volume() const { return _volume; }

    void connect(std::string const & port);
    void autoconnect();

    // JACK transport
    bool transport_rolling() const;
    jack_position_t position() const;
    nframes_t frame() const { return position().frame; }
    bool set_position(jack_position_t const &);
    bool set_frame(nframes_t);

    bool is_shutdown() const { return _shutdown; }

    // start playing audio chunk at offset into the current period
    void play(AudioChunkConstPtr chunk,
              nframes_t offset,
              float volume = 1.0);

  private:
    static int process_callback_(nframes_t, void *);
    static void timebase_callback_(jack_transport_state_t, nframes_t, jack_position_t *, int, void *);
    static void shutdown_callback_(void *);

    void process_mix(sample_t *, nframes_t);
    void process_mix_samples(sample_t *dest, sample_t const * src, nframes_t length, float volume = 1.0);

    boost::shared_ptr<ProcessCallback> _process_obj;
    boost::shared_ptr<TimebaseCallback> _timebase_obj;
    bool _process_mix;
    volatile bool _shutdown;

    jack_client_t *_client;
    jack_port_t *_output_port;

    float _volume;

    struct PlayingChunk {
        AudioChunkConstPtr chunk;
        nframes_t offset;
        nframes_t pos;
        float volume;
    };

    typedef boost::array<PlayingChunk, MAX_PLAYING_CHUNKS> ChunkArray;

    ChunkArray _chunks;
    int _next_chunk;
};

#endif // _AUDIO_INTERFACE_HH
