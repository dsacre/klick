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

#include "metronome_simple.hh"

#include <algorithm>
#include <boost/lambda/lambda.hpp>

#include <sys/time.h>
#include <time.h>

#include "util/debug.hh"


MetronomeSimple::MetronomeSimple(AudioInterface & audio)
  : Metronome(audio)
  , _tempo(120.0)
  , _tempo_increment(0.0)
  , _tempo_limit(120.0)
  , _current_tempo(0.0)
  , _beats(4)
  , _denom(4)
  , _frame(0)
  , _next(0)
  , _beat(0)
  , _tapped(false)
{
}


MetronomeSimple::MetronomeSimple(AudioInterface & audio, TempoMap::Entry const & params)
  : Metronome(audio)
  , _tempo(0.0)
  , _tempo_increment(0.0)
  , _tempo_limit(120.0)
  , _current_tempo(0.0)
  , _beats(0)
  , _denom(0)
  , _frame(0)
  , _next(0)
  , _beat(0)
  , _tapped(false)
{
    set_all(params);
}


MetronomeSimple::~MetronomeSimple()
{
}


void MetronomeSimple::set_tempo(float tempo)
{
    _tempo = tempo;
    if (_active) {
        _current_tempo = _tempo;
    }
}


void MetronomeSimple::set_tempo_increment(float tempo_increment)
{
    _tempo_increment = tempo_increment;
}


void MetronomeSimple::set_tempo_limit(float tempo_limit)
{
    _tempo_limit = tempo_limit;
}


void MetronomeSimple::set_meter(int beats, int denom)
{
    if (beats != _beats && _pattern.size()) {
        // TODO: handle this properly
        _pattern.clear();
    }

    _beats = beats;
    _denom = denom;
}


void MetronomeSimple::set_pattern(TempoMap::Pattern const & pattern)
{
    _pattern = pattern;
}


void MetronomeSimple::set_all(TempoMap::Entry const & params)
{
    set_tempo(params.tempo);
    set_meter(params.beats, params.denom);
    set_pattern(params.pattern);
}


void MetronomeSimple::set_active(bool b)
{
    if (b && !_active) {
        _beat = 0;
        _next = 0;
        _frame = 0;
        _current_tempo = _tempo;
    } else if (!b) {
        _current_tempo = 0.0f;
    }

    _active = b;
}


void MetronomeSimple::tap(double now)
{
    _taps.push_back(now);

    if (_taps.size() && now < _taps.back()) {
        // distortion in space-time continuum
        return;
    }

    if ((int)_taps.size() > MAX_TAPS) {
        _taps.pop_front();
    }

    _taps.erase(
        std::remove_if(_taps.begin(), _taps.end(), boost::lambda::_1 < now - MAX_TAP_AGE),
        _taps.end()
    );

    if (_taps.size() > 1) {
        _tempo = 60.0f * (_taps.size() - 1) / (_taps.back() - _taps.front());
        if (active()) {
            _current_tempo = _tempo;
            _tapped = true;
        }
    }
}


void MetronomeSimple::tap()
{
    ::timeval tv;
    ::gettimeofday(&tv, NULL);
    double now = tv.tv_sec + 1.e-6 * tv.tv_usec;

    tap(now);
}


void MetronomeSimple::process_callback(sample_t * /*buffer*/, nframes_t nframes)
{
    if (_tapped) {
        // TODO: this is crap. read user's mind instead
        nframes_t delta = (nframes_t)(TAP_DIFF * _audio.samplerate());

        if (_frame - _prev < delta) {
            // delay next beat
            _next += _frame - _prev;
        } else {
            // play beat now
            _next = _frame;
        }

        _tapped = false;
    }

    if (!active()) {
        return;
    }

    if (_frame + nframes > _next)
    {
        // speed trainer
        if (_frame && _tempo_increment) {
            _current_tempo += _tempo_increment / std::max(_beats, 1);
            _current_tempo = std::min(_tempo_limit, _current_tempo);
        }

        nframes_t offset = _next - _frame;

        if (_pattern.size()) {
            ASSERT((int)_pattern.size() == std::max(1, _beats));
            if (_pattern[_beat] != TempoMap::BEAT_SILENT) {
                bool emphasis = (_pattern[_beat] == TempoMap::BEAT_EMPHASIS);
                play_click(emphasis, offset);
            }
        } else {
            play_click(_beat == 0 && _beats > 0, offset);
        }

        _prev = _next;

        _next += (nframes_t)(_audio.samplerate() * 240.0 / (_current_tempo * _denom));

        if (++_beat >= _beats) {
            _beat = 0;
        }
    }

    _frame += nframes;
}
