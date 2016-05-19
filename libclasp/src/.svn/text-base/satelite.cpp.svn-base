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
#include <clasp/satelite.h>
#include <clasp/clause.h>
#include <ctime>

#ifdef _MSC_VER
#pragma warning (disable : 4200) // nonstandard extension used : zero-sized array
#endif
namespace Clasp { namespace SatElite {

inline uint64 abstractLit(Literal p) {
	assert(p.var() > 0); 
	return uint64(1) << ((p.var()-1) & 63); 
}

// A clause class optimized for the SatElite preprocessor.
// Uses 1-literal watching to implement forward-subsumption
class Clause {
public:
	static Clause* newClause(const LitVec& lits) {
		void* mem = ::operator new( sizeof(Clause) + (lits.size()*sizeof(Literal)) );
		return new (mem) Clause(lits);
	}
	bool simplify(Solver& s) {
		uint32 i;
		for (i = 0; i != size_ && s.value(lits_[i].var()) == value_free; ++i) {;}
		if      (i == size_)          { return false; }
		else if (s.isTrue(lits_[i]))  { return true;  }
		uint32 j = i++;
		for (; i != size_; ++i) {
			if (s.isTrue(lits_[i]))   { return true; }
			if (!s.isFalse(lits_[i])) { lits_[j++] = lits_[i]; }
		}
		size_ = j;
		return false;
	}
	void destroy() {
		void* mem = this;
		this->~Clause();
		::operator delete(mem);
	}
	uint32          size()                const { return size_; }
	const Literal&  operator[](uint32 x)  const { return lits_[x]; }
	bool            inSQ()                const { return inSQ_ != 0; }
	uint64          abstraction()         const { return abstr_; }
	Literal&        operator[](uint32 x)        { return lits_[x]; }    
	void            setInSQ(bool b)             { inSQ_   = (uint32)b; }
	uint64&         abstraction()               { return abstr_; }
	void            strengthen(Literal p)       {
		abstr_ = 0;
		uint32 i, end;
		for (i   = 0; lits_[i] != p; ++i) { abstr_ |= abstractLit(lits_[i]); }
		for (end = size_-1; i < end; ++i) { lits_[i] = lits_[i+1]; abstr_ |= abstractLit(lits_[i]); }
		--size_;
	}
private:
	Clause(const LitVec& lits) : abstr_(0), size_((uint32)lits.size()), inSQ_(0) {
		std::memcpy(lits_, &lits[0], lits.size()*sizeof(Literal));
	}
	uint64  abstr_;       // abstraction - to speed up backward subsumption check
	uint32  size_   : 31; // size of the clause
	uint32  inSQ_   : 1;  // in subsumption-queue?
	Literal lits_[0];     // literals of the clause: [lits_[0], lits_[size_])
};
/////////////////////////////////////////////////////////////////////////////////////////
// SatElite preprocessing
//
/////////////////////////////////////////////////////////////////////////////////////////
SatElite::SatElite(Solver* s) 
	: occurs_(0)
	, elimList_(0)
	, elimHeap_(LessOccCost(occurs_))
	, qFront_(0)
	, facts_(0)
	, clRemoved_(0) {
	if (s) setSolver(*s);
}

SatElite::~SatElite() {
	cleanUp();
	if (elimList_) {
		for (VarVec::size_type i = 0; i != elimList_->size(); ++i) {
			Clause* c = 0;
			for (VarVec::size_type j = 0; (c = (*elimList_)[i].eliminated[j]) != 0; ++j) {
				c->destroy();
			}
			delete [] (*elimList_)[i].eliminated;
		}
		delete elimList_;
		elimList_ = 0;
	}
}

void SatElite::cleanUp() {
	delete [] occurs_;  occurs_ = 0; 
	for (ClauseList::size_type i = 0; i != clauses_.size(); ++i) {
		if (clauses_[i]) { clauses_[i]->destroy(); }
	}
	ClauseList().swap(clauses_);
	ClauseList().swap(resCands_);
	ClauseList().swap(posT_);
	ClauseList().swap(negT_);
	LitVec().swap(resolvent_);
	VarVec().swap(queue_);
	elimHeap_.clear();
	qFront_ = facts_ = clRemoved_ = 0;
}

bool SatElite::addClause(const LitVec& clause) {
	assert(solver_);
	if (clause.empty()) {
		return false;
	}
	else if (clause.size() == 1) {
		return solver_->force(clause[0], 0) && solver_->propagate();
	}
	else {
		Clause* c = Clause::newClause( clause );
		clauses_.push_back( c );
	}
	return true;
}

Clause* SatElite::popSubQueue() {
	if (Clause* c = clauses_[ queue_[qFront_++] ]) {
		c->setInSQ(false);
		return c;
	}
	return 0;
}

void SatElite::addToSubQueue(uint32 clauseId) {
	assert(clauses_[clauseId] != 0);
	if (!clauses_[clauseId]->inSQ()) {
		queue_.push_back(clauseId);
		clauses_[clauseId]->setInSQ(true);
	}
}

void SatElite::attach(uint32 clauseId, bool initialClause) {
	Clause& c = *clauses_[clauseId];
	c.abstraction() = 0;
	for (uint32 i = 0; i != c.size(); ++i) {
		Var v = c[i].var();
		occurs_[v].add(clauseId, c[i].sign());
		occurs_[v].unmark();
		c.abstraction() |= abstractLit(c[i]);
		if (initialClause && !solver_->eliminated(v) && !solver_->frozen(v) && !elimHeap_.is_in_queue(v)) {
			elimHeap_.push(v);
		}
		else if (elimHeap_.is_in_queue(v)) {
			elimHeap_.decrease(v);
		}
	}
	occurs_[c[0].var()].watches.push_back(clauseId);
	addToSubQueue(clauseId);
}

void SatElite::detach(uint32 id, bool destroy) {
	Clause& c = *clauses_[id];
	VarVec& watches = occurs_[c[0].var()].watches;
	watches.erase(std::find(watches.begin(), watches.end(), id));
	for (uint32 i = 0; i != c.size(); ++i) {
		Var v = c[i].var();
		occurs_[v].remove(id, c[i].sign(), false);
		updateHeap(v);
	}
	clauses_[id] = 0;
	if (destroy)  ++clRemoved_, c.destroy();
	else          c.abstraction() = id;
}

bool SatElite::preprocess() {
	setState(SatPreprocessor::pre_start, 0);
	struct Scope { SatElite* self; SatPreprocessor::State action; ~Scope() { 
		self->cleanUp();
		self->setState(action, 0);
	}} sc = {this, SatPreprocessor::pre_done};
	// skip preprocessing if other constraints are UNSAT
	if (!solver_->propagate()) return false;
	// 0. allocate & init state
	occurs_     = new OccurList[solver_->numVars()+1];
	if (elimList_ == 0) {
		elimList_   = new ElimList();
		elimList_->reserve(std::min(ElimList::size_type(100), ElimList::size_type(solver_->numVars() / 3)));
		elimPos_    = ElimPos(0,0);
	}
	qFront_ = clRemoved_ = 0;
	// 1. remove SAT-clauses, strengthen clauses w.r.t false literals, init occur-lists
	facts_ = solver_->numAssignedVars();
	ClauseList::size_type j = 0; 
	for (ClauseList::size_type i = 0; i != clauses_.size(); ++i) {
		Clause* c = clauses_[i]; assert(c);
		if      (c->simplify(*solver_)) { c->destroy(); clauses_[i] = 0;}
		else if (c->size() < 2)         {
			Literal unit = c->size() == 1 ? (*c)[0] : negLit(0);
			c->destroy(); clauses_[i] = 0;
			if (!(solver_->force(unit, 0)&&solver_->propagate()) || !propagateFacts()) {
				for (++i; i != clauses_.size(); ++i) {
					clauses_[i]->destroy();
				}
				clauses_.erase(clauses_.begin()+j, clauses_.end());
				return false;
			}
		}
		else                            {
			clauses_[j] = c;
			attach(uint32(j), true);
			++j;
		}
	}
	clauses_.erase(clauses_.begin()+j, clauses_.end());
	assert(facts_ == solver_->numAssignedVars());
	// simplify other constraints w.r.t new derived top-level facts
	if (!solver_->simplify()) return false;

	// 2. remove subsumed clauses, eliminate vars by clause distribution
	std::time_t timeOut = options.maxTime != uint32(-1) ? time(0) + options.maxTime : std::numeric_limits<std::time_t>::max();
	uint32 eliminated   = 0;
	uint32 iteration    = 0;
	while (!queue_.empty() || !elimHeap_.empty()) {
		++iteration;
		setState(SatPreprocessor::iter_start, iteration);
		if (!backwardSubsume())     { return false; }
		assert(queue_.empty());
		while (!elimHeap_.empty())  {
			Var elim = elimHeap_.top();
			elimHeap_.pop();
			if (!eliminateVar(elim))  { return false; }
			if (++eliminated == 1000 && timeOut != std::numeric_limits<std::time_t>::max()) {
				eliminated  = 0;
				if (time(0) > timeOut) { 
					sc.action = SatPreprocessor::pre_stopped; 
					elimHeap_.clear(); 
				}
			}
		}
		setState(SatPreprocessor::iter_done, iteration);
		if (!queue_.empty() && (iteration == options.maxIters || time(0) > timeOut)) {
			sc.action = SatPreprocessor::pre_stopped;
			// we reached our limit for preprocessing; stop early but
			// make sure that we don't add subsumed clauses
			if (!backwardSubsume()) return false;
			break;
		}
	}
	assert( facts_ == solver_->numAssignedVars() );
	// simplify other constraints w.r.t new derived top-level facts
	if (!solver_->simplify()) return false;
	// 3. Transfer simplified clausal problem to solver
	ClauseCreator nc(solver_);
	for (ClauseList::size_type i = 0; i != clauses_.size(); ++i) {
		if (clauses_[i]) {
			Clause& c = *clauses_[i];
			nc.start();
			for (uint32 x = 0; x != c.size(); ++x) {  nc.add(c[x]);  }
			nc.end();
			c.destroy();
		}
	}
	clauses_.clear();
	return true;  
}

// (Destructive) unit propagation on clauses.
// Removes satisfied clauses and shortens clauses w.r.t. false literals.
// Pre:   Assignment is propagated w.r.t other non-clause constraints
// Post:  Assignment is fully propagated and no clause contains an assigned literal
bool SatElite::propagateFacts() {
	assert(solver_->queueSize() == 0);
	while (facts_ != solver_->numAssignedVars()) {
		Literal l     = solver_->assignment()[facts_++];
		OccurList& ov = occurs_[l.var()];
		LitVec& cls   = occurs_[l.var()].clauses;
		for (uint32 i = 0, clId; i != cls.size(); ++i) {
			if      (clauses_[clId=cls[i].var()] == 0)  { continue; }
			else if (cls[i].sign() == l.sign())         { detach(clId, true); }
			else if (!strengthenClause(clId, ~l))       { return false; }
		}
		ov.clear();
		ov.mark(!l.sign());
	}
	assert(solver_->queueSize() == 0);
	return true;
}

// Backward subsumption and self-subsumption resolution until fixpoint
bool SatElite::backwardSubsume() {
	if (!propagateFacts()) return false;
	while (qFront_ != queue_.size()) {
		if (peekSubQueue() == 0) { ++qFront_; continue; }
		Clause& c = *popSubQueue();
		// Try to minimize effort by testing against the var in c that occurs least often;
		Literal best  = c[0];
		for (uint32 i = 1; i < c.size(); ++i) {
			if (occurs_[c[i].var()].numOcc() < occurs_[best.var()].numOcc()) {
				best  = c[i];
			}
		}
		
		// Test against all clauses containing best
		LitVec& cls   = occurs_[best.var()].clauses;
		Literal res   = negLit(0);
		uint32  j     = 0;
		for (uint32 i = 0; i != cls.size(); ++i) {
			uint32 otherId  = cls[i].var();
			Clause* other   = clauses_[otherId];
			if (other && other!= &c && (res = subsumes(c, *other, best.sign()==cls[i].sign()?posLit(0):best)) != negLit(0)) {
				if (res == posLit(0)) {
					// other is subsumed - remove it
					detach(otherId, true);
					other = 0;
				}
				else {
					// self-subsumption resolution; other is subsumed by c\{res} U {~res}
					// remove ~res from other, add it to subQ so that we can check if it now subsumes c
					res = ~res;
					occurs_[res.var()].remove(otherId, res.sign(), res.var() != best.var());
					updateHeap(res.var());
					if (!strengthenClause(otherId, res))              { return false; }
					if (res.var() == best.var() || clauses_[otherId] == 0)  { other = 0; }
				}
			}
			if (other && j++ != i)  { cls[j-1] = cls[i]; }
		}
		cls.erase(cls.begin()+j, cls.end());
		occurs_[best.var()].dirty = 0;
		assert(occurs_[best.var()].numOcc() == (uint32)occurs_[best.var()].clauses.size());
		if (!propagateFacts()) return false;
	}   
	queue_.clear();
	qFront_ = 0;
	return true;
}

// checks if 'c' subsumes 'other', and at the same time, if it can be used to 
// simplify 'other' by subsumption resolution.
// Return:
//  - negLit(0) - No subsumption or simplification
//  - posLit(0) - 'c' subsumes 'other'
//  - l         - The literal l can be deleted from 'other'
Literal SatElite::subsumes(const Clause& c, const Clause& other, Literal res) const {
	if (other.size() < c.size() || (c.abstraction() & ~other.abstraction()) != 0) {
		return negLit(0);
	}
	if (c.size() < 10 || other.size() < 10) {
		for (uint32 i = 0; i != c.size(); ++i) {
			for (uint32 j = 0; j != other.size(); ++j) {
				if (c[i].var() == other[j].var()) {
					if (c[i].sign() == other[j].sign())     { goto found; }
					else if (res != posLit(0) && res!=c[i]) { return negLit(0); }
					res = c[i];
					goto found;
				}
			}
			return negLit(0); 
		found:;
		}
	}
	else {
		for (uint32 i = 0; i != other.size(); ++i) {
			assert(!occurs_[other[i].var()].marked(other[i].sign()));
			occurs_[other[i].var()].mark(other[i].sign());
		}
		for (uint32 i = 0; i != c.size(); ++i) {
			if (occurs_[c[i].var()].litMark == 0) { res = negLit(0); break; }
			if (occurs_[c[i].var()].marked(!c[i].sign())) {
				if (res != posLit(0)&&res!=c[i]) { res = negLit(0); break; }
				res = c[i];
			}
		}
		for (uint32 i = 0; i != other.size(); ++i) { occurs_[other[i].var()].unmark(); }
	}
	return res;
}

uint32 SatElite::findUnmarkedLit(const Clause& c, uint32 x) const {
	for (; x != c.size() && occurs_[c[x].var()].marked(c[x].sign()); ++x)
		;
	return x;
}

// checks if 'cl' is subsumed by one of the existing clauses and at the same time
// strengthens 'cl' if possible.
// Return:
//  - true  - 'cl' is subsumed
//  - false - 'cl' is not subsumed but may itself subsume other clauses
// Pre: All literals of l are marked, i.e. 
// for each literal l in cl, occurs_[l.var()].marked(l.sign()) == true
bool SatElite::subsumed(LitVec& cl) {
	Literal l;
	uint32 x = 0;
	uint32 str = 0;
	LitVec::size_type j = 0;
	for (LitVec::size_type i = 0; i != cl.size(); ++i) {
		l = cl[i];
		if (occurs_[l.var()].litMark == 0) { --str; continue; }
		VarVec& watches = occurs_[l.var()].watches; // all clauses watching either l or ~l
		VarVec::size_type wj = 0;
		for (VarVec::size_type w = 0, end = watches.size(); w != end; ++w) {
			Clause& c = *clauses_[watches[w]];
			if (c[0] == l)  {
				if ( (x = findUnmarkedLit(c, 1)) == c.size() ) {
					while (w != end) { watches[wj++] = watches[w++]; }
					watches.erase( watches.begin()+wj, watches.end() );
					return true;
				}
				c[0] = c[x];
				c[x] = l;
				occurs_[c[0].var()].watches.push_back(watches[w]);
				if (occurs_[c[0].var()].litMark != 0 && findUnmarkedLit(c, x+1) == c.size()) {
					occurs_[c[0].var()].litMark = 0;  // no longer part of cl
					++str;
				}
			}
			else if ( findUnmarkedLit(c, 1) == c.size() ) {
				occurs_[l.var()].litMark = 0; // no longer part of cl
				while (w != end) { watches[wj++] = watches[w++]; }
				watches.erase( watches.begin()+wj, watches.end() );
				goto removeLit;
			}
			else { watches[wj++] = watches[w]; }  
		}
		watches.erase(watches.begin()+wj, watches.end());
		if (j++ != i) { cl[j-1] = cl[i]; }
removeLit:;
	}
	cl.erase(cl.begin()+j, cl.end());
	if (str > 0) {
		for (LitVec::size_type i = 0; i != cl.size();) {
			if (occurs_[cl[i].var()].litMark == 0) {
				cl[i] = cl.back();
				cl.pop_back();
				if (--str == 0) break;
			}
			else { ++i; }
		}
	}
	return false;
}

// Pre: c contains l
// Pre: c was already removed from l's occur-list
bool SatElite::strengthenClause(uint32 clauseId, Literal l) {
	Clause& c = *clauses_[clauseId];
	if (c[0] == l) {
		VarVec& w = occurs_[c[0].var()].watches;
		w.erase(std::find(w.begin(), w.end(), clauseId));
		// Note: Clause::strengthen shifts literals after l to the left. Thus
		// c[1] will be c[0] after strengthen
		occurs_[c[1].var()].watches.push_back(clauseId);
	}
	c.strengthen(l);
	if (c.size() == 1) {
		Literal unit = c[0];
		detach(clauseId, true);
		return solver_->force(unit, 0) && solver_->propagate();
	}
	addToSubQueue(clauseId);
	return true;
}

// Removes v by clause distribution if this reduces the number of clauses
bool SatElite::eliminateVar(Var v) {
	assert(!solver_->frozen(v) && !solver_->eliminated(v));
	// check cutoff
	if (solver_->value(v) != value_free
		|| occurs_[v].numOcc() == 0       
		|| (!options.elimPure && occurs_[v].cost() == 0)  // elim pure only if allowed
		|| (occurs_[v].pos > options.maxOcc && occurs_[v].neg > options.maxOcc)) {
		return true;
	}
	// split occurrences of v into pos and neg
	LitVec& cls = occurs_[v].clauses;
	posT_.clear(); negT_.clear();
	uint32 j = 0;
	for (uint32 i = 0; i != cls.size(); ++i) {
		if (Clause* c = clauses_[cls[i].var()]) {
			if (j != i) cls[j] = cls[i];
			(cls[i].sign() ? negT_ : posT_).push_back(c);
			++j;
		}
	}
	cls.erase(cls.begin()+j, cls.end());
	// check if number of clauses decreases if we'd eliminate v
	uint32 cnt = 0, maxCnt = occurs_[v].numOcc();
	resCands_.clear();
	for (VarVec::size_type p = 0; p != posT_.size(); ++p) {
		for (VarVec::size_type n = 0; n != negT_.size(); ++n) {
			if (!trivialResolvent(*posT_[p], *negT_[n], v)) {
				if (++cnt > maxCnt) return true;
				resCands_.push_back( posT_[p] );
				resCands_.push_back( negT_[n] );
			}
		}
	}
	// remove old clauses, store them in the elimination table so that
	// (partial) models can be extended.
	solver_->eliminate(v, true);  // mark var as eliminated
	ElimData d(v);
	d.eliminated = new Clause*[cls.size()+1];
	d.eliminated[cls.size()] = 0;
	for (uint32 i = 0; i != cls.size(); ++i) {
		d.eliminated[i] = clauses_[cls[i].var()];
		detach(cls[i].var(), false);
	}
	elimList_->push_back(d);
	// release memory
	occurs_[v].clear();
	// add non trivial resolvents
	assert( resCands_.size() % 2 == 0 );
	for (VarVec::size_type i = 0; i != resCands_.size(); i+=2) {
		if (!addResolvent(*resCands_[i], *resCands_[i+1], v)) {
			return false;
		}
	}
	assert(occurs_[v].numOcc() == 0);
	return options.maxIters != uint32(-1) || backwardSubsume();
}

// returns true if the result of resolving c1 and c2 on v yields a tautologous clause
bool SatElite::trivialResolvent(const Clause& c1, const Clause& c2, Var v) const {
	const Clause& shorter = c1.size() < c2.size() ? c1 : c2;
	const Clause& other   = c1.size() < c2.size() ? c2 : c1;
	for (uint32 i = 0; i != shorter.size(); ++i) {
		if (shorter[i].var() == v) continue;
		for (uint32 j = 0; j != other.size(); ++j) {
			if (other[j].var() == shorter[i].var()) {
				if (other[j].sign() != shorter[i].sign()) { return true; }
				break;
			}
		}
	}
	return false;
}

// Pre: lhs and rhs can be resolved on v
// Pre: trivialResolvent(lhs, rhs, v) == false
bool SatElite::addResolvent(const Clause& lhs, const Clause& rhs, Var v) {
	resolvent_.clear();
	uint32 i, end;
	Literal l;
	for (i = 0, end = lhs.size(); i != end; ++i) {
		l = lhs[i];
		if (l.var() != v && !solver_->isFalse(l)) {
			if (solver_->isTrue(l)) goto unmark;
			occurs_[l.var()].litMark = (1u + l.sign());
			resolvent_.push_back(l);
		}
	}
	for (i = 0, end = rhs.size(); i != end; ++i) {
		l = rhs[i];
		if (l.var() != v && !solver_->isFalse(l) && !occurs_[l.var()].marked(l.sign())) {
			if (solver_->isTrue(l)) goto unmark;
			occurs_[l.var()].litMark = (1u + l.sign());
			resolvent_.push_back(l);
		}
	}
	if (!subsumed(resolvent_))  {
		if (resolvent_.empty())   { return false; }
		if (resolvent_.size()==1) { 
			occurs_[resolvent_[0].var()].litMark = 0; 
			return solver_->force(resolvent_[0], 0) && solver_->propagate() && propagateFacts();
		}
		uint32 id     = freeClauseId();
		clauses_[id]  = Clause::newClause(resolvent_);
		attach(id, false);
		return true;
	}
	else {
unmark:
		for (i = 0, end = resolvent_.size(); i != end; ++i) {
			occurs_[resolvent_[i].var()].litMark = 0;
		}
	}
	return true;
}

uint32 SatElite::freeClauseId() {
	if (elimPos_.first < elimList_->size() && (*elimList_)[elimPos_.first].eliminated[elimPos_.second] != 0) {
		// reuse id of an eliminated clause
		Clause& elim  = *(*elimList_)[elimPos_.first].eliminated[elimPos_.second];
		uint32 id     = (uint32)elim.abstraction(); assert(clauses_[id] == 0);
		if ((*elimList_)[elimPos_.first].eliminated[++elimPos_.second] == 0) {
			++elimPos_.first; 
			elimPos_.second = 0;
		}
		// make sure the clause is no longer referenced in any occur-list
		for (uint32 i = 0; i != elim.size(); ++i) { 
			OccurList& ol = occurs_[elim[i].var()];
			if (ol.dirty != 0) {
				LitVec::iterator j = ol.clauses.begin(), end = ol.clauses.end();
				for (LitVec::iterator i = j; i != end; ++i) {
					if (clauses_[i->var()]) { *j++ = *i; }
				}
				ol.clauses.erase(j, end);
				ol.dirty = 0;
			}
		}
		return id;
	}
	else {
		// Note: as long as we eliminate only those vars for which clause distribution does not increase
		// the number of clauses, this branch is *never* executed!
		if (clRemoved_ > 1000) {
			// compact clause array
			delete [] occurs_;
			occurs_ = new OccurList[solver_->numVars()+1];
			queue_.clear(); assert(qFront_ == 0);
			uint32 j = 0;
			for (uint32 i = 0; i != clauses_.size(); ++i) {
				if (clauses_[i]) {
					Clause& c     = *clauses_[i];
					if (c.inSQ()) { queue_.push_back( j ); }
					for (uint32 x = 0; x != c.size(); ++x) {
						occurs_[c[x].var()].add(j, c[x].sign());
					}
					occurs_[c[0].var()].watches.push_back(j);
					clauses_[j++] = &c;
				}
			}
			clauses_.erase(clauses_.begin()+j, clauses_.end());
			clRemoved_ = 0;
		}
		uint32 id = (uint32)clauses_.size();
		clauses_.push_back(0);
		return id;
	}
}

// extends the model given in vars by the vars that were eliminated
void SatElite::extendModel(Assignment& assign) {
	if (!elimList_) return;
	// 1. solver set all eliminated vars to true, thus before we can compute the
	// implied values we first need to set them back to free
	for (ElimList::const_iterator it = elimList_->begin(); it != elimList_->end(); ++it) {
		assign.clearValue(it->var);
	}
	// 2. some of the eliminated vars are unconstraint w.r.t the current model, i.e.
	// they can be either true or false. Since we may be interested in all models
	// we "enumerate" the unconstraint vars
	if (!unconstr_.empty()) {
		unconstr_.back() = ~unconstr_.back();
		for (VarVec::size_type i = 0; i != unconstr_.size(); ++i) {
			assign.setValue(unconstr_[i].var(), trueValue(unconstr_[i]));
		}
	}
	// 3. for each eliminated var, compute its implied value by "unit propagating" its
	// eliminated clauses.
	// Start with the vars, eliminated last
	for (ElimList::reverse_iterator it = elimList_->rbegin(); it != elimList_->rend(); ++it) {
		Var v = it->var;
		if (solver_->value(v) != value_free) continue;
		Literal x;
		for (VarVec::size_type j = 0; it->eliminated[j] != 0; ++j) {
			const Clause& c = *it->eliminated[j];
			for (uint32 k = 0; k != c.size(); ++k) {
				if      (c[k].var() == v) { x = c[k]; }
				else if (!solver_->isFalse(c[k])) { goto nextClause; }
			}
			assert(x != Literal() && !solver_->isFalse(x));
			assign.setValue(v, trueValue(x));
			break;
nextClause:;
		}
		if (solver_->value(v) == value_free) {  
			// v is unconstraint w.r.t the model. Assume v to true; remember it
			// so that we can also enumerate the model containing ~v.
			assign.setValue(v, value_true);
			unconstr_.push_back(posLit(v));
		}
	}
	while (!unconstr_.empty() && unconstr_.back().sign()) {
		unconstr_.pop_back();
	}
}
}}
