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

#include "position.h"
#include "options.h"
#include "audio_interface.h"
#include "tempomap.h"

#include <algorithm>
#include <iterator>
#include <numeric>
#include <boost/lambda/lambda.hpp>
#include <cmath>

#include <sys/time.h> /////////////
#include <time.h>

using namespace std;
using namespace boost::lambda;


Position::Position(TempoMapConstPtr tempomap, float multiplier)
  : _tempomap(tempomap),
    _multiplier(multiplier)
{
    float_frames_t f = 0.0;
    int b = 0;

    // calculate first frame of each tempomap entry
    for (TempoMap::Entries::const_iterator i = tempomap->entries().begin(); i != tempomap->entries().end(); ++i) {
        _start_frames.push_back(f);
        _start_bars.push_back(b);
        cout << f << " ";
        f += frame_dist(*i);
        b += i->bars;
    }
    cout << endl;
    // add end of tempomap
    _start_frames.push_back(f);
    _start_bars.push_back(b);

    reset();
}


void Position::reset()
{
    _frame = 0.0;
    _entry = _bar = _beat = 0;
    _bar_total = 0;
    _init = true;
    _end = false;
}


void Position::set_start_label(const string & start_label)
{
    TempoMapPtr t(new TempoMap());

    // remove everything before the start label
    TempoMap::Entries::const_iterator i = _tempomap->entries().begin();
    while (i->label != start_label) ++i;
    for ( ; i != _tempomap->entries().end(); ++i) t->add(*i);
    _tempomap = t;
}


void Position::add_preroll(int nbars)
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


void Position::locate(nframes_t f)
{
//#if 0

    timeval tv1, tv2, tv3, tv4;
    gettimeofday(&tv1, NULL);

    reset();

    if (f == 0) return;

    float_frames_t dist = 0.0;

    // this will be very slow for long tempomaps...
    // should be improved some day
    while (dist = dist_to_next(), frame() + dist <= f && !_end) {
        advance(/*dist*/);
    }

    // make sure we don't miss the first beat if it starts at f
    if (frame() == f) {
        _init = true;
    }

    gettimeofday(&tv2, NULL);

    float_frames_t xframe = _frame;
    int xentry = _entry, xbar = _bar, xbeat = _beat;
    int xbar_total = _bar_total;
    bool xinit = _init, xend = _end;

//#endif

    gettimeofday(&tv3, NULL);

    reset();

    if (f == 0) return;

    _entry = distance(_start_frames.begin(),
                      upper_bound(_start_frames.begin(), _start_frames.end(), f) - 1);

    if (_entry == (int)_tempomap->size()) {
        _end = true;
        return;
    }

    const TempoMap::Entry & e = (*_tempomap)[_entry];

    // difference between start of entry and desired position
    float_frames_t diff = f - _start_frames[_entry];
    double secs = diff / (float_frames_t)Audio->samplerate() * _multiplier;

    // constant tempo
    if (e.tempo && !e.tempo2) {
        int nbeats = (int)(secs / 240.0 * e.tempo * e.denom);

        _bar  = nbeats / e.beats;
        _beat = nbeats % e.beats;

        _frame = _start_frames[_entry] + frame_dist(e, 0, 0, _bar, _beat);
        _bar_total = _start_bars[_entry] + _bar;
    }
    // gradual tempo change
    else if (e.tempo && e.tempo2) {
        float_frames_t v = f - _start_frames[_entry];
        int nbeats = 0;

        int low = 0;
        int high = e.bars * e.beats;

        while(low <= high) {
            int mid = (low + high) / 2;
            float_frames_t d = frame_dist(e, 0, 0, 0, mid);
            //cout << "d:" << d << endl;
            if (d > v) {
                //cout << "A" << endl;
                high = mid - 1;
            } else if (d <= v) {
                float_frames_t dd = frame_dist(e, 0, 0, 0, mid + 1);
                //cout << "dd:" << dd << endl;
                if (dd < v) {
                    //cout << "B" << endl;
                    low = mid + 1;
                } else {
                    //cout << "nbeats = " << mid << endl;
                    nbeats = mid;
                    break;
                }
            }
        }

        _bar  = nbeats / e.beats;
        _beat = nbeats % e.beats;

        _frame = _start_frames[_entry] + frame_dist(e, 0, 0, _bar, _beat);
        _bar_total = _start_bars[_entry] + _bar;
    }
    // tempo per beat
    else if (!e.tempo) {
        _bar = _beat = 0;
        _frame = _start_frames[_entry];
        _bar_total = _start_bars[_entry];

        // terribly inefficient, but who uses tempo per beat anyway...?
        while (_frame + dist_to_next() <= f && !_end) {
            advance();
        }
    }

    // make sure we don't miss the first beat if it starts at f
    _init = (_frame == f);

    //cout << _frame << ": " << _bar << "|" << _beat << endl;

    gettimeofday(&tv4, NULL);

//#if 0
//    if (_entry != xentry) cout << _entry << " " << xentry << endl;
//    if (_frame != xframe) cout << _frame << " " << xframe << endl;
    ASSERT(abs(_frame - xframe) < 0.0000001);
    ASSERT(_entry == xentry);
    ASSERT(_bar == xbar);
    ASSERT(_beat == xbeat);
    ASSERT(_bar_total == xbar_total);
    ASSERT(_init == xinit);
    ASSERT(_end == xend);
//#endif

    cout << "before: " << (tv2.tv_usec - tv1.tv_usec) << endl;
    cout << "after:  " << (tv4.tv_usec - tv3.tv_usec) << endl;
}


