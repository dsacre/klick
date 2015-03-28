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

#include "position.hh"
#include "options.hh"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <limits>
#include <cmath>

#include "util/debug.hh"


Position::Position(TempoMapConstPtr tempomap, float_frames_t samplerate, float multiplier)
  : _tempomap(tempomap),
    _samplerate(samplerate),
    _multiplier(multiplier)
{
    reset();
    calculate_entry_positions();
}


void Position::reset()
{
    _frame = 0.0;
    _entry = _bar = _beat = 0;
    _bar_total = 0;
    _beat_total = 0;
    _init = true;
    _end = false;
}


void Position::calculate_entry_positions()
{
    _start_frames.clear();
    _start_bars.clear();
    _start_beats.clear();

    float_frames_t frame = 0.0;
    int bar = 0;
    int beat = 0;

    // calculate first frame of each tempomap entry
    for (auto & e : _tempomap->entries()) {
        _start_frames.push_back(frame);
        _start_bars.push_back(bar);
        _start_beats.push_back(beat);
        if (e.bars != -1) {
            frame += frame_dist(e, 0, e.bars * e.beats);
            bar += e.bars;
            beat += e.bars * e.beats;
        } else {
            // play entry ad infinitum
            frame = std::numeric_limits<float_frames_t>::max();
            bar = std::numeric_limits<int>::max();
            beat = std::numeric_limits<int>::max();
        }
    }

    // add end of tempomap
    _start_frames.push_back(frame);
    _start_bars.push_back(bar);
    _start_beats.push_back(beat);

//    for (std::size_t n = 0; n != _start_frames.size(); ++n) {
//        std::cout << std::fixed << "entry " << n << " starts at frame " << _start_frames[n] << ", bar " << _start_bars[n] << ", beat " << _start_beats[n] << std::endl;
//    }
}


void Position::set_start_label(std::string const & start_label)
{
    auto map = std::make_shared<TempoMap>();

    // remove everything before the start label
    auto i = _tempomap->entries().begin();
    while (i->label != start_label) ++i;
    for ( ; i != _tempomap->entries().end(); ++i) map->add(*i);
    _tempomap = map;

    reset();
    calculate_entry_positions();
}


void Position::add_preroll(int nbars)
{
    TempoMap::Entry const & e = (*_tempomap)[0];

    TempoMapPtr preroll;

    // create a new tempomap for preroll
    if (nbars == Options::PREROLL_2_BEATS) {
        std::vector<TempoMap::BeatType> pattern(2, TempoMap::BEAT_NORMAL);
        preroll = TempoMap::new_simple(1, e.tempo, 2, e.denom, pattern, 0.66f);
    } else {
        preroll = TempoMap::new_simple(nbars, e.tempo, e.beats, e.denom, e.pattern, 0.66f);
    }

    // join preroll and our actual tempomap
    _tempomap = TempoMap::join(preroll, _tempomap);

    reset();
    calculate_entry_positions();
}


