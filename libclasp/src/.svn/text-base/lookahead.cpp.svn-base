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
#include <clasp/lookahead.h>
#include <algorithm>
namespace Clasp { 
/////////////////////////////////////////////////////////////////////////////////////////
// Lookahead scoring
/////////////////////////////////////////////////////////////////////////////////////////
uint32 ScoreLook::countNant(const Solver& s, const Literal* b, const Literal *e) const {
	uint32 sc = 0;
	for (; b != e; ++b) { sc += s.nant(b->var()); }
	return sc;
}
void ScoreLook::scoreLits(const Solver& s, const Literal* b, const Literal *e) {
	assert(b < e);
	uint32 sc = !nant ? uint32(e-b) : countNant(s, b, e);
	Var v     = b->var();
	score[v].setScore(*b, sc);
	if (addDeps) {
		if ((score[v].testedBoth() || mode == score_max) && greater(v, best)) {
			best = v;
		}
		for (; b != e; ++b) {
			v = b->var();
			if ( (s.type(v) & types) != 0) {
				if (!score[v].seen()) { deps.push_back(v); }
				score[v].setDepScore(*b, sc);
				score[v].setSeen(*b);
			}
		}
	}
}
void ScoreLook::clearDeps() {
	for (VarVec::size_type i = 0, end = deps.size(); i != end; ++i) {
		score[deps[i]].clear();
	}
	deps.clear();
	best = 0;
}
bool ScoreLook::greater(Var lhs, Var rhs) const {
	uint32 rhsMax, rhsMin;
	score[rhs].score(rhsMax, rhsMin);
	return mode == score_max 
		? greaterMax(lhs, rhsMax)
		: greaterMaxMin(lhs, rhsMax, rhsMin);
}
/////////////////////////////////////////////////////////////////////////////////////////
// Lookahead propagator
/////////////////////////////////////////////////////////////////////////////////////////
Lookahead::Lookahead()
	: head_(posLit(0)) // dummy node
	, undo_(posLit(0)) // dummy node
	, last_(&head_)    // circular list
	, pos_(&head_)     // lookahead start pos
	, top_(uint32(-2)) {
	last_->next= &head_;
}
Lookahead::~Lookahead() { clear(); }
void Lookahead::enable(Solver& s) {
	if (top_ == uint32(-2)) {
		s.addPost(this);
		top_ = uint32(-1);
		score.score.resize(s.numVars()+1);
	}
}
void Lookahead::disable(Solver& s) {
	if (top_ != uint32(-2)) {
		while (saved_.size()>1) {
			s.removeUndoWatch(uint32(saved_.size()-1), this);
			splice(saved_.back());
			saved_.pop_back();
		}
		s.removePost(this);
		top_ = uint32(-2);
	}
}
uint32 Lookahead::priority() const { return priority_lookahead; }
void Lookahead::clear() {
	score.clearDeps();
	while (!saved_.empty()) {
		if (saved_.back() != 0) {
			splice(saved_.back());
		}
		saved_.pop_back();
	}
	for (LitNode* r = head_.next; r != &head_; ) {
		LitNode* t = r;
		r = r->next;
		delete t;
	}
	last_       = &head_;
	last_->next = &head_;
	top_        = uint32(-1);
}

void Lookahead::append(Literal p, bool testBoth) {
	last_->next = new LitNode(p);
	last_       = last_->next;
	last_->next = &head_;
	// remember to also test ~p by setting watched-flag
	if (testBoth) { last_->lit.watch(); }
}

// test p and optionally ~p if necessary
bool Lookahead::test(Solver& s, Literal p) {
	return (score.score[p.var()].seen(p) || s.test(p, this))
		&& (!p.watched() || score.score[p.var()].seen(~p) || s.test(~p, this));
}

// failed-literal detection - stop on failed-literal
bool Lookahead::propagate(Solver& s) {
	assert(!s.hasConflict());
	saved_.resize(s.decisionLevel()+1, 0);
	LitNode* undoLast = saved_[s.decisionLevel()];
	if (undoLast == 0) {
		undoLast = &undo_;
		if (s.decisionLevel() != 0) {
			s.addUndoWatch(s.decisionLevel(), this);
		}
	}
	score.clearDeps(); 
	score.addDeps = true;
	Literal p    = pos_->lit;
	bool   ok    = s.value(p.var()) != value_free || test(s, p);
	for (LitNode* r = pos_; r->next != pos_ && ok; ) {
		p = r->next->lit;
		if (s.value(p.var()) == value_free) {
			if (test(s, p)) { r   = r->next; }
			else            { pos_= r->next; ok = false; }
		}
		else if (r->next != last_ && r->next != &head_) {
			// unlink from candidate list
			LitNode* t = r->next;
			r->next    = t->next;
			// append to undo list
			t->next        = undoLast->next;
			undoLast->next = t;
			undoLast       = t;
		}
		else { r = r->next; } // keep iterators valid; never unlink last node and dummy head
	}
	saved_.back() = undoLast;
	return ok;
}

bool Lookahead::propagateFixpoint(Solver& s) {
	if (empty() || top_ == s.assignment().size()) {
		// nothing to lookahead
		return true;
	}
	bool ok   = true;
	uint32 dl;
	for (dl = s.decisionLevel(); !propagate(s); dl = s.decisionLevel()) {
		// some literal failed
		// resolve and propagate conflict
		assert(dl+1 == s.decisionLevel());
		if (!s.resolveConflict() || !s.propagateUntil(this)) {
			ok = false;
			score.clearDeps();
			break;
		}
	}
	if (dl == 0 && ok) {
		// remember top-level size - no need to redo lookahead 
		// on level 0 unless we learn a new implication
		assert(s.queueSize() == 0);
		top_    = uint32(s.assignment().size());
	}
	return ok;
}

// splice list [undo_.next, ul] back into candidate list
void Lookahead::splice(LitNode* ul) {
	if (ul != &undo_) { 
		assert(undo_.next);
		// unlink from undo list
		LitNode* first = undo_.next;
		undo_.next     = ul->next;
		// splice into look-list
		ul->next       = head_.next;
		head_.next     = first;
	}
}

void Lookahead::undoLevel(Solver& s) {
	if (s.decisionLevel() == saved_.size()) {
		const LitVec& a = s.assignment();
		score.scoreLits(s, &a[0]+s.levelStart(s.decisionLevel()), &a[0]+a.size());
	}
	else {
		assert(s.decisionLevel()+1 == saved_.size());
		LitNode* n = saved_.back();
		saved_.pop_back();
		splice(n);
		assert(last_->next == &head_);
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// Lookahead heuristic
/////////////////////////////////////////////////////////////////////////////////////////
UnitHeuristic::UnitHeuristic(Lookahead::Type t, bool nant, DecisionHeuristic* h, int m)
	: look_(0)
	, heu_(h)
	, maxLook_(!h ? -1 : m)
	, reinit_(0)
	, type_(t) 
	, nant_(nant && t == Lookahead::atom_lookahead) {
}
UnitHeuristic::~UnitHeuristic() {
	delete heu_;
}

void UnitHeuristic::startInit(const Solver& s) {
	if (reinit_) { maxLook_ = int32(reinit_-1); }
	if (heu_)    { heu_->startInit(s); }
	if (look_) {
		// disable lookahead during setup - 
		// we re-enable it once all variables and
		// constraints are known
		look_->disable(const_cast<Solver&>(s));
	}
}

void UnitHeuristic::endInit(Solver& s) { 
	if (look_ == 0) { 
		look_ = new Lookahead(); 
	}
	ScoreLook& sc = look_->score;
	sc.clearDeps();
	Var start     = (uint32)sc.score.size();
	sc.score.resize(s.numVars()+1);
	if (type_ != Lookahead::hybrid_lookahead) {
		sc.mode = ScoreLook::score_max_min;
		sc.types= (type_ == Lookahead::body_lookahead)
			? Var_t::body_var
			: Var_t::atom_var;
	}
	else {
		sc.mode = ScoreLook::score_max;
		sc.types= Var_t::atom_body_var;
	}
	const VarType types= sc.types;
	const bool uniform = types != Var_t::atom_body_var;
	uint32 nant = 0;
	uint32 added= 0;
	for (Var v = start; v <= s.numVars(); ++v) {
		if (s.value(v) == value_free && (s.type(v) & types) != 0 ) {
			++added;
			nant += s.nant(v);
			look_->append(s.preferredLiteralByType(v), uniform || s.type(v) == Var_t::atom_body_var);
		}
	}
	if (nant_ && nant != added) {
		sc.nant = true;
	}
	look_->enable(s);
	if (heu_) heu_->endInit(s); 
	if (heu_ && maxLook_ == -1 && !reinit_) {
		suicide(s);	
	}
}

void UnitHeuristic::resurrect(const Solver& s, Var v) {
	if ( (s.type(v) & look_->score.types) != 0 ) {
		look_->score.score.resize(s.numVars()+1);
		look_->append(s.preferredLiteralByType(v), 
			s.type(v) == Var_t::atom_body_var || type_ != Lookahead::hybrid_lookahead);
	}
	if (heu_) heu_->resurrect(s, v);
}

void UnitHeuristic::reinit(bool b) {
	if (b && maxLook_ >= 0){ reinit_ = uint32(maxLook_)+1; }
	else                   { reinit_ = 0; }
	if (heu_) heu_->reinit(b); 
}

Literal UnitHeuristic::heuristic(Solver& s) {
	ScoreLook& sc = look_->score;
	if (sc.deps.empty()) {
		if (s.value(top_.var()) == value_free) return top_;
		// No candidates. This can happen if the problem 
		// contains choice rules and lookahead is not atom-based.
		// Add remaining free vars to lookahead so that we can
		// make an informed decision.
		for (Var v = 1, end = s.numVars()+1; v != end; ++v) {
			if (s.value(v) == value_free) {
				look_->append(s.preferredLiteralByType(v), true);
			}
		}
		sc.types  = Var_t::atom_body_var;
		uint32 dl = s.decisionLevel();
		if (!look_->propagate(s)) {
			Literal choice = s.decision(s.decisionLevel());
			s.undoUntil(dl);
			return choice;
		}
	}
	assert(s.value(sc.best) == value_free);
	Literal choice = Literal(sc.best, sc.score[sc.best].prefSign());
	if (sc.mode == ScoreLook::score_max_min) {
		uint32 min, max;
		sc.score[sc.best].score(max, min);
		sc.addDeps = false;
		bool ok    = true;
		LitVec::size_type i = 0;
		do {
			Var v        = sc.deps[i];
			VarScore& vs = sc.score[v];
			if (!vs.testedBoth()) {
				uint32 vMin, vMax;
				vs.score(vMax, vMin);
				if (vMin == 0 || vMin > min || (vMin == min && vMax > max)) {
					uint32 neg = vs.score(negLit(v)) > 0 ? vs.score(negLit(v)) : max+1;
					uint32 pos = vs.score(posLit(v)) > 0 ? vs.score(posLit(v)) : max+1;
					if (!vs.tested(negLit(v))) {
						ok  = ok && s.test(negLit(v), look_);
						neg = vs.score(negLit(v));
					}
					if ((neg > min || (neg == min && pos > max)) && !vs.tested(posLit(v))) {
						ok  = ok && s.test(posLit(v), look_);
						pos = vs.score(posLit(v));
					}
				}
				if (vs.testedBoth() && sc.greaterMaxMin(v, max, min)) {
					vs.score(max, min);
					choice = Literal(v, vs.prefSign());
				}
			}
			vs.clear();
		} while (++i != sc.deps.size() && ok); 
		if (!ok) {
			// One of the candidates failed. Since none of them failed
			// during previous propagation, this indicates that some
			// post propagator has wrong priority. 
			// Since we can't resolve the problem here, we simply return the
			// literal that caused the conflict in the hope that
			// it is immediately regenerated once the literal is used as assumption.
			sc.clearDeps();
			assert(s.hasConflict());
			choice = s.decision(s.decisionLevel());
			s.undoUntil(s.decisionLevel()-1);
		}
		sc.deps.clear();	
	}
	assert(!isSentinel(choice));
	if (s.decisionLevel() == 0) top_ = choice;
	return choice;
}

Literal UnitHeuristic::doSelect(Solver& s) {
	if (maxLook_ == -1) {
		if (!heu_) return heuristic(s);
		else       return heu_->doSelect(s);
	}
	else { // maxLook >= 0
		// > 0: choice = unit heuristic
		// = 0: choice = other heuristic + disable lookahead
		Literal choice;
		if (maxLook_ > 0)   { choice = heuristic(s); --maxLook_; }
		else                { choice = heu_->doSelect(s); }
		if (maxLook_ <= 0)  {
			maxLook_ = -1;
			look_->disable(s);
			if (!reinit_) { 
				delete look_;
				suicide(s); 
			}
		}
		return choice;
	}
}

void UnitHeuristic::suicide(Solver& s) {
	s.strategies().heuristic.release();
	s.strategies().heuristic.reset(heu_);
	heu_ = 0;
	delete this;
}

}
