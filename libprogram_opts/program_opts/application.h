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
#ifndef APP_OPTIONS_H_INCLUDED
#define APP_OPTIONS_H_INCLUDED

#ifdef _MSC_VER
#pragma warning (disable : 4200) // nonstandard extension used : zero-sized array
#pragma once
#endif

#include <string>
#include <utility>
#include "program_options.h"
#include <stdio.h>
#include <signal.h>

namespace ProgramOptions {
typedef std::vector<std::string> StringSeq;
/////////////////////////////////////////////////////////////////////////////////////////
// Application base class
/////////////////////////////////////////////////////////////////////////////////////////
#define WRITE_STDERR(type,sys,msg) ( fprintf(stderr, "*** %-5s: (%s): %s\n", (type),(sys),(msg)), fflush(stderr) )
class Application {
public:
	//! Description of and max value for help option.
	typedef std::pair<const char*, unsigned> HelpOpt;
	/*!
	 * \name Basic functions.
	 */
	//@{
	//! Returns the name of this application.
	virtual const char* getName()       const   = 0;
	//! Returns the version number of this application.
	virtual const char* getVersion()    const   = 0;
	//! Returns a null-terminated array of signals that this application handles.
	virtual const int*  getSignals()    const { return 0; }
	//! Returns the usage information of this application.
	virtual const char* getUsage()      const { return "[options]"; }
	//! Returns the application's help option and its description.
	virtual HelpOpt     getHelpOption() const { return HelpOpt("Print help information and exit", 1); }
	//! Returns the parser function for handling positional options.
	virtual PosOption   getPositional() const { return 0; }
	//! Prints the given error message to stderr.
	virtual void        error(const char* msg) const { WRITE_STDERR("ERROR", getName(), msg); }
	//! Prints the given info message to stderr.
	virtual void        info(const char*  msg) const { WRITE_STDERR("Info",  getName(), msg); }
	//! Prints the given warning message to stderr.
	virtual void        warn(const char*  msg) const { WRITE_STDERR("Warn",  getName(), msg); }
	//@}
	
	/*!
	 * \name Main functions.
	 */
	//@{
	//! Runs this application with the given command-line arguments.
	int main(int argc, char** argv);
	//! Sets the value that should be returned as the application's exit code. 
	void setExitCode(int n);
	//! Returns the application's exit code.
	int  getExitCode() const;
	//! Returns the application object that is running.
	static Application* getInstance();
	//! Prints the application's help information (called if options contain '--help').
	virtual void        printHelp(const OptionContext& root);
	//! Prints the application's version message (called if options contain '--version').
	virtual void        printVersion();
	//! Prints the application's usage message (default is: "usage: getName() getUsage()").
	virtual void        printUsage();
	//@}
protected:
	/*!
	 * \name Life cycle and option handling
	 */
	//@{
	//! Adds all application options to the given context.
	virtual void        initOptions(OptionContext& root) = 0;
	//! Validates parsed options. Shall throw to signal error.
	virtual void        validateOptions(const OptionContext& root, const ParsedOptions& parsed, const ParsedValues& values) = 0;
	//! Called once after option processing is done.
	virtual void        setup() = 0;
	//! Shall run the application. Called after setup and option processing.
	virtual void        run()   = 0;
	//! Called after run returned. The default is a noop.
	virtual void        shutdown();
	//! Called on an exception from run(). The default terminates the application.
	virtual void        onUnhandledException();
	//! Called when a signal is received. Tthe default terminates the application.
	virtual bool        onSignal(int);
	//@}
protected:
	Application();
	virtual ~Application();
	void     shutdown(bool hasError);
	void     exit(int exitCode) const;
	unsigned verbose() const;
	void     setVerbose(unsigned v);
	void     killAlarm();
	int      blockSignals();
	void     unblockSignals(bool deliverPending);
	void     processSignal(int sigNum);
private:
	bool getOptions(int argc, char** argv);
	int                   exitCode_;  // application's exit code
	unsigned              timeout_;   // active time limit or 0 for no limit
	unsigned              verbose_;   // active verbosity level
	bool                  fastExit_;  // force fast exit?
	volatile long         blocked_;   // temporarily block signals?
	volatile long         pending_;   // pending signal or 0 if no pending signal
	static Application*   instance_s; // running instance (only valid during run()).
	static void sigHandler(int sig);  // signal/timeout handler
};

}
#endif
