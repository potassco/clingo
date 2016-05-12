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

#ifndef __GRINGO_MINGW_H
#define __GRINGO_MINGW_H

#include <iterator>

// Note 1: harmless because I don't have any throwing move constructors
// Note 2: should be first include

#undef _GLIBCXX_MAKE_MOVE_ITERATOR
#undef _GLIBCXX_MAKE_MOVE_IF_NOEXCEPT_ITERATOR

#define _GLIBCXX_MAKE_MOVE_ITERATOR(_Iter) std::make_move_iterator(_Iter)
#define _GLIBCXX_MAKE_MOVE_IF_NOEXCEPT_ITERATOR(_Iter) std::make_move_iterator(_Iter)

#ifdef MISSING_STD_TO_STRING

#include <sstream>

namespace std {

inline string to_string(int x) {
    ostringstream ss;
    ss << x;
    return ss.str();
}

inline string to_string(unsigned x) {
    ostringstream ss;
    ss << x;
    return ss.str();
}

inline string to_string(long x) {
    ostringstream ss;
    ss << x;
    return ss.str();
}

inline string to_string(unsigned long x) {
    ostringstream ss;
    ss << x;
    return ss.str();
}

inline string to_string(long long x) {
    ostringstream ss;
    ss << x;
    return ss.str();
}

inline string to_string(unsigned long long x) {
    ostringstream ss;
    ss << x;
    return ss.str();
}

} // namespace std

#endif // MISSING_STD_TO_STRING

#endif // __GRINGO_MINGW_H
