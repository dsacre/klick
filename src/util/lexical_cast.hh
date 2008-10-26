/*
 * Copyright (C) 2008  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _DAS_LEXICAL_CAST_HH
#define _DAS_LEXICAL_CAST_HH

#include <boost/lexical_cast.hpp>
#include <stdexcept>

namespace das {


typedef boost::bad_lexical_cast bad_lexical_cast;


template<typename T, typename S>
T lexical_cast(S const & arg)
{
    return boost::lexical_cast<T>(arg);
}


template<typename T, typename S>
T lexical_cast(S const & arg, T const & fallback)
{
    try {
        return boost::lexical_cast<T>(arg);
    }
    catch (boost::bad_lexical_cast const &) {
        return fallback;
    }
}


template<typename T, typename S>
T lexical_cast(S const & arg, std::exception const & exc)
{
    try {
        return boost::lexical_cast<T>(arg);
    }
    catch (boost::bad_lexical_cast const &) {
        throw exc;
    }
}


} // namespace das


#endif // _DAS_LEXICAL_CAST_HH
