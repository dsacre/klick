/*
 * klick - an advanced metronome for jack
 *
 * Copyright (C) 2008  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef _OSC_INTERFACE_HH
#define _OSC_INTERFACE_HH

#include <string>
#include <vector>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/any.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/noncopyable.hpp>

#include <lo/lo_types.h>


class OSCInterface
  : boost::noncopyable
{
  public:
    class Address {
      public:
        Address() : _addr(NULL) { }
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

    struct Message {
        std::string path;
        std::string types;
        std::vector<boost::any> args;
        Address src;
    };

    typedef boost::function<void(Message const &)> Callback;


    OSCInterface(std::string const & port);
    virtual ~OSCInterface();


    template <typename T>
    void add_method(char const *path, char const *types, T *obj,
                    void (T::*func)(Message const &)) {
        add_method_(path, types, boost::bind(func, obj, _1));
    }

    void add_method_(char const * path, char const * types, Callback const & func);

    void start();


    // basic send function
    void send(Address const & target, std::string const & path,
              std::vector<boost::any> const & args = std::vector<boost::any>());

    // address as string
    void send(std::string const & target, std::string const & path,
              std::vector<boost::any> const & args = std::vector<boost::any>()) {
        send(Address(target), path, args);
    }

    // allow multiple recipients
    template <typename T>
    void send(T const & targets, std::string const & path,
              std::vector<boost::any> const & args = std::vector<boost::any>()) {
        for (typename T::const_iterator i = targets.begin(); i != targets.end(); ++i) {
            send(*i, path, args);
        }
    }

    // allow arguments to be passed directly to send(), without manually filling a vector
    #define PP_PUSH_BACK(z, n, arg) args.push_back(arg ## n);
    #define PP_SEND(z, n, data) \
        template <typename A, BOOST_PP_ENUM_PARAMS(n, typename T)> \
        void send(A target, std::string const & path, BOOST_PP_ENUM_BINARY_PARAMS(n, const T, & t)) { \
            std::vector<boost::any> args; \
            BOOST_PP_REPEAT_ ## z(n, PP_PUSH_BACK, t) \
            send(target, path, args); \
        }
    BOOST_PP_REPEAT_FROM_TO(1, 10, PP_SEND, ~)
    #undef PP_PUSH_BACK
    #undef PP_SEND


    std::string const & url() const { return _url; }

  private:
    static int callback_(char const *path, char const *types, lo_arg **argv, int argc, lo_message msg, void *data);

    lo_server_thread _thread;
    std::string _url;

    std::vector<Callback*> _callbacks;
};


#endif // _OSC_INTERFACE_HH
