// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Roland Kaminski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// }}}

#ifndef _GRINGO_REPORT_HH
#define _GRINGO_REPORT_HH

#include <bitset>
#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <memory>

#include <iostream>
#include <clingo.h>

namespace Gringo {

// {{{1 declaration of GringoError

class GringoError : public std::runtime_error {
public:
    GringoError(char const *msg) : std::runtime_error(msg) { }
};

class MessageLimitError : public std::runtime_error {
public:
    MessageLimitError(char const *msg) : std::runtime_error(msg) { }
};

// {{{1 declaration of Logger

using Warnings = clingo_warning;
using Errors = clingo_error;

class Logger {
public:
    using Printer = std::function<void (clingo_warning_t, char const *)>;
    Logger(Printer p = nullptr, unsigned limit = 20)
    : p_(p)
    , limit_(limit) { }
    bool check(Errors id);
    bool check(Warnings id);
    bool hasError() const;
    void enable(Warnings id, bool enable);
    void print(clingo_warning_t code, char const *msg);
    ~Logger();
private:
    Printer p_;
    unsigned limit_;
    std::bitset<clingo_warning_other+1> disabled_;
    bool error_ = false;
};

// }}}1

// {{{1 definition of Logger

inline bool Logger::check(Errors) {
    if (!limit_ && error_) { throw MessageLimitError("too many messages."); }
    if (limit_) { --limit_; }
    error_ = true;
    return true;
}

inline bool Logger::check(Warnings id) {
    if (!limit_ && error_) { throw MessageLimitError("too many messages."); }
    return !disabled_[id] && limit_ && (--limit_, true);
}

inline bool Logger::hasError() const {
    return error_;
}

inline void Logger::enable(Warnings id, bool enabled) {
    disabled_[id] = !enabled;
}

inline void Logger::print(clingo_warning_t code, char const *msg) {
    if (p_) { p_(code, msg); }
    else {
        fprintf(stderr, "%s\n", msg);
        fflush(stderr);
    }
}

inline Logger::~Logger() { }

// {{{1 definition of Report

class Report {
public:
    Report(Logger &p, clingo_warning_t code) : p_(p), code_(code) { }
    ~Report() { p_.print(code_, out.str().c_str()); }
    std::ostringstream out;
private:
    Logger &p_;
    clingo_warning_t code_;
};

// }}}1

} // namespace GRINGO

#define GRINGO_REPORT(p, id) \
if (!(p).check(id)) { } \
else Gringo::Report(p, id).out

#endif // _GRINGO_REPORT_HH

