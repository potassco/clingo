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
#include <functional>

#include <iostream>

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

enum class Errors : int {
    Success   = 0,
    Runtime   = 1,
    Logic     = 2,
    Bad_alloc = 3,
    Unknown   = 4
};

enum class Warnings : int {
    OperationUndefined = 0,
    RuntimeError       = 1,
    AtomUndefined      = 2,
    FileIncluded       = 3,
    VariableUnbounded  = 4,
    GlobalVariable     = 5,
    Other              = 6,
};

class Logger {
public:
    using Printer = std::function<void (Warnings, char const *)>;
    Logger(Printer p = nullptr, unsigned limit = 20)
    : p_(p)
    , limit_(limit) { }
    bool check(Errors id);
    bool check(Warnings id);
    bool hasError() const;
    void enable(Warnings id, bool enable);
    void print(Warnings code, char const *msg);
    ~Logger();
private:
    Printer p_;
    unsigned limit_;
    std::bitset<static_cast<int>(Warnings::Other)+1> disabled_;
    bool error_ = false;
};

// }}}1

// {{{1 definition of Logger

inline bool Logger::check(Warnings id) {
    if (id == Warnings::RuntimeError) {
        if (!limit_ && error_) { throw MessageLimitError("too many messages."); }
        if (limit_) { --limit_; }
        error_ = true;
        return true;
    }
    else {
        if (!limit_ && error_) { throw MessageLimitError("too many messages."); }
        return !disabled_[static_cast<int>(id)] && limit_ && (--limit_, true);
    }
}

inline bool Logger::hasError() const {
    return error_;
}

inline void Logger::enable(Warnings id, bool enabled) {
    disabled_[static_cast<int>(id)] = !enabled;
}

inline void Logger::print(Warnings code, char const *msg) {
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
    Report(Logger &p, Warnings code) : p_(p), code_(code) { }
    ~Report() { p_.print(code_, out.str().c_str()); }
    std::ostringstream out;
private:
    Logger &p_;
    Warnings code_;
};

// }}}1

} // namespace GRINGO

#define GRINGO_REPORT(p, id) \
if (!(p).check(id)) { } \
else Gringo::Report(p, id).out

#endif // _GRINGO_REPORT_HH

