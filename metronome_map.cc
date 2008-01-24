/*
 * klick - an advanced metronome for jack
 *
 * Copyright (C) 2007  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "metronome_map.h"
#include "klick.h"
#include "options.h"
#include "audio_interface.h"
#include "audio_chunk.h"
#include "tempomap.h"

#include <jack/jack.h>
#include <jack/transport.h>

#include <cmath>

using namespace std;


MetronomeMap::MetronomeMap(
    TempoMapConstPtr tempomap,
    float tempo_multiplier,
    bool transport,
    bool master,
    int preroll,
    const string & start_label,
    AudioChunkConstPtr emphasis,
    AudioChunkConstPtr normal
)
  : Metronome(emphasis, normal),
    _current(0),
    _pos(tempomap, tempo_multiplier),
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
    // enable preroll only if jack transport is off -- why?
    if (/*!_transport_enabled &&*/ preroll != Options::PREROLL_NONE) {
        _pos.add_preroll(preroll);
    }

    if (_transport_master) {
        // reset transport position
        Audio->set_frame(0);
    }
}


MetronomeMap::~MetronomeMap()
{
    Audio->set_timebase_callback(NULL);
}


void MetronomeMap::start()
{
    if (_transport_master) {
        // receive jack timebase callbacks
        Audio->set_timebase_callback(this);
    }
    // base class sets process callback
    Metronome::start();
}


bool MetronomeMap::running() const
{
    // if transport is enabled, we never quit, even at the end of the tempomap
    return _transport_enabled ? true : !_pos.end();
}


