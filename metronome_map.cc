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
#include "audio.h"
#include "tempomap.h"

#include <jack/jack.h>
#include <jack/transport.h>

#include <cmath>

using namespace std;
using boost::shared_ptr;


MetronomeMap::MetronomeMap(shared_ptr<const TempoMap> tempomap,
            float tempo_multiplier, bool transport, bool master,
            int preroll, const string & start_label,
            AudioChunkPtr emphasis, AudioChunkPtr normal)
  : Metronome(emphasis, normal),
    _current(0),
    _pos(tempomap, tempo_multiplier),
    _transport_enabled(transport),
    _transport_master(master)
{
    if (!start_label.empty()) {
        _pos.set_start_label(start_label);
    }
    // enable preroll only if jack transport is off
    if (!_transport_enabled && preroll != Klick::Options::PREROLL_NONE) {
        _pos.add_preroll(preroll);
    }

    if (_transport_master) {
        Audio->set_frame(0);
    }
}


MetronomeMap::~MetronomeMap()
{
    Audio->set_timebase_callback(NULL);
}


void MetronomeMap::start()
{
    if (_transport_master) Audio->set_timebase_callback(this);
    Metronome::start();
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
            reset_click();
        }
    } else {
        if (_pos.end()) return;
    }

    // does a new click start in this period?
    if (_current + nframes > _pos.next_frame())
    {
        // move position to next click
        // loop just in case two beats are less than one period apart
        do { _pos.advance(); } while (_pos.frame() < _current);
        Click click = _pos.click();

        AudioChunkPtr click_data;
        // determine click type
        if (click.type == TempoMap::BEAT_EMPHASIS) {
            click_data = _click_emphasis;
        } else if (click.type == TempoMap::BEAT_NORMAL) {
            click_data = _click_normal;
        }

        if (click_data) {
            start_click(buffer, nframes, click.frame - _current, click_data, click.volume);
        }
    }
    else {
        continue_click(buffer, nframes);
    }

    _current += nframes;
}


void MetronomeMap::timebase_callback(jack_position_t *p)
{
    if (p->frame != _current) {
        _current = p->frame;
        _pos.locate(p->frame);
        reset_click();
    }

    if (_pos.end()) {
        p->valid = (jack_position_bits_t)0;
        return;
    }

    p->valid = JackPositionBBT;

    p->bar = _pos.bar_total() + 1;  // jack starts counting at 1
    p->beat = _pos.beat() + 1;
    p->beats_per_bar = _pos.map_entry().beats;
    p->beat_type = _pos.map_entry().denom;

    float d = _pos.dist_to_next();
    p->tick = d ? nframes_t(float(_current - _pos.frame()) * 1920.0f / d) : 0;
    p->ticks_per_beat = 1920.0f;

    // NOTE: all tempo values are converted from "quarters per minute"
    // to the actual beats per minute used by jack

    if (!_pos.map_entry().tempo2 || d == 0.0f) {
        // constant tempo, and/or start of tempo map
        p->beats_per_minute = _pos.map_entry().tempo * _pos.map_entry().denom / 4.0f;
    }
    else if (_pos.map_entry().tempo2 && _pos.end()) {
        // end of tempomap, last entry had tempo change
        p->beats_per_minute = _pos.map_entry().tempo2 * _pos.map_entry().denom / 4.0f;
    }
    else {
        // tempo change
        p->beats_per_minute = (float)Audio->samplerate() * 60.0f / d;
    }
}



MetronomeMap::Position::Position(shared_ptr<const TempoMap> tempomap, float multiplier)
  : _tempomap(tempomap),
    _multiplier(multiplier)
{
    reset();
}


void MetronomeMap::Position::reset()
{
    _frame = 0.0f;
    _entry = _bar = _beat = 0;
    _bar_total = 0;
    _init = true;
    _end = false;
}


void MetronomeMap::Position::set_start_label(const string & start_label)
{
    shared_ptr<TempoMap> t(new TempoMap());

    // remove everything before the start label
    TempoMap::Entries::const_iterator i = _tempomap->entries().begin();
    while (i->label != start_label) ++i;
    for ( ; i != _tempomap->entries().end(); ++i) t->add(*i);
    _tempomap = t;
}


