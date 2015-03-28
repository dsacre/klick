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

#include "metronome_simple.hh"

#include <algorithm>

#include <sys/time.h>
#include <time.h>

#include "util/debug.hh"


MetronomeSimple::MetronomeSimple(AudioInterface & audio, TempoMap::Entry const * params)
  : Metronome(audio)
  , _tempo(120.0)
  , _tempo_increment(0.0)
  , _tempo_start(0.0)
  , _tempo_limit(0.0)
  , _current_tempo(0.0)
  , _beats(4)
  , _denom(4)
  , _frame(0)
  , _next(0)
  , _beat(0)
  , _tapped(false)
{
    if (params) {
        set_all(*params);
    }
}


MetronomeSimple::~MetronomeSimple()
{
}


void MetronomeSimple::set_tempo(float tempo)
{
    _tempo = tempo;
    if (active()) {
        _current_tempo = _tempo;
    }
}


void MetronomeSimple::set_tempo_increment(float tempo_increment)
{
    _tempo_increment = tempo_increment;
}


void MetronomeSimple::set_tempo_start(float tempo_start)
{
    _tempo_start = tempo_start;
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


void MetronomeSimple::do_start()
{
    _beat = 0;
    _next = 0;
    _frame = 0;

    if (_tempo_increment && _tempo_start) {
        _current_tempo = _tempo_start;
    } else {
        _current_tempo = _tempo;
    }
}


void MetronomeSimple::do_stop()
{
    _current_tempo = 0.0f;
}


void MetronomeSimple::tap(double now)
{
    if (_taps.size() && now < _taps.back()) {
        // distortion in space-time continuum
        _taps.clear();
    }

    _taps.push_back(now);

    if (static_cast<int>(_taps.size()) > MAX_TAPS) {
        _taps.pop_front();
    }

    // forget taps which happened too long ago
    _taps.erase(
        std::remove_if(_taps.begin(), _taps.end(),
                       [&](double t){ return t < now - MAX_TAP_AGE; }),
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
        nframes_t delta = static_cast<nframes_t>(TAP_DIFF * _audio.samplerate());

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
        // offset in current period
        nframes_t offset = _next - _frame;

        if (_pattern.size()) {
            // play click, user-defined pattern
            ASSERT(static_cast<int>(_pattern.size()) == std::max(1, _beats));
            if (_pattern[_beat] != TempoMap::BEAT_SILENT) {
                bool emphasis = (_pattern[_beat] == TempoMap::BEAT_EMPHASIS);
                play_click(emphasis, offset);
            }
        } else {
            // play click, default pattern
            play_click(_beat == 0 && _beats > 0, offset);
        }

        // speed trainer
        if (_frame && _tempo_increment) {
            _current_tempo += _tempo_increment / std::max(_beats, 1);
            if (_tempo_limit) {
                _current_tempo = _tempo_increment > 0.0f ? std::min(_current_tempo, _tempo_limit)
                                                         : std::max(_current_tempo, _tempo_limit);
            } else if (_tempo_start) {
                _current_tempo = _tempo_increment > 0.0f ? std::min(_current_tempo, _tempo)
                                                         : std::max(_current_tempo, _tempo);
            }
        }

        _prev = _next;
        _next += static_cast<nframes_t>(_audio.samplerate() * 240.0 / (_current_tempo * _denom));

        if (++_beat >= _beats) {
            _beat = 0;
        }
    }

    _frame += nframes;
}
