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

#include "osc_handler.hh"
#include "main.hh"
#include "klick.hh"
#include "metronome.hh"
#include "metronome_simple.hh"
#include "metronome_map.hh"
#include "metronome_jack.hh"
#include "audio_interface_jack.hh"
#include "tempomap.hh"

#include <iostream>
#include <functional>
#include <algorithm>

#include "util/debug.hh"
#include "util/logstream.hh"


OSCHandler::OSCHandler(std::string const & port,
                       std::string const & return_port,
                       Klick & klick,
                       AudioInterfaceJack & audio)
  : _osc(new OSCInterface(port))
  , _klick(klick)
  , _audio(audio)
  , _current_tempo(0.0f)
{
    add_method("/klick/ping", "", &OSCHandler::on_ping);
    add_method("/klick/ping", "s", &OSCHandler::on_ping);
    add_method("/klick/check","", &OSCHandler::on_check);
    add_method("/klick/check","s", &OSCHandler::on_check);
    add_method("/klick/register_client", "", &OSCHandler::on_register_client);
    add_method("/klick/register_client", "s", &OSCHandler::on_register_client);
    add_method("/klick/unregister_client", "", &OSCHandler::on_unregister_client);
    add_method("/klick/unregister_client", "s", &OSCHandler::on_unregister_client);
    add_method("/klick/query", "", &OSCHandler::on_query);
    add_method("/klick/query", "s", &OSCHandler::on_query);
    add_method("/klick/quit", "", &OSCHandler::on_quit);

    add_method("/klick/config/set_sound", "i", &OSCHandler::on_config_set_sound);
    add_method("/klick/config/set_sound", "ss", &OSCHandler::on_config_set_sound_custom);
    add_method("/klick/config/set_sound_volume", "ff", &OSCHandler::on_config_set_sound_volume);
    add_method("/klick/config/set_sound_pitch", "ff", &OSCHandler::on_config_set_sound_pitch);
    add_method("/klick/config/set_volume", "f", &OSCHandler::on_config_set_volume);
    add_method("/klick/config/connect", NULL, &OSCHandler::on_config_connect);
    add_method("/klick/config/autoconnect", "", &OSCHandler::on_config_autoconnect);
    add_method("/klick/config/disconnect_all", "", &OSCHandler::on_config_disconnect_all);
    add_method("/klick/config/get_available_ports", "", &OSCHandler::on_config_get_available_ports);
    add_method("/klick/config/get_available_ports", "s", &OSCHandler::on_config_get_available_ports);
    add_method("/klick/config/query", "", &OSCHandler::on_config_query);
    add_method("/klick/config/query", "s", &OSCHandler::on_config_query);

    add_method("/klick/metro/set_type", "s", &OSCHandler::on_metro_set_type);
    add_method("/klick/metro/start", "", &OSCHandler::on_metro_start);
    add_method("/klick/metro/stop", "", &OSCHandler::on_metro_stop);
    add_method("/klick/metro/query", "", &OSCHandler::on_metro_query);
    add_method("/klick/metro/query", "s", &OSCHandler::on_metro_query);

    add_method<MetronomeSimple>("/klick/simple/set_tempo", "f", &OSCHandler::on_simple_set_tempo);
    add_method<MetronomeSimple>("/klick/simple/set_tempo_increment", "f", &OSCHandler::on_simple_set_tempo_increment);
    add_method<MetronomeSimple>("/klick/simple/set_tempo_start", "f", &OSCHandler::on_simple_set_tempo_start);
    add_method<MetronomeSimple>("/klick/simple/set_tempo_limit", "f", &OSCHandler::on_simple_set_tempo_limit);
    add_method<MetronomeSimple>("/klick/simple/set_meter", "ii", &OSCHandler::on_simple_set_meter);
    add_method<MetronomeSimple>("/klick/simple/tap", "", &OSCHandler::on_simple_tap);
    add_method<MetronomeSimple>("/klick/simple/tap", "d", &OSCHandler::on_simple_tap);
    add_method<MetronomeSimple>("/klick/simple/set_pattern", "s", &OSCHandler::on_simple_set_pattern);
    add_method<MetronomeSimple>("/klick/simple/query", "", &OSCHandler::on_simple_query);
    add_method<MetronomeSimple>("/klick/simple/query", "s", &OSCHandler::on_simple_query);

    add_method<MetronomeMap>("/klick/map/load_file", "s", &OSCHandler::on_map_load_file);
    add_method<MetronomeMap>("/klick/map/set_preroll", "i", &OSCHandler::on_map_set_preroll);
    add_method<MetronomeMap>("/klick/map/set_tempo_multiplier", "f", &OSCHandler::on_map_set_tempo_multiplier);
    add_method<MetronomeMap>("/klick/map/query", "", &OSCHandler::on_map_query);
    add_method<MetronomeMap>("/klick/map/query", "s", &OSCHandler::on_map_query);

    add_method<MetronomeJack>("/klick/jack/query", "", &OSCHandler::on_jack_query);
    add_method<MetronomeJack>("/klick/jack/query", "s", &OSCHandler::on_jack_query);

    add_method(NULL, NULL, &OSCHandler::fallback);

    if (!return_port.empty()) {
        _osc->send(return_port, "/klick/ready");
    }
}


