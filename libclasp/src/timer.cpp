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
#include <clasp/util/timer.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // exclude APIs such as Cryptography, DDE, RPC, Shell, and Windows Sockets.
#define NOMINMAX            // do not let windows.h define macros min and max
#include <windows.h>        // GetProcessTimes, GetCurrentProcess, FILETIME
#define TICKS_PER_SEC 10000000

namespace Clasp {

double RealTime::getTime() {
	union Convert {
		FILETIME time;
		__int64  asUint;
	} now;
	GetSystemTimeAsFileTime(&now.time);
	return static_cast<double>(now.asUint/static_cast<double>(TICKS_PER_SEC));
}

double ProcessTime::getTime() {
	FILETIME ignoreStart, ignoreExit;
	union Convert {
		FILETIME time;
		__int64  asUint;
	} user, system;
	GetProcessTimes(GetCurrentProcess(), &ignoreStart, &ignoreExit, &user.time, &system.time);
	return (user.asUint + system.asUint) / double(TICKS_PER_SEC);
}

double ThreadTime::getTime() {
	FILETIME ignoreStart, ignoreExit;
	union Convert {
		FILETIME time;
		__int64  asUint;
	} user, system;
	GetThreadTimes(GetCurrentThread(), &ignoreStart, &ignoreExit, &user.time, &system.time);
	return (user.asUint + system.asUint) / double(TICKS_PER_SEC);
}

}
#else
#include <sys/times.h>   // times()
#include <sys/time.h>    // gettimeofday()
#include <unistd.h>      // sysconf()
#include <sys/resource.h>// getrusage
#include <limits>
#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/thread_info.h>
#endif
namespace Clasp {

double RealTime::getTime() {
	struct timeval now;
	gettimeofday(&now, NULL);
	return static_cast<double>(now.tv_sec) + static_cast<double>(now.tv_usec / 1000000.0);
}

double ProcessTime::getTime() {
	struct tms nowTimes;
	times(&nowTimes);
	return (nowTimes.tms_utime + nowTimes.tms_stime) / double(sysconf(_SC_CLK_TCK));
}
double ThreadTime::getTime() {
	double res = std::numeric_limits<double>::quiet_NaN();
#if defined(RUSAGE_THREAD)
	int who = RUSAGE_THREAD;
	struct rusage usage;
	getrusage(who, &usage);
	res = (static_cast<double>(usage.ru_utime.tv_sec) + static_cast<double>(usage.ru_utime.tv_usec / 1000000.0))
	    + (static_cast<double>(usage.ru_stime.tv_sec) + static_cast<double>(usage.ru_stime.tv_usec / 1000000.0));
#elif __APPLE__
	struct thread_basic_info t_info;
	mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;
	if (thread_info(mach_thread_self(), THREAD_BASIC_INFO, (thread_info_t)&t_info, &t_info_count) == KERN_SUCCESS) {
		time_value_add(&t_info.user_time, &t_info.system_time);
		res = static_cast<double>(t_info.user_time.seconds) + static_cast<double>(t_info.user_time.microseconds / static_cast<double>(TIME_MICROS_MAX));
	}
#endif
	return res;
}

}
#endif

