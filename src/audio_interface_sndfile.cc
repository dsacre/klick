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

#include "audio_interface_sndfile.hh"

#include <algorithm>
#include <cstring>
#include <cctype>
#include <sndfile.h>

#include "util/string.hh"


AudioInterfaceSndfile::AudioInterfaceSndfile(std::string const & filename, nframes_t samplerate)
  : _samplerate(samplerate)
{
    SF_INFO sfinfo;
    std::memset(&sfinfo, 0, sizeof(sfinfo));

    sfinfo.samplerate = samplerate;
    sfinfo.channels = 1;

    // detect desired file format based on filename extension
    std::string ext = get_filename_extension(filename);
    if (ext == "wav") {
        sfinfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    } else if (ext == "aiff" || ext == "aif") {
        sfinfo.format = SF_FORMAT_AIFF | SF_FORMAT_PCM_16;
    } else if (ext == "flac") {
        sfinfo.format = SF_FORMAT_FLAC | SF_FORMAT_PCM_16;
#ifdef HAVE_SNDFILE_OGG
    } else if (ext == "ogg" || ext == "oga") {
        sfinfo.format = SF_FORMAT_OGG | SF_FORMAT_VORBIS;
#endif
    } else if (ext == "raw") {
        sfinfo.format = SF_FORMAT_RAW | SF_FORMAT_PCM_16;
    } else {
        throw AudioError(das::make_string() << "failed to recognize file extension '" << ext << "'");
    }

    // open output file for writing
    SNDFILE *f = sf_open(filename.c_str(), SFM_WRITE, &sfinfo);
    if (!f) {
        throw AudioError(das::make_string() << "couldn't open '" << filename << "' for output");
    }
    _sndfile.reset(f, sf_close);
}


void AudioInterfaceSndfile::process(std::size_t buffer_size)
{
    sample_t buffer[buffer_size];
    std::fill(buffer, buffer+buffer_size, 0.0f);

    // run process callback (metronome)
    _process_cb(buffer, buffer_size);
    // mix audio data to buffer
    process_mix(buffer, buffer_size);

    // write to output file
    sf_writef_float(_sndfile.get(), buffer, buffer_size);
}


std::string AudioInterfaceSndfile::get_filename_extension(std::string const & filename)
{
    std::string::size_type period = filename.find_last_of('.');
    if (period == std::string::npos) {
        return "";
    }

    std::string ext(filename, period + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    return ext;
}
