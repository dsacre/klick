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

#include "metronome.hh"
#include "audio_interface_jack.hh"

#include "util/debug.hh"


Metronome::Metronome(AudioInterface & audio)
  : _audio(audio)
  , _active(false)
{
}

void Metronome::set_active(bool b)
{
    if (b) {
        do_start();
    } else {
        do_stop();
    }

    _active = b;
}


void Metronome::set_sound(AudioChunkConstPtr emphasis, AudioChunkConstPtr normal)
{
    _click_emphasis = emphasis;
    _click_normal = normal;
}


void Metronome::play_click(bool emphasis, nframes_t offset, float volume)
{
    ASSERT(_click_emphasis);
    ASSERT(_click_normal);

    AudioChunkConstPtr click = emphasis ? _click_emphasis : _click_normal;

    _audio.play(click, offset, volume);
}
