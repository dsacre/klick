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

#ifndef KLICK_AUDIO_HH
#define KLICK_AUDIO_HH

#include <memory>

#include <jack/types.h>
#include <jack/transport.h>

typedef jack_default_audio_sample_t sample_t;
typedef jack_nframes_t nframes_t;
typedef jack_position_t position_t;

typedef std::shared_ptr<class AudioChunk> AudioChunkPtr;
typedef std::shared_ptr<class AudioChunk const> AudioChunkConstPtr;


#endif // KLICK_AUDIO_HH
