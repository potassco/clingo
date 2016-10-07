//
// Copyright (c) 2012, Benjamin Kaufmann
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

#ifndef CLASP_UTIL_MUTEX_H_INCLUDED
#define CLASP_UTIL_MUTEX_H_INCLUDED

#if !defined(CLASP_USE_STD_THREAD)
#if _WIN32||_WIN64
#define WIN32_LEAN_AND_MEAN // exclude APIs such as Cryptography, DDE, RPC, Shell, and Windows Sockets.
#define NOMINMAX            // do not let windows.h define macros min and max
#endif
#include <tbb/mutex.h>
#include <tbb/spin_mutex.h>
#if defined(TBB_IMPLEMENT_CPP0X)
#define RESTORE_TBB_IMPLEMENT_CPP0X TBB_IMPLEMENT_CPP0X
#undef TBB_IMPLEMENT_CPP0X
#endif
#define TBB_IMPLEMENT_CPP0X 0
#include <tbb/compat/condition_variable>
#undef TBB_IMPLEMENT_CPP0X
#if defined(RESTORE_TBB_IMPLEMENT_CPP0X)
#define TBB_IMPLEMENT_CPP0X RESTORE_TBB_IMPLEMENT_CPP0X
#undef RESTORE_TBB_IMPLEMENT_CPP0X
#endif
#define NS_MUTEX tbb
#define NS_COMPAT tbb::interface5
#else
#include <mutex>
#include <condition_variable>
#define NS_MUTEX  std
#define NS_COMPAT std
#endif
namespace Clasp { namespace mt {
	using NS_MUTEX::mutex;
	using NS_COMPAT::lock_guard;
	using NS_COMPAT::unique_lock;
	using NS_COMPAT::swap;
	using NS_COMPAT::defer_lock_t;
	struct condition_variable : private NS_COMPAT::condition_variable {
		typedef NS_COMPAT::condition_variable base_type;
		using base_type::notify_one;
		using base_type::notify_all;
		using base_type::wait;
		using base_type::native_handle;

		bool wait_for(unique_lock<mutex>& lock, double timeInSecs);
	};
#if !defined(CLASP_USE_STD_THREAD)
	// Due to a bug in the computation of the wait time in older tbb versions
	// wait_for might fail with eid_condvar_wait_failed.
	// See: http://software.intel.com/en-us/forums/topic/280012
	// Ignore the error and retry the wait - the computed wait time will be valid, eventually.
	inline bool condition_variable::wait_for(unique_lock<mutex>& lock, double s) {
		if (s < 0) { throw std::logic_error("invalid wait time"); }
		for (;;) {
			try { return base_type::wait_for(lock, tbb::tick_count::interval_t(s)) == NS_COMPAT::no_timeout; }
			catch (const std::runtime_error&) {}
		}
	}
#else
	inline bool condition_variable::wait_for(unique_lock<mutex>& lock, double s) {
		return base_type::wait_for(lock, std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::duration<double>(s)))
			== std::cv_status::no_timeout;
	}
#endif
}}

#undef NS_MUTEX
#undef NS_COMPAT

#endif
