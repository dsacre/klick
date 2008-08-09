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

#include "metronome_map.hh"
#include "options.hh"
#include "audio_interface.hh"
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
    bool master,
    int preroll,
    std::string const & start_label
)
  : Metronome(audio),
    _current(0),
    _pos(tempomap, audio.samplerate(), tempo_multiplier),
    _transport_enabled(transport),
    _transport_master(master)
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


bool MetronomeMap::running() const
{
    // if transport is enabled, we never quit, even at the end of the tempomap
    return _transport_enabled ? true : !_pos.end();
}


void MetronomeMap::process_callback(sample_t * /*buffer*/, nframes_t nframes)
{
    if (!active()) {
        return;
    }

    if (_transport_enabled) {
        if (!_audio.transport_rolling()) return;

        nframes_t p = _audio.frame();

        if (p != _current) {
            // position changed since last period, need to relocate
            _current = p;
            _pos.locate(p);
        }
    }
    else {
        if (_pos.end()) return;
    }

    // does a new tick start in this period?
    if (_current + nframes > _pos.next_frame()) {
        // move position to next tick.
        // loop just in case two beats are less than one period apart (which we don't really handle)
        do { _pos.advance(); } while (_pos.frame() < _current);

        Position::Tick tick = _pos.tick();

        //cout << tick.frame << ": " << (tick.type == TempoMap::BEAT_EMPHASIS) << endl;

        if (tick.type != TempoMap::BEAT_SILENT) {
            // start playing the click sample
            play_click(tick.type == TempoMap::BEAT_EMPHASIS, tick.frame - _current, tick.volume);
        }
    }

    _current += nframes;
}


void MetronomeMap::timebase_callback(jack_position_t *p)
{
    if (p->frame != _current) {
        // current position doesn't match jack transport frame.
        // assume we're wrong and jack is right ;)
        _current = p->frame;
        _pos.locate(p->frame);
    }

    if (_pos.end()) {
        // end of tempomap, no valid position
        p->valid = (jack_position_bits_t)0;
        return;
    }

    p->valid = JackPositionBBT;

    p->bar = _pos.bar_total() + 1;  // jack counts from 1
    p->beat = _pos.beat() + 1;
    p->beats_per_bar = _pos.map_entry().beats;
    p->beat_type = _pos.map_entry().denom;

    double d = _pos.dist_to_next();

    if (d) {
        p->tick = (nframes_t)(((double)_current - _pos.frame()) * TICKS_PER_BEAT / d);
    } else {
        p->tick = 0;
    }

    if (p->tick >= TICKS_PER_BEAT) {
        // already at the next beat, but _pos.advance() won't be called until the next process cycle
        p->tick -= (int32_t)TICKS_PER_BEAT;
        p->beat++;
        if (p->beat > _pos.map_entry().beats) {
            p->bar++;
        }
    }

    //cout << p->frame << ": " << p->bar << "|" << p->beat << "|" << p->tick << endl;

    p->ticks_per_beat = TICKS_PER_BEAT;

    // NOTE: jack's notion of bpm is different from ours.
    // all tempo values are converted from "quarters per minute"
    // to the actual beats per minute used by jack

    if (_pos.map_entry().tempo && (!_pos.map_entry().tempo2 || d == 0.0)) {
        // constant tempo, and/or start of tempomap
        p->beats_per_minute = _pos.map_entry().tempo * _pos.map_entry().denom / 4.0;
    }
    else if (_pos.map_entry().tempo2 && _pos.end()) {
        // end of tempomap, last entry had tempo change, so use tempo2
        p->beats_per_minute = _pos.map_entry().tempo2 * _pos.map_entry().denom / 4.0;
    }
    else if (_pos.map_entry().tempo2) {
        // tempo change, use average tempo for this beat
        p->beats_per_minute = (double)_audio.samplerate() * 60.0 / d;
    }
    else if (!_pos.map_entry().tempo) {
        // tempo per beat
        std::size_t n = _pos.bar() * _pos.map_entry().beats + _pos.beat();
        p->beats_per_minute = _pos.map_entry().tempi[n];
    }
}
