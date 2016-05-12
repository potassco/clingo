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
#include <program_opts/detail/alarm.h>
#if !defined(_WIN32) && !defined(_WIN64) 
#include <unistd.h>
void setAlarmHandler(void(*f)(int)) {
	signal(SIGALRM, f);
}
int setAlarm(unsigned sec) {
	alarm(sec);
	return 1;
}
unsigned long initMainThread() { return 0; }
void resetMainThread(){}
void protectMainThread(bool) {}
#else 
#define WIN32_LEAN_AND_MEAN // exclude APIs such as Cryptography, DDE, RPC, Shell, and Windows Sockets.
#define NOMINMAX            // do not let windows.h define macros min and max
#include <process.h>        // _beginthreadex, _endthreadex, ...
#include <windows.h>        // WaitForSingleObject, CRITICAL_SECTION, ...
namespace {
typedef CRITICAL_SECTION LOCK;
// Since Windows does not support alarm(),
// we need a separate thread for timeout management.
class AlarmThread {
public:
	AlarmThread();
	~AlarmThread();
	// Creates a thread that waits for an event to be signaled. If the event
	// is not signaled after sec seconds, the alarm handler is called.
	int start(unsigned sec);
	// Kills a pending alarm. I.e. wakes up the alarm thread and
	// waits until it has completed.
	void kill();
	// Sets the alarm handler
	void setCallback( void(*f)(int) ) {
		callback = f;
	}
	static void ign(int) {}
	static HANDLE mainHandle_s;
	static DWORD  mainId_s;
	static LOCK*  signalLock_s;
private:
	HANDLE  threadHandle;  // alarm thread handle
	HANDLE  eventHandle;   // wake up event handle
	void (*callback)(int);
	unsigned sec;
	static unsigned __stdcall run(void* p);
};

HANDLE AlarmThread::mainHandle_s = INVALID_HANDLE_VALUE;
DWORD  AlarmThread::mainId_s     = 0;
LOCK*  AlarmThread::signalLock_s = 0;
static struct LockInit {
	LockInit() { InitializeCriticalSection((AlarmThread::signalLock_s=new CRITICAL_SECTION)); }
	~LockInit(){ DeleteCriticalSection(AlarmThread::signalLock_s); delete AlarmThread::signalLock_s;  }
} lock_s;

AlarmThread::AlarmThread() : threadHandle(0), callback(&AlarmThread::ign) {
	eventHandle = CreateEvent(0, FALSE, FALSE, TEXT("KillAlarmEvent"));
}
AlarmThread::~AlarmThread() {
	if (eventHandle != INVALID_HANDLE_VALUE) {
		CloseHandle(eventHandle);
	}
	resetMainThread();
}
int AlarmThread::start(unsigned int sec) {
	if (!initMainThread()) { return 0; }
	if (eventHandle != INVALID_HANDLE_VALUE) {
		if (threadHandle) kill(); // kill existing timer
		if (!callback) callback = &AlarmThread::ign;
		this->sec   = sec;
		threadHandle= (HANDLE)_beginthreadex(0, 0, &AlarmThread::run, this, 0, 0);
	}
	return threadHandle != 0;
}
void AlarmThread::kill() {
	if (threadHandle) {
		SetEvent(eventHandle);
		WaitForSingleObject(threadHandle, INFINITE);
		threadHandle = 0;
	}
}
unsigned __stdcall AlarmThread::run(void* p) {
	AlarmThread* self = (AlarmThread*)p;
	if (WaitForSingleObject(self->eventHandle, self->sec * 1000) == WAIT_TIMEOUT) {
		struct ScopedProtect {
			ScopedProtect() { protectMainThread(true); }
			~ScopedProtect(){ protectMainThread(false);}
			void raise(AlarmThread* x) { x->callback(SIGALRM); }
		};
		ScopedProtect().raise(self);
	}
	return 0;
}

void (*alarmHandler)(int) = &AlarmThread::ign;

} // unnamed namespace

// public alarm functions

void setAlarmHandler(void(*f)(int)) {
	alarmHandler = f;
}

int setAlarm(unsigned sec) {
	static AlarmThread alarmT;
	if (sec > 0) { 
		alarmT.setCallback(alarmHandler);
		return alarmT.start(sec);
	}
	else         { 
		alarmT.kill();
		return 1;
	}
}
void lockAlarm()   { if (AlarmThread::signalLock_s) EnterCriticalSection(AlarmThread::signalLock_s); }
void unlockAlarm() { if (AlarmThread::signalLock_s) LeaveCriticalSection(AlarmThread::signalLock_s); }

unsigned long initMainThread() {
	if (AlarmThread::mainHandle_s != INVALID_HANDLE_VALUE && GetCurrentThreadId() != AlarmThread::mainId_s) {
		resetMainThread();
	}
	if (AlarmThread::mainHandle_s == INVALID_HANDLE_VALUE) {
		if (DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &AlarmThread::mainHandle_s, 0, FALSE, DUPLICATE_SAME_ACCESS) == 0) {
			return 0;
		}
		AlarmThread::mainId_s = GetCurrentThreadId();
	}
	return AlarmThread::mainId_s;
}
void resetMainThread() {
	if (AlarmThread::mainHandle_s != INVALID_HANDLE_VALUE) {
		CloseHandle(AlarmThread::mainHandle_s);
		AlarmThread::mainHandle_s = INVALID_HANDLE_VALUE;
		AlarmThread::mainId_s     = 0;
	}
}
void protectMainThread(bool protect) {
	if (AlarmThread::mainHandle_s != INVALID_HANDLE_VALUE && GetCurrentThreadId() != AlarmThread::mainId_s) {
		if (protect) { lockAlarm();   SuspendThread(AlarmThread::mainHandle_s); }
		else         { unlockAlarm(); ResumeThread(AlarmThread::mainHandle_s);  }
	}
}
#endif
