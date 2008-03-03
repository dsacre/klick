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

#include "audio_chunk.h"

#include <sstream>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include <samplerate.h>
#include <sndfile.h>

#include "util/exception.h"
#include "util/string.h"
#include "util/debug.h"

using namespace std;
using namespace das;


AudioChunk::AudioChunk(const string & filename, nframes_t samplerate)
{
    SF_INFO sfinfo = { 0 };
    SNDFILE *f;

    if ((f = sf_open(filename.c_str(), SFM_READ, &sfinfo)) == NULL) {
        throw das::exception(make_string() << "failed to open audio file '" << filename << "'");
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
    resample(mono_buf, sfinfo.frames, sfinfo.samplerate, _samples, _length, _samplerate);

    free(mono_buf);

    sf_close(f);
}


AudioChunk::AudioChunk(const AudioChunk & in)
  : _length(in.length()),
    _samplerate(in.samplerate())
{
    size_t s = in.length() * sizeof(sample_t);
    _samples = (sample_t*)malloc(s);
    memcpy(_samples, in.samples(), s);
}


AudioChunk::AudioChunk(const sample_t * samples, nframes_t length, nframes_t samplerate)
  : _length(length),
    _samplerate(samplerate)
{
    size_t s = length * sizeof(sample_t);
    _samples = (sample_t*)malloc(s);
    memcpy(_samples, samples, s);
}


AudioChunk::~AudioChunk()
{
    ASSERT(_samples);
    free(_samples);
}


void AudioChunk::resample(const sample_t *samples_in,
                          nframes_t length_in,
                          nframes_t samplerate_in,
                          sample_t *& samples_out,
                          nframes_t & length_out,
                          nframes_t samplerate_out)
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
        throw das::exception(make_string() << "error converting samplerate: " << src_strerror(error));
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
    free(_samples);
    _samples = s;
    _length = l;
}


void AudioChunk::resample(nframes_t samplerate)
{
    if (samplerate == _samplerate) return;

    adjust_frequency(float(_samplerate) / float(samplerate));
    _samplerate = samplerate;
}

