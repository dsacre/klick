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

#include "osc_handler.hh"
#include "klick.hh"
#include "metronome.hh"
#include "metronome_simple.hh"
#include "metronome_map.hh"
#include "metronome_jack.hh"
#include "audio_interface.hh"
#include "tempomap.hh"

#include <iostream>

#include "util/debug.hh"
#include "util/logstream.hh"


typedef boost::shared_ptr<Metronome> MetronomePtr;
typedef boost::shared_ptr<MetronomeSimple> MetronomeSimplePtr;
typedef boost::shared_ptr<MetronomeMap> MetronomeMapPtr;
typedef boost::shared_ptr<MetronomeJack> MetronomeJackPtr;


OSCHandler::OSCHandler(std::string const & port,
                       std::string const & return_port,
                       Klick & klick,
                       AudioInterface & audio)
  : _osc(new OSCInterface(port))
  , _klick(klick)
  , _audio(audio)
{
    _osc->add_method("/klick/ping", "", this, &OSCHandler::on_ping);
    _osc->add_method("/klick/check","", this, &OSCHandler::on_check);
    _osc->add_method("/klick/register_client", "", this, &OSCHandler::on_register_client);
    _osc->add_method("/klick/register_client", "s", this, &OSCHandler::on_register_client);
    _osc->add_method("/klick/unregister_client", "", this, &OSCHandler::on_unregister_client);
    _osc->add_method("/klick/unregister_client", "s", this, &OSCHandler::on_unregister_client);
    _osc->add_method("/klick/query", "", this, &OSCHandler::on_query);
    _osc->add_method("/klick/quit", "", this, &OSCHandler::on_quit);

    _osc->add_method("/klick/config/set_sound", "i", this, &OSCHandler::on_config_set_sound);
    _osc->add_method("/klick/config/set_sound", "ss", this, &OSCHandler::on_config_set_sound_custom);
    _osc->add_method("/klick/config/set_volume", "f", this, &OSCHandler::on_config_set_volume);
    _osc->add_method("/klick/config/autoconnect", "", this, &OSCHandler::on_config_autoconnect);
    _osc->add_method("/klick/config/query", "", this, &OSCHandler::on_config_query);

    _osc->add_method("/klick/metro/set_type", "s", this, &OSCHandler::on_metro_set_type);
    _osc->add_method("/klick/metro/start", "", this, &OSCHandler::on_metro_start);
    _osc->add_method("/klick/metro/stop", "", this, &OSCHandler::on_metro_stop);
    _osc->add_method("/klick/metro/query", "", this, &OSCHandler::on_metro_query);

    _osc->add_method("/klick/simple/set_tempo", "f", this, &OSCHandler::on_simple_set_tempo);
    _osc->add_method("/klick/simple/set_meter", "ii", this, &OSCHandler::on_simple_set_meter);
    _osc->add_method("/klick/simple/tap", "", this, &OSCHandler::on_simple_tap);
    _osc->add_method("/klick/simple/tap", "d", this, &OSCHandler::on_simple_tap);
    _osc->add_method("/klick/simple/set_pattern", "s", this, &OSCHandler::on_simple_set_pattern);
    _osc->add_method("/klick/simple/query", "", this, &OSCHandler::on_simple_query);

    _osc->add_method("/klick/map/load_file", "s", this, &OSCHandler::on_map_load_file);
    _osc->add_method("/klick/map/query", "", this, &OSCHandler::on_map_query);

    _osc->add_method("/klick/jack/query", "", this, &OSCHandler::on_jack_query);

    if (!return_port.empty()) {
        _osc->send(return_port, "/klick/ready");
    }
}


OSCHandler::~OSCHandler()
{
    _osc.reset();
}


template <typename T>
boost::shared_ptr<T> OSCHandler::cast_metronome(std::string const & func)
{
    boost::shared_ptr<T> m = boost::dynamic_pointer_cast<T>(_klick.metronome());

    if (!m && !func.empty()) {
        std::cerr << "function " << func << " not available for current metronome type" << std::endl;
    }

    return m;
}


