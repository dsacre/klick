/*
 * klick - an advanced metronome for jack
 *
 * Copyright (C) 2007-2009  Dominic Sacré  <dominic.sacre@gmx.de>
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
#include <cstdlib>
#include <regex.h>
#include <boost/tokenizer.hpp>
#include <functional>
#include <algorithm>

#include "util/string.hh"
#include "util/lexical_cast.hh"

typedef boost::char_separator<char> char_sep;
typedef boost::tokenizer<char_sep> tokenizer;


#define REGEX_LABEL     "([[:alnum:]_-]+)"
#define REGEX_INT       "([[:digit:]]+)"
#define REGEX_FLOAT     "([[:digit:]]+(\\.[[:digit:]]*)?|\\.[[:digit:]]+)"
#define REGEX_PATTERN   "([Xx.]+)"


static inline bool is_specified(::regmatch_t const &m) {
    return ((m.rm_eo - m.rm_so) != 0);
}
static inline std::string extract_string(std::string const &s, ::regmatch_t const &m) {
    int len = m.rm_eo - m.rm_so;
    return len ? std::string(s.c_str() + m.rm_so, len) : "";
}
static inline int extract_int(std::string const &s, ::regmatch_t const &m) {
    int len = m.rm_eo - m.rm_so;
    return len ? atoi(std::string(s.c_str() + m.rm_so, len).c_str()) : 0;
}
static inline float extract_float(std::string const &s, ::regmatch_t const &m) {
    int len = m.rm_eo - m.rm_so;
    return len ? atof(std::string(s.c_str() + m.rm_so, len).c_str()) : 0.0f;
}


// matches a line that contains nothing but whitespace or comments
static char const regex_blank[] = "^[[:blank:]]*(#.*)?$";

// matches any valid line in a tempomap file
static char const regex_valid[] =
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

static int const
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
static char const regex_cmdline[] =
    // bars
    "^[[:blank:]]*("REGEX_INT"[[:blank:]]+)?" \
    // meter
    "("REGEX_INT"/"REGEX_INT"[[:blank:]]+)?" \
    // tempo
    REGEX_FLOAT"(-"REGEX_FLOAT"/"REGEX_FLOAT")?" \
    // pattern
    "([[:blank:]]+"REGEX_PATTERN")?[[:blank:]]*$";

static int const
    RE_NMATCHES_CMD = 15,
    IDX_BARS_CMD    =  2,
    IDX_BEATS_CMD   =  4,
    IDX_DENOM_CMD   =  5,
    IDX_TEMPO_CMD   =  6,
    IDX_TEMPO2_CMD  =  9,
    IDX_ACCEL_CMD   = 11,
    IDX_PATTERN_CMD = 14;



TempoMap::Pattern TempoMap::parse_pattern(std::string const &s, int nbeats)
{
    Pattern pattern;

    if (!s.empty()) {
        if ((int)s.length() != nbeats) {
            throw ParseError("pattern length doesn't match number of beats");
        }
        pattern.resize(nbeats);
        for (int n = 0; n < nbeats; ++n) {
            pattern[n] = (s[n] == 'X') ? BEAT_EMPHASIS :
                         (s[n] == 'x') ? BEAT_NORMAL : BEAT_SILENT;
        }
    }
    return pattern;
}


std::string TempoMap::pattern_to_string(Pattern const & p)
{
    std::ostringstream os;

    for (Pattern::const_iterator i = p.begin(); i != p.end(); ++i) {
        os << (*i == BEAT_EMPHASIS ? "X" : *i == BEAT_NORMAL ? "x" : ".");
    }

    return os.str();
}


std::vector<float> TempoMap::parse_tempi(std::string const &s, float tempo1, int nbeats_total)
{
    std::vector<float> tempi;

    char_sep sep(",");
    tokenizer tok(s, sep);
    if (std::distance(tok.begin(), tok.end()) != nbeats_total - 1) {
        throw ParseError("number of tempo values doesn't match number of beats");
    }
    tempi.push_back(tempo1);
    for (tokenizer::iterator i = tok.begin(); i != tok.end(); ++i) {
        tempi.push_back(das::lexical_cast<float>(*i));
    }
    return tempi;
}


void TempoMap::check_entry(Entry const & e)
{
    if ((e.tempo <= 0 && e.tempi.empty()) ||
        std::find_if(e.tempi.begin(), e.tempi.end(), std::bind2nd(std::less_equal<float>(), 0.0f)) != e.tempi.end()) {
        throw ParseError("tempo must be greater than zero");
    }
    if (e.bars <= 0 && e.bars != -1) {
        throw ParseError("number of bars must be greater than zero");
    }
    if (e.beats <= 0 || e.denom <= 0) {
        throw ParseError("invalid time signature");
    }
}


std::string TempoMap::dump() const
{
    std::ostringstream os;

    os << std::fixed << std::setprecision(2);

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
            for (std::vector<float>::const_iterator k = i->tempi.begin() + 1; k != i->tempi.end(); ++k) {
                os << "," << *k;
            }
        }
        os << " ";
        // pattern
        if (i->pattern.empty()) {
            os << "-";
        } else {
            os << pattern_to_string(i->pattern);
        }
        os << " ";
        // volume
        os << i->volume;
        os << std::endl;
    }

    return os.str();
}


TempoMapPtr TempoMap::join(TempoMapConstPtr const m1, TempoMapConstPtr const m2)
{
    TempoMapPtr m(new TempoMap());

    std::back_insert_iterator<Entries> p(m->_entries);

    std::copy(m1->entries().begin(), m1->entries().end(), p);
    std::copy(m2->entries().begin(), m2->entries().end(), p);

    return m;
}


/*
 * loads tempomap from a file
 */
