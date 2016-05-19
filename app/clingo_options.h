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
#ifndef GRINGO_CLINGO_OPTIONS_H_INCLUDED
#define GRINGO_CLINGO_OPTIONS_H_INCLUDED

#ifdef _MSC_VER
#pragma warning (disable : 4200) // nonstandard extension used : zero-sized array
#pragma once
#endif

#include <string>
#include <utility>
#include <program_opts/app_options.h>
#include <gringo/grounder.h>
#include <clasp/clasp_facade.h>
namespace gringo {
struct GringoOptions;

/////////////////////////////////////////////////////////////////////////////////////////
// Option groups - Mapping between command-line options and gringo options
/////////////////////////////////////////////////////////////////////////////////////////
struct iClingoConfig : public Clasp::IncrementalControl {
	iClingoConfig() : minSteps(1), maxSteps(uint32(-1)), stopUnsat(false), keepLearnt(true), keepHeuristic(false) {}
	uint32 minSteps;      /**< Perform at least minSteps incremental steps */
	uint32 maxSteps;      /**< Perform at most maxSteps incremental steps */
	bool   stopUnsat;     /**< Stop on first unsat problem? */
	bool   keepLearnt;    /**< Keep learnt nogoods between incremental steps? */
	bool   keepHeuristic; /**< Keep heuristic values between incremental steps? */
	void initStep(Clasp::ClaspFacade& f);
	bool nextStep(Clasp::ClaspFacade& f);
};

bool mapStop(const std::string& s, bool&);
bool mapKeepForget(const std::string& s, bool&);
struct ClingoOptions {
	ClingoOptions();
	void initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden);
	bool validateOptions(ProgramOptions::OptionValues& values, GringoOptions& opts, Messages&);
	void addDefaults(std::string& def);
	bool claspMode; // default: false
	bool clingoMode;// default: true for clingo, false for iclingo
	bool iStats;
	iClingoConfig inc;
};

}
#endif
