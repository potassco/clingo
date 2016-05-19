// 
// Copyright (c) 2006-2007, Benjamin Kaufmann
// 
// This file is part of Clasp. See http://www.cs.uni-potsdam.de/clasp/ 
// 
// Clasp is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// Clasp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Clasp; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//

#ifndef CLASP_PLATFORM_H_INCLUDED
#define CLASP_PLATFORM_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1200
#include <basetsd.h>
typedef UINT8     uint8;
typedef UINT16    uint16;
typedef INT32     int32;
typedef UINT32    uint32;
typedef UINT64    uint64;
typedef INT64     int64;
typedef UINT_PTR  uintp;
#define PRIu64 "llu"
#define PRId64 "lld"
template <unsigned> struct Uint_t;
template <> struct Uint_t<sizeof(uint8)>  { typedef uint8  type; };
template <> struct Uint_t<sizeof(uint16)> { typedef uint16 type; };
template <> struct Uint_t<sizeof(uint32)> { typedef uint32 type; };
template <> struct Uint_t<sizeof(uint64)> { typedef uint64 type; };
#define BIT_MASK(x,n) ( static_cast<Uint_t<sizeof((x))>::type>(1) << (n) )
#elif defined(__GNUC__) && __GNUC__ >= 3
#if !defined(__STDC_FORMAT_MACROS)
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
typedef uint8_t	    uint8;
typedef uint16_t    uint16;
typedef int32_t	    int32;
typedef uint32_t    uint32;
typedef uint64_t    uint64;
typedef int64_t     int64;
typedef uintptr_t   uintp;
#define BIT_MASK(x,n) ( static_cast<__typeof((x))>(1)<<(n) )
#else 
#error unknown compiler or platform. Please add typedefs manually.
#endif
#ifndef UINT32_MAX
#define UINT32_MAX (~uint32(0))
#endif
#ifndef UINT64_MAX
#define UINT64_MAX (~uint64(0))
#endif

// set, clear, toggle bit n of x and return new value
#define set_bit(x,n)   ( (x) |  BIT_MASK((x),(n)) )
#define clear_bit(x,n) ( (x) & ~BIT_MASK((x),(n)) )
#define toggle_bit(x,n)( (x) ^  BIT_MASK((x),(n)) )

// set, clear, toggle bit n of x and store new value in x
#define store_set_bit(x,n)   ( (x) |=  BIT_MASK((x),(n)) )
#define store_clear_bit(x,n) ( (x) &= ~BIT_MASK((x),(n)) )
#define store_toggle_bit(x,n)( (x) ^=  BIT_MASK((x),(n)) )

// return true if bit n in x is set
#define test_bit(x,n)  ( ((x) & BIT_MASK((x),(n))) != 0 )

#endif
