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
    Audio->set_process_callback(this, true);
}
