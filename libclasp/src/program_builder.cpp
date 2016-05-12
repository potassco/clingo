//
// Copyright (c) 2006, 2007, 2012 Benjamin Kaufmann
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
#include <clasp/program_builder.h>
#include <clasp/shared_context.h>
#include <clasp/solver.h>
#include <clasp/clause.h>
#include <clasp/minimize_constraint.h>
#include <clasp/weight_constraint.h>
#include <clasp/parser.h>
#include <limits>
namespace Clasp {

/////////////////////////////////////////////////////////////////////////////////////////
// class ProgramBuilder
/////////////////////////////////////////////////////////////////////////////////////////
ProgramBuilder::ProgramBuilder() : ctx_(0), min_(0), minCon_(0), frozen_(true) {}
ProgramBuilder::~ProgramBuilder() {}
bool ProgramBuilder::ok() const { return ctx_ && ctx_->ok(); }
bool ProgramBuilder::startProgram(SharedContext& ctx) {
	ctx.report(message(Event::subsystem_load, "Reading"));
	ctx_    = &ctx;
	min_    = 0;
	minCon_ = 0;
	frozen_ = ctx.frozen();
	return ctx_->ok() && doStartProgram();
}
bool ProgramBuilder::updateProgram() {
	CLASP_ASSERT_CONTRACT_MSG(ctx_, "startProgram() not called!");
	bool up = frozen();
	bool ok = ctx_->ok() && ctx_->unfreeze() && doUpdateProgram();
	frozen_ = ctx_->frozen();
	min_    = 0;
	if (minCon_.get())  { minCon_->resetBounds(); }
	if (up && !frozen()){ ctx_->report(message(Event::subsystem_load, "Reading")); }
	return ok;
}
bool ProgramBuilder::endProgram() {
	CLASP_ASSERT_CONTRACT_MSG(ctx_, "startProgram() not called!");
	bool ok = ctx_->ok();
	if (ok && !frozen_) {
		ctx_->report(message(Event::subsystem_prepare, "Preprocessing"));
		ok      = doEndProgram();
		frozen_ = true;
	}
	return ok;
}
void ProgramBuilder::getAssumptions(LitVec& out) const {
	CLASP_ASSERT_CONTRACT(ctx_ && frozen());
	if (!isSentinel(ctx_->stepLiteral())) {
		out.push_back(ctx_->stepLiteral());
	}
	doGetAssumptions(out);
}
bool ProgramBuilder::parseProgram(StreamSource& prg) {
	CLASP_ASSERT_CONTRACT(ctx_ && !frozen());
	return doParse(prg);
}
bool ProgramBuilder::parseProgram(std::istream& prg) {
	StreamSource input(prg);
	return parseProgram(input);
}
void ProgramBuilder::addMinLit(WeightLiteral lit)         { if (!min_.get()) { min_ = new MinimizeBuilder(); } min_->addLit(0, lit); }
void ProgramBuilder::addMinRule(const WeightLitVec& lits) { if (!min_.get()) { min_ = new MinimizeBuilder(); } min_->addRule(lits);  }
void ProgramBuilder::disposeMin()                         { min_ = 0; }
void ProgramBuilder::disposeMinimizeConstraint()          { minCon_ = 0; }
void ProgramBuilder::getMinBound(SumVec&) const           {}
SharedMinimizeData* ProgramBuilder::getMinimizeConstraint(SumVec* bound) const {
	if (min_.get() && min_->hasRules()) {
		if (bound) getMinBound(*bound);
		minCon_ = min_->build(*ctx_);
		min_    = 0;
	}
	return minCon_.get(); 
}
/////////////////////////////////////////////////////////////////////////////////////////
// class SatBuilder
/////////////////////////////////////////////////////////////////////////////////////////
SatBuilder::SatBuilder(bool maxSat) : ProgramBuilder(), hardWeight_(0), vars_(0), pos_(0), maxSat_(maxSat) {}
bool SatBuilder::markAssigned() {
	if (pos_ == ctx()->master()->trail().size()) { return true; }
	bool ok = ctx()->ok() && ctx()->master()->propagate();
	for (const LitVec& trail = ctx()->master()->trail(); pos_ < trail.size(); ++pos_) {
		markLit(~trail[pos_]);
	}
	return ok;
}
void SatBuilder::prepareProblem(uint32 numVars, wsum_t cw, uint32 clauseHint) {
	CLASP_ASSERT_CONTRACT_MSG(ctx(), "startProgram() not called!");
	ctx()->resizeVars(numVars + 1);
	ctx()->symbolTable().startInit(SymbolTable::map_direct);
	ctx()->symbolTable().add(numVars+1);
	ctx()->symbolTable().endInit();
	ctx()->startAddConstraints(std::min(clauseHint, uint32(10000)));
	varState_.resize(numVars + 1);
	vars_       = ctx()->numVars();
	hardWeight_ = cw;
	markAssigned();
}
bool SatBuilder::addClause(LitVec& clause, wsum_t cw) {
	if (!ctx()->ok() || satisfied(clause)) { return ctx()->ok(); }
	CLASP_ASSERT_CONTRACT_MSG(cw >= 0 && (cw <= std::numeric_limits<weight_t>::max() || cw == hardWeight_), "Clause weight out of bounds!");
	if (cw == 0 && maxSat_){ cw = 1; }
	if (cw == hardWeight_) {
		return ClauseCreator::create(*ctx()->master(), clause, Constraint_t::static_constraint).ok() && markAssigned();
	}
	else {
		// Store weight, relaxtion var, and (optionally) clause
		softClauses_.push_back(Literal::fromRep((uint32)cw));
		if      (clause.size() > 1){ softClauses_.push_back(posLit(++vars_)); softClauses_.insert(softClauses_.end(), clause.begin(), clause.end()); }
		else if (!clause.empty())  { softClauses_.push_back(~clause.back());  }
		else                       { softClauses_.push_back(posLit(0)); }
		softClauses_.back().watch(); // mark end of clause
	}
	return true;
}
bool SatBuilder::satisfied(LitVec& cc) {
	bool sat = false;
	LitVec::iterator j = cc.begin();
	for (LitVec::const_iterator it = cc.begin(), end = cc.end(); it != end; ++it) {
		Literal x = *it;
		uint32  m = 1+x.sign();
		uint32  n = uint32(varState_[it->var()] & 3u) + m;
		if      (n == m) { varState_[it->var()] |= m; x.clearWatch(); *j++ = x; }
		else if (n == 3u){ sat = true; break; } 
	}
	cc.erase(j, cc.end());
	for (LitVec::const_iterator it = cc.begin(), end = cc.end(); it != end; ++it) { 
		if (!sat) { varState_[it->var()] |= (varState_[it->var()] & 3u) << 2; }
		varState_[it->var()] &= ~3u;
	}
	return sat;
}
bool SatBuilder::doStartProgram() {
	vars_ = ctx()->numVars();
	pos_  = 0;
	return markAssigned();
}
bool SatBuilder::doParse(StreamSource& prg) { return DimacsParser(*this).parse(prg); }
bool SatBuilder::doEndProgram() {
	bool ok = ctx()->ok();
	if (!softClauses_.empty() && ok) {
		ctx()->setPreserveModels(true);
		ctx()->resizeVars(vars_+1);
		ctx()->startAddConstraints();
		LitVec cc; 
		for (LitVec::const_iterator it = softClauses_.begin(), end = softClauses_.end(); it != end && ok; ++it) {
			weight_t w     = (weight_t)it->asUint();
			Literal  relax = *++it;
			if (!relax.watched()) { 
				cc.assign(1, relax);
				do { cc.push_back(*++it); } while (!cc.back().watched());
				cc.back().clearWatch();
				ok = ClauseCreator::create(*ctx()->master(), cc, Constraint_t::static_constraint).ok();
			}
			relax.clearWatch();
			addMinLit(WeightLiteral(relax, w));
		}
		LitVec().swap(softClauses_);
	}
	if (ok && !ctx()->preserveModels()) {
		uint32 p    = 12;
		for (Var v  = 1; v != (Var)varState_.size() && ok; ++v) {
			uint32 m  = varState_[v];
			if ( (m & p) != p ) { ok = ctx()->addUnary(Literal(v, ((m>>2) & 1u) != 1)); }
		}
	}
	return ok;
}
/////////////////////////////////////////////////////////////////////////////////////////
// class PBBuilder
/////////////////////////////////////////////////////////////////////////////////////////
PBBuilder::PBBuilder() : nextVar_(0) {}
void PBBuilder::prepareProblem(uint32 numVars, uint32 numProd, uint32 numSoft, uint32 numCons) {
	CLASP_ASSERT_CONTRACT_MSG(ctx(), "startProgram() not called!");
	uint32 maxVar = numVars + numProd + numSoft;
	nextVar_      = numVars;
	maxVar_       = maxVar;
	ctx()->resizeVars(maxVar + 1);
	ctx()->symbolTable().startInit(SymbolTable::map_direct);
	ctx()->symbolTable().add(numVars+1);
	ctx()->symbolTable().endInit();
	ctx()->startAddConstraints(numCons);
}
uint32 PBBuilder::getNextVar() {
	CLASP_ASSERT_CONTRACT_MSG(ctx()->validVar(nextVar_+1), "Variables out of bounds");
	return ++nextVar_;
}
bool PBBuilder::addConstraint(WeightLitVec& lits, weight_t bound, bool eq, weight_t cw) {
	if (!ctx()->ok()) { return false; }
	Var eqVar = 0;
	if (cw > 0) { // soft constraint
		if (lits.size() != 1) {
			eqVar = getNextVar();
			addMinLit(WeightLiteral(negLit(eqVar), cw));
		}
		else {
			if (lits[0].second < 0)    { bound += (lits[0].second = -lits[0].second); lits[0].first = ~lits[0].first; }
			if (lits[0].second < bound){ lits[0].first = negLit(0); }
			addMinLit(WeightLiteral(~lits[0].first, cw));
			return true;
		}
	}
	return WeightConstraint::create(*ctx()->master(), posLit(eqVar), lits, bound, !eq ? 0 : WeightConstraint::create_eq_bound).ok();
}

bool PBBuilder::addObjective(const WeightLitVec& min) {
	addMinRule(min);
	return ctx()->ok();
}

bool PBBuilder::setSoftBound(wsum_t b) {
	if (b > 0) { soft_ = b-1; }
	return true;
}

void PBBuilder::getMinBound(SumVec& out) const {
	if (soft_ != std::numeric_limits<wsum_t>::max()) {
		if      (out.empty())   { out.push_back(soft_); }
		else if (out[0] > soft_){ out[0] = soft_; }
	}
}

Literal PBBuilder::addProduct(LitVec& lits) {
	if (!ctx()->ok()) { return negLit(0); }
	Literal prodLit;
	if (productSubsumed(lits, prodLit)){ return prodLit; }
	ProductIndex::iterator it = products_.find(lits);
	if (it != products_.end()) { return it->second; }
	prodLit = posLit(getNextVar());
	products_.insert(it, ProductIndex::value_type(lits, prodLit));
	addProductConstraints(prodLit, lits);
	return prodLit;
}
bool PBBuilder::productSubsumed(LitVec& lits, Literal& subLit) {
	Literal last       = posLit(0);
	LitVec::iterator j = lits.begin();
	Solver& s          = *ctx()->master();
	subLit             = posLit(0);
	for (LitVec::const_iterator it = lits.begin(), end = lits.end(); it != end; ++it) {
		if (s.isFalse(*it) || ~*it == last) { // product is always false
			subLit = negLit(0);
			return true;
		}
		else if (last.var() > it->var()) { // not sorted - redo with sorted product
			std::sort(lits.begin(), lits.end());
			return productSubsumed(lits, subLit);
		}
		else if (!s.isTrue(*it) && last != *it) {
			last  = *it;
			*j++  = last;
		}
	}
	lits.erase(j, lits.end());
	if (lits.size() == 1) { subLit = lits[0]; }
	return lits.size() < 2;
}
void PBBuilder::addProductConstraints(Literal eqLit, LitVec& lits) {
	Solver& s = *ctx()->master();
	assert(s.value(eqLit.var()) == value_free);
	bool ok   = ctx()->ok();
	for (LitVec::iterator it = lits.begin(), end = lits.end(); it != end && ok; ++it) {
		assert(s.value(it->var()) == value_free);
		ok    = ctx()->addBinary(~eqLit, *it);
		*it   = ~*it;
	}
	lits.push_back(eqLit);
	if (ok) { ClauseCreator::create(s, lits, ClauseCreator::clause_no_prepare, ClauseInfo()); }
}

bool PBBuilder::doStartProgram() {
	nextVar_ = ctx()->numVars();
	soft_    = std::numeric_limits<wsum_t>::max();
	return true;
}
bool PBBuilder::doEndProgram() {
	while (nextVar_ < maxVar_) {
		if (!ctx()->addUnary(negLit(++nextVar_))) { return false; }
	}
	return true;
}
bool PBBuilder::doParse(StreamSource& prg) { return OPBParser(*this).parse(prg); }

}
