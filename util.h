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

#ifndef _UTIL_H
#define _UTIL_H

#include <string>
#include <iostream>
#include <boost/noncopyable.hpp>


template <typename T, T **pp>
class global_object
  : boost::noncopyable
{
  protected:
    global_object() {
        *pp = static_cast<T*>(this);
    }
    ~global_object() {
        *pp = NULL;
    }
};


class logstream
{
  public:
    logstream(std::ostream & s)
      : _stream(s), _enabled(true)
    {
    }
    void enable(bool b) {
        _enabled = b;
    }
    template <typename T> logstream & operator<< (const T & p) {
        if (_enabled) _stream << p;
        return *this;
    }
    logstream & operator<< (std::ostream & (*pf)(std::ostream &)) {
        if (_enabled) pf(_stream);
        return *this;
    }

  private:
    std::ostream & _stream;
    bool _enabled;
};

extern logstream logv;


std::string indent(const std::string & s, uint n);

#endif // _UTIL_H