void OSCHandler::on_ping(Message const & msg)
{
    std::cout << "ping from " << msg.src.url() << std::endl;
    _osc->send(msg.src, "/klick/pong");
}


void OSCHandler::on_check(Message const & msg)
{
    _osc->send(msg.src, "/klick/ready");
}


void OSCHandler::on_register_client(Message const & msg)
{
    OSCInterface::Address addr;
    if (msg.args.empty()) {
        addr = msg.src;
    } else {
        addr = OSCInterface::Address(boost::any_cast<std::string>(msg.args[0]));
    }

    ClientList::iterator i = find(_clients.begin(), _clients.end(), addr);
    if (i == _clients.end()) {
        _clients.push_back(addr);
        das::logv << "client " << addr.url() << " registered" << std::endl;
    }
}


void OSCHandler::on_unregister_client(Message const & msg)
{
    OSCInterface::Address addr;
    if (msg.args.empty()) {
        addr = msg.src;
    } else {
        addr = OSCInterface::Address(boost::any_cast<std::string>(msg.args[0]));
    }

    ClientList::iterator i = find(_clients.begin(), _clients.end(), addr);
    if (i != _clients.end()) {
        _clients.erase(i);
        das::logv << "client " << addr.url() << " unregistered" << std::endl;
    }
}


void OSCHandler::on_query(Message const & msg)
{
    on_config_query(msg);
    on_metro_query(msg);

    MetronomePtr m = _klick.metronome();

    if (boost::dynamic_pointer_cast<MetronomeSimple>(m)) {
        on_simple_query(msg);
    }
    else if (boost::dynamic_pointer_cast<MetronomeMap>(m)) {
        on_map_query(msg);
    }
    else if (boost::dynamic_pointer_cast<MetronomeJack>(m)) {
        on_jack_query(msg);
    }
    else {
        FAIL();
    }
}


void OSCHandler::on_quit(Message const & /*msg*/)
{
    _klick.signal_quit();
}



void OSCHandler::on_config_set_sound(Message const & msg)
{
    _klick.set_sound(boost::any_cast<int>(msg.args[0]));
    _osc->send(_clients, "/klick/config/sound", _klick.sound());
}


void OSCHandler::on_config_set_sound_custom(Message const & msg)
{
    _klick.set_sound_custom(boost::any_cast<std::string>(msg.args[0]), boost::any_cast<std::string>(msg.args[1]));
    _osc->send(_clients, "/klick/config/sound", _klick.sound_custom_emphasis(), _klick.sound_custom_normal());
}


void OSCHandler::on_config_set_volume(Message const & msg)
{
    _audio.set_volume(boost::any_cast<float>(msg.args[0]));
    _osc->send(_clients, "/klick/config/volume", _audio.volume());
}


void OSCHandler::on_config_autoconnect(Message const & /*msg*/)
{
    _audio.autoconnect();
}


void OSCHandler::on_config_query(Message const & msg)
{
    if (_klick.sound() != -1) {
        _osc->send(msg.src, "/klick/config/sound", _klick.sound());
    } else {
        _osc->send(msg.src, "/klick/config/sound", _klick.sound_custom_emphasis(), _klick.sound_custom_normal());
    }
    _osc->send(msg.src, "/klick/config/volume", _audio.volume());
}



void OSCHandler::on_metro_set_type(Message const & msg)
{
    std::string type = boost::any_cast<std::string>(msg.args[0]);

    if (type == "simple") {
        _klick.set_metronome(Klick::METRONOME_TYPE_SIMPLE);
    }
    else if (type == "map") {
        _klick.set_metronome(Klick::METRONOME_TYPE_MAP);
    }
    else if (type == "jack") {
        _klick.set_metronome(Klick::METRONOME_TYPE_JACK);
    }
    else {
        std::cerr << "invalid metronome type " << type << std::endl;
    }
}


void OSCHandler::on_metro_start(Message const & /*msg*/)
{
    MetronomePtr m = _klick.metronome();
    m->start();
    _osc->send(_clients, "/klick/metro/active", m->active());
}


