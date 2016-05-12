// 
// Copyright (c) 2006-2011, Benjamin Kaufmann
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

#include <clasp/weight_constraint.h>
#include <clasp/clause.h>
#include <clasp/solver.h>
#include <clasp/util/misc_types.h>
#include <algorithm>

namespace Clasp {
/////////////////////////////////////////////////////////////////////////////////////////
// WeightLitsRep
/////////////////////////////////////////////////////////////////////////////////////////
// Removes assigned and merges duplicate/complementary literals.
// return: achievable weight
// post  : lits is sorted by decreasing weights
WeightLitsRep WeightLitsRep::create(Solver& s, WeightLitVec& lits, weight_t bound) {
	// Step 1: remove assigned/superfluous literals and merge duplicate/complementary ones
	LitVec::size_type j = 0, other;
	const weight_t MAX_W= std::numeric_limits<weight_t>::max();
	for (LitVec::size_type i = 0; i != lits.size(); ++i) {
		lits[i].first.clearWatch();
		Literal x = lits[i].first;
		if (lits[i].second != 0 && s.topValue(x.var()) == value_free) {
			if (lits[i].second < 0) {
				lits[i].second = -lits[i].second;
				lits[i].first  = x = ~lits[i].first;
				CLASP_ASSERT_CONTRACT_MSG(bound < 0 || (MAX_W-bound) >= lits[i].second, "bound out of range");
				bound         += lits[i].second;
			}
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
	weight_t minW = MAX_W, maxW = 1;
	weight_t B    = std::max(bound, 1);
	for (LitVec::size_type i = 0; i != lits.size(); ++i) {
		assert(lits[i].second > 0);
		s.clearSeen(lits[i].first.var());
		if (lits[i].second > maxW) { maxW = lits[i].second = std::min(lits[i].second, B);  }
		if (lits[i].second < minW) { minW = lits[i].second; }
		CLASP_ASSERT_CONTRACT_MSG((MAX_W - sumW) >= lits[i].second, "Sum of weights out of range");
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
	WeightLitsRep result = { !lits.empty() ? &lits[0] : 0, (uint32)lits.size(), bound, sumW };
	return result;
}

// Propagates top-level assignment.
bool WeightLitsRep::propagate(Solver& s, Literal W) {
	if      (sat())  { return s.force(W); } // trivially SAT
	else if (unsat()){ return s.force(~W);} // trivially UNSAT
	else if (s.topValue(W.var()) == value_free) { 
		return true; 
	}
	// backward propagate
	bool bpTrue = s.isTrue(W);
	weight_t B  = bpTrue ? (reach-bound)+1 : bound;
	while (lits->second >= B) {
		reach -= lits->second;
		if (!s.force(bpTrue ? lits->first : ~lits->first, 0))        { return false; }
		if ((bpTrue && (bound -= lits->second) <= 0) || --size == 0) { return true;  }
		++lits;
	}
	if (lits->second > 1 && lits->second == lits[size-1].second) {
		B     = lits->second;
		bound = (bound + (B-1)) / B;
		reach = (reach + (B-1)) / B;
		for (uint32 i = 0; i != size && lits[i].second != 1; ++i) { lits[i].second = 1; }
	}
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////
// WeightConstraint::WL
/////////////////////////////////////////////////////////////////////////////////////////
WeightConstraint::WL::WL(uint32 s, bool shared, bool hasW) : sz(s), rc(shared), w(hasW) { }
uint8* WeightConstraint::WL::address() { return reinterpret_cast<unsigned char*>(this) - (sizeof(uint32) * rc); }
WeightConstraint::WL* WeightConstraint::WL::clone(){
	if (shareable()) {
		++*reinterpret_cast<Clasp::atomic<uint32>*>(address());
		return this;
	}
	else {
		uint32 litSize = (size() << uint32(weights()))*sizeof(Literal);
		WL* x          = new (::operator new(sizeof(WL) + litSize)) WL(size(), false, weights());
		std::memcpy(x->lits, this->lits, litSize);
		return x;
	}
}
void WeightConstraint::WL::release() {
	unsigned char* x = address();
	if (!shareable() || --*reinterpret_cast<Clasp::atomic<uint32>*>(x) == 0) {
		::operator delete(x);
	}
}
uint32 WeightConstraint::WL::refCount() const {
	assert(shareable());
	return *reinterpret_cast<const Clasp::atomic<uint32>*>(const_cast<WL*>(this)->address());
}
/////////////////////////////////////////////////////////////////////////////////////////
// WeightConstraint
/////////////////////////////////////////////////////////////////////////////////////////
WeightConstraint::CPair WeightConstraint::create(Solver& s, Literal W, WeightLitVec& lits, weight_t bound, uint32 flags) {
	if ((flags & create_eq_bound) == 0) {
		return WeightConstraint::create(s, W, WeightLitsRep::create(s, lits, bound), flags);
	}
	WeightLitsRep rep = WeightLitsRep::create(s, lits, bound + 1);
	CPair res;
	res.con[1] = WeightConstraint::createImpl(s, ~W, rep, flags);
	if (res.ok()) {
		rep.bound -= 1;
		// redo coefficient reduction
		for (unsigned i = 0; i != rep.size && rep.lits[i].second > rep.bound; ++i) {
			rep.reach -= rep.lits[i].second;
			rep.reach += (rep.lits[i].second = rep.bound);
		}
		res.con[0] = WeightConstraint::createImpl(s, W, rep, flags);
	}
	return res;	
}
WeightConstraint::CPair WeightConstraint::create(Solver& s, Literal W, WeightLitsRep rep, uint32 flags) {
	CPair res;
	res.con[0] = createImpl(s, W, rep, flags);
	if ((flags & create_eq_bound) != 0 && res.ok()) {
		// Adds constraints for W == (lits == bound)
		++rep.bound;
		res.con[1] = createImpl(s, ~W, rep, flags);
	}
	return res;
}

WeightConstraint* WeightConstraint::createImpl(Solver& s, Literal W, WeightLitsRep& rep, uint32 flags) {
	WeightConstraint* conflict = (WeightConstraint*)0x1;
	bool addSat = (flags&create_sat) != 0 && rep.size;
	if (!rep.propagate(s, W))                 { return conflict; }
	if (rep.unsat() || (rep.sat() && !addSat)){ return 0; }
	if ((rep.bound == 1 || rep.bound == rep.reach) && (flags & create_explicit) == 0) {
		LitVec clause; clause.reserve(1 + rep.size);
		Literal bin[2];
		bool disj = rep.bound == 1; // con == disjunction or con == conjunction
		bool sat  = false;
		clause.push_back(W ^ disj);
		for (LitVec::size_type i = 0; i != rep.size; ++i) { 
			bin[0] = ~clause[0];
			bin[1] = rep.lits[i].first ^ disj;
			if (bin[0] != ~bin[1]) {
				if (bin[0] != bin[1])                 { clause.push_back(~bin[1]); }
				if (!s.add(ClauseRep::create(bin, 2))){ return conflict; }
			}
			else { sat = true; }
		}
		return (sat || ClauseCreator::create(s, clause, 0)) ? 0 : conflict;
	}
	assert(rep.open() || (rep.sat() && addSat));
	uint32 act  = 3u;
	if (flags & (create_only_btb|create_only_bfb)) {
		act  = ((flags & (create_only_btb|create_only_bfb)) / create_only_btb) - 1u;
		act += (act == 2u);
	}
	if (!s.sharedContext()->physicalShareProblem()) { flags |= create_no_share; }
	if (s.sharedContext()->frozen())                { flags |= (create_no_freeze|create_no_share); }
	bool   hasW = rep.hasWeights();
	uint32 size = 1 + rep.size;
	uint32 nb   = sizeof(WeightConstraint) + (size+uint32(hasW))*sizeof(UndoInfo);
	uint32 wls  = sizeof(WL) + (size << uint32(hasW))*sizeof(Literal);
	void*  m    = 0;
	WL*    sL   = 0;
	if ((flags & create_no_share) != 0) {
		nb        = ((nb + 3) / 4)*4;
		m         = ::operator new (nb + wls);
		sL        = new (reinterpret_cast<unsigned char*>(m)+nb) WL(size, false, hasW);
	}
	else {
		static_assert(sizeof(Clasp::atomic<uint32>) == sizeof(uint32), "Invalid size!");
		m         = ::operator new(nb);
		uint8* t  = (uint8*)::operator new(wls + sizeof(uint32));
		*(new (t) Clasp::atomic<uint32>()) = 1;
		sL        = new (t+sizeof(uint32)) WL(size, true, hasW);
	}
	assert(m && (reinterpret_cast<uintp>(m) & 7u) == 0);
	SharedContext*  ctx = (flags & create_no_freeze) == 0 ? const_cast<SharedContext*>(s.sharedContext()) : 0;
	WeightConstraint* c = new (m) WeightConstraint(s, ctx, W, rep, sL, act);
	if (!c->integrateRoot(s)) {
		c->destroy(&s, true);
		return conflict;
	}
	if ((flags & create_no_add) == 0) { s.add(c); }
	return c;
}
WeightConstraint::WeightConstraint(Solver& s, SharedContext* ctx, Literal W, const WeightLitsRep& rep, WL* out, uint32 act) {
	const bool hasW = rep.hasWeights();
	lits_           = out;
	active_         = act;
	ownsLit_        = !out->shareable();
	Literal* p      = lits_->lits;
	Literal* h      = (Literal*)undo_;
	weight_t w      = 1;
	Var      mv     = W.var();
	bound_[FFB_BTB]	= (rep.reach-rep.bound)+1; // ffb-btb
	bound_[FTB_BFB]	= rep.bound;               // ftb-bfb
	*p++            = ~W;                      // store constraint literal
	if (hasW) *p++  = Literal::fromRep(w);     // and weight if necessary
	if (ctx) ctx->setFrozen(W.var(), true);    // exempt from variable elimination
	if (s.topValue(W.var()) != value_free) {   // only one direction is relevant
		active_ = FFB_BTB+s.isFalse(W);
	}
	watched_        = 3u - (active_ != 3u || ctx == 0);
	for (uint32 i = 0, j = 1, end = rep.size; i != end; ++i, ++j) {
		*p++ = h[j] = rep.lits[i].first;         // store constraint literal
		w    = rep.lits[i].second;               // followed by weight 
		if (hasW) *p++= Literal::fromRep(w);     // if necessary
		addWatch(s, j, FTB_BFB);                 // watches  lits[idx]
		addWatch(s, j, FFB_BTB);                 // watches ~lits[idx]
		if (ctx) ctx->setFrozen(h[j].var(), true);// exempt from variable elimination
		mv   = std::max(mv, h[j].var());
	}
	if (hasW) s.requestData(mv+1);         // weight constraints can become unit more than once
	// init heuristic
	uint32 off = active_ != NOT_ACTIVE;
	h[0]       = W;
	s.heuristic()->newConstraint(s, h+off, rep.size+(1-off), Constraint_t::static_constraint);
	// init undo stack
	up_                 = undoStart();     // undo stack is initially empty
	undo_[0].data       = 0;
	undo_[up_].data     = 0;
	setBpIndex(1);                         // where to start back propagation
	if (s.topValue(W.var()) == value_free){
		addWatch(s, 0, FTB_BFB);             // watch con in both phases
		addWatch(s, 0, FFB_BTB);             // in order to allow for backpropagation
	}
	else {
		uint32 d = active_;                  // propagate con
		WeightConstraint::propagate(s, ~lit(0, (ActiveConstraint)active_), d);
	}
}

WeightConstraint::WeightConstraint(Solver& s, const WeightConstraint& other) {
	lits_        = other.lits_->clone();
	ownsLit_     = 0;
	Literal* heu = (Literal*)undo_;
	heu[0]       = ~lits_->lit(0);
	bound_[0]	   = other.bound_[0];
	bound_[1]	   = other.bound_[1];
	active_      = other.active_;
	watched_     = other.watched_;
	if (active_ == NOT_ACTIVE && s.value(heu[0].var()) == value_free) {
		addWatch(s, 0, FTB_BFB);  // watch con in both phases
		addWatch(s, 0, FFB_BTB);  // in order to allow for backpropagation
	}
	for (uint32 i = 1, end = size(); i < end; ++i) {
		heu[i]      = lits_->lit(i);
		if (s.value(heu[i].var()) == value_free) {
			addWatch(s, i, FTB_BFB);  // watches  lits[i]
			addWatch(s, i, FFB_BTB);  // watches ~lits[i]
		}
	}
	// Initialize heuristic with literals (no weights) in constraint.
	uint32 off = active_ != NOT_ACTIVE;
	s.heuristic()->newConstraint(s, heu+off, size()-off, Constraint_t::static_constraint);
	// Init undo stack
	std::memcpy(undo_, other.undo_, sizeof(UndoInfo)*(size()+isWeight()));
	up_ = other.up_;
}

WeightConstraint::~WeightConstraint() {}
Constraint* WeightConstraint::cloneAttach(Solver& other) { 
	void* m = ::operator new(sizeof(WeightConstraint) + (size()+isWeight())*sizeof(UndoInfo));
	return new (m) WeightConstraint(other, *this);
}

bool WeightConstraint::integrateRoot(Solver& s) {
	if (!s.decisionLevel() || highestUndoLevel(s) >= s.rootLevel() || s.hasConflict()) { return !s.hasConflict(); }
	// check if constraint has assigned literals
	uint32 low = s.decisionLevel(), vDL;
	uint32 np  = 0;
	for (uint32 i = 0, end = size(); i != end; ++i) {
		Var v = lits_->var(i);
		if (s.value(v) != value_free && (vDL = s.level(v)) != 0) { 
			++np; 
			s.markSeen(v);
			low = std::min(low, vDL);
		}
	}
	// propagate assigned literals in assignment order
	const LitVec& trail = s.trail();
	const uint32  end   = trail.size() - s.queueSize();
	GenericWatch* w     = 0;
	for (uint32 i = s.levelStart(low); i != end && np; ++i) {
		Literal p = trail[i];
		if (s.seen(p) && np--) {
			s.clearSeen(p.var());
			if (!s.hasConflict() && (w = s.getWatch(trail[i], this)) != 0) {
				w->propagate(s, p);
			}
		}
	}
	for (uint32 i = end; i != trail.size() && np; ++i) {
		if (s.seen(trail[i].var())) { --np; s.clearSeen(trail[i].var()); }
	}
	return !s.hasConflict();
}
void WeightConstraint::addWatch(Solver& s, uint32 idx, ActiveConstraint c) {
	// Add watch only if c is relevant.
	if (uint32(c^1) != active_) {
		// Use LSB to store the constraint that watches the literal.
		s.addWatch(~lit(idx, c), this, (idx<<1)+c);
	}
}

void WeightConstraint::destroy(Solver* s, bool detach) {
	if (s && detach) {
		for (uint32 i = 0, end = size(); i != end; ++i) {
			s->removeWatch( lits_->lit(i), this );
			s->removeWatch(~lits_->lit(i), this );
		}
		for (uint32 i = s->decisionLevel(); i; --i) {
			s->removeUndoWatch(i, this);
		}
	}
	if (ownsLit_ == 0) { lits_->release(); }
	void* mem    = static_cast<Constraint*>(this);
	this->~WeightConstraint();
	::operator delete(mem);	
}

void WeightConstraint::setBpIndex(uint32 n){ 
	if (isWeight()) undo_[0].data = (n<<1)+(undo_[0].data&1); 
}

// Returns the numerical highest decision level watched by this constraint.
uint32 WeightConstraint::highestUndoLevel(Solver& s) const {
	return up_ != undoStart() 
		? s.level(lits_->var(undoTop().idx()))
		: 0;
}

// Updates the bound of sub-constraint c and adds the literal at index idx to the 
// undo stack. If the current decision level is not watched, an undo watch is added
// so that the bound can be adjusted once the solver backtracks.
void WeightConstraint::updateConstraint(Solver& s, uint32 idx, ActiveConstraint c) {
	bound_[c] -= weight(idx);
	if (highestUndoLevel(s) != s.decisionLevel()) {
		s.addUndoWatch(s.decisionLevel(), this);
	}
	undo_[up_].data = (idx<<2) + (c<<1) + (undo_[up_].data & 1);
	++up_;
	assert(!litSeen(idx));
	toggleLitSeen(idx);
}

// Since clasp uses an eager assignment strategy where literals are assigned as soon
// as they are added to the propagation queue, we distinguish processed from unprocessed literals.
// Processed literals are those for which propagate was already called and the corresponding bound 
// was updated; they are flagged in updateConstraint(). 
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
Constraint::PropResult WeightConstraint::propagate(Solver& s, Literal, uint32& d) {
	// determine the affected constraint and its body literal
	ActiveConstraint c  = (ActiveConstraint)(d&1);
	const uint32   idx  = d >> 1;
	Literal body        = lit(0, c);
	if ( uint32(c^1) == active_ || s.isTrue(body) ) {
		// the other constraint is active or this constraint is already satisfied; 
		// nothing to do
		return PropResult(true, true);		
	}
	if (idx == 0 && s.decisionLevel() == s.rootLevel() && watched_ == 3u) {
		watched_ = c;
		for (uint32 i = 1, end = size(); i != end; ++i) {
			s.removeWatch(lit(i, c), this);
		}
	}
	// the constraint is not yet satisfied; update it and
	// check if we can now propagate any literals.
	updateConstraint(s, idx, c);
	if (bound_[c] <= 0 || (isWeight() && litSeen(0))) {
		uint32 reasonData = !isWeight() ? UINT32_MAX : up_;
		if (!litSeen(0)) {
			// forward propagate constraint to true
			active_ = c;
			return PropResult(s.force(body, this, reasonData), true);
		}
		else {
			// backward propagate false constraint
			uint32 n = getBpIndex();
			for (const uint32 end = size(); n != end && (bound_[c] - weight(n)) < 0; ++n) {
				if (!litSeen(n)) {
					active_   = c;
					Literal x = lit(n, c);
					if (!s.force(x, this, reasonData)) {
						return PropResult(false, true);
					}
				}
			}
			assert(n == 1 || n == size() || isWeight());
			setBpIndex(n);
		}
	}
	return PropResult(true, true);
}

// Builds the reason for p from the undo stack of this constraint.
// The reason will only contain literals that were processed by the 
// active sub-constraint.
void WeightConstraint::reason(Solver& s, Literal p, LitVec& r) {
	assert(active_ != NOT_ACTIVE);
	Literal x;
	uint32 stop = !isWeight() ? up_ : s.reasonData(p);
	assert(stop <= up_);
	for (uint32 i = undoStart(); i != stop; ++i) {
		UndoInfo u = undo_[i];
		// Consider only lits that are relevant to the active constraint
		if (u.constraint() == (ActiveConstraint)active_) {
			x = lit(u.idx(), u.constraint());
			r.push_back( ~x );
		}
	}
}

bool WeightConstraint::minimize(Solver& s, Literal p, CCMinRecursive* rec) {
	assert(active_ != NOT_ACTIVE);
	Literal x;
	uint32 stop = !isWeight() ? up_ : s.reasonData(p);
	assert(stop <= up_);
	for (uint32 i = undoStart(); i != stop; ++i) {
		UndoInfo u = undo_[i];
		// Consider only lits that are relevant to the active constraint
		if (u.constraint() == (ActiveConstraint)active_) {
			x = lit(u.idx(), u.constraint());
			if (!s.ccMinimize(~x, rec)) {
				return false;
			}
		}
	}
	return true;
}

// undoes processed assignments 
void WeightConstraint::undoLevel(Solver& s) {
	setBpIndex(1);
	for (UndoInfo u; up_ != undoStart() && s.value(lits_->var((u=undoTop()).idx())) == value_free;) {
		assert(litSeen(u.idx()));
		toggleLitSeen(u.idx());
		bound_[u.constraint()] += weight(u.idx());
		--up_;
	}
	if (!litSeen(0)) {
		active_ = NOT_ACTIVE;
		if (watched_ < 2u) {
			ActiveConstraint other = static_cast<ActiveConstraint>(watched_^1);
			for (uint32 i = 1, end = size(); i != end; ++i) {
				addWatch(s, i, other);
			}
			watched_ = 3u;
		}
	}
}

bool WeightConstraint::simplify(Solver& s, bool) {
	if (bound_[0] <= 0 || bound_[1] <= 0) {
		for (uint32 i = 0, end = size(); i != end; ++i) {
			s.removeWatch( lits_->lit(i), this );
			s.removeWatch(~lits_->lit(i), this );
		}
		return true;
	}
	else if (s.value(lits_->var(0)) != value_free && (active_ == NOT_ACTIVE || isWeight())) {
		if (active_ == NOT_ACTIVE) {
			Literal W = ~lits_->lit(0);
			active_   = FFB_BTB+s.isFalse(W);	
		}
		for (uint32 i = 0, end = size(); i != end; ++i) {
			s.removeWatch(lit(i, (ActiveConstraint)active_), this);
		}
	}
	if (lits_->unique() && size() > 4 && (up_ - undoStart()) > size()/2) {
		Literal*     lits = lits_->lits;
		const uint32 inc  = 1 + lits_->weights();
		uint32       end  = lits_->size()*inc;
		uint32 i, j, idx  = 1;
		// find first assigned literal - must be there otherwise undo stack would be empty
		for (i = inc; s.value(lits[i].var()) == value_free; i += inc) {
			assert(!litSeen(idx));
			++idx;
		}
		// move unassigned literals down
		// update watches because indices have changed
		for (j = i, i += inc; i != end; i += inc) {
			if (s.value(lits[i].var()) == value_free) {
				lits[j] = lits[i];
				if (lits_->weights()) { lits[j+1] = lits[i+1]; }
				undo_[idx].data = 0;
				assert(!litSeen(idx));
				if (Clasp::GenericWatch* w = s.getWatch(lits[i], this)) {
					w->data = (idx<<1) + 1;
				}
				if (Clasp::GenericWatch* w = s.getWatch(~lits[i], this)) {
					w->data = (idx<<1) + 0;
				}
				j += inc;
				++idx;
			}
			else {
				s.removeWatch(lits[i], this);
				s.removeWatch(~lits[i], this);
			}
		}
		// clear undo stack & update to new size
		up_ = undoStart();
		setBpIndex(1);
		lits_->sz = idx;
	}
	return false;
}

uint32 WeightConstraint::estimateComplexity(const Solver& s) const {
	weight_t B = std::min(bound_[0], bound_[1]);
	uint32 r   = 2;
	for (uint32 i = 1; i != size() && B > 0; ++i) {
		if (s.value(lits_->var(i)) == value_free) {
			++r;
			B -= weight(i);
		}
	}
	return r;
}
}
