// 
// Copyright (c) 2010-2016, Benjamin Kaufmann
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

#ifndef CLASP_UTIL_THREAD_H_INCLUDED
#define CLASP_UTIL_THREAD_H_INCLUDED

#if !defined(CLASP_USE_STD_THREAD)

#if _WIN32||_WIN64
#define WIN32_LEAN_AND_MEAN // exclude APIs such as Cryptography, DDE, RPC, Shell, and Windows Sockets.
#define NOMINMAX            // do not let windows.h define macros min and max
#endif
#include <tbb/tbb_thread.h>
namespace Clasp { namespace mt {
	typedef tbb::tbb_thread thread;
	namespace this_thread { using tbb::this_tbb_thread::yield; }
}}

#else
#include <thread>
namespace Clasp { namespace mt {
	using std::thread;
	namespace this_thread { using std::this_thread::yield; }
}}

#endif
#endif
