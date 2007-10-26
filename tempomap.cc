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

#include "tempomap.h"

#include <sstream>
#include <fstream>
#include <cmath>
#include <boost/tokenizer.hpp>

#include "util.h"

using namespace std;
using boost::shared_ptr;
typedef boost::char_separator<char> char_sep;
typedef boost::tokenizer<char_sep> tokenizer;


#define REGEX_LABEL     "([[:alnum:]_-]+)"
#define REGEX_INT       "([[:digit:]]+)"
#define REGEX_FLOAT     "([[:digit:]]+(\\.[[:digit:]]*)?|\\.[[:digit:]]+)"
#define REGEX_PATTERN   "([Xx.]+)"

static const uint MAX_LINE_LENGTH = 256;


// matches a line that contains nothing but whitespace or comments
static const TempoMap::Regex regex_blank(
    "^[[:blank:]]*(#.*)?$",
    REG_EXTENDED | REG_NOSUB
);

// matches any valid line in a tempomap file
static const TempoMap::Regex regex_valid(
    // label
    "^[[:blank:]]*("REGEX_LABEL":)?" \
    // bars
    "[[:blank:]]*"REGEX_INT"" \
    // meter
    "([[:blank:]]+"REGEX_INT"/"REGEX_INT")?" \
    // tempo
    "[[:blank:]]+"REGEX_FLOAT"(-"REGEX_FLOAT"|((,"REGEX_FLOAT")*))?" \
    // accents
    "([[:blank:]]+"REGEX_PATTERN")?" \
    // volume
    "([[:blank:]]+"REGEX_FLOAT")?" \
    // comment
    "[[:blank:]]*(#.*)?$",
    REG_EXTENDED
);
static const int RE_NMATCHES = 22,
                 IDX_LABEL = 2, IDX_BARS = 3, IDX_BEATS = 5, IDX_DENOM = 6,
                 IDX_TEMPO = 7, IDX_TEMPO2 = 10, IDX_TEMPI = 12, IDX_ACCENTS = 17, IDX_VOLUME = 19;


// matches valid tempo parameters on the command line
static const TempoMap::Regex regex_cmdline(
    // meter
    "^[[:blank:]]*("REGEX_INT"/"REGEX_INT"[[:blank:]]+)?" \
    // tempo
    REGEX_FLOAT"(-"REGEX_FLOAT"/"REGEX_INT")?" \
    // accents
    "([[:blank:]]+"REGEX_PATTERN")?[[:blank:]]*$",
    REG_EXTENDED
);
static const int RE_NMATCHES_CMD = 12,
                 IDX_BEATS_CMD = 2, IDX_DENOM_CMD = 3, IDX_TEMPO_CMD = 4,
                 IDX_TEMPO2_CMD = 7, IDX_ACCEL_CMD = 9, IDX_ACCENTS_CMD = 11;


vector<TempoMap::BeatType> TempoMap::parse_accents(const string &s, uint nbeats)
{
    vector<BeatType> accents;

    if (!s.empty()) {
        if (s.length() != nbeats) {
            throw "accent pattern length doesn't match number of beats";
        }
        accents.resize(nbeats);
        for (uint n = 0; n < nbeats; n++) {
            accents[n] = (s[n] == 'X') ? BEAT_EMPHASIS :
                         (s[n] == 'x') ? BEAT_NORMAL : BEAT_SILENT;
        }
    }
    return accents;
}


vector<float> TempoMap::parse_tempi(const string &s, float tempo1, uint nbeats_total)
{
    vector<float> tempi;

    char_sep sep(",");
    tokenizer tok(s, sep);
    if (count_iter(tok) != nbeats_total - 1) {
        throw "number of tempo values doesn't match number of beats";
    }
    if (tempo1) tempi.push_back(tempo1);
    for (tokenizer::iterator i = tok.begin(); i != tok.end(); ++i) {
        tempi.push_back(::strtof(i->c_str(), NULL));
    }
    return tempi;
}


string TempoMap::dump() const
{
    ostringstream os;
    for (Entries::const_iterator i = _entries.begin(); i != _entries.end(); ++i) {
        // label
        os << (i->label.length() ? i->label : "-") << ": ";
        // bars
        if (i->bars != UINT_MAX) os << i->bars; else os << "*"; os << " ";
        // meter
        os << i->beats << "/" << i->denom << " ";
        // tempo
        if (i->tempo) {
            os << i->tempo;
            if (i->tempo2) os << "-" << i->tempo2;
        } else {
            os << i->tempi[0];
            for (vector<float>::const_iterator k = i->tempi.begin() + 1; k != i->tempi.end(); ++k) {
                os << "," << *k;
            }
        }
        os << " ";
        // accents
        if (i->accents.empty()) {
            os << "-";
        } else {
            for (vector<BeatType>::const_iterator j = i->accents.begin(); j != i->accents.end(); ++j)
                os << (*j == BEAT_EMPHASIS ? "X" : *j == BEAT_NORMAL ? "x" : ".");
        }
        os << " ";
        // volume
        os << i->volume;
        os << endl;
    }
    return os.str();
}


