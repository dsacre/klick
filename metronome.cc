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

#include "metronome.h"
#include "audio.h"

#include <cmath>
#include <cstring>

using namespace std;


Metronome::Metronome(AudioChunkPtr emphasis, AudioChunkPtr normal)
  : _click_emphasis(emphasis),
    _click_normal(normal)
{
}


Metronome::~Metronome()
{
    Audio->set_process_callback(NULL);
}


void Metronome::start()
{
    Audio->set_process_callback(this);
}


void Metronome::start_click(sample_t *buffer, nframes_t nframes,
                            nframes_t offset, AudioChunkPtr click, float volume)
{
    _play_click = click;
    _play_volume = volume;
    copy_audio(buffer + offset, _play_click->samples(),
                min(nframes - offset, _play_click->length()), _play_volume);
    _play_sample = nframes - offset;

    // finished already?
    if (_play_click->length() <= nframes - offset) {
        _play_click.reset();
    }
}


void Metronome::continue_click(sample_t *buffer, nframes_t nframes)
{
    if (_play_click) {
        // continue previous click
        copy_audio(buffer, _play_click->samples() + _play_sample,
                   min(nframes, _play_click->length() - _play_sample), _play_volume);
        _play_sample += nframes;

        // finished?
        if (_play_sample >= _play_click->length()) {
            _play_click.reset();
        }
    }
}


void Metronome::copy_audio(sample_t *dest, const sample_t *src, nframes_t length, float volume)
{
    if (volume == 1.0f) {
        memcpy(dest, src, length * sizeof(sample_t));
    } else {
        for (sample_t *end = dest + length; dest < end; dest++, src++) {
            *dest = *src * volume;
        }
    }
}

