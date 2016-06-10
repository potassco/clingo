// 
// Copyright (c) 2015-2016, Benjamin Kaufmann
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
#include <clasp/clingo.h>
#include <clasp/solver.h>
#include <clasp/clause.h>
#include <clasp/util/atomic.h>
#include <algorithm>
namespace Clasp { namespace {
	template <class O, class L, void (L::*enter)(), void (L::*exit)(), class OP>
	struct Scoped {
		Scoped(L* lock, O* obj, const OP& op = OP()) : lock_(lock), obj_(obj), op_(op) { if (lock) (lock->*enter)(); }
		~Scoped() { if (lock_) (lock_->*exit)(); }
		O* operator->() const { op_(); return obj_; }
		L* lock_;
		O* obj_;
		OP op_;
	};
	struct Nop { void operator()() const {} };
	struct Inc { Inc(uint32& e) : epoch_(&e) {} void operator()() const { ++*epoch_; } uint32* epoch_; };
	typedef Scoped<Potassco::AbstractPropagator, ClingoPropagatorLock, &ClingoPropagatorLock::lock, &ClingoPropagatorLock::unlock, Inc> ScopedLock;
	typedef Scoped<ClingoPropagator, ClingoPropagatorLock, &ClingoPropagatorLock::unlock, &ClingoPropagatorLock::lock, Nop> ScopedUnlock;
}
ClingoPropagatorLock::~ClingoPropagatorLock() {}
/////////////////////////////////////////////////////////////////////////////////////////
// ClingoPropagator::Control
/////////////////////////////////////////////////////////////////////////////////////////
class ClingoPropagator::Control : public Potassco::AbstractSolver, Potassco::AbstractAssignment {
public:
	Control(ClingoPropagator& ctx, Solver& s, uint32 st = 0u) : ctx_(&ctx), s_(&s), state_(st | state_ctrl) {}
	const AbstractAssignment& assignment() const { return *this; }
	// AbstractAssignment
	virtual bool     hasConflict()     const { return s_->hasConflict(); }
	virtual uint32_t level()           const { return s_->decisionLevel(); }
	virtual bool     hasLit(Lit_t lit) const { return s_->validVar(decodeVar(lit)); }
	virtual Value_t  value(Lit_t lit)  const {
		CLASP_ASSERT_CONTRACT_MSG(Control::hasLit(lit), "Invalid literal");
		switch (s_->value(decodeVar(lit))) {
			default: return Value_t::Free;
			case value_true:  return lit >= 0 ? Value_t::True  : Value_t::False;
			case value_false: return lit >= 0 ? Value_t::False : Value_t::True;
		}
	}
	virtual uint32_t level(Lit_t lit)  const { return Control::value(lit) != Potassco::Value_t::Free ? s_->level(decodeVar(lit)) : UINT32_MAX; }
	virtual Lit_t    decision(uint32_t dl) const {
		CLASP_ASSERT_CONTRACT_MSG(dl <= s_->decisionLevel(), "Invalid decision level");
		return encodeLit(dl ? s_->decision(dl) : lit_true());
	}
	
