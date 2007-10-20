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

#include "audio.h"

#include <cerrno>

#include <jack/jack.h>
#include <jack/transport.h>

using namespace std;

AudioInterface *Audio = NULL;


AudioInterface::AudioInterface(const string & name, const vector<string> & connect_ports, bool auto_connect)
  : _process_obj(NULL),
    _timebase_obj(NULL),
    _shutdown(false)
{
    if ((_client = jack_client_new(name.c_str())) == 0) {
        throw "can't connect to jack server";
    }
    jack_set_process_callback(_client, &process_callback_, static_cast<void*>(this));
    jack_on_shutdown(_client, &shutdown_callback_, static_cast<void*>(this));

    if ((_output_port = jack_port_register(_client, "out", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)) == NULL) {
        throw "can't register output port";
    }

    if (jack_activate(_client)) {
        throw "can't activate client";
    }

    for (vector<string>::const_iterator i = connect_ports.begin(); i != connect_ports.end(); ++i) {
        int error = jack_connect(_client, jack_port_name(_output_port), i->c_str());
        if (error && error != EEXIST) {
            cerr << "can't connect " << jack_port_name(_output_port) << " to " << i->c_str() << endl;
        } else {
            logv << "connected to " << i->c_str() << "" << endl;
        }
    }

    if (auto_connect) {
        // connect to first two hardware outs
        const char ** hw_ports = jack_get_ports(_client, NULL, NULL, JackPortIsPhysical | JackPortIsInput);
        if (hw_ports) {
            for (int n = 0; n < 2 && hw_ports[n] != NULL; ++n) {
                jack_connect(_client, jack_port_name(_output_port), hw_ports[n]);
            }
            free(hw_ports);
        }
    }

    _samplerate = jack_get_sample_rate(_client);
}


AudioInterface::~AudioInterface()
{
    jack_deactivate(_client);
    jack_client_close(_client);
}


void AudioInterface::set_process_callback(ProcessCallback *obj)
{
    _process_obj = obj;
}


void AudioInterface::set_timebase_callback(TimebaseCallback *obj)
{
    if (obj) {
        if (jack_set_timebase_callback(_client, 0, &timebase_callback_, static_cast<void*>(this)) != 0) {
            cerr << "failed to become jack transport master" << endl;
        }
    } else {
        if (_timebase_obj) {
            jack_release_timebase(_client);
        }
    }
    _timebase_obj = obj;
}


bool AudioInterface::transport_rolling() const
{
    return (jack_transport_query(_client, NULL) == JackTransportRolling);
}


jack_position_t AudioInterface::position() const
{
    jack_position_t pos;
    jack_transport_query(_client, &pos);
    return pos;
}


bool AudioInterface::set_position(const jack_position_t & pos)
{
    // jack doesn't modify pos, should have been const anyway
    return (jack_transport_reposition(_client, const_cast<jack_position_t*>(&pos)) == 0);
}


bool AudioInterface::set_frame(nframes_t frame)
{
    return (jack_transport_locate(_client, frame) == 0);
}


int AudioInterface::process_callback_(nframes_t nframes, void *arg)
{
    AudioInterface *this_ = static_cast<AudioInterface*>(arg);
    sample_t *buffer = (sample_t *)jack_port_get_buffer(this_->_output_port, nframes);

    memset(buffer, 0, nframes * sizeof(sample_t));

    if (this_->_process_obj) {
        (this_->_process_obj)->process_callback(buffer, nframes);
    }

    return 0;
}


void AudioInterface::timebase_callback_(jack_transport_state_t state, nframes_t nframes,
                               jack_position_t *pos, int new_pos, void *arg)
{
    AudioInterface *this_ = static_cast<AudioInterface*>(arg);

    if (this_->_timebase_obj) {
        (this_->_timebase_obj)->timebase_callback(pos);
    }
}


void AudioInterface::shutdown_callback_(void *arg)
{
    static_cast<AudioInterface*>(arg)->_shutdown = true;
}
