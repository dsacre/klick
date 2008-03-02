/*
 * Copyright (C) 2007  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "util/logstream.h"
#include "util/string.h"

#include <iostream>
#include <sstream>
#include <cstring>

namespace das {

logstream logv(std::cout);


std::string indent(const std::string & s, int n) {
    char buf[n + 256];
    std::memset(buf, ' ', n);
    std::istringstream is(s);
    std::ostringstream os;
    while (true) {
        is.getline(buf + n, 256);
        if (is.eof()) break;
        os << buf << std::endl;
    }
    return os.str();
}


} // namespace das
