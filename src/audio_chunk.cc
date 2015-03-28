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

#include "audio_chunk.hh"

#include <sstream>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <algorithm>

#include <samplerate.h>
#include <sndfile.h>
#ifdef ENABLE_RUBBERBAND
    #include <rubberband/RubberBandStretcher.h>
#endif

#include "util/string.hh"
#include "util/debug.hh"


AudioChunk::AudioChunk(std::string const & filename, nframes_t samplerate)
{
    SF_INFO sfinfo;
    std::memset(&sfinfo, 0, sizeof(sfinfo));

    SNDFILE *f;

    if ((f = sf_open(filename.c_str(), SFM_READ, &sfinfo)) == NULL) {
        throw std::runtime_error(das::make_string() << "failed to open audio file '" << filename << "'");
    }

    _samples.reset(new sample_t[sfinfo.frames * sfinfo.channels]);
    _length = sfinfo.frames;
    _samplerate = sfinfo.samplerate;

    sf_readf_float(f, _samples.get(), sfinfo.frames);

    // convert stereo to mono
    if (sfinfo.channels == 2) {
        SamplePtr mono_samples(new sample_t[sfinfo.frames]);
        for (int i = 0; i < sfinfo.frames; ++i) {
            mono_samples[i] = (_samples[i*2] + _samples[i*2 + 1]) / 2;
        }
        _samples = std::move(mono_samples);
    }

    // convert samplerate
    if (_samplerate != samplerate) {
        resample(samplerate);
    }

    sf_close(f);
}


void AudioChunk::adjust_volume(float volume)
{
    if (volume == 1.0f) return;

    for (nframes_t i = 0; i < _length; ++i) {
        _samples[i] *= volume;
    }
}


void AudioChunk::adjust_pitch(float factor)
{
    if (factor == 1.0f || !_length) return;

#ifdef ENABLE_RUBBERBAND
    pitch_shift(factor);
#else
    nframes_t s = _samplerate;
    resample(static_cast<nframes_t>(_samplerate / factor));
    _samplerate = s;
#endif
}


void AudioChunk::resample(nframes_t samplerate)
{
    SRC_DATA src_data;
    int error;

    src_data.input_frames = _length;
    src_data.data_in = _samples.get();

    src_data.src_ratio = static_cast<float>(samplerate) / static_cast<float>(_samplerate);
    src_data.output_frames = std::max(static_cast<long>(_length * src_data.src_ratio), 1L);

    SamplePtr samples_new(new sample_t[src_data.output_frames]());
    src_data.data_out = samples_new.get();

    if ((error = src_simple(&src_data, SRC_SINC_BEST_QUALITY, 1)) != 0) {
        throw std::runtime_error(das::make_string() << "error converting samplerate: " << src_strerror(error));
    }

    _samples = std::move(samples_new);
    _length = src_data.output_frames;
    _samplerate = samplerate;
}


#ifdef ENABLE_RUBBERBAND

void AudioChunk::pitch_shift(float factor)
{
    nframes_t const BLOCK_SIZE = 1024;

    RubberBand::RubberBandStretcher rb(_samplerate, 1, RubberBand::RubberBandStretcher::PercussiveOptions);
    rb.setPitchScale(factor);
    rb.setExpectedInputDuration(_length);

    SamplePtr samples_new(new sample_t[_length]());
    sample_t *buf;
    nframes_t k = 0;

    // study all samples
    buf = _samples.get();
    rb.study(&buf, _length, true);

    for (nframes_t i = 0; i < _length; i += BLOCK_SIZE)
    {
        // process one block of samples
        buf = _samples.get() + i;
        nframes_t s = std::min(BLOCK_SIZE, _length - i);
        rb.process(&buf, s, i + BLOCK_SIZE >= _length);

        // retrieve available samples
        buf = samples_new.get() + k;
        s = std::min(static_cast<nframes_t>(rb.available()), _length - k);
        if (s > 0) {
            k += rb.retrieve(&buf, s);
        }
    }

    ASSERT(rb.available() == -1);

    _length = k;
    _samples = std::move(samples_new);
}

#endif
