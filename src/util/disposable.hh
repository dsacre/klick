/*
 * Copyright (C) 2008  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef DAS_UTIL_DISPOSABLE_HH
#define DAS_UTIL_DISPOSABLE_HH


namespace das {


// base class for objects that can be garbage collected
class disposable
{
  public:
    disposable() { }
    virtual ~disposable() { }
};


} // namespace das


#endif // DAS_UTIL_DISPOSABLE_HH
