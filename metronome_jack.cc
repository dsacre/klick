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

#include "metronome_jack.h"
#include "audio_interface.h"
#include "audio_chunk.h"

#include <jack/jack.h>
#include <jack/transport.h>


void MetronomeJack::process_callback(sample_t *buffer, nframes_t nframes)
{
    if (!Audio->transport_rolling()) return;

    jack_position_t pos = Audio->position();

    if (!(pos.valid & JackPositionBBT)) {
        // not much we can do, but at least continue any click that's already playing
//        continue_click(buffer, nframes);
        return;
    }

    // convert BBT position to a frame number in this period
    double frames_per_beat = Audio->samplerate() * 60.0f / pos.beats_per_minute;
    nframes_t offset = (nframes_t)(frames_per_beat * (1.0f - (pos.tick / pos.ticks_per_beat)));

    if (offset % (nframes_t)frames_per_beat == 0) {
        // click starts at first frame. pos already refers to this beat
        bool emphasis = (pos.beat % (int32_t)pos.beats_per_bar == 1);
        AudioChunkPtr click = emphasis ? _click_emphasis : _click_normal;
        Audio->play(click, 0);
    }
    else if (offset < nframes) {
        // click starts somewhere during this period. since pos is the position at the start
        // of the period, the click played is actually at "pos + 1"
        bool emphasis = (pos.beat == (int32_t)pos.beats_per_bar);
        AudioChunkPtr click = emphasis ? _click_emphasis : _click_normal;
        Audio->play(click, offset);
    }
}
