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

#if WITH_THREADS
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
namespace Clasp { 
	using tbb::mutex; 
	using tbb::spin_mutex;
	using tbb::interface5::condition_variable;
	using tbb::interface5::lock_guard;
	using tbb::interface5::unique_lock;
	using tbb::interface5::swap;
	using tbb::interface5::defer_lock_t;
}
#else
namespace no_multi_threading {
class NullMutex {   
public:   
	NullMutex()     {}
	void lock()     {}
	bool try_lock() { return true; }
	void unlock()   {}
private:
	NullMutex(const NullMutex&);   
	NullMutex& operator=(const NullMutex&);   
};
typedef NullMutex mutex; 
typedef NullMutex spin_mutex;
template<typename M>
class lock_guard {
public:
	typedef M mutex_type;
	explicit lock_guard(mutex_type& m) : pm(m) {m.lock();}
	~lock_guard() { pm.unlock(); }
private:
	lock_guard(const lock_guard&);
	lock_guard& operator=(const lock_guard&);
	mutex_type& pm;
};
}
namespace Clasp {
	using no_multi_threading::mutex;
	using no_multi_threading::lock_guard;
	using no_multi_threading::spin_mutex;
}
#endif

#endif
