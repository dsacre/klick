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

#ifndef _TEMPOMAP_HH
#define _TEMPOMAP_HH

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "util/exception.hh"

typedef boost::shared_ptr<class TempoMap> TempoMapPtr;
typedef boost::shared_ptr<const class TempoMap> TempoMapConstPtr;


class TempoMap
{
  public:
    struct ParseError : public das::exception {
        ParseError(const std::string & w)
          : das::exception(w) { }
    };

    enum BeatType {
        BEAT_EMPHASIS = 1,
        BEAT_NORMAL,
        BEAT_SILENT
    };

    struct Entry {
        std::string label;
        int bars;                       // -1 means play ad infinitum
        float tempo;                    // zero if tempo is given for each beat
        float tempo2;                   // zero if tempo is constant
        std::vector<float> tempi;       // empty unless tempo == 0.0
        int beats;
        int denom;
        std::vector<BeatType> pattern;  // empty if default
        float volume;
    };

    typedef std::vector<Entry> Entries;

    // get all entries
    const Entries & entries() const { return _entries; }
    // get n'th entry
    const Entry & entry(size_t n) const { return _entries[n]; }
    const Entry & operator[](size_t n) const { return _entries[n]; }
    // get number of entries
    size_t size() const { return _entries.size(); }

    // get entry with label l, NULL if no such entry exists
    const Entry * entry(const std::string & l) const {
        if (l.empty()) return NULL;
        for (Entries::const_iterator i = _entries.begin(); i != _entries.end(); ++i) {
            if (i->label == l) return &*i;
        }
        return NULL;
    }

    void add(const Entry & e) {
        _entries.push_back(e);
    }

    std::string dump() const;

    static TempoMapPtr join(const TempoMapConstPtr, const TempoMapConstPtr);

    static TempoMapPtr new_from_file(const std::string & filename);
    static TempoMapPtr new_from_cmdline(const std::string & line);

    static TempoMapPtr new_simple(int bars, float tempo, int beats, int denom,
                                  const std::vector<BeatType> & pattern = std::vector<BeatType>(),
                                  float volume = 1.0f);

  protected:
    // builds a vector of beat types, based on the string description
    static std::vector<BeatType> parse_pattern(const std::string &s, int nbeats);
    // parses a comma-separated tempo string
    static std::vector<float> parse_tempi(const std::string &s, float tempo1, int nbeats_total);

    static void check_entry(const Entry & e);

    Entries _entries;
};


#endif // _TEMPOMAP_HH
