/*
 * klick - an advanced metronome for jack
 *
 * Copyright (C) 2007-2009  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _AUDIO_HH
#define _AUDIO_HH

#include <jack/types.h>
#include <jack/transport.h>
#include <boost/shared_ptr.hpp>

typedef jack_default_audio_sample_t sample_t;
typedef jack_nframes_t nframes_t;
typedef jack_position_t position_t;

typedef boost::shared_ptr<class AudioChunk> AudioChunkPtr;
typedef boost::shared_ptr<class AudioChunk const> AudioChunkConstPtr;


#endif // _AUDIO_HH
