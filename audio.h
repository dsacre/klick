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

#ifndef _AUDIO_H
#define _AUDIO_H

#include <string>
#include <vector>

#include <jack/types.h>
#include <jack/transport.h>

#include "util.h"

typedef jack_default_audio_sample_t sample_t;
typedef jack_nframes_t nframes_t;


extern class AudioInterface *Audio;


class AudioInterface
  : public global_object<AudioInterface, &::Audio>
{
  public:
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

    AudioInterface(const std::string & name, const std::vector<std::string> & connect_ports, bool auto_connect);
    ~AudioInterface();


    void set_process_callback(ProcessCallback *);
    void set_timebase_callback(TimebaseCallback *);
    nframes_t samplerate() const { return _samplerate; }
    bool transport_rolling() const;
    jack_position_t position() const;
    nframes_t frame() const { return position().frame; }
    bool set_position(const jack_position_t &);
    bool set_frame(nframes_t);
    bool is_shutdown() const { return _shutdown; }

  protected:
    static int process_callback_(nframes_t, void *);
    static void timebase_callback_(jack_transport_state_t, nframes_t, jack_position_t *, int, void *);
    static void shutdown_callback_(void *);

    ProcessCallback *_process_obj;
    TimebaseCallback *_timebase_obj;
    volatile bool _shutdown;

    jack_client_t *_client;
    jack_port_t   *_output_port;
    nframes_t      _samplerate;
};

#endif // _AUDIO_H
