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

#include "audio_interface_jack.hh"

#include <jack/jack.h>
#include <jack/transport.h>
#include <iostream>
#include <cerrno>
#include <cstring>

#include "util/string.hh"
#include "util/debug.hh"


AudioInterfaceJack::AudioInterfaceJack(std::string const & name)
  : _shutdown(false)
{
    if ((_client = jack_client_open(name.c_str(), JackNullOption, NULL)) == 0) {
        throw AudioError("can't connect to jack server");
    }

    jack_set_process_callback(_client, &process_callback_, static_cast<void*>(this));
    jack_on_shutdown(_client, &shutdown_callback_, static_cast<void*>(this));

    if ((_output_port = jack_port_register(_client, "out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)) == NULL) {
        throw AudioError("can't register output port");
    }

    if (jack_activate(_client)) {
        throw AudioError("can't activate client");
    }
}


AudioInterfaceJack::~AudioInterfaceJack()
{
    jack_deactivate(_client);
    jack_client_close(_client);
}


std::string AudioInterfaceJack::client_name() const
{
    return std::string(jack_get_client_name(_client));
}


pthread_t AudioInterfaceJack::client_thread() const
{
    return jack_client_thread_id(_client);
}


nframes_t AudioInterfaceJack::samplerate() const
{
    return jack_get_sample_rate(_client);
}


bool AudioInterfaceJack::is_shutdown() const
{
    return _shutdown;
}


void AudioInterfaceJack::set_timebase_callback(TimebaseCallback cb)
{
    if (cb) {
        if (jack_set_timebase_callback(_client, 0, &timebase_callback_, static_cast<void*>(this)) != 0) {
            throw AudioError("failed to become jack transport master");
        }
    } else {
        if (_timebase_cb) {
            jack_release_timebase(_client);
        }
    }
    _timebase_cb = cb;
}


void AudioInterfaceJack::connect(std::string const & port)
{
    int error = jack_connect(_client, jack_port_name(_output_port), port.c_str());
    if (error && error != EEXIST) {
        throw AudioError(das::make_string() << "can't connect " << jack_port_name(_output_port) << " to " << port.c_str());
    }
}


void AudioInterfaceJack::autoconnect()
{
    // find first two hardware outs
    char const **hw_ports = jack_get_ports(_client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsPhysical | JackPortIsInput);

    if (hw_ports) {
        for (int n = 0; n < 2 && hw_ports[n] != NULL; ++n) {
            jack_connect(_client, jack_port_name(_output_port), hw_ports[n]);
        }
        jack_free(hw_ports);
    }
}


void AudioInterfaceJack::disconnect_all()
{
    jack_port_disconnect(_client, _output_port);
}


std::vector<std::string> AudioInterfaceJack::available_ports()
{
    std::vector<std::string> v;

    char const **ports = jack_get_ports(_client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);

    if (ports) {
        char const **p = ports;
        while (*p) {
            v.push_back(*p++);
        }
        jack_free(ports);
    }

    return v;
}


bool AudioInterfaceJack::transport_rolling() const
{
    return (jack_transport_query(_client, NULL) == JackTransportRolling);
}


position_t AudioInterfaceJack::position() const
{
    position_t pos;
    jack_transport_query(_client, &pos);
    return pos;
}


nframes_t AudioInterfaceJack::frame() const
{
    return position().frame;
}


bool AudioInterfaceJack::set_position(position_t const & pos)
{
    // jack doesn't modify pos, should have been const anyway, i guess...
    return (jack_transport_reposition(_client, const_cast<position_t*>(&pos)) == 0);
}


bool AudioInterfaceJack::set_frame(nframes_t frame)
{
    return (jack_transport_locate(_client, frame) == 0);
}


int AudioInterfaceJack::process_callback_(nframes_t nframes, void *arg)
{
    AudioInterfaceJack *this_ = static_cast<AudioInterfaceJack*>(arg);
    sample_t *buffer = (sample_t *)jack_port_get_buffer(this_->_output_port, nframes);

    std::memset(buffer, 0, nframes * sizeof(sample_t));

    if (this_->_process_cb) {
        this_->_process_cb(buffer, nframes);
    }

    this_->process_mix(buffer, nframes);

    return 0;
}


void AudioInterfaceJack::timebase_callback_(jack_transport_state_t /*state*/, nframes_t /*nframes*/,
                                            position_t *pos, int /*new_pos*/, void *arg)
{
    AudioInterfaceJack *this_ = static_cast<AudioInterfaceJack*>(arg);

    if (this_->_timebase_cb) {
        this_->_timebase_cb(pos);
    }
}


void AudioInterfaceJack::shutdown_callback_(void *arg)
{
    static_cast<AudioInterfaceJack*>(arg)->_shutdown = true;
}
