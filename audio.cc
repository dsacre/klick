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
#include "util.h"

#include <iostream>
#include <sstream>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <cerrno>

#include <jack/jack.h>
#include <jack/transport.h>
#include <samplerate.h>
#include <sndfile.h>

using namespace std;

AudioInterface *Audio = NULL;


AudioInterface::AudioInterface(const string & name, const vector<string> & connect_ports)
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
            cerr << "failed to become jack transport master";
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


AudioData::AudioData(const sample_t *samples, nframes_t length, nframes_t samplerate, float volume)
  : _static_samples(samples),
    _samples(NULL),
    _length(length),
    _samplerate(samplerate)
{
    if (volume != 1.0f) {
        _samples = (sample_t*)calloc(length, sizeof(sample_t));
        memcpy(_samples, samples, length * sizeof(sample_t));
        _static_samples = NULL;
        adjust_volume(volume);
    }
}

AudioData::AudioData(const string & filename, nframes_t samplerate)
  : _static_samples(NULL)
{
    SF_INFO sfinfo = { 0 };
    SNDFILE* f;

    if ((f = sf_open(filename.c_str(), SFM_READ, &sfinfo)) == NULL) {
        ostringstream os;
        os << "failed to open audio file '" << filename << "'";
        throw os.str();
    }

    sample_t *buf = (sample_t*)calloc(sfinfo.frames * sfinfo.channels, sizeof(sample_t));
    sf_readf_float(f, buf, sfinfo.frames);

    // convert stereo to mono
    sample_t *mono_buf;

    if (sfinfo.channels == 2) {
        mono_buf = (sample_t*)calloc(sfinfo.frames, sizeof(sample_t));
        for (int i = 0; i < sfinfo.frames; i++) {
            mono_buf[i] = (buf[i*2] + buf[i*2 + 1]) / 2;
        }
        free(buf);
    } else {
        // mono or more than 2 channels.
        // in case there are more than 2 channels, use only the first
        mono_buf = buf;
    }

    // convert samplerate
    if (samplerate) {
        _samplerate = samplerate;
    } else {
        _samplerate = sfinfo.samplerate;
    }
    resample(mono_buf, sfinfo.frames, sfinfo.samplerate, &_samples, &_length, _samplerate);

    free(mono_buf);

    sf_close(f);
}

AudioData::AudioData(const AudioData & in, nframes_t samplerate)
  : _static_samples(NULL)
{
    if (!samplerate || samplerate == in._samplerate) {
        _samples = (sample_t*)calloc(in.length(), sizeof(sample_t));
        memcpy(_samples, in.samples(), in.length() * sizeof(sample_t));
        _length = in.length();
        _samplerate = in._samplerate;
    } else {
        _samplerate = samplerate;
        resample(in.samples(), in.length(), in._samplerate, &_samples, &_length, _samplerate);
    }
}


AudioData::~AudioData()
{
    if (_samples) {
        free(_samples);
    }
}

void AudioData::resample(const sample_t *samples_in, nframes_t length_in, nframes_t samplerate_in,
                         sample_t **samples_out, nframes_t *length_out, nframes_t samplerate_out)
{
    SRC_DATA src_data;
    int error;

    src_data.input_frames = length_in;
    src_data.data_in = const_cast<float*>(samples_in);

    src_data.src_ratio = (float)samplerate_out / (float)samplerate_in;

    src_data.output_frames = (long)ceil((float)length_in * src_data.src_ratio);
    src_data.data_out = (sample_t*)calloc(src_data.output_frames, sizeof(sample_t));

    if ((error = src_simple(&src_data, SRC_SINC_BEST_QUALITY, 1)) != 0) {
        free(src_data.data_out);
        throw src_strerror(error);
    }

    *samples_out = src_data.data_out;
    *length_out = src_data.output_frames;
}

void AudioData::adjust_volume(float volume)
{
    for (uint i = 0; i < _length; i++) {
        _samples[i] *= volume;
    }
}

void AudioData::adjust_frequency(float factor)
{
    sample_t *s;
    nframes_t l;

    resample(samples(), length(), (nframes_t)(samplerate() * factor), &s, &l, samplerate());
    if (_samples) free(_samples);
    _samples = s;
    _static_samples = NULL;
    _length = l;
}
