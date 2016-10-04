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
	struct Inc { Inc(LitVec::size_type& e) : epoch_(&e) {} void operator()() const { ++*epoch_; } LitVec::size_type* epoch_; };
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
	virtual Lit  pushVariable();
	virtual bool hasWatch(Lit lit) const;
	virtual void addWatch(Lit lit);
	virtual void removeWatch(Lit lit);
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
	ClingoPropagator::size_t epoch = ctx_->epoch_;
	return (state_ & state_prop) != 0u && s_->propagateUntil(unlocked.obj_) && epoch == ctx_->epoch_;
}
Potassco::Lit_t ClingoPropagator::Control::pushVariable() {
	CLASP_ASSERT_CONTRACT_MSG(!s_->hasConflict(), "Invalid pushVariable() on conflicting assignment");
	ScopedUnlock unlocked(ctx_->lock_, ctx_);
	return encodeLit(posLit(s_->pushAuxVar()));
}
bool ClingoPropagator::Control::hasWatch(Lit_t lit) const {
	ScopedUnlock unlocked(ctx_->lock_, ctx_);
	return Control::hasLit(lit) && s_->hasWatch(decodeLit(lit), ctx_);
}
void ClingoPropagator::Control::addWatch(Lit_t lit) {
	ScopedUnlock unlocked(ctx_->lock_, ctx_);
	CLASP_ASSERT_CONTRACT_MSG(Control::hasLit(lit), "Invalid literal");
	Literal p = decodeLit(lit);
	if (!s_->hasWatch(p, ctx_)) {
		s_->addWatch(p, ctx_);
	}
}
void ClingoPropagator::Control::removeWatch(Lit_t lit) {
	ScopedUnlock unlocked(ctx_->lock_, ctx_);
	if (Control::hasLit(lit)) {
		s_->removeWatch(decodeLit(lit), ctx_);
	}
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
	, init_(0), prop_(0), epoch_(0), level_(0) {
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
			s.addWatch(p, this, toU32(init_));
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
	CLASP_ASSERT_CONTRACT_MSG(todo_.empty(), "Assignment not propagated");
	Literal max;
	LitVec& mem = todo_.mem;
	for (const Potassco::Lit_t* it = Potassco::begin(clause); it != Potassco::end(clause); ++it) {
		Literal p = decodeLit(*it);
		if (max < p) { max = p; }
		mem.push_back(p);
	}
	if (aux_ < max) { aux_ = max; }
	if ((Potassco::Clause_t::isVolatile(prop) || s.auxVar(max.var())) && !isSentinel(s.sharedContext()->stepLiteral())) {
		mem.push_back(~s.sharedContext()->stepLiteral());
	}
	todo_.clause = ClauseCreator::prepare(s, mem, Clasp::ClauseCreator::clause_force_simplify, Constraint_t::Other);
	todo_.flags  = ccFlags_s[int(Potassco::Clause_t::isStatic(prop))];
}
bool ClingoPropagator::addClause(Solver& s, uint32 st) {
	if (s.hasConflict()) { todo_.clear(); return false; }
	if (todo_.empty())   { return true; }
	const ClauseRep& clause = todo_.clause;
	Literal first   = clause.size > 0 ? clause.lits[0] : lit_false();
	uint32 impLevel = clause.size > 1 ? ClauseCreator::watchOrder(s, clause.lits[1]) : 0;
	if (impLevel < s.decisionLevel() && s.isUndoLevel()) {
		if ((st & state_ctrl) != 0u) { return false; }
		if ((st & state_prop) != 0u) { ClingoPropagator::reset(); cancelPropagation(); }
		s.undoUntil(impLevel);
	}
	bool local = (todo_.flags & ClauseCreator::clause_no_add) != 0;
	if (!s.isFalse(first) || local || s.force(first, this)) {
		ClauseCreator::Result res = ClauseCreator::create(s, clause, todo_.flags);
		if (res.local && local) { db_.push_back(res.local); }
	}
	todo_.clear();
	return !s.hasConflict();
}

void ClingoPropagator::reason(Solver&, Literal p, LitVec& r) {
	if (!todo_.empty() && todo_.mem[0] == p) {
		for (LitVec::const_iterator it = todo_.mem.begin() + 1, end = todo_.mem.end(); it != end; ++it) {
			r.push_back(~*it);
		}
	}
}

bool ClingoPropagator::simplify(Solver& s, bool) {
	if (!s.validVar(aux_.var())) {
		ClauseDB::size_type i, j, end = db_.size();
		LitVec cc;
		Var last = s.numVars();
		aux_ = lit_true();
		for (i = j = 0; i != end; ++i) {
			db_[j++] = db_[i];
			ClauseHead* c = db_[i]->clause();
			if (c && c->aux()) {
				cc.clear();
				c->toLits(cc);
				Literal x = *std::max_element(cc.begin(), cc.end());
				if (x.var() > last) {
					c->destroy(&s, true);
					--j;
				}
				else if (aux_ < x) { aux_ = x; }
			}
		}
		db_.erase(db_.begin()+j, db_.end());
	}
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
