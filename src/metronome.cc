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

#include "metronome.hh"
#include "audio_interface.hh"

#include "util/debug.hh"


Metronome::Metronome(AudioChunkConstPtr emphasis,
                     AudioChunkConstPtr normal)
  : _click_emphasis(emphasis),
    _click_normal(normal)
{
    ASSERT(_click_emphasis);
    ASSERT(_click_normal);
    ASSERT(_click_emphasis->samplerate() == Audio->samplerate());
    ASSERT(_click_normal->samplerate() == Audio->samplerate());
}


Metronome::~Metronome()
{
    Audio->set_process_callback(NULL);
}


void Metronome::start()
{
    Audio->set_process_callback(this, true);
}


void Metronome::play_click(bool emphasis, nframes_t offset, float volume)
{
    AudioChunkConstPtr click = emphasis ? _click_emphasis : _click_normal;

    Audio->play(click, offset, volume);
}
