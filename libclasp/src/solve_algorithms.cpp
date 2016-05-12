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
#include <clasp/solve_algorithms.h>
#include <clasp/solver.h>
#include <clasp/enumerator.h>
#include <clasp/minimize_constraint.h>
#include <clasp/util/timer.h>
#include <clasp/util/atomic.h>
namespace Clasp { 
/////////////////////////////////////////////////////////////////////////////////////////
// Basic solve
/////////////////////////////////////////////////////////////////////////////////////////
struct BasicSolve::State {
	typedef BasicSolveEvent EventType;
	State(Solver& s, const SolveParams& p);
	ValueRep solve(Solver& s, const SolveParams& p, SolveLimits* lim);
	uint64           dbGrowNext;
	double           dbMax;
	double           dbHigh;
	ScheduleStrategy dbRed;
	uint32           nRestart;
	uint32           nGrow;
	uint32           dbRedInit;
	uint32           dbPinned;
	uint32           rsShuffle;
};

BasicSolve::BasicSolve(Solver& s, SolveLimits* lim) : solver_(&s), params_(&s.searchConfig()), limits_(lim), state_(0) {}
BasicSolve::BasicSolve(Solver& s, const SolveParams& p, SolveLimits* lim) 
	: solver_(&s)
	, params_(&p)
	, limits_(lim)
	, state_(0) {
}

BasicSolve::~BasicSolve(){ delete state_; }
void BasicSolve::reset(bool reinit) { 
	if (!state_ || reinit) {
		delete state_;
		state_ = 0;
	}
	else {
		state_->~State();
		new (state_) State(*solver_, *params_); 
	}
}
void BasicSolve::reset(Solver& s, const SolveParams& p, SolveLimits* lim) { 
	solver_ = &s; 
	params_ = &p; 
	limits_ = lim;
	reset(true); 
}

ValueRep BasicSolve::solve() {
	if (limits_ && limits_->reached())           { return value_free;  }
	if (!state_ && !params_->randomize(*solver_)){ return value_false; }
	if (!state_)                                 { state_ = new State(*solver_, *params_); }
	return state_->solve(*solver_, *params_, limits_);
}

bool BasicSolve::satisfiable(const LitVec& path, bool init) {
	if (!solver_->clearAssumptions() || !solver_->pushRoot(path)){ return false; }
	if (init && !params_->randomize(*solver_))                   { return false; }
	State temp(*solver_, *params_);
	return temp.solve(*solver_, *params_, 0) == value_true;
}

bool BasicSolve::assume(const LitVec& path) {
	return solver_->pushRoot(path);
}

BasicSolve::State::State(Solver& s, const SolveParams& p) {
	Range32 dbLim= p.reduce.sizeInit(*s.sharedContext());
	dbGrowNext   = p.reduce.growSched.current();
	dbMax        = dbLim.lo;
	dbHigh       = dbLim.hi;
	dbRed        = p.reduce.cflSched;
	nRestart     = 0;
	nGrow        = 0;
	dbRedInit    = p.reduce.cflInit(*s.sharedContext());
	dbPinned     = 0;
	rsShuffle    = p.restart.shuffle;
	if (dbLim.lo < s.numLearntConstraints()) { 
		dbMax      = std::min(dbHigh, double(s.numLearntConstraints() + p.reduce.initRange.lo));
	}
	if (dbRedInit && dbRed.type != ScheduleStrategy::luby_schedule) {
		if (dbRedInit < dbRed.base) {
			dbRedInit  = std::min(dbRed.base, std::max(dbRedInit,(uint32)5000));
			dbRed.grow = dbRedInit != dbRed.base ? std::min(dbRed.grow, dbRedInit/2.0f) : dbRed.grow;
			dbRed.base = dbRedInit;
		}
		dbRedInit = 0;
	}
	if (p.restart.dynamic()) {
		s.stats.enableQueue(p.restart.sched.base);
		s.stats.queue->resetGlobal();
		s.stats.queue->dynamicRestarts((float)p.restart.sched.grow, true);
	}
	s.stats.lastRestart = s.stats.analyzed;
}

ValueRep BasicSolve::State::solve(Solver& s, const SolveParams& p, SolveLimits* lim) {
	assert(!lim || !lim->reached());
	if (s.hasConflict() && s.decisionLevel() == s.rootLevel()) {
		return value_false;
	}
	struct ConflictLimits {
		uint64 restart; // current restart limit
		uint64 reduce;  // current reduce limit
		uint64 grow;    // current limit for next growth operation
		uint64 global;  // current global limit
		uint64 min()      const { return std::min(std::min(restart, grow), std::min(reduce, global)); }
		void  update(uint64 x)  { restart -= x; reduce -= x; grow -= x; global -= x; }
	};
	WeightLitVec inDegree;
	SearchLimits sLimit;
	ScheduleStrategy rs     = p.restart.sched;
	ScheduleStrategy dbGrow = p.reduce.growSched;
	Solver::DBInfo  db      = {0,0,dbPinned};
	ValueRep result         = value_free;
	ConflictLimits cLimit   = {UINT64_MAX, dbRed.current() + dbRedInit, dbGrowNext, lim ? lim->conflicts : UINT64_MAX};
	uint64  limRestarts     = lim ? lim->restarts : UINT64_MAX;
	uint64& rsLimit         = p.restart.local() ? sLimit.local : cLimit.restart;
	if (!dbGrow.disabled())  { dbGrow.advanceTo(nGrow); }
	if (nRestart == UINT32_MAX && p.restart.update() == RestartParams::seq_disable) {
		cLimit.restart  = UINT64_MAX;
		sLimit          = SearchLimits();
	}
	else if (p.restart.dynamic()) {
		if (!nRestart) { s.stats.queue->resetQueue(); s.stats.queue->dynamicRestarts((float)p.restart.sched.grow, true); }
		sLimit.dynamic  = s.stats.queue; 
		rsLimit         = sLimit.dynamic->upForce - std::min(sLimit.dynamic->samples, sLimit.dynamic->upForce - 1);
	}
	else {
		rs.advanceTo(!rs.disabled() ? nRestart : 0);
		rsLimit         = rs.current();
	}
	sLimit.setMemLimit(p.reduce.memMax);
	for (EventType progress(s, EventType::event_restart, 0, 0); cLimit.global; ) {
		uint64 minLimit = cLimit.min(); assert(minLimit);
		sLimit.learnts  = (uint32)std::min(dbMax + (db.pinned*p.reduce.strategy.noGlue), dbHigh);
		sLimit.conflicts= minLimit;
		progress.cLimit = std::min(minLimit, sLimit.local);
		progress.lLimit = sLimit.learnts;
		if (progress.op) { s.sharedContext()->report(progress); progress.op = (uint32)EventType::event_none; }
		result          = s.search(sLimit, p.randProb);
		minLimit       -= sLimit.conflicts; // number of conflicts in this iteration
		if (result != value_free) {
			progress.op = static_cast<uint32>(EventType::event_exit);
			if (result == value_true && p.restart.update() != RestartParams::seq_continue) { 
				if      (p.restart.update() == RestartParams::seq_repeat) { nRestart = 0; }
				else if (p.restart.update() == RestartParams::seq_disable){ nRestart = UINT32_MAX; }
			}
			if (!dbGrow.disabled()) { dbGrowNext = std::max(cLimit.grow - minLimit, uint64(1)); }
			s.sharedContext()->report(progress);
			break;
		}
		cLimit.update(minLimit);
		minLimit = 0;
		if (rsLimit == 0 || sLimit.hasDynamicRestart()) {
			// restart reached - do restart
			++nRestart;
			if (p.restart.counterRestart && (nRestart % p.restart.counterRestart) == 0 ) {
				inDegree.clear();
				s.heuristic()->bump(s, inDegree, p.restart.counterBump / (double)s.inDegree(inDegree));
			}
			if (sLimit.dynamic) {
				minLimit = sLimit.dynamic->samples;
				rsLimit  = sLimit.dynamic->restart(rs.len ? rs.len : UINT32_MAX, (float)rs.grow);
			}
			s.restart();
			if (rsLimit == 0)              { rsLimit   = rs.next(); }
			if (!minLimit)                 { minLimit  = rs.current(); }
			if (p.reduce.strategy.fRestart){ db        = s.reduceLearnts(p.reduce.fRestart(), p.reduce.strategy); }
			if (nRestart == rsShuffle)     { rsShuffle+= p.restart.shuffleNext; s.shuffleOnNextSimplify();}
			if (--limRestarts == 0)        { break; }
			s.stats.lastRestart = s.stats.analyzed;
			progress.op         = (uint32)EventType::event_restart;
		}
		if (cLimit.reduce == 0 || s.learntLimit(sLimit)) {
			// reduction reached - remove learnt constraints
			db              = s.reduceLearnts(p.reduce.fReduce(), p.reduce.strategy);
			cLimit.reduce   = dbRedInit + (cLimit.reduce == 0 ? dbRed.next() : dbRed.current());
			progress.op     = std::max(progress.op, (uint32)EventType::event_deletion);
			if (s.learntLimit(sLimit) || db.pinned >= dbMax) { 
				ReduceStrategy t; t.algo = 2; t.score = 2; t.glue = 0;
				db.pinned /= 2;
				db.size    = s.reduceLearnts(0.5f, t).size;
				if (db.size >= sLimit.learnts) { dbMax = std::min(dbMax + std::max(100.0, s.numLearntConstraints()/10.0), dbHigh); }
			}
		}
		if (cLimit.grow == 0 || (dbGrow.defaulted() && progress.op == (uint32)EventType::event_restart)) {
			// grow sched reached - increase max db size
			if (cLimit.grow == 0)                             { cLimit.grow = dbGrow.next(); minLimit = cLimit.grow; ++nGrow; }
			if ((s.numLearntConstraints() + minLimit) > dbMax){ dbMax  *= p.reduce.fGrow; progress.op = std::max(progress.op, (uint32)EventType::event_grow); }
			if (dbMax > dbHigh)                               { dbMax   = dbHigh; cLimit.grow = UINT64_MAX; dbGrow = ScheduleStrategy::none(); }
		}
	}
	dbPinned            = db.pinned;
	s.stats.lastRestart = s.stats.analyzed - s.stats.lastRestart;
	if (lim) {
		if (lim->conflicts != UINT64_MAX) { lim->conflicts = cLimit.global; }
		if (lim->restarts  != UINT64_MAX) { lim->restarts  = limRestarts;   }
	}
	return result;
}
/////////////////////////////////////////////////////////////////////////////////////////
// SolveAlgorithm
/////////////////////////////////////////////////////////////////////////////////////////
SolveAlgorithm::SolveAlgorithm(Enumerator* e, const SolveLimits& lim) : limits_(lim), enum_(e), onModel_(0), enumLimit_(UINT64_MAX)  {}
SolveAlgorithm::~SolveAlgorithm() {}

bool SolveAlgorithm::interrupt() {
	return doInterrupt();
}
bool SolveAlgorithm::solve(SharedContext& ctx, const LitVec& assume, ModelHandler* onModel) {
	struct Scoped {
		explicit Scoped(SolveAlgorithm& self, SharedContext& x) : algo(&self), ctx(&x), temp(0), time(0.0) { }
		~Scoped() {
			ctx->master()->stats.addCpuTime(ThreadTime::getTime() - time);
			if (algo->enum_ == temp) { algo->enum_ = 0; }
			algo->onModel_ = 0;
			delete temp;
		}
		bool solve(const LitVec& assume, ModelHandler* h) {
			time = ThreadTime::getTime();
			if (!algo->enum_) { temp = EnumOptions::nullEnumerator(); algo->setEnumerator(*temp); }
			algo->onModel_ = h;
			if (algo->maxModels() != UINT64_MAX) {
				if (algo->enum_->optimize() && !algo->enum_->tentative()) { 
					ctx->report(warning(Event::subsystem_solve, "#models not 0: optimality of last model not guaranteed."));
				}
				if (algo->enum_->lastModel().consequences()) { 
					ctx->report(warning(Event::subsystem_solve, "#models not 0: last model may not cover consequences.")); 
				}
			}
			return algo->doSolve(*ctx, assume);
		}
		SolveAlgorithm* algo;
		SharedContext*  ctx;
		Enumerator*     temp;
		double          time;
	};
	if (!ctx.frozen()) { ctx.endInit(); }
	ctx.report(message<Event::verbosity_low>(Event::subsystem_solve, "Solving"));
	if (!ctx.ok() || !limits_.conflicts || interrupted()) {
		return ctx.ok();
	}
	return Scoped(*this, ctx).solve(assume, onModel);
}
bool SolveAlgorithm::reportModel(Solver& s) const {
	for (const Model& m = enum_->lastModel();;) {
		bool r1 = !onModel_ || onModel_->onModel(s, m);
		bool r2 = s.sharedContext()->report(s, m);
		bool res= r1 && r2 && (enumLimit_ > m.num || enum_->tentative());
		if (!res || (res = !interrupted()) == false || !enum_->commitSymmetric(s)) { return res; }
	}
}
bool SolveAlgorithm::moreModels(const Solver& s) const {
	return s.decisionLevel() != 0 || !s.symmetric().empty() || (!s.sharedContext()->preserveModels() && s.sharedContext()->numEliminatedVars());
}
/////////////////////////////////////////////////////////////////////////////////////////
// SequentialSolve
/////////////////////////////////////////////////////////////////////////////////////////
struct SequentialSolve::InterruptHandler : public MessageHandler {
	InterruptHandler() : solver(0) { term = 0; }
	bool   terminated() const { return term != 0; }
	bool   terminate()        { return (term = 1) != 0; }
	bool   attach(Solver& s)  { return !terminated() && (solver = &s)->addPost(this); }
	void   detach()           { if (solver) { solver->removePost(this); solver = 0; } }
	bool   handleMessages()                           { return !terminated() || (solver->setStopConflict(), false); }
	bool   propagateFixpoint(Solver&, PostPropagator*){ return InterruptHandler::handleMessages(); }
	Solver*            solver;
	Clasp::atomic<int> term;
};
SequentialSolve::SequentialSolve(Enumerator* enumerator, const SolveLimits& limit)
	: SolveAlgorithm(enumerator, limit)
	, term_(0) {
}
SequentialSolve::~SequentialSolve() {
	if (term_) { term_->detach(); delete term_; }
}
void SequentialSolve::resetSolve()       { if (term_) { term_->term = 0; } }
bool SequentialSolve::doInterrupt()      { return term_ && term_->terminate(); }
void SequentialSolve::enableInterrupts() { if (!term_) { term_ = new InterruptHandler(); } }
bool SequentialSolve::interrupted() const{ return term_ && term_->terminated(); }

bool SequentialSolve::doSolve(SharedContext& ctx, const LitVec& gp) {
	Solver&        s = *ctx.master();
	SolveLimits  lim = limits();
	uint32      root = s.rootLevel();
	BasicSolve solve(s, ctx.configuration()->search(0), &lim);
	bool        stop = term_ && !term_->attach(s);
	bool        more = !stop && ctx.attach(s) && enumerator().start(s, gp);
	// Add assumptions - if this fails, the problem is unsat 
	// under the current assumptions but not necessarily unsat.
	for (ValueRep res; more; solve.reset()) {
		while ((res = solve.solve()) == value_true && (!enumerator().commitModel(s) || reportModel(s))) {
			enumerator().update(s);
		}
		if      (res != value_false)           { more = (res == value_free || moreModels(s)); break; }
		else if ((stop=interrupted()) == true) { break; }
		else if (enumerator().commitUnsat(s))  { enumerator().update(s); }
		else if (enumerator().commitComplete()){ more = false; break; }
		else                                   { enumerator().end(s); more = enumerator().start(s, gp); }
	}
	s.popRootLevel(s.rootLevel() - root);
	if (term_) { term_->detach(); }
	ctx.detach(s);
	return more || stop;
}
}
