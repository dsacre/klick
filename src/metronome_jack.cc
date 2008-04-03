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

#include "metronome_jack.hh"
#include "audio_interface.hh"
#include "audio_chunk.hh"

#include <jack/jack.h>
#include <jack/transport.h>


MetronomeJack::MetronomeJack(AudioChunkConstPtr emphasis,
                             AudioChunkConstPtr normal)
  : Metronome(emphasis, normal),
    _last_click_frame(0)
{
}


MetronomeJack::~MetronomeJack()
{
}


void MetronomeJack::process_callback(sample_t * /*buffer*/, nframes_t nframes)
{
    if (!Audio->transport_rolling()) {
        return;
    }

    jack_position_t pos = Audio->position();

    if (!(pos.valid & JackPositionBBT)) {
        // not much we can do
        return;
    }

    // make sure the transport master provided us with sane position info
    ASSERT(pos.beats_per_minute > 0.0);
    ASSERT(pos.beats_per_bar > 0.0);
    ASSERT(pos.ticks_per_beat > 0.0);
    ASSERT(pos.beat > 0 && pos.beat <= pos.beats_per_bar);
    ASSERT(pos.tick >= 0 && pos.tick < pos.ticks_per_beat);

    // convert BBT position to a frame number in this period
    double frames_per_beat = Audio->samplerate() * 60.0 / pos.beats_per_minute;
    nframes_t offset = (nframes_t)(frames_per_beat * (1.0 - (pos.tick / pos.ticks_per_beat)));
    bool emphasis;

    // avoid playing the same click twice due to rounding errors
    if (_last_click_frame && (pos.frame >= _last_click_frame) &&
        (pos.frame < _last_click_frame + MIN_FRAMES_DIFF)) {
        return;
    }

    if (offset % (nframes_t)frames_per_beat == 0) {
        // click starts at first frame. pos already refers to this beat
        offset = 0;
        emphasis = (pos.beat == 1);

        // deal with faulty timebase masters
        if ((pos.beat == pos.beats_per_bar + 1) ||
            (pos.beat == pos.beats_per_bar && pos.tick == pos.ticks_per_beat)) {
            emphasis = true;
        }

        _last_click_frame = pos.frame;
    }
    else if (offset < nframes) {
        // click starts somewhere during this period. since pos is the position at the start
        // of the period, the click played is actually at "pos + 1"
        emphasis = (pos.beat == (int)pos.beats_per_bar);

        _last_click_frame = pos.frame + offset;
    }
    else {
        // no click in this period
        return;
    }

    //std::cout << (pos.frame + offset) << ": " << emphasis << std::endl;

    play_click(emphasis, offset);
}
