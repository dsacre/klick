/*
 * klick - an advanced metronome for jack
 *
 * Copyright (C) 2007-2013  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "klick.hh"
#include "main.hh"
#include "audio_interface_jack.hh"
#include "audio_interface_sndfile.hh"
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
#include <functional>
#include <time.h>
#include <stdint.h>

#include "util/debug.hh"
#include "util/string.hh"
#include "util/logstream.hh"
#include "util/garbage_collector.hh"


Klick::Klick(int argc, char *argv[])
  : _options(new Options)
  , _gc(new das::garbage_collector)
  , _quit(false)
{
    _options->parse(argc, argv);
    logv.enable(_options->verbose);

    // determine client name
    if (_options->client_name.empty()) {
        _options->client_name = "klick";
    }

    if (!_options->follow_transport) {
        load_tempomap();
    }

    if (_options->output_filename.empty()) {
        setup_jack();
    } else {
        setup_sndfile();
    }

    load_samples();
    load_metronome();

#ifdef ENABLE_OSC
    if (_options->use_osc) {
        // yuck!
        AudioInterfaceJack & a = dynamic_cast<AudioInterfaceJack &>(*_audio);
        _osc.reset(new OSCHandler(_options->osc_port, _options->osc_return_port, *this, a));
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


void Klick::setup_jack()
{
    std::unique_ptr<AudioInterfaceJack> audio(new AudioInterfaceJack(_options->client_name));

    logv << "jack client name: " << audio->client_name() << std::endl;

    if (_options->connect_ports.size()) {
        for (auto & p : _options->connect_ports) {
            try {
                audio->connect(p);
                logv << "connected to " << p.c_str() << std::endl;
            }
            catch (AudioInterface::AudioError const & e) {
                std::cerr << e.what() << std::endl;
            }
        }
    }
    if (_options->auto_connect) {
        audio->autoconnect();
    }

    _audio = std::move(audio);
}


void Klick::setup_sndfile()
{
    _audio.reset(new AudioInterfaceSndfile(_options->output_filename, _options->output_samplerate));

    logv << "output filename: " << _options->output_filename << std::endl;
}


void Klick::load_tempomap()
{
    if (_options->filename.length()) {
        logv << "loading tempo map from file '" << _options->filename << "'" << std::endl;
        _map = TempoMap::new_from_file(_options->filename);
    } else if (!_options->cmdline.empty()) {
        logv << "loading tempo map from command line" << std::endl;
        _map = TempoMap::new_from_cmdline(_options->cmdline);
    } else {
        _map = TempoMap::new_simple(-1, 120, 4, 4);
    }

    logv << "tempo map:\n"
         << das::indent(_map->dump(), 2);

    // make sure the start label exists
    if (_options->start_label.length()) {
        if (_map->entry(_options->start_label)) {
            logv << "starting at label: " << _options->start_label << std::endl;
        } else {
            throw std::runtime_error(das::make_string()
                        << "label '" << _options->start_label << "' not found in tempo map");
        }
    }

    if (!_options->output_filename.empty() && _map->entries().back().bars == -1) {
        throw std::runtime_error("can't export tempo map of infinite length");
    }
}


std::tuple<std::string, std::string> Klick::sample_filenames(int n, Options::EmphasisMode emphasis_mode)
{
    std::string emphasis, normal;

    switch (n) {
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
      case Options::CLICK_SAMPLE_SILENT:
        emphasis = "";
        normal   = "";
        break;
      default:
        FAIL();
    }

    switch (emphasis_mode) {
      case Options::EMPHASIS_MODE_NONE:
        emphasis = normal;
        break;
      case Options::EMPHASIS_MODE_ALL:
        normal = emphasis;
        break;
      default:
        break;
    }

    return std::make_tuple(emphasis, normal);
}


AudioChunkPtr Klick::load_sample(std::string const & filename, float volume, float pitch)
{
    AudioChunkPtr p;

    if (!filename.empty()) {
        p.reset(new AudioChunk(filename, _audio->samplerate()));
    } else {
        p.reset(new AudioChunk(_audio->samplerate()));
    }
    _gc->manage(p);

    if (volume != 1.0f) {
        p->adjust_volume(volume);
    }

    if (pitch != 1.0f) {
        p->adjust_pitch(pitch);
    }

    return p;
}


void Klick::load_samples()
{
    std::string emphasis, normal;
    std::tie(emphasis, normal) = sample_filenames(_options->click_sample, _options->emphasis_mode);

    logv << "loading samples:\n"
         << "  emphasis: " << emphasis << "\n"
         << "  normal:   " << normal << std::endl;

    _click_emphasis = load_sample(emphasis, _options->volume_emphasis, _options->pitch_emphasis);
    _click_normal = load_sample(normal, _options->volume_normal, _options->pitch_normal);
}


void Klick::set_sound(int n)
{
    if ((n < 0 || n > 3) && !(n == Options::CLICK_SAMPLE_SILENT)) return;
    if (n == _options->click_sample) return;

    _options->click_sample = n;

    load_samples();
    _metro->set_sound(_click_emphasis, _click_normal);
}


void Klick::set_sound_custom(std::string const & emphasis, std::string const & normal)
{
    _options->click_sample = Options::CLICK_SAMPLE_FROM_FILE;
    _options->click_filename_emphasis = emphasis;
    _options->click_filename_normal = normal;

    logv << "loading samples:\n"
         << "  emphasis: " << emphasis << "\n"
         << "  normal:   " << normal << std::endl;

    try {
        _click_emphasis = load_sample(emphasis, _options->volume_emphasis, _options->pitch_emphasis);
    }
    catch (std::runtime_error const & e) {
        std::cerr << e.what() << std::endl;
        _click_emphasis.reset(new AudioChunk(_audio->samplerate()));
        _gc->manage(_click_emphasis);
        _options->click_filename_emphasis = "";
    }

    try {
        _click_normal = load_sample(normal, _options->volume_normal, _options->pitch_normal);
    }
    catch (std::runtime_error const & e) {
        std::cerr << e.what() << std::endl;
        _click_normal.reset(new AudioChunk(_audio->samplerate()));
        _gc->manage(_click_normal);
        _options->click_filename_normal = "";
    }

    _metro->set_sound(_click_emphasis, _click_normal);
}


void Klick::set_sound_volume(float emphasis, float normal)
{
    if (emphasis == _options->volume_emphasis && normal == _options->volume_normal) {
        return;
    }

    _options->volume_emphasis = emphasis;
    _options->volume_normal = normal;

    load_samples();
    _metro->set_sound(_click_emphasis, _click_normal);
}


void Klick::set_sound_pitch(float emphasis, float normal)
{
    if (emphasis == _options->pitch_emphasis && normal == _options->pitch_normal) {
        return;
    }

    _options->pitch_emphasis = emphasis;
    _options->pitch_normal = normal;

    load_samples();
    _metro->set_sound(_click_emphasis, _click_normal);
}


void Klick::set_metronome(Options::MetronomeType type)
{
    _options->type = type;
    load_metronome();
}


void Klick::load_metronome()
{
    using namespace std::placeholders;

    Metronome * m = NULL;

    switch (_options->type) {
      case Options::METRONOME_TYPE_SIMPLE:
        m = new MetronomeSimple(*_audio, &(*_map)[0]);
        break;
      case Options::METRONOME_TYPE_JACK:
        ASSERT(dynamic_cast<AudioInterfaceJack*>(&*_audio));
        m = new MetronomeJack(dynamic_cast<AudioInterfaceJack &>(*_audio));
        break;
      case Options::METRONOME_TYPE_MAP:
        m = new MetronomeMap(*_audio,
                             _map,
                             _options->tempo_multiplier,
                             _options->transport_enabled,
                             _options->preroll,
                             _options->start_label);
        break;
    }

    // store metronome in shared_ptr with custom deleter
    _metro.reset(m);
    _gc->manage(_metro);

    _metro->set_sound(_click_emphasis, _click_normal);
    _audio->set_process_callback(std::bind(&Metronome::process_callback, _metro, _1, _2));

    if (_options->transport_master) {
        auto m = std::dynamic_pointer_cast<MetronomeMap>(_metro);
        auto a = dynamic_cast<AudioInterfaceTransport*>(&*_audio);

        // register timebase callback if supported by both the metronome and the audio backend
        if (m && a) {
            try {
                a->set_timebase_callback(std::bind(&Metronome::timebase_callback, m, _1));
            } catch (AudioInterface::AudioError const & e) {
                std::cerr << e.what() << std::endl;
            }
        }
    }
}


void Klick::set_tempomap_filename(std::string const & filename)
{
    _options->filename = filename;
    load_tempomap();

    load_metronome();
}


void Klick::set_tempomap_preroll(int bars)
{
    _options->preroll = bars;
    load_metronome();
}


void Klick::set_tempomap_multiplier(float mult)
{
    _options->tempo_multiplier = mult;
    load_metronome();
}


void Klick::run()
{
    if (_options->output_filename.empty()) {
        run_jack();
    } else {
        run_sndfile();
    }
}


void Klick::run_jack()
{
    if (!_options->transport_enabled && _options->delay) {
        logv << "waiting for " << _options->delay << " seconds..." << std::endl;

        ::uint64_t delay_nsec = static_cast< ::uint64_t>(_options->delay * 1000000000);
        ::timespec ts = { static_cast< ::time_t>(delay_nsec / 1000000000),
                          static_cast<long>(delay_nsec % 1000000000) };
        ::nanosleep(&ts, NULL);
    }

    if (!_osc) {
        logv << "starting metronome..." << std::endl;
        _metro->start();
    }

#ifdef ENABLE_OSC
    if (_osc) {
        _osc->start();
    }
#endif

    for (;;)
    {
        ::timespec ts = { 0, 10000000 };
        ::nanosleep(&ts, NULL);

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
            logv << "terminating" << std::endl;
            break;
        } else if (!_metro->running() && !_osc) {
            logv << "end of tempo map reached" << std::endl;
            break;
        } else if (_audio->is_shutdown()) {
            throw std::runtime_error("shut down by the jack server");
        }
    }
}


void Klick::run_sndfile()
{
    auto a = dynamic_cast<AudioInterfaceSndfile*>(&*_audio);
    ASSERT(a);
    auto m = dynamic_cast<MetronomeMap*>(&*_metro);
    ASSERT(m);

    static nframes_t const BUFFER_SIZE = 1024;

    m->start();
    while (m->current_frame() < m->total_frames() && !_quit) {
        a->process(std::min(BUFFER_SIZE, m->total_frames() - m->current_frame()));
    }
}


void Klick::signal_quit()
{
    _quit = true;
}
