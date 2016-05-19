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
#include "timer.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // exclude APIs such as Cryptography, DDE, RPC, Shell, and Windows Sockets.
#define NOMINMAX            // do not let windows.h define macros min and max
#include <windows.h>        // GetProcessTimes, GetCurrentProcess, FILETIME

double Timer::clockStamp() {
	FILETIME ignoreStart, ignoreExit;
	union Convert {
		FILETIME time;
		__int64  asUint;
	} user, system;
	GetProcessTimes(GetCurrentProcess(), &ignoreStart, &ignoreExit, &user.time, &system.time);
	return user.asUint + system.asUint;
}

double Timer::ticksPerSec() {
	return 10000000;
}
#else
#include <sys/times.h>  // times()
#include <unistd.h>     // sysconf()			

double Timer::clockStamp() {
  struct tms nowTimes;
	times(&nowTimes);
	return nowTimes.tms_utime + nowTimes.tms_stime;
}

double Timer::ticksPerSec() {
	return sysconf(_SC_CLK_TCK);
}

#endif
