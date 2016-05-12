// 
// Copyright (c) 2006-2016, Benjamin Kaufmann
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
#include <clasp/util/atomic.h>
#if WITH_THREADS
#include <clasp/util/thread.h>
#define ACQUIRE_LOCK(m) while ( (m).fetch_and_store(1) != 0 ) Clasp::mt::this_thread::yield()
#define RELEASE_LOCK(m) (m) = 0
#else
#define ACQUIRE_LOCK(m)
#define RELEASE_LOCK(m)
#endif
namespace Clasp {
/////////////////////////////////////////////////////////////////////////////////////////
// CBConsequences::SharedConstraint
/////////////////////////////////////////////////////////////////////////////////////////
class CBConsequences::SharedConstraint {
public:
	SharedConstraint() : current(0) { mutex = 0; }
	SharedLiterals* fetch_if_neq(SharedLiterals* last) const {
		ACQUIRE_LOCK(mutex);
		SharedLiterals* ret = last != current ? current->share() : 0;
		RELEASE_LOCK(mutex);
		return ret;
	}
	void release(SharedLiterals* newLits) {
		ACQUIRE_LOCK(mutex);
		SharedLiterals* old = current;
		current = newLits;
		RELEASE_LOCK(mutex);
		if (old) { old->release(); }
	}
	SharedLiterals* current;
	typedef Clasp::Atomic_t<int>::type MutexType;
	mutable MutexType mutex;
};
#undef ACQUIRE_LOCK
#undef RELEASE_LOCK
/////////////////////////////////////////////////////////////////////////////////////////
// CBConsequences::CBFinder
/////////////////////////////////////////////////////////////////////////////////////////
class CBConsequences::CBFinder : public EnumerationConstraint {
public:
	typedef CBConsequences::SharedConstraint  SharedCon;
	typedef Solver::ConstraintDB              ConstraintDB;
	typedef SharedLiterals                    SharedLits;
	explicit CBFinder(SharedCon* sh) : EnumerationConstraint(), shared(sh), last(0) {}
	ConPtr clone() { return new CBFinder(shared); }
	void   doCommitModel(Enumerator& ctx, Solver& s) { static_cast<CBConsequences&>(ctx).addCurrent(s, current, s.model, rootLevel()); }
	void   destroy(Solver* s, bool detach);
	bool   doUpdate(Solver& s);
	void   pushLocked(Solver& s, ClauseHead* h);
	LitVec       current;
	SharedCon*   shared;
	SharedLits*  last;
	ConstraintDB locked;
};
/////////////////////////////////////////////////////////////////////////////////////////
// CBConsequences
/////////////////////////////////////////////////////////////////////////////////////////
CBConsequences::CBConsequences(Type type) 
	: Enumerator()
	, shared_(0)
	, type_(type) {}

Enumerator* EnumOptions::createConsEnumerator(const EnumOptions& opts) {
	return new CBConsequences(opts.enumMode == enum_brave ? CBConsequences::Brave : CBConsequences::Cautious);
}
CBConsequences::~CBConsequences() {
	delete shared_;
}
EnumerationConstraint* CBConsequences::doInit(SharedContext& ctx, SharedMinimizeData*, int) {
	cons_.clear();
	const OutputTable& out = ctx.output;
	if (out.projectMode() == OutputTable::project_output) {
		for (OutputTable::pred_iterator it = out.pred_begin(), end = out.pred_end(); it != end; ++it) {
			addLit(ctx, it->cond);
		}
		for (OutputTable::range_iterator it = out.vars_begin(), end = out.vars_end(); it != end; ++it) {
			addLit(ctx, posLit(*it));
		}
	}
	else {
		for (OutputTable::lit_iterator it = out.proj_begin(), end = out.proj_end(); it != end; ++it) {
			addLit(ctx, *it);
		}
	}
	// init M to either cons or {} depending on whether we compute cautious or brave cons.
	const uint32 fMask = (type_ == Cautious);
	const uint32 vMask = (type_ == Cautious) ? 3u : 0u;
	for (LitVec::iterator it = cons_.begin(), end = cons_.end(); it != end; ++it) {
		it->rep() |= fMask;
		ctx.unmark(it->var());
		if (!ctx.varInfo(it->var()).nant()) {
			ctx.master()->setPref(it->var(), ValueSet::def_value, static_cast<ValueRep>(trueValue(*it) ^ vMask));
		}
	}
	delete shared_;
	shared_ = ctx.concurrency() > 1 ? new SharedConstraint() : 0;
	setIgnoreSymmetric(true);
	return new CBFinder(shared_);
}
void CBConsequences::addLit(SharedContext& ctx, Literal p) {
	if (!ctx.marked(p) && !ctx.eliminated(p.var())) {
		cons_.push_back(p);
		ctx.setFrozen(p.var(), true);
		ctx.mark(p);
	}
}
void CBConsequences::addCurrent(Solver& s, LitVec& con, ValueVec& m, uint32 root) {
	con.assign(1, ~s.sharedContext()->stepLiteral());
	// reset state of relevant variables
	for (LitVec::iterator it = cons_.begin(), end = cons_.end(); it != end; ++it) {
		m[it->var()] = 0;
	}
	// let M be all lits p with p.watch() == true
	for (LitVec::iterator it = cons_.begin(), end = cons_.end(); it != end; ++it) {
		Literal& p = *it;
		uint32  dl = s.level(p.var());
		uint32 ost = dl > root ? Model::estMask(p) : 0;
		if (type_ == Brave) {
			// brave: extend M with true literals and force a literal not in M to true
			if      (p.flagged() || s.isTrue(p)) { p.flag(); ost = 0; }
			else if (dl)                         { con.push_back(p); }
		}
		else if (type_ == Cautious) {
			// cautious: intersect M with true literals and force a literal in M to false
			if      (!p.flagged() || s.isFalse(p)) { p.unflag(); ost = 0; }
			else if (dl)                           { con.push_back(~p); }
		}
		// set output state
		if (p.flagged()) { ost |= trueValue(p); }
		m[p.var()] |= ost;
	}
	if (shared_) {
		shared_->release(SharedLiterals::newShareable(con, Constraint_t::Other, 1));
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// CBConsequences::CBFinder implementation
/////////////////////////////////////////////////////////////////////////////////////////
void CBConsequences::CBFinder::destroy(Solver* s, bool detach) {
	Clasp::destroyDB(locked, s, detach);
	if (last) {
		last->release();
	}
	EnumerationConstraint::destroy(s, detach);
}
void CBConsequences::CBFinder::pushLocked(Solver& s, ClauseHead* c) {
	for (ClauseHead* h; !locked.empty() && !(h = static_cast<ClauseHead*>(locked.back()))->locked(s);) {
		h->destroy(&s, true);
		locked.pop_back();
	}
	locked.push_back(c);
}
bool CBConsequences::CBFinder::doUpdate(Solver& s) {
	ClauseCreator::Result ret;
	uint32 flags = ClauseCreator::clause_explicit|ClauseCreator::clause_no_add;
	if (!shared) {
		ret  = !current.empty() ? ClauseCreator::create(s, current, flags, ConstraintInfo(Constraint_t::Other)) : ClauseCreator::Result();
	}
	else if (SharedLiterals* x = shared->fetch_if_neq(last)) {
		if (last) { last->release(); }
		last = x;
		ret  = ClauseCreator::integrate(s, x, flags | ClauseCreator::clause_no_release);
	}
	if (ret.local) { pushLocked(s, ret.local); }
	current.clear();
	return ret.ok();
}
}
