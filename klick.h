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

#ifndef _KLICK_H
#define _KLICK_H

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/noncopyable.hpp>

#include "audio.h"


class Options;
class AudioInterface;
class TempoMap;
class Metronome;


class Klick
  : boost::noncopyable
{
  public:
    Klick(int argc, char *argv[]);
    ~Klick();

    void load_tempomap();
    void load_samples();

    void run();
    void signal_quit();

  private:
//    Options _options;
    boost::scoped_ptr<Options> _options;

    AudioChunkPtr _click_emphasis;
    AudioChunkPtr _click_normal;

    boost::shared_ptr<TempoMap> _map;
    boost::shared_ptr<AudioInterface> _audio;
    boost::shared_ptr<Metronome> _metro;

    volatile bool _quit;
};


#endif // _KLICK_H
