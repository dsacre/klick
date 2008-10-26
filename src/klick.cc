/*
 * klick - an advanced metronome for jack
 *
 * Copyright (C) 2007-2008  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "klick.hh"
#include "main.hh"
#include "options.hh"
#include "audio_interface.hh"
#include "audio_chunk.hh"

#ifdef ENABLE_OSC
  #include "osc_handler.hh"
#endif
#ifdef ENABLE_TERMINAL
  #include "terminal_handler.hh"
#endif

#include "tempomap.hh"
#include "metronome_map.hh"
#include "metronome_jack.hh"
#include "metronome_simple.hh"

#include <string>
#include <iostream>
#include <stdexcept>
#include <unistd.h>

#include "util/debug.hh"
#include "util/string.hh"
#include "util/logstream.hh"
#include "util/garbage_collector.hh"


Klick::Klick(int argc, char *argv[])
  : _options(new Options)
  , _gc(new das::garbage_collector(15))
  , _quit(false)
{
    _options->parse(argc, argv);
    das::logv.enable(_options->verbose);

    // determine client name
    if (_options->client_name.empty()) {
        _options->client_name = "klick";
    }

    // setup jack
    _audio.reset(new AudioInterface(_options->client_name));

    if (_options->connect_ports.size()) {
        for (std::vector<std::string>::const_iterator i = _options->connect_ports.begin(); i != _options->connect_ports.end(); ++i) {
            try {
                _audio->connect(*i);
                das::logv << "connected to " << i->c_str() << "" << std::endl;
            }
            catch (AudioInterface::AudioError const & e) {
                std::cerr << e.what() << std::endl;
            }
        }
    }
    if (_options->auto_connect) {
        _audio->autoconnect();
    }

    _gc->set_thread(_audio->client_thread());

    das::logv << "jack client name: " << _audio->client_name() << std::endl;


    if (!_options->follow_transport) {
        load_tempomap();
    }


    load_samples();


    // create metronome object
    if (_options->use_osc) {
        set_metronome(METRONOME_TYPE_SIMPLE);
    } else if (_options->interactive) {
        set_metronome(METRONOME_TYPE_SIMPLE);
    } else if (_options->follow_transport) {
        set_metronome(METRONOME_TYPE_JACK);
    } else {
        set_metronome(METRONOME_TYPE_MAP);
    }


#ifdef ENABLE_OSC
    if (_options->use_osc) {
        _osc.reset(new OSCHandler(_options->osc_port, _options->osc_return_port, *this, *_audio));
    }
#endif

#ifdef ENABLE_TERMINAL
    if (_options->interactive) {
        _term.reset(new TerminalHandler(*this, *_audio));
    }
#endif
}


Klick::~Klick()
{
}


void Klick::load_tempomap()
{
    if (_options->filename.length()) {
        das::logv << "loading tempomap from file" << std::endl;
        _map = TempoMap::new_from_file(_options->filename);
    } else if (!_options->cmdline.empty()) {
        das::logv << "loading tempomap from command line" << std::endl;
        _map = TempoMap::new_from_cmdline(_options->cmdline);
    } else {
        _map = TempoMap::new_simple(-1, 120, 4, 4);
    }

    das::logv << "tempomap:" << std::endl
              << das::indent(_map->dump(), 2);

    // make sure the start label exists
    if (_options->start_label.length()) {
        if (_map->entry(_options->start_label)) {
            das::logv << "starting at label: " << _options->start_label << std::endl;
        } else {
            throw std::runtime_error(das::make_string() << "label '" << _options->start_label << "' not found in tempomap");
        }
    }
}


void Klick::set_tempomap_filename(std::string const & filename)
{
    _options->filename = filename;
    load_tempomap();

    set_metronome(METRONOME_TYPE_MAP);
}


std::string const & Klick::tempomap_filename() const
{
    return _options->filename;
}


void Klick::load_samples()
{
    std::string emphasis, normal;

    switch (_options->click_sample) {
      case 0:
        emphasis = data_file("samples/square_emphasis.wav");
        normal   = data_file("samples/square_normal.wav");
        break;
      case 1:
        emphasis = data_file("samples/sine_emphasis.wav");
        normal   = data_file("samples/sine_normal.wav");
        break;
      case 2:
        emphasis = data_file("samples/noise_emphasis.wav");
        normal   = data_file("samples/noise_normal.wav");
        break;
      case 3:
        emphasis = data_file("samples/click_emphasis.wav");
        normal   = data_file("samples/click_normal.wav");
        break;
      case Options::CLICK_SAMPLE_FROM_FILE:
        emphasis = _options->click_filename_emphasis;
        normal   = _options->click_filename_normal;
        break;
      default:
        FAIL();
    }

    switch (_options->emphasis) {
      case Options::EMPHASIS_NONE:
        emphasis = normal;
        break;
      case Options::EMPHASIS_ALL:
        normal = emphasis;
        break;
      default:
        break;
    }

    das::logv << "loading samples:\n"
              << "  emphasis: " << emphasis << "\n"
              << "  normal: " << normal << std::endl;

    _click_emphasis.reset(new AudioChunk(emphasis, _audio->samplerate()), _gc->disposer);
    _click_normal.reset(new AudioChunk(normal, _audio->samplerate()), _gc->disposer);

    if (_options->volume_emphasis != 1.0f) {
        _click_emphasis->adjust_volume(_options->volume_emphasis);
    }
    if (_options->frequency_emphasis != 1.0f) {
        _click_emphasis->adjust_frequency(_options->frequency_emphasis);
    }

    if (_options->volume_normal != 1.0f) {
        _click_normal->adjust_volume(_options->volume_normal);
    }
    if (_options->frequency_normal != 1.0f) {
        _click_normal->adjust_frequency(_options->frequency_normal);
    }
}


void Klick::set_sound(int n)
{
    _options->click_sample = n;

    load_samples();
    _metro->set_sound(_click_emphasis, _click_normal);
}


void Klick::set_sound_custom(std::string const & emphasis, std::string const & normal)
{
    _options->click_sample = Options::CLICK_SAMPLE_FROM_FILE;
    _options->click_filename_emphasis = emphasis;
    _options->click_filename_normal = normal;

    load_samples();
    _metro->set_sound(_click_emphasis, _click_normal);
}


int Klick::sound() const
{
    return _options->click_sample;
}


std::string const & Klick::sound_custom_emphasis() const
{
    return _options->click_filename_emphasis;
}


std::string const & Klick::sound_custom_normal() const
{
    return _options->click_filename_normal;
}


void Klick::set_metronome(MetronomeType type)
{
    Metronome * m = NULL;

    switch (type) {
      case METRONOME_TYPE_SIMPLE:
        m = new MetronomeSimple(*_audio, (*_map)[0]);
        break;
      case METRONOME_TYPE_JACK:
        m = new MetronomeJack(*_audio);
        break;
      case METRONOME_TYPE_MAP:
        m = new MetronomeMap(*_audio,
                             _map,
                             _options->tempo_multiplier,
                             _options->transport_enabled,
                             _options->transport_master,
                             _options->preroll,
                             _options->start_label);
        break;
    }

    _metro.reset(m, _gc->disposer);

    _metro->set_sound(_click_emphasis, _click_normal);

    _audio->set_process_callback(_metro, true);

    if (_options->transport_master) {
        if (boost::shared_ptr<AudioInterface::TimebaseCallback> cb =
            boost::dynamic_pointer_cast<AudioInterface::TimebaseCallback>(_metro)) {
            try {
                _audio->set_timebase_callback(cb);
            } catch (AudioInterface::AudioError const & e) {
                std::cerr << e.what() << std::endl;
            }
        }
    }
}


void Klick::run()
{
    if (!_options->transport_enabled && _options->delay) {
        das::logv << "waiting for " << _options->delay << " seconds..." << std::endl;
        ::usleep((unsigned long)(_options->delay * 1000000));
    }

    if (!_osc) {
        das::logv << "starting metronome..." << std::endl;
        _metro->start();
    }

#ifdef ENABLE_OSC
    if (_osc) {
        _osc->start();
    }
#endif

    for (;;)
    {
        ::usleep(10000);
        _gc->collect();

#ifdef ENABLE_TERMINAL
        if (_term) {
            _term->handle_input();
        }
#endif

#ifdef ENABLE_OSC
        if (_osc) {
            _osc->update();
        }
#endif

        if (_quit) {
            das::logv << "terminating" << std::endl;
            break;
        } else if (!_metro->running() && !_osc) {
            das::logv << "end of tempomap reached" << std::endl;
            break;
        } else if (_audio->is_shutdown()) {
            throw std::runtime_error("shut down by the jack server");
        }
    }
}


void Klick::signal_quit()
{
    _quit = true;
}
