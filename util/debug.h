/*
 * Copyright (C) 2007  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _DAS_DEBUG_H
#define _DAS_DEBUG_H


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


#endif // _DAS_DEBUG_H