OSCHandler::~OSCHandler()
{
    _osc->stop();
}


void OSCHandler::start()
{
    _osc->start();
}


void OSCHandler::update()
{
    auto m = metro_simple();
    if (m) {
        if (_current_tempo != m->current_tempo()) {
            _current_tempo = m->current_tempo();
            _osc->send(_clients, "/klick/simple/current_tempo", _current_tempo);
        }
    }
}


void OSCHandler::add_method(char const *path, char const *types, MessageHandler func)
{
    _osc->add_method(path, types, std::bind(&OSCHandler::generic_callback,
                                            this, func, std::placeholders::_1));
}


template <typename M>
void OSCHandler::add_method(char const *path, char const *types, MessageHandler func)
{
    _osc->add_method(path, types, std::bind(&OSCHandler::type_specific_callback<M>,
                                            this, func, std::placeholders::_1));
}


void OSCHandler::generic_callback(MessageHandler func, Message const & msg)
{
    try {
        (this->*func)(msg);
    }
    catch (OSCInterface::OSCError const & e) {
        std::cerr << msg.path << ": " << e.what() << std::endl;
    }
}


template <typename M>
void OSCHandler::type_specific_callback(MessageHandler func, Message const & msg)
{
    try {
        if (std::dynamic_pointer_cast<M>(metro())) {
            (this->*func)(msg);
        } else {
            std::cerr << msg.path << ": function not available for current metronome type" << std::endl;
        }
    }
    catch (OSCInterface::OSCError const & e) {
        std::cerr << msg.path << ": " << e.what() << std::endl;
    }
}


OSCInterface::Address OSCHandler::optional_address(Message const & msg, std::size_t i)
{
    if (msg.args.size() > i) {
        return OSCInterface::Address(boost::get<std::string>(msg.args[i]));
    } else {
        return msg.src;
    }
}


void OSCHandler::on_ping(Message const & msg)
{
    auto addr = optional_address(msg);

    std::cout << "ping from " << msg.src.url() << std::endl;
    _osc->send(addr, "/klick/pong");
}


void OSCHandler::on_check(Message const & msg)
{
    _osc->send(optional_address(msg), "/klick/ready");
}


void OSCHandler::on_register_client(Message const & msg)
{
    auto addr = optional_address(msg);

    auto i = std::find(_clients.begin(), _clients.end(), addr);
    if (i == _clients.end()) {
        _clients.push_back(addr);
        logv << "client " << addr.url() << " registered" << std::endl;
    }
}


void OSCHandler::on_unregister_client(Message const & msg)
{
    auto addr = optional_address(msg);

    auto i = std::find(_clients.begin(), _clients.end(), addr);
    if (i != _clients.end()) {
        _clients.erase(i);
        logv << "client " << addr.url() << " unregistered" << std::endl;
    }
}