TempoMapPtr TempoMap::join(const TempoMapConstPtr m1, const TempoMapConstPtr m2)
{
    TempoMapPtr m(new TempoMap());

    back_insert_iterator<Entries> p(m->_entries);

    copy(m1->entries().begin(), m1->entries().end(), p);
    copy(m2->entries().begin(), m2->entries().end(), p);

    return m;
}


/*
 * loads tempomap from a file
 */
TempoMapPtr TempoMap::new_from_file(const string & filename)
{
    TempoMapPtr map(new TempoMap());

    ifstream file(filename.c_str());

    if (!file.is_open()) {
        ostringstream os;
        os << "can't open tempomap file: '" << filename << "'";
        throw os.str();
    }

    char line[MAX_LINE_LENGTH];
    int  lineno = 1;

    while (!file.eof())
    {
        file.getline(line, MAX_LINE_LENGTH);

        if (!regex_blank.match(line, 0, NULL, 0)) {
            regmatch_t match[RE_NMATCHES];

            if (regex_valid.match(line, RE_NMATCHES, match, 0)) {
                Entry e;

                e.label   = extract_string(line, match[IDX_LABEL]);
                e.bars    = extract_int(line, match[IDX_BARS]);
                e.tempo   = extract_float(line, match[IDX_TEMPO]);
                e.tempo2  = extract_float(line, match[IDX_TEMPO2]);   // 0 if empty
                e.beats   = is_specified(line, match[IDX_BEATS]) ? extract_int(line, match[IDX_BEATS]) : 4;
                e.denom   = is_specified(line, match[IDX_DENOM]) ? extract_int(line, match[IDX_DENOM]) : 4;
                e.accents = parse_accents(extract_string(line, match[IDX_ACCENTS]), e.beats);
                e.volume  = is_specified(line, match[IDX_VOLUME]) ? extract_float(line, match[IDX_VOLUME]) : 1.0f;

                if (is_specified(line, match[IDX_TEMPI])) {
                    e.tempi = parse_tempi(extract_string(line, match[IDX_TEMPI]), e.tempo, e.beats * e.bars);
                    e.tempo = 0.0f;
                }

                map->_entries.push_back(e);
            } else {
                ostringstream os;
                os << "invalid tempomap entry at line " << lineno << ":" << endl << line;
                throw os.str();
            }
        }
        lineno++;
    }

    return map;
}


/*
 * loads single-line tempomap from a string
 */
TempoMapPtr TempoMap::new_from_cmdline(const string & line)
{
    TempoMapPtr map(new TempoMap());

    regmatch_t match[RE_NMATCHES_CMD];

    if (regex_cmdline.match(line, RE_NMATCHES_CMD, match, 0))
    {
        Entry e;

        e.label   = "";
        e.bars    = UINT_MAX;
        e.beats   = is_specified(line, match[IDX_BEATS_CMD]) ? extract_int(line, match[IDX_BEATS_CMD]) : 4;
        e.denom   = is_specified(line, match[IDX_DENOM_CMD]) ? extract_int(line, match[IDX_DENOM_CMD]) : 4;
        e.tempo   = extract_float(line, match[IDX_TEMPO_CMD]);
        e.tempo2  = 0.0f;
        e.accents = parse_accents(extract_string(line, match[IDX_ACCENTS_CMD]), e.beats);
        e.volume  = 1.0f;

        if (is_specified(line, match[IDX_TEMPO2_CMD])) {
            // tempo change...
            e.tempo2 = extract_float(line, match[IDX_TEMPO2_CMD]);
            uint accel = extract_int(line, match[IDX_ACCEL_CMD]);
            if (accel < 1) throw "accel must be greater than 0";
            e.bars = accel * (int)fabs(e.tempo2 - e.tempo);
            map->_entries.push_back(e);

            // add a second entry, to be played once the "target" tempo is reached
            e.bars = UINT_MAX;
            e.tempo = e.tempo2;
            e.tempo2 = 0.0f;
            map->_entries.push_back(e);
        } else {
            // no tempo change, just add this single entry
            map->_entries.push_back(e);
        }
    }
    else
    {
        ostringstream os;
        os << "invalid tempomap string:" << endl << line;
        throw os.str();
    }

    return map;
}


TempoMapPtr TempoMap::new_simple(uint bars, float tempo, uint beats, uint denom,
                                 const vector<TempoMap::BeatType> & acc, float volume)
{
    TempoMapPtr map(new TempoMap());

    Entry e;
    e.bars    = bars;
    e.tempo   = tempo;
    e.tempo2  = 0.0f;
    e.beats   = beats;
    e.denom   = denom;
    e.volume  = volume;
    e.accents = acc;

/*    if (!acc) {
        e.accents.insert(e.accents.end(), beats, BEAT_NORMAL);
    }*/
    map->_entries.push_back(e);

    return map;
}
