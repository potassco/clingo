// 
// Copyright (c) 2010-2012, Benjamin Kaufmann
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
#include <clasp/minimize_constraint.h>
#include <clasp/solver.h>
#include <clasp/weight_constraint.h>
#include <clasp/clause.h>
#include <stdio.h>
namespace Clasp { 
/////////////////////////////////////////////////////////////////////////////////////////
// SharedMinimizeData
/////////////////////////////////////////////////////////////////////////////////////////
SharedMinimizeData::SharedMinimizeData(const SumVec& lhsAdjust, MinimizeMode m) : mode_(m) {
	adjust_  = lhsAdjust;
	count_   = 1;
	resetBounds();
	setMode(MinimizeMode_t::optimize);
}
SharedMinimizeData::~SharedMinimizeData() {}

void SharedMinimizeData::destroy() const {
	this->~SharedMinimizeData();
	::operator delete(const_cast<SharedMinimizeData*>(this));
}

void SharedMinimizeData::resetBounds() {
	gCount_  = 0;
	optGen_  = 0;
	lower_.assign(numRules(), 0);
	up_[0].assign(numRules(), maxBound());
	up_[1].assign(numRules(), maxBound());
	const WeightLiteral* lit = lits;
	for (weight_t wPos = 0, end = (weight_t)weights.size(), x; wPos != end; wPos = x+1) {
		assert(weights[wPos].weight >= 0);
		for (x = wPos; weights[x].next; ) { // compound weight - check for negative
			if (weights[++x].weight < 0) {
				while (lit->second != wPos) { ++lit; }
				for (const WeightLiteral* t = lit; t->second == wPos; ++t) {
					lower_[weights[x].level] += weights[x].weight;
				}
			}
		}
	}
}

bool SharedMinimizeData::setMode(MinimizeMode m, const wsum_t* bound, uint32 boundSize)  {
	mode_ = m;
	if (boundSize && bound) {
		SumVec& opt = up_[0];
		bool    ok  = false;
		gCount_     = 0;
		optGen_     = 0;
		boundSize   = std::min(boundSize, numRules());
		for (uint32 i = 0, end = boundSize; i != end; ++i) {
			wsum_t B = bound[i], a = adjust(i);
			B        = a >= 0 || (maxBound() + a) >= B ? B - a : maxBound();
			wsum_t d = B - lower_[i];
			if (d < 0 && !ok) { return false; }
			opt[i]   = B;
			ok       = ok || d > 0;
		}
		for (uint32 i = boundSize, end = (uint32)opt.size(); i != end; ++i) { opt[i] = maxBound(); }
	}
	return true;
}

MinimizeConstraint* SharedMinimizeData::attach(Solver& s, MinimizeMode_t::Strategy strat, uint32 param, bool addRef) {
	if (addRef) this->share();
	MinimizeConstraint* ret;
	if (strat == MinimizeMode_t::opt_bb || mode() == MinimizeMode_t::enumerate) {
		ret = new DefaultMinimize(this, param);
	}
	else {
		ret = new UncoreMinimize(this, param);
	}
	ret->attach(s);
	return ret;
}

const SumVec* SharedMinimizeData::setOptimum(const wsum_t* newOpt) {
	if (optGen_) { return up_ + (optGen_&1u); }
	uint32  g = gCount_;
	uint32  n = 1u - (g & 1u);
	SumVec& U = up_[n];
	U.assign(newOpt, newOpt + numRules());
	assert(mode() != MinimizeMode_t::enumerate || n == 1);
	if (mode() != MinimizeMode_t::enumerate) {
		if (++g == 0) { g = 2; }
		gCount_  = g;
	}
	return &U;
}
void SharedMinimizeData::setLower(uint32 lev, wsum_t low) {
	lower_[lev] = low;
}
void SharedMinimizeData::markOptimal() {
	optGen_ = generation();
}
void SharedMinimizeData::sub(wsum_t* lhs, const LevelWeight* w, uint32& aLev) const {
	if (w->level < aLev) { aLev = w->level; }
	do { lhs[w->level] -= w->weight; } while (w++->next);
}
bool SharedMinimizeData::imp(wsum_t* lhs, const LevelWeight* w, const wsum_t* rhs, uint32& lev) const {
	assert(lev <= w->level && std::equal(lhs, lhs+lev, rhs));
	while (lev != w->level && lhs[lev] == rhs[lev]) { ++lev; }
	for (uint32 i = lev, end = numRules(); i != end; ++i) {
		wsum_t temp = lhs[i];
		if (i == w->level) { temp += w->weight; if (w->next) ++w; }
		if (temp != rhs[i]){ return temp > rhs[i]; }
	}
	return false;
}
/////////////////////////////////////////////////////////////////////////////////////////
// MinimizeConstraint
/////////////////////////////////////////////////////////////////////////////////////////
MinimizeConstraint::MinimizeConstraint(SharedData* d) : shared_(d) {}

MinimizeConstraint::~MinimizeConstraint() {
	assert(shared_ == 0 && "MinimizeConstraint not destroyed!");
}
bool MinimizeConstraint::prepare(Solver& s, bool useTag) {
	CLASP_ASSERT_CONTRACT_MSG(!s.isFalse(tag_), "Tag literal must not be false!");
	if (useTag && tag_ == posLit(0))      { tag_ = posLit(s.pushTagVar(false)); }
	if (s.isTrue(tag_) || s.hasConflict()){ return !s.hasConflict(); }
	return useTag ? s.pushRoot(tag_) : s.force(tag_, 0);
}
void MinimizeConstraint::destroy(Solver* s, bool d) {
	shared_->release();
	shared_ = 0;
	Constraint::destroy(s, d);
}
/////////////////////////////////////////////////////////////////////////////////////////
// DefaultMinimize
/////////////////////////////////////////////////////////////////////////////////////////
union DefaultMinimize::UndoInfo {
	UndoInfo() : rep(0) {}
	struct {
		uint32 idx    : 30; // index of literal on stack
		uint32 newDL  :  1; // first literal of new DL?
		uint32 idxSeen:  1; // literal with idx already propagated?
	}      data;
	uint32 rep;
	uint32 index() const { return data.idx; }
	bool   newDL() const { return data.newDL != 0u; }
};
DefaultMinimize::DefaultMinimize(SharedData* d, uint32 strat)
	: MinimizeConstraint(d)
	, bounds_(0)
	, pos_(d->lits)
	, undo_(0)
	, undoTop_(0)
	, size_(d->numRules()) {
	step_.type = strat & 3u;
	if (step_.type == MinimizeMode_t::bb_step_hier && d->numRules() == 1) {
		step_.type = 0;
	}
}

DefaultMinimize::~DefaultMinimize() {
	delete [] bounds_;
	delete [] undo_;
}

void DefaultMinimize::destroy(Solver* s, bool detach) {
	if (s && detach) {
		for (const WeightLiteral* it = shared_->lits; !isSentinel(it->first); ++it) {
			s->removeWatch(it->first, this);
		}
		for (uint32 dl = 0; (dl = lastUndoLevel(*s)) != 0; ) {
			s->removeUndoWatch(dl, this);
			DefaultMinimize::undoLevel(*s);
		}
	}
	MinimizeConstraint::destroy(s, detach);
}

bool DefaultMinimize::attach(Solver& s) {
	assert(s.decisionLevel() == 0 && !bounds_);
	uint32 numL = 0;
	VarVec up;
	for (const WeightLiteral* it = shared_->lits; !isSentinel(it->first); ++it, ++numL) {
		if (s.value(it->first.var()) == value_free) {
			s.addWatch(it->first, this, numL);
		}
		else if (s.isTrue(it->first)) {
			up.push_back(numL);
		}
	}
	bounds_ = new wsum_t[(numRules() * (3 + uint32(step_.type != 0)))]; // upper, sum, temp, lower
	std::fill(this->opt(), this->sum(), SharedData::maxBound());
	std::fill(this->sum(), this->end(), wsum_t(0));
	stepInit(0);
	// [0,numL+1)      : undo stack
	// [numL+1, numL*2): pos  stack
	undo_    = new UndoInfo[(numL*2)+1];
	undoTop_ = 0;
	posTop_  = numL+1;
	actLev_  = 0;
	for (WeightVec::size_type i = 0; i != up.size(); ++i) {
		DefaultMinimize::propagate(s, shared_->lits[up[i]].first, up[i]);
	}
	return true;
}

// Returns the numerical highest decision level watched by this constraint.
uint32 DefaultMinimize::lastUndoLevel(const Solver& s) const {
	return undoTop_ != 0
		? s.level(shared_->lits[undo_[undoTop_-1].index()].first.var())
		: 0;
}
bool DefaultMinimize::litSeen(uint32 i) const { return undo_[i].data.idxSeen != 0; }

// Pushes the literal at index idx onto the undo stack
// and marks it as seen; if literal is first in current level
// adds a new undo watch.
void DefaultMinimize::pushUndo(Solver& s, uint32 idx) {
	assert(idx >= static_cast<uint32>(pos_ - shared_->lits));
	undo_[undoTop_].data.idx  = idx;
	undo_[undoTop_].data.newDL= 0;
	if (lastUndoLevel(s) != s.decisionLevel()) {
		// remember current "look at" position and start
		// a new decision level on the undo stack
		undo_[posTop_++].data.idx = static_cast<uint32>(pos_-shared_->lits);
		s.addUndoWatch(s.decisionLevel(), this);
		undo_[undoTop_].data.newDL = 1;
	}
	undo_[idx].data.idxSeen   = 1;
	++undoTop_;
}
/////////////////////////////////////////////////////////////////////////////////////////
// MinimizeConstraint - arithmetic strategy implementation
//
// For now we use a simple "switch-on-type" approach. 
// In the future, if new strategies emerge, we may want to use a strategy hierarchy.
/////////////////////////////////////////////////////////////////////////////////////////
#define STRATEGY(x) shared_->x
// set *lhs = *rhs, where lhs != rhs
void DefaultMinimize::assign(wsum_t* lhs, wsum_t* rhs) const {
	std::memcpy(lhs, rhs, size_*sizeof(wsum_t));
}
// returns lhs > rhs
bool DefaultMinimize::greater(wsum_t* lhs, wsum_t* rhs, uint32 len, uint32& aLev) const {
	while (*lhs == *rhs && --len) { ++lhs, ++rhs; ++aLev; }
	return *lhs > *rhs;
}
/////////////////////////////////////////////////////////////////////////////////////////
Constraint::PropResult DefaultMinimize::propagate(Solver& s, Literal, uint32& data) {
	pushUndo(s, data);
	STRATEGY(add(sum(), shared_->lits[data]));
	return PropResult(propagateImpl(s, propagate_new_sum), true);
}

// Computes the set of literals implying p and returns 
// the highest decision level of that set.
// PRE: p is implied on highest undo level
uint32 DefaultMinimize::computeImplicationSet(const Solver& s, const WeightLiteral& p, uint32& undoPos) {
	wsum_t* temp    = this->temp(), *opt = this->opt();
	uint32 up       = undoTop_, lev = actLev_;
	uint32 minLevel = std::max(s.level(tag_.var()), s.level(s.sharedContext()->stepLiteral().var()));
	// start from current sum
	assign(temp, sum());
	// start with full set
	for (UndoInfo u; up != 0; --up) {
		u = undo_[up-1];
		// subtract last element from set
		STRATEGY(sub(temp, shared_->lits[u.index()], lev));
		if (!STRATEGY(imp(temp, p, opt, lev))) {
			// p is no longer implied after we removed last literal,
			// hence [0, up) implies p @ level of last literal
			undoPos = up;
			return std::max(s.level(shared_->lits[u.index()].first.var()), minLevel);
		}
	}
	undoPos = 0; 
	return minLevel;
}

bool DefaultMinimize::propagateImpl(Solver& s, PropMode m) {
	Iter it    = pos_;
	uint32 idx = static_cast<uint32>(it - shared_->lits);
	uint32 DL  = s.decisionLevel();
	// current implication level or "unknown" if
	// we propagate a new optimum 
	uint32 impLevel = DL + (m == propagate_new_opt);
	weight_t lastW  = -1;
	uint32 undoPos  = undoTop_;
	bool ok         = true;
	actLev_         = std::min(actLev_, shared_->level(idx));
	for (wsum_t* sum= this->sum(), *opt = this->opt(); ok && !isSentinel(it->first); ++it, ++idx) {
		// skip propagated/false literals
		if (litSeen(idx) || (m == propagate_new_sum && s.isFalse(it->first))) {
			continue;
		}
		if (lastW != it->second) {
			// check if the current weight is implied
			if (!STRATEGY(imp(sum, *it, opt, actLev_))) {
				// all good - current optimum is safe
				pos_    = it;
				return true;
			}
			// compute implication set and level of current weight
			if (m == propagate_new_opt) {
				impLevel = computeImplicationSet(s, *it, undoPos);
			}
			lastW = it->second;
		}
		assert(active());
		// force implied literals
		if (!s.isFalse(it->first) || (impLevel < DL && s.level(it->first.var()) > impLevel)) {
			if (impLevel != DL) { DL = s.undoUntil(impLevel, Solver::undo_pop_bt_level); }
			ok = s.force(~it->first, impLevel, this, undoPos);
		}
	}
	return ok;
}

// pops free literals from the undo stack and decreases current sum
void DefaultMinimize::undoLevel(Solver&) {
	assert(undoTop_ != 0 && posTop_ > undoTop_);
	uint32 up  = undoTop_;
	uint32 idx = undo_[--posTop_].index();
	for (wsum_t* sum = this->sum();;) {
		UndoInfo& u = undo_[--up];
		undo_[u.index()].data.idxSeen = 0;
		STRATEGY(sub(sum, shared_->lits[u.index()], actLev_));
		if (u.newDL()) { break; }
	}
	undoTop_ = up;
	Iter temp= shared_->lits + idx;
	if (temp < pos_) {
		pos_    = temp;
		actLev_ = std::min(actLev_, shared_->level(idx));
	}
}

// computes the reason for p - 
// all literals that were propagated before p
void DefaultMinimize::reason(Solver& s, Literal p, LitVec& lits) {
	assert(s.isTrue(tag_));
	uint32 stop = s.reasonData(p);
	Literal   x = s.sharedContext()->stepLiteral();
	assert(stop <= undoTop_);
	if (!isSentinel(x) && s.isTrue(x)) { lits.push_back(x); }
	if (s.level(tag_.var()))           { lits.push_back(tag_); }
	for (uint32 i = 0; i != stop; ++i) {
		UndoInfo u = undo_[i];
		x = shared_->lits[u.index()].first;
		lits.push_back(x);
	}
}

bool DefaultMinimize::minimize(Solver& s, Literal p, CCMinRecursive* rec) {
	assert(s.isTrue(tag_));
	uint32 stop = s.reasonData(p);
	Literal   x = s.sharedContext()->stepLiteral();
	assert(stop <= undoTop_);
	if (!s.ccMinimize(x, rec) || !s.ccMinimize(tag_, rec)) { return false; }
	for (uint32 i = 0; i != stop; ++i) {
		UndoInfo u = undo_[i];
		x = shared_->lits[u.index()].first;
		if (!s.ccMinimize(x, rec)) {
			return false;
		}
	}
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////
// DefaultMinimize - bound management
/////////////////////////////////////////////////////////////////////////////////////////
// Stores the current sum as the shared optimum.
void DefaultMinimize::commitUpperBound(const Solver&)  {
	shared_->setOptimum(sum());
	if (step_.type == MinimizeMode_t::bb_step_inc) { step_.size *= 2; }
}
bool DefaultMinimize::commitLowerBound(const Solver&, bool upShared) {
	bool act   = active() && shared_->checkNext();
	bool more  = step_.lev < size_ && (step_.size > 1 || step_.lev != size_-1);
	if (act && step_.type && step_.lev < size_) {
		uint32 x = step_.lev;
		wsum_t L = opt()[x] + 1;
		stepLow()= L;
		while (upShared && shared_->lower(x) < L) { shared_->setLower(x, L); }
		if (step_.type == MinimizeMode_t::bb_step_inc){ step_.size = 1; }
	}
	return more;
}
bool DefaultMinimize::handleUnsat(Solver& s, bool up, LitVec& out) {
	bool more = shared_->optimize() && commitLowerBound(s, up);
	uint32 dl = s.isTrue(tag_) ? s.level(tag_.var()) : 0;
	relaxBound(false);
	if (more && dl && dl <= s.rootLevel()) {
		s.popRootLevel(s.rootLevel()-dl, &out); // pop and remember new path
		return s.popRootLevel(1);               // pop tag - disable constraint
	}
	return false;
}

// Disables the minimize constraint by clearing its upper bound.
bool DefaultMinimize::relaxBound(bool full) {
	if (active()) { std::fill(opt(), opt()+size_, SharedData::maxBound()); }
	pos_       = shared_->lits;
	actLev_    = 0;
	if (full || !shared_->optimize()) { stepInit(0); }
	return true;
}

void DefaultMinimize::stepInit(uint32 n) {
	step_.size = uint32(step_.type != MinimizeMode_t::bb_step_dec);
	if (step_.type) { step_.lev = n; if (n != size_) stepLow() = 0-shared_->maxBound(); }
	else            { step_.lev = shared_->maxLevel(); }
}

// Integrates new (tentative) bounds from the ones stored in the shared data object.
bool DefaultMinimize::integrateBound(Solver& s) {
	bool useTag = shared_->optimize() && (step_.type != 0 || shared_->mode() == MinimizeMode_t::enumOpt);
	if (!prepare(s, useTag)) { return false; }
	if (useTag && s.level(tag_.var()) == 0) {
		step_.type = 0;
		stepInit(0);
	}
	if ((active() && !shared_->checkNext())) { return !s.hasConflict(); }
	WeightLiteral min(posLit(0), shared_->weights.empty() ? uint32(0) : (uint32)shared_->weights.size()-1);
	while (!s.hasConflict() && updateBounds(shared_->checkNext())) {
		uint32 x = 0;
		uint32 dl= s.decisionLevel() + 1;
		if (!STRATEGY(imp(sum(), min, opt(), actLev_)) || (dl = computeImplicationSet(s, min, x)) > s.rootLevel()) {
			for (--dl; !s.hasConflict() || s.resolveConflict(); ) {
				if      (s.undoUntil(dl, Solver::undo_pop_bt_level) > dl){ s.backtrack(); }
				else if (propagateImpl(s, propagate_new_opt))            { return true;   }
			}
		}
		if (!shared_->checkNext()) {
			break;
		}
		if (!step_.type) { ++step_.lev; }
		else             { stepLow() = ++opt()[step_.lev]; }
	}
	relaxBound(false);
	if (!s.hasConflict()) { 
		s.undoUntil(0);
		s.setStopConflict();
	}
	return false;
}

// Loads bounds from the shared data object and (optionally) applies
// the current step to get the next tentative bound to check.
bool DefaultMinimize::updateBounds(bool applyStep) {
	for (;;) {
		const uint32  seq   = shared_->generation();
		const wsum_t* upper = shared_->upper();
		const wsum_t* myLow = step_.type ? end() : shared_->lower();
		const wsum_t* sLow  = shared_->lower();
		wsum_t*       bound = opt();
		uint32        appLev= applyStep ? step_.lev : size_;
		for (uint32 i = 0; i != size_; ++i) {
			wsum_t U = upper[i], B = bound[i];
			if (i != appLev) {
				if (myLow != sLow && (i > step_.lev || sLow[i] > myLow[i])) {
					end()[i] = sLow[i];
				}
				if      (i > appLev)   { bound[i] = SharedData::maxBound(); }
				else if (U >= myLow[i]){ bound[i] = U; }
				else                   { stepInit(size_); return false; }
				continue;
			}
			if (step_.type) {
				wsum_t L = (stepLow() = std::max(myLow[i], sLow[i]));
				if (U < L) { // path exhausted?
					stepInit(size_);
					return false;
				}
				if (B < L) { // tentative bound too strong?
					return false;
				}
				if (B < U) { // existing step?
					assert(std::count(bound+i+1, bound+size_, SharedData::maxBound()) == int(size_ - (i+1)));
					return true;
				}
				if (U == L) {// done with current level?
					bound[i] = U;
					stepInit(++appLev);
					continue;
				}
				assert(U > L && B > L);
				wsum_t diff = U - L;
				uint32 half = static_cast<uint32>( (diff>>1) | (diff&1) );
				if (step_.type == MinimizeMode_t::bb_step_inc) {
					step_.size = std::min(step_.size, half);
				}
				else if (step_.type == MinimizeMode_t::bb_step_dec) {
					if (!step_.size) { step_.size = static_cast<uint32>(diff); }
					else             { step_.size = half; }
				}
			}
			assert(step_.size != 0);
			bound[i] = U - step_.size;
			actLev_  = 0;
			pos_     = shared_->lits;
		}
		if (seq == shared_->generation()) { break; }
	}
	return step_.lev != size_ || !applyStep;
}
#undef STRATEGY
/////////////////////////////////////////////////////////////////////////////////////////
// MinimizeBuilder
/////////////////////////////////////////////////////////////////////////////////////////
MinimizeBuilder::MinimizeBuilder() : ready_(false) { }
MinimizeBuilder::~MinimizeBuilder() { clear(); }
void MinimizeBuilder::clear() {
	for (LitVec::size_type i = 0; i != lits_.size(); ++i) {
		Weight::free(lits_[i].second);
	}
	LitRepVec().swap(lits_);
	SumVec().swap(adjust_);
	ready_ = false;
}

// adds a new minimize statement
MinimizeBuilder& MinimizeBuilder::addRule(const WeightLitVec& lits, wsum_t initSum) {
	unfreeze();
	uint32 lev = (uint32)adjust_.size();
	adjust_.push_back(initSum);
	for (WeightLitVec::const_iterator it = lits.begin(); it != lits.end(); ++it) {
		adjust_[lev] += addLitImpl(lev, *it);
	}
	return *this;
}
MinimizeBuilder& MinimizeBuilder::addLit(uint32 lev, WeightLiteral lit) {
	unfreeze();
	if (lev >= adjust_.size()) { adjust_.resize(lev + 1, 0); }
	adjust_[lev] += addLitImpl(lev, lit);
	return *this;
}

// adds the weights of the given lit to the appropriate levels in vec
void MinimizeBuilder::addTo(LitRep lit, SumVec& vec) {
	vec.resize(numRules());
	for (Weight* r = lit.second; r; r = r->next) {
		vec[r->level] += r->weight;
	}
}

void MinimizeBuilder::unfreeze() {
	if (ready_) {
		assert(isSentinel(lits_.back().first));
		lits_.pop_back();
		ready_ = false;
	}
}

// merges duplicate literals and removes literals that are already assigned
// POST: the literals in lits_ are unique and sorted by decreasing weight
bool MinimizeBuilder::prepare(SharedContext& ctx) {
	std::sort(lits_.begin(), lits_.end(), CmpByLit());
	LitVec::size_type j = 0;
	Solver& s           = *ctx.master();
	Weight* w           = 0;
	for (LitVec::size_type i = 0, k = 0, end = lits_.size(); i != end;) {
		w    = lits_[i].second;
		if (s.value(lits_[i].first.var()) == value_free) {
			for (k = i+1; k < end && lits_[i].first == lits_[k].first; ++k) {
				// duplicate literal - merge weights
				if (w->level == lits_[k].second->level) {
					// add up weights from same level
					w->weight += lits_[k].second->weight;
				}
				else {
					// extend weight vector with new level
					w->next         = lits_[k].second;
					w               = w->next;
					lits_[k].second = 0;
				}
				Weight::free(lits_[k].second);
			}	
			// exempt from variable elimination
			ctx.setFrozen(lits_[i].first.var(), true);
			lits_[j++] = lits_[i];
			i = k;
		}
		else {
			if (s.isTrue(lits_[i].first)) {
				addTo(lits_[i], adjust_);
			}
			Weight::free(lits_[i].second);
			++i;	
		}
	}
	shrinkVecTo(lits_, j);
	// allocate enough reason data for all our vars
	ctx.requestData(!lits_.empty() ? lits_.back().first.var() : 0);
	// now literals are unique - merge any complementary literals
	j = 0; CmpByWeight greaterW; int cmp;
	for (LitVec::size_type i = 0, k = 1; i < lits_.size(); ) {
		if (k == lits_.size() || lits_[i].first.var() != lits_[k].first.var()) {
			lits_[j++] = lits_[i];
			++i, ++k;
		}
		else if ( (cmp = greaterW.compare(lits_[i], lits_[k])) != 0 ) {
			LitVec::size_type wMin = cmp > 0 ? k : i;
			LitVec::size_type wMax = cmp > 0 ? i : k;
			addTo(lits_[wMin], adjust_);
			mergeReduceWeight(lits_[wMax], lits_[wMin]);
			assert(lits_[wMin].second == 0);
			lits_[j++] = lits_[wMax];
			i += 2;
			k += 2;
		}
		else {
			// weights are equal
			addTo(lits_[i], adjust_);
			Weight::free(lits_[i].second);
			Weight::free(lits_[k].second);
			i += 2;
			k += 2;
		}
	}
	shrinkVecTo(lits_, j);
	std::stable_sort(lits_.begin(), lits_.end(), greaterW);
	if (adjust_.empty()) {
		adjust_.push_back(0);
	}
	// add terminating sentinel literal
	lits_.push_back(LitRep(posLit(0), new Weight(static_cast<uint32>(adjust_.size()-1), 0)));
	return true;
}

// creates a suitable minimize constraint from the 
// previously added minimize statements
MinimizeBuilder::SharedData* MinimizeBuilder::build(SharedContext& ctx) {
	assert(!ctx.master()->hasConflict());
	if (!ctx.master()->propagate()) { return 0; }
	if (!ready_ && !prepare(ctx))   { return 0; }
	SharedData* srep = new (::operator new(sizeof(SharedData) + (lits_.size()*sizeof(WeightLiteral)))) SharedData(adjust_);
	if (adjust_.size() == 1) {
		for (LitVec::size_type i = 0; i != lits_.size(); ++i) {
			srep->lits[i] = WeightLiteral(lits_[i].first, lits_[i].second->weight);
		}
	}
	else {
		// The weights of a multi-level constraint are stored in a flattened way,
		// i.e. we store all weights in one vector and each literal stores
		// an index into that vector. For a (weight) literal i, weights[i.second]
		// is the first weight of literal i and weights[i.second].next denotes
		// whether i has more than one weight.
		srep->lits[0].first  = lits_[0].first;
		srep->lits[0].second = addFlattened(srep->weights, *lits_[0].second);
		for (LitVec::size_type i = 1; i < lits_.size(); ++i) {
			srep->lits[i].first = lits_[i].first;
			if (eqWeight(&srep->weights[srep->lits[i-1].second], *lits_[i].second)) {
				// reuse existing weight
				srep->lits[i].second = srep->lits[i-1].second;
			}
			else {
				// add a new flattened list of weights to srep->weights
				srep->lits[i].second = addFlattened(srep->weights, *lits_[i].second);
			}
		}
	}
	srep->resetBounds();
	ready_ = true;
	return srep;
}

// computes x.weight -= by.weight
// PRE: x.weight > by.weight
void MinimizeBuilder::mergeReduceWeight(LitRep& x, LitRep& by) {
	assert(x.second->level <= by.second->level);
	Weight dummy(0,0);
	dummy.next  = x.second;
	Weight* ins = &dummy;
	for (;by.second;) {
		// unlink head
		Weight* t = by.second;
		by.second = by.second->next;
		// prepare for subtraction
		t->weight*= -1;
		// find correct insert location
		while (ins->next && ins->next->level < t->level) {
			ins = ins->next;
		}
		if (!ins->next || ins->next->level > t->level) {
			t->next   = ins->next ? ins->next : 0;
			ins->next = t;
		}
		else if ( (ins->next->weight += t->weight) != 0 ) {
			delete t;
		}
		else {
			Weight* t2 = ins->next;
			ins->next  = t2->next;
			delete t2;
			delete t;
		}
	}
	x.second = dummy.next;
}

// sort by literal id followed by weight
bool MinimizeBuilder::CmpByLit::operator()(const LitRep& lhs, const LitRep& rhs) const {
	return lhs.first < rhs.first ||
		(lhs.first == rhs.first && lhs.second->level < rhs.second->level);
}
// sort by final weight
bool MinimizeBuilder::CmpByWeight::operator()(const LitRep& lhs, const LitRep& rhs) const {
	Weight* wLhs = lhs.second;
	Weight* wRhs = rhs.second;
	while (wLhs && wRhs) {
		if (wLhs->level != wRhs->level) {
			return wLhs->level < wRhs->level;
		}
		if (wLhs->weight != wRhs->weight) {
			return wLhs->weight > wRhs->weight;
		}
		wLhs = wLhs->next;
		wRhs = wRhs->next;
	}
	return (wLhs && wLhs->weight > 0)
	  ||   (wRhs && wRhs->weight < 0);
}
int MinimizeBuilder::CmpByWeight::compare(const LitRep& lhs, const LitRep& rhs) const {
	if (this->operator()(lhs, rhs)) return 1;
	if (this->operator()(rhs, lhs)) return -1;
	return 0;
}

// frees the given weight list
void MinimizeBuilder::Weight::free(Weight*& head) {
	for (Weight* r = head; r;) {
		Weight* t = r;
		r         = r->next;
		delete t;
	}
	head = 0;
}

// flattens the given weight w and adds the flattened representation to x
// RETURN: starting position of w in x
weight_t MinimizeBuilder::addFlattened(SharedData::WeightVec& x, const Weight& w) {
	typedef SharedData::LevelWeight WT;
	uint32 idx       = static_cast<uint32>(x.size());
	const Weight* r  = &w;
	while (r) {
		x.push_back(WT(r->level, r->weight));
		x.back().next  = (r->next != 0);
		r              = r->next;
	}
	return idx;
}
// returns true if lhs is equal to w
bool MinimizeBuilder::eqWeight(const SharedData::LevelWeight* lhs, const Weight& w) {
	const Weight* r = &w;
	do {
		if (lhs->level != r->level || lhs->weight != r->weight) {
			return false;
		}
		r = r->next;
		if (lhs->next == 0) return r == 0;
		++lhs;
	} while(r);
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////
// UncoreMinimize
/////////////////////////////////////////////////////////////////////////////////////////
UncoreMinimize::UncoreMinimize(SharedMinimizeData* d, uint32 strat) 
	: MinimizeConstraint(d)
	, enum_(0)
	, sum_(new wsum_t[d->numRules()])
	, auxInit_(UINT32_MAX)
	, auxAdd_(0)
	, freeOpen_(0)
	, options_(0) {
	options_ = strat & 7u;
}
void UncoreMinimize::init() {
	releaseLits();
	fix_.clear();
	eRoot_ = 0;
	aTop_  = 0;
	upper_ = shared_->upper(0);
	lower_ = 0;
	gen_   = 0;
	level_ = 0;
	valid_ = 0;
	sat_   = 0;
	pre_   = 0;
	path_  = 1;
	next_  = 0;
	init_  = 1;
}
bool UncoreMinimize::attach(Solver& s) {
	init();
	initRoot(s);
	auxInit_ = UINT32_MAX;
	auxAdd_  = 0;
	if (s.sharedContext()->concurrency() > 1 && shared_->mode() == MinimizeMode_t::enumOpt) {
		enum_ = new DefaultMinimize(shared_->share(), 0);
		enum_->attach(s);
		enum_->relaxBound(true);
	}
	return true;
}
// Detaches the constraint and all cores from s and removes 
// any introduced aux vars.
void UncoreMinimize::detach(Solver* s, bool b) {
	releaseLits();
	destroyDB(closed_, s, b);
	if (s && s->numAuxVars() == (auxInit_ + auxAdd_)) {
		s->popAuxVar(auxAdd_);
		auxAdd_ = 0;
	}
	fix_.clear();
}
// Destroys this object and optionally detaches it from the given solver.
void UncoreMinimize::destroy(Solver* s, bool b) {
	detach(s, b);
	delete [] sum_;
	if (enum_) { enum_->destroy(s, b); enum_ = 0; }
	MinimizeConstraint::destroy(s, b);
}
Constraint::PropResult UncoreMinimize::propagate(Solver&, Literal, uint32&) {
	return PropResult(true, true);
}
bool UncoreMinimize::simplify(Solver& s, bool) {
	if (s.decisionLevel() == 0) { simplifyDB(s, closed_, false); }
	return false;
}
void UncoreMinimize::reason(Solver& s, Literal, LitVec& out) {
	uint32 r = initRoot(s);
	for (uint32 i = 1; i <= r; ++i) {
		out.push_back(s.decision(i));
	}
}

// Integrates any new information from this constraint into s.
bool UncoreMinimize::integrate(Solver& s) {
	assert(!s.isFalse(tag_));
	bool useTag = (shared_->mode() == MinimizeMode_t::enumOpt) || s.sharedContext()->concurrency() > 1;
	if (!prepare(s, useTag)) {
		return false;
	}
	if (enum_ && !shared_->optimize() && !enum_->integrateBound(s)) {
		return false;
	}
	for (uint32 gGen = shared_->generation(); gGen != gen_; ) {
		gen_   = gGen;
		upper_ = shared_->upper(level_);
		gGen   = shared_->generation();
		valid_ = 0;
	}
	return pushPath(s);
}

// Initializes the next optimization level to look at.
bool UncoreMinimize::initLevel(Solver& s) {
	initRoot(s);
	sat_   = 0;
	pre_   = 0;
	path_  = 1;
	init_  = 0;
	sum_[0]= -1;
	if (!fixLevel(s)) {
		return false;
	}
	for (LitVec::const_iterator it = fix_.begin(), end = fix_.end(); it != end; ++it) {
		if (!s.force(*it, eRoot_, this)) { return false; }
	}
	if (!shared_->optimize()) {
		next_  = 0;
		level_ = shared_->maxLevel();
		lower_ = shared_->lower(level_);
		upper_ = shared_->upper(level_);
		return true;
	}
	bool hasWeights = false;
	for (uint32 level = (level_ + next_), n = 1; level <= shared_->maxLevel() && assume_.empty(); ++level) {
		level_ = level;
		lower_ = 0;
		upper_ = shared_->upper(level_);
		for (const WeightLiteral* it = shared_->lits; !isSentinel(it->first); ++it) {
			if (weight_t w = shared_->weight(*it, level_)){
				Literal x = it->first;
				if (w < 0) {
					lower_ += w;
					w       = -w;
					x       = ~x;
				}
				if (s.value(x.var()) == value_free || s.level(x.var()) > eRoot_) {
					addLit(x, w);
					if (w != 1) { hasWeights = true; }
				}
				else if (s.isTrue(x)) {
					lower_ += w;
				}
			}
		}
		if (n == 1) {
			if      (lower_ > upper_){ next_ = 1; break; }
			else if (lower_ < upper_){ next_ = (n = 0);  }
			else                     {
				n     = shared_->checkNext() || level != shared_->maxLevel();
				next_ = n;
				while (!assume_.empty()) {
					fixLit(s, assume_.back().lit);
					assume_.pop_back();
				}
				litData_.clear();
				assume_.clear();
			}
		}
	}
	pre_  = (options_ & MinimizeMode_t::usc_preprocess) != 0u;
	valid_= (pre_ == 0 && !hasWeights);
	if (next_ && !s.hasConflict()) {
		s.force(~tag_, Antecedent(0));
		next_ = 0;
		pre_  = 0;
	}
	return !s.hasConflict();
}

UncoreMinimize::LitData& UncoreMinimize::addLit(Literal p, weight_t w) {
	assert(w > 0);
	litData_.push_back(LitData(w, true, 0));
	assume_.push_back(LitPair(~p, litData_.size()));
	return litData_.back();
}

// Pushes the active assumptions from the active optimization level to
// the root path.
bool UncoreMinimize::pushPath(Solver& s) {
	bool ok   = !s.hasConflict() && !sat_ && (!path_ || (!next_ && !init_) || initLevel(s));
	bool path = path_ != 0;
	while (path) {
		uint32 j = 0;
		path_    = uint32(path = false);
		ok       = ok && s.simplify();
		initRoot(s);
		for (uint32 i = 0, end = assume_.size(), dl; i != end; ++i) {
			LitData& x = getData(assume_[i].id);
			if (x.assume) {
				assume_[j++] = assume_[i];
				Literal lit  = assume_[i].lit;
				if      (!ok || s.isTrue(lit)) { continue; }
				else if (s.value(lit.var()) == value_free) {
					ok    = path || s.pushRoot(lit);
					aTop_ = s.rootLevel();
				}
				else if (s.level(lit.var()) > eRoot_) {
					todo_.push_back(LitPair(~lit, assume_[i].id));
					ok = s.force(lit, Antecedent(0));
				}
				else {
					LitPair core(~lit, assume_[i].id);
					dl   = s.decisionLevel();
					ok   = addCore(s, &core, 1, x.weight);
					end  = assume_.size();
					path = path || (ok && s.decisionLevel() != dl);
					--j;
				}
			}
		}
		shrinkVecTo(assume_, j);
		CLASP_FAIL_IF(!sat_ && s.decisionLevel() != s.rootLevel(), "pushPath must be called on root level (%u:%u)", s.rootLevel(), s.decisionLevel()); 
	}
	if (sat_ || (ok && !validLowerBound())) {
		ok   = false;
		sat_ = 1;
		s.setStopConflict();
	}
	return ok;
}

// Removes invalid assumptions from the root path.
bool UncoreMinimize::popPath(Solver& s, uint32 dl, LitVec& out) {
	CLASP_ASSERT_CONTRACT(dl <= aTop_ && eRoot_ <= aTop_ && "You must not mess with my root level!");
	if (dl < eRoot_) { dl = eRoot_; }
	if (s.rootLevel() > aTop_) {
		// remember any assumptions added after us and
		// force removal of our assumptions because we want
		// them nicely arranged one after another
		s.popRootLevel(s.rootLevel() - aTop_, &out, true);
		dl    = eRoot_;
		path_ = 1;
		CLASP_FAIL_IF(true, "TODO: splitting not yet supported!");
	}
	return s.popRootLevel(s.rootLevel() - (aTop_ = dl));
}

bool UncoreMinimize::relax(Solver& s, bool reset) {
	if (sat_ && !reset) {
		// commit cores of last model
		s.setStopConflict();
		LitVec ignore;
		handleUnsat(s, false, ignore);
	}
	if ((reset && shared_->optimize()) || !assume_.empty() || level_ != shared_->maxLevel() || next_) {
		detach(&s, true);
		init();
	}
	else {
		releaseLits();
	}
	if (!shared_->optimize()) {
		gen_  = shared_->generation();
		next_ = 0;
		valid_= 1;
	}
	init_ = 1;
	sat_  = 0;
	assert(assume_.empty());
	return !enum_ || enum_->relax(s, reset);
}

// Computes the costs of the current assignment.
wsum_t* UncoreMinimize::computeSum(Solver& s) const {
	std::fill_n(sum_, shared_->numRules(), wsum_t(0));
	for (const WeightLiteral* it = shared_->lits; !isSentinel(it->first); ++it) {
		if (s.isTrue(it->first)) { shared_->add(sum_, *it); }
	}
	return sum_;
}

void UncoreMinimize::setLower(wsum_t x) {
	if (!pre_ && x > lower_) { 
		fprintf(stderr, "*** WARNING: Fixing lower bound (%u - %u)\n", (uint32)lower_, (uint32)x); 
		lower_ = x;
	}
}

// Checks whether the current assignment in s is valid w.r.t the
// bound stored in the shared data object.
bool UncoreMinimize::valid(Solver& s) {
	if (shared_->upper(level_) == SharedData::maxBound()){ return true; }
	if (gen_ == shared_->generation() && valid_ == 1)    { return true; }
	if (sum_[0] < 0) { computeSum(s); }
	const wsum_t* rhs;
	uint32 end  = shared_->numRules();
	wsum_t cmp  = 0;
	do {
		gen_  = shared_->generation();
		rhs   = shared_->upper();
		upper_= rhs[level_];
		for (uint32 i = level_; i != end && (cmp = sum_[i]-rhs[i]) == 0; ++i) { ; }
	} while (gen_ != shared_->generation());
	wsum_t low = sum_[level_];
	if (s.numFreeVars() != 0) { sum_[0] = -1; }
	if (cmp < wsum_t(!shared_->checkNext())) {
		valid_ = s.numFreeVars() == 0;
		return true;
	}
	valid_ = 0;
	sat_   = 1;
	setLower(low);
	s.setStopConflict();
 	return false;
}
// Sets the current sum as the current shared optimum.
bool UncoreMinimize::handleModel(Solver& s) {
	if (!valid(s))  { return false; }
	if (sum_[0] < 0){ computeSum(s);}
	shared_->setOptimum(sum_);
	sat_  = shared_->checkNext();
	gen_  = shared_->generation();
	upper_= shared_->upper(level_);
	valid_= 1;
	if (sat_) { setLower(sum_[level_]); }
	return true;
}

// Tries to recover from either a model or a root-level conflict.
bool UncoreMinimize::handleUnsat(Solver& s, bool up, LitVec& out) {
	assert(s.hasConflict());
	if (enum_) { enum_->relaxBound(true); }
	path_   = 1;
	sum_[0] = -1;
	do {
		if (sat_ == 0) {
			if (s.hasStopConflict()) { return false; }
			conflict_ = s.conflict();
			if (s.searchMode() == SolverStrategies::no_learning) {
				conflict_.clear();
				for (uint32 i = 1, end = s.decisionLevel(); i <= end; ++i) { conflict_.push_back(s.decision(i)); }
			}
			weight_t mw;
			uint32 cs = analyze(s, conflict_, mw, out);
			if (!cs) {
				todo_.clear();
				return false;
			}
			if (pre_ == 0) {
				addCore(s, &todo_[0], cs, mw);
				todo_.clear();
			}
			else { // preprocessing: remove assumptions and remember core
				todo_.push_back(LitPair(posLit(0), 0)); // null-terminate core
				lower_ += mw;
				for(LitSet::iterator it = todo_.end() - (cs + 1); it->id; ++it) {
					getData(it->id).assume = 0;
				}
			}
			sat_ = !validLowerBound();
		}
		else {
			s.clearStopConflict();
			popPath(s, 0, out);
			sat_ = 0;
			for (LitSet::iterator it = todo_.begin(), end = todo_.end(), cs; (cs = it) != end; ++it) {
				weight_t w = std::numeric_limits<weight_t>::max();
				while (it->id) { w = std::min(w, getData(it->id).weight); ++it; }
				lower_    -= w;
				if (!addCore(s, &*cs, uint32(it - cs), w)) { break; }
			}
			wsum_t cmp = (lower_ - upper_);
			if (cmp >= 0) {
				fixLevel(s);
				if (cmp > 0) { s.hasConflict() || s.force(~tag_, Antecedent(0)); }
				else         { next_ = level_ != shared_->maxLevel() || shared_->checkNext(); }
			}
			if (pre_) { 
				LitSet().swap(todo_);
				pre_ = 0; 
			}
		}
		if (up && shared_->lower(level_) < lower_) { shared_->setLower(level_, lower_); }
	} while (sat_ || s.hasConflict() || (next_ && out.empty() && !initLevel(s)));
	return true;
}

// Analyzes the current root level conflict and stores the set of our assumptions
// that caused the conflict in todo_.
uint32 UncoreMinimize::analyze(Solver& s, LitVec& rhs, weight_t& minW, LitVec& poppedOther) {
	uint32 marked = 0;                // number of literals to resolve out
	uint32 tPos   = s.trail().size(); // current position in trail
	uint32 cs     = 0, dl, roots = 0;
	minW          = std::numeric_limits<weight_t>::max();
	uint32 minDL  = s.decisionLevel();
	if (!todo_.empty() && todo_.back().id) { 
		cs   = 1;
		minW = getData(todo_.back().id).weight;
		minDL= s.level(todo_.back().lit.var());
	}
	if (s.decisionLevel() <= eRoot_) {
		return cs;
	}
	// resolve all-last uip
	Literal p;
	for (Var v;;) {
		// process current rhs
		for (LitVec::size_type i = 0; i != rhs.size(); ++i) {
			if (!s.seen(v = rhs[i].var())) {
				assert(s.level(v) <= s.decisionLevel());
				s.markSeen(v);
				++marked;
			}
		}
		rhs.clear();
		if (marked-- == 0) { break; }
		// search for the last assigned literal that needs to be analyzed...
		while (!s.seen(s.trail()[--tPos].var())) { ; }	
		p  = s.trail()[tPos];
		dl = s.level(p.var());
		assert(dl);
		s.clearSeen(p.var());
		if      (!s.reason(p).isNull()) { s.reason(p, rhs); }
		else if (p == s.decision(dl) && dl > eRoot_ && dl <= aTop_) {
			s.markSeen(p);
			++roots;
		}
	}
	// map marked root decisions back to our assumptions
	for (LitSet::iterator it = assume_.begin(), end = assume_.end(); it != end && roots; ++it) {
		if (s.seen(p = it->lit) && (dl = s.level(p.var())) != 0) {
			assert(p == s.decision(dl) && getData(it->id).assume);
			if (dl < minDL) { minDL = dl; }
			minW = std::min(getData(it->id).weight, minW);
			todo_.push_back(LitPair(~p, it->id));
			++cs;
			s.clearSeen(p.var());
			--roots;
		}
	}
	popPath(s, minDL - (minDL != 0), poppedOther);
	if (roots) { // clear remaining levels - can only happen if someone messed with our assumptions
		for (uint32 i = s.decisionLevel(); i; --i) { s.clearSeen(s.decision(i).var()); }
	}
	return cs;
}

// Eliminates the given core by adding suitable constraints to the solver.
bool UncoreMinimize::addCore(Solver& s, const LitPair* lits, uint32 cs, weight_t w) {
	assert(s.decisionLevel() == s.rootLevel());
	assert(w && cs);
	// apply weight and check for subcores
	lower_ += w;
	for (uint32 i = 0; i != cs; ++i) {
		LitData& x = getData(lits[i].id);
		if ( (x.weight -= w) <= 0) {
			x.assume = 0;
			x.weight = 0;
		}
		else if (pre_ && !x.assume) {
			x.assume = 1;
			assume_.push_back(LitPair(~lits[i].lit, lits[i].id));
		}
		if (x.weight == 0 && hasCore(x)) {
			Core& core = getCore(x);
			temp_.start(core.bound + 1);
			for (uint32 k = 0, end = core.size(); k != end; ++k) {
				Literal p = core.at(k);
				while (s.topValue(p.var()) != s.value(p.var()) && s.rootLevel() > eRoot_) {
					s.popRootLevel(s.rootLevel() - std::max(s.level(p.var())-1, eRoot_));
					aTop_ = std::min(aTop_, s.rootLevel());
				}
				temp_.add(s, p);
			}
			weight_t cw = core.weight;
			if (!closeCore(s, x, temp_.bound <= 1) || !addCore(s, temp_, cw)) {
				return false;
			}
		}
	}
	// add new core
	if ((options_ & MinimizeMode_t::usc_clauses) == 0u) {
		temp_.start(2);
		for (uint32 i = 0; i != cs; ++i) { temp_.add(s, lits[i].lit); }
		if (!temp_.unsat()) { 
			return addCore(s, temp_, w);
		}
		Literal fix = !temp_.lits.empty() ? temp_.lits[0].first : lits[0].lit;
		return temp_.bound < 2 || fixLit(s, fix);
	}
	return addClauses(s, lits, cs, w);
}

bool UncoreMinimize::addClauses(Solver& s, const LitPair* lits, uint32 size, weight_t w) {
	if (size == 1) { 
		return fixLit(s, lits[0].lit); 
	}
	uint32 i   = size - 1;
	Literal bp = lits[i].lit; 
	while (--i != 0) {
		Literal an = lits[i].lit;
		Literal bn = posLit(s.pushAuxVar());
		Literal cn = posLit(s.pushAuxVar());
		auxAdd_ += 2;
		addLit(cn, w);
		if (!add(comp_disj, s, bn, an, bp)) { return false; }
		if (!add(comp_conj, s, cn, an, bp)) { return false; }
		bp = bn;
	}
	Literal an = lits[i].lit;
	Literal cn = posLit(s.pushAuxVar());
	++auxAdd_;
	addLit(cn, w);
	return add(comp_conj, s, cn, an, bp);
}
bool UncoreMinimize::add(CompType c, Solver& s, Literal head, Literal body1, Literal body2) {
	typedef ClauseCreator::Result Result;
	const uint32 flags = ClauseCreator::clause_explicit | ClauseCreator::clause_not_root_sat | ClauseCreator::clause_no_add;
	const bool    sign = c == comp_conj;
	uint32       first = 0, last = 3;
	if ((options_ & MinimizeMode_t::usc_imp_only) != 0u) {
		first = c == comp_disj;
		last  = first + (1 + (c == comp_disj));
	}
	Literal temp[3][3] = {
		{ (~head)^ sign,   body1 ^ sign, body2 ^ sign },
		{   head ^ sign, (~body1)^ sign, negLit(0) }, 
		{   head ^ sign, (~body2)^ sign, negLit(0) }
	};
	for (uint32 i = first, sz = 3; i != last; ++i) {
		Result res = ClauseCreator::create(s, ClauseRep::create(temp[i], sz, Constraint_t::learnt_other), flags);
		if (res.local) { closed_.push_back(res.local); }
		if (!res.ok()) { return false; }
		sz = 2;
	}
	return true;
}
bool UncoreMinimize::addCore(Solver& s, const WCTemp& wc, weight_t weight) {
	typedef WeightConstraint::CPair ResPair;
	weight_t B = wc.bound;
	if (B <= 0) { 
		// constraint is sat and hence conflicting w.r.t new assumption -
		// relax core
		lower_ += ((1-B)*weight);
		B       = 1;
	}
	if (static_cast<uint32>(B) > static_cast<uint32>(wc.lits.size())) {
		// constraint is unsat and hence the new assumption is trivially satisfied
		return true; 
	}
	// create new var for this core
	if (auxInit_ == UINT32_MAX) { auxInit_ = s.numAuxVars(); }
	Var newAux = s.pushAuxVar();
	++auxAdd_;
	LitData& x = addLit(negLit(newAux), weight);
	WeightLitsRep rep = {&const_cast<WCTemp&>(wc).lits[0], (uint32)wc.lits.size(), B, (weight_t)wc.lits.size()};
	uint32       fset = WeightConstraint::create_explicit | WeightConstraint::create_no_add | WeightConstraint::create_no_freeze | WeightConstraint::create_no_share;
	if ((options_ & MinimizeMode_t::usc_imp_only) != 0u) { fset |= WeightConstraint::create_only_bfb; }
	ResPair       res = WeightConstraint::create(s, negLit(newAux), rep, fset);
	if (res.ok() && res.first()) {
		x.coreId = allocCore(res.first(), B, weight, rep.bound != rep.reach);
	}
	return !s.hasConflict();
}


// Computes the solver's initial root level, i.e. all assumptions that are not from us.
uint32 UncoreMinimize::initRoot(Solver& s) {
	if (eRoot_ == aTop_ && !s.hasStopConflict()) {
		eRoot_ = s.rootLevel();
		aTop_  = eRoot_;
	}
	return eRoot_;
}

// Assigns p at the solver's initial root level.
bool UncoreMinimize::fixLit(Solver& s, Literal p) {
	assert(s.decisionLevel() >= eRoot_);
	if (s.decisionLevel() > eRoot_ && (!s.isTrue(p) || s.level(p.var()) > eRoot_)) {
		// go back to root level
		s.popRootLevel(s.rootLevel() - eRoot_);
		aTop_ = s.rootLevel();
	}
	if (eRoot_ && s.topValue(p.var()) != trueValue(p)) { fix_.push_back(p); } 
	return !s.hasConflict() && s.force(p, this);
}
// Fixes any remaining assumptions of the active optimization level.
bool UncoreMinimize::fixLevel(Solver& s) {
	for (LitSet::iterator it = assume_.begin(), end = assume_.end(); it != end; ++it) {
		if (getData(it->id).assume) { fixLit(s, it->lit); }
	}
	releaseLits();
	return !s.hasConflict();
}
void UncoreMinimize::releaseLits() {
	// remaining cores are no longer open - move to closed list
	for (CoreTable::iterator it = open_.begin(), end = open_.end(); it != end; ++it) {
		if (it->con) { closed_.push_back(it->con); }
	}
	open_.clear();
	litData_.clear();
	assume_.clear();
	todo_.clear();
	freeOpen_ = 0;
}

uint32 UncoreMinimize::allocCore(WeightConstraint* con, weight_t bound, weight_t weight, bool open) {
	if (!open) {
		closed_.push_back(con);
		return 0;
	}
	if (freeOpen_) { // pop next slot from free list
		assert(open_[freeOpen_-1].con == 0);
		uint32 id   = freeOpen_;
		freeOpen_   = static_cast<uint32>(open_[id-1].bound);
		open_[id-1] = Core(con, bound, weight);
		return id;
	}
	open_.push_back(Core(con, bound, weight));
	return static_cast<uint32>(open_.size());
}
bool UncoreMinimize::closeCore(Solver& s, LitData& x, bool sat) {
	if (uint32 coreId = x.coreId) {
		Core& core = open_[coreId-1];
		x.coreId   = 0;
		// close by moving to closed list
		if (!sat) { closed_.push_back(core.con); }
		else      { fixLit(s, core.tag()); core.con->destroy(&s, true); }
		// link slot to free list
		core      = Core(0, 0, static_cast<weight_t>(freeOpen_)); 
		freeOpen_ = coreId;
	}
	return !s.hasConflict();
}
uint32  UncoreMinimize::Core::size()       const { return con->size() - 1; }
Literal UncoreMinimize::Core::at(uint32 i) const { return con->lit(i+1, WeightConstraint::FFB_BTB); }
Literal UncoreMinimize::Core::tag()        const { return con->lit(0, WeightConstraint::FTB_BFB); }
void    UncoreMinimize::WCTemp::add(Solver& s, Literal p) {
	if      (s.topValue(p.var()) == value_free) { lits.push_back(WeightLiteral(p, 1)); }
	else if (s.isTrue(p))                       { --bound; }
}

} // end namespaces Clasp
