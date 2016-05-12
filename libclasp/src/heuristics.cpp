// 
// Copyright (c) 2006-2014, Benjamin Kaufmann
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
#include <clasp/heuristics.h>
#include <clasp/dependency_graph.h>
#include <clasp/enumerator.h>
#include <clasp/clause.h>
#include <algorithm>
#include <limits>
#include <cstdlib>
#include <string>
#include <utility>
namespace Clasp {
/////////////////////////////////////////////////////////////////////////////////////////
// Lookback selection strategies
/////////////////////////////////////////////////////////////////////////////////////////
uint32 momsScore(const Solver& s, Var v) {
	uint32 sc;
	if (s.sharedContext()->numBinary()) {
		uint32 s1 = s.estimateBCP(posLit(v), 0) - 1;
		uint32 s2 = s.estimateBCP(negLit(v), 0) - 1;
		sc = ((s1 * s2)<<10) + (s1 + s2);
	}
	else {
		// problem does not contain binary constraints - fall back to counting watches
		uint32 s1 = s.numWatches(posLit(v));
		uint32 s2 = s.numWatches(negLit(v));
		sc = ((s1 * s2)<<10) + (s1 + s2);
	}
	return sc;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Berkmin selection strategy
/////////////////////////////////////////////////////////////////////////////////////////
#define BERK_NUM_CANDIDATES 5
#define BERK_CACHE_GROW 2.0
#define BERK_MAX_MOMS_VARS 9999
#define BERK_MAX_MOMS_DECS 50
#define BERK_MAX_DECAY 65534

ClaspBerkmin::ClaspBerkmin(uint32 maxB, const HeuParams& params, bool berkHuang) 
	: order_(berkHuang, params.resScore ? params.resScore : 3u)
	, topConflict_(UINT32_MAX)
	, topOther_(UINT32_MAX)
	, front_(1) 
	, cacheSize_(5)
	, numVsids_(0)
	, maxBerkmin_(maxB == 0 ? UINT32_MAX : maxB) {
	if (params.otherScore > 0u) { types_.addSet(Constraint_t::learnt_loop); }
	if (params.otherScore ==2u) { types_.addSet(Constraint_t::learnt_other);}
	if (params.initScore)       { types_.addSet(Constraint_t::static_constraint); }
}

Var ClaspBerkmin::getMostActiveFreeVar(const Solver& s) {
	++numVsids_;
	// first: check for a cache hit
	for (Pos end = cache_.end(); cacheFront_ != end; ++cacheFront_) {
		if (s.value(*cacheFront_) == value_free) {
			return *cacheFront_;
		}
	}
	// Second: cache miss - refill cache with most active vars
	if (!cache_.empty() && cacheSize_ < s.numFreeVars()/10) {
		cacheSize_ = static_cast<uint32>( (cacheSize_*BERK_CACHE_GROW) + .5 );
	}
	cache_.clear();
	Order::Compare  comp(&order_);
	// Pre: At least one unassigned var!
	for (; s.value( front_ ) != value_free; ++front_) {;}
	Var v = front_;
	LitVec::size_type cs = std::min(cacheSize_, s.numFreeVars());
	for (;;) { // add first cs free variables to cache
		cache_.push_back(v);
		std::push_heap(cache_.begin(), cache_.end(), comp);
		if (cache_.size() == cs) break;
		while ( s.value(++v) != value_free ) {;} // skip over assigned vars
	} 
	for (v = (cs == cacheSize_ ? v+1 : s.numVars()+1); v <= s.numVars(); ++v) {
		// replace vars with low activity
		if (s.value(v) == value_free && comp(v, cache_[0])) {
			std::pop_heap(cache_.begin(), cache_.end(), comp);
			cache_.back() = v;
			std::push_heap(cache_.begin(), cache_.end(), comp);
		}
	}
	std::sort_heap(cache_.begin(), cache_.end(), comp);
	return *(cacheFront_ = cache_.begin());
}

Var ClaspBerkmin::getTopMoms(const Solver& s) {
	// Pre: At least one unassigned var!
	for (; s.value( front_ ) != value_free; ++front_) {;}
	Var var   = front_;
	uint32 ms = momsScore(s, var);
	uint32 ls = 0;
	for (Var v = var+1; v <= s.numProblemVars(); ++v) {
		if (s.value(v) == value_free && (ls = momsScore(s, v)) > ms) {
			var = v;
			ms  = ls;
		}
	}
	if (++numVsids_ >= BERK_MAX_MOMS_DECS || ms < 2) { 
		// Scores are not relevant or too many moms-based decisions
		// - disable MOMS
		hasActivities(true);           
	}
	return var;
}

void ClaspBerkmin::startInit(const Solver& s) {
	if (order_.score.empty()) {
		rng_.srand(s.rng.seed());
	}
	order_.score.resize(s.numVars()+1);
	initHuang(order_.huang);

	cache_.clear();
	cacheSize_ = 5;
	cacheFront_ = cache_.end();
	
	freeLits_.clear();
	freeOtherLits_.clear();
	topConflict_ = topOther_ = (uint32)-1;
	
	front_    = 1;
	numVsids_ = 0;
}

void ClaspBerkmin::endInit(Solver& s) {
	if (initHuang()) {
		const bool clearScore = types_.inSet(Constraint_t::static_constraint);
		// initialize preferred values of vars
		cache_.clear();
		for (Var v = 1; v <= s.numVars(); ++v) {
			order_.decayedScore(v);
			if (order_.occ(v) != 0 && s.pref(v).get(ValueSet::saved_value) == value_free) {
				s.setPref(v, ValueSet::saved_value, order_.occ(v) > 0 ? value_true : value_false);
			}
			if   (clearScore) order_.score[v] = HScore(order_.decay);
			else cache_.push_back(v);
		}
		initHuang(false);
	}
	if (!types_.inSet(Constraint_t::static_constraint) || s.numFreeVars() > BERK_MAX_MOMS_VARS) {
		hasActivities(true);
	}
	std::stable_sort(cache_.begin(), cache_.end(), Order::Compare(&order_));
	cacheFront_ = cache_.begin();
}

void ClaspBerkmin::updateVar(const Solver& s, Var v, uint32 n) {
	if (s.validVar(v)) { growVecTo(order_.score, v+n); }
	front_ = 1;
	cache_.clear();
	cacheFront_ = cache_.end();
}

void ClaspBerkmin::newConstraint(const Solver&, const Literal* first, LitVec::size_type size, ConstraintType t) {
	if (t == Constraint_t::learnt_conflict) {
		hasActivities(true);
		if (order_.resScore == 1u) {
			for (const Literal* x = first, *end = first+size; x != end; ++x) { order_.inc(*x); }
		}
	}
	if (order_.huang == (t == Constraint_t::static_constraint)) {
		for (const Literal* x = first, *end = first+size; x != end; ++x) {
			order_.incOcc(*x);
		}
	}
}

void ClaspBerkmin::updateReason(const Solver& s, const LitVec& lits, Literal r) {
	if (order_.resScore > 1) {
		const bool ms = order_.resScore == 3u;
		for (LitVec::size_type i = 0, e = lits.size(); i != e; ++i) {
			if (ms || !s.seen(lits[i])) { order_.inc(~lits[i]); }
		}
	}
	if ((order_.resScore & 1u) != 0 && !isSentinel(r)) {
		order_.inc(r);
	}
}

bool ClaspBerkmin::bump(const Solver&, const WeightLitVec& lits, double adj) {
	for (WeightLitVec::const_iterator it = lits.begin(), end = lits.end(); it != end; ++it) {
		uint32 xf = order_.decayedScore(it->first.var()) + static_cast<weight_t>(it->second*adj);
		order_.score[it->first.var()].act = (uint16)std::min(xf, UINT32_MAX>>16);
	}
	return true;
}

void ClaspBerkmin::undoUntil(const Solver&, LitVec::size_type) {
	topConflict_ = topOther_ = static_cast<uint32>(-1);
	front_ = 1;
	cache_.clear();
	cacheFront_ = cache_.end();
	if (cacheSize_ > 5 && numVsids_ > 0 && (numVsids_*3) < cacheSize_) {
		cacheSize_ = static_cast<uint32>(cacheSize_/BERK_CACHE_GROW);
	}
	numVsids_ = 0;
}

bool ClaspBerkmin::hasTopUnsat(Solver& s) {
	topConflict_  = std::min(s.numLearntConstraints(), topConflict_);
	topOther_     = std::min(s.numLearntConstraints(), topOther_);
	assert(topConflict_ <= topOther_);
	freeOtherLits_.clear();
	freeLits_.clear();
	TypeSet ts = types_;
	if (ts.m > 1) {
		while (topOther_ > topConflict_) {
			if (s.getLearnt(topOther_-1).isOpen(s, ts, freeLits_) != 0) {
				freeLits_.swap(freeOtherLits_);
				ts.m = 0;
				break;
			}
			--topOther_;
			freeLits_.clear();
		}
	}
	ts.addSet(Constraint_t::learnt_conflict);
	uint32 stopAt = topConflict_ < maxBerkmin_ ? 0 : topConflict_ - maxBerkmin_;
	while (topConflict_ != stopAt) {
		uint32 x = s.getLearnt(topConflict_-1).isOpen(s, ts, freeLits_);
		if (x != 0) {
			if (x == Constraint_t::learnt_conflict) { break; }
			topOther_  = topConflict_;
			freeLits_.swap(freeOtherLits_);
			ts.m = 0;
			ts.addSet(Constraint_t::learnt_conflict);
		}
		--topConflict_;
		freeLits_.clear();
	}
	if (freeOtherLits_.empty())  topOther_ = topConflict_;
	if (freeLits_.empty())      freeOtherLits_.swap(freeLits_);
	return !freeLits_.empty();
}

Literal ClaspBerkmin::doSelect(Solver& s) {
	const uint32 decayMask = order_.huang ? 127 : 511;
	if ( ((s.stats.choices + 1)&decayMask) == 0 ) {
		if ((order_.decay += (1+!order_.huang)) == BERK_MAX_DECAY) {
			order_.resetDecay();
		}
	}
	if (hasTopUnsat(s)) {        // Berkmin based decision
		assert(!freeLits_.empty());
		Literal x = selectRange(s, &freeLits_[0], &freeLits_[0]+freeLits_.size());
		return selectLiteral(s, x.var(), false);
	}
	else if (hasActivities()) {  // Vsids based decision
		return selectLiteral(s, getMostActiveFreeVar(s), true);
	}
	else {                       // Moms based decision
		return selectLiteral(s, getTopMoms(s), true);
	}
}

Literal ClaspBerkmin::selectRange(Solver& s, const Literal* first, const Literal* last) {
	Literal candidates[BERK_NUM_CANDIDATES];
	candidates[0] = *first;
	uint32 c = 1;
	uint32  ms  = static_cast<uint32>(-1);
	uint32  ls  = 0;
	for (++first; first != last; ++first) {
		Var v = first->var();
		assert(s.value(v) == value_free);
		int cmp = order_.compare(v, candidates[0].var());
		if (cmp > 0) {
			candidates[0] = *first;
			c = 1;
			ms  = static_cast<uint32>(-1);
		}
		else if (cmp == 0) {
			if (ms == static_cast<uint32>(-1)) ms = momsScore(s, candidates[0].var());
			if ( (ls = momsScore(s,v)) > ms) {
				candidates[0] = *first;
				c = 1;
				ms  = ls;
			}
			else if (ls == ms && c != BERK_NUM_CANDIDATES) {
				candidates[c++] = *first;
			}
		}
	}
	return c == 1 ? candidates[0] : candidates[rng_.irand(c)];
}

Literal ClaspBerkmin::selectLiteral(Solver& s, Var v, bool vsids) const {
	ValueSet pref = s.pref(v);
	int signScore = order_.occ(v);
	if (order_.huang && std::abs(signScore) > 32 && !pref.has(ValueSet::user_value)) {
		return Literal(v, signScore < 0);
	}
	// compute expensive sign score only if necessary
	if (vsids && !pref.has(ValueSet::user_value | ValueSet::pref_value | ValueSet::saved_value)) {
		int32 w0 = (int32)s.estimateBCP(posLit(v), 5);
		int32 w1 = (int32)s.estimateBCP(negLit(v), 5);
		if (w1 != 1 || w0 != w1) { signScore = w0 - w1; }
	}
	return DecisionHeuristic::selectLiteral(s, v, signScore);
}

void ClaspBerkmin::Order::resetDecay() {
	for (Scores::size_type i = 1, end = score.size(); i < end; ++i) {
		decayedScore(i);
		score[i].dec = 0;
	}
	decay = 0;
}
/////////////////////////////////////////////////////////////////////////////////////////
// ClaspVmtf selection strategy
/////////////////////////////////////////////////////////////////////////////////////////
ClaspVmtf::ClaspVmtf(LitVec::size_type mtf, const HeuParams& params) : decay_(0), MOVE_TO_FRONT(mtf >= 2 ? mtf : 2) {
	scType_  = params.resScore ? params.resScore : 1u;
	uint32 x = params.otherScore + 1;
	if (x & Constraint_t::learnt_loop) { types_.addSet(Constraint_t::learnt_loop); }
	if (x & Constraint_t::learnt_other){ types_.addSet(Constraint_t::learnt_other);}
	if (scType_ == 1u)                 { types_.addSet(Constraint_t::learnt_conflict); }
	if (params.initScore)              { types_.addSet(Constraint_t::static_constraint); }
}

void ClaspVmtf::startInit(const Solver& s) {
	score_.resize(s.numVars()+1, VarInfo(vars_.end()));
}

void ClaspVmtf::endInit(Solver& s) {
	bool moms = types_.inSet(Constraint_t::static_constraint);
	for (Var v = 1; v <= s.numVars(); ++v) {
		if (s.value(v) == value_free && score_[v].pos_ == vars_.end()) {
			score_[v].activity(decay_);
			if (moms) {
				score_[v].activity_ = momsScore(s, v);
				score_[v].decay_    = decay_+1;
			}
			score_[v].pos_ = vars_.insert(vars_.end(), v);
		}
	}
	if (moms) {
		vars_.sort(LessLevel(s, score_));
		for (VarList::iterator it = vars_.begin(), end = vars_.end(); it != end; ++it) {
			if (score_[*it].decay_ != decay_) { 
				score_[*it].activity_ = 0;
				score_[*it].decay_    = decay_;
			}
		}
	}
	front_ = vars_.begin();
}

void ClaspVmtf::updateVar(const Solver& s, Var v, uint32 n) {
	if (s.validVar(v)) {
		growVecTo(score_, v+n, VarInfo(vars_.end()));
		for (uint32 end = v+n; v != end; ++v) {
			if (score_[v].pos_ == vars_.end()) { score_[v].pos_ = vars_.insert(vars_.end(), v); }
			else                               { front_ = vars_.begin(); }
		}
	}
	else if (v < score_.size()) {
		if ((v + n) > score_.size()) { n = score_.size() - v; }
		for (uint32 x = v + n; x-- != v; ) {
			if (score_[x].pos_ != vars_.end()) {
				vars_.erase(score_[x].pos_);
				score_[x].pos_ = vars_.end();
			}
		}
	}
}

void ClaspVmtf::simplify(const Solver& s, LitVec::size_type i) {
	for (; i < s.numAssignedVars(); ++i) {
		if (score_[s.trail()[i].var()].pos_ != vars_.end()) {
			vars_.erase(score_[s.trail()[i].var()].pos_);
			score_[s.trail()[i].var()].pos_ = vars_.end();
		}
	}
	front_ = vars_.begin();
}

void ClaspVmtf::newConstraint(const Solver& s, const Literal* first, LitVec::size_type size, ConstraintType t) {
	if (t != Constraint_t::static_constraint) {
		LessLevel comp(s, score_);
		const bool upAct = types_.inSet(t);
		const VarVec::size_type mtf = t == Constraint_t::learnt_conflict ? MOVE_TO_FRONT : (MOVE_TO_FRONT*int(upAct))/2;
		for (LitVec::size_type i = 0; i < size; ++i, ++first) {
			Var v = first->var(); 
			score_[v].occ_ += 1 - (((int)first->sign())<<1);
			if (upAct) { ++score_[v].activity(decay_); }
			if (mtf)   {
				if (mtf_.size() < mtf) {
					mtf_.push_back(v);
					std::push_heap(mtf_.begin(), mtf_.end(), comp);
				}
				else if (comp(v, mtf_[0])) {
					assert(s.level(v) <= s.level(mtf_[0]));
					std::pop_heap(mtf_.begin(), mtf_.end(), comp);
					mtf_.back() = v;
					std::push_heap(mtf_.begin(), mtf_.end(), comp);
				}
			}
		}
		for (VarVec::size_type i = 0; i != mtf_.size(); ++i) {
			Var v = mtf_[i];
			if (score_[v].pos_ != vars_.end()) {
				vars_.splice(vars_.begin(), vars_, score_[v].pos_);
			}
		}
		mtf_.clear();
		front_ = vars_.begin();
	} 
}

void ClaspVmtf::undoUntil(const Solver&, LitVec::size_type) {
	front_ = vars_.begin();
}

void ClaspVmtf::updateReason(const Solver& s, const LitVec& lits, Literal r) {
	if (scType_ > 1u) {
		const bool  ms = scType_ == 3u;
		const uint32 D = decay_;
		for (LitVec::size_type i = 0, e = lits.size(); i != e; ++i) {
			if (ms || !s.seen(lits[i])) { ++score_[lits[i].var()].activity(D); }
		}
	}
	if (scType_ & 1u) { ++score_[r.var()].activity(decay_); }
}

bool ClaspVmtf::bump(const Solver&, const WeightLitVec& lits, double adj) {
	for (WeightLitVec::const_iterator it = lits.begin(), end = lits.end(); it != end; ++it) {
		score_[it->first.var()].activity(decay_) += static_cast<uint32>(it->second*adj);
	}
	return true;
}

Literal ClaspVmtf::doSelect(Solver& s) {
	decay_ += ((s.stats.choices + 1) & 511) == 0;
	for (; s.value(*front_) != value_free; ++front_) {;}
	Literal c;
	if (s.numFreeVars() > 1) {
		VarList::iterator v2 = front_;
		uint32 distance = 0;
		do {
			++v2;
			++distance;
		} while (s.value(*v2) != value_free);
		c = (score_[*front_].activity(decay_) + (distance<<1)+ 3) > score_[*v2].activity(decay_)
		    ? selectLiteral(s, *front_, score_[*front_].occ_)
		    : selectLiteral(s, *v2, score_[*v2].occ_);
	}
	else {
		c = selectLiteral(s, *front_, score_[*front_].occ_);
	}
	return c;
}

Literal ClaspVmtf::selectRange(Solver&, const Literal* first, const Literal* last) {
	Literal best = *first;
	for (++first; first != last; ++first) {
		if (score_[first->var()].activity(decay_) > score_[best.var()].activity(decay_)) {
			best = *first;
		}
	}
	return best;
}

/////////////////////////////////////////////////////////////////////////////////////////
// ClaspVsids selection strategy
/////////////////////////////////////////////////////////////////////////////////////////
template <class ScoreType>
ClaspVsids_t<ScoreType>::ClaspVsids_t(double decay, const HeuParams& params) 
	: vars_(CmpScore(score_)) 
	, decay_(1.0 / std::max(0.01, std::min(1.0, decay)))
	, inc_(1.0) {
	scType_  = params.resScore ? params.resScore : 1u;
	uint32 x = params.otherScore + 1;
	if (x & Constraint_t::learnt_loop) { types_.addSet(Constraint_t::learnt_loop); }
	if (x & Constraint_t::learnt_other){ types_.addSet(Constraint_t::learnt_other);}
	if (scType_ == 1u)                 { types_.addSet(Constraint_t::learnt_conflict); }
	if (params.initScore)              { types_.addSet(Constraint_t::static_constraint); }
}

template <class ScoreType>
void ClaspVsids_t<ScoreType>::startInit(const Solver& s) {
	score_.resize(s.numVars()+1);
	occ_.resize(s.numVars()+1);
	vars_.reserve(s.numVars() + 1);
}

template <class ScoreType>
void ClaspVsids_t<ScoreType>::endInit(Solver& s) {
	vars_.clear();
	initScores(s, types_.inSet(Constraint_t::static_constraint));
	for (Var v = 1; v <= s.numVars(); ++v) {
		if (s.value(v) == value_free && !vars_.is_in_queue(v)) {
			vars_.push(v);
 		}
	}
}
template <class ScoreType>
void ClaspVsids_t<ScoreType>::updateVar(const Solver& s, Var v, uint32 n) {
	if (s.validVar(v)) {
		growVecTo(score_, v+n);
		growVecTo(occ_, v+n);
		for (uint32 end = v+n; v != end; ++v) { vars_.update(v); }
	}
	else {
		for (uint32 end = v+n; v != end; ++v) { vars_.remove(v); }
	}
}
template <class ScoreType>
void ClaspVsids_t<ScoreType>::initScores(Solver& s, bool moms) {
	if (!moms) { return; }
	double maxS = 0.0, ms;
	for (Var v = 1; v <= s.numVars(); ++v) {
		if (s.value(v) == value_free && score_[v].get() == 0.0 && (ms = (double)momsScore(s, v)) != 0.0) {
			maxS      = std::max(maxS, ms);
			score_[v].set(-ms);
		}
	}
	for (Var v = 1; v <= s.numVars(); ++v) {
		double d = score_[v].get();
		if (d < 0) {
			d *= -1.0;
			d /= maxS;
			score_[v].set(d);
		}
	}
}
template <class ScoreType>
void ClaspVsids_t<ScoreType>::simplify(const Solver& s, LitVec::size_type i) {
	for (; i < s.numAssignedVars(); ++i) {
		vars_.remove(s.trail()[i].var());
	}
}

template <class ScoreType>
void ClaspVsids_t<ScoreType>::normalize() {
	const double min  = std::numeric_limits<double>::min();
	const double minD = min * 1e100;
	inc_             *= 1e-100;
	for (LitVec::size_type i = 0; i != score_.size(); ++i) {
		double d = score_[i].get();
		if (d > 0) {
			// keep relative ordering but 
			// actively avoid denormals
			d += minD;
			d *= 1e-100;
		}
		score_[i].set(d);
	}
}
template <class ScoreType>
void ClaspVsids_t<ScoreType>::newConstraint(const Solver&, const Literal* first, LitVec::size_type size, ConstraintType t) {
	if (t != Constraint_t::static_constraint) {
		const bool upAct = types_.inSet(t);
		for (LitVec::size_type i = 0; i < size; ++i, ++first) {
			incOcc(*first);
			if (upAct) {
				updateVarActivity(first->var());
			}
		}
		if (t == Constraint_t::learnt_conflict) {
			inc_ *= decay_;
		}
	}
}
template <class ScoreType>
void ClaspVsids_t<ScoreType>::updateReason(const Solver& s, const LitVec& lits, Literal r) {
	if (scType_ > 1u) {
		const bool ms = scType_ == 3u;
		for (LitVec::size_type i = 0, end = lits.size(); i != end; ++i) {
			if (ms || !s.seen(lits[i])) { updateVarActivity(lits[i].var()); }
		}
	}
	if ((scType_ & 1u) != 0 && r.var() != 0) { updateVarActivity(r.var()); }
}
template <class ScoreType>
bool ClaspVsids_t<ScoreType>::bump(const Solver&, const WeightLitVec& lits, double adj) {
	for (WeightLitVec::const_iterator it = lits.begin(), end = lits.end(); it != end; ++it) {
		updateVarActivity(it->first.var(), it->second*adj);
	}
	return true;
}
template <class ScoreType>
void ClaspVsids_t<ScoreType>::undoUntil(const Solver& s , LitVec::size_type st) {
	const LitVec& a = s.trail();
	for (; st < a.size(); ++st) {
		if (!vars_.is_in_queue(a[st].var())) {
			vars_.push(a[st].var());
		}
	}
}
template <class ScoreType>
Literal ClaspVsids_t<ScoreType>::doSelect(Solver& s) {
	while ( s.value(vars_.top()) != value_free ) {
		vars_.pop();
	}
	return selectLiteral(s, vars_.top(), occ(vars_.top()));
}
template <class ScoreType>
Literal ClaspVsids_t<ScoreType>::selectRange(Solver&, const Literal* first, const Literal* last) {
	Literal best = *first;
	for (++first; first != last; ++first) {
		if (vars_.key_compare()(first->var(), best.var())) {
			best = *first;
		}
	}
	return best;
}
template class ClaspVsids_t<VsidsScore>;

/////////////////////////////////////////////////////////////////////////////////////////
// DomainHeuristic selection strategy
/////////////////////////////////////////////////////////////////////////////////////////
const char* const domSym_s = "_heuristic(";
const std::size_t domLen_s = std::strlen(domSym_s);
bool DomEntry::isDomEntry(const SymbolType& sym) { return !sym.name.empty() && std::strncmp(sym.name.c_str(), domSym_s, domLen_s) == 0; }
bool DomEntry::isHeadOf(const char* head, const SymbolType& domSym) {
	assert(isDomEntry(domSym));
	std::size_t len = std::strlen(head);
	const char* dom = domSym.name.c_str() + domLen_s;
	return std::strncmp(head, dom, len) == 0 && dom[len] == ',';
}
bool DomEntry::Cmp::operator()(const SymbolType& lhs, const SymbolType& rhs) const {
	assert(DomEntry::isDomEntry(lhs) && DomEntry::isDomEntry(rhs));
	return std::strcmp(lhs.name.c_str() + domLen_s, rhs.name.c_str() + domLen_s) < 0;
}
bool DomEntry::Lookup::operator()(const char* head, const SymbolType& domSym) const {
	assert(DomEntry::isDomEntry(domSym));
	return std::strncmp(head, domSym.name.c_str() + domLen_s, std::strlen(head)) < 0;
}
bool DomEntry::Lookup::operator()(const SymbolType& domSym, const char* head) const {
	assert(DomEntry::isDomEntry(domSym));
	return std::strncmp(domSym.name.c_str() + domLen_s, head, std::strlen(head)) < 0;
}
void DomEntry::init(const SymbolType& atomSym, const SymbolType& domSym) {
	CLASP_ASSERT_CONTRACT(isDomEntry(domSym));
	std::memset(this, 0, sizeof(*this));
	this->lit = atomSym.lit;
	this->cond= domSym.lit;
	const char* rule = domSym.name.c_str() + domLen_s;
	// skip head
	rule += std::strlen(atomSym.name.c_str());
	CLASP_FAIL_IF(!*rule++, "Invalid atom name in heuristic predicate!");
	// extract modifier
	if      (std::strncmp(rule, "sign"  , 4) == 0) { this->mod = mod_sign;   rule += 4; }
	else if (std::strncmp(rule, "true"  , 4) == 0) { this->mod = mod_tf;     rule += 4; this->pref = trueValue(lit); }
	else if (std::strncmp(rule, "init"  , 4) == 0) { this->mod = mod_init;   rule += 4; }
	else if (std::strncmp(rule, "level" , 5) == 0) { this->mod = mod_level;  rule += 5; }
	else if (std::strncmp(rule, "false" , 5) == 0) { this->mod = mod_tf;     rule += 5; this->pref = falseValue(lit); }
	else if (std::strncmp(rule, "factor", 6) == 0) { this->mod = mod_factor; rule += 6; }
	CLASP_FAIL_IF(*rule++ != ',', "Invalid modifier in heuristic predicate!");
	// extract value
	char* next;
	this->value = Range<int>(INT16_MIN, INT16_MAX).clamp(std::strtol(rule, &next, 10));
	CLASP_FAIL_IF(rule == next || *(rule = next) == 0, "Invalid value in heuristic predicate!");
	this->prio = value > 0 ? value : -value;
	if (mod == mod_sign) {
		pref  = value_free + ValueRep(value != 0) + ValueRep(value < 0);
		if (lit.sign() && pref) { pref ^= 3u; }
		value = pref;
	}
	// extract optional priority
	if (*rule == ',') { // _heuristic/4
		prio = Range<int>(0, INT16_MAX).clamp(std::strtol(++rule, &next, 10));
		CLASP_FAIL_IF(rule == next || *(rule = next) == 0, "Invalid priority in heuristic predicate!");
	}
	CLASP_FAIL_IF(*rule++ != ')' || *rule != 0, "Invalid extra argument in heuristic predicate!");
}
DomainHeuristic::DomainHeuristic(double d, const HeuParams& params) : ClaspVsids_t<DomScore>(d, params), domMin_(0), symSize_(0), defMod_(0), defPref_(0) {
	frames_.push_back(Frame(0,UINT32_MAX));
}
DomainHeuristic::~DomainHeuristic() {
}
void DomainHeuristic::setDefaultMod(GlobalModifier mod, uint32 prefSet) {
	defMod_ = (uint16)mod;
	defPref_= prefSet;
}
void DomainHeuristic::detach(Solver& s) {
	if (!actions_.empty()) {
		for (SymbolTable::const_iterator it = s.symbolTable().begin(), end = s.symbolTable().end(); it != end; ++it) {
			if (it->second.lit.var() && DomEntry::isDomEntry(it->second)) {
				s.removeWatch(it->second.lit, this);
			}
		}
	}
	while (frames_.back().dl != 0) {
		s.removeUndoWatch(frames_.back().dl, this);
		frames_.pop_back();
	}
	actions_.clear();
	prios_.clear();
	symSize_ = 0;
}
void DomainHeuristic::startInit(const Solver& s) {
	BaseType::startInit(s);
	if (symSize_ > s.symbolTable().size()) {
		symSize_ = 0;
	}
}
void DomainHeuristic::initScores(Solver& s, bool moms) {
	if (moms)  { BaseType::initScores(s, moms); }
	if (s.symbolTable().type() != SymbolTable::map_indirect) { return; }
	typedef PodVector<SymbolTable::symbol_type>::type DomTable;
	DomTable domTab;
	// get domain entries from symbol table
	for (SymbolTable::const_iterator sIt = s.symbolTable().begin() + symSize_, end = s.symbolTable().end(); sIt != end; ++sIt) {
		if (DomEntry::isDomEntry(sIt->second)){ domTab.push_back(sIt->second); }
	}
	// prepare table for fast name-based lookup
	std::sort(domTab.begin(), domTab.end(), DomEntry::Cmp());
	// apply modifications to relevant atoms
	Literal dyn;
	uint32 nKey   = (uint32)prios_.size(), uKey = nKey;
	uint32 nRules = (uint32)domTab.size();
	if (&s == s.sharedContext()->master()) { domMin_ = &s.sharedContext()->symbolTable().domLits; }
	typedef PodVector<std::pair<Var, uint32> >::type InitVec;
	InitVec initVals;
	for (SymbolTable::const_iterator sIt = s.symbolTable().begin(), end = s.symbolTable().end(); sIt != end && nRules; ++sIt) {
		if (sIt->second.name.empty() || s.topValue(sIt->second.lit.var()) != value_free) { continue; }
		const char* head = sIt->second.name.c_str();
		Var         ev   = 0;
		int16       init = 0;
		for (DomTable::const_iterator rIt = std::lower_bound(domTab.begin(), domTab.end(), head, DomEntry::Lookup()); rIt != domTab.end() && DomEntry::isHeadOf(head, *rIt); ++rIt, --nRules) {
			DomEntry e;
			e.init(sIt->second, *rIt);
			if (e.mod == DomEntry::mod_init && !s.isTrue(e.cond))   { continue; }
			ev = e.lit.var();
			if (score_[ev].domKey >= nKey){
				score_[ev].setDom(nKey++);
				prios_.push_back(DomPrio());
				prios_.back().clear();
			}
			int mods = 1;
			if (e.mod == DomEntry::mod_tf) { e.mod = DomEntry::mod_level; ++mods; }
			for (;;) {
				if (e.prio >= prio(ev, e.mod)){
					if      (s.isTrue(e.cond)) { prio(ev, e.mod) = e.prio; }
					else if (e.cond != dyn)    { s.addWatch(dyn = e.cond, this, (uint32)actions_.size()); }
					else                       { actions_.back().comp = 1; }
					if (addAction(s, e, init) && score_[ev].domKey >= uKey) {
						uKey = score_[ev].domKey + 1;
					}
				}
				if (--mods == 0) { break; }
				e.mod   = DomEntry::mod_sign;
				e.value = (int16)e.pref;
			}
		}
		if (ev && init) {
			initVals.push_back(std::make_pair(ev, uint32(init)));
		}
	}
	std::sort(initVals.begin(), initVals.end());
	for (InitVec::const_iterator it = initVals.begin(), end = initVals.end(); it != end; ++it) {
		uint32 val = it->second;
		for (InitVec::const_iterator j = it + 1; j != end && j->first == it->first; it = j++) {
			if (j->second > val) { val = j->second; }
		}
		score_[it->first].value += val;
	}
	InitVec().swap(initVals);
	DomTable().swap(domTab);
	if (s.symbolTable().incremental()) { uKey = nKey; }
	if (uKey != nKey) {
		PrioVec(prios_.begin(), prios_.begin()+uKey).swap(prios_);
	}
	// apply default modification
	if (defMod_) {
		MinimizeConstraint* m = s.enumerationConstraint() ? static_cast<EnumerationConstraint*>(s.enumerationConstraint())->minimizer() : 0;
		const uint32 prefSet  = defPref_, graphSet = gpref_scc | gpref_hcc | gpref_disj;
		const uint32 userKey  = prios_.size(), graphKey = userKey + 1, minKey = userKey + 2, showKey = userKey + 3;
		if ((prefSet & gpref_show) != 0) {
			for (SymbolTable::const_iterator it = s.symbolTable().begin() + symSize_, end = s.symbolTable().end(); it != end; ++it) {
				if (!it->second.name.empty() && *it->second.name.c_str() != '_') {
					addDefAction(s, it->second.lit, gpref_show, showKey);
				}
			}
		}
		if ((prefSet & gpref_min) != 0 && m) {
			weight_t w = -1; 
			int16    x = gpref_show;
			for (const WeightLiteral* it = m->shared()->lits; !isSentinel(it->first); ++it) {
				if (x > 4 && it->second != w) {
					--x;
					w = it->second;
				}
				addDefAction(s, it->first, x, minKey);
			}
		}
		if ((prefSet & graphSet) != 0 && s.sharedContext()->sccGraph.get()) {
			for (uint32 i = 0; i !=  s.sharedContext()->sccGraph->numAtoms(); ++i) {
				const SharedDependencyGraph::AtomNode& a = s.sharedContext()->sccGraph->getAtom(i);
				int16 lev = 0;
				if      ((prefSet & gpref_disj) != 0 && a.inDisjunctive()){ lev = 3; }
				else if ((prefSet & gpref_hcc) != 0 && a.inNonHcf())      { lev = 2; }
				else if ((prefSet & gpref_scc) != 0)                      { lev = 1; }
				addDefAction(s, a.lit, lev, graphKey);
			}
		}
		for (Var v = 1, end = s.numVars() + 1; v != end; ++v) {
			if      (!prefSet && (s.varInfo(v).type() & Var_t::atom_var) != 0)    { addDefAction(s, posLit(v), 1, userKey); }
			else if (score_[v].domKey != UINT32_MAX && score_[v].domKey > userKey){ score_[v].setDom(userKey); }
		}
	}
	if (domMin_) {
		LitVec::iterator j = domMin_->begin();
		for (LitVec::const_iterator it = domMin_->begin(), end = domMin_->end(); it != end; ++it) {
			if (s.value(it->var()) == value_free && score_[it->var()].level > 0 && !s.seen(*it)) {
				s.markSeen(*it);
				*j++ = *it;
			}
		}
		domMin_->erase(j, domMin_->end());
		for (LitVec::const_iterator it = domMin_->begin(), end = domMin_->end(); it != end; ++it) { s.clearSeen(it->var()); }
		domMin_ = 0;
	}
	symSize_ = s.symbolTable().size();
}
bool DomainHeuristic::addAction(Solver& s, const DomEntry& e, int16& init) {
	assert(e.mod < DomEntry::mod_tf);
	Var domVar = e.lit.var();
	if (s.isTrue(e.cond)) {
		if      (e.mod == DomEntry::mod_factor){ score_[domVar].factor= e.value; }
		else if (e.mod == DomEntry::mod_level) { score_[domVar].level = e.value; }
		else if (e.mod == DomEntry::mod_init)  { init = e.value; }
		else if (e.mod == DomEntry::mod_sign)  { 
			s.setPref(domVar, ValueSet::user_value, e.pref);
			if (domMin_ && e.pref) { domMin_->push_back(e.pref == falseValue(e.lit) ? e.lit : ~e.lit); }
		}
		return false;
	}
	else {
		DomAction a = { domVar, (uint32)e.mod, uint32(0), UINT32_MAX, e.value, e.prio };
		if (e.mod == DomEntry::mod_sign) { a.val = (int16)e.pref; }
		actions_.push_back(a);
		return true;
	}
}
void DomainHeuristic::addDefAction(Solver& s, Literal x, int16 lev, uint32 domKey) {
	if (s.value(x.var()) != value_free || score_[x.var()].domKey < domKey) { return; }
	const uint16 signMod = gmod_spos|gmod_sneg;
	if (score_[x.var()].domKey > domKey && (defMod_ & gmod_level) != 0){
		score_[x.var()].level += lev;
	}
	if ((defMod_ & signMod) != 0) {
		if (!s.pref(x.var()).has(ValueSet::user_value)) {
			s.setPref(x.var(), ValueSet::user_value, (defMod_ & gmod_spos) != 0 ? trueValue(x) : falseValue(x));
		}
		if (domMin_) { domMin_->push_back((defMod_ & gmod_spos) != 0 ? ~x : x); }
	}
	score_[x.var()].setDom(domKey);
}

Literal DomainHeuristic::doSelect(Solver& s) {
	Literal x = BaseType::doSelect(s);
	s.stats.addDomChoice(score_[x.var()].isDom());
	return x;
}

Constraint::PropResult DomainHeuristic::propagate(Solver& s, Literal, uint32& aId) {
	uint32 n = aId;
	do {
		DomAction& a = actions_[n];
		uint16& prio = this->prio(a.var, a.mod);
		if (s.value(a.var) == value_free && a.prio >= prio) {
			applyAction(s, a, prio);
			pushUndo(s, n);
		}
	} while (actions_[n++].comp);
	return PropResult(true, true);
}

void DomainHeuristic::applyAction(Solver& s, DomAction& a, uint16& gPrio) {
	std::swap(gPrio, a.prio);
	switch (a.mod) {
		default: assert(false); break;
		case DomEntry::mod_factor: std::swap(score_[a.var].factor, a.val); break;
		case DomEntry::mod_level:
			std::swap(score_[a.var].level, a.val);
			if (vars_.is_in_queue(a.var)) { vars_.update(a.var); } 
			break;
		case DomEntry::mod_sign:
			ValueSet old = s.pref(a.var);
			s.setPref(a.var, ValueSet::user_value, ValueRep(a.val));
			a.val        = old.get(ValueSet::user_value);
			break;
	}
}
void DomainHeuristic::pushUndo(Solver& s, uint32 actionId) {
	if (s.decisionLevel() != frames_.back().dl) {
		frames_.push_back(Frame(s.decisionLevel(), UINT32_MAX));
		s.addUndoWatch(s.decisionLevel(), this);
	}
	actions_[actionId].undo = frames_.back().head;
	frames_.back().head     = actionId;
}

void DomainHeuristic::undoLevel(Solver& s) {
	while (frames_.back().dl >= s.decisionLevel()) {
		for (uint32 n = frames_.back().head; n != UINT32_MAX;) {
			DomAction& a = actions_[n];
			n            = a.undo;
			applyAction(s, a, prio(a.var, a.mod));
		}
		frames_.pop_back();
	}
}
template class ClaspVsids_t<DomScore>;
}
