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

#ifndef _MAIN_HH
#define _MAIN_HH

#include <exception>


class Exit : public std::exception {
  public:
    Exit(int status) : _status(status) { }
    virtual ~Exit() throw () { }
    int status() const throw() { return _status; }
  protected:
    int _status;
};


#endif // _MAIN_HH