	// Potassco::AbstractSolver
	virtual Potassco::Id_t id() const { return s_->id(); }
	virtual bool addClause(const Potassco::LitSpan& clause, Potassco::Clause_t prop);
	virtual bool propagate();
protected:
	typedef ClingoPropagator::State State;
	ClingoPropagator* ctx_;
	Solver*           s_;
	uint32            state_;
};
bool ClingoPropagator::Control::addClause(const Potassco::LitSpan& clause, Potassco::Clause_t prop) {
	CLASP_ASSERT_CONTRACT_MSG(!s_->hasConflict(), "Invalid addClause() on conflicting assignment");
	ScopedUnlock pp(ctx_->lock_, ctx_);
	pp->toClause(*s_, clause, prop);
	return pp->addClause(*s_, state_);
}
bool ClingoPropagator::Control::propagate() {
	ScopedUnlock unlocked(ctx_->lock_, ctx_);
	if (s_->hasConflict())    { return false; }
	if (s_->queueSize() == 0) { return true;  }
	uint32 epoch = ctx_->epoch_;
	return (state_ & state_prop) != 0u && s_->propagateUntil(unlocked.obj_) && epoch == ctx_->epoch_;
}
/////////////////////////////////////////////////////////////////////////////////////////
// ClingoPropagator
/////////////////////////////////////////////////////////////////////////////////////////
// flags for clauses from propagator
static const uint32 ccFlags_s[2] = {
	/* 0: learnt */ Clasp::ClauseCreator::clause_not_sat | Clasp::ClauseCreator::clause_int_lbd,
	/* 1: static */ ClauseCreator::clause_no_add | ClauseCreator::clause_explicit
};		
ClingoPropagator::ClingoPropagator(const LitVec& watches, Potassco::AbstractPropagator& cb, ClingoPropagatorLock* ctrl)
	: watches_(watches)
	, call_(&cb)
	, lock_(ctrl)
	, init_(0), level_(0), prop_(0), epoch_(0) {
}
uint32 ClingoPropagator::priority() const { return static_cast<uint32>(priority_class_general); }

void ClingoPropagator::destroy(Solver* s, bool detach) {
	destroyDB(db_, s, detach);
	PostPropagator::destroy(s, detach);
}

bool ClingoPropagator::init(Solver& s) {
	CLASP_ASSERT_CONTRACT_MSG(init_ <= watches_.size(), "Invalid watch list update");
	uint32 ignore = 0;
	for (size_t max = watches_.size(); init_ != max; ++init_) {
		Literal p = watches_[init_];
		if (s.value(p.var()) == value_free || s.level(p.var()) > s.rootLevel()) {
			s.addWatch(p, this, init_);
		}
		else if (s.isTrue(p)) {
			ClingoPropagator::propagate(s, p, ignore);
		}
	}
	return true;
}

Constraint::PropResult ClingoPropagator::propagate(Solver& s, Literal p, uint32&) {
	uint32 dl = s.decisionLevel();
	CLASP_ASSERT_CONTRACT_MSG(dl >= level_, "Stack property violated");
	if (dl != level_) { // remember start of new decision level
		s.addUndoWatch(level_ = dl, this);
		undo_.push_back(static_cast<uint32>(trail_.size()));
	}
	trail_.push_back(encodeLit(p));
	return PropResult(true, true);
}
void ClingoPropagator::undoLevel(Solver& s) {
	CLASP_ASSERT_CONTRACT_MSG(s.decisionLevel() == level_, "Invalid undo");
	uint32 beg = undo_.back();
	undo_.pop_back();
	if (prop_ > beg) {
		Potassco::LitSpan change = Potassco::toSpan(&trail_[0] + beg, prop_ - beg);
		ScopedLock(lock_, call_, Inc(epoch_))->undo(Control(*this, s), change);
		prop_ = beg;
	}
	trail_.resize(beg);
	level_ = !trail_.empty() ? s.level(decodeLit(trail_.back()).var()) : 0;
}
bool ClingoPropagator::propagateFixpoint(Clasp::Solver& s, Clasp::PostPropagator*) {
	CLASP_ASSERT_CONTRACT_MSG(prop_ <= trail_.size(), "Invalid propagate");
	for (Control ctrl(*this, s, state_prop); prop_ != trail_.size();) {
		Potassco::LitSpan change = Potassco::toSpan(&trail_[0] + prop_, trail_.size() - prop_);
		prop_ = static_cast<uint32>(trail_.size());
		ScopedLock(lock_, call_, Inc(epoch_))->propagate(ctrl, change);
		if (!addClause(s, state_prop) || (s.queueSize() && !s.propagateUntil(this))) {
			return false;
		}
	}
	return true;
}

void ClingoPropagator::toClause(Solver& s, const Potassco::LitSpan& clause, Potassco::Clause_t prop) {
	CLASP_ASSERT_CONTRACT_MSG(clause_.empty(), "Assignment not propagated");
	for (const Potassco::Lit_t* it = Potassco::begin(clause); it != Potassco::end(clause); ++it) {
		clause_.push_back(decodeLit(*it));
	}
	if (Potassco::Clause_t::isVolatile(prop) && !isSentinel(s.sharedContext()->stepLiteral())) {
		clause_.push_back(~s.sharedContext()->stepLiteral());
	}
	ClauseCreator::prepare(s, clause_, Clasp::ClauseCreator::clause_force_simplify, Constraint_t::Other);
	if (clause_.empty()) { clause_.push_back(lit_false()); }
	clause_.push_back(Literal::fromRep(static_cast<uint32>(prop))); // remember properties
}
bool ClingoPropagator::addClause(Solver& s, uint32 st) {
	if (s.hasConflict()) { clause_.clear(); return false; }
	if (clause_.empty()) { return true; }
	assert(clause_.size() > 1);
	uint32 impLevel = clause_.size() > 2 ? ClauseCreator::watchOrder(s, clause_[1]) : 0;
	if (impLevel < s.decisionLevel() && s.isUndoLevel()) {
		if ((st & state_ctrl) != 0u) { return false; }
		if ((st & state_prop) != 0u) { ClingoPropagator::reset(); cancelPropagation(); }
		s.undoUntil(impLevel);
	}
	bool isStatic = Potassco::Clause_t::isStatic(static_cast<Potassco::Clause_t>(clause_.back().rep()));
	clause_.pop_back();
	if (!s.isFalse(clause_[0]) || isStatic || s.force(clause_[0], this)) {
		ClauseCreator::Result res = ClauseCreator::create(s, ClauseRep::prepared(&clause_[0], clause_.size(), Constraint_t::Other), ccFlags_s[int(isStatic)]);
		if (res.local && isStatic) { db_.push_back(res.local); }
	}
	clause_.clear();
	return !s.hasConflict();
}

void ClingoPropagator::reason(Solver&, Literal p, LitVec& r) {
	for (LitVec::const_iterator it = clause_.begin() + (p == clause_[0]), end = clause_.end(); it != end; ++it) {
		r.push_back(~*it);
	}
}

bool ClingoPropagator::simplify(Solver& s, bool) {
	simplifyDB(s, db_, false);
	return false;
}

bool ClingoPropagator::isModel(Solver& s) {
	CLASP_ASSERT_CONTRACT_MSG(prop_ == trail_.size(), "Assignment not propagated");
	Control ctrl(*this, s);
	ScopedLock(lock_, call_, Inc(epoch_))->check(ctrl);
	return addClause(s, 0u) && s.numFreeVars() == 0 && s.queueSize() == 0;
}
/////////////////////////////////////////////////////////////////////////////////////////
// ClingoPropagatorInit
/////////////////////////////////////////////////////////////////////////////////////////
ClingoPropagatorInit::ClingoPropagatorInit(Potassco::AbstractPropagator& cb, ClingoPropagatorLock* lock) : prop_(&cb), lock_(lock) {}
ClingoPropagatorInit::~ClingoPropagatorInit()      {}
void ClingoPropagatorInit::prepare(SharedContext&) {}
bool ClingoPropagatorInit::addPost(Solver& s)      { return s.addPost(new ClingoPropagator(watches_, *prop_, lock_)); }
Potassco::Lit_t ClingoPropagatorInit::addWatch(Literal lit) {
	uint32 const word = lit.id() / 32;
	uint32 const bit  = lit.id() & 31;
	if (word >= seen_.size()) { seen_.resize(word + 1, 0); }
	if (!test_bit(seen_[word], bit)) {
		watches_.push_back(lit);
		store_set_bit(seen_[word], bit);
	}
	return encodeLit(lit);
}
}
