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

#include "util.h"

#include <sstream>
#include <cstring>

using namespace std;


namespace das {

logstream logv(std::cout);


string indent(const string & s, int n) {
    char buf[n + 256];
    memset(buf, ' ', n);
    istringstream is(s);
    ostringstream os;
    while (true) {
        is.getline(buf + n, 256);
        if (is.eof()) break;
        os << buf << endl;
    }
    return os.str();
}


} // namespace das
