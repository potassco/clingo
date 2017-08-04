//
// Copyright (c) 2015-2017 Benjamin Kaufmann
//
// This file is part of Clasp. See http://www.cs.uni-potsdam.de/clasp/
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//
#include <clasp/clingo.h>
#include <clasp/solver.h>
#include <clasp/clause.h>
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
	virtual uint32_t size()            const { return s_->numVars(); }
	virtual uint32_t unassigned()      const { return s_->numFreeVars(); }
	virtual bool     isTotal()         const { return s_->numFreeVars() == 0u; }
	virtual bool     hasConflict()     const { return s_->hasConflict(); }
	virtual uint32_t level()           const { return s_->decisionLevel(); }
	virtual bool     hasLit(Lit_t lit) const { return s_->validVar(decodeVar(lit)); }
	virtual Value_t  value(Lit_t lit)  const {
		POTASSCO_REQUIRE(Control::hasLit(lit), "Invalid literal");
		switch (s_->value(decodeVar(lit))) {
			default: return Value_t::Free;
			case value_true:  return lit >= 0 ? Value_t::True  : Value_t::False;
			case value_false: return lit >= 0 ? Value_t::False : Value_t::True;
		}
	}
	virtual uint32_t level(Lit_t lit)  const { return Control::value(lit) != Potassco::Value_t::Free ? s_->level(decodeVar(lit)) : UINT32_MAX; }
	virtual Lit_t    decision(uint32_t dl) const {
		POTASSCO_REQUIRE(dl <= s_->decisionLevel(), "Invalid decision level");
		return encodeLit(dl ? s_->decision(dl) : lit_true());
	}

	// Potassco::AbstractSolver
	virtual Potassco::Id_t id() const { return s_->id(); }
	virtual bool addClause(const Potassco::LitSpan& clause, Potassco::Clause_t prop);
	virtual bool propagate();
	virtual Lit  addVariable();
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
	POTASSCO_REQUIRE(!s_->hasConflict(), "Invalid addClause() on conflicting assignment");
	ScopedUnlock pp(ctx_->call_->lock(), ctx_);
	pp->toClause(*s_, clause, prop);
	return pp->addClause(*s_, state_);
}
bool ClingoPropagator::Control::propagate() {
	ScopedUnlock unlocked(ctx_->call_->lock(), ctx_);
	if (s_->hasConflict())    { return false; }
	if (s_->queueSize() == 0) { return true;  }
	ClingoPropagator::size_t epoch = ctx_->epoch_;
	return (state_ & state_prop) != 0u && s_->propagateUntil(unlocked.obj_) && epoch == ctx_->epoch_;
}
Potassco::Lit_t ClingoPropagator::Control::addVariable() {
	POTASSCO_REQUIRE(!s_->hasConflict(), "Invalid addVariable() on conflicting assignment");
	ScopedUnlock unlocked(ctx_->call_->lock(), ctx_);
	return encodeLit(posLit(s_->pushAuxVar()));
}
bool ClingoPropagator::Control::hasWatch(Lit_t lit) const {
	ScopedUnlock unlocked(ctx_->call_->lock(), ctx_);
	return Control::hasLit(lit) && s_->hasWatch(decodeLit(lit), ctx_);
}
void ClingoPropagator::Control::addWatch(Lit_t lit) {
	ScopedUnlock unlocked(ctx_->call_->lock(), ctx_);
	POTASSCO_REQUIRE(Control::hasLit(lit), "Invalid literal");
	Literal p = decodeLit(lit);
	if (!s_->hasWatch(p, ctx_)) {
		s_->addWatch(p, ctx_);
	}
}
void ClingoPropagator::Control::removeWatch(Lit_t lit) {
	ScopedUnlock unlocked(ctx_->call_->lock(), ctx_);
	if (Control::hasLit(lit)) {
		s_->removeWatch(decodeLit(lit), ctx_);
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// ClingoPropagator
/////////////////////////////////////////////////////////////////////////////////////////
// flags for clauses from propagator
static const uint32 ccFlags_s[2] = {
	/* 0: learnt */ ClauseCreator::clause_not_sat | Clasp::ClauseCreator::clause_int_lbd,
	/* 1: static */ ClauseCreator::clause_no_add  | ClauseCreator::clause_explicit
};
ClingoPropagator::ClingoPropagator(Propagator* p)
	: call_(p)
	, init_(0), prop_(0), epoch_(0), level_(0) {
}
uint32 ClingoPropagator::priority() const { return static_cast<uint32>(priority_class_general); }

void ClingoPropagator::destroy(Solver* s, bool detach) {
	destroyDB(db_, s, detach);
	PostPropagator::destroy(s, detach);
}

bool ClingoPropagator::init(Solver& s) {
	POTASSCO_REQUIRE(init_ <= call_->watches().size(), "Invalid watch list update");
	uint32 ignore = 0;
	const LitVec& watches = call_->watches();
	for (size_t max = watches.size(); init_ != max; ++init_) {
		Literal p = watches[init_];
		if (s.value(p.var()) == value_free || s.level(p.var()) > s.rootLevel()) {
			s.addWatch(p, this, toU32(init_));
		}
		else if (s.isTrue(p)) {
			ClingoPropagator::propagate(s, p, ignore);
		}
	}
	front_ = call_->checkMode() == ClingoPropagatorCheck_t::Fixpoint ? -1 : INT32_MAX;
	return true;
}

void ClingoPropagator::registerUndo(Solver& s) {
	uint32 dl = s.decisionLevel();
	if (dl != level_) {
		POTASSCO_REQUIRE(dl > level_, "Stack property violated");
		POTASSCO_REQUIRE(front_ == INT32_MAX || (dl - level_) == 1, "Propagate must be called on each level");
		// first time we see this level
		s.addUndoWatch(level_ = dl, this);
		undo_.push_back(static_cast<uint32>(trail_.size()));
	}
}
Constraint::PropResult ClingoPropagator::propagate(Solver& s, Literal p, uint32&) {
	registerUndo(s);
	trail_.push_back(encodeLit(p));
	return PropResult(true, true);
}
void ClingoPropagator::undoLevel(Solver& s) {
	POTASSCO_REQUIRE(s.decisionLevel() == level_, "Invalid undo");
	uint32 beg = undo_.back();
	undo_.pop_back();
	if (prop_ > beg) {
		Potassco::LitSpan change = Potassco::toSpan(&trail_[0] + beg, prop_ - beg);
		ScopedLock(call_->lock(), call_->propagator(), Inc(epoch_))->undo(Control(*this, s), change);
		prop_ = beg;
	}
	trail_.resize(beg);
	if (front_ != INT32_MAX) {
		front_ = -1;
		--level_;
	}
	else {
		level_ = !trail_.empty() ? s.level(decodeLit(trail_.back()).var()) : 0;
	}
}
bool ClingoPropagator::propagateFixpoint(Clasp::Solver& s, Clasp::PostPropagator*) {
	POTASSCO_REQUIRE(prop_ <= trail_.size(), "Invalid propagate");
	for (Control ctrl(*this, s, state_prop); prop_ != trail_.size() || front_ < (int32)s.numAssignedVars();) {
		if (prop_ != trail_.size()) {
			// create copy because trail might change during call to user propagation
			temp_.assign(trail_.begin() + prop_, trail_.end());
			prop_ = static_cast<uint32>(trail_.size());
			ScopedLock(call_->lock(), call_->propagator(), Inc(epoch_))->propagate(ctrl, Potassco::toSpan(temp_));
		}
		else {
			registerUndo(s);
			front_ = (int32)s.numAssignedVars();
			ScopedLock(call_->lock(), call_->propagator(), Inc(epoch_))->check(ctrl);
		}
		if (!addClause(s, state_prop) || (s.queueSize() && !s.propagateUntil(this))) {
			return false;
		}
	}
	return true;
}

void ClingoPropagator::toClause(Solver& s, const Potassco::LitSpan& clause, Potassco::Clause_t prop) {
	POTASSCO_REQUIRE(todo_.empty(), "Assignment not propagated");
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
	if (mem.empty()) {
		mem.push_back(lit_false());
	}
}
bool ClingoPropagator::addClause(Solver& s, uint32 st) {
	if (s.hasConflict()) { todo_.clear(); return false; }
	if (todo_.empty())   { return true; }
	const ClauseRep& clause = todo_.clause;
	Literal w0 = clause.size > 0 ? clause.lits[0] : lit_false();
	Literal w1 = clause.size > 1 ? clause.lits[1] : lit_false();
	uint32  cs = (ClauseCreator::status(s, clause) & (ClauseCreator::status_unit|ClauseCreator::status_unsat));
	if (cs && s.level(w1.var()) < s.decisionLevel() && s.isUndoLevel()) {
		if ((st & state_ctrl) != 0u) { return false; }
		if ((st & state_prop) != 0u) { ClingoPropagator::reset(); cancelPropagation(); }
		s.undoUntil(s.level(w1.var()));
	}
	bool local = (todo_.flags & ClauseCreator::clause_no_add) != 0;
	if (!s.isFalse(w0) || local || s.force(w0, this)) {
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
	POTASSCO_REQUIRE(prop_ == trail_.size(), "Assignment not propagated");
	if (call_->checkMode() == ClingoPropagatorCheck_t::Total) {
		Control ctrl(*this, s);
		ScopedLock(call_->lock(), call_->propagator(), Inc(epoch_))->check(ctrl);
		return addClause(s, 0u) && s.numFreeVars() == 0 && s.queueSize() == 0;
	}
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////
// ClingoPropagatorInit
/////////////////////////////////////////////////////////////////////////////////////////
ClingoPropagatorInit::ClingoPropagatorInit(Potassco::AbstractPropagator& cb, ClingoPropagatorLock* lock, CheckType m) : prop_(&cb), lock_(lock), check_(m) {}
ClingoPropagatorInit::~ClingoPropagatorInit()      {}
void ClingoPropagatorInit::prepare(SharedContext&) {}
bool ClingoPropagatorInit::addPost(Solver& s)      { return s.addPost(new ClingoPropagator(this)); }
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
void ClingoPropagatorInit::enableClingoPropagatorCheck(CheckType checkMode) {
	check_ = checkMode;
}
}
