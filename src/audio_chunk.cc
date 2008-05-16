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

#include "audio_chunk.hh"

#include <sstream>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

#include <samplerate.h>
#include <sndfile.h>

#include "util/string.hh"
#include "util/debug.hh"

using namespace std;
using namespace das;


AudioChunk::AudioChunk(const string & filename, nframes_t samplerate)
{
    SF_INFO sfinfo = { 0, 0, 0, 0, 0, 0 };
    SNDFILE *f;

    if ((f = sf_open(filename.c_str(), SFM_READ, &sfinfo)) == NULL) {
        throw runtime_error(make_string() << "failed to open audio file '" << filename << "'");
    }

    sample_t *buf = new sample_t[sfinfo.frames * sfinfo.channels];
    sf_readf_float(f, buf, sfinfo.frames);

    // convert stereo to mono
    sample_t *mono_buf = NULL;

    if (sfinfo.channels == 2) {
        mono_buf = new sample_t[sfinfo.frames];
        for (int i = 0; i < sfinfo.frames; i++) {
            mono_buf[i] = (buf[i*2] + buf[i*2 + 1]) / 2;
        }
        delete [] buf;
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
    resample(mono_buf, sfinfo.frames, sfinfo.samplerate, _samples, _length, _samplerate);

    delete [] mono_buf;

    sf_close(f);
}


AudioChunk::~AudioChunk()
{
    ASSERT(_samples);
    delete [] _samples;
}


void AudioChunk::resample(const sample_t *samples_in, nframes_t length_in, nframes_t samplerate_in,
                          sample_t *& samples_out, nframes_t & length_out, nframes_t samplerate_out)
{
    SRC_DATA src_data;
    int error;

    src_data.input_frames = length_in;
    src_data.data_in = const_cast<float*>(samples_in);

    src_data.src_ratio = (float)samplerate_out / (float)samplerate_in;

    src_data.output_frames = (long)((float)length_in * src_data.src_ratio);
    src_data.data_out = new sample_t[src_data.output_frames];
    src_data.data_out[src_data.output_frames-1] = 0;

    if ((error = src_simple(&src_data, SRC_SINC_BEST_QUALITY, 1)) != 0) {
        delete [] src_data.data_out;
        throw runtime_error(make_string() << "error converting samplerate: " << src_strerror(error));
    }

    samples_out = src_data.data_out;
    length_out = src_data.output_frames;
}


void AudioChunk::adjust_volume(float volume)
{
    if (volume == 1.0f) return;

    for (nframes_t i = 0; i < _length; i++) {
        _samples[i] *= volume;
    }
}


void AudioChunk::adjust_frequency(float factor)
{
    if (factor == 1.0f) return;

    sample_t *s;
    nframes_t l;

    resample(_samples, _length, (nframes_t)(_samplerate * factor), s, l, _samplerate);
    delete [] _samples;
    _samples = s;
    _length = l;
}


void AudioChunk::resample(nframes_t samplerate)
{
    if (samplerate == _samplerate) return;

    adjust_frequency(float(_samplerate) / float(samplerate));
    _samplerate = samplerate;
}

