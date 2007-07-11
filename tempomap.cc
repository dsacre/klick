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

using namespace std;
using boost::shared_ptr;


#define REGEX_LABEL     "([[:alnum:]_-]+)"
#define REGEX_INT       "([[:digit:]]+)"
#define REGEX_FLOAT     "([[:digit:]]+(\\.[[:digit:]]*)?|\\.[[:digit:]]+)"
#define REGEX_PATTERN   "([Xx.]+)"

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
    "[[:blank:]]+"REGEX_FLOAT"(-"REGEX_FLOAT")?" \
    // accents
    "([[:blank:]]+"REGEX_PATTERN")?" \
    // volume
    "([[:blank:]]+"REGEX_FLOAT")?" \
    // comment
    "[[:blank:]]*(#.*)?$",
    REG_EXTENDED
);
static const int RE_NMATCHES = 18,
                 IDX_LABEL = 2, IDX_BARS = 3, IDX_BEATS = 5, IDX_DENOM = 6,
                 IDX_TEMPO = 7, IDX_TEMPO2 = 10, IDX_ACCENTS = 13, IDX_VOLUME = 15;


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


vector<TempoMap::BeatType> TempoMap::parse_accents(const string &s, uint nbeats) const
{
    vector<BeatType> accents;

    if (!s.empty()) {
        if (s.length() == nbeats) {
            accents.resize(nbeats);
            for (uint n = 0; n < nbeats; n++) {
                accents[n] = (s[n] == 'X') ? BEAT_EMPHASIS :
                             (s[n] == 'x') ? BEAT_NORMAL : BEAT_SILENT;
            }
        } else {
            ostringstream os;
            os << "accent pattern length doesn't match number of beats";
            throw os.str();
        }
    }
    return accents;
}

string TempoMap::dump() const
{
    ostringstream os;
    for (Entries::const_iterator i = _entries.begin(); i != _entries.end(); ++i) {
        os << (i->label.length() ? i->label : "<none>") << ": ";
        if (i->bars != UINT_MAX) os << i->bars; else os << "<unlimited>"; os << " ";
        os << i->beats << "/" << i->denom << " ";
        os << i->tempo << " ";
        if (!i->tempo2) os << "<constant> "; else os << i->tempo2 << " ";
        if (i->accents.empty()) {
            os << "<default>";
        } else {
            for (vector<BeatType>::const_iterator j = i->accents.begin(); j != i->accents.end(); ++j)
                os << (*j == BEAT_EMPHASIS ? "X" : *j == BEAT_NORMAL ? "x" : ".");
        }
        os << " ";
        os << i->volume;
        os << endl;
    }
    return os.str();
}

shared_ptr<TempoMap> TempoMap::join(shared_ptr<const TempoMap> m1, shared_ptr<const TempoMap> m2)
{
    shared_ptr<TempoMap> m(new TempoMap());

    back_insert_iterator<Entries> p(m->_entries);

    copy(m1->entries().begin(), m1->entries().end(), p);
    copy(m2->entries().begin(), m2->entries().end(), p);

    return m;
}

/*
 * loads tempomap from a file
 */
TempoMapFile::TempoMapFile(const string & filename)
{
    ifstream file(filename.c_str());

    if (!file.is_open()) {
        ostringstream os;
        os << "can't open tempomap file: '" << filename << "'";
        throw os.str();
    }

    char line[256];
    int  lineno = 1;

    while (!file.eof())
    {
        file.getline(line, 256);

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

                _entries.push_back(e);
            } else {
                ostringstream os;
                os << "invalid tempomap entry at line " << lineno << ":" << endl << line;
                throw os.str();
            }
        }
        lineno++;
    }
}

/*
 * loads single-line tempomap from a string
 */
TempoMapCmdline::TempoMapCmdline(const string & line)
{
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
            _entries.push_back(e);

            // add a second entry, to be played once the "target" tempo is reached
            e.bars = UINT_MAX;
            e.tempo = e.tempo2;
            e.tempo2 = 0.0f;
            _entries.push_back(e);
        } else {
            // no tempo change, just add this single entry
            _entries.push_back(e);
        }
    }
    else
    {
        ostringstream os;
        os << "invalid tempomap string:" << endl << line;
        throw os.str();
    }
}
