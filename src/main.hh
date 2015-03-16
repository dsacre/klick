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

#ifndef KLICK_MAIN_HH
#define KLICK_MAIN_HH

#include <exception>
#include <string>

#include "util/logstream.hh"


class Exit : public std::exception
{
  public:
    Exit(int status) : _status(status) { }
    virtual ~Exit() throw() { }

    int status() const { return _status; }

  protected:
    int _status;
};


std::string data_file(std::string const & path);


extern das::logstream logv;


#ifdef __clang__
#define REALTIME __attribute__((annotate("realtime")))
#define NONREALTIME __attribute__((annotate("nonrealtime")))
#else
#define REALTIME
#define NONREALTIME
#endif


#endif // KLICK_MAIN_HH
