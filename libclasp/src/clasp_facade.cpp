// 
// Copyright (c) 2006-2009, Benjamin Kaufmann
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
#include <clasp/clasp_facade.h>
#include <clasp/model_enumerators.h>
#include <clasp/cb_enumerator.h>
#include <clasp/smodels_constraints.h>
namespace Clasp {

ApiOptions::ApiOptions()
	: transExt(ProgramBuilder::mode_native)
	, loopRep(DefaultUnfoundedCheck::common_reason)
	, eq(5)
	, backprop(false)
	, eqDfs(false)
	, supported(false) {
}

ProgramBuilder* ApiOptions::createApi(AtomIndex& index) {
	ProgramBuilder* api = new ProgramBuilder();
	api->setExtendedRuleMode(transExt);
	api->setEqOptions((uint32)eq, eqDfs);
	api->startProgram(index
		, supported ? 0 : new DefaultUnfoundedCheck(loopRep));
	return api;
}

EnumerateOptions::EnumerateOptions() 
	: numModels(-1)
	, projectOpts(7)
	, project(false)
	, record(false)
	, restartOnModel(false)
	, brave(false)
	, cautious(false) {
}

Enumerator* EnumerateOptions::createEnumerator() const {
	ModelEnumerator* e = 0;
	Enumerator* ret    = 0;
	if (consequences()) {
		ret = new CBConsequences(brave ? CBConsequences::brave_consequences : CBConsequences::cautious_consequences);
	}
	else if (record) {
		ret = (e = new RecordEnumerator());
	}
	else {
		ret = (e = new BacktrackEnumerator(projectOpts));
	}
	if (e) { e->setEnableProjection(project); }
	ret->setRestartOnModel(restartOnModel);
	return ret;
}

HeuristicOptions::HeuristicOptions() 
	: heuristic("berkmin")
	, lookahead(Lookahead::no_lookahead)
	, lookaheadNum(-1)
	, loops(-1)
	, berkMoms(true)
	, berkHuang(false) {
		extra.berkMax = -1;
}

DecisionHeuristic* HeuristicOptions::createHeuristic() const {
	DecisionHeuristic* heu = 0;
	if (heuristic == "berkmin") {
		bool   l = loops == -1 || loops == 1;
		uint32 m = extra.berkMax < 0 ? 0 : extra.berkMax;
		heu = new ClaspBerkmin(m, l, berkMoms, berkHuang);
	}
	else if (heuristic == "vmtf") {
		uint32 m = extra.vmtfMtf < 0 ? 8 : extra.vmtfMtf;
		heu = new ClaspVmtf( m, loops == 1);
	}
	else if (heuristic == "vsids") {
		heu = new ClaspVsids(loops == 1);
	}
	else if (heuristic == "none") {
		heu = new SelectFirst();
	}
	if (lookahead != Lookahead::no_lookahead || lookaheadNum != -1) {
		return new UnitHeuristic(lookahead, nant, heu, lookaheadNum);
	}
	return heu;
}

bool ClaspConfig::validate(std::string& err) {
	if (!solver) {
		err = "Solver not set!";
		return false;
	}
	if (enumerate.brave && enumerate.cautious) {
		err = "Options 'brave' and 'cautious' are mutually exclusive!";
		return false;
	}
	if (enumerate.restartOnModel) { enumerate.record  = true; }
	if (solver && solver->strategies().search == SolverStrategies::no_learning) {
		if (heuristic.heuristic != "unit" && heuristic.heuristic != "none") {
			err = "Selected heuristic requires lookback strategy!";
			return false;
		}
		SolverStrategies* s = &solver->strategies();
		s->cflMinAntes = SolverStrategies::no_antes;
		s->setCompressionStrategy(0);
		s->saveProgress = 0;
		solve.restart.setStrategy(0, 0, 0);
		solve.reduce.setStrategy(-1.0, 0.0, 0.0);
		solve.setRandomizeParams(0,0);
		solve.setShuffleParams(0,0);
		solve.restart.local = solve.restart.bounded = solve.reduce.reduceOnRestart = false;
	}
	return true;
}

IncrementalControl::IncrementalControl() {}
IncrementalControl::~IncrementalControl(){}
/////////////////////////////////////////////////////////////////////////////////////////
// ClaspFacade
/////////////////////////////////////////////////////////////////////////////////////////
ClaspFacade::ClaspFacade() 
	: config_(0)
	, inc_(0)
	, cb_(0)
	, input_(0)
	, api_(0)
	, result_(result_unknown)
	, state_(state_not_started)
	, step_(0)
	, more_(true) {
}

void ClaspFacade::init(Input& problem, ClaspConfig& config, IncrementalControl* inc, Callback* c) {
	assert(config.solver && "ClaspFacade: solver not set!\n");
	config_       = &config;
	inc_          = inc;
	cb_           = c;
	input_        = &problem;
	result_       = result_unknown;
	state_        = state_not_started;
	step_         = 0;
	more_         = true;
	validateWeak();
	config.solve.setEnumerator( config.enumerate.createEnumerator() );
	config.solve.enumerator()->setReport(this);
	config.solver->strategies().heuristic.reset(config.heuristic.createHeuristic());
	if (config.enumerate.limits.get()) {
		if (!inc) {
			config.solve.enumerator()->setSearchLimit(config.enumerate.limits->first, config.enumerate.limits->second);
		}
		else {
			warning("Option 'search-limit' is ignored in incremental setting!");
		}
	}
}

void ClaspFacade::validateWeak() {
	if (inc_) {
		if (config_->onlyPre) {
			warning("Option 'onlyPre' is ignored in incremental setting!"); 
			config_->onlyPre = false;
		}
	}
	if (config_->api.supported && config_->api.eq != 0) {
		warning("Supported models requires --eq=0. Disabling eq-preprocessor!");
		config_->api.eq = 0;
	}
	if (config_->heuristic.heuristic == "unit") {
		if (config_->heuristic.lookahead == Lookahead::no_lookahead) {
			warning("Unit-heuristic implies lookahead. Forcing atom-lookahead!");
			config_->heuristic.lookahead = Lookahead::atom_lookahead;
		}
		else if (config_->heuristic.lookaheadNum != -1) {
			warning("Unit-heuristic implies lookahead. Ignoring 'initial-lookahead'!");
			config_->heuristic.lookaheadNum = -1;
		}
	}
	else if (config_->heuristic.lookaheadNum != -1 && config_->heuristic.lookahead == Lookahead::no_lookahead) {
		config_->heuristic.lookahead = Lookahead::atom_lookahead;
	}
}

// Non-incremental solving...
void ClaspFacade::solve(Input& problem, ClaspConfig& config, Callback* c) {
	init(problem, config, 0, c);
	if (!read() || !preprocess()) {
		result_ = result_unsat;
		more_   = false;
		reportSolution(*config.solver, *config.solve.enumerator(), true);
	}
	else if (!config.onlyPre) {
		config_->solve.reduce.setProblemSize(computeProblemSize());
		setState(state_solve, event_state_enter);
		more_ = Clasp::solve(*config.solver, config.solve); 
	}	
}

// Incremental solving...
void ClaspFacade::solveIncremental(Input& problem, ClaspConfig& config, IncrementalControl& inc, Callback* c) {
	init(problem, config, &inc, c);
	LitVec assume;
	do {
		inc.initStep(*this);
		result_   = result_unknown;
		more_     = true;
		if (!read() || !preprocess()) {
			result_ = result_unsat;
			more_   = false;
			reportSolution(*config.solver, *config.solve.enumerator(), true);
			break;
		}
		else {
			config_->solve.reduce.setProblemSize(computeProblemSize());
			setState(state_solve, event_state_enter);
			assume.clear();
			problem.getAssumptions(assume);
			more_    = Clasp::solve(*config.solver, assume, config.solve); 
			if (result_ == result_unknown && !more_) {
				// initial assumptions are unsat
				result_ = result_unsat;
				setState(state_solve, event_state_exit);
			}
		}
	} while (inc.nextStep(*this) && ++step_);
}

// Creates a ProgramBuilder-object if necessary and reads
// the input by calling input_->read().
// Returns false, if the problem is trivially UNSAT.
bool ClaspFacade::read() {
	setState(state_read, event_state_enter);
	if (input_->format() == Input::SMODELS) {
		if (step_ == 0) {
			config_->solver->strategies().symTab.reset(new AtomIndex());
			api_ = config_->api.createApi(*config_->solver->strategies().symTab);
		}
		if (inc_) {
			api_->updateProgram();
		}
	}
	if (input_->format() == Input::DIMACS && config_->enumerate.numModels == -1) {
		config_->enumerate.numModels = 1;
	}
	if (!input_->read(*config_->solver, api_.get(), config_->enumerate.numModels)) {
		return false;
	}
	setState(state_read, event_state_exit);
	return true;
}

// Prepare the solving state:
//  - if necessary, transforms the input to nogoods by calling ProgramBuilder::endProgram()
//  - fires event_p_prepared after input was transformed to nogoods
//  - adds any minimize statements to the solver and initializes the enumerator
//  - calls Solver::endAddConstraints().
// Returns false, if the problem is trivially UNSAT.
bool ClaspFacade::preprocess() {
	setState(state_preprocess, event_state_enter);
	MinimizeConstraint* m = 0;
	if (api_.get()) {
		if (!api_->endProgram(*config_->solver, false, config_->api.backprop)) {
			return false;
		}
	}
	if (!config_->enumerate.opt.no && step_ == 0) {
		m = input_->getMinimize(*config_->solver, api_.get(), config_->enumerate.opt.heu);
	}
	fireEvent(event_p_prepared);
	if (!inc_ && api_.is_owner()) {
		api_ = 0;
	}
	if (!initEnumerator(m) || !config_->solver->endAddConstraints()) {
		return false;
	}
	setState(state_preprocess, event_state_exit);
	return true;
}

// Configures the given minimize constraint and adds it to the enumerator.
// Optimize values that are given in config are added to min.
bool ClaspFacade::configureMinimize(MinimizeConstraint* min) const {
	min->setMode( config_->enumerate.opt.all ? MinimizeConstraint::compare_less_equal : MinimizeConstraint::compare_less );
	if (!config_->enumerate.opt.vals.empty()) {
		WeightVec& vals = config_->enumerate.opt.vals;
		uint32 m = std::min((uint32)vals.size(), min->numRules());
		for (uint32 x = 0; x != m; ++x) {
			if (!min->setOptimum(x, vals[x])) return false;
		}
	}
	min->simplify(*config_->solver, false);
	config_->solve.enumerator()->setMinimize(min);
	if (config_->enumerate.consequences()) {
		warning("Minimize statements: Consequences may depend on enumeration order!");
	}
	return true;
}

// Finalizes the initialization of the enumerator.
// Sets the number of models to compute and adds warnings
// if this number violates the preferred number of the enumerator.
bool ClaspFacade::initEnumerator(MinimizeConstraint* min) const {
	Enumerator* e = config_->solve.enumerator();
	if (step_ == 0) {
		if (min && !configureMinimize(min)) {
			return false;
		}
		uint32 defM = !e->minimize() && !config_->enumerate.consequences();
		if (config_->enumerate.numModels == -1) { 
			config_->enumerate.numModels = defM; 
		}
		else if (config_->enumerate.numModels > 0 && defM == 0) {
			if (config_->enumerate.consequences()) {
				warning("Option '--number' not 0: last model may not cover consequences!");
			}
			if (e->minimize()) {
				warning("Option '--number' not 0: Optimality of last model not guaranteed!");
			}
		}
	}
	e->init(*config_->solver, config_->enumerate.numModels);
	if (config_->enumerate.project && min) {
		const Solver& s = *config_->solver;
		for (uint32 i = 0; i != min->numRules();) {
			const MinimizeConstraint::Rule& r = min->minRule(i++);
			for (MinimizeConstraint::Rule::const_iterator it = r.begin(); it != r.end(); ++it) {
				if (s.value(it->first.var()) == value_free && !s.project(it->first.var())) {
					warning("Projection: Optimization values may depend on enumeration order!");
					i = min->numRules();
					break;
				}
			}
		}
	}
	return true;
}

// Computes a value that represents the problem size.
// The value is then used by the reduce-heuristic
// to determine the initial learnt db size.
uint32 ClaspFacade::computeProblemSize() const {
	const Solver& s = *config_->solver;
	uint32 ps;
	if (config_->solve.reduce.estimate) {
		ps = config_->solver->problemComplexity();
	}
	else if (input_->format() != Input::DIMACS) {
		double r = s.numVars() / std::max(1.0, double(s.numConstraints()));
		if (r < 0.1 || r > 10.0) {
			ps = std::max(s.numVars(), s.numConstraints());
		}
		else {
			ps = std::min(s.numVars(), s.numConstraints());
		}
	}
	else {
		ps = s.numConstraints();
	}
	return ps;
}
}
