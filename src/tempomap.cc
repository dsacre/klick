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

#include "tempomap.hh"

#include <sstream>
#include <fstream>
#include <iomanip>
#include <cmath>
#include <regex.h>
#include <boost/tokenizer.hpp>
#include <functional>
#include <algorithm>

#include "util/string.hh"

using namespace std;
using namespace das;
using boost::shared_ptr;
typedef boost::char_separator<char> char_sep;
typedef boost::tokenizer<char_sep> tokenizer;


#define REGEX_LABEL     "([[:alnum:]_-]+)"
#define REGEX_INT       "([[:digit:]]+)"
#define REGEX_FLOAT     "([[:digit:]]+(\\.[[:digit:]]*)?|\\.[[:digit:]]+)"
#define REGEX_PATTERN   "([Xx.]+)"


// frees the regex when going out of scope
struct regfreeer {
    regfreeer(regex_t *r) : re(r) { }
    ~regfreeer() { regfree(re); }
    regex_t *re;
};


static inline bool is_specified(const regmatch_t &m) {
    return ((m.rm_eo - m.rm_so) != 0);
}
static inline string extract_string(const string &s, const regmatch_t &m) {
    int len = m.rm_eo - m.rm_so;
    return len ? string(s.c_str() + m.rm_so, len) : "";
}
static inline int extract_int(const string &s, const regmatch_t &m) {
    int len = m.rm_eo - m.rm_so;
    return len ? atoi(string(s.c_str() + m.rm_so, len).c_str()) : 0;
}
static inline float extract_float(const string &s, const regmatch_t &m) {
    int len = m.rm_eo - m.rm_so;
    return len ? atof(string(s.c_str() + m.rm_so, len).c_str()) : 0.0f;
}


// matches a line that contains nothing but whitespace or comments
static const char regex_blank[] = "^[[:blank:]]*(#.*)?$";

// matches any valid line in a tempomap file
static const char regex_valid[] =
    // label
    "^[[:blank:]]*("REGEX_LABEL":)?" \
    // bars
    "[[:blank:]]*"REGEX_INT"" \
    // meter
    "([[:blank:]]+"REGEX_INT"/"REGEX_INT")?" \
    // tempo
    "[[:blank:]]+"REGEX_FLOAT"(-"REGEX_FLOAT"|((,"REGEX_FLOAT")*))?" \
    // pattern
    "([[:blank:]]+"REGEX_PATTERN")?" \
    // volume
    "([[:blank:]]+"REGEX_FLOAT")?" \
    // comment
    "[[:blank:]]*(#.*)?$";

static const int
    RE_NMATCHES = 22,
    IDX_LABEL   =  2,
    IDX_BARS    =  3,
    IDX_BEATS   =  5,
    IDX_DENOM   =  6,
    IDX_TEMPO   =  7,
    IDX_TEMPO2  = 10,
    IDX_TEMPI   = 12,
    IDX_PATTERN = 17,
    IDX_VOLUME  = 19;


// matches valid tempo parameters on the command line
static const char regex_cmdline[] =
    // meter
    "^[[:blank:]]*("REGEX_INT"/"REGEX_INT"[[:blank:]]+)?" \
    // tempo
    REGEX_FLOAT"(-"REGEX_FLOAT"/"REGEX_FLOAT")?" \
    // pattern
    "([[:blank:]]+"REGEX_PATTERN")?[[:blank:]]*$";

static const int
    RE_NMATCHES_CMD = 13,
    IDX_BEATS_CMD   =  2,
    IDX_DENOM_CMD   =  3,
    IDX_TEMPO_CMD   =  4,
    IDX_TEMPO2_CMD  =  7,
    IDX_ACCEL_CMD   =  9,
    IDX_PATTERN_CMD = 12;



vector<TempoMap::BeatType> TempoMap::parse_pattern(const string &s, int nbeats)
{
    vector<BeatType> pattern;

    if (!s.empty()) {
        if ((int)s.length() != nbeats) {
            throw ParseError("pattern length doesn't match number of beats");
        }
        pattern.resize(nbeats);
        for (int n = 0; n < nbeats; n++) {
            pattern[n] = (s[n] == 'X') ? BEAT_EMPHASIS :
                         (s[n] == 'x') ? BEAT_NORMAL : BEAT_SILENT;
        }
    }
    return pattern;
}


vector<float> TempoMap::parse_tempi(const string &s, float tempo1, int nbeats_total)
{
    vector<float> tempi;

    char_sep sep(",");
    tokenizer tok(s, sep);
    if (distance(tok.begin(), tok.end()) != nbeats_total - 1) {
        throw ParseError("number of tempo values doesn't match number of beats");
    }
    tempi.push_back(tempo1);
    for (tokenizer::iterator i = tok.begin(); i != tok.end(); ++i) {
        tempi.push_back(::strtof(i->c_str(), NULL));
    }
    return tempi;
}


void TempoMap::check_entry(const Entry & e)
{
    if ((e.tempo <= 0 && e.tempi.empty()) ||
        find_if(e.tempi.begin(), e.tempi.end(), bind2nd(less_equal<float>(), 0.0f)) != e.tempi.end()) {
        throw ParseError("tempo must be greater than zero");
    }
    if (e.bars <= 0 && e.bars != -1) {
        throw ParseError("number of bars must be greater than zero");
    }
    if (e.beats <= 0 || e.denom <= 0) {
        throw ParseError("invalid time signature");
    }
}


