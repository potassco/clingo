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

#ifndef _GRINGO_PRIMES_HH
#define _GRINGO_PRIMES_HH

#include <cstdint>
#include <type_traits>

namespace Gringo {

template <class T>
T constexpr maxPrime();

template <>
inline constexpr uint32_t maxPrime<uint32_t>() { return 0xfffffffb; }

template <>
inline constexpr uint64_t maxPrime<uint64_t>() { return 0xffffffffffffffc5; }

uint32_t nextPrime(uint32_t n);
uint64_t nextPrime(uint64_t n);

} // namspace Gringo

#endif
