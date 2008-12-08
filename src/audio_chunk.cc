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


AudioChunk::AudioChunk(std::string const & filename, nframes_t samplerate)
{
    SF_INFO sfinfo = { 0, 0, 0, 0, 0, 0 };
    SNDFILE *f;

    if ((f = sf_open(filename.c_str(), SFM_READ, &sfinfo)) == NULL) {
        throw std::runtime_error(das::make_string() << "failed to open audio file '" << filename << "'");
    }

    SamplePtr buf(new sample_t[sfinfo.frames * sfinfo.channels]);
    sf_readf_float(f, buf.get(), sfinfo.frames);

    if (sfinfo.channels == 2) {
        // convert stereo to mono
        SamplePtr mono_buf(new sample_t[sfinfo.frames]);
        for (int i = 0; i < sfinfo.frames; i++) {
            mono_buf[i] = (buf[i*2] + buf[i*2 + 1]) / 2;
        }
        buf = mono_buf;
    }

    // convert samplerate
    if (samplerate) {
        _samplerate = samplerate;
    } else {
        _samplerate = sfinfo.samplerate;
    }
    resample(buf, sfinfo.frames, sfinfo.samplerate, _samples, _length, _samplerate);

    sf_close(f);
}


void AudioChunk::resample(SamplePtr samples_in, nframes_t length_in, nframes_t samplerate_in,
                          SamplePtr & samples_out, nframes_t & length_out, nframes_t samplerate_out)
{
    SRC_DATA src_data;
    int error;

    src_data.input_frames = length_in;
    src_data.data_in = samples_in.get();

    src_data.src_ratio = static_cast<float>(samplerate_out) / static_cast<float>(samplerate_in);
    src_data.output_frames = std::max(static_cast<long>(length_in * src_data.src_ratio), 1L);

    samples_out.reset(new sample_t[src_data.output_frames]);
    src_data.data_out = samples_out.get();
    src_data.data_out[src_data.output_frames-1] = 0.0f;

    if ((error = src_simple(&src_data, SRC_SINC_BEST_QUALITY, 1)) != 0) {
        throw std::runtime_error(das::make_string() << "error converting samplerate: " << src_strerror(error));
    }

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
    if (factor == 1.0f || !_length) return;

    SamplePtr s;
    nframes_t l;

    resample(_samples, _length, (nframes_t)(_samplerate * factor), s, l, _samplerate);

    _samples = s;
    _length = l;
}


void AudioChunk::resample(nframes_t samplerate)
{
    if (samplerate == _samplerate) return;

    adjust_frequency(static_cast<float>(_samplerate) / static_cast<float>(samplerate));
    _samplerate = samplerate;
}

