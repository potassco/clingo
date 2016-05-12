//
//  Copyright (c) Benjamin Kaufmann
//
//  This is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version. 
// 
//  This file is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <http://www.gnu.org/licenses/>.
//
#ifndef ALARM_HELPER_H_INCLUDED
#define ALARM_HELPER_H_INCLUDED
/////////////////////////////////////////////////////////////////////////////////////////
// Alarm handling
/////////////////////////////////////////////////////////////////////////////////////////
#include <signal.h>
// Schedules an alarm signal
// The function causes the system to generate a SIGALRM
// signal for the process after the number of real-time seconds
// given in sec.
//
// If seconds is 0, a pending alarm request, if  any,  is  cancelled.
// 
// Alarm requests are not stacked; only one SIGALRM generation
// can be scheduled with this function; 
// 
// setAlarm() returns a value > 0 if the alarm request was set.
// Otherwise it returns 0.
int  setAlarm(unsigned sec);

// Sets the SIGALRM handler
void setAlarmHandler(void(*f)(int));

unsigned long initMainThread();
void resetMainThread();
void protectMainThread(bool);

// Implementation:
//  - On POSIX-systems setAlarm() calls alarm() and setAlarmHandler() calls signal()
//  - On Windows a separate alarm thread is created that
//    waits for sec seconds on a termination event and,
//    if the event is not signaled, calls the installed alarm handler in the context
//    of the alarm thread.
//    NOTE: Since the handler is called from a different thread, alarm
//    handling is subject to race-conditions. 
//    USe lockAlarm(), unlockAlarm() to protect functions that
//    are not thread-safe and are called by the handler
#if defined(_WIN32) || defined(_WIN64) 
void lockAlarm();
void unlockAlarm();
#else
inline void lockAlarm()    {}
inline void unlockAlarm()  {}
#endif

struct ScopedAlarmLock {
	ScopedAlarmLock()  { lockAlarm(); }
	~ScopedAlarmLock() { unlockAlarm();  }
};

#define SCOPE_ALARM_LOCK() ScopedAlarmLock __alarm_lock__


#if !defined(SIGALRM)
#define SIGALRM 14
#endif

#endif
