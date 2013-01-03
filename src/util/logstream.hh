/*
 * Copyright (C) 2007-2008  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef DAS_UTIL_LOGSTREAM_HH
#define DAS_UTIL_LOGSTREAM_HH

#include <sstream>


namespace das {


class logstream
{
  public:
    logstream(std::ostream & s, bool b = true)
      : _stream(s), _enabled(b)
    {
    }

    void enable(bool b) {
        _enabled = b;
    }

    template <typename T>
    logstream & operator<< (T const & p) {
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


} // namespace das


#endif // DAS_UTIL_LOGSTREAM_HH
