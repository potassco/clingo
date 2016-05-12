// Copyright (c) 2010, Roland Kaminski <kaminski@cs.uni-potsdam.de>
// Copyright (c) 2009, Benjamin Kaufmann
//
// This file is part of gringo.
//
// gringo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with gringo.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <string>
#include <utility>
#include <program_opts/app_options.h>
#include <gringo/grounder.h>
#include <clasp/clasp_facade.h>
#include <program_opts/value.h>
#include "gringo/gringo_options.h"

enum Mode { CLASP, CLINGO, ICLINGO };

struct iClingoConfig : public Clasp::IncrementalControl
{
	iClingoConfig()
		: minSteps(1)
		, maxSteps(~uint32(0))
		, iQuery(1)
		, stopUnsat(false)
		, keepLearnt(true)
		, keepHeuristic(false)
	{ }
	void initStep(Clasp::ClaspFacade& f);
	bool nextStep(Clasp::ClaspFacade& f);

	uint32 minSteps;      /**< Perform at least minSteps incremental steps */
	uint32 maxSteps;      /**< Perform at most maxSteps incremental steps */
	int    iQuery;
	bool   stopUnsat;     /**< Stop on first unsat problem? */
	bool   keepLearnt;    /**< Keep learnt nogoods between incremental steps? */
	bool   keepHeuristic; /**< Keep heuristic values between incremental steps? */
};

template <Mode M>
struct ClingoOptions
{
	ClingoOptions();
	void initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden);
	bool validateOptions(ProgramOptions::OptionValues& values, GringoOptions& opts, Messages&);
	void addDefaults(std::string& def);

	bool claspMode;  // default: false
	bool clingoMode; // default: true for clingo, false for iclingo
	Mode mode;       // default: highest mode the current binary supports
	bool iStats;     // default: false
	iClingoConfig inc;
};

bool mapStop(const std::string& s, bool&);
bool mapKeepForget(const std::string& s, bool&);

//////////////////////////// ClingoOptions ////////////////////////////////////

template <Mode M>
ClingoOptions<M>::ClingoOptions()
	: claspMode(false)
	, clingoMode(M == CLINGO)
	, mode(M)
	, iStats(false)
{ }

template <Mode M>
void ClingoOptions<M>::initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden)
{
	(void)hidden;
	using namespace ProgramOptions;
	if(M == ICLINGO)
	{
		clingoMode = false;
		OptionGroup incremental("Incremental Computation Options");
		incremental.addOptions()
			("istats"      , bool_switch(&iStats) , "Print statistics for each incremental step\n")

			("imin"        , storeTo(inc.minSteps), "Perform at least <num> incremental solve steps", "<num>")
			("imax"        , storeTo(inc.maxSteps), "Perform at most <num> incremental solve steps\n", "<num>")

			("istop"      , storeTo(inc.stopUnsat)->parser(mapStop),
				"Configure termination condition\n"
				"      Default: SAT\n"
				"      Valid:   SAT, UNSAT\n"
				"        SAT  : Terminate after first satisfiable subproblem\n"
				"        UNSAT: Terminate after first unsatisfiable subproblem\n")

			("iquery"      , storeTo(inc.iQuery)->defaultValue(1),
				"Start solving after <num> step\n"
				"      Default: 1\n", "<num>")

			("ilearnt" , storeTo(inc.keepLearnt)->parser(mapKeepForget),
				"Configure persistence of learnt nogoods\n"
				"      Default: keep\n"
				"      Valid:   keep, forget\n"
				"        keep  : Maintain learnt nogoods between incremental steps\n"
				"        forget: Drop learnt nogoods after every incremental step")
			("iheuristic", storeTo(inc.keepHeuristic)->parser(mapKeepForget),
				"Configure persistence of heuristic information\n"
				"      Default: forget\n"
				"      Valid:   keep, forget\n"
				"        keep  : Maintain heuristic values between incremental steps\n"
				"        forget: Drop heuristic values after every incremental step\n")
		;
		root.addOptions(incremental);
	}
	OptionGroup basic("Basic Options");
	basic.addOptions()("clasp",    bool_switch(&claspMode),  "Run in Clasp mode");
	if(M == ICLINGO)
		basic.addOptions()("clingo",    bool_switch(&clingoMode),  "Run in Clingo mode");
	root.addOptions(basic,true);

}

template <Mode M>
bool ClingoOptions<M>::validateOptions(ProgramOptions::OptionValues& values, GringoOptions& opts, Messages& m)
{
	(void)values;
	(void)opts;
	if(M == ICLINGO)
	{
		if (claspMode && clingoMode)
		{
			m.error = "Options '--clingo' and '--clasp' are mutually exclusive";
			return false;
		}
		if(opts.ibase)
		{
			inc.minSteps = 1;
			inc.maxSteps = 1;
		}
	}
	if(claspMode)       mode = CLASP;
	else if(clingoMode) mode = CLINGO;
	else                mode = ICLINGO;
	return true;
}

template <Mode M>
void ClingoOptions<M>::addDefaults(std::string& def)
{
	if(M == ICLINGO)
	{
		def += "  --istop=SAT --iquery=1 --ilearnt=keep --iheuristic=forget\n";
	}
}

//////////////////////////// iClingoConfig ////////////////////////////////////

void iClingoConfig::initStep(Clasp::ClaspFacade& f)
{
	if (f.step() == 0)
	{
		if (maxSteps == 0)
		{
			f.warning("Max incremental steps must be > 0!");
			maxSteps = 1;
		}
		f.config()->solver->strategies().heuristic->reinit(!keepHeuristic);
	}
	else if (!keepLearnt)
	{
		f.config()->solver->reduceLearnts(1.0f);
	}
}

bool iClingoConfig::nextStep(Clasp::ClaspFacade& f)
{
	using Clasp::ClaspFacade;
	ClaspFacade::Result stopRes = stopUnsat ? ClaspFacade::result_unsat : ClaspFacade::result_sat;
	return --maxSteps && ((minSteps > 0 && --minSteps) || f.result() != stopRes);
}

bool mapStop(const std::string& s, bool& b)
{
	std::string temp = ProgramOptions::toLower(s);
	return (b=true,(temp=="unsat")) || (b=false,(temp=="sat"));
}

bool mapKeepForget(const std::string& s, bool& b)
{
	std::string temp = ProgramOptions::toLower(s);
	return (b=true,(temp == "keep")) || (b=false,(temp == "forget"));
}
