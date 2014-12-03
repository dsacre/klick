/*
 * klick - an advanced metronome for jack
 *
 * Copyright (C) 2008-2013  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef KLICK_OSC_INTERFACE_HH
#define KLICK_OSC_INTERFACE_HH

#include <string>
#include <vector>
#include <list>
#include <functional>
#include <type_traits>
#include <stdexcept>

#include <boost/variant.hpp>
#include <boost/noncopyable.hpp>

#include <lo/lo_types.h>


class OSCInterface
  : boost::noncopyable
{
  public:

    struct OSCError : public std::runtime_error
    {
        OSCError(std::string const & w)
          : std::runtime_error(w) { }
    };

    class Address
    {
      public:
        Address(Address const &);
        Address(std::string const & url);
        ~Address();

        Address & operator=(Address const &);

        bool operator==(Address const &);

        lo_address addr() const { return _addr; }
        std::string url() const;

      private:
        lo_address _addr;
    };

    typedef boost::variant<int, float, double, std::string> ArgumentVariant;
    typedef std::vector<ArgumentVariant> ArgumentVector;

    struct Message
    {
        Message(std::string const & path_, std::string const & types_, Address const & src_)
          : path(path_)
          , types(types_)
          , src(src_)
        { }

        std::string path;
        std::string types;
        ArgumentVector args;
        Address src;
    };

    typedef std::function<void (Message const &)> Callback;


    OSCInterface(std::string const & port);
    virtual ~OSCInterface();

    void add_method(char const *path, char const *types, Callback const & cb);

    void start();
    void stop();


    // basic send function
    void send(Address const & target, std::string const & path, ArgumentVector const & args = {});

    // allow multiple recipients
    void send(std::list<Address> const & targets, std::string const & path, ArgumentVector const & args = {}) {
        for (auto & t : targets) {
            send(t, path, args);
        }
    }

    // allow arguments to be passed directly to send(), without manually
    // filling a vector
    template <typename T, typename Head, typename... Tail>
    typename std::enable_if<!std::is_same<Head, ArgumentVector>::value, void>::type
    send(T && targets, std::string const & path, Head head, Tail... tail) {
        ArgumentVector v = {head, tail...};
        send(std::forward<T>(targets), path, v);
    }


    std::string const & url() const { return _url; }


  private:

    static int callback_(char const *path, char const *types, lo_arg **argv, int argc, lo_message msg, void *data);

    lo_server_thread _thread;
    std::string _url;

    std::list<Callback> _callbacks;
};


#endif // KLICK_OSC_INTERFACE_HH
