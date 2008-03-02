/*
 * Copyright (C) 2007  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _DAS_WATCHED_H
#define _DAS_WATCHED_H

//#include <boost/signal.hpp>
#include <sigc++/signal.h>
#include <sigc++/connection.h>


namespace das {


template <typename T>
class watched
{
  public:
    watched() : _val(), _sig() { }
    watched(const T& v) : _val(v), _sig() { }

    operator T() const { return _val; }

    T& operator=(const T& v) {
        T prev = _val;
        _val = v;
        _sig(_val, prev);
        return _val;
    }

    watched& operator=(const watched& w) {
        T prev = _val;
        _val = w._val;
        _sig(_val, prev);
    }

    // P should be T or const T&
    template <typename P>
    /*boost::signals::connection*/ sigc::connection connect(void (&f)(P, P)) {
        return _sig.connect(f);
    }
    template <typename P>
    /*boost::signals::connection*/ sigc::connection connect(void (&f)(P)) {
        signal_adapter a(f);
        return _sig.connect(a);
    }

  private:
    // discards the signal's second parameter
    class signal_adapter {
      public:
        signal_adapter(void (&f)(T)) : _f(f) { }
        void operator()(const T& v, const T& p) { _f(v); }
      private:
        void (&_f)(T);
    };

    T _val;
//    boost::signal<void (const T&, const T&)> _sig;
    sigc::signal<void, const T&, const T&> _sig;
};


} // namespace das

#endif // _DAS_WATCHED_H
