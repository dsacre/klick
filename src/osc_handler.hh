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

#ifndef KLICK_OSC_HANDLER_HH
#define KLICK_OSC_HANDLER_HH

#include <string>
#include <list>
#include <memory>
#include <boost/noncopyable.hpp>

#include "klick.hh"
#include "osc_interface.hh"
#include "audio_interface_jack.hh"


class OSCHandler
  : boost::noncopyable
{
  public:
    OSCHandler(std::string const & port,
               std::string const & return_port,
               Klick & klick,
               AudioInterfaceJack & audio);
    ~OSCHandler();

    void start();
    void update();

  private:
    typedef OSCInterface::Message Message;
    typedef void (OSCHandler::*MessageHandler)(Message const &);

    void add_method(char const *path, char const *types, MessageHandler func);
    template <typename M>
    void add_method(char const *path, char const *types, MessageHandler func);

    void generic_callback(MessageHandler func, Message const & msg);
    template <typename M>
    void type_specific_callback(MessageHandler func, Message const & msg);

    OSCInterface::Address optional_address(OSCInterface::Message const & msg, std::size_t i = 0);


    std::shared_ptr<class Metronome> metro() { return _klick.metronome(); }

    std::shared_ptr<class MetronomeSimple> metro_simple() {
        return std::dynamic_pointer_cast<MetronomeSimple>(_klick.metronome());
    }
    std::shared_ptr<class MetronomeMap> metro_map() {
        return std::dynamic_pointer_cast<MetronomeMap>(_klick.metronome());
    }
    std::shared_ptr<class MetronomeJack> metro_jack() {
        return std::dynamic_pointer_cast<MetronomeJack>(_klick.metronome());
    }


    void on_ping(Message const &);
    void on_check(Message const &);
    void on_register_client(Message const &);
    void on_unregister_client(Message const &);
    void on_query(Message const &);
    void on_quit(Message const &);

    void on_config_set_sound(Message const &);
    void on_config_set_sound_custom(Message const &);
    void on_config_set_sound_volume(Message const &);
    void on_config_set_sound_pitch(Message const &);
    void on_config_set_volume(Message const &);
    void on_config_connect(Message const &);
    void on_config_autoconnect(Message const &);
    void on_config_disconnect_all(Message const &);
    void on_config_get_available_ports(Message const &);
    void on_config_query(Message const &);

    void on_metro_set_type(Message const &);
    void on_metro_start(Message const &);
    void on_metro_stop(Message const &);
    void on_metro_query(Message const &);

    void on_simple_set_tempo(Message const &);
    void on_simple_set_tempo_increment(Message const &);
    void on_simple_set_tempo_start(Message const &);
    void on_simple_set_tempo_limit(Message const &);
    void on_simple_set_meter(Message const &);
    void on_simple_set_pattern(Message const &);
    void on_simple_tap(Message const &);
    void on_simple_query(Message const &);

    void on_map_load_file(Message const &);
    void on_map_set_preroll(Message const &);
    void on_map_set_tempo_multiplier(Message const &);
    void on_map_query(Message const &);

    void on_jack_query(Message const &);

    void fallback(Message const &);


    std::shared_ptr<OSCInterface> _osc;

    Klick & _klick;
    AudioInterfaceJack & _audio;

    typedef std::list<OSCInterface::Address> ClientList;
    ClientList _clients;


    float _current_tempo;
};


#endif // KLICK_OSC_HANDLER_HH
