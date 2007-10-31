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
#include <sstream>
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

    template <typename T>
    logstream & operator<< (const T & p) {
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


class make_string
{
  public:
    template <typename T>
    make_string & operator<< (T const& t) {
        _stream << t;
        return *this;
    }

    make_string & operator<< (std::ostream & (*pf)(std::ostream &)) {
        pf(_stream);
        return *this;
    }

    operator std::string() {
        return _stream.str();
    }

  private:
    std::ostringstream _stream;
};


template <typename T>
uint count_iter(const T t) {
    int c = 0;
    for (typename T::const_iterator i = t.begin(); i != t.end(); ++i) {
        c++;
    }
    return c;
}


#ifdef _DEBUG
    #include <assert.h>
    #define ASSERT(f) assert(f)
    #define VERIFY(f) assert(f)
    #define FAIL()    assert(false)
#else
    #define ASSERT(f) ((void)0)
    #define VERIFY(f) ((void)f)
    #define FAIL()    ((void)0)
#endif


#endif // _UTIL_H
