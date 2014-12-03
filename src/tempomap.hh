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

#ifndef KLICK_TEMPOMAP_HH
#define KLICK_TEMPOMAP_HH

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>

typedef std::shared_ptr<class TempoMap> TempoMapPtr;
typedef std::shared_ptr<class TempoMap const> TempoMapConstPtr;


class TempoMap
{
  public:
    struct ParseError
      : public std::runtime_error
    {
        ParseError(std::string const & w)
          : std::runtime_error(w) { }
    };

    enum BeatType {
        BEAT_EMPHASIS = 1,
        BEAT_NORMAL,
        BEAT_SILENT
    };

    typedef std::vector<BeatType> Pattern;

    struct Entry {
        std::string label;
        int bars;                       // -1 means play ad infinitum
        float tempo;                    // zero if tempo is given for each beat
        float tempo2;                   // zero if tempo is constant
        std::vector<float> tempi;       // empty unless tempo == 0.0
        int beats;
        int denom;
        Pattern pattern;                // empty if default
        float volume;
    };

    typedef std::vector<Entry> Entries;

    // get all entries
    Entries const & entries() const { return _entries; }
    // get n'th entry
    Entry const & entry(std::size_t n) const { return _entries[n]; }
    Entry const & operator[](std::size_t n) const { return _entries[n]; }
    // get number of entries
    std::size_t size() const { return _entries.size(); }

    // get entry with label l, NULL if no such entry exists
    Entry const * entry(std::string const & l) const {
        if (l.empty()) return NULL;
        for (auto & e : _entries) {
            if (e.label == l) return &e;
        }
        return NULL;
    }

    void add(Entry const & e) {
        _entries.push_back(e);
    }

    std::string dump() const;

    static TempoMapPtr join(TempoMapConstPtr const, TempoMapConstPtr const);

    static TempoMapPtr new_from_file(std::string const & filename);
    static TempoMapPtr new_from_cmdline(std::string const & line);

    static TempoMapPtr new_simple(int bars, float tempo, int beats, int denom,
                                  Pattern const & pattern = Pattern(),
                                  float volume = 1.0f);

  public:
    // builds a vector of beat types, based on the string description
    static Pattern parse_pattern(std::string const &s, int nbeats);

    static std::string pattern_to_string(Pattern const & p);

  private:
    // parses a comma-separated tempo string
    static std::vector<float> parse_tempi(std::string const &s, float tempo1, int nbeats_total);

    static void check_entry(Entry const & e);

    Entries _entries;
};


#endif // KLICK_TEMPOMAP_HH
