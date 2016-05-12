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
#include "clasp_options.h"
#include "program_opts/value.h"  
#include <clasp/satelite.h>
#include <sstream>
#include <algorithm>

using namespace ProgramOptions;
using namespace std;

namespace Clasp { namespace {

bool mapSeed(const std::string& s, int& i) {
	if (ProgramOptions::parseValue(s, i, 1)) {
		if    (i > 0) { Clasp::srand(i); }
		return i > 0 || i == -1;
	}
	return false;
}
} // end unnamed namespace

/////////////////////////////////////////////////////////////////////////////////////////
// Parseing & Mapping of options
/////////////////////////////////////////////////////////////////////////////////////////
bool parsePositional(const std::string& t, std::string& out) {
	int num;
	if (ProgramOptions::parseValue(t, num, 1)) { out = "number"; }
	else                                       { out = "file";   }
	return true;
}

bool parseValue(const std::string&, SolveOptionsWrapper&, int)      { return false; }
bool parseValue(const std::string&, SolverStrategiesWrapper&, int)  { return false; }
bool parseValue(const std::string&s , ApiOptions::ExtRuleMode& i, int){
	std::string temp = toLower(s);
	bool b = temp == "all";
	if ( b || ProgramOptions::parseValue(s, b, 1)) {
		i = b ? ProgramBuilder::mode_transform : ProgramBuilder::mode_native;
		return true;
	}
	else if (temp == "choice")  { i = ProgramBuilder::mode_transform_choice; return true; }
	else if (temp == "card")    { i = ProgramBuilder::mode_transform_card; return true; }
	else if (temp == "weight")  { i = ProgramBuilder::mode_transform_weight; return true; }
	else if (temp == "integ")   { i = ProgramBuilder::mode_transform_integ; return true; }
	else if (temp == "dynamic") { i = ProgramBuilder::mode_transform_dynamic; return true; }
	return false;
}
bool parseValue(const std::string& s, ApiOptions::LoopMode& i, int) {
	std::string temp = toLower(s);
	bool b = temp == "common";
	if (b || ProgramOptions::parseValue(s, b, 1)) {
		i = b ? DefaultUnfoundedCheck::common_reason : DefaultUnfoundedCheck::only_reason;
		return true;
	}
	else if (temp == "shared")    { i = DefaultUnfoundedCheck::shared_reason; return true; }
	else if (temp == "distinct")  { i = DefaultUnfoundedCheck::distinct_reason; return true; }
	return false;
}
bool parseValue(const std::string& s, HeuristicOptions::LookaheadType& i, int) {
	std::string temp = toLower(s);
	bool b;
	if (ProgramOptions::parseValue(s, b, 1)) {
		i = b ? Lookahead::atom_lookahead : Lookahead::no_lookahead;
		return true;
	}
	else if (temp == "atom")      { i = Lookahead::atom_lookahead; return true; }
	else if (temp == "body")      { i = Lookahead::body_lookahead; return true; }
	else if (temp == "hybrid")    { i = Lookahead::hybrid_lookahead; return true; }
	return false;
}
bool SearchOptions::mapHeuristic(const std::string& s, std::string& out) {
	std::string temp = toLower(s);
	if      (temp == "berkmin")   { out = temp; return true; }
	else if (temp == "vmtf")      { out = temp; return true; }
	else if (temp == "vsids")     { out = temp; return true; }
	else if (temp == "unit")      { out = temp; return true; }
	else if (temp == "none")      { out = temp; return true; }
	return false;
}
bool SolveOptionsWrapper::mapRandFreq(const std::string& s, SolveOptionsWrapper& i) {
	double p;
	if (ProgramOptions::parseValue(s, p, 1)) {
		i.opts->setRandomProbability(p);
		return true;
	}
	return false;
}
bool SolveOptionsWrapper::mapRandProb(const std::string &s, SolveOptionsWrapper& i) {
	bool b = false;
	std::pair<int, int> r(0, 0);
	if (ProgramOptions::parseValue(s, b, 1) || ProgramOptions::parseValue(s, r, 1)) {
		if (b) { r.first = 50; r.second = 20; }
		i.opts->setRandomizeParams(r.first, r.second);
		return true;
	}
	return false;
}
bool SolveOptionsWrapper::mapRestarts(const std::string& s, SolveOptionsWrapper& i) {
	bool b = true, first; std::vector<double> v;
	if ((first=ProgramOptions::parseValue(s, b, 1)) || ProgramOptions::parseValue(s, v, 1)) {
		if (first) {
			if (!b) i.opts->restart.setStrategy(0, 0, 0);
			return true;
		}
		else if (!v.empty() && v.size() <= 3) {
			v.resize(3, 0.0);
			i.opts->restart.setStrategy(uint32(v[0]), v[1], uint32(v[2]));
			return true;
		}
	}
	return false;
}
bool SolveOptionsWrapper::mapReduce(const std::string& s, SolveOptionsWrapper& i) {
	bool b; std::vector<double> v;
	if (ProgramOptions::parseValue(s, b, 1)) {
		i.opts->reduce.disable = !b;
		return true;
	}
	if (ProgramOptions::parseValue(s, v, 1) && v.size() <= 3) {
		if (v.empty())  v.push_back(3.0);
		if (v.size()==1)v.push_back(1.1);
		if (v.size()==2)v.push_back(3.0);
		i.opts->reduce.setStrategy(v[0], v[1], v[2]);
		return true;
	}
	return false;
}
bool SolveOptionsWrapper::mapShuffle(const std::string& s, SolveOptionsWrapper& i) {
	std::pair<int, int> shuf;
	if (ProgramOptions::parseValue(s, shuf, 1)) {
		i.opts->setShuffleParams(shuf.first, shuf.second);
		return true;
	}
	return false;
}
bool SolverStrategiesWrapper::mapSaveProg(const std::string& s, SolverStrategiesWrapper& i) {
	int x;
	if (ProgramOptions::parseValue(s, x, 1) || (s.empty() && (x = 1))) {
		i.opts->saveProgress = x;
		return true;
	}
	return false;
}
bool SolverStrategiesWrapper::mapStrengthen(const std::string& s, SolverStrategiesWrapper& i) {
	std::string temp = toLower(s);
	bool b = temp == "all";
	if (b || ProgramOptions::parseValue(s, b, 1)) {
		i.opts->cflMinAntes = b ? SolverStrategies::all_antes : SolverStrategies::no_antes;
		return true;
	}
	else if (temp == "bin")   { i.opts->cflMinAntes = SolverStrategies::binary_antes; return true; }
	else if (temp == "tern")  { i.opts->cflMinAntes = SolverStrategies::binary_ternary_antes; return true; }
	return false;
}
bool SolverStrategiesWrapper::mapContract(const std::string& s, SolverStrategiesWrapper& i) {
	int x;
	if (ProgramOptions::parseValue(s, x, 1)) {
		i.opts->setCompressionStrategy(x);
		return true;
	}
	return false;
}
bool SolverStrategiesWrapper::mapSatElite(const std::string& s, SolverStrategiesWrapper& i) {
	bool b = true; std::vector<int> v;
	if ( (s != "1" && ProgramOptions::parseValue(s, b, 1)) || ProgramOptions::parseValue(s, v, 1) ) {
		if (b) {
			SatElite::SatElite* pre = new SatElite::SatElite();
			pre->options.maxIters = v.size()>0 ? v[0] : -1;
			pre->options.maxOcc   = v.size()>1 ? v[1] : -1;
			pre->options.maxTime  = v.size()>2 ? v[2] : -1;
			pre->options.maxFrozen= v.size()>3 && v[3] > 0 ? (v[3]/100.0) : 1.0;
			i.opts->satPrePro.reset( pre );
		}
		i.satPreproDef = false;
		return true;
	}
	return false;
}
/////////////////////////////////////////////////////////////////////////////////////////
// Clasp specific basic options
/////////////////////////////////////////////////////////////////////////////////////////
BasicOptions::BasicOptions() : timeout(-1), stats(0), quiet(false), asp09(false) {}
void BasicOptions::initOptions(ProgramOptions::OptionGroup& root) {
	OptionGroup basic("Basic Options");
	basic.addOptions()
		("stats"   , storeTo(stats)->parser(&BasicOptions::mapStats)->setImplicit(),"Print extended statistics")
		("quiet,q", bool_switch(&quiet), "Do not print models")
		("asp09" , bool_switch(&asp09),  "Write output in ASP Competition'09 format")
		("time-limit" , storeTo(timeout), "Set time limit to <n> seconds", "<n>")
	;
	root.addOptions(basic, true);
}
bool BasicOptions::mapStats(const std::string& s, uint8& stats) {
	int parsed = 1;
	if (s.empty() || ProgramOptions::parseValue(s, parsed, 1)) {
		stats = static_cast<uint8>(parsed);
		return true;
	}
	return false;
}
/////////////////////////////////////////////////////////////////////////////////////////
// Clasp specific mode options
/////////////////////////////////////////////////////////////////////////////////////////
void GeneralOptions::initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden) {
	OptionGroup general("Clasp - General Options");
	general.addOptions()
		("number,n", storeTo(config->enumerate.numModels), 
			"Enumerate at most <num> models (0 for all)\n"
			"      Default: 1 (0 when optimizing/computing consequences)", "<num>")
		("seed"    , value<int>()->parser(mapSeed),    "Set random number generator's seed to <num>\n", "<num>")

		("solution-recording", bool_switch(&config->enumerate.record), "Add conflicts for computed models")
		("restart-on-model", bool_switch(&config->enumerate.restartOnModel), "Restart (instead of backtrack) after each model")
		("project", bool_switch(&config->enumerate.project), "Project models to named atoms in enumeration mode\n")
		
		("brave"    , bool_switch(&config->enumerate.brave), "Compute brave consequences")
		("cautious" , bool_switch(&config->enumerate.cautious), "Compute cautious consequences\n")

		("pre" , bool_switch(&config->onlyPre), "Run ASP preprocessor and exit")
		("search-limit", value<std::pair<int, int> >(), "Terminate search after <n> conflicts or <m> restarts\n", "<n,m>")
		
		("opt-all"    , bool_switch(&config->enumerate.opt.all), "Compute all optimal models")
		("opt-ignore" , bool_switch(&config->enumerate.opt.no), "Ignore minimize statements")
		("opt-heu"    , bool_switch(&config->enumerate.opt.heu), "Consider minimize statements in heuristics")
		("opt-value"  , value<vector<int> >(), 
			"Initialize objective function(s)\n"
			"      Valid:   <n1[,n2,n3,...]>\n")
		
		("supp-models",bool_switch(&config->api.supported), "Compute supported models (disable unfounded set check)\n")
		
		("trans-ext", storeTo(config->api.transExt),
			"Configure handling of Lparse-like extended rules\n"
			"      Default: no\n"
			"      Valid:   all, choice, card, weight, integ, dynamic, no\n"
			"        all    : Transform all extended rules to basic rules\n"
			"        choice : Transform choice rules, but keep cardinality and weight rules\n"
			"        card   : Transform cardinality rules, but keep choice and weight rules\n"
			"        weight : Transform cardinality and weight rules, but keep choice rules\n"
			"        integ  : Transform cardinality integrity constraints\n"
			"        dynamic: Transform \"simple\" extended rules, but keep more complex ones\n"
			"        no     : Do not transform extended rules\n")

		("eq", storeTo(config->api.eq), 
			"Configure equivalence preprocessing\n"
			"      Default: 5\n"
			"      Valid:\n"
			"        -1 : Run to fixpoint\n"
			"        0  : Do not run equivalence preprocessing\n"
			"        > 0: Run for at most <n> iterations", "<n>")
		("backprop",bool_switch(&config->api.backprop), "Enable backpropagation in ASP-preprocessing\n")
		
		("sat-prepro", storeTo(solverOpts)->parser(&Strategies::mapSatElite)->setImplicit(),
			"Configure SatELite-like preprocessing\n"
			"      Default: no\n"
			"      Valid:   yes, no, <n1[,n2,n3,n4]>\n"
			"        <n1>: Run for at most <n1> iterations           (-1=run to fixpoint)\n"
			"        <n2>: Run variable elimination with cutoff <n2> (-1=no cutoff)\n"
			"        <n3>: Run for at most <n3> seconds              (-1=no time limit)\n"
			"        <n4>: Disable if <n4>% of vars are frozen       (-1=no time limit)\n"
			"        yes : Run to fixpoint, no cutoff and no time limit\n","<opts>")
	;
	root.addOptions(general);
	hidden.addOptions()
		("project-opt", storeTo(config->enumerate.projectOpts), "Additional options for projection as octal digit\n")
		("dfs-eq", bool_switch(&config->api.eqDfs), "Enable df-order in eq-preprocessing\n")
	;
}
bool GeneralOptions::validateOptions(ProgramOptions::OptionValues& vm, Messages&) {
	if (vm.count("search-limit") != 0) {
		std::pair<int, int> limits = value_cast<std::pair<int, int> >(vm["search-limit"]);
		if (limits.first <= 0) limits.first = -1;
		if (limits.second<= 0) limits.second= -1;
		config->enumerate.limits.reset(new std::pair<int, int>(limits));
	}
	if (vm.count("opt-value") != 0) {
		const std::vector<int>& vals = value_cast<std::vector<int> >(vm["opt-value"]);
		config->enumerate.opt.vals.assign(vals.begin(), vals.end());
	}
	if (config->api.supported && vm.count("eq") == 0) {
		config->api.eq = 0;
	}
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////
// Clasp specific search options
/////////////////////////////////////////////////////////////////////////////////////////
void SearchOptions::initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden) {
	OptionGroup search("Clasp - Search Options");
	search.addOptions()
		("lookahead"  , storeTo(config->heuristic.lookahead)->setImplicit(),
			"Configure failed-literal detection (fld)\n"
			"      Default: no (atom, if --nolookback)\n"
			"      Valid:   atom, body, hybrid, no\n"
			"        atom  : Apply failed-literal detection to atoms\n"
			"        body  : Apply failed-literal detection to bodies\n"
			"        hybrid: Apply Nomore++-like failed-literal detection\n"
			"        no    : Do not apply failed-literal detection", "<arg>")
		("initial-lookahead", storeTo(config->heuristic.lookaheadNum), "Restrict fld to <n> decisions\n", "<n>")

		("heuristic", storeTo(config->heuristic.heuristic)->parser(SearchOptions::mapHeuristic), 
			"Configure decision heuristic\n"
			"      Default: Berkmin (Unit, if --no-lookback)\n"
			"      Valid:   Berkmin, Vmtf, Vsids, Unit, None\n"
			"        Berkmin: Apply BerkMin-like heuristic\n"
			"        Vmtf   : Apply Siege-like heuristic\n"
			"        Vsids  : Apply Chaff-like heuristic\n"
			"        Unit   : Apply Smodels-like heuristic\n"
			"        None   : Select the first free variable")
		("rand-freq", storeTo(parseSolve)->parser(&SolveOptionsWrapper::mapRandFreq), 
			"Make random decisions with probability <p>\n"
			"      Default: 0.0\n"
			"      Valid:   [0.0...1.0]\n", "<p>")

		("rand-prob", storeTo(parseSolve)->parser(&SolveOptionsWrapper::mapRandProb)->setImplicit(),
			"Configure random probing\n"
			"      Default: no\n"
			"      Valid:   yes, no, <n1,n2> (<n1> >= 0, <n2> > 0)\n"
			"        yes    : Run 50 random passes up to at most 20 conflicts each\n"
			"        no     : Do not run random probing\n"
			"        <n1,n2>: Run <n1> random passes up to at most <n2> conflicts each\n", "<opts>")

		("rand-watches", bool_switch()->defaultValue(true),
			"Configure watched literal initialization\n"
			"      Default: yes\n"
			"      Valid:   yes, no\n"
			"        yes: Randomly determine watched literals\n"
			"        no : Watch first and last literal in a nogood\n")
	;
	
	OptionGroup lookback("Clasp - Lookback Options");
	lookback.addOptions()
		("no-lookback"   ,bool_switch(), "Disable all lookback strategies\n")

		("restarts,r", storeTo(parseSolve)->parser(&SolveOptionsWrapper::mapRestarts),
			"Configure restart policy\n"
			"      Default: 100,1.5\n"
			"      Valid:   <n1[,n2,n3]> (<n1> >= 0, <n2>,<n3> > 0), no\n"
			"        <n1>          : Run Luby et al.'s sequence with unit length <n1>\n"
			"        <n1>,<n2>     : Run geometric sequence of <n1>*(<n2>^i) conflicts\n"
			"        <n1>,<n2>,<n3>: Run Biere's inner-outer geometric sequence (<n3>=outer)\n"
			"        <n1> = 0, no  : Disable restarts")
		("local-restarts"  , bool_switch(&config->solve.restart.local), "Enable Ryvchin et al.'s local restarts")
		("bounded-restarts", bool_switch(&config->solve.restart.bounded), "Enable (bounded) restarts during model enumeration")
		("reset-restarts",   bool_switch(&config->solve.restart.resetOnModel), "Reset restart strategy during model enumeration")
		("save-progress"   , storeTo(solverOpts)->setImplicit()->parser(&SolverStrategiesWrapper::mapSaveProg), "Enable RSat-like progress saving on backjumps > <n>\n", "<n>")

		("shuffle,s", storeTo(parseSolve)->parser(&SolveOptionsWrapper::mapShuffle),
			"Configure shuffling after restarts\n"
			"      Default: 0,0\n"
			"      Valid:   <n1,n2> (<n1> >= 0, <n2> >= 0)\n"
			"        <n1> > 0: Shuffle problem after <n1> and re-shuffle every <n2> restarts\n"
			"        <n1> = 0: Do not shuffle problem after restarts\n"
			"        <n2> = 0: Do not re-shuffle problem\n", "<n1,n2>")

		("deletion,d", storeTo(parseSolve)->parser(&SolveOptionsWrapper::mapReduce), 
			"Configure size of learnt nogood database\n"
			"      Default: 3.0,1.1,3.0\n"
			"      Valid:   <n1[,n2,n3]> (<n3> >= <n1> >= 0, <n2> >= 1.0), no\n"
			"        <n1,n2,n3>: Store at most min(P/<n1>*(<n2>^i),P*<n3>) learnt nogoods,\n"
			"                    P and i being initial problem size and number of restarts\n"
			"        no        : Do not delete learnt nogoods")
		("reduce-on-restart", bool_switch(&config->solve.reduce.reduceOnRestart), "Delete some learnt nogoods after every restart\n")
		("estimate", bool_switch(&config->solve.reduce.estimate), "Use estimated problem complexity to init learnt db\n")

		("strengthen", storeTo(solverOpts)->parser(&SolverStrategiesWrapper::mapStrengthen),
			"Configure conflict nogood strengthening\n"
			"      Default: all\n"
			"      Valid:   bin, tern, all, no\n"
			"        bin : Check only binary antecedents for self-subsumption\n"
			"        tern: Check binary and ternary antecedents for self-subsumption\n"
			"        all : Check all antecedents for self-subsumption\n"
			"        no  : Do not check antecedents for self-subsumption")
		("recursive-str", bool_switch(&solverOpts.opts->strengthenRecursive), "Enable MiniSAT-like conflict nogood strengthening\n")

		("loops", storeTo(config->api.loopRep),
			"Configure representation and learning of loop formulas\n"
			"      Default: common\n"
			"      Valid:   common, distinct, shared, no\n"
			"        common  : Create loop nogoods for atoms in an unfounded set\n"
			"        distinct: Create distinct loop nogood for each atom in an unfounded set\n"
			"        shared  : Create loop formula for a whole unfounded set\n"
			"        no      : Do not learn loop formulas\n")

		("contraction", storeTo(solverOpts)->parser(&SolverStrategiesWrapper::mapContract),
			"Configure (temporary) contraction of learnt nogoods\n"
			"      Default: 250\n"
			"      Valid:\n"
			"        0  : Do not contract learnt nogoods\n"
			"        > 0: Contract learnt nogoods containing more than <num> literals\n", "<num>")
	;

	hidden.addOptions()
		("loops-in-heu", storeTo(config->heuristic.loops), "Consider loop nogoods in heuristic")
		("berk-max", storeTo(config->heuristic.extra.berkMax), "Consider at most <n> nogoods in Berkmin")
		("berk-moms", bool_switch(&config->heuristic.berkMoms), "Enable/Disable MOMs in Berkmin")
		("berk-huang",bool_switch(&config->heuristic.berkHuang), "Enable/Disable Huang-scoring in Berkmin")
		("vmtf-mtf",storeTo(config->heuristic.extra.vmtfMtf), "In Vmtf move up to <n> conflict-literals to the front")
		("nant",bool_switch(&config->heuristic.nant), "In Unit count only atoms in NAnt(P)")
	;
	root.addOptions(search);
	root.addOptions(lookback);
}

