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
#include "util/global_object.hh"

#include <string>
#include <vector>
#include <boost/array.hpp>
#include <stdexcept>

#include <jack/transport.h>


extern class AudioInterface *Audio;


class AudioInterface
  : public das::global_object<AudioInterface, ::Audio>
{
    static const int MAX_PLAYING_CHUNKS = 4;

  public:
    struct AudioError : public std::runtime_error {
        AudioError(const std::string & w)
          : std::runtime_error(w) { }
    };

    class ProcessCallback {
      protected:
        ProcessCallback() { }
        virtual ~ProcessCallback() { }
        friend class AudioInterface;
        virtual void process_callback(sample_t *, nframes_t) = 0;
    };

    class TimebaseCallback {
      protected:
        TimebaseCallback() { }
        virtual ~TimebaseCallback() { }
        friend class AudioInterface;
        virtual void timebase_callback(jack_position_t *) = 0;
    };

    AudioInterface(const std::string & name,
                   const std::vector<std::string> & connect_ports,
                   bool auto_connect);
    ~AudioInterface();

    std::string client_name() const;

    void set_process_callback(ProcessCallback *, bool mix = false);
    void set_timebase_callback(TimebaseCallback *);

    nframes_t samplerate() const { return _samplerate; }
    bool transport_rolling() const;
    jack_position_t position() const;
    nframes_t frame() const { return position().frame; }
    bool set_position(const jack_position_t &);
    bool set_frame(nframes_t);
    bool is_shutdown() const { return _shutdown; }

    void play(AudioChunkConstPtr chunk,
              nframes_t offset,
              float volume = 1.0);

  private:
    static int process_callback_(nframes_t, void *);
    static void timebase_callback_(jack_transport_state_t, nframes_t, jack_position_t *, int, void *);
    static void shutdown_callback_(void *);

    void process_mix(sample_t *, nframes_t);
    void process_mix_samples(sample_t *dest, const sample_t *src, nframes_t length, float volume = 1.0);

    ProcessCallback *_process_obj;
    TimebaseCallback *_timebase_obj;
    bool _process_mix;
    volatile bool _shutdown;

    jack_client_t *_client;
    jack_port_t   *_output_port;
    nframes_t      _samplerate;

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
