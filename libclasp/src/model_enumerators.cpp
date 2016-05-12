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
#include <clasp/model_enumerators.h>
#include <clasp/solver.h>
#include <clasp/minimize_constraint.h>
#include <algorithm>
#include <cstdlib>
namespace Clasp {
class ModelEnumerator::ModelFinder : public EnumerationConstraint {
protected:
	explicit ModelFinder(VarVec* p) : EnumerationConstraint(), project(p) {}
	bool hasModel() const { return !solution.empty(); }
	void destroy(Solver* s, bool detach) {
		if (project && s && s->sharedContext()->master() == s) {
			SharedContext& problem = const_cast<SharedContext&>(*s->sharedContext());
			while (!project->empty()) {
				problem.setProject(project->back(), false);
				project->pop_back();
			}
		}
		delete project;
		EnumerationConstraint::destroy(s, detach);
	}
	VarVec* project;
	LitVec  solution;
};
/////////////////////////////////////////////////////////////////////////////////////////
// strategy_record
/////////////////////////////////////////////////////////////////////////////////////////
class ModelEnumerator::RecordFinder : public ModelFinder {
public:
	explicit RecordFinder(VarVec* project) : ModelFinder(project) { }
	ConPtr clone() { return new RecordFinder(0); }
	void   doCommitModel(Enumerator& ctx, Solver& s);
	bool   doUpdate(Solver& s);
};

bool ModelEnumerator::RecordFinder::doUpdate(Solver& s) {
	if (hasModel()) {
		ClauseInfo e(Constraint_t::learnt_other);
		ClauseCreator::Result ret = ClauseCreator::create(s, solution, ClauseCreator::clause_no_add, e);
		solution.clear();
		if (ret.local) { add(ret.local);}
		if (!ret.ok()) { return false;  }
	}
	return true;
}

void ModelEnumerator::RecordFinder::doCommitModel(Enumerator& en, Solver& s) {
	ModelEnumerator& ctx = static_cast<ModelEnumerator&>(en);
	assert(solution.empty() && "Update not called!");
	solution.clear();
	const LitVec* dom = (ctx.projectOpts() & uint32(project_dom_lits)) != 0 ? &s.sharedContext()->symbolTable().domLits : 0;
	if (dom && dom->empty()) {
		if (en.lastModel().num == 0) { s.sharedContext()->report(warning(Event::subsystem_solve, "domRec ignored: no domain atoms found!")); }
		dom = 0;
	}
	if (ctx.trivial() && !dom) { 
		return; 
	}
	else if (!dom && !ctx.projectionEnabled()) { 
		for (uint32 x = s.decisionLevel(); x != 0; --x) {
			Literal d = s.decision(x);
			if      (!s.auxVar(d.var()))  { solution.push_back(~d); }
			else if (d != s.tagLiteral()) {
				// Todo: set of vars could be reduced to those having the aux var in their reason set.
				const LitVec& tr = s.trail();
				const uint32  end= x != s.decisionLevel() ? s.levelStart(x+1) : (uint32)tr.size();
				for (uint32 n = s.levelStart(x)+1; n != end; ++n) {
					if (!s.auxVar(tr[n].var())) { solution.push_back(~tr[n]); }
				}
			}
		}
	}
	else if (!dom) {
		for (uint32 i = 0, end = ctx.numProjectionVars(); i != end; ++i) {
			solution.push_back(~s.trueLit(ctx.projectVar(i)));
		}
	}
	else {
		const bool project = ctx.projectionEnabled();
		for (LitVec::const_iterator it = dom->begin(), end = dom->end(); it != end; ++it) {
			if (s.isTrue(*it) && (!project || s.sharedContext()->varInfo(it->var()).project())) { solution.push_back(~*it); }
		}
		solution.push_back(~s.sharedContext()->stepLiteral());
	}
	if (solution.empty()) { solution.push_back(negLit(0)); }
	if (s.sharedContext()->concurrency() > 1) {
		// parallel solving active - share solution nogood with other solvers
		en.commitClause(solution);
		solution.clear();
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// strategy_backtrack
/////////////////////////////////////////////////////////////////////////////////////////
class ModelEnumerator::BacktrackFinder : public ModelFinder {
public:
	BacktrackFinder(VarVec* project, uint32 projOpts) : ModelFinder(project), opts(projOpts) {}
	// EnumerationConstraint interface
	ConPtr clone() { return new BacktrackFinder(0, opts); }
	void   doCommitModel(Enumerator& ctx, Solver& s);
	bool   doUpdate(Solver& s);
	// Constraint interface
	PropResult  propagate(Solver&, Literal, uint32&);
	void        reason(Solver& s, Literal p, LitVec& x){
		for (uint32 i = 1, end = s.level(p.var()); i <= end; ++i) { 
			x.push_back(s.decision(i)); 
		}
	}
	bool simplify(Solver& s, bool reinit) {
		for (ProjectDB::iterator it = projNogoods.begin(), end = projNogoods.end(); it != end; ++it) {
			if (it->second && it->second->simplify(s, false)) { 
				s.removeWatch(it->first, this);
				it->second->destroy(&s, false);
				it->second = 0;
			}
		}
		while (!projNogoods.empty() && projNogoods.back().second == 0) { projNogoods.pop_back(); }
		return ModelFinder::simplify(s, reinit);
	}
	void destroy(Solver* s, bool detach) {
		while (!projNogoods.empty()) {
			NogoodPair x = projNogoods.back();
			if (x.second) {
				if (s) { s->removeWatch(x.first, this); }
				x.second->destroy(s, detach);
			}
			projNogoods.pop_back();
		}
		ModelFinder::destroy(s, detach);
	}
	typedef std::pair<Literal, Constraint*> NogoodPair;
	typedef PodVector<NogoodPair>::type     ProjectDB;
	ProjectDB projNogoods;
	uint32    opts;
};

Constraint::PropResult ModelEnumerator::BacktrackFinder::propagate(Solver& s, Literal, uint32& pos) {
	assert(pos < projNogoods.size() && projNogoods[pos].second != 0);
	ClauseHead* c = static_cast<ClauseHead*>(projNogoods[pos].second);
	if (!c->locked(s)) {
		c->destroy(&s, true);
		projNogoods[pos].second = (c = 0);
		while (!projNogoods.empty() && !projNogoods.back().second) {
			projNogoods.pop_back();
		}
	}
	return PropResult(true, c != 0);
}
bool ModelEnumerator::BacktrackFinder::doUpdate(Solver& s) {
	if (hasModel()) {
		bool   ok = true;
		uint32 sp = (opts & ModelEnumerator::project_save_progress) != 0 ? Solver::undo_save_phases : 0;
		s.undoUntil(s.backtrackLevel(), sp);
		ClauseRep rep = ClauseCreator::prepare(s, solution, 0, Constraint_t::learnt_conflict);
		if (rep.size == 0 || s.isFalse(rep.lits[0])) { // The decision stack is already ordered.
			ok = s.backtrack();
		}
		else if (rep.size == 1 || s.isFalse(rep.lits[1])) { // The projection nogood is unit. Force the single remaining literal from the current DL.
			ok = s.force(rep.lits[0], this);
		}
		else if (!s.isTrue(rep.lits[0])) { // Shorten the projection nogood by assuming one of its literals to false.
			uint32  f = static_cast<uint32>(std::stable_partition(rep.lits+2, rep.lits+rep.size, std::not1(std::bind1st(std::mem_fun(&Solver::isFalse), &s))) - rep.lits);
			Literal x = (opts & ModelEnumerator::project_use_heuristic) != 0 ? s.heuristic()->selectRange(s, rep.lits, rep.lits+f) : rep.lits[0];
			LearntConstraint* c = Clause::newContractedClause(s, rep, f, true);
			CLASP_FAIL_IF(!c, "Invalid constraint!");
			s.assume(~x);
			// Remember that we must backtrack the current decision
			// level in order to guarantee a different projected solution.
			s.setBacktrackLevel(s.decisionLevel());
			// Attach nogood to the current decision literal. 
			// Once we backtrack to x, the then obsolete nogood is destroyed 
			// keeping the number of projection nogoods linear in the number of (projection) atoms.
			s.addWatch(x, this, (uint32)projNogoods.size());
			projNogoods.push_back(NogoodPair(x, c));
			ok = true;
		}
		solution.clear();
		return ok;
	}
	if (optimize() || s.sharedContext()->concurrency() == 1 || disjointPath()) {
		return true;
	}
	s.setStopConflict();
	return false;
}

void ModelEnumerator::BacktrackFinder::doCommitModel(Enumerator& ctx, Solver& s) {
	ModelEnumerator& en = static_cast<ModelEnumerator&>(ctx);
	uint32           dl = s.decisionLevel();
	solution.assign(1, dl ? ~s.decision(dl) : negLit(0));
	if (en.projectionEnabled()) {
		// Remember the current projected assignment as a nogood.
		solution.clear();
		for (uint32 i = 0, end = en.numProjectionVars(); i != end; ++i) {
			solution.push_back(~s.trueLit(en.projectVar(i)));
		}
		// Remember initial decisions that are projection vars.
		for (dl = s.backtrackLevel(); dl < s.decisionLevel(); ++dl) {
			if (!s.varInfo(s.decision(dl+1).var()).project()) { break; }
		}
	}
	s.setBacktrackLevel(dl);
}
/////////////////////////////////////////////////////////////////////////////////////////
// class ModelEnumerator
/////////////////////////////////////////////////////////////////////////////////////////
ModelEnumerator::ModelEnumerator(Strategy st)
	: Enumerator()
	, project_(0)
	, options_(st) {
}

Enumerator* EnumOptions::createModelEnumerator(const EnumOptions& opts) {
	ModelEnumerator*          e = new ModelEnumerator();
	ModelEnumerator::Strategy s = ModelEnumerator::strategy_auto;
	if (opts.enumMode && opts.models()) {
		s = opts.enumMode == enum_bt ? ModelEnumerator::strategy_backtrack : ModelEnumerator::strategy_record;
	}
	e->setStrategy(s, opts.project | (opts.enumMode == enum_dom_record ? ModelEnumerator::project_dom_lits : 0));
	return e;
}

ModelEnumerator::~ModelEnumerator() {}

void ModelEnumerator::setStrategy(Strategy st, uint32 projection) {
	options_ = uint32(st) | ((projection & 15u) << 4u);
	project_ = 0;
	if ((projection & 15u) != 0) { 
		options_ |= uint32(project_enable_simple) << 4u;
		project_  = new VarVec();
	}
	if (st == strategy_auto) {
		options_ |= detect_strategy_flag;
	}
}

EnumerationConstraint* ModelEnumerator::doInit(SharedContext& ctx, SharedMinimizeData* opt, int numModels) {
	initProjection(ctx); 
	uint32 st = strategy();
	if (detectStrategy() || (ctx.concurrency() > 1 && !ModelEnumerator::supportsParallel())) {
		st = 0;
	}
	bool optOne  = opt && opt->mode() == MinimizeMode_t::optimize;
	bool trivial = optOne || std::abs(numModels) == 1;
	if (optOne && project_.get()) {
		for (const WeightLiteral* it =  minimizer()->lits; !isSentinel(it->first) && trivial; ++it) {
			trivial = ctx.varInfo(it->first.var()).project();
		}
		if (!trivial) { ctx.report(warning(Event::subsystem_prepare, "Projection: Optimization may depend on enumeration order.")); }
	}
	if (st == strategy_auto) { st  = trivial || (project_.get() && ctx.concurrency() > 1) ? strategy_record : strategy_backtrack; }
	if (trivial)             { st |= trivial_flag; }
	options_ &= ~uint32(strategy_opts_mask);
	options_ |= st;
	EnumerationConstraint* c = st == strategy_backtrack 
	  ? static_cast<ConPtr>(new BacktrackFinder(project_.release(), projectOpts()))
	  : static_cast<ConPtr>(new RecordFinder(project_.release()));
	if (projectionEnabled()) { setIgnoreSymmetric(true); }
	return c;
}

void ModelEnumerator::initProjection(SharedContext& ctx) {
	if (!projectionEnabled()) { return; }
	if (!project_.is_owner()) { project_ = new VarVec(); }
	project_->clear();
	const SymbolTable& index = ctx.symbolTable();
	if (index.type() == SymbolTable::map_indirect) {
		for (SymbolTable::const_iterator it = index.begin(); it != index.end(); ++it) {
			if (!it->second.name.empty() && it->second.name[0] != '_') {
				addProjectVar(ctx, it->second.lit.var(), true);
			}
		}
		for (VarVec::const_iterator it = project_->begin(), end = project_->end(); it != end; ++it) {
			ctx.unmark(*it);
		}
	}
	else {
		for (Var v = 1; v < index.size(); ++v) { addProjectVar(ctx, v, false); }
	}
	// tag projection nogoods with step literal (if any).
	addProjectVar(ctx, ctx.stepLiteral().var(), false);
	if (project_->empty()) { 
		// We project to the empty set. Add true-var so that 
		// we can distinguish this case from unprojected search
		project_->push_back(0);
	}
}

void ModelEnumerator::addProjectVar(SharedContext& ctx, Var v, bool tag) {
	if (ctx.master()->value(v) == value_free && (!tag || !ctx.marked(posLit(v)))) {
		project_->push_back(v);
		ctx.setFrozen(v, true);
		ctx.setProject(v, true);
		if (tag) { ctx.mark(posLit(v)); ctx.mark(negLit(v)); }
	}
}

}
