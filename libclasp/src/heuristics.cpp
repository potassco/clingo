// 
// Copyright (c) 2006-2007, Benjamin Kaufmann
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
#include <algorithm>
namespace Clasp { namespace {

// our own rng for heuristics
uint32 heuRand_g = 1;
inline uint32 heuRand() {
	return( ((heuRand_g = heuRand_g * 214013L + 2531011L) >> 16) & 0x7fff );
}
}
/////////////////////////////////////////////////////////////////////////////////////////
// Lookback selection strategies
/////////////////////////////////////////////////////////////////////////////////////////
uint32 momsScore(const Solver& s, Var v) {
	uint32 sc;
	if (s.numBinaryConstraints()) {
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

ClaspBerkmin::ClaspBerkmin(uint32 maxB, bool loops, bool moms, bool huang) 
	: order_(huang, moms, loops)
	, topConflict_(UINT32_MAX)
	, topLoop_(UINT32_MAX)
	, front_(1) 
	, cacheSize_(5)
	, numVsids_(0)
	, maxBerkmin_(maxB == 0 ? UINT32_MAX : maxB) {
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
	for (v = cs == cacheSize_ ? v+1 : s.numVars()+1; v <= s.numVars(); ++v) {
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
	for (Var v = var+1; v <= s.numVars(); ++v) {
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

void ClaspBerkmin::Order::init(const Solver& s) {
	if (reinit) { score.clear(); }
	score.resize(s.numVars()+1);
}

void ClaspBerkmin::startInit(const Solver& s) {
	order_.init(s);
	initHuang(order_.huang);

	cache_.clear();
	cacheSize_ = 5;
	cacheFront_ = cache_.end();
	
	freeLits_.clear();
	freeLoopLits_.clear();
	topConflict_ = topLoop_ = (uint32)-1;
	
	front_ = 1;
	order_.decay = numVsids_ = 0;
}

void ClaspBerkmin::endInit(Solver& s) {
	if (initHuang()) {
		const bool clearScore = order_.moms;
		// initialize preferred values of vars
		cache_.clear();
		for (Var v = 1; v <= s.numVars(); ++v) {
			order_.decayedScore(v);
			if (order_.occ(v) != 0) {
				s.initSavedValue(v, order_.occ(v) > 0 ? value_true : value_false);
			}
			if   (clearScore) order_.score[v] = HScore(order_.decay);
			else cache_.push_back(v);
		}
		initHuang(false);
	}
	if (!order_.moms || s.numFreeVars() > BERK_MAX_MOMS_VARS) {
		hasActivities(true);
	}
	std::stable_sort(cache_.begin(), cache_.end(), Order::Compare(&order_));
	cacheFront_   = cache_.begin();
}

void ClaspBerkmin::newConstraint(const Solver&, const Literal* first, LitVec::size_type size, ConstraintType t) {
	if (order_.huang == (t == Constraint_t::static_constraint)) {
		for (; size--; ++first) { order_.incOcc(*first); }
	}
	if (t == Constraint_t::learnt_conflict) {
		hasActivities(true);
	}
}

void ClaspBerkmin::updateReason(const Solver&, const LitVec& lits, Literal r) {
	for (LitVec::size_type i = 0, e = lits.size(); i != e; ++i) {
		order_.inc(~lits[i]);
	}
	if (!isSentinel(r)) { order_.inc(r); }
}

void ClaspBerkmin::undoUntil(const Solver&, LitVec::size_type) {
	topConflict_ = topLoop_ = static_cast<uint32>(-1);
	front_ = 1;
	cache_.clear();
	cacheFront_ = cache_.end();
	if (cacheSize_ > 5 && numVsids_ > 0 && (numVsids_*3) < cacheSize_) {
		cacheSize_ = static_cast<uint32>(cacheSize_/BERK_CACHE_GROW);
	}
	numVsids_ = 0;
}

bool ClaspBerkmin::hasTopUnsat(Solver& s, uint32& maxIdx, uint32 minIdx, ConstraintType t) {
	while (maxIdx != minIdx) {
		const LearntConstraint& c = s.getLearnt(maxIdx-1);
		if (c.type() == t && !c.isSatisfied(s, freeLits_)) {
			return true;
		}
		--maxIdx;
		freeLits_.clear();
	}
	return false;
}

bool ClaspBerkmin::hasTopUnsat(Solver& s) {
	topConflict_  = std::min(s.numLearntConstraints(), topConflict_);
	topLoop_      = std::min(s.numLearntConstraints(), topLoop_);
	freeLoopLits_.clear();
	freeLits_.clear();
	if (topConflict_ > topLoop_ && hasTopUnsat(s, topConflict_, topLoop_, Constraint_t::learnt_conflict)) {
		return true;
	}
	if (order_.loops && topLoop_ > topConflict_ && hasTopUnsat(s, topLoop_, topConflict_, Constraint_t::learnt_loop)) {
		freeLits_.swap(freeLoopLits_);
	}
	uint32 stopAt = topConflict_ < maxBerkmin_ ? 0 : topConflict_ - maxBerkmin_;
	while (topConflict_ != stopAt) {
		const LearntConstraint& c = s.getLearnt(topConflict_-1);
		if (c.type() == Constraint_t::learnt_conflict && !c.isSatisfied(s, freeLits_)) {
			break;
		}
		else if (order_.loops && freeLoopLits_.empty() && c.type() == Constraint_t::learnt_loop && !c.isSatisfied(s, freeLits_)) {
			topLoop_  = topConflict_;
			freeLits_.swap(freeLoopLits_);
		}
		--topConflict_;
		freeLits_.clear();
	}
	if (freeLoopLits_.empty())  topLoop_ = topConflict_;
	if (freeLits_.empty())      freeLoopLits_.swap(freeLits_);
	return !freeLits_.empty();
}

Literal ClaspBerkmin::doSelect(Solver& s) {
	const uint32 decayMask = order_.huang ? 127 : 511;
	if ( ((s.stats.solve.choices + 1)&decayMask) == 0 ) {
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
	return c == 1 ? candidates[0] : candidates[heuRand()%c];
}

Literal ClaspBerkmin::selectLiteral(Solver& s, Var var, bool vsids) {
	Literal l;
	if ( (l = savedLiteral(s,var)) == posLit(0) ) {
		int32 w0 = vsids ? (int32)s.estimateBCP(posLit(var), 5) : order_.occ(var);
		int32 w1 = vsids ? (int32)s.estimateBCP(negLit(var), 5) : 0;
		if (w1 == 1 && w0 == w1) {
			// no binary bcp - use occurrences
			w0 = order_.occ(var);
			w1 = 0;
		}
		return w0 != w1 
			? Literal(var, (w0-w1)<0)
			: s.preferredLiteralByType(var);
	}
	else if (order_.huang && (order_.occ(var)*(-1+(2*l.sign()))) > 32) {
		l = ~l;
	}
	return l;
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
ClaspVmtf::ClaspVmtf(LitVec::size_type mtf, bool loops) : MOVE_TO_FRONT(mtf), loops_(loops), reinit_(true) { 
}


void ClaspVmtf::startInit(const Solver& s) {
	if (reinit_) { score_.clear(); vars_.clear(); }
	score_.resize(s.numVars()+1, VarInfo(vars_.end()));
}

void ClaspVmtf::endInit(Solver& s) {
	if (reinit_) vars_.clear();
	bool moms = reinit_ || score_[0].activity_ == 0;
	for (Var v = 1; v <= s.numVars(); ++v) {
		if (score_[v].pos_ == vars_.end() && s.finalValue(v) == value_free) {
			if (moms) {
				score_[v].activity_ = momsScore(s, v);
			}
			score_[v].pos_ = vars_.insert(vars_.end(), v);
		}
	}
	if (moms) {
		vars_.sort(LessLevel(s, score_));
		for (VarList::iterator it = vars_.begin(); it != vars_.end(); ++it) {
			score_[*it].activity_ = 0;
		}
	}
	front_ = vars_.begin();
	decay_ = 0;
}

void ClaspVmtf::simplify(const Solver& s, LitVec::size_type i) {
	for (; i < s.numAssignedVars(); ++i) {
		if (score_[s.assignment()[i].var()].pos_ != vars_.end()) {
			vars_.erase(score_[s.assignment()[i].var()].pos_);
			score_[s.assignment()[i].var()].pos_ = vars_.end();
		}
	}
	front_ = vars_.begin();
}

void ClaspVmtf::newConstraint(const Solver& s, const Literal* first, LitVec::size_type size, ConstraintType t) {
	if (t != Constraint_t::static_constraint) {
		LessLevel comp(s, score_);
		VarVec::size_type maxMove = t == Constraint_t::learnt_conflict ? MOVE_TO_FRONT : MOVE_TO_FRONT/2;
		for (LitVec::size_type i = 0; i < size; ++i, ++first) {
			Var v = first->var(); 
			score_[v].occ_ += 1 - (((int)first->sign())<<1);
			if (t == Constraint_t::learnt_conflict || loops_) {
				++score_[v].activity(decay_);
				if (mtf_.size() < maxMove) {
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

Literal ClaspVmtf::getLiteral(const Solver& s, Var v) const {
	Literal r;
	if ( (r = savedLiteral(s, v)) == posLit(0) ) {
		r = score_[v].occ_== 0
			? s.preferredLiteralByType(v)
			: Literal(v, score_[v].occ_ < 0 );
	}
	return r;
}

void ClaspVmtf::undoUntil(const Solver&, LitVec::size_type) {
	front_ = vars_.begin();
}

void ClaspVmtf::updateReason(const Solver&, const LitVec&, Literal r) {
	++score_[r.var()].activity(decay_);
}

Literal ClaspVmtf::doSelect(Solver& s) {
	decay_ += ((s.stats.solve.choices + 1) & 511) == 0;
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
				? getLiteral(s, *front_)
				: getLiteral(s, *v2);
	}
	else {
		c = getLiteral(s, *front_);
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
const double vsidsVarDecay_g = 1 / 0.95;

ClaspVsids::ClaspVsids(bool loops) 
	: vars_(GreaterActivity(score_)) 
	, inc_(1.0)
	, scoreLoops_(loops) 
	, reinit_(true) {}

void ClaspVsids::startInit(const Solver& s) {
	if (reinit_)score_.clear();
	score_.resize(s.numVars()+1);
}
void ClaspVsids::endInit(Solver& s) {
	if (reinit_) vars_.clear();
	bool moms = vars_.empty();
	inc_ = 1.0;
	double maxS = 0.0;
	for (Var v = 1; v <= s.numVars(); ++v) {
		if (s.value(v) == value_free && (reinit_ || !vars_.is_in_queue(v))) {
			if (moms) {
				// initialize activity to moms-score
				score_[v].first = momsScore(s, v);
				if (score_[v].first > maxS) {
					maxS = score_[v].first;
				}
			}
			vars_.push(v);
		}
	}
	if (moms) {
		for (VarVec::size_type i = 0; i != score_.size(); ++i) {
			score_[i].first /= maxS;
		}
	}
}

void ClaspVsids::simplify(const Solver& s, LitVec::size_type i) {
	for (; i < s.numAssignedVars(); ++i) {
		vars_.remove(s.assignment()[i].var());
	}
}

void ClaspVsids::newConstraint(const Solver&, const Literal* first, LitVec::size_type size, ConstraintType t) {
	if (t != Constraint_t::static_constraint) {
		for (LitVec::size_type i = 0; i < size; ++i, ++first) {
			score_[first->var()].second += 1 - (first->sign() + first->sign());
			if (t == Constraint_t::learnt_conflict || scoreLoops_) {
				updateVarActivity(first->var());
			}
		}
		if (t == Constraint_t::learnt_conflict) {
			inc_ *= vsidsVarDecay_g;
		}
	}
}

void ClaspVsids::updateReason(const Solver&, const LitVec&, Literal r) {
	if (r.var() != 0) updateVarActivity(r.var());
}

void ClaspVsids::undoUntil(const Solver& s , LitVec::size_type st) {
	const LitVec& a = s.assignment();
	for (; st < a.size(); ++st) {
		if (!vars_.is_in_queue(a[st].var())) {
			vars_.push(a[st].var());
		}
	}
}

Literal ClaspVsids::doSelect(Solver& s) {
	Var var;
	while ( s.value(vars_.top()) != value_free ) {
		vars_.pop();
	}
	var = vars_.top();
	Literal r;
	if ( (r = savedLiteral(s, var)) == posLit(0) ) {
		r = score_[var].second == 0
			? s.preferredLiteralByType(var)
			: Literal( var, score_[var].second < 0 );
	}
	return r;
}

Literal ClaspVsids::selectRange(Solver&, const Literal* first, const Literal* last) {
	Literal best = *first;
	for (++first; first != last; ++first) {
		if (score_[first->var()].first > score_[best.var()].first) {
			best = *first;
		}
	}
	return best;
}
}
