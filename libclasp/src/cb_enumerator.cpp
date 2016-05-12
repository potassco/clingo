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

#include <clasp/cb_enumerator.h>
#include <clasp/solver.h>
#include <clasp/clause.h>
#include <clasp/smodels_constraints.h>
#include <stdio.h> // sprintf
#ifdef _MSC_VER
#pragma warning (disable : 4996) // sprintf may be unfase
#endif
namespace Clasp {

CBConsequences::CBConsequences(Consequences_t type) 
	: Enumerator(0)
	, current_(0)
	, type_(type) {
}

CBConsequences::~CBConsequences() {
	if (current_) current_->destroy();
	for (ConstraintDB::size_type i = 0; i != locked_.size(); ++i) {
		locked_[i]->destroy();
	}
	locked_.clear();
}

void CBConsequences::doInit(Solver& s) {
	if (!s.strategies().symTab.get()) {
		s.strategies().symTab.reset(new AtomIndex());
		s.strategies().symTab->startInit();
		char buf[1024];
		for (Var v = 1; v <= s.numVars(); ++v) {
			sprintf(buf, "x%u", v);
			Atom a(buf);
			a.lit = posLit(v);
			s.strategies().symTab->addUnique(v, a);
		}
		s.strategies().symTab->endInit();
	}
	AtomIndex& index = *s.strategies().symTab;
	for (AtomIndex::const_iterator it = index.curBegin(); it != index.end(); ++it) {
		if (!it->second.name.empty()) { 
			s.setFrozen(it->second.lit.var(), true);
			if (type_ == cautious_consequences) {
				it->second.lit.watch();  
			}
		}
	} 
}

bool CBConsequences::ignoreSymmetric() const { return true; }

void CBConsequences::updateModel(Solver& s) {
	C_.clear();
	type_ == brave_consequences
			? updateBraveModel(s)
			: updateCautiousModel(s);
	for (LitVec::size_type i = 0; i != C_.size(); ++i) {
		s.clearSeen(C_[i].var());
	}
}

void CBConsequences::terminateSearch(Solver & s) {
	removeConstraints(s);
}

// Delete old constraints that are no longer locked.
void CBConsequences::removeConstraints(Solver& s) {
	ConstraintDB::size_type j = 0; 
	for (ConstraintDB::size_type i = 0; i != locked_.size(); ++i) {
		LearntConstraint* c = (LearntConstraint*)locked_[i];
		if (c->locked(s)) locked_[j++] = c;
		else c->destroy();
	}
	locked_.erase(locked_.begin()+j, locked_.end());
	if (current_ != 0) {
		LearntConstraint* c = (LearntConstraint*)current_;
		c->removeWatches(s);
		if (!c->locked(s)) {
			c->destroy();
		}
		else {
			locked_.push_back(current_);
		}
		current_ = 0;
	}
}

void CBConsequences::addNewConstraint(Solver& s) {
	removeConstraints(s);
	if (C_.size() > 1) {
		ClauseCreator nc(&s);
		nc.start(Constraint_t::learnt_conflict);
		for (LitVec::size_type i = 0; i != C_.size(); ++i) {
			nc.add(~C_[i]);
		}
		// Create new clause, but do not add to DB of solver.
		current_ = Clause::newLearntClause(s, nc.lits(), Constraint_t::learnt_conflict,  nc.sw());
	}
}

void CBConsequences::add(Solver& s, Literal p) {
	assert(s.isTrue(p));
	if (!s.seen(p.var())) {
		C_.push_back(p);
		if (s.level(p.var()) > s.level(C_[0].var())) {
			std::swap(C_[0], C_.back());
		}
		s.markSeen(p);
	}
}

void CBConsequences::updateBraveModel(Solver& s) {
	assert(s.strategies().symTab.get() && "CBConsequences: Symbol table not set!\n");
	AtomIndex& index = *s.strategies().symTab;
	for (AtomIndex::const_iterator it = index.begin(); it != index.end(); ++it) {
		if (!it->second.name.empty()) {
			Literal& p = it->second.lit;
			if (s.isTrue(p))  { p.watch(); }
			if (!p.watched()) { add(s, ~p); }
		}
	}
}

void CBConsequences::updateCautiousModel(Solver& s) {
	assert(s.strategies().symTab.get() && "CBConsequences: Symbol table not set!\n");
	AtomIndex& index = *s.strategies().symTab;
	for (AtomIndex::const_iterator it = index.begin(); it != index.end(); ++it) {
		Literal& p = it->second.lit;
		if (p.watched()) { 
			if      (s.isFalse(p))  { p.clearWatch(); }
			else if (s.isTrue(p))   { add(s, p); }
		}
	}
}

bool CBConsequences::backtrack(Solver& s) {
	if (C_.empty()) return false;
	if (s.backtrackLevel() > 0) {
		s.setBacktrackLevel(0);
	}
	// C_ stores the violated nogood, ie. the new integrity constraint.
	// C_[0] is the literal assigned on the highest DL and hence the
	// decision level on which we must analyze the conflict.
	uint32 newDl = s.level(C_[0].var());
	if (getHighestActiveLevel() < newDl) {
		// C_ is not the most important nogood, ie. there is some other
		// nogood that is violated below C_. 
		newDl = getHighestActiveLevel() - 1;
	}
	s.undoUntil(newDl);
	addNewConstraint(s);
	if (s.isTrue(C_[0])) {
		// C_ is still violated - set and resolve conflict
		s.setConflict(C_);
		return s.resolveConflict();
	}
	return true;
}

}
