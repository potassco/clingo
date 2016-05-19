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

#include <clasp/smodels_constraints.h>
#include <clasp/clause.h>
#include <clasp/solver.h>
#include <algorithm>
namespace Clasp {

/////////////////////////////////////////////////////////////////////////////////////////
// WeightConstraint
/////////////////////////////////////////////////////////////////////////////////////////
bool WeightConstraint::newWeightConstraint(Solver& s, Literal con, WeightLitVec& lits, weight_t bound) {
	assert(s.decisionLevel() == 0);
	weight_t sumW = canonicalize(s, lits, bound);
	if      (bound <= 0)  { return s.force(  con, 0 ); } // trivially SAT
	else if (bound > sumW){ return s.force( ~con, 0 ); } // trivially UNSAT
	
	if (s.value(con.var()) != value_free) {
		// backward propagate
		weight_t b  = s.isTrue(con) ? (sumW-bound)+1 : bound;
		bool bpTrue = s.isTrue(con);
		uint32 i;
		for (i = 0; i != lits.size() && (b - lits[i].second) <= 0; ++i) {
			sumW -= lits[i].second;
			if (bpTrue) bound -= lits[i].second;
			if (!s.force(bpTrue ? lits[i].first : ~lits[i].first, 0)) {
				return false;
			}
		}
		if (i == lits.size()) return true;
		lits.erase(lits.begin(), lits.begin()+i);
		if (lits.front().second == lits.back().second && lits.front().second != 1) {
			// disguised cardinality constraint
			weight_t w = lits.front().second;
			bound      = (bound+(w-1))/w;
			sumW       = (sumW+(w-1))/w;
		}
		if (bound == 1) { // clause
			assert(s.isTrue(con));
			ClauseCreator clause(&s); clause.reserve(lits.size());
			clause.start();
			for (LitVec::size_type i = 0; i != lits.size(); ++i) { clause.add(lits[i].first); }
			return clause.end();
		}
	}
	uint32 size = (uint32)lits.size()+1;
	void*  m    = lits.front().second != lits.back().second
		? ::operator new( sizeof(WeightConstraint) + (3*size) * sizeof(Literal) )
		: ::operator new( sizeof(WeightConstraint) + (2*size) * sizeof(Literal) );
	s.add(new (m) WeightConstraint(s, con, lits, bound, sumW, lits.front().second == lits.back().second));
	return !s.hasConflict();
}

// removes assigned and merges duplicate/complementary literals
// return: achievable weight
// post  : lits is sorted by decreasing weights
weight_t WeightConstraint::canonicalize(Solver& s, WeightLitVec& lits, weight_t& bound) {
	// Step 1: remove assigned/superfluous literals and merge duplicate/complementary ones
	LitVec::size_type j = 0, other;
	for (LitVec::size_type i = 0; i != lits.size(); ++i) {
		Literal x = lits[i].first;
		if (s.value(x.var()) == value_free && lits[i].second > 0) {
			if (!s.seen(x.var())) { // first time we see x, keep and mark x
				if (i != j) lits[j] = lits[i];
				s.markSeen(x);
				++j;
			}
			else if (!s.seen(~x)) { // multi-occurrences of x, merge
				for (other = 0; other != j && lits[other].first != x; ++other) { ; }
				lits[other].second += lits[i].second;
			}
			else {                  // complementary literals ~x x
				for (other = 0; other != j && lits[other].first != ~x; ++other) { ; }
				bound              -= lits[i].second; // decrease by min(w(~x), w(x)) ; assume w(~x) > w(x)
				lits[other].second -= lits[i].second; // keep ~x, 
				if (lits[other].second < 0) {         // actually, w(x) > w(~x), 
					lits[other].first  = x;             // replace ~x with x
					lits[other].second = -lits[other].second;
					s.clearSeen(x.var());
					s.markSeen(x);
					bound += lits[other].second;        // and correct the bound
				}
				else if (lits[other].second == 0) {   // w(~x) == w(x) - drop both lits
					s.clearSeen(x.var());
					std::memmove(&lits[0]+other, &lits[0]+other+1, (j-other-1)*sizeof(lits[other]));
					--j;
				}
			}
		}
		else if (s.isTrue(x)) { bound -= lits[i].second; }
	}
	lits.erase(lits.begin()+j, lits.end());
	// Step 2: compute min,max, achievable weight and clear flags set in step 1
	weight_t sumW = 0;
	uint32 minW   = std::numeric_limits<uint32>::max(), maxW = 0;
	for (LitVec::size_type i = 0; i != lits.size(); ++i) {
		assert(lits[i].second > 0);
		s.clearSeen(lits[i].first.var());
		if (uint32(lits[i].second) > maxW) { maxW = lits[i].second; }
		if (uint32(lits[i].second) < minW) { minW = lits[i].second; }
		sumW += lits[i].second;
	}
	// Step 3: sort by decreasing weight
	if (maxW != minW) {
		std::stable_sort(lits.begin(), lits.end(), compose22(
			std::greater<weight_t>(),
			select2nd<WeightLiteral>(),
			select2nd<WeightLiteral>()));
	}
	else if (minW != 1) {
		// disguised cardinality constraint
		bound = (bound+(minW-1))/minW;
		sumW  = (sumW+(minW-1))/minW;
		for (LitVec::size_type i = 0; i != lits.size(); ++i) { lits[i].second = 1; }
	}
	return sumW;
}

/////////////////////////////////////////////////////////////////////////////////////////
// WeightConstraint
/////////////////////////////////////////////////////////////////////////////////////////
WeightConstraint::WeightConstraint(Solver& s, Literal con, const WeightLitVec& lits, uint32 bound, uint32 sumWeights, bool card) {
	wc_             = !card;                  // weight rule?
	size_           = (uint32)lits.size()+1;  // counting con
	bound_[FFB_BTB]	= (sumWeights-bound)+1;   // ffb-btb
	bound_[FTB_BFB]	= bound;                  // ftb-bfb
	undo_           = undoStart()-1;          // undo stack is initially empty
	lits_[0]        = ~con;                   // store constraint literal
	if (wc_) lits_[1].asUint() = 1;           // and weight if necessary
	s.setFrozen(con.var(), true);             // exempt from variable elimination
	active_ = s.value(con.var())==value_free  // if unassigned
		? NOT_ACTIVE                            // both constraints are relevant
		: FFB_BTB+s.isFalse(con);               // otherwise only one is relevant
	lits_[undoStart()] = con;                 // Copy to undo set
	for (uint32 i = 0, end = (uint32)lits.size(); i != end; ++i) {
		lits_[undoStart()+i+1]=lits[i].first;   // in order to initialize heuristic
		uint32 idx = (i+1)<<wc_;                // For weight rules, 
		lits_[idx] = lits[i].first;             // literals are stored at even
		if (wc_) {                              // while weights are stored at 
			lits_[idx+1].asUint()= lits[i].second;// odd indices.
		}
		addWatch(s, idx, FTB_BFB);              // watches  lits[idx]
		addWatch(s, idx, FFB_BTB);              // watches ~lits[idx]
		s.setFrozen(lits_[idx].var(), true);    // exempt from variable elimination
	}
	// Initialize heuristic with literals (no weights) in constraint.
	s.strategies().heuristic->newConstraint(s, &lits_[undoStart()], size_, Constraint_t::native_constraint);
	if (active_ == NOT_ACTIVE) {
		addWatch(s, 0, FTB_BFB);                // watch con in both phases
		addWatch(s, 0, FFB_BTB);                // in order to allow for backpropagation
	}
	else {
		uint32 d = active_;                     // propagate con
		WeightConstraint::propagate(~lit(0, (ActiveConstraint)active_), d, s);
	}
}

WeightConstraint::~WeightConstraint() {
}

void WeightConstraint::addWatch(Solver& s, uint32 idx, ActiveConstraint c) {
	// Add watch only if c is relevant.
	if (uint32(c^1) != active_) {
		// Use LSB to store the constraint that watches the literal.
		s.addWatch(~lit(idx, c), this, (idx<<1)+c);
	}
}

void WeightConstraint::destroy() {
	void* mem = static_cast<Constraint*>(this);
	this->~WeightConstraint();
	::operator delete(mem);
}

// Returns the numerical highest decision level watched by this constraint.
uint32 WeightConstraint::highestUndoLevel(Solver& s) const {
	return undo_ >= undoStart() 
		? s.level( lits_[undoTop().idx()].var() )
		: 0;
}

// Updates the bound of sub-constraint c and adds the literal at index idx to the 
// undo stack. If the current decision level is not watched, an undo watch is added
// so that the bound can be adjusted once the solver backtracks.
void WeightConstraint::updateConstraint(Solver& s, uint32 idx, ActiveConstraint c) {
	bound_[c] -= weight(idx);
	lits_[idx].watch(); // mark as processed
	if (highestUndoLevel(s) != s.decisionLevel()) {
		s.addUndoWatch(s.decisionLevel(), this);
	}
	undoPush(idx, c, true);
}


// Since clasp uses an eager assignment strategy where literals are assigned as soon
// as they are added to the propagation queue, we distinguish processed from unprocessed literals.
// Processed literals are those for which propagate was already called and the corresponding bound 
// was updated; their watch flag was set in updateConstraint(). 
// Unprocessed literals are either free or were not yet propagated. During propagation
// we treat all unprocessed literals as free. This way, conflicts are detected early.
// Consider: x :- 3 [a=3, b=2, c=1,d=1] and PropQ: b, ~Body, c. 
// Initially b, ~Body, c are unprocessed and the bound is 3.
// Step 1: propagate(b)    : b is marked as processed and bound is reduced to 1.
//   Now, although we already know that the body is false, we do not backpropagate yet
//   because the body is unprocessed. Deferring backpropagation until the body is processed
//   makes reason computation easier.
// Step 2: propagate(~Body): ~body is marked as processed and bound is reduced to 0.
//   Since the body is now part of our reason set, we can start backpropagation.
//   First we assign the unprocessed and free literal ~a. Literal ~b is skipped, because
//   its complementary literal was already successfully processed. Finally, we force 
//   the unprocessed but false literal ~c to true. This will generate a conflict and 
//   propagation is stopped. Without the distinction between processed and unprocessed
//   lits we would have to skip ~c. We would then have to manually trigger the conflict
//   {b, ~Body, c} in step 3, when propagate(c) sets the bound to -1.
Constraint::PropResult WeightConstraint::propagate(const Literal&, uint32& d, Solver& s) {
	// determine the affected constraint and its body literal
	ActiveConstraint c  = (ActiveConstraint)(d&1);
	Literal body        = lit(0, c);
	if ( uint32(c^1) == active_ || s.isTrue(body) ) {
		// the other constraint is active or this constraint is already satisfied; 
		// nothing to do
		return PropResult(true, true);		
	}
	// the constraint is not yet satisfied; update it and
	// check if we can now propagate any literals.
	updateConstraint(s, d >> 1, c);
	if (bound_[c] <= 0 || (wc_ && lits_[0].watched())) {           
		if (!lits_[0].watched()) {
			// forward propagate constraint to true
			active_ = c;
			undoPush(0, c, false);
			return PropResult(s.force(body, this), true);
		}
		else {
			// backward propagate false constraint
			assert(s.isFalse(body));
			uint32 next = 1;
			uint32& n   = getBpIndex(next);
			for (uint32 inc = 1+wc_; n < undoStart() && bound_[c] - weight(n) < 0; n+=inc) {
				if (!lits_[n].watched()) {
					active_ = c;
					Literal x = lit(n, c);
					if (s.value(x.var()) == value_free) {
						undoPush(n, c, false);
						s.force(x, this);
					}
					else if (!s.force(x, this)) {
						return PropResult(false, true);
					}
				}
			}
		}
	}
	return PropResult(true, true);
}

// Builds the reason for p from the undo stack of this constraint.
// The reason will only contain literals that were processed by the 
// active sub-constraint.
void WeightConstraint::reason(const Literal& p, LitVec& r) {
	assert(active_ != NOT_ACTIVE);
	Literal x;
	for (uint32 i = undoStart(); i <= undo_; ++i) {
		UndoInfo u = undoPos(i);
		// Consider only lits that are relevant to the active constraint
		if (u.constraint() == (ActiveConstraint)active_) {
			x = lit(u.idx(), u.constraint());
			if (u.inReason()) {
				// Add only lits to reason that were not propagated by this constraint
				r.push_back( ~x );
			}
			else if (x == p) {
				break;
			}
		}
	}
}

// undoes processed assignments 
void WeightConstraint::undoLevel(Solver& s) {
	if (wc_) lits_[1].asUint() = 1;
	for (UndoInfo u = undoTop(); undo_ >= undoStart() && s.value(lits_[u.idx()].var()) == value_free;) {
		if (u.inReason()) {
			lits_[u.idx()].clearWatch();
			bound_[u.constraint()] += weight(u.idx());
		}
		--undo_;
		u = undoTop();
	}
	if (!lits_[0].watched()) active_ = NOT_ACTIVE;
}

bool WeightConstraint::simplify(Solver& s, bool) {
	if (bound_[0] <= 0 || bound_[1] <= 0) {
		s.removeWatch(~lits_[0], this);
		s.removeWatch( lits_[0], this);
		uint32 inc = 1 + wc_;
		for (uint32 i = inc, end = undoStart(); i != end; i += inc) {
			s.removeWatch(~lits_[i], this);
			s.removeWatch( lits_[i], this);
		}
		return true;
	}
	else if (active_ != NOT_ACTIVE) {
		uint32 inc = 1 + wc_;
		for (uint32 i = 0, end = undoStart(); i != end; i += inc) {
			s.removeWatch(active_ == FTB_BFB ? ~lits_[i] : lits_[i], this);
		}
	}
	return false;
}

uint32 WeightConstraint::estimateComplexity(const Solver& s) const {
	weight_t w1 = bound_[0];
	weight_t w2 = bound_[1];
	uint32 i=0, inc = 1+wc_;
	uint32 r = 2;
	for (i += inc; i < undoStart(); i+=inc) {
		if (s.value(lits_[i].var()) == value_free) {
			++r;
			if ( (w1 -= weight(i)) <= 0 ) break;
			if ( (w2 -= weight(i)) <= 0 ) break;
		}
	}
	return r;
}


/////////////////////////////////////////////////////////////////////////////////////////
// MinimizeConstraint
/////////////////////////////////////////////////////////////////////////////////////////
MinimizeConstraint::MinimizeConstraint() 
	: models_(0)
	, activePL_(0)
	, activeIdx_(0)
	, mode_(compare_less) {
	undoList_.push_back( UndoLit(posLit(0), 0, true) ); // sentinel
}

MinimizeConstraint::~MinimizeConstraint() {
	for (uint32 i = 0; i != minRules_.size(); ++i) {
		delete minRules_[i];
	}
}

bool MinimizeConstraint::setOptimum(uint32 level, WeightSum opt) {
	assert(level < minRules_.size());
	if (level > activePL_ || opt >= minRules_[level]->sum_) {
		minRules_[level]->opt_ = opt;
		return true;
	}
	return false;
}

void MinimizeConstraint::minimize(Solver& s, const WeightLitVec& literals, bool heu) {
	assert(s.decisionLevel() == 0 && "can't add minimize rules after init!");
	index_.resize( (s.numVars()+1) << 1, varMax );
	MinRule* nr = new MinRule;
	nr->lits_.assign(literals.begin(), literals.end());
	std::stable_sort(nr->lits_.begin(), nr->lits_.end(), compose22(
			std::greater<weight_t>(),
			select2nd<WeightLiteral>(),
			select2nd<WeightLiteral>()));
	minRules_.push_back( nr );
	WeightLitVec::iterator j = nr->lits_.begin();
	// Consider only relevant literals, i.e. those with weights > 0
	for (WeightLitVec::iterator i = nr->lits_.begin(); i != nr->lits_.end() && i->second > 0; ++i) {
		if (s.isTrue( i->first )) {
			nr->sum_ += i->second;
		}
		else if (!s.isFalse(i->first)) {
			*j = *i;
			uint32 idx = i->first.index();
			if (index_[idx] == varMax) {
				index_[idx] = (uint32)occurList_.size();
				s.addWatch(i->first, this, index_[idx]);
				occurList_.push_back( LitOccurrence() );
				s.setFrozen(i->first.var(), true);
				if (heu) {
					s.initSavedValue(i->first.var(), falseValue(i->first));
				}
			}
			LitRef newOcc;
			newOcc.ruleIdx_ = (uint32)minRules_.size()-1;
			newOcc.litIdx_  = (uint32)(j - nr->lits_.begin());
			occurList_[ index_[idx] ].push_back( newOcc );
			++j;
		}
	}
}

bool MinimizeConstraint::simplify(Solver&, bool) {
	Index().swap(index_);
	return false;
}

void MinimizeConstraint::updateSum(uint32 key) {
	const LitOccurrence& occp = occurList_[key];
	for (LitOccurrence::size_type i = 0; i < occp.size(); ++i) {
		rule(occp[i])->sum_ += weight(occp[i]);
	}
	activePL_ = std::min(pLevel(occp[0]), activePL_);
}

// Checks whether the assignment is conflicting under the assumption
// that all rules in the range [0, x) are equal to their optimum
bool MinimizeConstraint::conflict(uint32& x) const {
	while (x < minRules_.size() && rule(x)->sum_ == rule(x)->opt_) ++x;
	return x < minRules_.size() && rule(x)->sum_ > rule(x)->opt_;
}

Constraint::PropResult MinimizeConstraint::propagate(const Literal& p, uint32& data, Solver& s) {
	assert(data < occurList_.size() && lit(occurList_[data][0]) == p);
	updateSum(data);
	addUndo(s, p, data, false);
	if (rule(activePL_)->opt_ == std::numeric_limits<WeightSum>::max()) {
		// no model found, yet!
		return PropResult(true, true);
	}
	if (rule(activePL_)->sum_ > rule(activePL_)->opt_ || !backpropagate(s, rule(activePL_))) {
		// The first condition can fire if we backtrack from ~x to x and 
		// x is contained in a minimize statement.
		// Eg. m1 = a, m2 = b. Model: ~a, b. Last decision: ~a
		// backpropagation can fail whenever more than one literals is assigned before
		// propagate is called. In that case the sums intially do not match the assignment.
		// Eg: assume m1 = a,b,c,d,e and m2 = f,g, opt(m1) = 3, opt(m2) = 0, activePL_ = 0.
		// Further assume f a b c are assigned in one swoop.
		// First, we see f: no problem (sum(m1) < opt(m1))
		// Next, we see a: still ok (sum(m1) = 1 < opt(m1))
		// Now comes b: sum(m1) = 2 -> since m2 is vioalated we try to keep m1 optimal by backpropagating ~d ~e
		// Finally, we see c which results in sum(m1) = opt(m1) and sum(m2) > opt(m2), i.e. a conflict.
		
		// Remove all literals that were forced because of p
		while (undoList_.back().lit_ != p) {
			assert(undoList_.back().pos_ == 0);
			undoList_.pop_back();
		}
		// we need ~p to compute the reason but also p so that the sum is correctly backtracked.
		undoList_.pop_back(); // replace p with ~p, p.
		undoList_.push_back( UndoLit(~p, activePL_, false) );
		undoList_.push_back( UndoLit(p, data, true) );
		return PropResult(s.force(~p, this), true); // force conflict
	}
	return PropResult(true, true);
}

bool MinimizeConstraint::backpropagate(Solver& s, MinRule* r) {
	assert(r->sum_ <= r->opt_);
	do {
		for (uint32 propPL = activePL_; activeIdx_ < r->lits_.size(); ++activeIdx_) {
			WeightLiteral x = r->lits_[activeIdx_];
			if (s.value(x.first.var()) == value_free) {
				WeightSum newSum = r->sum_ + x.second;
				if (newSum < r->opt_ || (newSum == r->opt_ && propPL == activePL_ && !conflict(++propPL))) {
					return true;
				}
				else {  // setting x to true would lead to a conflict -> force ~x
					addUndo(s, ~x.first, propPL, true);
					s.force(~x.first, this);
				}
			}
		}
		// the active rule is completly assigned
		if (r->sum_ < r->opt_ || (activePL_ + 1) == minRules_.size()) {
			break;
		}
		++activePL_, activeIdx_ = 0;  // continue with next rule
		r = rule(activePL_);
	} while (r->sum_ <= r->opt_);
	return r->sum_ <= r->opt_;
}

void MinimizeConstraint::reason(const Literal& p, LitVec& r) {
	uint32 activeRule = 0;
	if (minRules_.size() > 1) {
		UndoList::iterator it = undoList_.end() - 1;
		while (it->lit_ != p) --it;
		activeRule = it->key_;
	}
	for (uint32 i = 1; undoList_[i].lit_ != p; ++i) {
		if (undoList_[i].pos_ == 1 && pLevel(occurList_[undoList_[i].key_][0]) <= activeRule) {
			r.push_back(undoList_[i].lit_);
		}
	}
}

uint64 MinimizeConstraint::models() const {
	return models_;
}

// Stores the current model as the optimum and determines the decision level 
// on which the search should continue.
// Returns maxDL(R)-1, where
//  R: the set of minimize rules
//  maxDL(R): the highest decision level on which one of the literals
//  from R was assigned true. 
uint32 MinimizeConstraint::setModel(Solver& s) {
	assert(!minRules_.empty());
	for (LitVec::size_type i = 0; i < minRules_.size()-1; ++i) {
		if (minRules_[i]->sum_ < minRules_[i]->opt_) {
			models_ = 0;
		}
		minRules_[i]->opt_ = minRules_[i]->sum_;
	}
	if (minRules_.back()->sum_ < minRules_.back()->opt_ || mode_ == compare_less) {
		models_ = 0;
	}
	minRules_.back()->opt_ = minRules_.back()->sum_ - (mode_ == compare_less);
	++models_;
	activePL_   = 0;
	activeIdx_  = 0;
	if (mode_ == compare_less) {
		// Since mode is compare_less, next model must be strictly better than this one.
		// Search the literal that was assigned true last, i.e. the last literal that increased
		// the sum. We must at least invert that literal in order to get a better model.
		UndoList::size_type i = undoList_.size();
		while (i-- != 0) {
			if (undoList_[i].pos_ == 1) {
				return s.level(undoList_[i].lit_.var())-1;
			}
		}
	}
	return (uint32)s.decisionLevel()-1; 
}

bool MinimizeConstraint::backtrackFromModel(Solver& s) {
	uint32 dl = setModel(s);
	if (dl < s.rootLevel() || dl == static_cast<uint32>(-1)) {
		return false;
	}
	while (s.backtrack() && dl < s.decisionLevel()) {;}
	assert(dl == s.decisionLevel());
	return backpropagate(s, rule(activePL_));
}

void MinimizeConstraint::addUndo(Solver& s, Literal p, uint32 key, bool forced) {
	if (s.level(undoList_.back().lit_.var()) != s.decisionLevel()) {
		s.addUndoWatch(s.decisionLevel(), this);
	}
	undoList_.push_back( UndoLit(p, key, forced == false) );
}

void MinimizeConstraint::undoLevel(Solver& s) {
	while (s.value(undoList_.back().lit_.var()) == value_free) {
		if (undoList_.back().pos_ == 1) {
			const LitOccurrence& occ = occurList_[undoList_.back().key_];
			for (LitOccurrence::size_type i = 0; i < occ.size(); ++i) {
				rule(occ[i])->sum_ -= weight(occ[i]);
				if (pLevel(occ[i]) <= activePL_) {
					activePL_   = pLevel(occ[i]);
					activeIdx_  = 0;
				}
			}
		}
		undoList_.pop_back();
	}
}

bool MinimizeConstraint::select(Solver& s) const {
	Rules::size_type x = minRules_.size();
	while (x != 0) {
		MinRule* r = minRules_[--x];
		for (LitVec::size_type i = 0, end = r->lits_.size(); i != end; ++i) {
			if (s.value(r->lits_[i].first.var()) == value_free) {
				s.assume(~r->lits_[i].first);
				return true;
			}
		}
	}
	return s.strategies().heuristic->select(s);
}

bool MinimizeConstraint::backpropagate(Solver &s) {
	while (!backpropagate(s, rule(activePL_)) || !s.propagate()) {
		if (!s.resolveConflict()) return false;
	}
	return s.decisionLevel() > 0 || s.simplify();
}


}
