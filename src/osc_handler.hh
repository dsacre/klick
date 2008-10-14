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

#ifndef _OSC_HANDLER_HH
#define _OSC_HANDLER_HH

#include <string>
#include <list>
#include <boost/shared_ptr.hpp>
#include <boost/any.hpp>
#include <boost/noncopyable.hpp>

#include "klick.hh"
#include "osc_interface.hh"
#include "audio_interface.hh"

class Metronome;
class MetronomeSimple;


class OSCHandler
  : boost::noncopyable
{
  public:
    OSCHandler(std::string const & port,
               std::string const & return_port,
               Klick & klick,
               AudioInterface & audio);
    ~OSCHandler();

    void start() {
        _osc->start();
    }

    void update();

  private:
    template <typename T>
    boost::shared_ptr<T> cast_metronome(std::string const & f = std::string()) const;

    typedef OSCInterface::Message Message;

    void on_ping(Message const &);
    void on_check(Message const &);
    void on_register_client(Message const &);
    void on_unregister_client(Message const &);
    void on_query(Message const &);
    void on_quit(Message const &);

    void on_config_set_sound(Message const &);
    void on_config_set_sound_custom(Message const &);
    void on_config_set_volume(Message const &);
    void on_config_autoconnect(Message const &);
    void on_config_query(Message const &);

    void on_metro_set_type(Message const &);
    void on_metro_start(Message const &);
    void on_metro_stop(Message const &);
    void on_metro_query(Message const &);

    void on_simple_set_tempo(Message const &);
    void on_simple_set_tempo_increment(Message const &);
    void on_simple_set_tempo_limit(Message const &);
    void on_simple_set_meter(Message const &);
    void on_simple_tap(Message const &);
    void on_simple_set_pattern(Message const &);
    void on_simple_query(Message const &);

    void on_map_load_file(Message const &);
    void on_map_query(Message const &);

    void on_jack_query(Message const &);


    boost::shared_ptr<OSCInterface> _osc;

    Klick & _klick;
    AudioInterface & _audio;

    typedef std::list<OSCInterface::Address> ClientList;
    ClientList _clients;


    float _current_tempo;
};


#endif // _OSC_HANDLER_HH
