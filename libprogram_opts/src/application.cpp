//
//  Copyright (c) Benjamin Kaufmann 2004
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
//
// NOTE: ProgramOptions is inspired by Boost.Program_options
//       see: www.boost.org/libs/program_options
//
#include <program_opts/application.h>
#include <program_opts/typed_value.h>
#include <program_opts/detail/alarm.h>
#include <cctype>
#include <limits.h>
#include <cstring>
#ifdef _MSC_VER
#pragma warning (disable : 4996)
#endif
#include <stdio.h>
#include <stdlib.h>
#if !defined(_WIN32)
#include <unistd.h> // for _exit
static long fetch_and_inc(volatile long& x) {
	return __sync_fetch_and_add(&x, 1);
}
static long fetch_and_dec(volatile long& x) {
	return __sync_fetch_and_sub(&x, 1);
}
#else
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
static long fetch_and_inc(volatile long& x) {
	return InterlockedIncrement(&x) - 1;
}
static long fetch_and_dec(volatile long& x) {
	return InterlockedDecrement(&x) + 1;
}
#endif
using namespace ProgramOptions;
using namespace std;
namespace ProgramOptions {
/////////////////////////////////////////////////////////////////////////////////////////
// Application
/////////////////////////////////////////////////////////////////////////////////////////
Application* Application::instance_s = 0;
Application::Application() : exitCode_(EXIT_FAILURE), timeout_(0), verbose_(0), fastExit_(false), blocked_(0), pending_(0) {}
Application::~Application() { 
	if (instance_s == this) { instance_s = 0; resetMainThread(); }
}
// Application entry point.
int Application::main(int argc, char** argv) {
	instance_s = this; // singleton instance used for signal handling
	initMainThread();
	exitCode_  = EXIT_FAILURE;
	blocked_   = pending_ = 0;
	if (getOptions(argc, argv)) {
		// install signal handlers
		for (const int* sig = getSignals(); sig && *sig; ++sig) {
			if (signal(*sig, &Application::sigHandler) == SIG_IGN) {
				signal(*sig, SIG_IGN);
			}
		}
		if (timeout_) {
			setAlarmHandler(&Application::sigHandler);
			if (setAlarm(timeout_) == 0) { warn("Could not set time limit!"); }
		}
		exitCode_ = EXIT_SUCCESS;
		try         { setup(); run(); shutdown(false); }
		catch (...) { shutdown(true); }
	}
	if (fastExit_) { exit(exitCode_); }
	fflush(stdout); 
	fflush(stderr);
	return exitCode_;
}

Application* Application::getInstance() {
	return instance_s;
}

void Application::onUnhandledException() {
	try { throw; }
	catch (const std::exception& e) { error(e.what()); }
	catch (...)                     { error("Unknown exception"); }
	exit(EXIT_FAILURE);
}

void Application::setExitCode(int n) {
	exitCode_ = n;
}

int Application::getExitCode() const {
	return exitCode_;
}

// Called on application shutdown
void Application::shutdown(bool hasError) {
	// ignore signals/alarms during shutdown
	fetch_and_inc(blocked_);
	killAlarm();
	if (hasError) { onUnhandledException(); }
	shutdown();
}

void Application::shutdown() {}

// Force exit without calling destructors.
void Application::exit(int status) const {
	fflush(stdout);
	fflush(stderr);
	_exit(status);
}

// Temporarily disable delivery of signals.
int Application::blockSignals() {
	lockAlarm();
	int x = fetch_and_inc(blocked_);
	unlockAlarm();
	return x;
}

// Re-enable signal handling and deliver any pending signal.
void Application::unblockSignals(bool deliverPending) {
	if (fetch_and_dec(blocked_) == 1) {
		int pend = pending_;
		pending_ = 0;
		// directly deliver any pending signal to our sig handler
		if (pend && deliverPending) { processSignal(pend); }
	}
}

void Application::sigHandler(int sig) {
	if (sig != SIGALRM) {
		// reset handler because on Windows and original Unix, a handler once invoked is set to SIG_DFL
		signal(sig, sigHandler);
		struct Protect {
			Protect() { protectMainThread(true);  }
			~Protect(){ protectMainThread(false); }
			void process(int sig) { Application::instance_s->processSignal(sig); }
		};
		Protect().process(sig);
	}
	else { Application::instance_s->processSignal(sig); }
}

// Called on timeout or signal.
void Application::processSignal(int sig) {
	if (fetch_and_inc(blocked_) == 0) {
		if (!onSignal(sig)) { return; } // block further signals
	}
	else if (pending_ == 0) { // signals are currently blocked because output is active
		info("Queueing signal...");
		pending_ = sig;
	}
	fetch_and_dec(blocked_);
}

bool Application::onSignal(int x) {
	info("INTERRUPTED by signal!");
	exit(EXIT_FAILURE | (128+x));
	return false;
}

// Kill any pending alarm.
void Application::killAlarm() {
	if (timeout_ > 0) { setAlarm(0); }
}

namespace {
	struct HelpParser {
		static unsigned maxValue_s;
		static bool parse(const std::string& v, unsigned& out) {
			return string_cast(v, out) && out > 0 && out <= maxValue_s;
		}
	};
	unsigned HelpParser::maxValue_s = 0;
}

// Process command-line options.
bool Application::getOptions(int argc, char** argv) {
	using namespace ProgramOptions;
	unsigned help = 0;
	bool version  = false;
	try {
		ParsedOptions parsed; // options found in command-line
		OptionContext allOpts(std::string("<").append(getName()).append(">"));
		HelpOpt helpO = getHelpOption();
		if (helpO.second == 0) { error("Invalid help option!"); exit(EXIT_FAILURE); }
		OptionGroup basic("Basic Options");
		HelpParser::maxValue_s = helpO.second;
		Value* hv = helpO.second == 1 ? storeTo(help)->flag() : storeTo(help, &HelpParser::parse)->arg("<n>")->implicit("1");
		basic.addOptions()
			("help,h"      , hv                               , helpO.first)
			("version,v"   , flag(version)                    , "Print version information and exit")
			("verbose,V"   , storeTo(verbose_ = 0)->implicit("-1")->arg("<n>"), "Set verbosity level to %A")
			("time-limit"  , storeTo(timeout_ = 0)->arg("<n>"), "Set time limit to %A seconds (0=no limit)")
			("fast-exit,@1", flag(fastExit_   = false)        , "Force fast exit (do not call dtors)")
		;
		allOpts.add(basic);
		initOptions(allOpts);
		ParsedValues values = parseCommandLine(argc, argv, allOpts, false, getPositional());
		parsed.assign(values);
		allOpts.assignDefaults(parsed);
		if (help || version) {
			exitCode_ = EXIT_SUCCESS;
			if (help) {
				DescriptionLevel x = (DescriptionLevel)(help-1);
				allOpts.setActiveDescLevel(x);
				printHelp(allOpts);
			}
			else {
				printVersion();
			}
			return false;
		}
		validateOptions(allOpts, parsed, values);
	}
	catch(const std::exception& e) {
		error(e.what());
		info("Try '--help' for usage information");
		return false;
	}
	return true;
}

void Application::printHelp(const OptionContext& root) {
	printf("%s version %s\n", getName(), getVersion());
	printUsage();
	ProgramOptions::FileOut out(stdout);
	root.description(out);
	printf("\n");
	printUsage();
	printf("Default command-line:\n%s %s\n", getName(), root.defaults(strlen(getName())+1).c_str());
	fflush(stdout);
}
void Application::printVersion() {
	printf("%s version %s\n", getName(), getVersion());
	printf("Address model: %d-bit\n", (int)(sizeof(void*)*CHAR_BIT));	
	fflush(stdout);
}

void Application::printUsage() {
	printf("usage: %s %s\n", getName(), getUsage());
}

unsigned Application::verbose() const {
	return verbose_;
}
void Application::setVerbose(unsigned v) {
	verbose_ = v;
}

}