string TempoMap::dump() const
{
    ostringstream os;

    os << fixed << setprecision(2);

    for (Entries::const_iterator i = _entries.begin(); i != _entries.end(); ++i) {
        // label
        os << (i->label.length() ? i->label : "-") << ": ";
        // bars
        if (i->bars != -1) os << i->bars;
        else os << "*";
        os << " ";
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
        // pattern
        if (i->pattern.empty()) {
            os << "-";
        } else {
            for (vector<BeatType>::const_iterator j = i->pattern.begin(); j != i->pattern.end(); ++j)
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
        throw runtime_error(make_string() << "can't open tempomap file: '" << filename << "'");
    }

    regex_t re_blank, re;
    regmatch_t match[RE_NMATCHES];
    // compile the regexes
    regcomp(&re_blank, regex_blank, REG_EXTENDED | REG_NOSUB);
    regcomp(&re, regex_valid, REG_EXTENDED);
    regfreeer foo(&re_blank);
    regfreeer bar(&re);

    string line;
    int lineno = 0;

    while (!file.eof()) {
        getline(file, line);
        lineno++;

        // discard blank lines right away
        if (regexec(&re_blank, line.c_str(), 0, NULL, 0) == 0) {
            continue;
        }

        try {
            // check if this line matches the regex
            if (regexec(&re, line.c_str(), RE_NMATCHES, match, 0) != 0) {
                throw ParseError("malformed tempomap entry");
            }

            Entry e;

            e.label   = extract_string(line, match[IDX_LABEL]);
            e.bars    = extract_int(line, match[IDX_BARS]);
            e.tempo   = extract_float(line, match[IDX_TEMPO]);
            e.tempo2  = extract_float(line, match[IDX_TEMPO2]);   // 0 if empty
            e.beats   = is_specified(match[IDX_BEATS]) ? extract_int(line, match[IDX_BEATS]) : 4;
            e.denom   = is_specified(match[IDX_DENOM]) ? extract_int(line, match[IDX_DENOM]) : 4;
            e.pattern = parse_pattern(extract_string(line, match[IDX_PATTERN]), e.beats);
            e.volume  = is_specified(match[IDX_VOLUME]) ? extract_float(line, match[IDX_VOLUME]) : 1.0f;

            if (is_specified(match[IDX_TEMPI])) {
                e.tempi = parse_tempi(extract_string(line, match[IDX_TEMPI]), e.tempo, e.beats * e.bars);
                e.tempo = 0.0f;
            }

            check_entry(e);
            map->_entries.push_back(e);
        }
        catch (ParseError & e) {
            throw ParseError(make_string() << e.what() << ":" << endl
                                << "line " << lineno << ": " << line);
        }
    }

    return map;
}


/*
 * loads single-line tempomap from a string
 */
TempoMapPtr TempoMap::new_from_cmdline(const string & line)
{
    TempoMapPtr map(new TempoMap());

    regex_t re;
    regmatch_t match[RE_NMATCHES_CMD];
    regcomp(&re, regex_cmdline, REG_EXTENDED);
    regfreeer foo(&re);

    try {
        if (regexec(&re, line.c_str(), RE_NMATCHES_CMD, match, 0) != 0) {
            throw ParseError("malformed tempomap string");
        }

        Entry e;

        e.label   = "";
        e.bars    = -1;
        e.beats   = is_specified(match[IDX_BEATS_CMD]) ? extract_int(line, match[IDX_BEATS_CMD]) : 4;
        e.denom   = is_specified(match[IDX_DENOM_CMD]) ? extract_int(line, match[IDX_DENOM_CMD]) : 4;
        e.tempo   = extract_float(line, match[IDX_TEMPO_CMD]);
        e.tempo2  = 0.0f;
        e.pattern = parse_pattern(extract_string(line, match[IDX_PATTERN_CMD]), e.beats);
        e.volume  = 1.0f;

        check_entry(e);

        if (is_specified(match[IDX_TEMPO2_CMD])) {
            // tempo change...
            e.tempo2 = extract_float(line, match[IDX_TEMPO2_CMD]);
            float accel = extract_float(line, match[IDX_ACCEL_CMD]);
            if (accel <= 0.0f) throw ParseError("accel must be greater than zero");
            e.bars = (int)ceilf(accel * fabs(e.tempo2 - e.tempo));
            map->_entries.push_back(e);

            // add a second entry, to be played once the "target" tempo is reached
            e.bars = -1;
            e.tempo = e.tempo2;
            e.tempo2 = 0.0f;
            map->_entries.push_back(e);
        } else {
            // no tempo change, just add this single entry
            map->_entries.push_back(e);
        }
    }
    catch (ParseError & e) {
        throw ParseError(make_string() << e.what() << ":" << endl << line);
    }

    return map;
}


/*
 * creates tempomap with one single entry
 */
TempoMapPtr TempoMap::new_simple(int bars, float tempo, int beats, int denom,
                                 const vector<TempoMap::BeatType> & pattern, float volume)
{
    TempoMapPtr map(new TempoMap());

    Entry e;
    e.bars    = bars;
    e.tempo   = tempo;
    e.tempo2  = 0.0f;
    e.beats   = beats;
    e.denom   = denom;
    e.volume  = volume;
    e.pattern = pattern;

    map->_entries.push_back(e);

    return map;
}
