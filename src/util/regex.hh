/*
 * Copyright (C) 2012-2013  Dominic Sacré  <dominic.sacre@gmx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifndef DAS_UTIL_REGEX_HH
#define DAS_UTIL_REGEX_HH

#include <string>
#include <vector>
#include <stdexcept>

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include <regex.h>


namespace das {


class regex
  : boost::noncopyable
{
  public:
    struct compile_error
      : public std::runtime_error
    {
        compile_error(std::string const & w)
          : std::runtime_error(w)
        {
        }
    };

    struct matches {
        matches(std::string const & str, std::size_t n)
          : _str(str)
          , _regmatches(n)
        { }

        bool valid() const { return _valid; }

        bool has(std::size_t index) {
            regmatch_t const & rm = _regmatches[index];
            return ((rm.rm_eo - rm.rm_so) != 0);
        }

        std::string get(std::size_t index) {
            regmatch_t const & rm = _regmatches[index];
            std::size_t len = rm.rm_eo - rm.rm_so;
            if (len) {
                return std::string(_str.c_str() + rm.rm_so, len);
            } else {
                return "";
            }
        }

        operator bool() const { return valid(); }
        std::string operator[](std::size_t index) { return get(index); }


      private:
        friend class regex;
        ::regmatch_t *regmatches() { return &_regmatches.front(); }
        void set_valid(bool b) { _valid = b; }

        std::string const & _str;
        std::vector< ::regmatch_t> _regmatches;
        bool _valid;
    };


    regex(std::string const & pattern) {
        int error = ::regcomp(&_reg, pattern.c_str(), REG_EXTENDED);
        _freer.reset(&_reg, ::regfree);

        if (error) {
            std::size_t bufsize = ::regerror(error, &_reg, NULL, 0);
            std::vector<char> buf(bufsize);
            ::regerror(error, &_reg, &(*buf.begin()), bufsize);

            throw compile_error(&*buf.begin());
        }
    }


    bool match(std::string const & str) const {
        return ::regexec(&_reg, str.c_str(), 0, NULL, 0) == 0;
    }

    matches match(std::string const & str, std::size_t nmatches) const {
        matches m(str, nmatches);
        m.set_valid(::regexec(&_reg, str.c_str(), nmatches, m.regmatches(), 0) == 0);
        return m;
    }

  private:
    ::regex_t _reg;
    boost::shared_ptr<void> _freer;
};


} // namespace das


#endif // DAS_UTIL_REGEX_HH
