// 
// Copyright (c) 2006-2012, Benjamin Kaufmann
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
#include <clasp/solver.h>
#include <clasp/clause.h>
#if (defined(__cplusplus) && __cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER > 1500) || (defined(_LIBCPP_VERSION))
#include <unordered_set>
typedef std::unordered_set<Clasp::Constraint*> ConstraintSet;
#else
#if defined(_MSC_VER)
#include <unordered_set>
#else
#include <tr1/unordered_set>
#endif
typedef std::tr1::unordered_set<Clasp::Constraint*> ConstraintSet;
#endif
#ifdef _MSC_VER
#pragma warning (disable : 4996) // 'std::copy': Function call with parameters that may be unsafe
#endif
namespace Clasp { 

DecisionHeuristic::~DecisionHeuristic() {}
/////////////////////////////////////////////////////////////////////////////////////////
// SelectFirst selection strategy
/////////////////////////////////////////////////////////////////////////////////////////
// selects the first free literal
Literal SelectFirst::doSelect(Solver& s) {
	for (Var i = 1; i <= s.numVars(); ++i) {
		if (s.value(i) == value_free) {
			return selectLiteral(s, i, 0);
		}
	}
	assert(!"SelectFirst::doSelect() - precondition violated!\n");
	return Literal();
}
/////////////////////////////////////////////////////////////////////////////////////////
// Post propagator list
/////////////////////////////////////////////////////////////////////////////////////////
static PostPropagator* sent_list;
Solver::PPList::PPList() : list(0) { enable(); }
Solver::PPList::~PPList() {
	for (PostPropagator* r = head(); r;) {
		PostPropagator* t = r;
		r = r->next;
		t->destroy();
	}
}

void Solver::PPList::disable() { act = &sent_list; }
void Solver::PPList::enable()  { act = &list; }

void Solver::PPList::add(PostPropagator* p, uint32 prio) {
	assert(p && p->next == 0);
	for (PostPropagator** r = &list, *x;; r = &x->next) {
		if ((x = *r) == 0 || prio <= x->priority()) {
			p->next = x;
			*r      = p;
			break;
		}
	}
}

void Solver::PPList::remove(PostPropagator* p) {
	assert(p);
	for (PostPropagator** r = &list, *x; *r; r = &x->next) {
		if ((x = *r) == p) {
			*r      = x->next;
			p->next = 0;
			break;
		}
	}
}

bool Solver::PPList::propagate(Solver& s, PostPropagator* x) const {
	for (PostPropagator** r = act, *t; *r != x; ) {
		t = *r;
		if (!t->propagateFixpoint(s, x)) { return false; }
		assert(s.queueSize() == 0);
		if (t == *r) { r = &t->next; }
		// else: t was removed during propagate
	}
	return true;
}

void Solver::PPList::simplify(Solver& s, bool shuf) {
	for (PostPropagator* r = active(); r;) {
		PostPropagator* t = r;
		r = r->next;
		if (t->simplify(s, shuf)) {
			remove(t);
		}
	}
}

void Solver::PPList::cancel()           const { for (PostPropagator* r = active(); r; r = r->next) { r->reset(); } }
bool Solver::PPList::isModel(Solver& s) const {
	if (s.hasConflict()) { return false; }
	for (PostPropagator* r = active(); r; r = r->next) {
		if (!r->isModel(s)){ return false; }
	}
	return !s.enumerationConstraint() || s.enumerationConstraint()->valid(s);
}
/////////////////////////////////////////////////////////////////////////////////////////
// Solver: Construction/Destruction/Setup
////////////////////////////////////////////////////////////////////////////////////////
Solver::Solver(SharedContext* ctx, uint32 id) 
	: shared_(ctx)
	, ccMin_(0)
	, smallAlloc_(new SmallClauseAlloc)
	, undoHead_(0)
	, enum_(0)
	, memUse_(0)
	, ccInfo_(Constraint_t::learnt_conflict)
	, lbdTime_(0)
	, dbIdx_(0)
	, lastSimp_(0)
	, shufSimp_(0)
	, initPost_(0){
	Var sentVar = assign_.addVar();
	assign_.setValue(sentVar, value_true);
	markSeen(sentVar);
	strategy_.id = id;
}

Solver::~Solver() {
	freeMem();
}

void Solver::freeMem() {
	std::for_each( constraints_.begin(), constraints_.end(), DestroyObject());
	std::for_each( learnts_.begin(), learnts_.end(), DestroyObject() );
	constraints_.clear();
	learnts_.clear();
	setEnumerationConstraint(0);
	heuristic_.reset(0);
	PodVector<WatchList>::destruct(watches_);
	// free undo lists
	// first those still in use
	for (DecisionLevels::size_type i = 0; i != levels_.size(); ++i) {
		delete levels_[i].undo;
	}
	// then those in the free list
	for (ConstraintDB* x = undoHead_; x; ) {
		ConstraintDB* t = x;
		x = (ConstraintDB*)x->front();
		delete t;
	}
	delete smallAlloc_;
	delete ccMin_;
	smallAlloc_ = 0;
	ccMin_      = 0;
	memUse_     = 0;
}
namespace {
	struct InSet {
		bool operator()(Constraint* c)        const { return set->find(c) != set->end(); }
		bool operator()(const ClauseWatch& w) const { return (*this)(w.head);  }
		bool operator()(const GenericWatch&w) const { return (*this)(w.con);  }
		const ConstraintSet* set;
	};
}
void Solver::destroyDB(ConstraintDB& db) {
	bool lazy = db.size() > 100;
	for (ConstraintDB::const_iterator it = db.begin(), end = db.end(); it != end; ++it) {
		(*it)->destroy(this, !lazy);
	}
	if (lazy) {
		ConstraintSet set(db.begin(), db.end());
		InSet inSet = { &set };
		for (Watches::iterator it = watches_.begin(), end = watches_.end(); it != end; ++it) {
			if (it->left_size()) { it->shrink_left(std::remove_if(it->left_begin(), it->left_end(), inSet)); }
			if (it->right_size()){ it->shrink_right(std::remove_if(it->right_begin(), it->right_end(), inSet)); }
		}
		for (uint32 i = 0, end = (uint32)levels_.size(); i != end; ++i) {
			if (ConstraintDB* db = levels_[i].undo) {
				db->erase(std::remove_if(db->begin(), db->end(), inSet), db->end());
			}
		}
	}
	db.clear();
}
void destroyDB(Solver::ConstraintDB& db, Solver* s, bool detach) {
	if (s && detach) { 
		s->destroyDB(db); 
		return;
	}
	while (!db.empty()) {
		db.back()->destroy(s, detach); 
		db.pop_back();
	}
}
SatPreprocessor*    Solver::satPrepro()     const { return shared_->satPrepro.get(); }
const SolveParams&  Solver::searchConfig()  const { return shared_->configuration()->search(id()); }

void Solver::reset() {
	SharedContext* myCtx = shared_;
	uint32         myId  = strategy_.id;
	this->~Solver();
	new (this) Solver(myCtx, myId);
}
DecisionHeuristic* Solver::releaseHeuristic(bool detach) {
	if (detach && heuristic_.is_owner() && heuristic_.get()) { heuristic_->detach(*this); }
	return heuristic_.release();
}
void Solver::setHeuristic(DecisionHeuristic* h) {
	if (heuristic_.is_owner() && heuristic_.get()) { heuristic_->detach(*this); }
	heuristic_.reset(h);
}
void Solver::resetConfig() {
	if (strategy_.hasConfig) {
		if (PostPropagator* pp = getPost(PostPropagator::priority_reserved_look)) { pp->destroy(this, true); }
		delete ccMin_; 
		ccMin_ = 0;
	}		
	strategy_.hasConfig = 0;
}
void Solver::startInit(uint32 numConsGuess, const SolverParams& params) {
	assert(numVars() <= shared_->numVars());
	assign_.resize(shared_->numVars() + 1);
	watches_.resize(assign_.numVars()<<1);
	// pre-allocate some memory
	assign_.trail.reserve(numVars());
	constraints_.reserve(numConsGuess/2);
	levels_.reserve(25);
	if (smallAlloc_ == 0){ smallAlloc_ = new SmallClauseAlloc();  }
	if (undoHead_ == 0)  {
		for (uint32 i = 0; i != 25; ++i) { 
			undoFree(new ConstraintDB(10)); 
		}
	}
	if (!popRootLevel(rootLevel())) { return; }
	if (!strategy_.hasConfig) {
		uint32 id             = this->id();
		uint32 hId            = strategy_.heuReserved;
		strategy_             = params;
		strategy_.id          = id; // keep id
		strategy_.heuReserved = hId;// and hId
		strategy_.hasConfig   = 1;  // strategy is now "up to date"
		if      (!params.ccMinRec)  { delete ccMin_; ccMin_ = 0; }
		else if (!ccMin_)           { ccMin_ = new CCMinRecursive; }
		if (id == params.id || !shared_->seedSolvers()) {
			rng.srand(params.seed);
		}
		else {
			RNG x(14182940); for (uint32 i = 0; i != id; ++i) { x.rand(); }
			rng.srand(x.seed());
		}
		if (hId != params.heuId || params.forgetHeuristic()) { // heuristic has changed
			setHeuristic(0);
		}
	}
	if (heuristic_.get() == 0) {
		setHeuristic(shared_->configuration()->heuristic(id()));
		strategy_.heuReserved = params.heuId;
	}
	post_.disable(); // disable post propagators during setup
	initPost_ = 0;   // defer calls to PostPropagator::init()
	heuristic_->startInit(*this);
}

bool Solver::cloneDB(const ConstraintDB& db) {
	assert(!hasConflict());
	while (dbIdx_ < (uint32)db.size() && !hasConflict()) {
		if (Constraint* c = db[dbIdx_++]->cloneAttach(*this)) {
			constraints_.push_back(c);
		}
	}
	return !hasConflict();
}
bool Solver::preparePost() {
	if (hasConflict()) { return false; }
	if (initPost_ == 0){
		initPost_ = 1;
		for (PostPropagator* x = post_.head(), *t; (t = x) != 0; ) {
			x = x->next;
			if (!t->init(*this)) { return false; }
		}
	}
	return shared_->configuration()->addPost(*this);
}
PostPropagator* Solver::getPost(uint32 prio) const {
	for (PostPropagator* x = post_.head(); x; x = x->next) {
		uint32 xp = x->priority();
		if (xp >= prio) { return xp == prio ? x : 0; }
	}
	return 0;
}
bool Solver::endInit() {
	if (hasConflict()) { return false; }
	heuristic_->endInit(*this);
	if (strategy_.signFix) {
		for (Var v = 1; v <= numVars(); ++v) {
			Literal x = DecisionHeuristic::selectLiteral(*this, v, 0);
			setPref(v, ValueSet::user_value, x.sign() ? value_false : value_true);
		}
	}
	post_.enable(); // enable all post propagators
	return propagate() && simplify();
}

bool Solver::endStep(uint32 top) {
	if (!popRootLevel(rootLevel())) { return false; }
	popAuxVar();
	uint32 tp = std::min(top, (uint32)lastSimp_);
	Literal x = shared_->stepLiteral();
	Solver* m = this != shared_->master() ? shared_->master() : 0;
	if (value(x.var()) == value_free){ force(~x, posLit(0));}
	post_.disable();
	if (simplify()) {
		while (tp < (uint32)assign_.trail.size()) {
			Var v = assign_.trail[tp].var();
			if      (v == x.var()){ std::swap(assign_.trail[tp], assign_.trail.back()); assign_.undoLast(); }
			else if (m)           { m->force(assign_.trail[tp++]); }
			else                  { ++tp; }
		}
		if (x.var() && value(x.var()) != value_free) {
			LitVec::iterator it = std::find(assign_.trail.begin(), assign_.trail.end(), trueLit(x.var()));
			if (it != assign_.trail.end()) {
				std::swap(*it, assign_.trail.back());
				assign_.undoLast();
			}
		}
		assign_.qReset();
		assign_.setUnits(lastSimp_ = (uint32)assign_.trail.size()); 
	}
	return true;
}

void Solver::add(Constraint* c) {
	constraints_.push_back(c);
}
bool Solver::add(const ClauseRep& c, bool isNew) {
	typedef ShortImplicationsGraph::ImpType ImpType;
	if (c.prep == 0) {
		return ClauseCreator::create(*this, c, ClauseCreator::clause_force_simplify).ok();
	}
	int added = 0;
	if (c.size > 1) {
		if (allowImplicit(c)) { added = shared_->addImp(static_cast<ImpType>(c.size), c.lits, c.info.type()); }
		else                  { return ClauseCreator::create(*this, c, ClauseCreator::clause_explicit).ok(); }
	}
	else {
		Literal u = c.size ? c.lits[0] : negLit(0);
		uint32  ts= trail().size();
		force(u);
		added     = int(ts != trail().size());
	}
	if (added > 0 && isNew && c.info.learnt()) {
		stats.addLearnt(c.size, c.info.type());
		distribute(c.lits, c.size, c.info);
	}
	return !hasConflict();
}
uint32 Solver::receive(SharedLiterals** out, uint32 maxOut) const {
	if (shared_->distributor.get()) {
		return shared_->distributor->receive(*this, out, maxOut);
	}
	return 0;
}
SharedLiterals* Solver::distribute(const Literal* lits, uint32 size, const ClauseInfo& extra) {
	if (shared_->distributor.get() && !extra.aux() && (size <= 3 || shared_->distributor->isCandidate(size, extra.lbd(), extra.type()))) {
		uint32 initialRefs = shared_->concurrency() - (size <= Clause::MAX_SHORT_LEN || !shared_->physicalShare(extra.type()));
		SharedLiterals* x  = SharedLiterals::newShareable(lits, size, extra.type(), initialRefs);
		shared_->distributor->publish(*this, x);
		stats.addDistributed(extra.lbd(), extra.type());
		return initialRefs == shared_->concurrency() ? x : 0;
	}
	return 0;
}
void Solver::setEnumerationConstraint(Constraint* c) {
	if (enum_) enum_->destroy(this, true);
	enum_ = c;
}

uint32 Solver::numConstraints() const {
	return static_cast<uint32>(constraints_.size())
		+ (shared_ ? shared_->numBinary()+shared_->numTernary() : 0);
}

Var Solver::pushAuxVar() {
	Var aux = assign_.addVar();
	setPref(aux, ValueSet::def_value, value_false);
	watches_.insert(watches_.end(), 2, WatchList()); 
	if (heuristic_.get()) { heuristic_->updateVar(*this, aux, 1); }
	return aux;
}
void Solver::popAuxVar(uint32 num) {
	num = numVars() >= shared_->numVars() ? std::min(numVars() - shared_->numVars(), num) : 0;
	if (!num) { return; }
	// 1. find first dl containing one of the aux vars
	Literal aux = posLit(assign_.numVars() - num);
	uint32  dl  = decisionLevel() + 1;
	for (ImpliedList::iterator it = impliedLits_.begin(); it != impliedLits_.end(); ++it) {
		if (!(it->lit < aux)) { dl = std::min(dl, it->level); }
	}
	for (Var v = aux.var(), end = aux.var()+num; v != end; ++v) {
		if (value(v) != value_free){ dl = std::min(dl, level(v)); }
	}
	// 2. remove aux vars from assignment
	if (dl > rootLevel()) {
		if (backtrackLevel() >= dl) { setBacktrackLevel(dl-1); }
		undoUntil(dl-1, undo_pop_bt_level);
	}
	else {
		popRootLevel((rootLevel() - dl) + 1);
		if (dl == 0) { // top-level has aux vars - cleanup manually
			uint32 j = shared_->numUnary(), units = assign_.units();
			for (uint32 i = j, end = assign_.trail.size(); i != end; ++i) {
				if (assign_.trail[i] < aux) { assign_.trail[j++] = assign_.trail[i]; }
				else                        {
					units         -= (i < units);
					assign_.front -= (i < assign_.front);
					lastSimp_     -= (i < lastSimp_);
				}
			}
			shrinkVecTo(assign_.trail, j);
			assign_.setUnits(units);
		}
	}
	// 3. remove constraints over aux
	ConstraintDB::size_type i, j, end = learnts_.size();
	LitVec cc;
	for (i = j = 0; i != end; ++i) {
		cc.clear();
		if (ClauseHead* c = static_cast<LearntConstraint*>(learnts_[i])->clause()) { c->toLits(cc); }
		if (std::find_if(cc.begin(), cc.end(), std::not1(std::bind2nd(std::less<Literal>(), aux))) == cc.end()) {
			learnts_[j++] = learnts_[i];
		}
		else { learnts_[i]->destroy(this, true); }
	}
	learnts_.erase(learnts_.begin()+j, learnts_.end());
	// 4. remove aux vars and their watches
	assign_.resize(assign_.numVars()-num);
	for (uint32 n = num; n--;) { 
		watches_.back().clear(true);
		watches_.pop_back();
		watches_.back().clear(true);
		watches_.pop_back();
	}
	if (!validVar(tag_.var())) { tag_ = posLit(0); }
	heuristic_->updateVar(*this, aux.var(), num);
}

bool Solver::pushRoot(const LitVec& path) {
	// make sure we are on the current root level
	if (!popRootLevel(0) || !simplify() || !propagate()) { return false; }
	// push path
	stats.addPath(path.size());
	for (LitVec::const_iterator it = path.begin(), end = path.end(); it != end; ++it) {
		if (!pushRoot(*it)) { return false; }
	}
	ccInfo_.setActivity(1);
	return true;
}

bool Solver::pushRoot(Literal x) {
	if (hasConflict())                 { return false; }
	if (decisionLevel()!= rootLevel()) { popRootLevel();  }
	if (queueSize() && !propagate())   { return false;    }
	if (value(x.var()) != value_free)  { return isTrue(x);}
	assume(x); --stats.choices;
	pushRootLevel();
	return propagate();
}

bool Solver::popRootLevel(uint32 n, LitVec* popped, bool aux)  {
	clearStopConflict();
	uint32 newRoot = levels_.root - std::min(n, rootLevel());
	if (popped && newRoot < rootLevel()) {
		for (uint32 i = newRoot+1; i <= rootLevel(); ++i) {
			Literal x = decision(i);
			if (aux || !auxVar(x.var())) { popped->push_back(x); }
		}
	}
	levels_.root       = newRoot;
	levels_.backtrack  = rootLevel();
	impliedLits_.front = 0;
	bool tagActive     = isTrue(tagLiteral());
	// go back to new root level and re-assert still implied literals
	undoUntil(rootLevel(), undo_pop_bt_level);
	if (tagActive && !isTrue(tagLiteral())) {
		removeConditional();
	}
	return !hasConflict();
}

bool Solver::clearAssumptions()  {
	return popRootLevel(rootLevel())
		&& simplify();
}

void Solver::clearStopConflict() {
	if (hasStopConflict()) {
		levels_.root      = conflict_[1].asUint();
		levels_.backtrack = conflict_[2].asUint();
		assign_.front     = conflict_[3].asUint();
		conflict_.clear();
	}
}

void Solver::setStopConflict() {
	if (!hasConflict()) {
		// we use the nogood {FALSE} to represent the unrecoverable conflict -
		// note that {FALSE} can otherwise never be a violated nogood because
		// TRUE is always true in every solver
		conflict_.push_back(negLit(0));
		// remember the current root-level
		conflict_.push_back(Literal::fromRep(rootLevel()));
		// remember the current bt-level
		conflict_.push_back(Literal::fromRep(backtrackLevel()));
		// remember the current propagation queue
		conflict_.push_back(Literal::fromRep(assign_.front));
	}
	// artificially increase root level -
	// this way, the solver is prevented from resolving the conflict
	pushRootLevel(decisionLevel());
}

void Solver::copyGuidingPath(LitVec& gpOut) {
	uint32 aux = rootLevel()+1;
	gpOut.clear();
	for (uint32 i = 1, end = rootLevel()+1; i != end; ++i) {
		Literal x = decision(i);
		if      (!auxVar(x.var())) { gpOut.push_back(x); }
		else if (i < aux)          { aux = i; }
	}
	for (ImpliedList::iterator it = impliedLits_.begin(); it != impliedLits_.end(); ++it) {
		if (it->level <= rootLevel() && (it->ante.ante().isNull() || it->level < aux) && !auxVar(it->lit.var())) {
			gpOut.push_back(it->lit);
		}
	}
}
bool Solver::splittable() const {
	if (decisionLevel() == rootLevel() || frozenLevel(rootLevel()+1)) { return false; }
	if (numAuxVars()) { // check if gp would contain solver local aux var
		uint32 minAux = rootLevel() + 2;
		for (uint32 i = 1; i != minAux; ++i) { 
			if (auxVar(decision(i).var()) && decision(i) != tag_) { return false; } 
		}
		for (ImpliedList::iterator it = impliedLits_.begin(); it != impliedLits_.end(); ++it) {
			if (it->ante.ante().isNull() && it->level < minAux && auxVar(it->lit.var()) && it->lit != tag_) { return false; }
		}
	}
	return true;
}
bool Solver::split(LitVec& out) {
	if (!splittable()) { return false; }
	copyGuidingPath(out);
	pushRootLevel();
	out.push_back(~decision(rootLevel()));
	stats.addSplit();
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////
// Solver: Watch management
////////////////////////////////////////////////////////////////////////////////////////
uint32 Solver::numWatches(Literal p) const {
	assert( validVar(p.var()) );
	if (!validWatch(p)) return 0;
	uint32 n = static_cast<uint32>(watches_[p.index()].size());
	if (!auxVar(p.var())){
		n += shared_->shortImplications().numEdges(p);
	}
	return n;
}
	
bool Solver::hasWatch(Literal p, Constraint* c) const {
	if (!validWatch(p)) return false;
	const WatchList& pList = watches_[p.index()];
	return std::find_if(pList.right_begin(), pList.right_end(), GenericWatch::EqConstraint(c)) != pList.right_end();
}

bool Solver::hasWatch(Literal p, ClauseHead* h) const {
	if (!validWatch(p)) return false;
	const WatchList& pList = watches_[p.index()];
	return std::find_if(pList.left_begin(), pList.left_end(), ClauseWatch::EqHead(h)) != pList.left_end();
}

GenericWatch* Solver::getWatch(Literal p, Constraint* c) const {
	if (!validWatch(p)) return 0;
	const WatchList& pList = watches_[p.index()];
	WatchList::const_right_iterator it = std::find_if(pList.right_begin(), pList.right_end(), GenericWatch::EqConstraint(c));
	return it != pList.right_end()
		? &const_cast<GenericWatch&>(*it)
		: 0;
}

void Solver::removeWatch(const Literal& p, Constraint* c) {
	assert(validWatch(p));
	WatchList& pList = watches_[p.index()];
	pList.erase_right(std::find_if(pList.right_begin(), pList.right_end(), GenericWatch::EqConstraint(c)));
}

void Solver::removeWatch(const Literal& p, ClauseHead* h) {
	assert(validWatch(p));
	WatchList& pList = watches_[p.index()];
	pList.erase_left(std::find_if(pList.left_begin(), pList.left_end(), ClauseWatch::EqHead(h)));
}

bool Solver::removeUndoWatch(uint32 dl, Constraint* c) {
	assert(dl != 0 && dl <= decisionLevel() );
	if (levels_[dl-1].undo) {
		ConstraintDB& uList = *levels_[dl-1].undo;
		ConstraintDB::iterator it = std::find(uList.begin(), uList.end(), c);
		if (it != uList.end()) {
			*it = uList.back();
			uList.pop_back();
			return true;
		}
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Solver: Basic DPLL-functions
////////////////////////////////////////////////////////////////////////////////////////

// removes all satisfied binary and ternary clauses as well
// as all constraints for which Constraint::simplify returned true.
bool Solver::simplify() {
	if (decisionLevel() != 0) return true;
	if (hasConflict())        return false;
	if (lastSimp_ != (uint32)assign_.trail.size()) {
		uint32 old = lastSimp_;
		if (!simplifySAT()) { return false; }
		assert(lastSimp_ == (uint32)assign_.trail.size());
		heuristic_->simplify(*this, old);
	}
	if (shufSimp_) { simplifySAT(); }
	return true;
}
Var  Solver::pushTagVar(bool pushToRoot) {
	if (isSentinel(tag_)) { tag_ = posLit(pushAuxVar()); }
	if (pushToRoot)       { pushRoot(tag_); }
	return tag_.var();
}
void Solver::removeConditional() { 
	Literal p = ~tagLiteral();
	if (!isSentinel(p)) {
		ConstraintDB::size_type i, j, end = learnts_.size();
		for (i = j = 0; i != end; ++i) {
			ClauseHead* c = static_cast<LearntConstraint*>(learnts_[i])->clause();
			if (!c || !c->tagged()) {
				learnts_[j++] = learnts_[i];
			}
			else {
				c->destroy(this, true);
			}
		}
		learnts_.erase(learnts_.begin()+j, learnts_.end());
	}
}

void Solver::strengthenConditional() { 
	Literal p = ~tagLiteral();
	if (!isSentinel(p)) {
		ConstraintDB::size_type i, j, end = learnts_.size();
		for (i = j = 0; i != end; ++i) {
			ClauseHead* c = static_cast<LearntConstraint*>(learnts_[i])->clause();
			if (!c || !c->tagged() || !c->strengthen(*this, p, true).second) {
				learnts_[j++] = learnts_[i];
			}
			else {
				assert((decisionLevel() == rootLevel() || !c->locked(*this)) && "Solver::strengthenConditional(): must not remove locked constraint!");
				c->destroy(this, false);
			}
		}
		learnts_.erase(learnts_.begin()+j, learnts_.end());
	}
}

bool Solver::simplifySAT() {
	if (queueSize() > 0 && !propagate()) {
		return false;
	}
	assert(assign_.qEmpty());
	assign_.front = lastSimp_;
	lastSimp_     = (uint32)assign_.trail.size();
	for (Literal p; !assign_.qEmpty(); ) {
		p = assign_.qPop();
		releaseVec(watches_[p.index()]);
		releaseVec(watches_[(~p).index()]);
		shared_->simplifyShort(*this, p);
	}
	bool shuffle = shufSimp_ != 0;
	shufSimp_    = 0;
	if (shuffle) {
		std::random_shuffle(constraints_.begin(), constraints_.end(), rng);
		std::random_shuffle(learnts_.begin(), learnts_.end(), rng);
	}
	if (this == shared_->master()) { shared_->simplify(shuffle); }
	else                           { simplifyDB(*this, constraints_, shuffle); }
	simplifyDB(*this, learnts_, shuffle);
	post_.simplify(*this, shuffle);
	if (enum_ && enum_->simplify(*this, shuffle)) {
		enum_->destroy(this, false);
		enum_ = 0;
	}
	return true;
}

void Solver::setConflict(Literal p, const Antecedent& a, uint32 data) {
	++stats.conflicts;
	conflict_.push_back(~p);
	if (searchMode() != SolverStrategies::no_learning && !a.isNull()) {
		if (data == UINT32_MAX) {
			a.reason(*this, p, conflict_);
		}
		else {
			// temporarily replace old data with new data
			uint32 saved = assign_.data(p.var());
			assign_.setData(p.var(), data);
			// extract conflict using new data
			a.reason(*this, p, conflict_);
			// restore old data
			assign_.setData(p.var(), saved);
		}
	}
}

bool Solver::assume(const Literal& p) {
	if (value(p.var()) == value_free) {
		assert(decisionLevel() != assign_.maxLevel());
		++stats.choices;
		levels_.push_back(DLevel(numAssignedVars(), 0));
		return assign_.assign(p, decisionLevel(), Antecedent());
	}
	return isTrue(p);
}

bool Solver::propagate() {
	if (unitPropagate() && post_.propagate(*this, 0)) {
		assert(queueSize() == 0);
		return true;
	}
	cancelPropagation();
	return false;
}

Constraint::PropResult ClauseHead::propagate(Solver& s, Literal p, uint32&) {
	Literal* head = head_;
	uint32 wLit   = (head[1] == ~p); // pos of false watched literal
	if (s.isTrue(head[1-wLit])) {
		return Constraint::PropResult(true, true);
	}
	else if (!s.isFalse(head[2])) {
		assert(!isSentinel(head[2]) && "Invalid ClauseHead!");
		head[wLit] = head[2];
		head[2]    = ~p;
		s.addWatch(~head[wLit], ClauseWatch(this));
		return Constraint::PropResult(true, false);
	}
	else if (updateWatch(s, wLit)) {
		assert(!s.isFalse(head_[wLit]));
		s.addWatch(~head[wLit], ClauseWatch(this));
		return Constraint::PropResult(true, false);
	}	
	return PropResult(s.force(head_[1^wLit], this), true);
}

bool Solver::unitPropagate() {
	assert(!hasConflict());
	Literal p, q, r;
	uint32 idx, ignore, DL = decisionLevel();
	const ShortImplicationsGraph& btig = shared_->shortImplications();
	const uint32 maxIdx = btig.size();
	while ( !assign_.qEmpty() ) {
		p             = assign_.qPop();
		idx           = p.index();
		WatchList& wl = watches_[idx];
		// first: short clause BCP
		if (idx < maxIdx && !btig.propagate(*this, p)) {
			return false;
		}
		// second: clause BCP
		if (wl.left_size() != 0) {
			WatchList::left_iterator it, end, j = wl.left_begin(); 
			Constraint::PropResult res;
			for (it = wl.left_begin(), end = wl.left_end();  it != end;  ) {
				ClauseWatch& w = *it++;
				res = w.head->ClauseHead::propagate(*this, p, ignore);
				if (res.keepWatch) {
					*j++ = w;
				}
				if (!res.ok) {
					wl.shrink_left(std::copy(it, end, j));
					return false;
				}
			}
			wl.shrink_left(j);
		}
		// third: general constraint BCP
		if (wl.right_size() != 0) {
			WatchList::right_iterator it, end, j = wl.right_begin(); 
			Constraint::PropResult res;
			for (it = wl.right_begin(), end = wl.right_end(); it != end; ) {
				GenericWatch& w = *it++;
				res = w.propagate(*this, p);
				if (res.keepWatch) {
					*j++ = w;
				}
				if (!res.ok) {
					wl.shrink_right(std::copy(it, end, j));
					return false;
				}
			}
			wl.shrink_right(j);
		}
	}
	return DL || assign_.markUnits();
}

bool Solver::test(Literal p, PostPropagator* c) {
	assert(value(p.var()) == value_free && !hasConflict());
	assume(p); --stats.choices;
	uint32 pLevel = decisionLevel();
	freezeLevel(pLevel); // can't split-off this level
	if (propagateUntil(c)) {
		assert(decisionLevel() == pLevel && "Invalid PostPropagators");
		if (c) c->undoLevel(*this);
		undoUntil(pLevel-1);
		return true;
	}
	assert(decisionLevel() == pLevel && "Invalid PostPropagators");
	unfreezeLevel(pLevel);
	cancelPropagation();
	return false;
}

bool Solver::resolveConflict() {
	assert(hasConflict());
	if (decisionLevel() > rootLevel()) {
		if (decisionLevel() != backtrackLevel() && searchMode() != SolverStrategies::no_learning) {
			uint32 uipLevel = analyzeConflict();
			stats.updateJumps(decisionLevel(), uipLevel, backtrackLevel(), ccInfo_.lbd());
			undoUntil( uipLevel );
			return ClauseCreator::create(*this, cc_, ClauseCreator::clause_no_prepare, ccInfo_);
		}
		else {
			return backtrack();
		}
	}
	return false;
}

bool Solver::backtrack() {
	Literal lastChoiceInverted;
	do {
		if (decisionLevel() == rootLevel()) {
			setStopConflict();
			return false;
		}
		lastChoiceInverted = ~decision(decisionLevel());
		levels_.backtrack = decisionLevel() - 1;
		undoUntil(backtrackLevel(), undo_pop_bt_level);
	} while (hasConflict() || !force(lastChoiceInverted, 0));
	// remember flipped literal for copyGuidingPath()
	impliedLits_.add(decisionLevel(), ImpliedLiteral(lastChoiceInverted, decisionLevel(), 0));
	return true;
}

bool ImpliedList::assign(Solver& s) {
	assert(front <= lits.size());
	bool ok             = !s.hasConflict();
	const uint32 DL     = s.decisionLevel();
	VecType::iterator j = lits.begin() + front;
	for (VecType::iterator it = j, end = lits.end(); it != end; ++it) {
		if(it->level <= DL) {
			ok = ok && s.force(it->lit, it->ante.ante(), it->ante.data());
			if (it->level < DL || it->ante.ante().isNull()) { *j++ = *it; }
		}
	}
	lits.erase(j, lits.end());
	level = DL * uint32(!lits.empty());
	front = level > s.rootLevel() ? front  : lits.size();
	return ok;
}

uint32 Solver::undoUntilImpl(uint32 level, bool forceSave) {
	level      = std::max( level, backtrackLevel() );
	if (level >= decisionLevel()) { return decisionLevel(); }
	uint32 num = decisionLevel() - level;
	bool sp    = forceSave || (strategy_.saveProgress > 0 && ((uint32)strategy_.saveProgress) <= num);
	bool ok    = conflict_.empty() && levels_.back().freeze == 0;
	conflict_.clear();
	heuristic_->undoUntil( *this, levels_[level].trailPos);
	undoLevel(sp && ok);
	while (--num) { undoLevel(sp); }
	return level;
}
uint32 Solver::undoUntil(uint32 level, uint32 mode) {
	assert(backtrackLevel() >= rootLevel());
	if ((mode & undo_pop_bt_level) != 0 && backtrackLevel() > level && !varInfo(decision(backtrackLevel()).var()).project()) {
		setBacktrackLevel(level);
	}
	level = undoUntilImpl(level, (mode & undo_save_phases) != 0);
	if (impliedLits_.active(level)) {
		impliedLits_.assign(*this);
	}
	return level;
}
uint32 Solver::estimateBCP(const Literal& p, int rd) const {
	if (value(p.var()) != value_free) return 0;
	LitVec::size_type first = assign_.assigned();
	LitVec::size_type i     = first;
	Solver& self            = const_cast<Solver&>(*this);
	self.assign_.setValue(p.var(), trueValue(p));
	self.assign_.trail.push_back(p);
	const ShortImplicationsGraph& btig = shared_->shortImplications();
	const uint32 maxIdx = btig.size();
	do {
		Literal x = assign_.trail[i++];  
		if (x.index() < maxIdx && !btig.propagateBin(self.assign_, x, 0)) {
			break;
		}
	} while (i < assign_.assigned() && rd-- != 0);
	i = assign_.assigned()-first;
	while (self.assign_.assigned() != first) {
		self.assign_.undoLast();
	}
	return (uint32)i;
}

uint32 Solver::inDegree(WeightLitVec& out) {
	if (decisionLevel() == 0) { return 1; }
	assert(!hasConflict());
	out.reserve((numAssignedVars()-levelStart(1))/10);
	uint32 maxIn  = 1;
	uint32 i      = assign_.trail.size(), stop = levelStart(1);
	for (Antecedent xAnte; i-- != stop; ) {
		Literal x     = assign_.trail[i];
		uint32  xLev  = assign_.level(x.var());
		xAnte         = assign_.reason(x.var());
		uint32  xIn   = 0;
		if (!xAnte.isNull() && xAnte.type() != Antecedent::binary_constraint) {
			xAnte.reason(*this, x, conflict_);
			for (LitVec::const_iterator it = conflict_.begin(); it != conflict_.end(); ++it) {
				xIn += level(it->var()) != xLev;
			}
			if (xIn) {
				out.push_back(WeightLiteral(x, xIn));
				maxIn     = std::max(xIn, maxIn);
			}
			conflict_.clear();
		}
	}
	assert(!hasConflict());
	return maxIn;
}
/////////////////////////////////////////////////////////////////////////////////////////
// Solver: Private helper functions
////////////////////////////////////////////////////////////////////////////////////////
Solver::ConstraintDB* Solver::allocUndo(Constraint* c) {
	if (undoHead_ == 0) {
		return new ConstraintDB(1, c);
	}
	assert(undoHead_->size() == 1);
	ConstraintDB* r = undoHead_;
	undoHead_ = (ConstraintDB*)undoHead_->front();
	r->clear();
	r->push_back(c);
	return r;
}
void Solver::undoFree(ConstraintDB* x) {
	// maintain a single-linked list of undo lists
	x->clear();
	x->push_back((Constraint*)undoHead_);
	undoHead_ = x;
}
// removes the current decision level
void Solver::undoLevel(bool sp) {
	assert(decisionLevel() != 0 && levels_.back().trailPos != assign_.trail.size() && "Decision Level must not be empty");
	assign_.undoTrail(levels_.back().trailPos, sp);
	if (levels_.back().undo) {
		const ConstraintDB& undoList = *levels_.back().undo;
		for (ConstraintDB::size_type i = 0, end = undoList.size(); i != end; ++i) {
			undoList[i]->undoLevel(*this);
		}
		undoFree(levels_.back().undo);
	}
	levels_.pop_back();
}

inline ClauseHead* clause(const Antecedent& ante) {
	return ante.isNull() || ante.type() != Antecedent::generic_constraint ? 0 : ante.constraint()->clause();
}

// computes the First-UIP clause and stores it in cc_, where cc_[0] is the asserting literal (inverted UIP)
// and cc_[1] is a literal from the asserting level (if > 0)
// RETURN: asserting level of the derived conflict clause
uint32 Solver::analyzeConflict() {
	// must be called here, because we unassign vars during analyzeConflict
	heuristic_->undoUntil( *this, levels_.back().trailPos );
	uint32 onLevel  = 0;        // number of literals from the current DL in resolvent
	uint32 resSize  = 0;        // size of current resolvent
	Literal p;                  // literal to be resolved out next
	cc_.assign(1, p);           // will later be replaced with asserting literal
	Antecedent lhs, rhs, last;  // resolve operands
	const bool doOtfs = strategy_.otfs > 0;
	for (bumpAct_.clear();;) {
		uint32 lhsSize = resSize;
		uint32 rhsSize = 0;
		heuristic_->updateReason(*this, conflict_, p);
		for (LitVec::size_type i = 0; i != conflict_.size(); ++i) {
			Literal& q = conflict_[i];
			uint32 cl  = level(q.var());
			rhsSize   += (cl != 0);
			if (!seen(q.var())) {
				++resSize;
				assert(isTrue(q) && "Invalid literal in reason set!");
				assert(cl > 0 && "Top-Level implication not marked!");
				markSeen(q.var());
				if (cl == decisionLevel()) {
					++onLevel;
				}
				else {
					cc_.push_back(~q);
					markLevel(cl);
				}
			}
		}
		if (resSize != lhsSize) { lhs = 0; }
		if (rhsSize != resSize) { rhs = 0; }
		if (doOtfs && (!rhs.isNull() || !lhs.isNull())) {
			// resolvent subsumes rhs and possibly also lhs
			otfs(lhs, rhs, p, onLevel == 1);
		}
		assert(onLevel > 0 && "CONFLICT MUST BE ANALYZED ON CONFLICT LEVEL!");
		// search for the last assigned literal that needs to be analyzed...
		while (!seen(assign_.last().var())) {
			assign_.undoLast();
		}
		p   = assign_.last();
		rhs = reason(p);
		clearSeen(p.var());
		if (--onLevel == 0) { 
			break; 
		}
		--resSize; // p will be resolved out next
		last = rhs;
		reason(p, conflict_);
	}
	cc_[0] = ~p; // store the 1-UIP
	assert(decisionLevel() == level(cc_[0].var()));
	ClauseHead* lastRes = 0;
	if (strategy_.otfs > 1 || !lhs.isNull()) {
		if (!lhs.isNull()) { 
			lastRes = clause(lhs);
		}
		else if (cc_.size() <= (conflict_.size()+1)) {
			lastRes = clause(last);
		}
	}
	if (strategy_.bumpVarAct && reason(p).learnt()) {
		bumpAct_.push_back(WeightLiteral(p, static_cast<LearntConstraint*>(reason(p).constraint())->activity().lbd()));
	}
	return simplifyConflictClause(cc_, ccInfo_, lastRes);
}

void Solver::otfs(Antecedent& lhs, const Antecedent& rhs, Literal p, bool final) {
	ClauseHead* cLhs = clause(lhs), *cRhs = clause(rhs);
	ClauseHead::BoolPair x;
	if (cLhs) {
		x = cLhs->strengthen(*this, ~p, !final);
		if (!x.first || x.second) {
			cLhs = !x.first ? 0 : otfsRemove(cLhs, 0);
		}
	}
	lhs = cLhs;
	if (cRhs) {
		x = cRhs->strengthen(*this, p, !final);
		if (!x.first || (x.second && otfsRemove(cRhs, 0) == 0)) {
			if (x.first && reason(p) == cRhs) { setReason(p, 0); }
			cRhs = 0;
		}
		if (cLhs && cRhs) {
			// lhs and rhs are now equal - only one of them is needed
			if (!cLhs->learnt()) {
				std::swap(cLhs, cRhs);
			}
			otfsRemove(cLhs, 0);
		}
		lhs = cRhs;
	}
}

ClauseHead* Solver::otfsRemove(ClauseHead* c, const LitVec* newC) {
	bool remStatic = !newC || (newC->size() <= 3 && shared_->allowImplicit(Constraint_t::learnt_conflict));
	if (c->learnt() || remStatic) {
		ConstraintDB& db = (c->learnt() ? learnts_ : constraints_);
		ConstraintDB::iterator it;
		if ((it = std::find(db.begin(), db.end(), c)) != db.end()) {
			if (this == shared_->master() && &db == &constraints_) {
				shared_->removeConstraint(static_cast<uint32>(it - db.begin()), true);
			}
			else {
				db.erase(it);
				c->destroy(this, true);
			}
			c = 0;
		}
	}
	return c;
}

// minimizes the conflict clause in cc w.r.t selected strategies.
// PRE:
//  - cc is a valid conflict clause and cc[0] is the UIP-literal
//  - all literals in cc except cc[0] are marked
//  - all decision levels of literals in cc are marked
//  - rhs is 0 or a clause that might be subsumed by cc
// RETURN: finalizeConflictClause(cc, info)
uint32 Solver::simplifyConflictClause(LitVec& cc, ClauseInfo& info, ClauseHead* rhs) {
	// 1. remove redundant literals from conflict clause
	temp_.clear();
	uint32 onAssert = ccMinimize(cc, temp_, strategy_.ccMinAntes, ccMin_);
	uint32 jl       = cc.size() > 1 ? level(cc[1].var()) : 0;
	// clear seen flags of removed literals - keep levels marked
	for (LitVec::size_type x = 0, stop = temp_.size(); x != stop; ++x) {
		clearSeen(temp_[x].var());
	}
	// 2. check for inverse arcs
	if (onAssert == 1 && strategy_.reverseArcs > 0) {
		uint32 maxN = (uint32)strategy_.reverseArcs;
		if      (maxN > 2) maxN = UINT32_MAX;
		else if (maxN > 1) maxN = static_cast<uint32>(cc.size() / 2);
		markSeen(cc[0].var());
		Antecedent ante = ccHasReverseArc(cc[1], jl, maxN);
		if (!ante.isNull()) {
			// resolve with inverse arc
			conflict_.clear();
			ante.reason(*this, ~cc[1], conflict_);
			ccResolve(cc, 1, conflict_);
		}
		clearSeen(cc[0].var());
	}
	// 3. check if final clause subsumes rhs
	if (rhs) {
		conflict_.clear();
		rhs->toLits(conflict_);
		uint32 open   = (uint32)cc.size();
		markSeen(cc[0].var());
		for (LitVec::const_iterator it = conflict_.begin(), end = conflict_.end(); it != end && open; ++it) {
			// NOTE: at this point the DB might not be fully simplified,
			//       e.g. because of mt or lookahead, hence we must explicitly
			//       check for literals assigned on DL 0
			open -= level(it->var()) > 0 && seen(it->var());
		}
		rhs = open ? 0 : otfsRemove(rhs, &cc); 
		if (rhs) { // rhs is subsumed by cc but could not be removed.
			// TODO: we could reuse rhs instead of learning cc
			//       but this would complicate the calling code.
			ClauseHead::BoolPair r(true, false);
			if (cc_.size() < conflict_.size()) {
				//     For now, we only try to strengthen rhs.
				for (LitVec::const_iterator it = conflict_.begin(), end = conflict_.end(); it != end && r.first; ++it) {
					if (!seen(it->var()) || level(it->var()) == 0) {
						r = rhs->strengthen(*this, *it, false);
					}
				}
				if (!r.first) { rhs = 0; }
			}
		}
		clearSeen(cc[0].var());
	}
	// 4. finalize
	uint32 repMode = cc.size() < std::max(strategy_.compress, decisionLevel()+1) ? 0 : strategy_.ccRepMode;
	jl = finalizeConflictClause(cc, info, repMode);
	// 5. bump vars implied by learnt constraints with small lbd 
	if (!bumpAct_.empty()) {
		WeightLitVec::iterator j = bumpAct_.begin();
		weight_t newLbd = info.lbd();
		for (WeightLitVec::iterator it = bumpAct_.begin(), end = bumpAct_.end(); it != end; ++it) {
			if (it->second < newLbd) {
				it->second = 1 + (it->second <= 2); 
				*j++ = *it;	
			}
		}
		bumpAct_.erase(j, bumpAct_.end());
		heuristic_->bump(*this, bumpAct_, 1.0);
	}
	bumpAct_.clear();
	// 6. clear level flags of redundant literals
	for (LitVec::size_type x = 0, stop = temp_.size(); x != stop; ++x) {
		unmarkLevel(level(temp_[x].var()));
	}
	temp_.clear();
	return jl;
}

// conflict clause minimization
// PRE: 
//  - cc is an asserting clause and cc[0] is the asserting literal
//  - all literals in cc are marked as seen
//  -  if ccMin != 0, all decision levels of literals in cc are marked
// POST:
//  - redundant literals were added to removed
//  - if (cc.size() > 1): cc[1] is a literal from the asserting level
// RETURN
//  - the number of literals from the asserting level
uint32 Solver::ccMinimize(LitVec& cc, LitVec& removed, uint32 antes, CCMinRecursive* ccMin) {
	if (ccMin) { ccMin->init(numVars()+1); }
	// skip the asserting literal
	LitVec::size_type j = 1;
	uint32 assertLevel  = 0;
	uint32 assertPos    = 1;
	uint32 onAssert     = 0;
	uint32 varLevel     = 0;
	for (LitVec::size_type i = 1; i != cc.size(); ++i) { 
		if (antes == 0 || !ccRemovable(~cc[i], antes-1, ccMin)) {
			if ( (varLevel = level(cc[i].var())) > assertLevel ) {
				assertLevel = varLevel;
				assertPos   = static_cast<uint32>(j);
				onAssert    = 0;
			}
			onAssert += (varLevel == assertLevel);
			cc[j++] = cc[i];
		}
		else { 
			removed.push_back(cc[i]);
		}
	}
	cc.erase(cc.begin()+j, cc.end());
	if (assertPos != 1) {
		std::swap(cc[1], cc[assertPos]);
	}
	if (ccMin) { ccMin->clear(); }
	return onAssert;
}

// returns true if p is redundant in current conflict clause
bool Solver::ccRemovable(Literal p, uint32 antes, CCMinRecursive* ccMin) {
	const Antecedent& ante = reason(p);
	if (ante.isNull() || !(antes <= (uint32)ante.type())) {
		return false;
	}
	if (!ccMin) { return ante.minimize(*this, p, 0); }
	// recursive minimization
	LitVec& dfsStack = ccMin->dfsStack;
	assert(dfsStack.empty());
	CCMinRecursive::State dfsState = CCMinRecursive::state_removable;
	p.clearWatch();
	dfsStack.push_back(p);
	for (Literal x;; ) {
		x = dfsStack.back();
		dfsStack.pop_back();
		assert(!seen(x.var()) || x == p);
		if (x.watched()) {
			if (x == p) return dfsState == CCMinRecursive::state_removable;
			ccMin->markVisited(x, dfsState);
		}
		else if (dfsState != CCMinRecursive::state_poison) {
			CCMinRecursive::State temp = ccMin->state(x);
			if (temp == CCMinRecursive::state_open) {
				assert(value(x.var()) != value_free && hasLevel(level(x.var())));
				x.watch();
				dfsStack.push_back(x);
				const Antecedent& next = reason(x);
				if (next.isNull() || !(antes <= (uint32)next.type()) || !next.minimize(*this, x, ccMin)) {
					dfsState = CCMinRecursive::state_poison;
				}
			}
			else if (temp == CCMinRecursive::state_poison) {
				dfsState = temp;
			}
		}
	}
}

// checks whether there is a valid "inverse arc" for the given literal p that can be used
// to resolve p out of the current conflict clause
// PRE: 
//  - all literals in the current conflict clause are marked
//  - p is a literal of the current conflict clause and level(p) == maxLevel
// RETURN
//  - An antecedent that is an "inverse arc" for p or null if no such antecedent exists.
Antecedent Solver::ccHasReverseArc(Literal p, uint32 maxLevel, uint32 maxNew) {
	assert(seen(p.var()) && isFalse(p) && level(p.var()) == maxLevel);
	const ShortImplicationsGraph& btig = shared_->shortImplications();
	Antecedent ante;
	if (p.index() < btig.size() && btig.reverseArc(*this, p, maxLevel, ante)) { return ante; }
	WatchList& wl   = watches_[p.index()];
	WatchList::left_iterator it, end; 
	for (it = wl.left_begin(), end = wl.left_end();  it != end;  ++it) {
		if (it->head->isReverseReason(*this, ~p, maxLevel, maxNew)) {
			return it->head;
		}
	}
	return ante;
}

// removes cc[pos] by resolving cc with reason
void Solver::ccResolve(LitVec& cc, uint32 pos, const LitVec& reason) {
	heuristic_->updateReason(*this, reason, cc[pos]);
	Literal x;
	for (LitVec::size_type i = 0; i != reason.size(); ++i) {
		x = reason[i];
		assert(isTrue(x));
		if (!seen(x.var())) {
			markLevel(level(x.var()));
			cc.push_back(~x);
		}
	}
	clearSeen(cc[pos].var());
	unmarkLevel(level(cc[pos].var()));
	cc[pos] = cc.back();
	cc.pop_back();
}

// computes asserting level and lbd of cc and clears flags.
// POST:
//  - literals and decision levels in cc are no longer marked
//  - if cc.size() > 1: cc[1] is a literal from the asserting level
// RETURN: asserting level of conflict clause.
uint32 Solver::finalizeConflictClause(LitVec& cc, ClauseInfo& info, uint32 ccRepMode) {
	// 2. clear flags and compute lbd
	uint32  lbd         = 1;
	uint32  onRoot      = 0;
	uint32  varLevel    = 0;
	uint32  assertLevel = 0;
	uint32  assertPos   = 1;
	uint32  maxVar      = cc[0].var();
	Literal tagLit      = ~tagLiteral();
	bool    tagged      = false;
	for (LitVec::size_type i = 1; i != cc.size(); ++i) {
		Var v = cc[i].var();
		clearSeen(v);
		if (cc[i] == tagLit) { tagged = true; }
		if (v > maxVar)      { maxVar = v;    }
		if ( (varLevel = level(v)) > assertLevel ) {
			assertLevel = varLevel;
			assertPos   = static_cast<uint32>(i);
		}
		if (hasLevel(varLevel)) {
			unmarkLevel(varLevel);
			lbd += (varLevel > rootLevel()) || (++onRoot == 1);
		}
	}
	if (assertPos != 1) { std::swap(cc[1], cc[assertPos]); }
	if (ccRepMode == SolverStrategies::cc_rep_dynamic) {
		ccRepMode = double(lbd)/double(decisionLevel()) > .66 ? SolverStrategies::cc_rep_decision : SolverStrategies::cc_rep_uip;
	}
	if (ccRepMode) {
		maxVar = cc[0].var(), tagged = false, lbd = 1;
		if (ccRepMode == SolverStrategies::cc_rep_decision) {
			// replace cc with decision sequence
			cc.resize(assertLevel+1);
			for (uint32 i = assertLevel; i;){
				Literal x = ~decision(i--);
				cc[lbd++] = x;
				if (x == tagLit)     { tagged = true; }
				if (x.var() > maxVar){ maxVar = x.var(); }
			}
		}
		else {
			// replace cc with all uip clause
			uint32 marked = cc.size() - 1;
			while (cc.size() > 1) { markSeen(~cc.back()); cc.pop_back(); }
			for (LitVec::const_iterator tr = assign_.trail.end(), next, stop; marked;) {
				while (!seen(*--tr)) { ; }
				bool n = --marked != 0 && !reason(*tr).isNull();
				clearSeen(tr->var());
				if (n) { for (next = tr, stop = assign_.trail.begin() + levelStart(level(tr->var())); next-- != stop && !seen(*next);) { ; } }
				if (!n || level(next->var()) != level(tr->var())) {
					cc.push_back(~*tr);
					if (tr->var() == tagLit.var()){ tagged = true; }
					if (tr->var() > maxVar)       { maxVar = tr->var(); }
				}
				else {
					for (reason(*tr, conflict_); !conflict_.empty(); conflict_.pop_back()) {
						if (!seen(conflict_.back())) { ++marked; markSeen(conflict_.back()); }
					}
				}
			}
			lbd = cc.size();
		}
	}
	info.setActivity(ccInfo_.activity());
	info.setLbd(lbd);
	info.setTagged(tagged);
	info.setAux(auxVar(maxVar));
	return assertLevel;
}

// (inefficient) default implementation 
bool Constraint::minimize(Solver& s, Literal p, CCMinRecursive* rec) {
	LitVec temp;
	reason(s, p, temp);
	for (LitVec::size_type i = 0; i != temp.size(); ++i) {
		if (!s.ccMinimize(temp[i], rec)) {
			return false;
		}
	}
	return true;
}

// Selects next branching literal
bool Solver::decideNextBranch(double f) { 
	if (f <= 0.0 || rng.drand() >= f || numFreeVars() == 0) {
		return heuristic_->select(*this);
	}
	// select randomly
	Literal choice;
	uint32 maxVar = numVars() + 1; 
	for (uint32 v = rng.irand(maxVar);;) {
		if (value(v) == value_free) {
			choice    = heuristic_->selectLiteral(*this, v, 0);
			break;
		}
		if (++v == maxVar) { v = 1; }
	}
	return assume(choice);
}
void Solver::resetLearntActivities() {
	Activity hint(0, Activity::MAX_LBD);
	for (uint32 i = 0, end = learnts_.size(); i != end; ++i) {
		static_cast<LearntConstraint*>(learnts_[i])->resetActivity(hint);
	}
}
// Removes up to remFrac% of the learnt nogoods but
// keeps those that are locked or are highly active.
Solver::DBInfo Solver::reduceLearnts(float remFrac, const ReduceStrategy& rs) {
	uint32 oldS = numLearntConstraints();
	uint32 remM = static_cast<uint32>(oldS * std::max(0.0f, remFrac));
	DBInfo r    = {0,0,0};
	CmpScore cmp(learnts_, (ReduceStrategy::Score)rs.score, rs.glue);
	if (remM >= oldS || !remM || rs.algo == ReduceStrategy::reduce_sort) {
		r = reduceSortInPlace(remM, cmp, false);
	}
	else if (rs.algo == ReduceStrategy::reduce_stable) { r = reduceSort(remM, cmp);  }
	else if (rs.algo == ReduceStrategy::reduce_heap)   { r = reduceSortInPlace(remM, cmp, true);}
	else                                               { r = reduceLinear(remM, cmp); }
	stats.addDeleted(oldS - r.size);
	shrinkVecTo(learnts_, r.size);
	return r;
}

// Removes up to maxR of the learnt nogoods.
// Keeps those that are locked or have a high activity and
// does not reorder learnts_.
Solver::DBInfo Solver::reduceLinear(uint32 maxR, const CmpScore& sc) {
	// compute average activity
	uint64 scoreSum  = 0;
	for (LitVec::size_type i = 0; i != learnts_.size(); ++i) {
		scoreSum += sc.score(static_cast<LearntConstraint*>(learnts_[i])->activity());
	}
	double avgAct    = (scoreSum / (double) numLearntConstraints());
	// constraints with socre > 1.5 times the average are "active"
	double scoreThresh = avgAct * 1.5;
	double scoreMax    = (double)sc.score(Activity(Activity::MAX_ACT, 1));
	if (scoreThresh > scoreMax) {
		scoreThresh = (scoreMax + (scoreSum / (double) numLearntConstraints())) / 2.0;
	}
	// remove up to maxR constraints but keep "active" and locked once
	const uint32 glue = sc.glue;
	DBInfo       res  = {0,0,0};
	for (LitVec::size_type i = 0; i != learnts_.size(); ++i) {
		LearntConstraint* c = static_cast<LearntConstraint*>(learnts_[i]);
		Activity a          = c->activity();
		bool isLocked       = c->locked(*this);
		bool isGlue         = (sc.score(a) > scoreThresh || a.lbd() <= glue);
		if (maxR == 0 || isLocked || isGlue) {
			res.pinned             += isGlue;
			res.locked             += isLocked;
			learnts_[res.size++]    = c;
			c->decreaseActivity();
		}
		else {
			--maxR;
			c->destroy(this, true);
		}
	}
	return res;
}

// Sorts learnt constraints by score and removes the
// maxR constraints with the lowest score without
// reordering learnts_.
Solver::DBInfo Solver::reduceSort(uint32 maxR, const CmpScore& sc) {
	typedef PodVector<CmpScore::ViewPair>::type HeapType;
	maxR              = std::min(maxR, (uint32)learnts_.size());
	const uint32 glue = sc.glue;
	DBInfo       res  = {0,0,0};
	HeapType     heap;
	heap.reserve(maxR);
	bool isGlue, isLocked;
	for (LitVec::size_type i = 0; i != learnts_.size(); ++i) {
		LearntConstraint* c = static_cast<LearntConstraint*>(learnts_[i]);
		CmpScore::ViewPair vp(i, c->activity());
		res.pinned += (isGlue   = (vp.second.lbd() <= glue));
		res.locked += (isLocked = c->locked(*this));
		if (!isLocked && !isGlue) { 
			if (maxR) { // populate heap with first maxR constraints
				heap.push_back(vp); 
				if (--maxR == 0) { std::make_heap(heap.begin(), heap.end(), sc); } 
			}
			else if (sc(vp, heap[0])) { // replace max element in heap
				std::pop_heap(heap.begin(), heap.end(), sc);
				heap.back() = vp;
				std::push_heap(heap.begin(), heap.end(), sc);
			}
		}
	}
	// Remove all constraints in heap - those are "inactive".
	for (HeapType::const_iterator it = heap.begin(), end = heap.end(); it != end; ++it) {
		learnts_[it->first]->destroy(this, true);
		learnts_[it->first] = 0;
	}
	// Cleanup db and decrease activity of remaining constraints.
	uint32 j = 0;
	for (LitVec::size_type i = 0; i != learnts_.size(); ++i) {
		if (LearntConstraint* c = static_cast<LearntConstraint*>(learnts_[i])) {
			c->decreaseActivity();
			learnts_[j++] = c;
		}
	}
	res.size = j;
	return res;
}

// Sorts the learnt db by score and removes the first 
// maxR constraints (those with the lowest score). 
Solver::DBInfo Solver::reduceSortInPlace(uint32 maxR, const CmpScore& sc, bool partial) {
	maxR                        = std::min(maxR, (uint32)learnts_.size());
	ConstraintDB::iterator nEnd = learnts_.begin();
	const uint32 glue           = sc.glue;
	DBInfo res                  = {0,0,0};
	bool isGlue, isLocked;
	if (!partial) {
		// sort whole db and remove first maxR constraints
		if (maxR && maxR != learnts_.size()) std::stable_sort(learnts_.begin(), learnts_.end(), sc);
		for (ConstraintDB::iterator it = learnts_.begin(), end = learnts_.end(); it != end; ++it) {
			LearntConstraint* c = static_cast<LearntConstraint*>(*it);
			res.pinned         += (isGlue = (c->activity().lbd() <= glue)); 
			res.locked         += (isLocked = c->locked(*this));
			if (!maxR || isLocked || isGlue) { c->decreaseActivity(); *nEnd++ = c; }
			else                             { c->destroy(this, true); --maxR; }
		}
	}
	else {
		ConstraintDB::iterator hBeg = learnts_.begin();
		ConstraintDB::iterator hEnd = learnts_.begin();
		for (ConstraintDB::iterator it = learnts_.begin(), end = learnts_.end(); it != end; ++it) {
			LearntConstraint* c = static_cast<LearntConstraint*>(*it);
			res.pinned         += (isGlue = (c->activity().lbd() <= glue)); 
			res.locked         += (isLocked = c->locked(*this));
			if      (isLocked || isGlue) { continue; }
			else if (maxR)               {
				*it     = *hEnd;
				*hEnd++ = c;
				if (--maxR == 0) { std::make_heap(hBeg, hEnd, sc); }
			}
			else if (sc(c, learnts_[0])) {
				std::pop_heap(hBeg, hEnd, sc);
				*it      = *(hEnd-1);
				*(hEnd-1)= c;
				std::push_heap(hBeg, hEnd, sc);
			}
		}
		// remove all constraints in heap
		for (ConstraintDB::iterator it = hBeg; it != hEnd; ++it) {
			static_cast<LearntConstraint*>(*it)->destroy(this, true);
		}
		// copy remaining constraints down
		for (ConstraintDB::iterator it = hEnd, end = learnts_.end(); it != end; ++it) {
			LearntConstraint* c = static_cast<LearntConstraint*>(*it);
			c->decreaseActivity();
			*nEnd++ = c;
		}
	}
	res.size = static_cast<uint32>(std::distance(learnts_.begin(), nEnd));
	return res;	
}

uint32 Solver::countLevels(const Literal* first, const Literal* last, uint32 maxLevel) {
	if (maxLevel < 2)    { return uint32(maxLevel && first != last); }
	if (++lbdTime_ != 0) { lbdStamp_.resize(levels_.size()+1, lbdTime_-1); }
	else                 { lbdStamp_.assign(levels_.size()+1, lbdTime_); lbdTime_ = 1; }
	lbdStamp_[0] = lbdTime_;
	uint32 levels= 0;
	for (uint32 lev; first != last; ++first) {
		lev = level(first->var());
		if (lbdStamp_[lev] != lbdTime_) {
			lbdStamp_[lev] = lbdTime_;
			if (++levels == maxLevel) { break; }
		}
	}
	return levels;
}

uint64 Solver::updateBranch(uint32 cfl) {
	int32 dl = (int32)decisionLevel(), xl = static_cast<int32>(cflStamp_.size())-1;
	if      (xl > dl) { do { cfl += cflStamp_.back(); cflStamp_.pop_back(); } while (--xl != dl); }
	else if (dl > xl) { cflStamp_.insert(cflStamp_.end(), dl - xl, 0); }
	return cflStamp_.back() += cfl;
}
/////////////////////////////////////////////////////////////////////////////////////////
// The basic DPLL-like search-function
/////////////////////////////////////////////////////////////////////////////////////////
ValueRep Solver::search(SearchLimits& limit, double rf) {
	assert(!isFalse(tagLiteral()));
	uint64 local = limit.local != UINT64_MAX ? limit.local : 0;
	rf           = std::max(0.0, std::min(1.0, rf));
	if (local && decisionLevel() == rootLevel()) { cflStamp_.assign(decisionLevel()+1, 0); }
	do {
		for (uint32 conflicts = hasConflict() || !propagate() || !simplify();;) {
			if (conflicts) {
				for (conflicts = 1; resolveConflict() && !propagate(); ) { ++conflicts; }
				limit.conflicts -= conflicts < limit.conflicts ? conflicts : limit.conflicts;
				if (local && updateBranch(conflicts) >= local)              { limit.local = 0; }
				if (hasConflict() || (decisionLevel() == 0 && !simplify())) { return value_false; }
				if ((limit.reached() || learntLimit(limit))&&numFreeVars()) { return value_free;  }
			}
			if (decideNextBranch(rf)) { conflicts = !propagate(); }
			else                      { break; }
		}
	} while (!post_.isModel(*this));
	temp_.clear();
	model.clear(); model.reserve(numVars()+1);
	for (Var v = 0; v <= numVars(); ++v) { model.push_back(value(v)); }
	if (satPrepro()) { satPrepro()->extendModel(model, temp_); }
	return value_true;
}
}