void OSCHandler::on_metro_stop(Message const & /*msg*/)
{
    MetronomePtr m = _klick.metronome();
    m->stop();
    _osc->send(_clients, "/klick/metro/active", m->active());
}


void OSCHandler::on_metro_query(Message const & msg)
{
    MetronomePtr m = _klick.metronome();

    if (boost::dynamic_pointer_cast<MetronomeSimple>(m)) {
        _osc->send(msg.src, "/klick/metro/type", std::string("simple"));
    }
    else if (boost::dynamic_pointer_cast<MetronomeMap>(m)) {
        _osc->send(msg.src, "/klick/metro/type", std::string("map"));
    }
    else if (boost::dynamic_pointer_cast<MetronomeJack>(m)) {
        _osc->send(msg.src, "/klick/metro/type", std::string("jack"));
    }
    else {
        FAIL();
    }

    _osc->send(msg.src, "/klick/metro/active", m->active());
}



void OSCHandler::on_simple_set_tempo(Message const & msg)
{
    MetronomeSimplePtr m = cast_metronome<MetronomeSimple>(msg.path);
    if (m) {
        m->set_tempo(boost::any_cast<float>(msg.args[0]));
        _osc->send(_clients, "/klick/simple/tempo", m->tempo());
    }
}


void OSCHandler::on_simple_set_meter(Message const & msg)
{
    MetronomeSimplePtr m = cast_metronome<MetronomeSimple>(msg.path);
    if (m) {
        m->set_meter(boost::any_cast<int>(msg.args[0]), boost::any_cast<int>(msg.args[1]));
        _osc->send(_clients, "/klick/simple/meter", m->beats(), m->denom());
    }
}


void OSCHandler::on_simple_tap(Message const & msg)
{
    MetronomeSimplePtr m = cast_metronome<MetronomeSimple>(msg.path);
    if (m) {
        if (!msg.args.empty()) {
            m->tap(boost::any_cast<double>(msg.args[0]));
        } else {
            m->tap();
        }
        _osc->send(_clients, "/klick/simple/tempo", m->tempo());
    }
}


void OSCHandler::on_simple_set_pattern(Message const & msg)
{
    MetronomeSimplePtr m = cast_metronome<MetronomeSimple>(msg.path);
    if (m) {
        try {
            TempoMap::Pattern p = TempoMap::parse_pattern(boost::any_cast<std::string>(msg.args[0]), std::max(1, m->beats()));
            m->set_pattern(p);
        } catch (TempoMap::ParseError & e) {
            std::cout << e.what() << std::endl;
        }
        _osc->send(_clients, "/klick/simple/pattern", TempoMap::pattern_to_string(m->pattern()));
    }
}


void OSCHandler::on_simple_query(Message const & msg)
{
    MetronomeSimplePtr m = cast_metronome<MetronomeSimple>(msg.path);
    if (m) {
        _osc->send(msg.src, "/klick/simple/tempo", m->tempo());
        _osc->send(msg.src, "/klick/simple/meter", m->beats(), m->denom());
        _osc->send(msg.src, "/klick/simple/pattern", TempoMap::pattern_to_string(m->pattern()));
    }
}


void OSCHandler::on_map_load_file(Message const & msg)
{
    MetronomeMapPtr m = cast_metronome<MetronomeMap>(msg.path);
    if (m) {
        try {
            _klick.set_tempomap_filename(boost::any_cast<std::string>(msg.args[0]));
        } catch (std::runtime_error & e) {
            std::cout << e.what() << std::endl;
        }

        _osc->send(_clients, "/klick/map/file", _klick.tempomap_filename());
    }
}


void OSCHandler::on_map_query(Message const & msg)
{
    MetronomeMapPtr m = cast_metronome<MetronomeMap>(msg.path);
    if (m) {
        _osc->send(msg.src, "/klick/map/file", _klick.tempomap_filename());
    }
}


void OSCHandler::on_jack_query(Message const & /*msg*/)
{
    // nothing
}
