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
#ifndef CLASP_CLASP_APP_H_INCLUDED
#define CLASP_CLASP_APP_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif
#include <program_opts/typed_value.h>
#include <program_opts/application.h>
#include <clasp/util/timer.h>
#include <clasp/cli/clasp_options.h>
#include <clasp/cli/clasp_output.h>
#include <string>
#include <vector>
#include <iosfwd>
#include <memory>
namespace Clasp { namespace Cli {
/////////////////////////////////////////////////////////////////////////////////////////
// clasp exit codes
/////////////////////////////////////////////////////////////////////////////////////////
enum ExitCode {
	E_UNKNOWN   = 0,  /*!< Satisfiablity of problem not knwon; search not started.    */
	E_INTERRUPT = 1,  /*!< Run was interrupted.                                       */
	E_SAT       = 10, /*!< At least one model was found.                              */
	E_EXHAUST   = 20, /*!< Search-space was completely examined.                      */
	E_MEMORY    = 33, /*!< Run was interrupted by out of memory exception.            */
	E_ERROR     = 65, /*!< Run was interrupted by internal error.                     */
	E_NO_RUN    = 128 /*!< Search not started because of syntax or command line error.*/
};
/////////////////////////////////////////////////////////////////////////////////////////
// clasp app helpers
/////////////////////////////////////////////////////////////////////////////////////////
class WriteCnf {
public:
	WriteCnf(const std::string& outFile);
	~WriteCnf();
	void writeHeader(uint32 numVars, uint32 numCons);
	void write(Var maxVar, const ShortImplicationsGraph& g);
	void write(ClauseHead* h);
	void write(Literal unit);
	void close();
	bool unary(Literal, Literal) const;
	bool binary(Literal, Literal, Literal) const;
private:
	WriteCnf(const WriteCnf&);
	WriteCnf& operator=(const WriteCnf&);
	FILE*  str_;
	LitVec lits_;
};
class LemmaLogger {
public:
	struct Options {
		Options() : lbdMax(UINT32_MAX), domOut(false), logText(false) {}
		uint32 lbdMax;  // only log lemmas with lbd <= lbdMax
		bool   domOut;  // only log lemmas that can be expressed over out variables
		bool   logText; // log lemmas in ground lp format
	};
	LemmaLogger(const std::string& outFile, const Options& opts);
	~LemmaLogger();
	void startStep(ProgramBuilder& prg, bool inc);
	void add(const Solver& s, const LitVec& cc, const ConstraintInfo& info);
	void close();
private:
	typedef PodVector<uint32>::type Var2Idx;
	LemmaLogger(const LemmaLogger&);
	LemmaLogger& operator=(const LemmaLogger&);
	void formatAspif(const LitVec& cc, uint32 lbd, std::string& out)  const;
	void formatText(const LitVec& cc, const OutputTable& tab, uint32 lbd, std::string& out) const;
	FILE*            str_;
	Potassco::LitVec solver2asp_;
	Var2Idx          solver2NameIdx_;
	ProblemType      inputType_;
	Options          options_;
	int              step_;
};
/////////////////////////////////////////////////////////////////////////////////////////
// clasp specific application options
/////////////////////////////////////////////////////////////////////////////////////////
struct ClaspAppOptions {
	typedef LemmaLogger::Options LogOptions;
	ClaspAppOptions();
	typedef std::vector<std::string>  StringSeq;
	static bool mappedOpts(ClaspAppOptions*, const std::string&, const std::string&);
	void initOptions(ProgramOptions::OptionContext& root);
	bool validateOptions(const ProgramOptions::ParsedOptions& parsed);
	StringSeq   input;     // list of input files - only first used!
	std::string lemmaLog;  // optional file name for writing learnt lemmas
	std::string lemmaIn;   // optional file name for reading learnt lemmas
	std::string hccOut;    // optional file name for writing scc programs
	std::string outAtom;   // optional format string for atoms
	uint32      outf;      // output format
	int         compute;   // force literal compute to true
	LogOptions  lemma;     // options for lemma logging
	char        ifs;       // output field separator
	bool        hideAux;   // output aux atoms?
	uint8       quiet[3];  // configure printing of models, optimization values, and call steps
	bool        onlyPre;   // run preprocessor and exit
	bool        printPort; // print portfolio and exit
	enum OutputFormat { out_def = 0, out_comp = 1, out_json = 2, out_none = 3 };
};
/////////////////////////////////////////////////////////////////////////////////////////
// clasp application base
/////////////////////////////////////////////////////////////////////////////////////////
// Base class for applications using the clasp library.
class ClaspAppBase : public ProgramOptions::Application, public Clasp::EventHandler {
public:
	typedef ClaspFacade::Summary  RunSummary;
	typedef ProgramOptions::PosOption PosOption;
protected:
	using ProgramOptions::Application::run;
	ClaspAppBase();
	~ClaspAppBase();
	// -------------------------------------------------------------------------------------------
	// Functions to be implemented by subclasses
	virtual ProblemType   getProblemType()             = 0;
	virtual void          run(ClaspFacade& clasp)      = 0;
	virtual Output*       createOutput(ProblemType f);
	virtual void          storeCommandArgs(const ProgramOptions::ParsedValues& values);
	// -------------------------------------------------------------------------------------------
	// Helper functions that subclasses might call during run
	void handleStartOptions(ClaspFacade& clasp);
	bool handlePostGroundOptions(ProgramBuilder& prg);
	bool handlePreSolveOptions(ClaspFacade& clasp);
	// -------------------------------------------------------------------------------------------
	// Application functions
	virtual const int*  getSignals()    const;
	virtual HelpOpt     getHelpOption() const { return HelpOpt("Print {1=basic|2=more|3=full} help and exit", 3); }
	virtual PosOption   getPositional() const { return parsePositional; }
	virtual void        initOptions(ProgramOptions::OptionContext& root);
	virtual void        validateOptions(const ProgramOptions::OptionContext& root, const ProgramOptions::ParsedOptions& parsed, const ProgramOptions::ParsedValues& values);
	virtual void        setup();
	virtual void        run();
	virtual void        shutdown();
	virtual bool        onSignal(int);
	virtual void        printHelp(const ProgramOptions::OptionContext& root);
	virtual void        printVersion();
	static  bool        parsePositional(const std::string& s, std::string& out);
	// -------------------------------------------------------------------------------------------
	// Event handler
	virtual void onEvent(const Event& ev);
	virtual bool onModel(const Solver& s, const Model& m);
	virtual bool onUnsat(const Solver& s, const Model& m);
	// -------------------------------------------------------------------------------------------
	// Status information & output
	int  exitCode(const RunSummary& sol)    const;
	void printTemplate()                    const;
	void printDefaultConfigs()              const;
	void printLibClaspVersion()             const;
	std::istream& getStream();
	// -------------------------------------------------------------------------------------------  
	// Functions called in handlePreSolveOptions()
	void writeNonHcfs(const PrgDepGraph& graph) const;
	typedef Potassco::ProgramReader     LemmaReader;
	typedef SingleOwnerPtr<Output>      OutPtr;
	typedef SingleOwnerPtr<ClaspFacade> ClaspPtr;
	typedef SingleOwnerPtr<LemmaLogger> LogPtr;
	typedef SingleOwnerPtr<LemmaReader> LemmaPtr;
	ClaspCliConfig  claspConfig_;
	ClaspAppOptions claspAppOpts_;
	ClaspPtr        clasp_;
	OutPtr          out_;
	LogPtr          logger_;
	LemmaPtr        lemmaIn_;
};
/////////////////////////////////////////////////////////////////////////////////////////
// clasp application
/////////////////////////////////////////////////////////////////////////////////////////
// Standalone clasp application.
class ClaspApp : public ClaspAppBase {
public:
	ClaspApp();
	const char* getName()       const { return "clasp"; }
	const char* getVersion()    const { return CLASP_VERSION; }
	const char* getUsage()      const { 
		return 
			"[number] [options] [file]\n"
			"Compute at most <number> models (0=all) of the instance given in <file>"; 
	}
protected:
	virtual ProblemType getProblemType();
	virtual void        run(ClaspFacade& clasp);
	virtual void        printHelp(const ProgramOptions::OptionContext& root);
private:
	ClaspApp(const ClaspApp&);
	ClaspApp& operator=(const ClaspApp&);
};
}}
#endif
