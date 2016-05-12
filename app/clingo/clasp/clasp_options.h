// 
// Copyright (c) 2006-2010, Benjamin Kaufmann
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
#ifndef CLASP_CLASP_OPTIONS_H_INCLUDED
#define CLASP_CLASP_OPTIONS_H_INCLUDED

#ifdef _MSC_VER
#pragma warning (disable : 4200) // nonstandard extension used : zero-sized array
#pragma once
#endif
#include <string>
#include <utility>
#include <program_opts/app_options.h>
#include <clasp/clasp_facade.h>
#include <clasp/solver.h>

namespace Clasp {

/////////////////////////////////////////////////////////////////////////////////////////
// Option groups - Mapping between command-line options and libclasp objects
/////////////////////////////////////////////////////////////////////////////////////////
// Basic mapping functions
bool parseValue(const std::string&s , ApiOptions::ExtRuleMode& i, int);
bool parseValue(const std::string& s, ApiOptions::LoopMode& i, int);
bool parseValue(const std::string& s, HeuristicOptions::LookaheadType& i, int);
// Function for mapping positional options
bool parsePositional(const std::string& s, std::string& out);
// Maps command-line arguments to functions of SolverStrategies
struct SolverStrategiesWrapper {
	SolverStrategiesWrapper() : satPreproDef(true) {}
	static bool mapSaveProg(const std::string& s, SolverStrategiesWrapper& i);
	static bool mapStrengthen(const std::string& s, SolverStrategiesWrapper& i);
	static bool mapContract(const std::string& s, SolverStrategiesWrapper& i);
	static bool mapSatElite(const std::string& s, SolverStrategiesWrapper& i);
	SolverStrategies* opts;
	bool              satPreproDef;
};

// Maps command-line arguments to ClaspSolveOptions
struct SolveOptionsWrapper {
	static bool mapRandFreq(const std::string& s, SolveOptionsWrapper& i);
	static bool mapRandProb(const std::string& s, SolveOptionsWrapper& i);
	static bool mapRestarts(const std::string& s, SolveOptionsWrapper& i);
	static bool mapReduce(const std::string& s, SolveOptionsWrapper& i);
	static bool mapShuffle(const std::string& s, SolveOptionsWrapper& i);
	SolveParams* opts;
};
// Always returns false - use SolverStrategiesWrapper 
bool parseValue(const std::string&, SolveOptionsWrapper&, int);
// Always returns false - use SolveOptionsWrapper 
bool parseValue(const std::string&, SolverStrategiesWrapper&, int);

// Group "Basic Options"
struct BasicOptions {
	BasicOptions();
	void initOptions(ProgramOptions::OptionGroup& root);
	static bool mapStats(const std::string& s, uint8& stats);
	int         timeout;// timeout in seconds (default: none=-1)
	uint8       stats;  // print statistics
	bool        quiet;  // do not print models
	bool        asp09;  // force ASP'09 output format
};

// Group "Clasp - General Options"
// Options of this group are mapped to ClaspConfig::api
// and ClaspConfig::enumerate
struct GeneralOptions {
	GeneralOptions() : config(0) {}
	void initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden);
	bool validateOptions(ProgramOptions::OptionValues& values, Messages&);
	typedef SolverStrategiesWrapper Strategies;
	ClaspConfig* config;
	Strategies   solverOpts;
};

// Groups "Clasp - Search Options" and "Clasp - Lookback Options"
// Options of these groups are mapped to ClaspConfig::solve 
// and ClaspConfig::solver
struct SearchOptions {
	SearchOptions() : config(0) {}
	void initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden);
	bool validateOptions(ProgramOptions::OptionValues& values, Messages&);
	typedef SolverStrategiesWrapper Strategies;
	static bool mapHeuristic(const std::string& s, std::string&);
	ClaspConfig*        config;
	Strategies          solverOpts;
	SolveOptionsWrapper parseSolve;
};

// Combines all groups and drives initialization/validation 
// of command-line options.
class ClaspOptions {
public:
	ClaspOptions() {}
	BasicOptions     basic;
	// Sets the configuration object in which parsed options are stored.
	// Must be called once before option parsing begins
	void setConfig(ClaspConfig* config);

	void initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden);
	bool validateOptions(ProgramOptions::OptionValues& values, Messages&);
	void addDefaults(std::string& def);

	void applyDefaults(Input::Format f);
private:
	GeneralOptions mode;
	SearchOptions  search;
};

}
#endif

