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

#include <clasp/model_enumerators.h>
#include <clasp/solver.h>
#include <clasp/smodels_constraints.h>
#include <algorithm>
namespace Clasp {

ModelEnumerator::ModelEnumerator(Report* p)
	: Enumerator(p)
	, project_(0) {
}

ModelEnumerator::~ModelEnumerator() {
	delete project_;
}

void ModelEnumerator::setEnableProjection(bool b) {
	delete project_;
	if (b) project_ = new VarVec();
}

void ModelEnumerator::doInit(Solver& s) {
	if (project_ && s.strategies().symTab.get()) {
		AtomIndex& index = *s.strategies().symTab;
		for (AtomIndex::const_iterator it = index.curBegin(); it != index.end(); ++it) {
			if (!it->second.name.empty()) {
				Var v = it->second.lit.var();
				s.setFrozen(v, true);
				if (it->second.name[0] != '_' && s.value(v) == value_free) {
					project_->push_back(v);
					s.setProject(v, true);
				}
			}
		}
		std::sort(project_->begin(), project_->end());
		project_->erase(std::unique(project_->begin(), project_->end()), project_->end());
		if (project_->empty()) { 
			// We project to the empty set. Add true-var so that 
			// we can distinguish this case from unprojected search
			project_->push_back(0);
		}
	}
}

bool ModelEnumerator::backtrack(Solver& s) {
	uint32 bl = getHighestActiveLevel();
	if (projectionEnabled()) {
		bl = std::min(bl, getProjectLevel(s));
	}
	if (bl <= s.rootLevel()) {
		return false;
	}
	return doBacktrack(s, bl-1);
}

bool ModelEnumerator::ignoreSymmetric() const {
	return projectionEnabled() || Enumerator::ignoreSymmetric();
}

/////////////////////////////////////////////////////////////////////////////////////////
// class BacktrackEnumerator
/////////////////////////////////////////////////////////////////////////////////////////
BacktrackEnumerator::BacktrackEnumerator(uint32 opts, Report* p)
	: ModelEnumerator(p)
	, projectOpts_((uint8)std::min(uint32(7), opts)) {
}

BacktrackEnumerator::~BacktrackEnumerator() {
	assert(nogoods_.empty() && "Enumerator::endSearch() not called!");
}

void BacktrackEnumerator::terminateSearch(Solver& s) {
	while (!nogoods_.empty()) {
		static_cast<Clause*>(nogoods_.back().first)->removeWatches(s);
		nogoods_.back().first->destroy();
		nogoods_.pop_back();
	}
}

uint32 BacktrackEnumerator::getProjectLevel(Solver& s) {
	uint32 maxL = 0;
	for (uint32 i = 0; i != numProjectionVars() && maxL != s.decisionLevel(); ++i) {
		if (s.level(projectVar(i)) > maxL) {
			maxL = s.level(projectVar(i));
		}
	}
	return maxL;
}

void BacktrackEnumerator::undoLevel(Solver& s) {
	while (!nogoods_.empty() && nogoods_.back().second >= s.decisionLevel()) {
		Clause* c = (Clause*)nogoods_.back().first;
		nogoods_.pop_back();
		*c->end() = posLit(0);
		c->removeWatches(s);
		c->destroy();
	}
}

uint32 BacktrackEnumerator::getHighestBacktrackLevel(const Solver& s, uint32 bl) const {
	if (!projectionEnabled() || (projectOpts_ & MINIMIZE_BACKJUMPING) == 0) {
		return s.backtrackLevel();
	}
	uint32 res = s.backtrackLevel();
	for (uint32 r = res+1; r <= bl; ++r) {
		if (!s.project(s.decision(r).var())) {
			return res;
		}
		++res;
	}
	return res;
}

bool BacktrackEnumerator::doBacktrack(Solver& s, uint32 bl) {
	// bl is the decision level on which search should proceed.
	// bl + 1 the minimum of:
	//  a) the highest DL on which one of the projected vars was assigned
	//  b) the highest DL on which one of the vars in a minimize statement was assigned
	//  c) the current decision level
	assert(bl >= s.rootLevel());
	++bl; 
	assert(bl <= s.decisionLevel());
	uint32 btLevel = getHighestBacktrackLevel(s, bl);
	if (!projectionEnabled() || bl <= btLevel) {
		// If we do not project or one of the projection vars is already on the backtracking level
		// proceed with simple backtracking.
		while (!s.backtrack() || s.decisionLevel() >= bl) {
			if (s.decisionLevel() == s.rootLevel()) {
				return false;
			}
		}
		return true;
	}
	else if (numProjectionVars() == 1u) {
		Literal x = s.trueLit(projectVar(0));
		s.undoUntil(0);
		s.force(~x, 0); // force the complement of x
		s.setBacktrackLevel(s.decisionLevel());
	}
	else {
		// Store the current projected assignment as a nogood
		// and attach it to the current decision level.
		// Once the solver goes above that level, the nogood is automatically
		// destroyed. Hence, the number of projection nogoods is linear in the
		// number of (projection) atoms.
		if ( (projectOpts_ & ENABLE_PROGRESS_SAVING) != 0 ) {
			s.strategies().saveProgress = 1;
		}
		projAssign_.clear();
		projAssign_.resize(numProjectionVars());
		LitVec::size_type front = 0;
		LitVec::size_type back  = numProjectionVars();
		for (uint32 i = 0; i != numProjectionVars(); ++i) {
			Literal x = ~s.trueLit(projectVar(i)); // Note: complement because we store the nogood as a clause!
			if (s.level(x.var()) > btLevel) {
				projAssign_[front++] = x;
			}
			else {
				projAssign_[--back] = x;
			}
		}
		s.undoUntil( btLevel );
		Literal x = projAssign_[0];
		LearntConstraint* c;
		if (front == 1) {
			// The projection nogood is unit. Force the single remaining literal
			// from the current DL. 
			++back; // so that the active part of the nogood contains at least two literals
			c = Clause::newContractedClause(s, projAssign_, back-1, back);
			s.force(x, c);
		}
		else {
			// Shorten the projection nogood by assuming one of its literals...
			if ( (projectOpts_ & ENABLE_HEURISTIC_SELECT) != 0 ) {
				x = s.strategies().heuristic->selectRange(s, &projAssign_[0], &projAssign_[0]+back);
			}
			c = Clause::newContractedClause(s, projAssign_, back-1, back);
			// to false.
			s.assume(~x);
		}
		if (s.backtrackLevel() < s.decisionLevel()) {
			// Remember that we must backtrack the current decision
			// level in order to guarantee a different projected solution.
			s.setBacktrackLevel(s.decisionLevel());
		}
		if (s.decisionLevel() != 0) {
			// Attach nogood to the current decision literal. 
			// Once the solver goes above that level, the nogood (which is then satisfied) is destroyed.
			s.addUndoWatch(s.decisionLevel(), this);
		}
		nogoods_.push_back( NogoodPair(c, s.decisionLevel()) );
		assert(s.backtrackLevel() == s.decisionLevel());
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////
// class RecordEnumerator
/////////////////////////////////////////////////////////////////////////////////////////
RecordEnumerator::RecordEnumerator(Report* p)
	: ModelEnumerator(p) {
}

RecordEnumerator::~RecordEnumerator() {
	assert(nogoods_.empty() && "Enumerator::endSearch() not called!");
}

void RecordEnumerator::terminateSearch(Solver& s) {
	while (!nogoods_.empty()) {
		static_cast<LearntConstraint*>(nogoods_.back())->removeWatches(s);
		nogoods_.back()->destroy();
		nogoods_.pop_back();
	}
}

uint32 RecordEnumerator::getProjectLevel(Solver& s) {
	solution_.clear(); solution_.setSolver(s);
	solution_.start(Constraint_t::learnt_conflict);
	for (uint32 i = 0; i != numProjectionVars(); ++i) {
		solution_.add( ~s.trueLit(projectVar(i))  );
	}
	return !solution_.empty() 
		? s.level( solution_[0].var() )
		: 0;
}

uint32 RecordEnumerator::assertionLevel(const Solver& s) {
	if (solution_.empty())      return s.decisionLevel();
	if (solution_.size() == 1)  return 0;
	return s.level(solution_[solution_.secondWatch()].var());
}

void RecordEnumerator::addSolution(Solver& s) {
	solution_.clear(); solution_.setSolver(s);
	if (minimize() && minimize()->mode() == MinimizeConstraint::compare_less) return;
	solution_.start(Constraint_t::learnt_conflict);
	for (uint32 x = s.decisionLevel(); x != 0; --x) {
		solution_.add(~s.decision(x));
	}
	if (minimize() && minimize()->models() == 1) {
		ConstraintDB::size_type i, end, j = 0;
		for (i = 0, end = nogoods_.size(); i != end; ++i) {
			Clause* c = (Clause*)nogoods_[i];
			if (c->locked(s)) {
				nogoods_[j++] = c;
			}
			else {
				c->removeWatches(s);
				c->destroy();
			}
		}
		nogoods_.erase(nogoods_.begin()+j, nogoods_.end());
	}
}

bool RecordEnumerator::simplify(Solver& s, bool r) {
	ConstraintDB::size_type i, j, end = nogoods_.size();
	for (i = j = 0; i != end; ++i) {
		Constraint* c = nogoods_[i];
		if (c->simplify(s, r))  { c->destroy(); }
		else                    { nogoods_[j++] = c;  }
	}
	nogoods_.erase(nogoods_.begin()+j, nogoods_.end());
	return false;
}

bool RecordEnumerator::doBacktrack(Solver& s, uint32 bl) {
	assert(bl >= s.rootLevel());
	if (!projectionEnabled()) {
		addSolution(s);
	}
	bl = std::min(bl, assertionLevel(s));
	if (s.backtrackLevel() > 0) {
		// must clear backtracking level;
		// not needed to guarantee redundancy-freeness
		// and may inadvertently bound undoUntil()
		s.setBacktrackLevel(0);
	}
	s.undoUntil(bl);
	if (solution_.empty()) {
		assert(minimize() && minimize()->mode() == MinimizeConstraint::compare_less);
		return true;
	}
	bool r = true;
	if (solution_.size() < 4) {
		r = solution_.end();
	}
	else {
		Literal x;
		if (s.isFalse(solution_[solution_.secondWatch()])) {
			x = solution_[0];
		}
		LearntConstraint* c = Clause::newLearntClause(s, solution_.lits(), Constraint_t::learnt_conflict, solution_.secondWatch());
		nogoods_.push_back((Clause*)c);
		r = s.force(x, c);
	}
	return r || s.resolveConflict();
}

}