void OSCHandler::on_query(Message const & msg)
{
    on_config_query(msg);
    on_metro_query(msg);

    if (metro_simple()) {
        on_simple_query(msg);
    }
    else if (metro_map()) {
        on_map_query(msg);
    }
    else if (metro_jack()) {
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
    _klick.set_sound(boost::get<int>(msg.args[0]));
    _osc->send(_clients, "/klick/config/sound", _klick.sound());
}


void OSCHandler::on_config_set_sound_custom(Message const & msg)
{
    std::string emphasis = boost::get<std::string>(msg.args[0]);
    std::string normal = boost::get<std::string>(msg.args[1]);

    _klick.set_sound_custom(emphasis, normal);

    std::string res_emphasis, res_normal;
    std::tie(res_emphasis, res_normal) = _klick.sound_custom();

    _osc->send(_clients, "/klick/config/sound", res_emphasis, res_normal);

    if (res_emphasis != emphasis) {
        _osc->send(_clients, "/klick/config/sound_loading_failed", emphasis);
    }
    if (res_normal != normal) {
        _osc->send(_clients, "/klick/config/sound_loading_failed", normal);
    }
}


void OSCHandler::on_config_set_sound_volume(Message const & msg)
{
    _klick.set_sound_volume(boost::get<float>(msg.args[0]), boost::get<float>(msg.args[1]));
    _osc->send(_clients, "/klick/config/sound_volume", std::get<0>(_klick.sound_volume()),
                                                       std::get<1>(_klick.sound_volume()));
}


void OSCHandler::on_config_set_sound_pitch(Message const & msg)
{
    _klick.set_sound_pitch(boost::get<float>(msg.args[0]), boost::get<float>(msg.args[1]));
    _osc->send(_clients, "/klick/config/sound_pitch", std::get<0>(_klick.sound_pitch()),
                                                      std::get<1>(_klick.sound_pitch()));
}


void OSCHandler::on_config_set_volume(Message const & msg)
{
    _audio.set_volume(boost::get<float>(msg.args[0]));
    _osc->send(_clients, "/klick/config/volume", _audio.volume());
}


void OSCHandler::on_config_connect(Message const & msg)
{
    if (msg.types.find_first_not_of("s") != std::string::npos) {
        std::cerr << msg.path << ": invalid argument type" << std::endl;
        return;
    }

    for (auto & a : msg.args) {
        try {
            _audio.connect(boost::get<std::string>(a));
        }
        catch (AudioInterfaceJack::AudioError const & e) {
            std::cerr << msg.path << ": " << e.what() << std::endl;
        }
    }
}


void OSCHandler::on_config_autoconnect(Message const & /*msg*/)
{
    _audio.autoconnect();
}


void OSCHandler::on_config_disconnect_all(Message const &)
{
    _audio.disconnect_all();
}


void OSCHandler::on_config_get_available_ports(Message const & msg)
{
    std::vector<std::string> v = _audio.available_ports();
    _osc->send(optional_address(msg), "/klick/config/available_ports",
               OSCInterface::ArgumentVector(v.begin(), v.end()));
}


void OSCHandler::on_config_query(Message const & msg)
{
    auto addr = optional_address(msg);

    if (_klick.sound() != -1) {
        _osc->send(addr, "/klick/config/sound", _klick.sound());
    } else {
        _osc->send(addr, "/klick/config/sound", std::get<0>(_klick.sound_custom()),
                                                std::get<1>(_klick.sound_custom()));
    }
    _osc->send(addr, "/klick/config/sound_volume", std::get<0>(_klick.sound_volume()),
                                                   std::get<1>(_klick.sound_volume()));
    _osc->send(addr, "/klick/config/sound_pitch", std::get<0>(_klick.sound_pitch()),
                                                  std::get<1>(_klick.sound_pitch()));
    _osc->send(addr, "/klick/config/volume", _audio.volume());
}



void OSCHandler::on_metro_set_type(Message const & msg)
{
    std::string type = boost::get<std::string>(msg.args[0]);

    if (type == "simple") {
        _klick.set_metronome(Options::METRONOME_TYPE_SIMPLE);
    }
    else if (type == "map") {
        _klick.set_metronome(Options::METRONOME_TYPE_MAP);
    }
    else if (type == "jack") {
        _klick.set_metronome(Options::METRONOME_TYPE_JACK);
    }
    else {
        std::cerr << msg.path << ": invalid metronome type '" << type << "'" << std::endl;
        return;
    }

    _osc->send(_clients, "/klick/metro/type", type);
}


void OSCHandler::on_metro_start(Message const & /*msg*/)
{
    auto m = metro();
    m->start();
    _osc->send(_clients, "/klick/metro/active", m->active());
}


void OSCHandler::on_metro_stop(Message const & /*msg*/)
{
    auto m = metro();
    m->stop();
    _osc->send(_clients, "/klick/metro/active", m->active());
}


void OSCHandler::on_metro_query(Message const & msg)
{
    auto addr = optional_address(msg);

    if (metro_simple()) {
        _osc->send(addr, "/klick/metro/type", "simple");
    }
    else if (metro_map()) {
        _osc->send(addr, "/klick/metro/type", "map");
    }
    else if (metro_jack()) {
        _osc->send(addr, "/klick/metro/type", "jack");
    }
    else {
        FAIL();
    }

    _osc->send(addr, "/klick/metro/active", metro()->active());
}



void OSCHandler::on_simple_set_tempo(Message const & msg)
{
    auto m = metro_simple();
    m->set_tempo(boost::get<float>(msg.args[0]));
    _osc->send(_clients, "/klick/simple/tempo", m->tempo());
}


void OSCHandler::on_simple_set_tempo_increment(Message const & msg)
{
    auto m = metro_simple();
    m->set_tempo_increment(boost::get<float>(msg.args[0]));
    _osc->send(_clients, "/klick/simple/tempo_increment", m->tempo_increment());
}


void OSCHandler::on_simple_set_tempo_start(Message const & msg)
{
    auto m = metro_simple();
    m->set_tempo_start(boost::get<float>(msg.args[0]));
    _osc->send(_clients, "/klick/simple/tempo_start", m->tempo_start());
}


void OSCHandler::on_simple_set_tempo_limit(Message const & msg)
{
    auto m = metro_simple();
    m->set_tempo_limit(boost::get<float>(msg.args[0]));
    _osc->send(_clients, "/klick/simple/tempo_limit", m->tempo_limit());
}


void OSCHandler::on_simple_set_meter(Message const & msg)
{
    auto m = metro_simple();
    m->set_meter(boost::get<int>(msg.args[0]), boost::get<int>(msg.args[1]));
    _osc->send(_clients, "/klick/simple/meter", m->beats(), m->denom());
}


void OSCHandler::on_simple_set_pattern(Message const & msg)
{
    auto m = metro_simple();
    try {
        TempoMap::Pattern p = TempoMap::parse_pattern(boost::get<std::string>(msg.args[0]),
                                                      std::max(1, m->beats()));
        m->set_pattern(p);
    } catch (TempoMap::ParseError const & e) {
        std::cerr << msg.path << ": " << e.what() << std::endl;
        return;
    }
    _osc->send(_clients, "/klick/simple/pattern", TempoMap::pattern_to_string(m->pattern()));
}


void OSCHandler::on_simple_tap(Message const & msg)
{
    auto m = metro_simple();
    if (!msg.args.empty()) {
        m->tap(boost::get<double>(msg.args[0]));
    } else {
        m->tap();
    }
    _osc->send(_clients, "/klick/simple/tempo", m->tempo());
}


void OSCHandler::on_simple_query(Message const & msg)
{
    auto addr = optional_address(msg);
    auto m = metro_simple();

    _osc->send(addr, "/klick/simple/tempo", m->tempo());
    _osc->send(addr, "/klick/simple/tempo_increment", m->tempo_increment());
    _osc->send(addr, "/klick/simple/tempo_start", m->tempo_start());
    _osc->send(addr, "/klick/simple/tempo_limit", m->tempo_limit());
    _osc->send(addr, "/klick/simple/current_tempo", m->current_tempo());
    _osc->send(addr, "/klick/simple/meter", m->beats(), m->denom());
    _osc->send(addr, "/klick/simple/pattern", TempoMap::pattern_to_string(m->pattern()));
}


void OSCHandler::on_map_load_file(Message const & msg)
{
    try {
        _klick.set_tempomap_filename(boost::get<std::string>(msg.args[0]));
    } catch (std::runtime_error const & e) {
        std::cerr << msg.path << ": " << e.what() << std::endl;
        return;
    }

    _osc->send(_clients, "/klick/map/filename", _klick.tempomap_filename());
}


void OSCHandler::on_map_set_preroll(Message const & msg)
{
    _klick.set_tempomap_preroll(boost::get<int>(msg.args[0]));
    _osc->send(_clients, "/klick/map/preroll", _klick.tempomap_preroll());
}


void OSCHandler::on_map_set_tempo_multiplier(Message const & msg)
{
    _klick.set_tempomap_multiplier(boost::get<float>(msg.args[0]));
    _osc->send(_clients, "/klick/map/tempo_multiplier", _klick.tempomap_multiplier());
}


void OSCHandler::on_map_query(Message const & msg)
{
    auto addr = optional_address(msg);
    _osc->send(addr, "/klick/map/filename", _klick.tempomap_filename());
    _osc->send(addr, "/klick/map/preroll", _klick.tempomap_preroll());
    _osc->send(addr, "/klick/map/tempo_multiplier", _klick.tempomap_multiplier());
}


void OSCHandler::on_jack_query(Message const & /*msg*/)
{
    // nothing
}


void OSCHandler::fallback(Message const & msg)
{
    std::cerr << "unknown message: " << msg.path << " ," << msg.types << std::endl;
}
