// 
// Copyright (c) 2009, Benjamin Kaufmann
// 
// This file is part of gringo. See http://www.cs.uni-potsdam.de/gringo/ 
// 
// gringo is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with gringo; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
#ifndef GRINGO_GRINGO_APP_H_INCLUDED
#define GRINGO_GRINGO_APP_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif
#include <program_opts/app_options.h>
#include "gringo_options.h"
#include <gringo/output.h>
#if defined(WITH_CLASP)
#include "clasp/clasp_options.h"
#include "clasp/clasp_output.h"
#include "clingo_options.h"
#include "timer.h"
#endif
#include <iosfwd>
#include <memory>
/////////////////////////////////////////////////////////////////////////////////////////
// Output macros and app exit codes
/////////////////////////////////////////////////////////////////////////////////////////
// exit codes
#define S_SATISFIABLE   10  // problem is satisfiable
#define S_UNSATISFIABLE 20  // problem was proved to be unsatisfiable
#define S_UNKNOWN       0   // satisfiablity of problem not knwon; search was interrupted or did not start
#define S_ERROR EXIT_FAILURE// internal error, except out of memory
#define S_MEMORY        127 // out of memory
/////////////////////////////////////////////////////////////////////////////////////////
// Application
/////////////////////////////////////////////////////////////////////////////////////////
namespace gringo {

// input streams to process
struct Streams {
	Streams() {}
	~Streams();
	typedef std::vector<std::istream*> StreamVec;
	void open(const std::vector<std::string>& fileList);
	StreamVec streams;
private:
	Streams(const Streams&);
	Streams& operator=(const Streams&);
};

// Base class for gringo, clingo, iclingo application
class Application : public AppOptions{
public:
	Application();
	void         printHelp()    const;
	virtual void printVersion() const = 0;
	int          run(int argc, char** argv); // "entry-point"
	static       Application& instance();
	void         printWarnings();
private:
	Application(const Application&);
	Application& operator=(const Application&);
	static void sigHandler(int sig);    // signal/timeout handler
	void    installSigHandlers();       // adds handlers for SIGINT, SIGTERM
	virtual std::string getVersion() const = 0;
	virtual std::string getUsage()   const = 0;
	virtual void handleSignal(int sig)     = 0;
	virtual int  doRun()                   = 0; 
	virtual ProgramOptions::PosOption getPositionalParser() const = 0;
};

// gringo application
class GringoApp : public Application {
public:
	void printVersion() const;
	void printSyntax()  const;
	void ground(NS_OUTPUT::Output&) const;
	void printGrounderStats(NS_OUTPUT::Output&) const;
	GringoOptions opts;
protected:
	void addConstStream(Streams& s) const;
	// ---------------------------------------------------------------------------------------
	// AppOptions interface
	void initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden) { 
		opts.initOptions(root, hidden); 
	}
	void addDefaults(std::string& defaults) { 
		opts.addDefaults(defaults);
	}
	bool validateOptions(ProgramOptions::OptionValues& v, Messages& m) { 
		return opts.validateOptions(v, m); 
	}
	// ---------------------------------------------------------------------------------------
	// Application interface
	std::string getVersion() const;
	std::string getUsage()   const;
	ProgramOptions::PosOption getPositionalParser() const;
	void handleSignal(int sig);
	int  doRun();
	// ---------------------------------------------------------------------------------------
};

#if defined(WITH_CLASP)
// (i)Clingo application, i.e. gringo+clasp
class ClingoApp : public GringoApp, public Clasp::ClaspFacade::Callback {
public:
	void printVersion() const;
protected:
	// ---------------------------------------------------------------------------------------
	// AppOptions interface
	void initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden) { 
		config_.solver = &solver_;
		cmdOpts_.setConfig(&config_);
		cmdOpts_.initOptions(root, hidden);
		clingo_.initOptions(root, hidden);
		generic.verbose = 1;
		GringoApp::initOptions(root, hidden);
	}
	void addDefaults(std::string& defaults) { 
		cmdOpts_.addDefaults(defaults);
		clingo_.addDefaults(defaults);
		defaults += "  --verbose=1";
		GringoApp::addDefaults(defaults);
	}
	bool validateOptions(ProgramOptions::OptionValues& v, Messages& m) { 
		if (cmdOpts_.basic.timeout != -1) {
			m.warning.push_back("Time limit not supported in " EXECUTABLE);
		}
		return cmdOpts_.validateOptions(v, m)
			&& GringoApp::validateOptions(v, m)
			&& clingo_.validateOptions(v, GringoApp::opts, m);
	}
	// ---------------------------------------------------------------------------------------
	// Application interface
	std::string getVersion() const;
	std::string getUsage()   const;
	ProgramOptions::PosOption getPositionalParser() const;
	void handleSignal(int sig);
	int  doRun();
	// -------------------------------------------------------------------------------------------
	// ClaspFacade::Callback interface
	void state(Clasp::ClaspFacade::Event e, Clasp::ClaspFacade& f);
	void event(Clasp::ClaspFacade::Event e, Clasp::ClaspFacade& f);
	void warning(const char* msg) { messages.warning.push_back(msg); }
	// -------------------------------------------------------------------------------------------
	enum ReasonEnd { reason_timeout, reason_interrupt, reason_end };
	enum { numStates = Clasp::ClaspFacade::num_states };
	void printResult(ReasonEnd re);
	void configureInOut(Streams& s);
	typedef std::auto_ptr<Clasp::OutputFormat> ClaspOutPtr;
	typedef std::auto_ptr<Clasp::Input> ClaspInPtr;
	Clasp::Solver        solver_;           // solver to use for search
	Clasp::SolveStats    stats_;            // accumulates clasp solve stats in incremental setting
	Clasp::ClaspConfig   config_;           // clasp configuration - from command-line
	Clasp::ClaspOptions  cmdOpts_;          // clasp basic options - from command-line
	ClingoOptions        clingo_;           // (i)clingo options   - from command-line
	Timer                timer_[numStates]; // one timer for each state
	ClaspOutPtr          out_;              // printer for printing result of search
	ClaspInPtr           in_;               // input for clasp
	Clasp::ClaspFacade*  facade_;           // interface to clasp lib
};
#endif

}
#endif
