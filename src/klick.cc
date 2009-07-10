/*
 * klick - an advanced metronome for jack
 *
 * Copyright (C) 2007-2009  Dominic Sacré  <dominic.sacre@gmx.de>
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
    boost::shared_ptr<AudioInterfaceJack> audio(new AudioInterfaceJack(_options->client_name));

    das::logv << "jack client name: " << audio->client_name() << std::endl;

    if (_options->connect_ports.size()) {
        for (std::vector<std::string>::const_iterator i = _options->connect_ports.begin(); i != _options->connect_ports.end(); ++i) {
            try {
                audio->connect(*i);
                das::logv << "connected to " << i->c_str() << std::endl;
            }
            catch (AudioInterface::AudioError const & e) {
                std::cerr << e.what() << std::endl;
            }
        }
    }
    if (_options->auto_connect) {
        audio->autoconnect();
    }

    _gc->set_thread(audio->client_thread());

    _audio = audio;
}


void Klick::setup_sndfile()
{
    _audio.reset(new AudioInterfaceSndfile(_options->output_filename, _options->output_samplerate));

    das::logv << "output filename: " << _options->output_filename << std::endl;
}


void Klick::load_tempomap()
{
    if (_options->filename.length()) {
        das::logv << "loading tempo map from file" << std::endl;
        _map = TempoMap::new_from_file(_options->filename);
    } else if (!_options->cmdline.empty()) {
        das::logv << "loading tempo map from command line" << std::endl;
        _map = TempoMap::new_from_cmdline(_options->cmdline);
    } else {
        _map = TempoMap::new_simple(-1, 120, 4, 4);
    }

    das::logv << "tempo map:" << std::endl
              << das::indent(_map->dump(), 2);

    // make sure the start label exists
    if (_options->start_label.length()) {
        if (_map->entry(_options->start_label)) {
            das::logv << "starting at label: " << _options->start_label << std::endl;
        } else {
            throw std::runtime_error(das::make_string() << "label '" << _options->start_label << "' not found in tempo map");
        }
    }

    if (!_options->output_filename.empty() && _map->entries().back().bars == -1) {
        throw std::runtime_error("can't export tempo map of infinite length");
    }
}


boost::tuple<std::string, std::string> Klick::sample_filenames(int n, Options::EmphasisMode emphasis_mode)
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

    return boost::make_tuple(emphasis, normal);
}


AudioChunkPtr Klick::load_sample(std::string const & filename, float volume, float pitch)
{
    AudioChunkPtr p;

    if (!filename.empty()) {
        p.reset(new AudioChunk(filename, _audio->samplerate()), _gc->disposer);
    } else {
        p.reset(new AudioChunk(_audio->samplerate()), _gc->disposer);
    }

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
    boost::tie(emphasis, normal) = sample_filenames(_options->click_sample, _options->emphasis_mode);

    das::logv << "loading samples:\n"
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

    das::logv << "loading samples:\n"
              << "  emphasis: " << emphasis << "\n"
              << "  normal:   " << normal << std::endl;

    try {
        _click_emphasis = load_sample(emphasis, _options->volume_emphasis, _options->pitch_emphasis);
    }
    catch (std::runtime_error const & e) {
        std::cerr << e.what() << std::endl;
        _click_emphasis.reset(new AudioChunk(_audio->samplerate()), _gc->disposer);
        _options->click_filename_emphasis = "";
    }

    try {
        _click_normal = load_sample(normal, _options->volume_normal, _options->pitch_normal);
    }
    catch (std::runtime_error const & e) {
        std::cerr << e.what() << std::endl;
        _click_normal.reset(new AudioChunk(_audio->samplerate()), _gc->disposer);
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
    Metronome * m = NULL;

    switch (_options->type) {
      case Options::METRONOME_TYPE_SIMPLE:
        m = new MetronomeSimple(*_audio, &(*_map)[0]);
        break;
      case Options::METRONOME_TYPE_JACK:
        // let's hope this cast is safe...
        m = new MetronomeJack(dynamic_cast<AudioInterfaceJack &>(*_audio));
        break;
      case Options::METRONOME_TYPE_MAP:
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
    _metro->register_process_callback();

    if (_options->transport_master) {
        if (boost::shared_ptr<MetronomeMap> m = boost::dynamic_pointer_cast<MetronomeMap>(_metro)) {
            try {
                m->register_timebase_callback();
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
        das::logv << "waiting for " << _options->delay << " seconds..." << std::endl;
        ::usleep(static_cast<unsigned long>(_options->delay * 1000000));
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
            das::logv << "end of tempo map reached" << std::endl;
            break;
        } else if (_audio->is_shutdown()) {
            throw std::runtime_error("shut down by the jack server");
        }
    }
}


void Klick::run_sndfile()
{
    AudioInterfaceSndfile *a = dynamic_cast<AudioInterfaceSndfile*>(&*_audio);
    ASSERT(a);
    MetronomeMap *m = dynamic_cast<MetronomeMap*>(&*_metro);
    ASSERT(m);

    static const nframes_t BUFFER_SIZE = 1024;

    m->start();
    while (m->current_frame() < m->total_frames() && !_quit) {
        a->process(std::min(BUFFER_SIZE, m->total_frames() - m->current_frame()));
    }
}


void Klick::signal_quit()
{
    _quit = true;
}
