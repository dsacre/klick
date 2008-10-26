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

#include "osc_interface.hh"

#include <iostream>
#include <sstream>
#include <cstdlib>

#include <lo/lo.h>

#include "util/logstream.hh"
#include "util/debug.hh"


OSCInterface::OSCInterface(std::string const & port)
{
    struct error_handler {
        static void func(int num, char const *msg, char const * /*where*/) {
            std::cerr << "liblo server error " << num << ": " << msg << std::endl;
        }
    };
    _thread = lo_server_thread_new(!port.empty() ? port.c_str() : NULL, error_handler::func);

    if (!_thread) {
        throw std::runtime_error("can't create OSC server thread");
    }

    char *tmp = lo_server_thread_get_url(_thread);
    _url = tmp;
    std::free(tmp);

    das::logv << "OSC server listening on: '" << _url << "'" << std::endl;
}


OSCInterface::~OSCInterface()
{
    lo_server_thread_stop(_thread);
    lo_server_thread_free(_thread);

    for (std::vector<Callback*>::iterator i = _callbacks.begin(); i != _callbacks.end(); ++i) {
        delete *i;
    }
}


int OSCInterface::callback_(char const *path, char const *types, lo_arg **argv, int argc, lo_message msg, void *data)
{
    Callback & cb = *static_cast<Callback *>(data);

    Message m;

    m.path = path;
    m.types = types;
    char *tmp = lo_address_get_url(lo_message_get_source(msg));
    m.src = Address(tmp);
    std::free(tmp);

    das::logv << "got message: " << path << " ," << types;

    for (int i = 0; i < argc; ++i) {
        das::logv << " ";
        switch (types[i]) {
          case 'i':
            m.args.push_back(argv[i]->i);
            das::logv << argv[i]->i;
            break;
          case 'f':
            m.args.push_back(argv[i]->f);
            das::logv << argv[i]->f;
            break;
          case 'd':
            m.args.push_back(argv[i]->d);
            das::logv << argv[i]->d;
            break;
          case 's':
            m.args.push_back(std::string(&argv[i]->s));
            das::logv << &argv[i]->s;
            break;
          default:
            FAIL();
            return 0;
        }
    }

    das::logv << std::endl;

    cb(m);

    return 0;
}


void OSCInterface::add_method_(char const * path, char const * types, Callback const & func)
{
    Callback * cb = new Callback(func);
    _callbacks.push_back(cb);
    lo_server_thread_add_method(_thread, path, types, &callback_, static_cast<void*>(cb));
}


void OSCInterface::start()
{
    lo_server_thread_start(_thread);
}


void OSCInterface::send(Address const & target, std::string const & path, std::vector<boost::any> const & args)
{
    lo_message msg = lo_message_new();

    for (std::vector<boost::any>::const_iterator i = args.begin(); i != args.end(); ++i) {
        if (i->type() == typeid(int)) {
            lo_message_add_int32(msg, boost::any_cast<int>(*i));
        } else if (i->type() == typeid(bool)) {
            lo_message_add_int32(msg, boost::any_cast<bool>(*i));  // bools in OSC suck, use int32 instead
        } else if (i->type() == typeid(float)) {
            lo_message_add_float(msg, boost::any_cast<float>(*i));
        } else if (i->type() == typeid(double)) {
            lo_message_add_double(msg, boost::any_cast<double>(*i));
        } else if (i->type() == typeid(std::string)) {
            lo_message_add_string(msg, boost::any_cast<std::string>(*i).c_str());
        } else {
            FAIL();
        }
    }

    lo_send_message_from(target.addr(), lo_server_thread_get_server(_thread), path.c_str(), msg);

    lo_message_free(msg);
}



OSCInterface::Address::Address(std::string const & url)
{
    std::istringstream ss(url);
    unsigned int i;

    if (ss >> i) {
        _addr = lo_address_new(NULL, url.c_str());
    } else {
        _addr = lo_address_new_from_url(url.c_str());
    }

    if (!_addr) {
        throw std::runtime_error("invalid OSC port/url");
    }
}


OSCInterface::Address::Address(Address const & a)
{
    _addr = lo_address_new_from_url(a.url().c_str());
}


OSCInterface::Address::~Address()
{
    if (_addr) {
        lo_address_free(_addr);
    }
}


OSCInterface::Address & OSCInterface::Address::operator=(Address const & a)
{
    if (this == &a) {
        return *this;
    }
    if (_addr) {
        lo_address_free(_addr);
    }
    _addr = lo_address_new_from_url(a.url().c_str());
    return *this;
}


bool OSCInterface::Address::operator==(Address const & a)
{
    return url() == a.url();
}


std::string OSCInterface::Address::url() const
{
    char *tmp = lo_address_get_url(_addr);
    std::string r = tmp;
    std::free(tmp);
    return r;
}
