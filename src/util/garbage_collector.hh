/*
 * Copyright (C) 2015  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef DAS_UTIL_GARBAGE_COLLECTOR_HH
#define DAS_UTIL_GARBAGE_COLLECTOR_HH

#include <memory>
#include <list>
#include <algorithm>
#include <boost/noncopyable.hpp>


namespace das {


/*
 * simple garbage collector that deletes objects held by a shared pointer
 * once only its own reference to the object remains.
 */
class garbage_collector
  : boost::noncopyable
{
  public:

    void manage(std::shared_ptr<void> p)
    {
        _pointers.push_back(p);
    }

    void collect()
    {
        auto it = std::remove_if(_pointers.begin(), _pointers.end(),
                         [](std::shared_ptr<void> p) { return p.unique(); });
        _pointers.erase(it, _pointers.end());
    }

    std::list<std::shared_ptr<void>> _pointers;
};


} // namespace das


#endif // DAS_UTIL_GARBAGE_COLLECTOR_HH
