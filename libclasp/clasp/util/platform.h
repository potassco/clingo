// 
// Copyright (c) 2006-2012, Benjamin Kaufmann
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
#if !defined(WITH_THREADS)
#error Invalid thread configuration - use WITH_THREADS=0 for single-threaded or WITH_THREADS=1 for multi-threaded version of libclasp!
#endif

#define STRING2(x) #x
#define STRING(x) STRING2(x)

#if defined(_MSC_VER) && _MSC_VER >= 1200
#define CLASP_PRAGMA_TODO(X) __pragma(message(__FILE__ "(" STRING(__LINE__) ") : TODO: " X))
#define FUNC_NAME __FUNCTION__
#include <basetsd.h>
#if _MSC_VER >= 1600
#include <stdint.h>
#endif
typedef UINT8     uint8;
typedef UINT16    uint16;
typedef INT16     int16;
typedef INT32     int32;
typedef UINT32    uint32;
typedef UINT64    uint64;
typedef INT64     int64;
typedef UINT_PTR  uintp;
typedef INT16     int16;
#define PRIu64 "llu"
#define PRId64 "lld"
#elif defined(__GNUC__) && __GNUC__ >= 3
#define FUNC_NAME __PRETTY_FUNCTION__
#if !defined(__STDC_FORMAT_MACROS)
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
typedef uint8_t   uint8;
typedef uint16_t  uint16;
typedef int16_t   int16;
typedef int32_t   int32;
typedef uint32_t  uint32;
typedef uint64_t  uint64;
typedef int64_t   int64;
typedef uintptr_t uintp;
#define APPLY_PRAGMA(x) _Pragma (#x)
#define CLASP_PRAGMA_TODO(x) APPLY_PRAGMA(message ("TODO: " #x))
#else 
#error unknown compiler or platform. Please add typedefs manually.
#endif
#ifndef UINT32_MAX
#define UINT32_MAX (~uint32(0))
#endif
#ifndef UINT64_MAX
#define UINT64_MAX (~uint64(0))
#endif
#ifndef INT64_MAX
#define INT64_MAX ((int64)(UINT64_MAX >> 1))
#endif
#ifndef UINTP_MAX
#define UINTP_MAX (~uintp(0))
#endif
#ifndef INT16_MAX
#define INT16_MAX (0x7fff)
#endif
#ifndef INT16_MIN
#define	INT16_MIN (-INT16_MAX - 1)
#endif
#ifndef FUNC_NAME
#define FUNC_NAME __FILE__
#endif


template <class T>
bool aligned(void* mem) {
	uintp x = reinterpret_cast<uintp>(mem);
#if (_MSC_VER >= 1300)
	return (x & (__alignof(T)-1)) == 0;
#elif defined(__GNUC__)
	return (x & (__alignof__(T)-1)) == 0;
#else
	struct AL { char x; T y; };
	return (x & (sizeof(AL)-sizeof(T))) == 0;
#endif
}

#if !defined(CLASP_HAS_STATIC_ASSERT)
#	if defined(__cplusplus) && __cplusplus >= 201103L 
#		define CLASP_HAS_STATIC_ASSERT 1
#	elif defined(static_assert)
#		define CLASP_HAS_STATIC_ASSERT 1
#	elif defined(_MSC_VER) && _MSC_VER >= 1600
#		define CLASP_HAS_STATIC_ASSERT 1
#	else
#		define CLASP_HAS_STATIC_ASSERT 0
#	endif
#endif

#if !defined(CLASP_HAS_STATIC_ASSERT) || CLASP_HAS_STATIC_ASSERT == 0
template <bool> struct static_assertion;
template <>     struct static_assertion<true> {};
#ifndef __GNUC__
#define static_assert(x, message) typedef bool clasp_static_assertion[sizeof(static_assertion< (x) >)] 
#else
#define static_assert(x, message) typedef bool clasp_static_assertion[sizeof(static_assertion< (x) >)]  __attribute__((__unused__))
#endif
#endif

extern const char* clasp_format_error(const char* m, ...);
extern const char* clasp_format(char* buf, unsigned size, const char* m, ...);

#define CLASP_FAIL_IF(exp, fmt, ...) \
	(void)( (!(exp)) || (throw std::logic_error(clasp_format_error(fmt, ##__VA_ARGS__ )), 0))	

#ifndef CLASP_NO_ASSERT_CONTRACT

#define CLASP_ASSERT_CONTRACT_MSG(exp, msg) \
	(void)( (!!(exp)) || (throw std::logic_error(clasp_format_error("%s@%d: contract violated: %s", FUNC_NAME, __LINE__, (msg))), 0))

#else
#include <cassert>
#define CLASP_ASSERT_CONTRACT_MSG(exp, msg) assert((exp) && ((msg),true))
#endif

#define CLASP_ASSERT_CONTRACT(exp) CLASP_ASSERT_CONTRACT_MSG(exp, #exp)

#if !defined(CLASP_ENABLE_PRAGMA_TODO) || CLASP_ENABLE_PRAGMA_TODO==0
#undef CLASP_PRAGMA_TODO
#define CLASP_PRAGMA_TODO(X)
#endif

#include <stdlib.h>
#include <string.h>
#if _WIN32||_WIN64
#include <malloc.h>
inline void* alignedAlloc(size_t size, size_t align) { return _aligned_malloc(size, align); }
inline void  alignedFree(void* p)                    { _aligned_free(p); }
#else
inline void* alignedAlloc(size_t size, size_t align) {
	void* result = 0;
	return posix_memalign(&result, align, size) == 0 ? result : static_cast<void*>(0);
}
inline void alignedFree(void* p) { free(p); }
#endif

#endif