void MetronomeMap::process_callback(sample_t *buffer, nframes_t nframes)
{
    if (_transport_enabled) {
        if (!Audio->transport_rolling()) return;

        nframes_t p = Audio->frame();

        if (p != _current) {
            // position changed since last period, need to relocate
            _current = p;
            _pos.locate(p);
        }
    } else {
        if (_pos.end()) return;
    }

    // does a new tick start in this period?
    if (_current + nframes > _pos.next_frame()) {
        // move position to next tick. loop just in case two beats are
        // less than one period apart (which we don't really handle)
        do { _pos.advance(); } while (_pos.frame() < _current);
        Tick tick = _pos.tick();

        if (tick.type == TempoMap::BEAT_SILENT) return;

        //cout << tick.frame << ": " << (tick.type == TempoMap::BEAT_EMPHASIS) << endl;

        // start playing the click sample
        play_click(tick.type == TempoMap::BEAT_EMPHASIS, tick.frame - _current, tick.volume);
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

    float_frames_t d = _pos.dist_to_next();

    if (d) {
        p->tick = (nframes_t)(((float_frames_t)_current - _pos.frame()) * TICKS_PER_BEAT / d);
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
        p->beats_per_minute = (double)Audio->samplerate() * 60.0 / d;
    }
    else if (!_pos.map_entry().tempo) {
        // tempo per beat
        size_t n = _pos.bar() * _pos.map_entry().beats + _pos.beat();
        p->beats_per_minute = _pos.map_entry().tempi[n];
    }
}



MetronomeMap::Position::Position(TempoMapConstPtr tempomap, float multiplier)
  : _tempomap(tempomap),
    _multiplier(multiplier)
{
    float_frames_t f = 0.0;

    // calculate first frame of each tempomap entry
    for (TempoMap::Entries::const_iterator i = tempomap->entries().begin(); i != tempomap->entries().end(); ++i) {
        _start_frames.push_back(f);
        //cout << f << endl;
        f += entry_frames(*i);
    }

    reset();
}


void MetronomeMap::Position::reset()
{
    _frame = 0.0;
    _entry = _bar = _beat = 0;
    _bar_total = 0;
    _init = true;
    _end = false;
}


void MetronomeMap::Position::set_start_label(const string & start_label)
{
    TempoMapPtr t(new TempoMap());

    // remove everything before the start label
    TempoMap::Entries::const_iterator i = _tempomap->entries().begin();
    while (i->label != start_label) ++i;
    for ( ; i != _tempomap->entries().end(); ++i) t->add(*i);
    _tempomap = t;
}


void MetronomeMap::Position::add_preroll(int nbars)
{
    const TempoMap::Entry & e = (*_tempomap)[0];

    TempoMapPtr preroll;

    // create a new tempomap for preroll
    if (nbars == Options::PREROLL_2_BEATS) {
        vector<TempoMap::BeatType> pattern;
        for (int n = 0; n < e.denom; n++) {
            pattern.push_back(TempoMap::BEAT_NORMAL);
        }
        preroll = TempoMap::new_simple(1, e.tempo, 2, e.denom, pattern, 0.66f);
    } else {
        preroll = TempoMap::new_simple(nbars, e.tempo, e.beats, e.denom, e.pattern, 0.66f);
    }

    // join preroll and our actual tempomap
    _tempomap = TempoMap::join(preroll, _tempomap);
}


void MetronomeMap::Position::locate(nframes_t f)
{
    reset();

    if (f == 0) return;

    float_frames_t dist = 0.0;

    // this will be very slow for long tempomaps...
    // should be improved some day
    while (dist = dist_to_next(), frame() + dist <= f && !_end) {
        advance(dist);
    }

    // make sure we don't miss the first beat if it starts at f
    if (frame() == f) {
        _init = true;
    }
}


MetronomeMap::float_frames_t MetronomeMap::Position::entry_frames(
    const TempoMap::Entry & e,
    int bar_start, int beat_start,
    int bar_end, int beat_end) const
{
    ASSERT(!(bar_start == bar_end && beat_start == beat_end));

    if (bar_end == -1) {
        // whole entry
        bar_end = e.bars;
        beat_end = 0;
    }

    int nbeats = (bar_end - bar_start) * e.beats + (beat_end - beat_start);
    float secs = 0.0f;

    // constant tempo
    if (e.tempo != 0.0f && e.tempo2 == 0.0f) {
        secs = nbeats * 240.0 / (e.tempo * e.denom);
    }
    // gradual tempo change
    else if (e.tempo != 0.0f && e.tempo2 != 0.0f) {
        float tdiff = e.tempo2 - e.tempo;
        // use doubles, otherwise the logarithms would lose some precision
        double t1 = (double)e.tempo + tdiff * (double(bar_start * e.beats) + beat_start) / (e.bars * e.beats);
        double t2 = (double)e.tempo + tdiff * (double(bar_end   * e.beats) + beat_end  ) / (e.bars * e.beats);

        float avg_tempo = (t1 - t2) / (log(t1) - log(t2));
        secs = (nbeats * 240.0) / (avg_tempo * e.denom);
    }
    // different tempo for each beat
    else if (e.tempo == 0.0f) {
        vector<float>::const_iterator begin = e.tempi.begin() + bar_start * e.beats + beat_start;
        vector<float>::const_iterator end   = e.tempi.begin() + bar_end   * e.beats + beat_end;
        secs = 0.0f;
        for (vector<float>::const_iterator i = begin; i != end; ++i) {
            secs += 240.0 / (*i * e.denom);
        }
    }

    return secs * (float_frames_t)Audio->samplerate() / _multiplier;
}


MetronomeMap::float_frames_t MetronomeMap::Position::dist_to_next() const
{
    // no valid next tick
    if (_init) return 0.0;
    if (_end) return numeric_limits<float_frames_t>::max();

    const TempoMap::Entry & e = (*_tempomap)[_entry];

    return entry_frames(e, _bar, _beat, _bar, _beat + 1);
}


void MetronomeMap::Position::advance(float_frames_t dist)
{
    if (_init) {
        _init = false;
        return;
    }

    if (!dist) {
        dist = dist_to_next();
    }
    _frame += dist;

    const TempoMap::Entry & e = (*_tempomap)[_entry];

    // move to next beat
    if (++_beat >= e.beats) {
        _beat = 0;
        // move to next bar
        if (++_bar >= e.bars) {
            _bar = 0;
            // move to next entry
            if (++_entry >= (int)_tempomap->size()) {
                _entry--;       // no such entry
                _end = true;
            }
        }
        _bar_total++;
    }
}


const MetronomeMap::Tick MetronomeMap::Position::tick() const
{
    if (_end) {
        // end of tempomap, return "nothing"
        return (Tick) { (nframes_t)_frame, TempoMap::BEAT_SILENT, 0 };
    }

    const TempoMap::Entry & e = (*_tempomap)[_entry];

    TempoMap::BeatType t;
    if (e.pattern.empty()) {
        // use default pattern
        t = (_beat == 0) ? TempoMap::BEAT_EMPHASIS : TempoMap::BEAT_NORMAL;
    } else {
        // use pattern as specified in the tempomap
        t = e.pattern[_beat];
    }
    return (Tick) { (nframes_t)_frame, t, e.volume };
}
