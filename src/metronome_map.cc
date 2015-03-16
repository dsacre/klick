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

#include "metronome_map.hh"
#include "options.hh"
#include "audio_interface_jack.hh"
#include "audio_chunk.hh"
#include "tempomap.hh"

#include <jack/jack.h>
#include <jack/transport.h>

#include "util/debug.hh"


MetronomeMap::MetronomeMap(
    AudioInterface & audio,
    TempoMapConstPtr tempomap,
    float tempo_multiplier,
    bool transport,
    int preroll,
    std::string const & start_label
)
  : Metronome(audio)
  , _frame(0)
  , _pos(tempomap, audio.samplerate(), tempo_multiplier)
  , _transport_enabled(transport)
{
    ASSERT(tempomap);
    ASSERT(tempomap->size() > 0);
    ASSERT(tempo_multiplier > 0.0f);

    // set start label
    if (!start_label.empty()) {
        _pos.set_start_label(start_label);
    }
    // add preroll
    if (preroll != Options::PREROLL_NONE) {
        _pos.add_preroll(preroll);
    }
}


MetronomeMap::~MetronomeMap()
{
}


void MetronomeMap::do_start()
{
    _pos.locate(0);
    _frame = 0;
}


void MetronomeMap::do_stop()
{
}


bool MetronomeMap::running() const
{
    // if transport is enabled, we never quit, even at the end of the tempomap
    return _transport_enabled ? true : !_pos.end();
}


nframes_t MetronomeMap::current_frame() const
{
    return _frame;
}


nframes_t MetronomeMap::total_frames() const
{
    return static_cast<nframes_t>(_pos.total_frames());
}


void MetronomeMap::process_callback(sample_t * /*buffer*/, nframes_t nframes)
{
    if (!active()) {
        return;
    }

    AudioInterfaceJack *a = dynamic_cast<AudioInterfaceJack *>(&_audio);

    if (_transport_enabled && a) {
        if (!a->transport_rolling()) return;

        nframes_t p = a->frame();

        if (p != _frame) {
            // position changed since last period, need to relocate
            _frame = p;
            _pos.locate(p);
        }
    } else {
        if (_pos.end()) return;
    }

    // check if a new tick starts in this period
    if (_frame + nframes > _pos.next_frame()) {
        // move position to next tick.
        // loop just in case two beats are less than one period apart (which we don't really handle)
        do { _pos.advance(); } while (_pos.frame() < _frame);

        Position::Tick tick = _pos.tick();

        //std::cout << tick.frame << ": " << (tick.type == TempoMap::BEAT_EMPHASIS) << std::endl;

        if (tick.type != TempoMap::BEAT_SILENT) {
            // start playing the click sample
            play_click(tick.type == TempoMap::BEAT_EMPHASIS, tick.frame - _frame, tick.volume);
        }
    }

    _frame += nframes;
}


void MetronomeMap::timebase_callback(position_t *p)
{
    if (p->frame != _frame) {
        // current position doesn't match jack transport frame.
        // assume we're wrong and jack is right ;)
        _frame = p->frame;
        _pos.locate(p->frame);
    }

    if (_pos.end()) {
        // end of tempomap, no valid position
        p->valid = (jack_position_bits_t)0;
        return;
    }

    // get the current tempomap entry
    TempoMap::Entry const & e = _pos.current_entry();

    p->valid = JackPositionBBT;

    p->bar = _pos.bar_total() + 1;  // jack counts from 1
    p->beat = _pos.beat() + 1;

    // get the distance from current to next beat, and calculate current tick
    double d = _pos.dist_to_next();
    if (d) {
        p->tick = (_frame - _pos.frame()) * TICKS_PER_BEAT / d;
    } else {
        p->tick = 0;
    }

    p->bar_start_tick = (_pos.beat_total() - _pos.beat()) * TICKS_PER_BEAT;
    p->beats_per_bar = e.beats;
    p->beat_type = e.denom;
    p->ticks_per_beat = TICKS_PER_BEAT;

    // NOTE: jack's notion of bpm is different from ours.
    // all tempo values are converted from "quarters per minute"
    // to the actual beats per minute used by jack
    if (e.tempo && (!e.tempo2 || d == 0.0)) {
        // constant tempo, and/or start of tempomap
        p->beats_per_minute = e.tempo * e.denom / 4.0;
    }
    else if (e.tempo2 && _pos.end()) {
        // end of tempomap, last entry had tempo change, so use tempo2
        p->beats_per_minute = e.tempo2 * e.denom / 4.0;
    }
    else if (e.tempo2) {
        // tempo change, use average tempo for this beat
        p->beats_per_minute = _audio.samplerate() * 60.0 / d;
    }
    else if (!e.tempo) {
        // tempo per beat
        std::size_t n = _pos.bar() * e.beats + _pos.beat();
        p->beats_per_minute = e.tempi[n];
    }

    if (p->tick >= TICKS_PER_BEAT) {
        // already at the next beat, but _pos.advance() won't be called until the next process cycle.
        // adjust bar, beat and tick accordingly
        p->tick -= TICKS_PER_BEAT;
        p->bar_start_tick += e.beats * TICKS_PER_BEAT;
        p->beat++;
        if (p->beat > e.beats) {
            p->bar++;
        }
    }

    //std::cout << p->frame << ": " << p->bar << "|" << p->beat << "|" << p->tick << " " << p->bar_start_tick << std::endl;
}
