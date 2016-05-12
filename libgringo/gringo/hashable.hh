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

#ifndef _GRINGO_HASHABLE_HH
#define _GRINGO_HASHABLE_HH

#include <ostream>
#include <typeinfo>
#include <functional>
#include <gringo/utility.hh>

#define GRINGO_HASH(T) namespace std { template <> struct hash<T> : hash<Gringo::Hashable> { }; } 
#define GRINGO_CALL_HASH(T) namespace std { template <> struct hash<T> { size_t operator()(T const &x) const { return x.hash(); } }; } 

namespace Gringo {

// {{{ declaration of Hashable

class Hashable {
public:
    virtual size_t hash() const = 0;
    virtual ~Hashable() { }
};

// }}}
// {{{ definition of call_hash

template <class T>
struct call_hash {
    size_t operator()(T const &x) const { return x.hash(); }
};

// }}}

} // namespace Gringo

namespace std {

// {{{ definition of hash<Gringo::Hashable>

template <>
struct hash<Gringo::Hashable> {
    size_t operator()(Gringo::Hashable const &x) const { return x.hash(); }
};

// }}}

} // namespace std

#endif // _GRINGO_HASHABLE_HH