Position::float_frames_t Position::frame_dist(
    const TempoMap::Entry & e,
    int bar_start, int beat_start,
    int bar_end, int beat_end) const
{
    if (bar_start == bar_end && beat_start == beat_end) {
        return 0.0;
    }

    if (bar_end == -2) {
        // whole entry
        bar_end = e.bars;
        beat_end = 0;
    }

    if (bar_end == -1) {
        return numeric_limits<float_frames_t>::max();
    }

    ASSERT(bar_start <= bar_end);
    ASSERT(bar_start != bar_end || beat_start < beat_end);

    int nbeats = (bar_end - bar_start) * e.beats + (beat_end - beat_start);
    double secs = 0.0;

    // constant tempo
    if (e.tempo && !e.tempo2) {
        secs = nbeats * 240.0 / (e.tempo * e.denom);
    }
    // gradual tempo change
    else if (e.tempo && e.tempo2) {
        double tdiff = e.tempo2 - e.tempo;

        double t1 = (double)e.tempo + tdiff * ((double)(bar_start * e.beats) + beat_start) / (e.bars * e.beats);
        double t2 = (double)e.tempo + tdiff * ((double)(bar_end   * e.beats) + beat_end  ) / (e.bars * e.beats);

        double avg_tempo = (t1 - t2) / (log(t1) - log(t2));
        secs = (nbeats * 240.0) / (avg_tempo * e.denom);
    }
    // different tempo for each beat
    else if (!e.tempo) {
        secs = accumulate(e.tempi.begin() + bar_start * e.beats + beat_start,
                          e.tempi.begin() + bar_end   * e.beats + beat_end,
                          0.0, _1 + 240.0 / (_2 * e.denom));
    }

    return secs * (float_frames_t)Audio->samplerate() / _multiplier;
}


Position::float_frames_t Position::dist_to_next() const
{
    // no valid next tick
    if (_init) return 0.0;
    if (_end) return numeric_limits<float_frames_t>::max();

    const TempoMap::Entry & e = (*_tempomap)[_entry];

    return frame_dist(e, _bar, _beat, _bar, _beat + 1);
}


void Position::advance()
{
    if (_init) {
        _init = false;
        return;
    }

    _frame += dist_to_next();

    const TempoMap::Entry & e = (*_tempomap)[_entry];

    // move to next beat
    if (++_beat >= e.beats) {
        _beat = 0;
        // move to next bar
        if (++_bar >= e.bars && e.bars != -1) {
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


const Position::Tick Position::tick() const
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
