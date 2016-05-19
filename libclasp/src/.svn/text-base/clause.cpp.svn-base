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

#include <clasp/clause.h>
#include <clasp/solver.h>


namespace Clasp { namespace Detail {

struct GreaterLevel {
	GreaterLevel(const Solver& s) : solver_(s) {}
	bool operator()(const Literal& p1, const Literal& p2) const {
		assert(solver_.value(p1.var()) != value_free && solver_.value(p2.var()) != value_free);
		return solver_.level(p1.var()) > solver_.level(p2.var());
	}
private:
	GreaterLevel& operator=(const GreaterLevel&);
	const Solver& solver_;
};

}

/////////////////////////////////////////////////////////////////////////////////////////
// ClauseCreator
/////////////////////////////////////////////////////////////////////////////////////////
ClauseCreator::ClauseCreator(Solver* s) 
	: solver_(s)
	, type_(Constraint_t::native_constraint)
	, w2_(0)
	, sat_(0)
	, unit_(0) {
}

ClauseCreator& ClauseCreator::start(ConstraintType t) {
	assert(!solver_ || solver_->decisionLevel() == 0 || t != Constraint_t::native_constraint);
	literals_.clear();
	type_ = t;
	w2_   = t != Constraint_t::native_constraint;
	sat_  = 0;
	unit_ = 0;
	return *this;
}

ClauseCreator& ClauseCreator::startAsserting(ConstraintType t, const Literal& p) {
	literals_.assign(1, p);
	type_ = t;
	w2_   = 1;
	unit_ = 1;
	sat_  = 0;
	return *this;
}

ClauseCreator& ClauseCreator::add(const Literal& p) {
	assert(solver_); const Solver& s = *solver_;
	if (s.value(p.var()) == value_free || s.level(p.var()) > 0) {
		literals_.push_back(p);
		if (unit_ != 0 && s.level(p.var()) > s.level(literals_[w2_].var())) {
			// make sure w2_ points to the false lit assigned last
			w2_ = (uint32)literals_.size()-1;
		}
		else if (unit_ == 0 && type_ != Constraint_t::native_constraint && literals_.size()>1) {
			// make sure lits[0] and lits[w2_] are the valid watches
			uint32 lp = s.isFalse(p) ? s.level(p.var()) : uint32(-1);
			if (s.isFalse(literals_[0]) && lp > s.level(literals_[0].var())) {
				std::swap(literals_[0], literals_.back());
				lp = s.level(literals_.back().var());
			}
			if (s.isFalse(literals_[w2_]) && lp > s.level(literals_[w2_].var())) {
				w2_ = (uint32)literals_.size()-1;
			}
		}
	}
	else if (s.isTrue(p)) { sat_ = 1; }
	return *this;
}

void ClauseCreator::simplify() {
	if (literals_.empty()) return;
	w2_          = 0;
	uint32 w2Lev = 0;
	solver_->markSeen(literals_[0]);
	LitVec::size_type i, j = 1;
	for (i = 1; i != literals_.size(); ++i) {
		Literal x = literals_[i];
		if (solver_->seen(~x)) { sat_ = 1; }
		if (!solver_->seen(x)) {
			solver_->markSeen(x);
			if (i != j) { literals_[j] = literals_[i]; }
			if (w2Lev != uint32(-1)) {
				uint32 xLev = solver_->value(x.var()) == value_free ? uint32(-1) : solver_->level(x.var());
				if (w2Lev < xLev) {
					w2Lev = xLev;
					w2_   = (uint32)j;
				}
			}
			++j;
		}
	}
	literals_.erase(literals_.begin()+j, literals_.end());
	for (LitVec::iterator it = literals_.begin(), end = literals_.end(); it != end; ++it) {
		solver_->clearSeen(it->var());
	}
}

bool ClauseCreator::createClause(Solver& s, ConstraintType type, const LitVec& lits, uint32 sw, Constraint** out) {
	if (out) *out = 0;
	if (lits.empty()) {
		LitVec cfl(1, Literal());
		s.setConflict(cfl);
		return false;           // UNSAT on level 0
	}
	else if (type == Constraint_t::native_constraint && s.strategies().satPrePro.get()) {
		return s.strategies().satPrePro->addClause(lits);
	}
	s.strategies().heuristic->newConstraint(s, &lits[0], lits.size(), type);
	bool asserting = lits.size() == 1 || s.isFalse(lits[sw]);
	if (lits.size() < 4) {
		if (type != Constraint_t::native_constraint) s.stats.solve.addLearnt((uint32)lits.size(), type);
		if (lits.size() == 1) {
			return s.addUnary(lits[0]);
		}
		else if (lits.size() == 2) {
			return s.addBinary(lits[0], lits[1], asserting); 
		}
		return s.addTernary(lits[0], lits[1], lits[2], asserting);
	}
	else {                          // general clause
		Constraint* newCon;
		Literal first = lits[0];
		if (type == Constraint_t::native_constraint) {
			newCon = Clause::newClause(s, lits);
			s.add(newCon);
		}
		else {
			assert(sw != 0);
			Clause* c = (Clause*)Clause::newLearntClause(s, lits, type, sw);
			s.addLearnt(c, (uint32)lits.size());
			newCon = c;
		}
		if (out) *out = newCon;
		return !asserting || s.force(first, newCon);
	}
}

Literal& ClauseCreator::operator[](LitVec::size_type i) {
	assert( i < literals_.size() );
	return literals_[i];
}
LitVec::size_type ClauseCreator::size() const {
	return literals_.size();
}

/////////////////////////////////////////////////////////////////////////////////////////
// Clause
/////////////////////////////////////////////////////////////////////////////////////////
Constraint* Clause::newClause(Solver& s, const LitVec& lits) {
	LitVec::size_type nl = 2 + lits.size(); // 2 sentinels
	void* mem = ::operator new( sizeof(Clause) + (nl*sizeof(Literal)) );
	return new (mem) Clause(s, lits, 0, Constraint_t::native_constraint, 0);
}

LearntConstraint* Clause::newLearntClause(Solver& s, const LitVec& lits, ConstraintType t, LitVec::size_type secondWatch) {
	assert(t != Constraint_t::native_constraint);
	LitVec::size_type nl = 3 + lits.size(); // 2 sentinels + 1 lit for activity
	void* mem = ::operator new( sizeof(Clause) + (nl * sizeof(Literal)) );
	return new (mem) Clause(s, lits, secondWatch, t, 0);
}

LearntConstraint* Clause::newContractedClause(Solver& s, const LitVec& lits, LitVec::size_type sw, LitVec::size_type tailStart) {
	LitVec::size_type nl = 3 + lits.size(); // 2 sentinels + 1 lit for activity
	void* mem = ::operator new( sizeof(Clause) + (nl * sizeof(Literal)) );
	return new (mem) Clause(s, lits, sw, Constraint_t::learnt_conflict, (uint32)tailStart);
}

Clause::Clause(Solver& s, const LitVec& theLits, LitVec::size_type sw, ConstraintType t, uint32 tail) {
	assert(int32(t) < 4);
	size_   = (uint32)theLits.size();
	type_   = t;
	lits()->asUint()  = 1;  // Starting sentinel  - var 0 + watch-flag on
	end()->asUint()   = 1;  // Ending Sentinel    - var 0 + watch-flag on
	other_  = theLits[0];
	std::memcpy(begin(), &theLits[0], sizeof(Literal) * theLits.size());
	if (t == Constraint_t::native_constraint) {
		initWatches(s);
	}
	else {
		act()   = (uint32)s.stats.solve.restarts + 1;
		if (tail > 0) {
			assert(sw < tail);
			Literal* newEnd = begin()+tail;
			if (newEnd != end()) {
				*newEnd = ~*newEnd;
				newEnd->watch();
				size_ = newEnd - begin();
			}
		}
		else if (s.isFalse(theLits[sw]) && size_ >= s.strategies().compress()) {
			contract(s);
			sw = 1;
		}
		initWatches(s, 0, uint32(sw));
	}
}


void Clause::destroy() {
	void* mem = static_cast<Constraint*>(this);
	this->~Clause();
	::operator delete(mem);
}

void Clause::initWatches(Solver& s, uint32 fw, uint32 sw) {
	Literal* first = begin() + fw;
	Literal* second = begin() + sw;
	fw = static_cast<uint32>(first - lits_);
	sw = static_cast<uint32>(second - lits_);
	bool fs = first < second;
	s.addWatch(~(*first), this, (fw<<1) + int32(fs) );
	s.addWatch(~(*second), this, (sw<<1) + int32(!fs) );
	first->watch();
	second->watch();
}

void Clause::initWatches(Solver& s) {
	if (s.strategies().randomWatches) {
		uint32 fw = irand(size_);
		uint32 sw;
		while ( (sw = irand(size_)) == fw) {/*intentionally empty*/}
		initWatches(s, fw, sw);
	}
	else {
		uint32 watch[2] = {0, size_-1};
		uint32 count[2] = {s.numWatches(~(*this)[0]), s.numWatches(~(*this)[size_-1])};
		uint32 maxCount = count[0] < count[1];
		for (uint32 x = 1, end = size_-1; count[maxCount] > 0u && x != end; ++x) {
			uint32 cxw = s.numWatches(~(*this)[x]);
			if (cxw < count[maxCount]) {
				if (cxw < count[1-maxCount]) {
					watch[maxCount]   = watch[1-maxCount];
					count[maxCount]   = count[1-maxCount];
					watch[1-maxCount] = x;
					count[1-maxCount] = cxw;
				}
				else {
					watch[maxCount]   = x;
					count[maxCount]   = cxw;
				}
			}
		}
		initWatches(s, watch[0], watch[1]);
	}
}

void Clause::undoLevel(Solver& s) {
	Literal* r = end();
	*r = ~*r;           // restore original literal, implicitly resets the watch-flag!
	for (++r; !isSentinel(*r) && s.value(r->var()) == value_free; ++r) { ; }
	if (!isSentinel(*r)) {
		assert(s.level(r->var()) != 0 && "Contracted clause may not contain literals from level 0");
		*r = ~*r;         // Note: ~r is true!
		r->watch();       // create a new artificial ending sentinel
		s.addUndoWatch(s.level(r->var()), this);
	}
	size_ = r - begin();
}

// Note: asserted lit is stored in other_ to
// make O(1) implementation of locked() possible and to speed up isSatisfied.
// Note: In previous versions, the asserted literal was stored in SL and SL was reset to 
// a sentinel on entry of propagate. The old scheme, although more space efficient, 
// empircally performs worse on long clauses. I guess this is due to cache misses 
// resulting from the forced write to the beginning of the literal array even if the 
// search only takes place in the middle or the end of the array.
Constraint::PropResult Clause::propagate(const Literal&, uint32& data, Solver& s) {
	if (s.isTrue(other_)) {                   // ignore clause; it is 
		return PropResult(true, true);          // already satisfied
	}
	// At this point other_ is either free or false.
	// Once we stumble upon the other watched literal
	// we'll store it in other_. Note that in case of a conflicting
	// clause, other_ will not necessarily be assigned to the other watched literal.
	// Nevertheless since other_ is always equal to one of the literals in the clause
	// it will be a false literal and thus trying to assign it to true
	// will force a conflict.
	uint32 run  = data>>1;                    // let run point to the false literal.            
	int   dir   = ((data & 1) << 1) - 1;      // either +1 or -1
	int   bounds= 0;                          // number of array bounds seen - 2 means clause is active
	for (;;) {
		for (run+=dir;s.isFalse(lits_[run]);run+=dir) ; // search non-false literal - sentinels guarantee termination
		if (!lits_[run].watched()) {                    // found a new watchable literal
			lits_[data>>1].clearWatch();                  // remove old watch
			lits_[run].watch();                           // and add the new one
			s.addWatch(~lits_[run], this, static_cast<uint32>(run << 1) + (dir==1));
			return Constraint::PropResult(true, false);
		}
		if (isBound(run, data)) {                       // Hit a bound,
			if (++bounds == 2) {                          // both ends seen, clause is unit, false, or sat
				return Constraint::PropResult(s.force(other_, this), true); 
			}
			run   = data >> 1;                            // halfway through, restart search, but
			dir   *= -1;                                  // this time walk in the opposite direction.
			data  ^= 1;                                   // Save new direction of watch
		}
		else {                                          // Hit the other watched literal
			other_  = lits_[run];                         // Store it in other_
			// At this point we could check, if other_ is true 
			// and if so terminate the search.
			// But although this approach means less work *right now*, continuing to
			// search for a new watch empirically seems to save work in the long run.
		}
	}
}

void Clause::reason(const Literal& p, LitVec& rLits) {
	assert(other_ == p);
	const Literal* e = end();
	for (Literal* r = begin(); r != e; ++r) {
		if (*r != p) {
			rLits.push_back(~*r);
		}
	}
	if (!isSentinel(*e)) {    // clause is contracted - add remaining literals to reason
		rLits.push_back( *e );  // this one was already inverted in contract
		for (++e; !isSentinel(*e); ++e) {
			rLits.push_back(~*e);
		}
	}
	bumpActivity();
}

bool Clause::simplify(Solver& s, bool reinit) {
	assert(s.decisionLevel() == 0);
	if (s.isTrue(other_)) {
		// clause is unit and therefore SAT
		Clause::removeWatches(s);
		return true;
	} 
	Literal* watches[2] = {0, 0};
	uint32 wc = 0;
	bool sat = false;
	Literal* j = end();
	for (Literal* i = begin(); i != j && (!sat || wc < 2);) {
		if (!s.isFalse( *i )) {
			if (i->watched()) {
				watches[wc++] = i;
			}
			sat = sat || s.isTrue(*i);
			++i;
		}
		else {
			assert(! i->watched() );
			*i = *(--j);
		}
	}
	j->asUint() = 1;  // Ending sentinel
	if (sat || size_ != uint32(j-begin()) || reinit) {
		size_   = j-begin();
		other_  = *begin();
		for (uint32 i = 0; i < wc; ++i) {
			watches[i]->clearWatch();
			s.removeWatch(~*watches[i], this);
		}
		if (sat)      { return true; }
		if (size_==2) { return s.addBinary(*begin(), *(begin()+1), false); }
		if (size_==3) { return s.addTernary(*begin(), *(begin()+1), *(begin()+2),false); }
		if (reinit)   { std::random_shuffle(begin(), end(), irand); }
		initWatches(s);
	}
	return false;
}

bool Clause::locked(const Solver& s) const {
	return s.isTrue(other_) && s.reason(other_) == this;
}

void Clause::removeWatches(Solver& s) {
	Literal* r = begin();
	for (int32 w = 0; w < 2; ++r) {
		if (r->watched()) {
			r->clearWatch();
			s.removeWatch(~(*r), this);
			++w;
		}
	}
	if (!isSentinel(*end())) {
		s.removeUndoWatch(s.level( end()->var() ), this );
		*end() = ~*end();
	}
}

bool Clause::isSatisfied(const Solver& s, LitVec& freeLits) const {
	if (s.isTrue(other_)) return true;
	const Literal* r = begin();
	while (!s.isTrue(*r)) {
		if (!s.isFalse(*r)) {
			freeLits.push_back(*r);
		}
		++r;
	}
	if (r != end()) {
		other_ = *r;  // cache true literal
		return true;
	}
	return false;
}

void Clause::contract(Solver& s) {
	assert(s.decisionLevel() > 0);
	// Pre: *begin() is the asserted literal, literals in [begin()+1, end()) are false
	// step 1: sort by decreasing decision level
	std::stable_sort(begin()+1, end(), Detail::GreaterLevel(s));

	// step 2: ignore level 0 literals - shouldn't be there in the first place
	Literal* newEnd = end();
	for (; s.level( (newEnd-1)->var() ) == 0; --newEnd) { ; }
	newEnd->asUint() = 1; // terminating sentinel
	size_ = newEnd - begin();

	// step 3: Determine the "active" part of the array.
	// The "active" part will contain only literals from the highest decision level.
	// Literals assigned earlier are temporarily removed from the clause.
	Literal xDL = s.decisionLevel() > 1 
		? s.decision(s.decisionLevel()-1)
		: Literal();
	newEnd      = std::lower_bound(begin()+2, end(), xDL, Detail::GreaterLevel(s));
	if (newEnd != end()) {
		// contract the clause by creating an artificial ending sentinel, i.e.
		// an out-of-bounds literal that is true and watched.
		*newEnd = ~*newEnd;
		newEnd->watch();
		s.addUndoWatch(s.level(newEnd->var()), this); 
		size_ = newEnd - begin();
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// LoopFormula
/////////////////////////////////////////////////////////////////////////////////////////
LoopFormula::LoopFormula(Solver& s, uint32 size, Literal* bodyLits, uint32 numBodies, uint32 bodyToWatch) {
	activity_       = (uint32)s.stats.solve.restarts + (size-numBodies);  
	end_            = numBodies + 2;
	size_           = end_+1;
	other_          = end_-1;
	lits_[0]        = Literal();  // Starting sentinel
	lits_[end_-1]   = Literal();  // Position of active atom
	lits_[end_]     = Literal();  // Ending sentinel - active part
	for (uint32 i = size_; i != size+3; ++i) {
		lits_[i] = Literal();
	}

	// copy bodies: S B1...Bn, watch one
	std::memcpy(lits_+1, bodyLits, numBodies * sizeof(Literal));
	s.addWatch(~lits_[1+bodyToWatch], this, ((1+bodyToWatch)<<1)+1);
	lits_[1+bodyToWatch].watch();
}

void LoopFormula::destroy() {
	void* mem = static_cast<Constraint*>(this);
	this->~LoopFormula();
	::operator delete(mem);
}


void LoopFormula::addAtom(Literal atom, Solver& s) {
	uint32 pos = size_++;
	assert(isSentinel(lits_[pos]));
	lits_[pos] = atom;
	lits_[pos].watch();
	s.addWatch( ~lits_[pos], this, (pos<<1)+0 );
	if (isSentinel(lits_[end_-1])) {
		lits_[end_-1] = lits_[pos];
	}
}

void LoopFormula::updateHeuristic(Solver& s) {
	Literal saved = lits_[end_-1];
	for (uint32 x = end_+1; x != size_; ++x) {
		lits_[end_-1] = lits_[x];
		s.strategies().heuristic->newConstraint(s, lits_+1, end_-1, Constraint_t::learnt_loop);
	}
	lits_[end_-1] = saved;
}

bool LoopFormula::watchable(const Solver& s, uint32 idx) {
	assert(!lits_[idx].watched());
	if (idx == end_-1) {
		for (uint32 x = end_+1; x != size_; ++x) {
			if (s.isFalse(lits_[x])) {
				lits_[idx] = lits_[x];
				return false;
			}
		}
	}
	return true;
}

bool LoopFormula::isTrue(const Solver& s, uint32 idx) {
	if (idx != end_-1) return s.isTrue(lits_[idx]);
	for (uint32 x = end_+1; x != size_; ++x) {
		if (!s.isTrue(lits_[x])) {
			lits_[end_-1] = lits_[x];
			return false;
		}
	}
	return true;
}

Constraint::PropResult LoopFormula::propagate(const Literal&, uint32& data, Solver& s) {
	if (isTrue(s, other_)) {          // ignore clause, as it is 
		return PropResult(true, true);  // already satisfied
	}
	uint32  pos   = data >> 1;
	uint32  idx   = pos;
	if (pos > end_) {
		// p is one of the atoms - move to active part
		lits_[end_-1] = lits_[pos];
		idx           = end_-1;
	}
	int     dir   = ((data & 1) << 1) - 1;
	int     bounds= 0;
	for (;;) {
		for (idx+=dir;s.isFalse(lits_[idx]);idx+=dir) {;} // search non-false literal - sentinels guarantee termination
		if (isSentinel(lits_[idx])) {             // Hit a bound,
			if (++bounds == 2) {                    // both ends seen, clause is unit, false, or sat
				if (other_ == end_-1) {
					uint32 x = end_+1;
					for (; x != size_ && s.force(lits_[x], this);  ++x) { ; }
					return Constraint::PropResult(x == size_, true);  
				}
				else {
					return Constraint::PropResult(s.force(lits_[other_], this), true);  
				}
			}
			idx   = std::min(pos, end_-1);          // halfway through, restart search, but
			dir   *= -1;                            // this time walk in the opposite direction.
			data  ^= 1;                             // Save new direction of watch
		}
		else if (!lits_[idx].watched() && watchable(s, idx)) { // found a new watchable literal
			if (pos > end_) {     // stop watching atoms
				lits_[end_-1].clearWatch();
				for (uint32 x = end_+1; x != size_; ++x) {
					if (x != pos) {
						s.removeWatch(~lits_[x], this);
						lits_[x].clearWatch();
					}
				}
			}
			lits_[pos].clearWatch();
			lits_[idx].watch();
			if (idx == end_-1) {  // start watching atoms
				for (uint32 x = end_+1; x != size_; ++x) {
					s.addWatch(~lits_[x], this, static_cast<uint32>(x << 1) + 0);
					lits_[x].watch();
				}
			}
			else {
				s.addWatch(~lits_[idx], this, static_cast<uint32>(idx << 1) + (dir==1));
			}
			return Constraint::PropResult(true, false);
		} 
		else if (lits_[idx].watched()) {          // Hit the other watched literal
			other_  = idx;                          // Store it in other_
		}
	}
}

// Body: all other bodies + active atom
// Atom: all bodies
void LoopFormula::reason(const Literal& p, LitVec& lits) {
	// all relevant bodies
	for (uint32 x = 1; x != end_-1; ++x) {
		if (lits_[x] != p) {
			lits.push_back(~lits_[x]);
		}
	}
	// if p is a body, add active atom
	if (other_ != end_-1) {
		lits.push_back(~lits_[end_-1]);
	}
	++activity_;
}

uint32 LoopFormula::size() const {
	return size_ - 3;
}

bool LoopFormula::locked(const Solver& s) const {
	if (other_ != end_-1) {
		return s.isTrue(lits_[other_]) && s.reason(lits_[other_]) == this;
	}
	for (uint32 x = end_+1; x != size_; ++x) {
		if (s.isTrue(lits_[x]) && s.reason(lits_[x]) == this) {
			return true;
		}
	}
	return false;
}

void LoopFormula::removeWatches(Solver& s) {
	for (uint32 x = 1; x != end_-1; ++x) {
		if (lits_[x].watched()) {
			s.removeWatch(~lits_[x], this);
			lits_[x].clearWatch();
		}
	}
	if (lits_[end_-1].watched()) {
		lits_[end_-1].clearWatch();
		for (uint32 x = end_+1; x != size_; ++x) {
			s.removeWatch(~lits_[x], this);
			lits_[x].clearWatch();
		}
	}
}

bool LoopFormula::isSatisfied(const Solver& s, LitVec& freeLits) const {
	if (other_ != end_-1 && s.isTrue(lits_[other_])) return true;
	for (uint32 x = 1; x != end_-1; ++x) {
		if (s.isTrue(lits_[x])) {
			other_ = x;
			return true;
		}
		else if (!s.isFalse(lits_[x])) { freeLits.push_back(lits_[x]); }
	}
	bool sat = true;
	for (uint32 x = end_+1; x != size_; ++x) {
		if (s.value(lits_[x].var()) == value_free) {
			freeLits.push_back(lits_[x]);
			sat = false;
		}
		else sat &= s.isTrue(lits_[x]);
	}
	return sat;
}

bool LoopFormula::simplify(Solver& s, bool) {
	assert(s.decisionLevel() == 0);
	typedef std::pair<uint32, uint32> WatchPos;
	bool      sat = false;          // is the constraint SAT?
	WatchPos  bodyWatches[2];       // old/new position of watched bodies
	uint32    bw  = 0;              // how many bodies are watched?
	uint32    j   = 1, i;
	// 1. simplify the set of bodies:
	// - search for a body that is true -> constraint is SAT
	// - remove all false bodies
	// - find the watched bodies
	for (i = 1; i != end_-1; ++i) {
		assert( !s.isFalse(lits_[i]) || !lits_[i].watched() ); // otherwise should be asserting 
		if (!s.isFalse(lits_[i])) {
			sat |= s.isTrue(lits_[i]);
			if (i != j) { lits_[j] = lits_[i]; }
			if (lits_[j].watched()) { bodyWatches[bw++] = WatchPos(i, j); }
			++j;
		}
	}
	uint32  newEnd    = j + 1;
	uint32  numBodies = j - 1;
	j += 2;
	// 2. simplify the set of atoms:
	// - remove all determined atoms
	// - remove/update watches if necessary
	for (i = end_ + 1; i != size_; ++i) {
		if (s.value(lits_[i].var()) == value_free) {
			if (i != j) { lits_[j] = lits_[i]; }
			if (lits_[j].watched()) {
				if (sat || numBodies <= 2) {
					s.removeWatch(~lits_[j], this);
					lits_[j].clearWatch();
				}
				else if (i != j) {
					Watch* w  = s.getWatch(~lits_[j], this);
					assert(w);
					w->data = (j << 1) + 0;
				}
			}
			++j;
		}
		else if (lits_[i].watched()) {
			s.removeWatch(~lits_[i], this);
			lits_[i].clearWatch();
		}
	}
	size_         = j;
	end_          = newEnd;
	lits_[end_]   = Literal();
	lits_[end_-1] = lits_[end_+1];
	if (sat || numBodies < 3 || size_ == end_ + 1) {
		for (i = 0; i != bw; ++i) {
			s.removeWatch(~lits_[bodyWatches[i].second], this);
			lits_[bodyWatches[i].second].clearWatch();
		}
		if (sat || size_ == end_+1) { return true; }
		// replace constraint with short clauses
		ClauseCreator creator(&s);
		creator.start();
		for (i = 1; i != end_; ++i) { creator.add(lits_[i]); }
		for (i = end_+1; i != size_; ++i) {
			creator[creator.size()-1] = lits_[i];
			creator.end();
		}
		return true;
	}
	other_ = 1;
	for (i = 0; i != bw; ++i) {
		if (bodyWatches[i].first != bodyWatches[i].second) {
			Watch* w  = s.getWatch(~lits_[bodyWatches[i].second], this);
			assert(w);
			w->data = (bodyWatches[i].second << 1) + (w->data&1);
		}
	}
	return false;
}
}
