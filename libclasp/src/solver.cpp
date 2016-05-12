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
#include <clasp/solver.h>
#include <clasp/clause.h>
#include <clasp/util/misc_types.h>
#include <functional>
#include <stdexcept>

#ifdef _MSC_VER
#pragma warning (disable : 4355) // 'this' used in base member initializer list - intended & safe
#pragma warning (disable : 4702) // unreachable code - intended
#endif

namespace Clasp { 

uint32 randSeed_g = 1;

SatPreprocessor::~SatPreprocessor() {}
DecisionHeuristic::~DecisionHeuristic() {}

/////////////////////////////////////////////////////////////////////////////////////////
// SelectFirst selection strategy
/////////////////////////////////////////////////////////////////////////////////////////
// selects the first free literal
Literal SelectFirst::doSelect(Solver& s) {
	for (Var i = 1; i <= s.numVars(); ++i) {
		if (s.value(i) == value_free) {
			return s.preferredLiteralByType(i);
		}
	}
	assert(!"SelectFirst::doSelect() - precondition violated!\n");
	return Literal();
}
/////////////////////////////////////////////////////////////////////////////////////////
// SelectRandom selection strategy
/////////////////////////////////////////////////////////////////////////////////////////
// Selects a random literal from all free literals.
class SelectRandom : public DecisionHeuristic {
public:
	SelectRandom() : randFreq_(1.0), pos_(0) {}
	void shuffle() {
		std::random_shuffle(vars_.begin(), vars_.end(), irand);
		pos_ = 0;
	}
	void   randFreq(double d) { randFreq_ = d; }
	double randFreq() const   { return randFreq_; }
	void   endInit(Solver& s) {
		vars_.clear();
		for (Var i = 1; i <= s.numVars(); ++i) {
			if (s.finalValue( i ) == value_free) {
				vars_.push_back(i);
			}
		}
		pos_ = 0;
	}
private:
	Literal doSelect(Solver& s) {
		LitVec::size_type old = pos_;
		do {
			if (s.value(vars_[pos_]) == value_free) {
				Literal l = savedLiteral(s, vars_[pos_]);
				return l != posLit(0)
					? l
					: s.preferredLiteralByType(vars_[pos_]);
			}
			if (++pos_ == vars_.size()) pos_ = 0;
		} while (old != pos_);
		assert(!"SelectRandom::doSelect() - precondition violated!\n");
		return Literal();
	}
	VarVec            vars_;
	double            randFreq_;
	VarVec::size_type pos_;
};
/////////////////////////////////////////////////////////////////////////////////////////
// Post propagator list
/////////////////////////////////////////////////////////////////////////////////////////
Solver::PPList::PPList() : head(0), look(0), saved(0) { }
Solver::PPList::~PPList() {
	for (PostPropagator* r = head; r;) {
		PostPropagator* t = r;
		r = r->next;
		t->destroy();
	}
}
void Solver::PPList::add(PostPropagator* p) {
	assert(p && p->next == 0);
	uint32 prio = p->priority();
	if (!head || prio == PostPropagator::priority_highest || prio < head->priority()) {
		p->next = head;
		head    = p;
	}
	else {
		for (PostPropagator* r = head; ; r = r->next) {
			if (r->next == 0) {
				r->next = p; break;
			}
			else if (prio < r->next->priority()) {
				p->next = r->next;
				r->next = p;
				break;
			}				
		}
	}
	if (prio >= PostPropagator::priority_lookahead && (!look || prio < look->priority())) {
		look = p;
	}
	saved = head;
}
void Solver::PPList::remove(PostPropagator* p) {
	assert(p);
	if (!head) return;
	if (p == head) {
		head = head->next;
		p->next = 0;
	}
	else {
		for (PostPropagator* r = head; ; r = r->next) {
			if (r->next == p) {
				r->next = r->next->next;
				p->next = 0;
				break;
			}
		}
	}
	if (p == look) { look = look->next; }
	saved = head;
}
bool Solver::PPList::propagate(Solver& s, PostPropagator* x) {
	if (x == head) return true;
	PostPropagator* p = head, *t;
	do {
		// just in case t removes itself from the list
		// during propagateFixpoint
		t = p;
		p = p->next;
		if (!t->propagateFixpoint(s)) { return false; }
	}
	while (p != x);
	return true;
}
bool PostPropagator::propagateFixpoint(Solver& s) {
	bool ok = propagate(s);
	while (ok && s.queueSize() > 0) {
		ok = s.propagateUntil(this) && propagate(s);
	}
	return ok;
}
void Solver::PPList::reset()            { for (PostPropagator* r = head; r; r = r->next) { r->reset(); } }
bool Solver::PPList::isModel(Solver& s) {
	saved = head;
	for (PostPropagator* r = head; r; r = r->next) {
		if (!r->isModel(s)) return false;
	}
	return true;
}
bool Solver::PPList::nextSymModel(Solver& s, bool expand) {
	for (; saved; saved = saved->next) {
		if (saved->nextSymModel(s, expand)) return true;
	}
	saved = head;
	return false;
}
/////////////////////////////////////////////////////////////////////////////////////////
// SolverStrategies
/////////////////////////////////////////////////////////////////////////////////////////
SolverStrategies::SolverStrategies()
	: satPrePro(0)
	, heuristic(new SelectFirst) 
	, symTab(0)
	, search(use_learning)
	, saveProgress(0) 
	, cflMinAntes(all_antes)
	, strengthenRecursive(false)
	, randomWatches(false)
	, compress_(250) {
}
/////////////////////////////////////////////////////////////////////////////////////////
// Solver: Construction/Destruction/Setup
////////////////////////////////////////////////////////////////////////////////////////
Solver::Solver() 
	: strategy_()
	, randHeuristic_(0)
	, levConflicts_(0)
	, undoHead_(0)
	, units_(0)
	, binCons_(0)
	, ternCons_(0)
	, lastSimplify_(0)
	, rootLevel_(0)
	, btLevel_(0)
	, eliminated_(0)
	, shuffle_(false) {
	// every solver contains a special sentinel var that is always true
	Var sentVar = addVar( Var_t::atom_body_var );
	assign_.setValue(sentVar, value_true);
	markSeen(sentVar);
}

Solver::~Solver() {
	freeMem();
}

void Solver::freeMem() {
	std::for_each( constraints_.begin(), constraints_.end(), DestroyObject());
	std::for_each( learnts_.begin(), learnts_.end(), DestroyObject() );
	constraints_.clear();
	learnts_.clear();
	PodVector<WL>::destruct(watches_);
	delete levConflicts_;
	delete randHeuristic_;
	// free undo lists
	// first those still in use
	for (TrailLevels::size_type i = 0; i != levels_.size(); ++i) {
		delete levels_[i].second;
	}
	// then those in the free list
	for (ConstraintDB* x = undoHead_; x; ) {
		ConstraintDB* t = x;
		x = (ConstraintDB*)x->front();
		delete t;
	}
}

void Solver::reset() {
	// hopefully, no one derived from this class...
	this->~Solver();
	new (this) Solver();
}
/////////////////////////////////////////////////////////////////////////////////////////
// Solver: Problem specification
////////////////////////////////////////////////////////////////////////////////////////
void Solver::startAddConstraints() {
	watches_.resize(info_.numVars()<<1);
	// pre-allocate some memory
	assign_.trail.reserve(numVars());
	levels_.reserve(25);
	if (undoHead_ == 0) {
		for (uint32 i = 0; i != 25; ++i) { 
			undoFree(new ConstraintDB(10)); 
		}
	}
	strategy_.heuristic->startInit(*this);
	if (strategies().satPrePro.get()) {
		strategies().satPrePro->setSolver(*this);
	}
	stats.problem.reset();
	stats.problem.vars = numVars();
}

bool Solver::endAddConstraints() {
	Antecedent::checkPlatformAssumptions();
	if (strategy_.satPrePro.get() != 0 && decisionLevel() == 0) {
		SolverStrategies::SatPrePro temp(strategy_.satPrePro.release());
		bool r = temp->preprocess();
		strategy_.satPrePro.reset(temp.release());
		stats.problem.eliminated = numEliminatedVars();
		if (!r) return false;
	}
	if (!propagate()) { return false; }
	strategy_.heuristic->endInit(*this);
	// Force propagation again - just in case
	// heuristic->endInit() added new information
	if (!propagate() || !simplify()) {
		return false;
	}
	stats.problem.constraints[0] = numConstraints();
	stats.problem.constraints[1] = numBinaryConstraints();
	stats.problem.constraints[2] = numTernaryConstraints();
	if (randHeuristic_) randHeuristic_->endInit(*this);
	return true;
}

uint32 Solver::problemComplexity() const {
	uint32 r = binCons_ + ternCons_;
	for (uint32 i = 0; i != constraints_.size(); ++i) {
		r += constraints_[i]->estimateComplexity(*this);
	}
	return r;
}

void Solver::reserveVars(uint32 numVars) {
	info_.reserve(numVars);
	assign_.reserve(numVars);
}

Var Solver::addVar(VarType t, bool eq) {
	Var v = info_.numVars();
	info_.add(t == Var_t::body_var);
	if (eq) info_.toggle(v, VarInfo::EQ);
	assign_.addVar();
	return v;
}

void Solver::eliminate(Var v, bool elim) {
	assert(validVar(v)); 
	if (elim && !eliminated(v)) {
		assert(value(v) == value_free && "Can not eliminate assigned var!\n");
		info_.toggle(v, VarInfo::ELIM);
		markSeen(v);
		// so that the var is ignored by heuristics
		assign_.setValue(v, value_true);
		++eliminated_;
	}
	else if (!elim && eliminated(v)) {
		info_.toggle(v, VarInfo::ELIM);
		clearSeen(v);
		assign_.clearValue(v);
		--eliminated_;
		strategy_.heuristic->resurrect(*this, v);
	}
}

bool Solver::addUnary(Literal p) {
	return addNewImplication(p, 0, Antecedent(posLit(0)));
}

bool Solver::addBinary(Literal p, Literal q) {
	assert(validWatch(~p) && validWatch(~q) && "ERROR: startAddConstraints not called!");
	++binCons_;
	watches_[(~p).index()].push_back(q);
	watches_[(~q).index()].push_back(p);
	return true;
}

bool Solver::addTernary(Literal p, Literal q, Literal r) {
	assert(validWatch(~p) && validWatch(~q) && validWatch(~r) && "ERROR: startAddConstraints not called!");
	assert(p != q && q != r && "ERROR: ternary clause contains duplicate literal");
	++ternCons_;
	watches_[(~p).index()].push_back(TernStub(q, r));
	watches_[(~q).index()].push_back(TernStub(p, r));
	watches_[(~r).index()].push_back(TernStub(p, q));
	return true;
}
void Solver::setStopConflict() {
	if (!hasConflict()) {
		// we use the nogood {FALSE} to represent the unrecoverable conflict -
		// note that {FALSE} can otherwise never be a violated nogood because
		// TRUE is always true in every solver
		conflict_.lits.push_back(negLit(0));
	}
	// artificially increase root level -
	// this way, the solver is prevented from resolving the conflict
	setRootLevel(decisionLevel());
	conflict_.level = decisionLevel();
}
bool Solver::hasStopConflict() const { return conflict_.lits[0] == negLit(0); }
bool Solver::clearAssumptions()  {
	rootLevel_ = btLevel_ = 0;
	undoUntil(0);
	assert(decisionLevel() == 0);
	if (!hasConflict() || hasStopConflict()) {
		conflict_.clear();
		for (ImpliedLits::size_type i = 0; i != impliedLits_.size(); ++i) {
			if (impliedLits_[i].level == 0 && !force(impliedLits_[i].lit, impliedLits_[i].ante)) {
				return false;
			}
		}
	}
	impliedLits_.clear();
	return simplify();
}
bool Solver::integrateClause(const LitVec& lits, bool addSat) {
	assert(!hasConflict()); // Precondition
	// Add clause to clause creator -
	// determine state of clause
	ClauseCreator nc(this); nc.start(Constraint_t::learnt_other);
	for (LitVec::size_type i = 0; i != lits.size(); ++i) {  nc.add(lits[i]);  }
	
	if (nc.end(addSat)) { return true; }
	if (nc.empty())     { return false; }
	// clause is conflicting 
	assert(nc.status() == ClauseCreator::status_conflicting);
	if (nc.conflictLevel() > rootLevel() && nc.conflictLevel() != nc.implicationLevel()) {
		// nc is already an asserting clause - i.e.
		// resolveConflict() would generate the exact same clause.
		undoUntil(nc.implicationLevel());
		return ClauseCreator::addClause(*this, nc.type(), nc.lits(), nc.sw());
	}
	undoUntil(nc.conflictLevel());
	if (decisionLevel() == rootLevel() || decisionLevel() > nc.conflictLevel()) {
		// remember the clause, because resolveConflict() won't learn anything 
		// from the conflict - either because root level conflicts are not resolved
		// or because the current backtracking level prevents learning.
		return ClauseCreator::addClause(*this, nc.type(), nc.lits(), nc.sw());
	}
	// store conflict
	conflict_.clear();
	conflict_.level = nc.conflictLevel();
	for (LitVec::size_type i = 0; i != nc.size(); ++i) {
		conflict_.lits.push_back( ~nc[i] );
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Solver: Watch management
////////////////////////////////////////////////////////////////////////////////////////
uint32 Solver::numWatches(Literal p) const {
	assert( validVar(p.var()) );
	if (!validWatch(p)) return 0;
	return (uint32)watches_[p.index()].size();
}
	
bool Solver::hasWatch(Literal p, Constraint* c) const {
	if (!validWatch(p)) return false;
	const GWL& pList = watches_[p.index()].genW;
	return std::find(pList.begin(), pList.end(), c) != pList.end();
}

Watch* Solver::getWatch(Literal p, Constraint* c) const {
	if (!validWatch(p)) return 0;
	const GWL& pList = watches_[p.index()].genW;
	GWL::const_iterator it = std::find(pList.begin(), pList.end(), c);
	return it != pList.end()
		? &const_cast<Watch&>(*it)
		: 0;
}

void Solver::removeWatch(const Literal& p, Constraint* c) {
	assert(validWatch(p));
	GWL& pList = watches_[p.index()].genW;
	GWL::iterator it = std::find(pList.begin(), pList.end(), c);
	if (it != pList.end()) {
		pList.erase(it);
	}
}

void Solver::removeUndoWatch(uint32 dl, Constraint* c) {
	assert(dl != 0 && dl <= decisionLevel() );
	if (levels_[dl-1].second) {
		ConstraintDB& uList = *levels_[dl-1].second;
		ConstraintDB::iterator it = std::find(uList.begin(), uList.end(), c);
		if (it != uList.end()) {
			*it = uList.back();
			uList.pop_back();
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// Solver: Basic DPLL-functions
////////////////////////////////////////////////////////////////////////////////////////
void Solver::initRandomHeuristic(double randFreq) {
	randFreq = std::min(1.0, std::max(0.0, randFreq));
	if (randFreq == 0.0) {
		delete randHeuristic_;
		randHeuristic_ = 0;
		return;
	}
	if (!randHeuristic_) {
		randHeuristic_ = new SelectRandom();
		randHeuristic_->endInit(*this);
	}
	static_cast<SelectRandom*>(randHeuristic_)->shuffle();
	static_cast<SelectRandom*>(randHeuristic_)->randFreq(randFreq);
}


// removes all satisfied binary and ternary clauses as well
// as all constraints for which Constraint::simplify returned true.
bool Solver::simplify() {
	if (decisionLevel() != 0) return true;
	if (hasConflict())        return false;
	if (lastSimplify_ != assign_.trail.size()) {
		LitVec::size_type old = lastSimplify_;
		if (!simplifySAT()) { return false; }
		assert(lastSimplify_ == assign_.trail.size());
		strategy_.heuristic->simplify(*this, old);
	}
	if (shuffle_) { simplifySAT(); }
	return true;
}

bool Solver::simplifySAT() {
	if (queueSize() > 0 && !propagate()) {
		return false;
	}
	assert(assign_.qEmpty());
	assign_.front = lastSimplify_;
	while (!assign_.qEmpty()) {
		simplifyShort(assign_.qPop()); // remove satisfied binary- and ternary clauses
	}
	lastSimplify_ = assign_.front;
	if (shuffle_) {
		std::random_shuffle(constraints_.begin(), constraints_.end(), irand);
		std::random_shuffle(learnts_.begin(), learnts_.end(), irand);
	}
	simplifyDB(constraints_);
	simplifyDB(learnts_);
	shuffle_ = false;
	return true;
}

void Solver::simplifyDB(ConstraintDB& db) {
	ConstraintDB::size_type i, j, end = db.size();
	for (i = j = 0; i != end; ++i) {
		Constraint* c = db[i];
		if (c->simplify(*this, shuffle_)) { c->destroy(); }
		else                              { db[j++] = c;  }
	}
	db.erase(db.begin()+j, db.end());
}

// removes all binary clauses containing p - those are now SAT
// binary clauses containing ~p are unit and therefore likewise SAT. Those
// are removed when their second literal is processed.
// Note: Binary clauses containing p are those that watch ~p.
//
// Simplifies ternary clauses.
// Ternary clauses containing p are SAT and therefore removed.
// Ternary clauses containing ~p are now either binary or SAT. Those that
// are SAT are removed when the satisfied literal is processed. 
// All conditional binary-clauses are replaced with a real binary clause.
// Note: Ternary clauses containing p watch ~p. Those containing ~p watch p.
// Note: Those clauses are now either binary or satisfied.
void Solver::simplifyShort(Literal p) {
	WL& pList     = watches_[p.index()];
	WL& negPList  = watches_[(~p).index()];
	releaseVec( pList.binW ); // this list was already propagated
	binCons_    -= (uint32)negPList.binW.size();
	for (LitVec::size_type i = 0; i < negPList.binW.size(); ++i) {
		remove_first_if(watches_[(~negPList.binW[i]).index()].binW, std::bind2nd(std::equal_to<Literal>(), p));
	}
	releaseVec(negPList.binW);
	
	// remove every ternary clause containing p -> clause is satisfied
	TWL& ptList = negPList.ternW;
	ternCons_   -= (uint32)ptList.size();
	for (LitVec::size_type i = 0; i < ptList.size(); ++i) {
		remove_first_if(watches_[(~ptList[i].first).index()].ternW, PairContains<Literal>(p));
		remove_first_if(watches_[(~ptList[i].second).index()].ternW, PairContains<Literal>(p));
	}
	releaseVec(ptList);
	// transform ternary clauses containing ~p to binary clause
	TWL& npList = pList.ternW;
	for (LitVec::size_type i = 0; i < npList.size(); ++i) {
		const Literal& q = npList[i].first;
		const Literal& r = npList[i].second;
		--ternCons_;
		remove_first_if(watches_[(~q).index()].ternW, PairContains<Literal>(~p));
		remove_first_if(watches_[(~r).index()].ternW, PairContains<Literal>(~p));
		if (value(q.var()) == value_free && value(r.var()) == value_free) {
			// clause is binary on dl 0
			addBinary(q, r);
		}
		// else: clause is SAT and removed when the satisfied literal is processed
	}
	releaseVec(npList);
	releaseVec(pList.genW);     // updated during propagation. 
	releaseVec(negPList.genW);  // ~p will never be true. List is no longer relevant.
}

bool Solver::force(const Literal& p, const Antecedent& c) {
	assert((!hasConflict() || isTrue(p)) && !eliminated(p.var()));
	if (assign_.assign(p, decisionLevel(), c)) {
		return true;
	}
	// else: conflict
	conflict_.level = decisionLevel();
	conflict_.lits.push_back(~p);
	if (strategy_.search != SolverStrategies::no_learning && !c.isNull()) {
		stats.solve.updateAntes(c.reason(p, conflict_.lits));
	}
	return false;
}

bool Solver::assume(const Literal& p) {
	assert( value(p.var()) == value_free && decisionLevel() != assign_.maxLevel());
	++stats.solve.choices;
	levels_.push_back(LevelInfo(numAssignedVars(), 0));
	if (levConflicts_) levConflicts_->push_back( (uint32)stats.solve.conflicts );
	return force(p, Antecedent());  // always true
}

bool Solver::propagate() {
	if (unitPropagate() && post_.propagate(*this, 0)) {
		assert(queueSize() == 0);
		return true;
	}
	assign_.qReset();
	post_.reset();
	return false;
}

uint32 Solver::mark(uint32 s, uint32 e) {
	while (s != e) { markSeen(assign_.trail[s++].var()); }
	return e;
}

bool Solver::unitPropagate() {
	assert(!hasConflict());
	Literal p;
	uint32 idx;
	while ( !assign_.qEmpty() ) {
		p       = assign_.qPop();
		idx     = p.index();
		WL& wl  = watches_[idx];
		LitVec::size_type i, bEnd = wl.binW.size(), tEnd = wl.ternW.size(), gEnd = wl.genW.size();
		// first, do binary BCP...    
		for (i = 0; i != bEnd; ++i) {
			if (!isTrue(wl.binW[i]) && !force(wl.binW[i], p)) {
				return false;
			}
		}
		// then, do ternary BCP...
		for (i = 0; i != tEnd; ++i) {
			Literal q = wl.ternW[i].first;
			Literal r = wl.ternW[i].second;
			if (isTrue(r) || isTrue(q)) continue;
			if (isFalse(r) && !force(q, Antecedent(p, ~r))) {
				return false;
			}
			else if (isFalse(q) && !force(r, Antecedent(p, ~q))) {
				return false;
			}
		}
		// and finally do general BCP
		if (gEnd != 0) {
			GWL& gWL = wl.genW;
			Constraint::PropResult r;
			LitVec::size_type j;
			for (j = 0, i = 0; i != gEnd; ) {
				Watch& w = gWL[i++];
				r = w.propagate(*this, p);
				if (r.second) { // keep watch
					gWL[j++] = w;
				}
				if (!r.first) {
					while (i != gEnd) {
						gWL[j++] = gWL[i++];
					}
					shrinkVecTo(gWL, j);
					return false;
				}
			}
			shrinkVecTo(gWL, j);
		}
	}
	return decisionLevel() > 0 || (units_=mark(units_, uint32(assign_.front))) == assign_.front;
}

bool Solver::test(Literal p, Constraint* c) {
	assert(value(p.var()) == value_free && !hasConflict());
	assume(p); --stats.solve.choices;
	if (unitPropagate() && post_.propagate(*this, post_.look)) {
		assert(decision(decisionLevel()) == p);
		if (c) c->undoLevel(*this);
		undoUntil(decisionLevel()-1);
		return true;
	}
	assert(decision(decisionLevel()) == p);
	assign_.qReset();
	post_.reset();
	return false;
}

void Solver::setConflict(LitVec& cfl, bool computeLevel) {
	conflict_.clear(); 
	uint32 maxDL = !computeLevel ? decisionLevel() : 0;
	for (LitVec::const_iterator it = cfl.begin(), end = cfl.end(); it != end && maxDL != decisionLevel(); ++it) {
		if (level(it->var()) > maxDL) {
			maxDL = level(it->var());
		}
	}
	conflict_.level = maxDL;
	conflict_.lits.swap(cfl);
}

bool Solver::resolveConflict() {
	assert(hasConflict() && conflict_.level <= decisionLevel());
	++stats.solve.conflicts;
	undoUntil(conflict_.level);
	if (decisionLevel() > rootLevel_) {
		if (decisionLevel() != btLevel_ && strategy_.search != SolverStrategies::no_learning) {
			uint32 sw;
			uint32 uipLevel = analyzeConflict(sw);
			stats.solve.updateJumps(decisionLevel(), uipLevel, btLevel_);
			undoUntil( uipLevel );
			assert(!hasConflict());
			return ClauseCreator::addClause(*this, Constraint_t::learnt_conflict, cc_, sw);
		}
		else {
			if (btLevel_ > conflict_.level) {
				setBacktrackLevel(conflict_.level);
				undoUntil(conflict_.level);
			}
			return backtrack();
		}
	}
	// do not count artificial conflicts that are only used
	// to stop the current search
	stats.solve.conflicts -= hasStopConflict();
	return false;
}

bool Solver::backtrack() {
	do {
		if (decisionLevel() == rootLevel_) return false;
		Literal lastChoiceInverted = ~decision(decisionLevel());
		btLevel_ = decisionLevel() - 1;
		undoUntil(btLevel_);
		if (!hasConflict() && force(lastChoiceInverted, 0)) {
			ImpliedLits::size_type j = 0;
			for (ImpliedLits::size_type i = 0; i < impliedLits_.size(); ++i) {
				if (impliedLits_[i].level <= btLevel_) {
					if (!hasConflict()) { force(impliedLits_[i].lit, impliedLits_[i].ante); }
					impliedLits_[j++] = impliedLits_[i];
				}
			}
			if (decisionLevel() == 0) { j = 0; }
			impliedLits_.erase(impliedLits_.begin()+j, impliedLits_.end());
		}
	} while (hasConflict());
	return true;
}

void Solver::undoUntil(uint32 level) {
	assert(btLevel_ >= rootLevel_);
	level = std::max( level, btLevel_ );
	if (level >= decisionLevel()) return;
	if (level < conflict_.level) {
		conflict_.clear();
	}
	strategy_.heuristic->undoUntil( *this, levels_[level].first);
	bool sp = strategy_.saveProgress > 0 && (decisionLevel() - level) > (uint32)strategy_.saveProgress;
	undoLevel(false);
	while (decisionLevel() != level) {
		undoLevel(sp);
	}
}

uint32 Solver::estimateBCP(const Literal& p, int rd) const {
	if (value(p.var()) != value_free) return 0;
	LitVec::size_type first = assign_.assigned();
	LitVec::size_type i     = first;
	Solver& self            = const_cast<Solver&>(*this);
	self.assign_.setValue(p.var(), trueValue(p));
	self.assign_.trail.push_back(p);
	do {
		Literal x = assign_.trail[i++];  
		const BWL& xList = watches_[x.index()].binW;
		for (LitVec::size_type k = 0; k < xList.size(); ++k) {
			Literal y = xList[k];
			if (value(y.var()) == value_free) {
				self.assign_.setValue(y.var(), trueValue(y));
				self.assign_.trail.push_back(y);
			}
		}
	} while (i < assign_.assigned() && rd-- != 0);
	i = assign_.assigned()-first;
	while (self.assign_.assigned() != first) {
		self.assign_.undoLast();
	}
	return (uint32)i;
}
/////////////////////////////////////////////////////////////////////////////////////////
// Solver: Private helper functions
////////////////////////////////////////////////////////////////////////////////////////
Solver::ConstraintDB* Solver::allocUndo(Constraint* c) {
	if (undoHead_ == 0) {
		return new ConstraintDB(1, c);
	}
	assert(undoHead_->size() == 1);
	ConstraintDB* r = undoHead_;
	undoHead_ = (ConstraintDB*)undoHead_->front();
	r->clear();
	r->push_back(c);
	return r;
}
void Solver::undoFree(ConstraintDB* x) {
	// maintain a single-linked list of undo lists
	x->clear();
	x->push_back((Constraint*)undoHead_);
	undoHead_ = x;
}
// removes the current decision level
void Solver::undoLevel(bool sp) {
	assert(decisionLevel() != 0 && levels_.back().first != assign_.trail.size() && "Decision Level must not be empty");
	assign_.undoTrail(levels_.back().first, sp);
	if (levels_.back().second) {
		const ConstraintDB& undoList = *levels_.back().second;
		for (ConstraintDB::size_type i = 0, end = undoList.size(); i != end; ++i) {
			undoList[i]->undoLevel(*this);
		}
		undoFree(levels_.back().second);
	}
	levels_.pop_back();
	if (levConflicts_) levConflicts_->pop_back();
}

// Computes the First-UIP clause and stores it in cc_, where cc_[0] is the asserting literal (inverted UIP).
// Returns the dl on which cc_ is asserting and stores the position of a literal 
// from this level in secondWatch.
uint32 Solver::analyzeConflict(uint32& secondWatch) {
	// must be called here, because we unassign vars during analyzeConflict
	strategy_.heuristic->undoUntil( *this, levels_.back().first );
	uint32 onLevel  = 0;        // number of literals from the current DL in resolvent
	uint32 abstr    = 0;        // abstraction of DLs in cc_
	Literal p;                  // literal to be resolved out next
	cc_.assign(1, p);           // will later be replaced with asserting literal
	LitVec& rhs    = conflict_.lits;
	strategy_.heuristic->updateReason(*this, rhs, p);
	for (;;) {
		for (LitVec::size_type i = 0; i != rhs.size(); ++i) {
			Literal& q = rhs[i];
			if (!seen(q.var())) {
				assert(isTrue(q) && "Invalid literal in reason set!");
				uint32 cl = level(q.var());
				assert(cl > 0 && "Top-Level implication not marked!");
				markSeen(q.var());
				if (cl == decisionLevel()) {
					++onLevel;
				}
				else {
					cc_.push_back(~q);
					abstr |= (1 << (cl & 31));
				}
			}
		}
		// search for the last assigned literal that needs to be analyzed...
		while (!seen(assign_.last().var())) {
			assign_.undoLast();
		}
		p = assign_.last();
		clearSeen(p.var());
		rhs.clear();
		if (--onLevel == 0) {
			break;
		}
		extractReason(p, rhs);
		strategy_.heuristic->updateReason(*this, rhs, p);
	}
	cc_[0] = ~p; // store the 1-UIP
	assert( decisionLevel() == level(p.var()));
	minimizeConflictClause(abstr);
	// clear seen-flag of all literals that are not from the current dl
	// and determine position of literal from second highest DL, which is
	// the asserting level of the newly derived conflict clause.
	secondWatch        = 1;
	uint32 assertLevel = 0;
	for (LitVec::size_type i = 1; i != cc_.size(); ++i) {
		clearSeen(cc_[i].var());
		if (level(cc_[i].var()) > assertLevel) {
			assertLevel = level(cc_[i].var());
			secondWatch = (uint32)i;
		}
	}
	return assertLevel;
}

void Solver::analyzeRootConflict(Literal p) {
	uint32 marked = 0;  // number of literals to resolve out
	uint32 tPos   = 0;  // current position in trail
	cc_.clear();        // will later hold the final result
	LitVec rhs    = conflict_.lits;
	if (!isSentinel(p)) {
		assert(isTrue(p));
		cc_.push_back(p);
		rhs.push_back(p);
	}
	if (decisionLevel() == 0) return;
	for (tPos = assign_.trail.size();;) {
		// process current rhs
		for (LitVec::size_type i = 0; i != rhs.size(); ++i) {
			Literal& q = rhs[i];
			if (!seen(q.var())) {
				assert(isTrue(q) && "Invalid literal in reason set!");
				assert(level(q.var()) > 0 && "Top-Level implication not marked!");
				markSeen(q.var());
				++marked;
			}
		}
		rhs.clear();
		if (marked == 0) { break; }
		// search for the last assigned literal that needs to be analyzed...
		while (!seen(assign_.trail[--tPos].var())) {
			;
		}	
		p = assign_.trail[tPos];
		clearSeen(p.var());
		--marked;
		if (reason(p).isNull()) {
			cc_.push_back(~p);
		}
		else {
			extractReason(p, rhs);
		}
	}
}

void Solver::minimizeConflictClause(uint32 abstr) {
	uint32 m = strategy_.cflMinAntes;
	LitVec::size_type t = assign_.trail.size();
	// skip the asserting literal
	LitVec::size_type j = 1;
	for (LitVec::size_type i = 1; i != cc_.size(); ++i) { 
		Literal p = ~cc_[i];
		if (reason(p).isNull() || ((reason(p).type()+1) & m) == 0  || !minimizeLitRedundant(p, abstr)) {
			cc_[j++] = cc_[i];
		}
		// else: p is redundant and can be removed from cc_
		// it was added to trail_ so that we can clear its seen flag
	}
	cc_.erase(cc_.begin()+j, cc_.end());
	while (assign_.trail.size() != t) {
		clearSeen(assign_.trail.back().var());
		assign_.trail.pop_back();
	}
}

bool Solver::minimizeLitRedundant(Literal p, uint32 abstr) {
	LitVec& temp = conflict_.lits;
	if (!strategy_.strengthenRecursive) {
		temp.clear(); extractReason(p, temp);
		for (LitVec::size_type i = 0; i != temp.size(); ++i) {
			if (!seen(temp[i].var())) {
				return false;
			}
		}
		assign_.trail.push_back(p);
		return true;
	}
	// else: een_minimization
	assign_.trail.push_back(p);  // assume p is redundant
	LitVec::size_type start = assign_.trail.size();
	for (LitVec::size_type f = start;;) {
		conflict_.clear();
		extractReason(p, temp);
		for (LitVec::size_type i = 0; i != temp.size(); ++i) {
			p = temp[i];
			if (!seen(p.var())) {
				if (!reason(p).isNull() && ((1<<(level(p.var())&31)) & abstr) != 0) {
					markSeen(p.var());
					assign_.trail.push_back(p);
				}
				else {
					while (assign_.trail.size() != start) {
						clearSeen(assign_.trail.back().var());
						assign_.trail.pop_back();
					}
					assign_.trail.pop_back(); // undo initial assumption
					return false;
				}
			}
		}
		if (f == assign_.trail.size()) break;
		p = assign_.trail[f++];
	}
	return true;
}

// Selects next branching literal. Use user-supplied heuristic if rand() < randProp.
// Otherwise makes a random choice.
// Returns false if assignment is total.
bool Solver::decideNextBranch() {
	DecisionHeuristic* heu = strategy_.heuristic.get();
	if (randHeuristic_ && drand() < static_cast<SelectRandom*>(randHeuristic_)->randFreq()) {
		heu = randHeuristic_;
	}
	return heu->select(*this);
}

// Remove upto maxRem% of the learnt nogoods.
// Keep those that are locked or have a high activity.
void Solver::reduceLearnts(float maxRem) {
	uint32 oldS = numLearntConstraints();
	ConstraintDB::size_type i, j = 0;
	if (maxRem < 1.0f) {    
		LitVec::size_type remMax = static_cast<LitVec::size_type>(numLearntConstraints() * std::min(1.0f, std::max(0.0f, maxRem)));
		uint64 actSum = 0;
		for (i = 0; i != learnts_.size(); ++i) {
			actSum += static_cast<LearntConstraint*>(learnts_[i])->activity();
		}
		double actThresh = (actSum / (double) numLearntConstraints()) * 1.5;
		for (i = 0; i != learnts_.size(); ++i) {
			LearntConstraint* c = static_cast<LearntConstraint*>(learnts_[i]);
			if (remMax == 0 || c->locked(*this) || c->activity() > actThresh) {
				c->decreaseActivity();
				learnts_[j++] = c;
			}
			else {
				--remMax;
				c->removeWatches(*this);
				c->destroy();
			}
		}
	}
	else {
		// remove all nogoods that are not locked
		for (i = 0; i != learnts_.size(); ++i) {
			LearntConstraint* c = static_cast<LearntConstraint*>(learnts_[i]);
			if (c->locked(*this)) {
				c->decreaseActivity();
				learnts_[j++] = c;
			}
			else {
				c->removeWatches(*this);
				c->destroy();
			}
		}
	}
	learnts_.erase(learnts_.begin()+j, learnts_.end());
	stats.solve.deleted += (oldS - numLearntConstraints());
}

/////////////////////////////////////////////////////////////////////////////////////////
// The basic DPLL-like search-function
/////////////////////////////////////////////////////////////////////////////////////////
ValueRep Solver::search(uint64 maxConflicts, uint32 maxLearnts, double randProp, bool localR) {
	initRandomHeuristic(randProp);
	maxConflicts = std::max(uint64(1), maxConflicts);
	if (localR) {
		if (!levConflicts_) levConflicts_ = new VarVec();
		levConflicts_->assign(decisionLevel()+1, (uint32)stats.solve.conflicts);
	}
	do {
		if ((hasConflict()&&!resolveConflict()) || !simplify()) { return value_false; }
		do {
			while (!propagate()) {
				if (!resolveConflict() || (decisionLevel() == 0 && !simplify())) {
					return value_false;
				}
				if ((!localR && --maxConflicts == 0) ||
					(localR && (stats.solve.conflicts - (*levConflicts_)[decisionLevel()]) > maxConflicts)) {
					undoUntil(0);
					return value_free;  
				}
			}
			if (numLearntConstraints()>maxLearnts) { 
				reduceLearnts(.75f); 
			}
		} while (decideNextBranch());
		// found a model candidate
		assert(numFreeVars() == 0);
	} while (!post_.isModel(*this));
	stats.solve.addModel(decisionLevel());
	if (strategy_.satPrePro.get()) {
		strategy_.satPrePro->extendModel(assign_);
	}
	return value_true;
}

bool Solver::nextSymModel(bool expand) {
	assert(numFreeVars() == 0);
	if (expand) {
		if (strategy_.satPrePro.get() != 0 && strategy_.satPrePro->hasSymModel()) {
			stats.solve.addModel(decisionLevel());
			strategy_.satPrePro->extendModel(assign_);
			return true;
		}
		else if (post_.nextSymModel(*this, true)) {
			stats.solve.addModel(decisionLevel());
			return true;
		}
	}
	else {
		post_.nextSymModel(*this, false);
		if (strategy_.satPrePro.get()) {
			strategy_.satPrePro->clearModel();
		}
	}
	return false;
}
}
