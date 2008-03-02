/*
 * Copyright (C) 2007  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _DAS_WATCHED_DUMMY_H
#define _DAS_WATCHED_DUMMY_H


namespace das {


template <typename T>
class watched
{
  public:
    watched() : _val() { }
    watched(const T& v) : _val(v) { }

    operator T() const { return _val; }

    T& operator=(const T& v) {
        _val = v;
        return _val;
    }

    watched& operator=(const watched& w) {
        _val = w._val;
    }

  private:
    T _val;
};


} // namespace das

#endif // _DAS_WATCHED_DUMMY_H
