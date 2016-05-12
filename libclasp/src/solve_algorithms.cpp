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
#include <clasp/solve_algorithms.h>
#include <clasp/solver.h>
#include <clasp/smodels_constraints.h>
#include <cmath>
using std::log;
namespace Clasp { 

/////////////////////////////////////////////////////////////////////////////////////////
// Enumerator
/////////////////////////////////////////////////////////////////////////////////////////
Enumerator::Enumerator(Report* r) : numModels_(1), report_(r), progress_(0), mini_(0), limits_(0), restartOnModel_(false)  {}
Enumerator::~Enumerator()     { if (mini_) mini_->destroy(); delete limits_; }
Enumerator::ProgressReport::ProgressReport()  {}
Enumerator::ProgressReport::~ProgressReport() {}
Enumerator::Report::Report()  {}
void Enumerator::setReport(Report* r)                   {  report_   = r; }
void Enumerator::enableProgressReport(ProgressReport* r){  progress_ = r; }
void Enumerator::setMinimize(MinimizeConstraint* min)   {  mini_ = min;   }
void Enumerator::setRestartOnModel(bool r)              { restartOnModel_ = r; }
void Enumerator::setSearchLimit(int64 maxC, int64 maxR) {
	delete limits_; limits_ = 0;
	if (maxC > 0 || maxR > 0) {
		limits_ = new SearchLimits();
		limits_->conflicts = maxC >= 0 ? maxC   : static_cast<int64>((uint64(1)<<63)-1);
		limits_->restarts  = maxR >= 0 ? maxR+1 : static_cast<int64>((uint64(1)<<63)-1);
	}
}
void Enumerator::init(Solver& s, uint64 m) { 
	numModels_ = m; 
	if (s.strategies().satPrePro.get() != 0) {
		s.strategies().satPrePro.get()->setEnumerate(numModels_ != 1);
	}
	doInit(s);
}
Constraint::PropResult 
Enumerator::propagate(const Literal&, uint32&, Solver&)   { return PropResult(true, false); }
ConstraintType Enumerator::reason(const Literal&, LitVec&){ return Enumerator::type(); }
ConstraintType Enumerator::type() const                   { return Constraint_t::static_constraint; }
void Enumerator::updateModel(Solver&) {}
void Enumerator::terminateSearch(Solver&) {}
bool Enumerator::ignoreSymmetric() const {
	return mini_ && mini_->mode() == MinimizeConstraint::compare_less;
}
bool Enumerator::continueFromModel(Solver& s) {
	if (restartOnModel_) {
		s.undoUntil(0);
	}
	if (mini_ && !mini_->backpropagate(s)) {
		return false;
	}
	if (mini_ && restartOnModel_ && s.queueSize() == 0) {
		mini_->select(s);
	}
	return true;
}

bool Enumerator::backtrackFromModel(Solver& s) {
	bool expandSym = !ignoreSymmetric();
	do {
		if (!onModel(s)) { // enough models enumerated?
			return false;
		}
		// Process symmetric models, i.e. models that differ only in the 
		// assignment of atoms outside of the solver's assignment. 
		// Typical example: vars eliminated by the SAT-preprocessor
	} while (s.nextSymModel(expandSym));
	if (activeLevel_ <= s.rootLevel() || !(backtrack(s) && continueFromModel(s))) {
		s.setBacktrackLevel(0);
		s.undoUntil(0);
		return false;
	}
	return true;
}

bool Enumerator::onModel(Solver& s) {
	updateModel(s); // Hook for derived classes
	activeLevel_ = mini_ ? mini_->setModel(s)+1 : s.decisionLevel();
	if (report_) {
		report_->reportModel(s, *this);
	}
	return numModels_ == 0 || --numModels_ != 0;
}

void Enumerator::endSearch(Solver& s, bool complete) {
	terminateSearch(s); // Hook for derived classes
	if (report_) {
		report_->reportSolution(s, *this, complete);
	}
}

ValueRep Enumerator::search(Solver& s, uint64 maxC, uint32 maxL, double rf, bool localR) {
	if (limits_ && maxC > static_cast<uint64>(limits_->conflicts)) {
		maxC = static_cast<uint64>(limits_->conflicts);
	}
	if (progress_) progress_->reportRestart(s, maxC, maxL);
	uint64 currentC = s.stats.solve.conflicts;
	ValueRep v = s.search(maxC, maxL, rf, localR);
	if (limits_ && !limits_->update(s.stats.solve.conflicts - currentC, static_cast<uint64>(v == value_free))) {
		limits_->conflicts = 0;
		limits_->restarts  = 0;
		v = set_bit(v, LIMIT_BIT);
	}
	return v;
}

namespace {
class NullEnum : public Enumerator {
private:
	bool backtrack(Solver&) { return false; }
	void doInit(Solver&)    {}
};
}
/////////////////////////////////////////////////////////////////////////////////////////
// SolveParams
/////////////////////////////////////////////////////////////////////////////////////////
SolveParams::SolveParams() 
	: randFreq_(0.0)
	, enumerator_(new NullEnum())
	, randRuns_(0), randConflicts_(0)
	, shuffleFirst_(0), shuffleNext_(0) {
}

double SolveParams::computeReduceBase(const Solver& s) const {
	double r = !reduce.disable ? (double)reduce.base() : (double)std::numeric_limits<uint32>::max();
	if (r == 0) {
		r = std::min(s.numVars(), s.numConstraints()) / 3.0;
	}
	if (r < s.numLearntConstraints()) { 
		r = std::min(r + s.numLearntConstraints(), (double)std::numeric_limits<uint32>::max());
	}
	return r;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Restarts
/////////////////////////////////////////////////////////////////////////////////////////
namespace {
// Implements clasp's configurable restart-strategies.
// Note: Currently all restart-strategies can be easily implemented using one class.
// In the future, as new restart strategies emerge, the class should be replaced with a strategy hierarchy
struct RestartStrategy {
public:
	RestartStrategy(const RestartParams& p) 
		: grow_(p.inc())
		, outer_(p.outer() ? p.outer() : UINT64_MAX)
		, base_(p.base())
		, idx_(0) {}
	void reset()  { idx_ = 0; }
	uint64 next() {
		uint64 x;
		if      (base_ == 0)      x = UINT64_MAX;
		else if (grow_ == 0)      x = lubyR();
		else                      x = innerOuterR();
		++idx_;
		return x;
	}
private:
	uint64 geomR() const { return static_cast<uint64>(base_ * pow(grow_, (double)idx_)); }
	uint64 lubyR() const {
		uint32 k = idx_+1;
		while (k) {
			uint32 nk = static_cast<uint32>(log((double)k) / log(2.0)) + 1;
			if (k == ((uint32(1) << nk) - 1)) {
				return base_ * (uint32(1) << (nk-1));
			}
			k -= uint32(1) << (nk-1);
			++k;
		}
		return base_;
	}
	uint64 innerOuterR() {
		uint64 x = geomR();
		if (x > outer_) {
			idx_    = 0;
			outer_  = static_cast<uint64>(outer_*grow_);
			return geomR();
		}
		return x;
	}
	double  grow_;
	uint64  outer_;
	uint32  base_;
	uint32  idx_;
};

}
/////////////////////////////////////////////////////////////////////////////////////////
// solve
/////////////////////////////////////////////////////////////////////////////////////////
bool solve(Solver& s, const SolveParams& p) {
	s.stats.solve.reset();
	if (s.hasConflict()) return false;
	double maxLearnts   = p.computeReduceBase(s);
	double boundLearnts = p.reduce.max();
	RestartStrategy rs(p.restart);
	ValueRep result;
	uint32 randRuns = p.randRuns();
	double randFreq = randRuns == 0 ? p.randomProbability() : 1.0;
	uint64 maxCfl   = randRuns == 0 ? rs.next() : p.randConflicts();
	uint32 shuffle  = p.shuffleBase();
	do {
		result    = p.enumerator()->search(s, maxCfl, (uint32)maxLearnts, randFreq, p.restart.local);
		if ((result & value_true) != 0) {
			if (!p.enumerator()->backtrackFromModel(s)) {
				break; // No more models requested
			}
			else {
				// continue enumeration
				// but cancel remaining probings
				randRuns = 0;        
				randFreq = p.randomProbability();
				if (p.restart.resetOnModel) {
					rs.reset();
					maxCfl = rs.next();
				}
				if (!p.restart.bounded && s.backtrackLevel() > 0) {
					// After the first solution was found, we allow further restarts only if this
					// is compatible with the enumerator used. 
					maxCfl = static_cast<uint64>(-1);	
				}
			}
		}
		else if (result == value_free){  // restart search
			if (randRuns == 0) {
				maxCfl = rs.next();
				if (p.reduce.reduceOnRestart) { s.reduceLearnts(.33f); }
				if (maxLearnts != (double)std::numeric_limits<uint32>::max() && maxLearnts < boundLearnts && (s.numLearntConstraints()+maxCfl) > maxLearnts) {
					maxLearnts = std::min(maxLearnts*p.reduce.inc(), (double)std::numeric_limits<uint32>::max());
				}
				if (++s.stats.solve.restarts == shuffle) {
					shuffle += p.shuffleNext();
					s.shuffleOnNextSimplify();
				}
			}
			else if (--randRuns == 0) {
				maxCfl    = rs.next();
				randFreq  = p.randomProbability();
			} 
		}
	} while (result < value_false);
	store_clear_bit(result, Enumerator::LIMIT_BIT);
	bool more = result == value_free || s.decisionLevel() > s.rootLevel();
	p.enumerator()->endSearch(s, !more);
	s.undoUntil(0);
	return more;
}

bool solve(Solver& s, const LitVec& assumptions, const SolveParams& params) {
	s.stats.solve.reset();
	// Remove any existing assumptions and simplify problem.
	// If this fails, the problem is unsat, even under no assumptions.
	if (!s.clearAssumptions()) return false;
	// Next, add assumptions.
	// If this fails, the problem is unsat under the current assumptions
	// but not necessarily unsat.
	LitVec::size_type i;
	for (i = 0; i != assumptions.size(); ++i) {
		Literal p = assumptions[i];
		if (s.value(p.var()) == value_free) {
			s.assume(p); --s.stats.solve.choices;
			// increase root level - assumption can't be undone during search
			s.setRootLevel(s.decisionLevel());
			if (!s.propagate())   { break; }
		} 
		else if (s.isFalse(p))  { s.analyzeRootConflict(~p); break; }
	}
	bool ret = i == assumptions.size() && solve(s, params);
	// Find the subset of assumptions that led to the final conflict
	if (s.hasConflict()) { s.analyzeRootConflict(); }
	// Finally, remove the assumptions again and restore
	// the solver to a usable state if possible.
	s.clearAssumptions();
	return ret;
}
}