void MetronomeMap::Position::add_preroll(int nbars)
{
    const TempoMap::Entry & e = (*_tempomap)[0];

    shared_ptr<TempoMap> preroll;

    if (nbars == Klick::Options::PREROLL_2_BEATS) {
        vector<TempoMap::BeatType> acc;
        for (uint n = 0; n < e.denom; n++) {
            acc.push_back(TempoMap::BEAT_NORMAL);
        }
        preroll = TempoMap::new_simple(1, e.tempo, 2, e.denom, acc, 0.66f);
    } else {
        preroll = TempoMap::new_simple(nbars, e.tempo, e.beats, e.denom, e.accents, 0.66f);
    }

    _tempomap = TempoMap::join(preroll, _tempomap);
}


void MetronomeMap::Position::locate(nframes_t frame)
{
    reset();

    if (frame == 0) return;

    while ((nframes_t)_frame < frame && !_end) {
        advance();
    }
}


float MetronomeMap::Position::dist_to_next() const
{
    if (_init) return 0.0f;

    if (_end) {
        return numeric_limits<float>::max();
    }

    const TempoMap::Entry & e = (*_tempomap)[_entry];
    float tempo;

    if (e.tempo != 0.0f && (e.tempo2 == 0.0f || e.tempo2 == e.tempo))
    {
        // constant tempo
        tempo = e.tempo;
    }
    else if (e.tempo != 0.0f)
    {
        /*
         * the tempo change is linear with regard to _beats_.
         * the following formula works by dividing the "time" between the two reference tempi t1 and t2
         * into n slices with equal length (in beats, not time!), with tempo linearly increasing
         * between them.
         * the distance between the start and end point of the tempo change doesn't matter here!
         *
         * ...and thanks to Derive(TM) being a lot smarter than me, i don't actually have to code
         * all this in C++ ;)
         *
         *   1            / n-1 /            1             \     \
         * ----- =  lim   |  Σ  | ------------------------ | / n |
         * tempo   n->inf \ x=0 \  t1 + (x/n) * (t2 - t1)  /     /
         *
         * NOTE: for slow tempo changes, the tempo calculated by this formula appears to be the same
         * as a simple (t1 + t2) / 2. but it really isn't...
         */

        float tdiff = e.tempo2 - e.tempo;
        // use doubles, otherwise the logarithms would lose some precision
        double t1 = e.tempo + tdiff * (double(_bar * e.beats) + _beat    ) / (e.bars * e.beats);
        double t2 = e.tempo + tdiff * (double(_bar * e.beats) + _beat + 1) / (e.bars * e.beats);

        tempo = (t1 - t2) / (log(t1) - log(t2));
    }
    else
    {
        // different tempo given for each beat
        tempo = e.tempi[(_bar * e.beats) + _beat];
    }

    return ((float)Audio->samplerate() * 240.0f) / (e.denom * tempo * _multiplier);
}


void MetronomeMap::Position::advance()
{
    if (_init) {
        _init = false;
        return;
    }

    _frame += dist_to_next();

    const TempoMap::Entry & e = (*_tempomap)[_entry];

    if (++_beat >= e.beats) {
        _beat = 0;
        if (++_bar >= e.bars) {
            _bar = 0;
            if (++_entry >= _tempomap->size()) {
                _entry--;
                _end = true;
            }
        }
        _bar_total++;
    }
}


const MetronomeMap::Click MetronomeMap::Position::click() const
{
    if (_end) {
        // end of tempomap, return "nothing"
        return (Click) { (nframes_t)_frame, TempoMap::BEAT_SILENT, 0 };
    }

    const TempoMap::Entry & e = (*_tempomap)[_entry];

    TempoMap::BeatType t;
    if (e.accents.empty()) {
        t = (_beat == 0) ? TempoMap::BEAT_EMPHASIS : TempoMap::BEAT_NORMAL;
    } else {
        t = e.accents[_beat];
    }
    return (Click) { (nframes_t)_frame, t, e.volume };
}

