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

#ifndef _GRINGO_LOCATABLE_HH
#define _GRINGO_LOCATABLE_HH

#include <gringo/value.hh>

namespace Gringo {

// {{{ declaration of Location

struct Location {
    Location(FWString beginFilename, unsigned beginLine, unsigned beginColumn, FWString endFilename, unsigned endLine, unsigned endColumn);
    //Location(Location const &loc);
    //Location(Location &&loc);
    
    FWString beginFilename;
    FWString endFilename;
    unsigned beginLine;
    unsigned endLine;
    unsigned beginColumn;
    unsigned endColumn;

    Location operator+(Location const &other) const;
    bool operator<(Location const &other) const;
};

std::ostream &operator<<(std::ostream &out, Location const &loc);

// }}}
// {{{ declaration of Locatable

class Locatable {
public:
    virtual Location const &loc() const = 0;
    virtual void loc(Location const &loc) = 0;
    virtual ~Locatable() { }
};

// }}}
// {{{ declaration of LocatableClass

template <class T>
class LocatableClass : public T {
public:
    template <typename... Args>
    LocatableClass(Location const &loc, Args&&... args);
    virtual void loc(Location const &loc) final;
    virtual Location const &loc() const final;
    virtual ~LocatableClass();
private:
    Location loc_;
};

// }}}
// {{{ declaration of make_locatable<T,Args...>

template <class T, class... Args>
std::unique_ptr<T> make_locatable(Location const &loc, Args&&... args);

// }}}

// {{{ defintion of Location

inline Location::Location(FWString beginFilename, unsigned beginLine, unsigned beginColumn, FWString endFilename, unsigned endLine, unsigned endColumn)
    : beginFilename(beginFilename)
    , endFilename(endFilename)
    , beginLine(beginLine)
    , endLine(endLine)
    , beginColumn(beginColumn)
    , endColumn(endColumn) { }

inline Location Location::operator+(Location const &other) const {
    return Location(beginFilename, beginLine, beginColumn, other.endFilename, other.endLine, other.endColumn);
}

inline bool Location::operator<(Location const &x) const {
    if (beginFilename != x.beginFilename) { return beginFilename < x.beginFilename; }
    if (endFilename != x.endFilename) { return endFilename < x.endFilename; }
    if (beginLine != x.beginLine) { return beginLine < x.beginLine; }
    if (endLine != x.endLine) { return endLine < x.endLine; }
    if (beginColumn != x.beginColumn) { return beginColumn < x.beginColumn; }
    return endColumn < x.endColumn;
}

inline std::ostream &operator<<(std::ostream &out, Location const &loc) {
    out << *loc.beginFilename << ":" << loc.beginLine << ":" << loc.beginColumn;
    if (loc.beginFilename != loc.endFilename) {
        out << "-" << *loc.endFilename << ":" << loc.endLine << ":" << loc.endColumn;
    }
    else if(loc.beginLine != loc.endLine) {
        out << "-" << loc.endLine << ":" << loc.endColumn;
    }
    else if (loc.beginColumn != loc.endColumn) {
        out << "-" << loc.endColumn;
    }
    return out;
}

// }}}
// {{{ defintion of LocatableClass<T>

template <class T>
template <typename... Args>
LocatableClass<T>::LocatableClass(Location const &loc, Args&&... args)
    : T(std::forward<Args>(args)...)
    , loc_(loc) { }

template <class T>
Location const &LocatableClass<T>::loc() const {
    return loc_;
}

template <class T>
void LocatableClass<T>::loc(Location const &loc) {
    loc_ = loc;
}

template <class T>
LocatableClass<T>::~LocatableClass() { }

// }}}
// {{{ defintion of make_locatable<T, Args>

template <class T, class... Args>
std::unique_ptr<T> make_locatable(Location const &loc, Args&&... args) {
    return gringo_make_unique<LocatableClass<T>>(loc, std::forward<Args>(args)...);
}

// }}}

} // namespace Gringo

#endif // _GRINGO_LOCATABLE_HH
