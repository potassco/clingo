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
#include "clingo_options.h"
#include "gringo_options.h"
#include <program_opts/value.h>
namespace gringo {

ClingoOptions::ClingoOptions()
	: claspMode(false)
	, clingoMode(true)
	, iStats(false) {
	inc.keepHeuristic = false;
	inc.keepLearnt    = true;
	inc.minSteps      = 1;
	inc.maxSteps      = ~uint32(0);
	inc.stopUnsat     = false;
}

void ClingoOptions::initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden) {
	using namespace ProgramOptions;
#if defined(WITH_ICLASP)
	clingoMode = false;
	OptionGroup incremental("Incremental Computation Options");
	incremental.addOptions()
		("istats"      , bool_switch(&iStats) , "Print statistics for each incremental step\n")

		("imin"        , storeTo(inc.minSteps), "Perform at least <num> incremental steps", "<num>")
		("imax"        , storeTo(inc.maxSteps), "Perform at most <num> incremental steps\n", "<num>")

		("istop"      , storeTo(inc.stopUnsat)->parser(mapStop),
			"Configure termination condition\n"
			"      Default: SAT\n"
			"      Valid:   SAT, UNSAT\n"
			"        SAT  : Terminate after first satisfiable subproblem\n"
			"        UNSAT: Terminate after first unsatisfiable subproblem\n")

		("iquery"      , value<int>()->defaultValue(1),
			"Start solving at step <num>\n"
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
#endif
	OptionGroup basic("Basic Options");
	basic.addOptions()
		("clasp",    bool_switch(&claspMode),  "Run in Clasp mode")
#if defined(WITH_ICLASP)
		("clingo",    bool_switch(&clingoMode),  "Run in Clingo mode")
#endif
	;
	root.addOptions(basic,true);
}

bool ClingoOptions::validateOptions(ProgramOptions::OptionValues& values, GringoOptions& opts, Messages& m) {
#if defined(WITH_ICLASP)
	if (claspMode && clingoMode) {
		m.error = "Options '--clingo' and '--clasp' are mutually exclusive";
		return false;
	}
	opts.grounderOptions.iquery = ProgramOptions::value_cast<int>(values["iquery"]);
	if (opts.grounderOptions.ibase) {
		opts.grounderOptions.iquery = 1;
		opts.grounderOptions.ifixed = -1;
		clingoMode = true;
	}
	if (opts.grounderOptions.iquery < 0) {
		opts.grounderOptions.iquery = 0;
	}
	if(opts.grounderOptions.ifixed >= 0 && !clingoMode && !claspMode && !opts.onlyGround) {
		opts.grounderOptions.ifixed = -1;
		m.warning.push_back("Option ifixed will be ignored!");
	}
#endif
	return true;
}

void ClingoOptions::addDefaults(std::string& def) {
#if defined(WITH_ICLASP)
	def += "  --istop=SAT --iquery=1 --ilearnt=keep --iheuristic=forget\n";
#endif
}

bool mapStop(const std::string& s, bool& b) {
	std::string temp = ProgramOptions::toLower(s);
	return (b=true,(temp=="unsat")) || (b=false,(temp=="sat"));
}

bool mapKeepForget(const std::string& s, bool& b) {
	std::string temp = ProgramOptions::toLower(s);
	return (b=true,(temp == "keep")) || (b=false,(temp == "forget"));
}

void iClingoConfig::initStep(Clasp::ClaspFacade& f) {
	if (f.step() == 0) {
		if (maxSteps == 0) {
			f.warning("Max incremental steps must be > 0!"); 
			maxSteps = 1;
		}
		f.config()->solver->strategies().heuristic->reinit(!keepHeuristic);
	}
	else if (!keepLearnt) {
		f.config()->solver->reduceLearnts(1.0f);
	}
}

bool iClingoConfig::nextStep(Clasp::ClaspFacade& f) {
	using Clasp::ClaspFacade;
	ClaspFacade::Result stopRes = stopUnsat ? ClaspFacade::result_unsat : ClaspFacade::result_sat;
	return --maxSteps && ((minSteps > 0 && --minSteps) || f.result() != stopRes);
}


}
