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

#include "audio_interface.hh"

#include <jack/jack.h>
#include <jack/transport.h>
#include <iostream>
#include <cerrno>
#include <cstring>

#include "util/string.hh"
#include "util/debug.hh"


AudioInterface::AudioInterface(std::string const & name)
  : _process_obj()
  , _timebase_obj()
  , _process_mix(false)
  , _shutdown(false)
  , _volume(1.0f)
  , _next_chunk(0)
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


AudioInterface::~AudioInterface()
{
    jack_deactivate(_client);
    jack_client_close(_client);
}


std::string AudioInterface::client_name() const
{
    return std::string(jack_get_client_name(_client));
}


nframes_t AudioInterface::samplerate() const
{
    return jack_get_sample_rate(_client);
}


pthread_t AudioInterface::client_thread() const
{
    return jack_client_thread_id(_client);
}


void AudioInterface::set_process_callback(boost::shared_ptr<ProcessCallback> obj, bool mix)
{
    _process_obj = obj;
    _process_mix = mix;
}


void AudioInterface::set_timebase_callback(boost::shared_ptr<TimebaseCallback> obj)
{
    if (obj) {
        if (jack_set_timebase_callback(_client, 0, &timebase_callback_, static_cast<void*>(this)) != 0) {
            throw AudioError("failed to become jack transport master");
        }
    } else {
        if (_timebase_obj) {
            jack_release_timebase(_client);
        }
    }
    _timebase_obj = obj;
}


void AudioInterface::connect(std::string const & port)
{
    int error = jack_connect(_client, jack_port_name(_output_port), port.c_str());
    if (error && error != EEXIST) {
        throw AudioError(das::make_string() << "can't connect " << jack_port_name(_output_port) << " to " << port.c_str());
    }
}


void AudioInterface::autoconnect()
{
    // find first two hardware outs
    char const **hw_ports = jack_get_ports(_client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsPhysical | JackPortIsInput);

    if (hw_ports) {
        for (int n = 0; n < 2 && hw_ports[n] != NULL; ++n) {
            jack_connect(_client, jack_port_name(_output_port), hw_ports[n]);
        }
        std::free(hw_ports);
    }
}


void AudioInterface::disconnect_all()
{
    jack_port_disconnect(_client, _output_port);
}


std::vector<std::string> AudioInterface::available_ports()
{
    std::vector<std::string> v;

    char const **ports = jack_get_ports(_client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);

    if (ports) {
        char const **p = ports;
        while (*p) {
            v.push_back(*p++);
        }
        std::free(ports);
    }

    return v;
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


bool AudioInterface::set_position(jack_position_t const & pos)
{
    // jack doesn't modify pos, should have been const anyway, i guess...
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

    std::memset(buffer, 0, nframes * sizeof(sample_t));

    if (this_->_process_obj) {
        (this_->_process_obj)->process_callback(buffer, nframes);
    }

    if (this_->_process_mix) {
        this_->process_mix(buffer, nframes);
    }

    return 0;
}


void AudioInterface::timebase_callback_(jack_transport_state_t /*state*/, nframes_t /*nframes*/,
                                        jack_position_t *pos, int /*new_pos*/, void *arg)
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


void AudioInterface::play(AudioChunkConstPtr chunk, nframes_t offset, float volume)
{
    ASSERT(chunk->samplerate() == samplerate());

    _chunks[_next_chunk].chunk  = chunk;
    _chunks[_next_chunk].offset = offset;
    _chunks[_next_chunk].pos    = 0;
    _chunks[_next_chunk].volume = volume;

    _next_chunk = (_next_chunk + 1) % _chunks.size();
}


void AudioInterface::process_mix(sample_t *buffer, nframes_t nframes)
{
    for (ChunkArray::iterator i = _chunks.begin(); i != _chunks.end(); ++i)
    {
        if (i->chunk) {
            process_mix_samples(buffer + i->offset,
                                i->chunk->samples() + i->pos,
                                std::min(nframes - i->offset, i->chunk->length() - i->pos),
                                i->volume * _volume);

            i->pos += nframes - i->offset;
            i->offset = 0;

            if (i->pos >= i->chunk->length()) {
                i->chunk.reset();
            }
        }
    }
}


inline void AudioInterface::process_mix_samples(sample_t *dest, sample_t const * src, nframes_t length, float volume)
{
    for (sample_t *end = dest + length; dest < end; dest++, src++) {
        *dest += *src * volume;
    }
}
