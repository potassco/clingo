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
#include <clasp/cb_enumerator.h>
#include <clasp/solver.h>
#include <clasp/clause.h>
#include <clasp/util/mutex.h>
#include <stdio.h> // sprintf
#ifdef _MSC_VER
#pragma warning (disable : 4996) // sprintf may be unfase
#endif
namespace Clasp {

/////////////////////////////////////////////////////////////////////////////////////////
// CBConsequences::SharedConstraint
/////////////////////////////////////////////////////////////////////////////////////////
class CBConsequences::SharedConstraint {
public:
	SharedConstraint() : current(0) {}
	SharedLiterals* fetch_if_neq(SharedLiterals* last) const {
		Clasp::lock_guard<Clasp::spin_mutex> lock(mutex);
		return last != current ? current->share() : 0;
	}
	void release(SharedLiterals* newLits) {
		SharedLiterals* old = current;
		{ Clasp::lock_guard<Clasp::spin_mutex> lock(mutex); current = newLits; }
		if (old) { old->release(); }
	}
	SharedLiterals*           current;
	mutable Clasp::spin_mutex mutex;
};
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
	void   doCommitModel(Enumerator& ctx, Solver& s) { static_cast<CBConsequences&>(ctx).addCurrent(s, current, s.model); }
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
CBConsequences::CBConsequences(Consequences_t type) 
	: Enumerator()
	, shared_(0)
	, type_(type) {}

Enumerator* EnumOptions::createConsEnumerator(const EnumOptions& opts) {
	return new CBConsequences(opts.enumMode == enum_brave ? CBConsequences::brave_consequences : CBConsequences::cautious_consequences);
}
CBConsequences::~CBConsequences() {
	delete shared_;
}
EnumerationConstraint* CBConsequences::doInit(SharedContext& ctx, SharedMinimizeData*, int) {
	cons_.clear();
	const SymbolTable& index = ctx.symbolTable();
	if (index.type() == SymbolTable::map_direct) {
		for (Var v = 1, end = index.size(); v < end; ++v) { 
			if (!ctx.marked(posLit(v))) {
				cons_.push_back(posLit(v));
				ctx.mark(cons_.back());
			}
		}
	}
	else {
		for (SymbolTable::const_iterator it = index.begin(); it != index.end(); ++it) {
			if (!it->second.name.empty() && !ctx.marked(it->second.lit)) { 
				cons_.push_back(it->second.lit);
				ctx.mark(cons_.back());
			}
		}
	}	
	uint32 m = (type_ == cautious_consequences);
	for (LitVec::iterator it = cons_.begin(), end = cons_.end(); it != end; ++it) {
		ctx.setFrozen(it->var(), true);
		ctx.unmark(it->var());
		it->asUint() |= m;
	}
	delete shared_;
	shared_ = ctx.concurrency() > 1 ? new SharedConstraint() : 0;
	setIgnoreSymmetric(true);
	return new CBFinder(shared_);
}
void CBConsequences::addCurrent(Solver& s, LitVec& con, ValueVec& m) {
	con.clear();
	con.push_back(~s.sharedContext()->stepLiteral());
	for (LitVec::iterator it = cons_.begin(), end = cons_.end(); it != end; ++it) {
		m[it->var()] = 0;
	}
	if (type_ == brave_consequences) {
		for (LitVec::iterator it = cons_.begin(), end = cons_.end(); it != end; ++it) {
			Literal& p = *it;
			if (s.isTrue(p) || p.watched())  { 
				m[p.var()] |= trueValue(p); 
				p.watch();
			}
			else if (s.level(p.var())) {
				con.push_back(p);
			}
		}
	}
	else if (type_ == cautious_consequences) {
		for (LitVec::iterator it = cons_.begin(), end = cons_.end(); it != end; ++it) {
			Literal& p = *it;
			if (!s.isTrue(p) || !p.watched()) {
				m[p.var()] &= ~trueValue(p);
				p.clearWatch();
			}
			else {
				if (s.level(p.var())) { con.push_back(~p); }
				m[p.var()] |= trueValue(p);
			}
		}
	}
	if (con.empty()) { con.push_back(negLit(0)); }
	if (shared_) {
		shared_->release(SharedLiterals::newShareable(con, Constraint_t::learnt_other, 1));
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// CBConsequences::CBFinder implementation
/////////////////////////////////////////////////////////////////////////////////////////
void CBConsequences::CBFinder::destroy(Solver* s, bool detach) {
	destroyDB(locked, s, detach);
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
		ret  = !current.empty() ? ClauseCreator::create(s, current, flags, ClauseInfo(Constraint_t::learnt_other)) : ClauseCreator::Result();
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
