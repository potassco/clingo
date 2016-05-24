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

namespace Gringo {

// {{{ declaration of Warnings and Errors

enum Warnings {
    W_OPERATION_UNDEFINED,   //< Undefined arithmetic operation or weight of aggregate.
    W_ATOM_UNDEFINED,        //< Undefined atom in program.
    W_FILE_INCLUDED,         //< Same file included multiple times.
    W_VARIABLE_UNBOUNDED,    //< CSP Domain undefined.
    W_GLOBAL_VARIABLE,       //< Global variable in tuple of aggregate element.
    W_TOTAL,                 //< Not a warning but the total number of warnings.
};

enum Errors { E_ERROR };

// }}}
// {{{ declaration of MessagePrinter

struct MessagePrinter {
    virtual bool check(Errors id) = 0;
    virtual bool check(Warnings id) = 0;
    virtual bool hasError() const = 0;
    virtual void enable(Warnings id) = 0;
    virtual void disable(Warnings id) = 0;
    virtual void print(std::string const &msg) = 0;
    virtual ~MessagePrinter() { }
};

// }}}
// {{{ declaration of DefaultMessagePrinter

struct DefaultMessagePrinter : MessagePrinter {
    virtual bool check(Errors id);
    virtual bool check(Warnings id);
    virtual bool hasError() const;
    virtual void enable(Warnings id);
    virtual void disable(Warnings id);
    virtual void print(std::string const &msg);
    virtual ~DefaultMessagePrinter();
private:
    std::bitset<Warnings::W_TOTAL> disabled_;
    unsigned messageLimit_ = 20;
    bool error_ = false;
};

/*
inline std::unique_ptr<MessagePrinter> &message_printer() {
    static std::unique_ptr<MessagePrinter> x(new DefaultMessagePrinter());
    return x;
}
inline void reset_message_printer() {
    message_printer().reset(new DefaultMessagePrinter());
}
*/

// }}}

// {{{ definition of DefaultMessagePrinter

inline bool DefaultMessagePrinter::check(Errors) {
    if (!messageLimit_ && error_) { throw std::runtime_error("too many messages."); }
    if (messageLimit_) { --messageLimit_; }
    error_ = true;
    return true;
}

inline bool DefaultMessagePrinter::check(Warnings id) {
    if (!messageLimit_ && error_) { throw std::runtime_error("too many messages."); }
    return !disabled_[id] && messageLimit_ && (--messageLimit_, true);
}

inline bool DefaultMessagePrinter::hasError() const {
    return error_;
}

inline void DefaultMessagePrinter::enable(Warnings id) {
    disabled_[id] = false;
}

inline void DefaultMessagePrinter::disable(Warnings id) {
    disabled_[id] = true;
}

inline void DefaultMessagePrinter::print(std::string const &msg) {
    fprintf(stderr, "%s\n", msg.c_str());
    fflush(stderr);
}

inline DefaultMessagePrinter::~DefaultMessagePrinter() { }

// }}}
// {{{ definition of Report

class Report {
public:
    Report(MessagePrinter &p) : p_(p) { }
    ~Report() { p_.print(out.str()); }
    std::ostringstream out;
private:
    MessagePrinter &p_;
};

// }}}

} // namespace GRINGO

#define GRINGO_REPORT(p, id) \
if (!(p)->check(id)) { } \
else Gringo::Report(p).out

#endif // _GRINGO_REPORT_HH

