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

#ifndef KLICK_AUDIO_INTERFACE_JACK_HH
#define KLICK_AUDIO_INTERFACE_JACK_HH

#include "audio_interface.hh"
#include "main.hh"

#include <string>
#include <vector>

#include <jack/transport.h>


class AudioInterfaceJack
  : public AudioInterfaceTransport
{
  public:

    AudioInterfaceJack(std::string const & name);
    virtual ~AudioInterfaceJack();

    virtual void set_timebase_callback(TimebaseCallback cb);

    // get JACK client name
    std::string client_name() const;
    // get id of JACK audio processing thread
    pthread_t client_thread() const;

    virtual nframes_t samplerate() const;
    virtual bool is_shutdown() const;

    // JACK connections
    void connect(std::string const & port);
    void autoconnect();
    void disconnect_all();
    std::vector<std::string> available_ports();

    // JACK transport
    virtual bool transport_rolling() const;
    virtual position_t position() const;
    virtual nframes_t frame() const;
    virtual bool set_position(position_t const &);
    virtual bool set_frame(nframes_t);

  private:

    static int process_callback_(nframes_t, void *) REALTIME;
    static void timebase_callback_(jack_transport_state_t, nframes_t, position_t *, int, void *) REALTIME;
    static void shutdown_callback_(void *);

    jack_client_t *_client;
    jack_port_t *_output_port;

    volatile bool _shutdown;
};


#endif // KLICK_AUDIO_INTERFACE_JACK_HH
