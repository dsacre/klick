/*
 * Copyright (C) 2007-2008  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "logstream.hh"
#include "string.hh"

#include <iostream>
#include <sstream>

namespace das {

logstream logv(std::cout);


std::string indent(std::string const & s, int n)
{
    std::string ws(n, ' ');
    std::istringstream is(s);
    std::ostringstream os;
    while (true) {
        std::string tmp;
        std::getline(is, tmp);
        if (is.eof()) break;
        os << ws << tmp << std::endl;
    }
    return os.str();
}


} // namespace das
