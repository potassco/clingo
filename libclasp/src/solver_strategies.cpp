// 
// Copyright (c) Benjamin Kaufmann
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
#include <clasp/solver_strategies.h>
#include <clasp/solver.h>
#include <clasp/heuristics.h>
#include <clasp/lookahead.h>
#include <cmath>
namespace Clasp {
/////////////////////////////////////////////////////////////////////////////////////////
// SolverStrategies / SolverParams
/////////////////////////////////////////////////////////////////////////////////////////
SolverStrategies::SolverStrategies() {
	struct X { uint32 z[2]; };
	static_assert(sizeof(SolverStrategies) == sizeof(X), "Unsupported Padding");
	std::memset(this, 0, sizeof(SolverStrategies));
	ccMinAntes = all_antes;
	initWatches= SolverStrategies::watch_rand;
	search     = use_learning;
}
void SolverStrategies::prepare() {
	if (search == SolverStrategies::no_learning) {
		compress    = 0;
		saveProgress= 0;
		reverseArcs = 0;
		otfs        = 0;
		updateLbd   = 0;
		ccMinAntes  = SolverStrategies::no_antes;
		bumpVarAct  = 0;
	}
}
SolverParams::SolverParams() {
	struct X { uint32 strat[2]; uint32 self[3]; };
	static_assert(sizeof(SolverParams) == sizeof(X), "Unsupported Padding");
	std::memset((&seed)+1, 0, sizeof(uint32)*2);
	seed     = RNG().seed();
	heuOther = 3;
	heuMoms  = 1;
}
uint32 SolverParams::prepare() {
	uint32 res = 0;
	if (search == SolverStrategies::no_learning && Heuristic_t::isLookback(heuId)) {
		heuId = Heuristic_t::heu_none;
		res  |= 1;
	}
	if (heuId == Heuristic_t::heu_unit) {
		if (!Lookahead::isType(lookType)) { res |= 2; lookType = Lookahead::atom_lookahead; }
		lookOps = 0;
	}
	if (heuId != Heuristic_t::heu_domain && (domPref || domMod)) {
		res |= 4;
		domPref= 0;
		domMod = 0;
	}
	SolverStrategies::prepare();
	return res;
}
/////////////////////////////////////////////////////////////////////////////////////////
// ScheduleStrategy
/////////////////////////////////////////////////////////////////////////////////////////
double growR(uint32 idx, double g) { return pow(g, (double)idx); }
double addR(uint32 idx, double a)  { return a * idx; }
uint32 lubyR(uint32 idx)           {
	uint32 i = idx + 1;
	while ((i & (i+1)) != 0) {
		i    -= ((1u << log2(i)) - 1);
	}
	return (i+1)>>1;
}
ScheduleStrategy::ScheduleStrategy(Type t, uint32 b, double up, uint32 lim)
	: base(b), type(t), idx(0), len(lim), grow(0.0)  {
	if      (t == geometric_schedule)  { grow = static_cast<float>(std::max(1.0, up)); }
	else if (t == arithmetic_schedule) { grow = static_cast<float>(std::max(0.0, up)); }
	else if (t == user_schedule)       { grow = static_cast<float>(std::max(0.0, up)); }
	else if (t == luby_schedule && lim){ len  = std::max(uint32(2), (static_cast<uint32>(std::pow(2.0, std::ceil(log(double(lim))/log(2.0)))) - 1)*2); }
}

uint64 ScheduleStrategy::current() const {
	enum { t_add = ScheduleStrategy::arithmetic_schedule, t_luby = ScheduleStrategy::luby_schedule };
	if      (base == 0)     return UINT64_MAX;
	else if (type == t_add) return static_cast<uint64>(addR(idx, grow)  + base);
	else if (type == t_luby)return static_cast<uint64>(lubyR(idx)) * base;
	uint64 x = static_cast<uint64>(growR(idx, grow) * base);
	return x + !x;
}
uint64 ScheduleStrategy::next() {
	if (++idx != len) { return current(); }
	// length reached or overflow
	len = (len + !!idx) << uint32(type == luby_schedule);
	idx = 0;
	return current();
}
void ScheduleStrategy::advanceTo(uint32 n) {
	if (!len || n < len)       { 
		idx = n; 
		return; 
	}
	if (type != luby_schedule) {
		double dLen = len;
		uint32 x    = uint32(sqrt(dLen * (4.0 * dLen - 4.0) + 8.0 * double(n+1))-2*dLen+1)/2;
    idx         = n - uint32(x*dLen+double(x-1.0)*x/2.0);
    len        += x;
		return;
	}
	while (n >= len) {
		n   -= len++;
		len *= 2;
	}
	idx = n;
}
/////////////////////////////////////////////////////////////////////////////////////////
// RestartParams
/////////////////////////////////////////////////////////////////////////////////////////
void RestartParams::disable() {
	std::memset(this, 0, sizeof(RestartParams));
	sched = ScheduleStrategy::none();
}
uint32 RestartParams::prepare(bool withLookback) {
	if (!withLookback || sched.disabled()) {
		disable();
	}
	return 0;
}
uint32 SumQueue::restart(uint32 maxLBD, float limMax) {
	++nRestart;
	if (upCfl >= upForce) {
		double avg = upCfl / double(nRestart);
		double gLbd= globalAvgLbd();
		bool   sx  = samples >= upForce;
		upCfl      = 0;
		nRestart   = 0;
		if      (avg >= 16000.0) { lim += 0.1f;  upForce = 16000; }
		else if (sx)             { lim += 0.05f; upForce = std::max(uint32(16000), upForce-10000); }
		else if (avg >= 4000.0)  { lim += 0.05f; }
		else if (avg >= 1000.0)  { upForce += 10000u; }
		else if (lim > limMax)   { lim -= 0.05f; }
		if ((gLbd > maxLBD)==lbd){ dynamicRestarts(limMax, !lbd); }
	}
	resetQueue();
	return upForce;
}
/////////////////////////////////////////////////////////////////////////////////////////
// ReduceParams
/////////////////////////////////////////////////////////////////////////////////////////
uint32 ReduceParams::getLimit(uint32 base, double f, const Range32& r) {
	base = (f != 0.0 ? (uint32)std::min(base*f, double(UINT32_MAX)) : UINT32_MAX);
	return r.clamp( base );
}
uint32 ReduceParams::getBase(const SharedContext& ctx) const {
	uint32 st = strategy.estimate != ReduceStrategy::est_dynamic || ctx.isExtended() ? strategy.estimate : (uint32)ReduceStrategy::est_num_constraints;
	switch(st) {
		default:
		case ReduceStrategy::est_dynamic        : {
			uint32 m = std::min(ctx.stats().vars, ctx.stats().numConstraints());
			uint32 M = std::max(ctx.stats().vars, ctx.stats().numConstraints());
			return M > (m * 10) ? M : m;
		}
		case ReduceStrategy::est_con_complexity : return ctx.stats().complexity;	
		case ReduceStrategy::est_num_constraints: return ctx.stats().numConstraints();
		case ReduceStrategy::est_num_vars       : return ctx.stats().vars;
	}
}
void ReduceParams::disable() {
	cflSched  = ScheduleStrategy::none();
	growSched = ScheduleStrategy::none();
	strategy.fReduce = 0;
	fGrow     = 0.0f; fInit = 0.0f; fMax = 0.0f;
	initRange = Range<uint32>(UINT32_MAX, UINT32_MAX); 
	maxRange  = UINT32_MAX;
	memMax    = 0;
}
Range32 ReduceParams::sizeInit(const SharedContext& ctx) const {
	if (!growSched.disabled() || growSched.defaulted()) {
		uint32 base = getBase(ctx);
		uint32 lo   = std::min(getLimit(base, fInit, initRange), maxRange);
		uint32 hi   = getLimit(base, fMax, Range32(lo, maxRange));
		return Range32(lo, hi);
	}
	return Range32(maxRange, maxRange);
}
uint32 ReduceParams::cflInit(const SharedContext& ctx) const {
	return cflSched.disabled() ? 0 : getLimit(getBase(ctx), fInit, initRange);
}
uint32 ReduceParams::prepare(bool withLookback) {
	if (!withLookback || fReduce() == 0.0f) {
		disable();
		return 0;
	}
	if (cflSched.defaulted() && growSched.disabled() && !growSched.defaulted()) {
		cflSched = ScheduleStrategy::arith(4000, 600);
	}
	if (fMax != 0.0f) { fMax = std::max(fMax, fInit); }
	return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////
// SolveParams
/////////////////////////////////////////////////////////////////////////////////////////
SolveParams::SolveParams() 
	: randRuns(0u), randConf(0u)
	, randProb(0.0f) {
}
uint32 SolveParams::prepare(bool withLookback) {
	return restart.prepare(withLookback) | reduce.prepare(withLookback);
}
bool SolveParams::randomize(Solver& s) const {
	for (uint32 r = 0, c = randConf; r != randRuns && c; ++r) {
		if (s.search(c, UINT32_MAX, false, 1.0) != value_free) { return !s.hasConflict(); }
		s.undoUntil(0);
	}
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////
// Configurations
/////////////////////////////////////////////////////////////////////////////////////////
Configuration::~Configuration() {}
bool UserConfiguration::addPost(Solver& s) const {
	const SolverOpts& x = solver(s.id());
	bool  ok            = true;
	if (x.lookType != Lookahead::no_lookahead) {
		PostPropagator* pp = s.getPost(PostPropagator::priority_reserved_look);
		if (pp) { pp->destroy(&s, true); }
		Lookahead::Params p(static_cast<Lookahead::Type>(x.lookType));
		p.nant(x.unitNant != 0);
		p.limit(x.lookOps);
		ok = s.addPost(new Lookahead(p));
	}
	return ok;
}
BasicSatConfig::BasicSatConfig() {
	solver_.push_back(SolverParams());
	search_.push_back(SolveParams());
}
void BasicSatConfig::prepare(SharedContext& ctx) {
	uint32 warn = 0;
	for (uint32 i = 0, end = solver_.size(), mod = search_.size(); i != end; ++i) {
		warn |= solver_[i].prepare();
		warn |= search_[i%mod].prepare(solver_[i].search != SolverStrategies::no_learning);
	}
	if ((warn & 1) != 0) { ctx.report(warning(Event::subsystem_facade, "Selected heuristic requires lookback strategy!")); }
	if ((warn & 2) != 0) { ctx.report(warning(Event::subsystem_facade, "Heuristic 'Unit' implies lookahead. Using atom.")); }
	if ((warn & 4) != 0) { ctx.report(warning(Event::subsystem_facade, "Domain options require heuristic 'Domain'!")); }
}
DecisionHeuristic* BasicSatConfig::heuristic(uint32 i)  const {
	return Heuristic_t::create(BasicSatConfig::solver(i));
}
SolverParams& BasicSatConfig::addSolver(uint32 i) {
	while (i >= solver_.size()) { 
		solver_.push_back(SolverParams());
		solver_.back().id = static_cast<uint32>(solver_.size()) - 1;
	}
	return solver_[i];
}
SolveParams& BasicSatConfig::addSearch(uint32 i) {
	if (i >= search_.size()) { search_.resize(i+1); }
	return search_[i];
}

void BasicSatConfig::reset() {
	static_cast<ContextParams&>(*this) = ContextParams();
	BasicSatConfig::resize(1, 1);
	solver_[0] = SolverParams(); 
	search_[0] = SolveParams();
}
void BasicSatConfig::resize(uint32 solver, uint32 search) {
	solver_.resize(solver);
	search_.resize(search);
}
/////////////////////////////////////////////////////////////////////////////////////////
// Heuristics
/////////////////////////////////////////////////////////////////////////////////////////
DecisionHeuristic* Heuristic_t::create(const SolverParams& str) {
	CLASP_FAIL_IF(str.search != SolverStrategies::use_learning && Heuristic_t::isLookback(str.heuId), "Selected heuristic requires lookback!");
	typedef DecisionHeuristic DH;
	uint32 heuParam = str.heuParam;
	uint32 id       = str.heuId;
	DH*    heu      = 0;
	HeuParams params;
	params.other(str.heuOther);
	params.init(str.heuMoms);
	params.score(str.heuScore);
	if      (id == heu_default) { id  = str.search == SolverStrategies::use_learning ? heu_berkmin : heu_none; }
	if      (id == heu_berkmin) { heu = new ClaspBerkmin(heuParam, params, str.berkHuang != 0); }
	else if (id == heu_vmtf)    { heu = new ClaspVmtf(heuParam == 0 ? 8 : heuParam, params);    }
	else if (id == heu_unit)    { heu = new UnitHeuristic(); }
	else if (id == heu_none)    { heu = new SelectFirst(); }
	else if (id == heu_vsids || id == heu_domain) {
		double m = heuParam == 0 ? 0.95 : heuParam;
		while (m > 1.0) { m /= 10; } 
		heu = id == heu_vsids ? (DH*)new ClaspVsids(m, params) : (DH*)new DomainHeuristic(m, params);
		if (id == heu_domain) {
			static_cast<DomainHeuristic*>(heu)->setDefaultMod(static_cast<DomainHeuristic::GlobalModifier>(str.domMod), str.domPref);
		}
	}
	else { throw std::logic_error("Unknown heuristic id!"); }
	if (str.lookType != Lookahead::no_lookahead && str.lookOps > 0 && id != heu_unit) {
		heu = UnitHeuristic::restricted(heu);
	}
	return heu;
}

}