void Position::locate(nframes_t f)
{
    reset();

    if (f == 0) {
        // nothing else to do
        return;
    }

    // find the tempomap entry f is in
    _entry = std::distance(_start_frames.begin(),
                           std::upper_bound(_start_frames.begin(), _start_frames.end(), f) - 1);

    if (_entry == static_cast<int>(_tempomap->size())) {
        // end of tempomap
        _end = true;
        return;
    }

    TempoMap::Entry const & e = current_entry();

    // difference between start of entry and desired position
    float_frames_t diff = f - _start_frames[_entry];
    double secs = diff / _samplerate * _multiplier;

    if (e.tempo && !e.tempo2) {
        // constant tempo
        int nbeats = static_cast<int>((secs / 240.0 * e.tempo * e.denom));

        _bar  = nbeats / e.beats;
        _beat = nbeats % e.beats;

        _frame = _start_frames[_entry] + frame_dist(e, 0, _bar * e.beats + _beat);
        _bar_total = _start_bars[_entry] + _bar;
        _beat_total = _start_beats[_entry] + _bar * e.beats + _beat;
    }
    else if (e.tempo && e.tempo2) {
        // gradual tempo change
        double tdiff_beat = (e.tempo2 - e.tempo) / (e.bars * e.beats);

        // this is the reverse of the formula from frame_dist().
        // it's good to have a CAS that can figure this stuff out...
        int nbeats = ((e.tempo * std::exp(tdiff_beat * secs * e.denom / 240.0)) - e.tempo) / tdiff_beat;

        _bar  = nbeats / e.beats;
        _beat = nbeats % e.beats;

        _frame = _start_frames[_entry] + frame_dist(e, 0, _bar * e.beats + _beat);
        _bar_total = _start_bars[_entry] + _bar;
        _beat_total = _start_beats[_entry] + _bar * e.beats + _beat;
    }
    else if (!e.tempo) {
        // tempo per beat
        _bar = _beat = 0;
        _frame = _start_frames[_entry];
        _bar_total = _start_bars[_entry];
        _beat_total = _start_beats[_entry];

        // terribly inefficient, but who uses tempo per beat anyway...?
        while (_frame + dist_to_next() <= f && !_end) {
            advance();
        }
    }

    // make sure we don't miss the first beat if it starts at f
    _init = (_frame == f);
}


void Position::advance()
{
    if (_init) {
        _init = false;
        return;
    }

    _frame += dist_to_next();

    TempoMap::Entry const & e = current_entry();

    // move to next beat
    if (++_beat >= e.beats) {
        _beat = 0;
        // move to next bar
        if (++_bar >= e.bars && e.bars != -1) {
            _bar = 0;
            // move to next entry
            if (++_entry >= static_cast<int>(_tempomap->size())) {
                _entry--;       // no such entry
                _end = true;
            }
        }
        _bar_total++;
    }
    _beat_total++;
}


Position::float_frames_t Position::dist_to_next() const
{
    // no valid next tick
    if (_init) return 0.0;
    if (_end) return std::numeric_limits<float_frames_t>::max();

    TempoMap::Entry const & e = current_entry();

    return frame_dist(e, _bar * e.beats + _beat,
                         _bar * e.beats + _beat + 1);
}


Position::float_frames_t Position::frame_dist(TempoMap::Entry const & e, int start, int end) const
{
    if (start == end) {
        return 0.0;
    }

    ASSERT(start < end);

    int nbeats = end - start;
    double secs = 0.0;

    if ((e.tempo && !e.tempo2) || (e.tempo && e.tempo == e.tempo2)) {
        // constant tempo
        secs = nbeats * 240.0 / (e.tempo * e.denom);
    }
    else if (e.tempo && e.tempo2) {
        // gradual tempo change
        double tdiff = e.tempo2 - e.tempo;

        double t1 = static_cast<double>(e.tempo) + tdiff * (static_cast<double>(start) / (e.bars * e.beats));
        double t2 = static_cast<double>(e.tempo) + tdiff * (static_cast<double>(end)   / (e.bars * e.beats));

        double avg_tempo = (t1 - t2) / (std::log(t1) - std::log(t2));
        secs = (nbeats * 240.0) / (avg_tempo * e.denom);
    }
    else if (!e.tempo) {
        // different tempo for each beat
        secs = std::accumulate(e.tempi.begin() + start,
                               e.tempi.begin() + end,
                               0.0,
                               [=](float acc, float t){ return acc + 240.0 / (t * e.denom); });
    }

    return secs * _samplerate / _multiplier;
}


Position::Tick const Position::tick() const
{
    if (_end) {
        // end of tempomap, return "nothing"
        return (Tick) { static_cast<nframes_t>(_frame), TempoMap::BEAT_SILENT, 0 };
    }

    TempoMap::Entry const & e = current_entry();

    TempoMap::BeatType t;
    if (e.pattern.empty()) {
        // use default pattern
        t = (_beat == 0) ? TempoMap::BEAT_EMPHASIS : TempoMap::BEAT_NORMAL;
    } else {
        // use pattern as specified in the tempomap
        t = e.pattern[_beat];
    }
    return (Tick) { static_cast<nframes_t>(_frame), t, e.volume };
}
