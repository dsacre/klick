/*
 * Copyright (C) 2008  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef DAS_UTIL_GARBAGE_COLLECTOR_HH
#define DAS_UTIL_GARBAGE_COLLECTOR_HH

#include "disposable.hh"
#include "jack_ringbuffer.hh"
#include "debug.hh"

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>

#include <pthread.h>


namespace das {


/*
 * explicitly driven garbage collector, allowing objects to be deleted from a realtime thread.
 * since jack_ringbuffer is single-writer, only one thread is allowed to queue deletions.
 */
class garbage_collector
  : boost::noncopyable
{
  public:
    garbage_collector(std::size_t size = 255, pthread_t tid = 0)
      : disposer(boost::bind(&garbage_collector::dispose, this, _1))
      , _rb(size)
      , _tid(tid)
    {
    }

    ~garbage_collector()
    {
    }

    // set the thread whose deletions are to be handled by the garbage collector
    void set_thread(pthread_t tid) {
        _tid = tid;
    }

    // if called from the realtime thread, queue object for deletion by the garbage collector,
    // otherwise delete immediately
    void dispose(disposable * p) {
        if (pthread_self() != _tid) {
            delete p;
        } else {
            queue_dispose(p);
        }
    }

    // queue object for deletion
    void queue_dispose(disposable * p) {
        VERIFY(_rb.write(p));
    }

    // collect the garbage and delete it
    void collect() {
        while (_rb.read_space()) {
            disposable * p;
            _rb.read(p);
            delete p;
        }
    }

    // functor that calls this->dispose(), useful as deleter for boost::shared_ptr
    boost::function<void (disposable *)> disposer;

  private:
    jack_ringbuffer<disposable *> _rb;
    pthread_t _tid;
};


} // namespace das


#endif // DAS_UTIL_GARBAGE_COLLECTOR_HH