bool SearchOptions::validateOptions(ProgramOptions::OptionValues& vm, Messages& m) {
	if (vm.count("no-lookback") != 0 && value_cast<bool>(vm["no-lookback"])) {
		solverOpts.opts->search = Clasp::SolverStrategies::no_learning;
		if (vm.count("heuristic") == 0) { config->heuristic.heuristic = "unit"; }
		if (vm.count("lookahead") == 0) { config->heuristic.lookahead = Lookahead::atom_lookahead; }
		bool warn = config->solve.restart.local || config->solve.restart.bounded || config->solve.reduce.reduceOnRestart;
		if (warn || vm.count("restarts") || vm.count("deletion") || vm.count("rand-prob") || vm.count("shuffle")) {
			m.warning.push_back("lookback-options ignored because lookback strategy is not used!");     
		}
	}
	solverOpts.opts->randomWatches = vm.count("rand-watches") > 0 && value_cast<bool>(vm["rand-watches"]);
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// clasp option validation
/////////////////////////////////////////////////////////////////////////////////////////
void ClaspOptions::setConfig(ClaspConfig* config) {
	assert(config && "clasp options: config must not be 0!\n");
	assert(config->solver && "clasp options: solver must not be 0\n");
	mode.config   = config;
	search.config = config;
	mode.solverOpts.opts   = &config->solver->strategies();
	search.solverOpts.opts = &config->solver->strategies();
	search.parseSolve.opts = &config->solve;
}
void ClaspOptions::initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden) {
	if (mode.config == 0) {
		throw std::runtime_error("clasp options: config not set!");
	}
	if (mode.config->solver == 0) {
		throw std::runtime_error("clasp options: solver not set!");
	}
	mode.initOptions(root, hidden);
	search.initOptions(root, hidden);
	basic.initOptions(root);

}

bool ClaspOptions::validateOptions(ProgramOptions::OptionValues& vm, Messages& m) {
	if (mode.validateOptions(vm, m) && search.validateOptions(vm, m)) {
		return mode.config->validate(m.error);
	}
	return false;
}

void ClaspOptions::addDefaults(std::string& def) {
	def += "1 --trans-ext=no --eq=5 --sat-prepro=no --rand-watches=yes\n";
  def += "  --lookahead=no --heuristic=Berkmin --rand-freq=0.0 --rand-prob=no\n";
	def += "  --restarts=100,1.5 --shuffle=0,0 --deletion=3.0,1.1,3.0\n";
	def += "  --strengthen=all --loops=common --contraction=250\n";
}

void ClaspOptions::applyDefaults(Input::Format f) {
	if (f != Input::SMODELS && mode.solverOpts.satPreproDef) {
		SolverStrategiesWrapper::mapSatElite("20,25,120", mode.solverOpts);
	}
}

}