TempoMapPtr TempoMap::new_from_file(std::string const & filename)
{
    TempoMapPtr map(new TempoMap());

    std::ifstream file(filename.c_str());

    if (!file.is_open()) {
        throw std::runtime_error(das::make_string() << "can't open tempo map file: '" << filename << "'");
    }

    ::regex_t re_blank, re;
    ::regmatch_t match[RE_NMATCHES];
    // compile the regexes
    ::regcomp(&re_blank, regex_blank, REG_EXTENDED | REG_NOSUB);
    ::regcomp(&re, regex_valid, REG_EXTENDED);
    boost::shared_ptr<void> foo(&re_blank, ::regfree);
    boost::shared_ptr<void> bar(&re, ::regfree);

    std::string line;
    int lineno = 0;

    while (!file.eof()) {
        std::getline(file, line);
        lineno++;

        // discard blank lines right away
        if (::regexec(&re_blank, line.c_str(), 0, NULL, 0) == 0) {
            continue;
        }

        try {
            // check if this line matches the regex
            if (::regexec(&re, line.c_str(), RE_NMATCHES, match, 0) != 0) {
                throw ParseError("malformed tempo map entry");
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
        catch (ParseError const & e) {
            throw ParseError(das::make_string() << e.what() << ":\n"
                                << "line " << lineno << ": " << line);
        }
    }

    return map;
}


/*
 * loads single-line tempomap from a string
 */
TempoMapPtr TempoMap::new_from_cmdline(std::string const & line)
{
    TempoMapPtr map(new TempoMap());

    ::regex_t re;
    ::regmatch_t match[RE_NMATCHES_CMD];
    ::regcomp(&re, regex_cmdline, REG_EXTENDED);
    boost::shared_ptr<void> foo(&re, ::regfree);

    try {
        if (::regexec(&re, line.c_str(), RE_NMATCHES_CMD, match, 0) != 0) {
            throw ParseError("malformed tempo map string");
        }

        Entry e;

        e.label   = "";
        e.bars    = is_specified(match[IDX_BARS_CMD]) ? extract_int(line, match[IDX_BARS_CMD]) : -1;
        e.beats   = is_specified(match[IDX_BEATS_CMD]) ? extract_int(line, match[IDX_BEATS_CMD]) : 4;
        e.denom   = is_specified(match[IDX_DENOM_CMD]) ? extract_int(line, match[IDX_DENOM_CMD]) : 4;
        e.tempo   = extract_float(line, match[IDX_TEMPO_CMD]);
        e.tempo2  = 0.0f;
        e.pattern = parse_pattern(extract_string(line, match[IDX_PATTERN_CMD]), e.beats);
        e.volume  = 1.0f;

        check_entry(e);

        if (!is_specified(match[IDX_TEMPO2_CMD])) {
            // no tempo change, just add this single entry
            map->_entries.push_back(e);
        } else {
            // tempo change...
            e.tempo2 = extract_float(line, match[IDX_TEMPO2_CMD]);
            float accel = extract_float(line, match[IDX_ACCEL_CMD]);
            if (accel <= 0.0f) {
                throw ParseError("accel must be greater than zero");
            }
            int bars_total = e.bars;
            int bars_accel = (int)ceilf(accel * fabs(e.tempo2 - e.tempo));

            if (bars_total == -1 || bars_total > bars_accel) {
                e.bars = bars_accel;
                map->_entries.push_back(e);

                // add a second entry, to be played once the "target" tempo is reached
                e.bars = bars_total == -1 ? -1 : bars_total - bars_accel;
                e.tempo = e.tempo2;
                e.tempo2 = 0.0f;
                map->_entries.push_back(e);
            } else {
                // total number of bars is shorter than acceleration
                e.bars = bars_total;
                e.tempo2 = e.tempo + (e.tempo2 - e.tempo) * bars_total / bars_accel;
                map->_entries.push_back(e);
            }
        }
    }
    catch (ParseError const & e) {
        throw ParseError(das::make_string() << e.what() << ":\n" << line);
    }

    return map;
}


/*
 * creates tempomap with one single entry
 */
TempoMapPtr TempoMap::new_simple(int bars, float tempo, int beats, int denom,
                                 Pattern const & pattern, float volume)
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
