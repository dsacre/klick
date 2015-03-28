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

#ifndef KLICK_AUDIO_INTERFACE_SNDFILE_HH
#define KLICK_AUDIO_INTERFACE_SNDFILE_HH

#include "audio_interface.hh"

#include <string>
#include <memory>
#include <sndfile.h>


class AudioInterfaceSndfile
  : public AudioInterface
{
  public:

    AudioInterfaceSndfile(std::string const & filename, nframes_t samplerate);

    void process(std::size_t buffer_size);

    nframes_t samplerate() const { return _samplerate; }
    bool is_shutdown() const { return false; }

  private:

    std::string get_filename_extension(std::string const & filename);

    nframes_t _samplerate;
    std::shared_ptr<SNDFILE> _sndfile;
};


#endif // KLICK_AUDIO_INTERFACE_SNDFILE_HH

