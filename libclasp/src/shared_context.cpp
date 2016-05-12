// 
// Copyright (c) 2010-2012, Benjamin Kaufmann
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
#include <clasp/shared_context.h>
#include <clasp/solver.h>
#include <clasp/clause.h>
#include <clasp/dependency_graph.h>
#if WITH_THREADS
#include <clasp/util/thread.h>
#endif
namespace Clasp {
double ProblemStats::operator[](const char* key) const {
#define RETURN_IF(x) if (std::strcmp(key, #x) == 0) return double(x)
	RETURN_IF(vars);
	RETURN_IF(vars_eliminated);
	RETURN_IF(vars_frozen);
	RETURN_IF(constraints);
	RETURN_IF(constraints_binary);
	RETURN_IF(constraints_ternary);
	RETURN_IF(complexity);
	return -1.0;
#undef RETURN_IF
}
const char* ProblemStats::keys(const char* k) {
	if (!k || !*k) { return "vars\0vars_eliminated\0vars_frozen\0constraints\0constraints_binary\0constraints_ternary\0complexity\0"; }
	return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////
// ShortImplicationsGraph::ImplicationList
/////////////////////////////////////////////////////////////////////////////////////////
#if WITH_THREADS
ShortImplicationsGraph::Block::Block() {
	for (int i = 0; i != block_cap; ++i) { data[i] = posLit(0); }
	size_lock = 0;
	next      = 0;
}
void ShortImplicationsGraph::Block::addUnlock(uint32 lockedSize, const Literal* x, uint32 xs) {
	std::copy(x, x+xs, data+lockedSize);
	size_lock = ((lockedSize+xs) << 1);
}
bool ShortImplicationsGraph::Block::tryLock(uint32& size) {
	uint32 s = size_lock;
	if ((s & 1) == 0 && size_lock.compare_and_swap(s | 1, s) == s) {
		size = s >> 1;
		return true;
	}
	return false;
}

#define FOR_EACH_LEARNT(x, Y) \
	for (Block* b = (x).learnt; b ; b = b->next) \
		for (const Literal* Y = b->begin(), *endof = b->end(); Y != endof; Y += 2 - Y->watched())

	
ShortImplicationsGraph::ImplicationList::~ImplicationList() {
	clear(true);
}

void ShortImplicationsGraph::ImplicationList::clear(bool b) {
	ImpListBase::clear(b);
	for (Block* x = learnt; x; ) {
		Block* t = x;
		x = x->next;
		delete t;
	}
	learnt = 0;
}
void ShortImplicationsGraph::ImplicationList::simplifyLearnt(const Solver& s) {
	Block* x = learnt;
	learnt   = 0;
	while (x) {
		for (const Literal* Y = x->begin(), *endof = x->end(); Y != endof; Y += 2 - Y->watched()) {
			Literal p = Y[0], q = !Y->watched() ? Y[1] : negLit(0);
			if (!s.isTrue(p) && !s.isTrue(q)) {
				addLearnt(p, q);
			}
		}
		Block* t = x;
		x = x->next;
		delete t;
	}
}
void ShortImplicationsGraph::ImplicationList::addLearnt(Literal p, Literal q) {
	Literal nc[2] = {p, q};
	uint32  ns    = 1 + !isSentinel(q);
	if (ns == 1) { nc[0].watch(); }
	for (Block* x;;) {
		x = learnt;
		if (x) {
			uint32 lockedSize;
			if (x->tryLock(lockedSize)) {
				if ( (lockedSize + ns) <=  Block::block_cap ) {
					x->addUnlock(lockedSize, nc, ns);
				}
				else {
					Block* t = new Block();
					t->addUnlock(0, nc, ns);
					t->next   = x; // x is full and remains locked forever
					x = learnt= t; // publish new block - unlocks x and learnt
				}
				return;
			}
			else { 
				Clasp::this_thread::yield();
			}
		}
		else {
			x = new Block();
			if (learnt.compare_and_swap(x, 0) != 0) {
				delete x;
			}
		}
	}
}

bool ShortImplicationsGraph::ImplicationList::hasLearnt(Literal q, Literal r) const {
	const bool binary = isSentinel(r);
	FOR_EACH_LEARNT(*this, imp) {
		if (imp[0] == q || imp[0] == r) {
			// binary clause subsumes new bin/tern clause
			if (imp->watched())                          { return true; }
			// existing ternary clause subsumes new tern clause
			if (!binary && (imp[1] == q || imp[1] == r)) { return true; }
		}
	}
	return false;
}

void ShortImplicationsGraph::ImplicationList::move(ImplicationList& other) {
	ImpListBase::move(other);
	delete learnt;
	learnt       = other.learnt;
	other.learnt = 0;
}
#endif
/////////////////////////////////////////////////////////////////////////////////////////
// ShortImplicationsGraph
/////////////////////////////////////////////////////////////////////////////////////////
ShortImplicationsGraph::ShortImplicationsGraph() {
	bin_[0]  = bin_[1]  = 0;
	tern_[0] = tern_[1] = 0;
	shared_  = false;
}
ShortImplicationsGraph::~ShortImplicationsGraph() {
	PodVector<ImplicationList>::destruct(graph_);
}
void ShortImplicationsGraph::resize(uint32 nodes) {
	if (graph_.capacity() >= nodes) {
		graph_.resize(nodes);
	}
	else {
		ImpLists temp; temp.resize(nodes);
		for (ImpLists::size_type i = 0; i != graph_.size(); ++i) {
			temp[i].move(graph_[i]);
		}
		graph_.swap(temp);
	}
}

uint32 ShortImplicationsGraph::numEdges(Literal p) const { return graph_[p.index()].size(); }

bool ShortImplicationsGraph::add(ImpType t, bool learnt, const Literal* lits) {
	uint32& stats= (t == ternary_imp ? tern_ : bin_)[learnt];
	Literal p = lits[0], q = lits[1], r = (t == ternary_imp ? lits[2] : negLit(0));
	p.clearWatch(), q.clearWatch(), r.clearWatch();
	if (!shared_) {
		if (learnt) { p.watch(), q.watch(), r.watch(); }
		if (t == binary_imp) {
			getList(~p).push_left(q);
			getList(~q).push_left(p);
		}
		else {
			getList(~p).push_right(std::make_pair(q, r));
			getList(~q).push_right(std::make_pair(p, r));
			getList(~r).push_right(std::make_pair(p, q));
		}
		++stats;
		return true;
	}
#if WITH_THREADS
	else if (learnt && !getList(~p).hasLearnt(q, r)) {
		getList(~p).addLearnt(q, r);
		getList(~q).addLearnt(p, r);
		if (t == ternary_imp) {
			getList(~r).addLearnt(p, q);
		}
		++stats;
		return true;
	}
#endif
	return false;
}

void ShortImplicationsGraph::remove_bin(ImplicationList& w, Literal p) {
	w.erase_left_unordered(std::find(w.left_begin(), w.left_end(), p)); 
	w.try_shrink(); 
}
void ShortImplicationsGraph::remove_tern(ImplicationList& w, Literal p) {
	w.erase_right_unordered(std::find_if(w.right_begin(), w.right_end(), PairContains<Literal>(p))); 
	w.try_shrink();	
}

// Removes all binary clauses containing p - those are now SAT.
// Binary clauses containing ~p are unit and therefore likewise SAT. Those
// are removed when their second literal is processed.
//
// Ternary clauses containing p are SAT and therefore removed.
// Ternary clauses containing ~p are now either binary or SAT. Those that
// are SAT are removed when the satisfied literal is processed.
// All conditional binary-clauses are replaced with real binary clauses.
// Note: clauses containing p watch ~p. Those containing ~p watch p.
void ShortImplicationsGraph::removeTrue(const Solver& s, Literal p) {
	assert(!shared_);
	typedef ImplicationList SWL;
	SWL& negPList = graph_[(~p).index()];
	SWL& pList    = graph_[ (p).index()];
	// remove every binary clause containing p -> clause is satisfied
	for (SWL::left_iterator it = negPList.left_begin(), end = negPList.left_end(); it != end; ++it) {
		--bin_[it->watched()];
		remove_bin(graph_[(~*it).index()], p);
	}
	// remove every ternary clause containing p -> clause is satisfied
	for (SWL::right_iterator it = negPList.right_begin(), end = negPList.right_end(); it != end; ++it) {
		--tern_[it->first.watched()];
		remove_tern(graph_[ (~it->first).index() ], p);
		remove_tern(graph_[ (~it->second).index() ], p);
	}
#if WITH_THREADS
	FOR_EACH_LEARNT(negPList, imp) {
		graph_[(~imp[0]).index()].simplifyLearnt(s);
		if (!imp->watched()){
			--tern_[1];
			graph_[(~imp[1]).index()].simplifyLearnt(s);
		}
		if (imp->watched()) { --bin_[1]; }
	}
#endif
	// transform ternary clauses containing ~p to binary clause
	for (SWL::right_iterator it = pList.right_begin(), end = pList.right_end(); it != end; ++it) {
		Literal q = it->first;
		Literal r = it->second;
		--tern_[q.watched()];
		remove_tern(graph_[(~q).index()], ~p);
		remove_tern(graph_[(~r).index()], ~p);
		if (s.value(q.var()) == value_free && s.value(r.var()) == value_free) {
			// clause is binary on dl 0
			Literal imp[2] = {q,r};
			add(binary_imp, false, imp);
		}
		// else: clause is SAT and removed when the satisfied literal is processed
	}
	graph_[(~p).index()].clear(true);
	graph_[ (p).index()].clear(true);
}
#undef FOR_EACH_LEARNT
struct ShortImplicationsGraph::Propagate {
	Propagate(Solver& a_s) : s(&a_s) {}
	bool unary(Literal p, Literal x) const { return s->isTrue(x) || s->force(x, Antecedent(p)); }
	bool binary(Literal p, Literal x, Literal y) const {
		ValueRep vx = s->value(x.var()), vy;
		if (vx != trueValue(x) && (vy=s->value(y.var())) != trueValue(y) && vx + vy) {
			return vx != 0 ? s->force(y, Antecedent(p, ~x)) : s->force(x, Antecedent(p, ~y));
		}
		return true;
	}
	Solver* s;
};
struct ShortImplicationsGraph::ReverseArc {
	ReverseArc(const Solver& a_s, uint32 m, Antecedent& o) : s(&a_s), out(&o), maxL(m) {}
	bool unary(Literal, Literal x) const {
		if (!isRevLit(*s, x, maxL)) { return true; }
		*out = Antecedent(~x); 
		return false;
	}
	bool binary(Literal, Literal x, Literal y) const {
		if (!isRevLit(*s, x, maxL) || !isRevLit(*s, y, maxL)) { return true; }
		*out = Antecedent(~x, ~y);
		return false;
	}
	const Solver* s; Antecedent* out; uint32 maxL;
};
bool ShortImplicationsGraph::propagate(Solver& s, Literal p) const { return forEach(p, Propagate(s)); }
bool ShortImplicationsGraph::reverseArc(const Solver& s, Literal p, uint32 maxLev, Antecedent& out) const { return !forEach(p, ReverseArc(s, maxLev, out)); }
bool ShortImplicationsGraph::propagateBin(Assignment& out, Literal p, uint32 level) const {
	const ImplicationList& x = graph_[p.index()];
	Antecedent ante(p);
	for (ImplicationList::const_left_iterator it = x.left_begin(), end = x.left_end(); it != end; ++it) {
		if (!out.assign(*it, level, p)) { return false; }
	}
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////
// SatPreprocessor
/////////////////////////////////////////////////////////////////////////////////////////
SatPreprocessor::~SatPreprocessor() {
	discardClauses(true);
}
void SatPreprocessor::discardClauses(bool full) {
	for (ClauseList::size_type i = 0; i != clauses_.size(); ++i) {
		if (clauses_[i]) { clauses_[i]->destroy(); }
	}
	ClauseList().swap(clauses_);
	if (Clause* r = (full ? elimTop_ : 0)) {
		do {
			Clause* t = r;
			 r = r->next();
			 t->destroy();
		} while (r);
		elimTop_ = 0;
	}
	if (full) { seen_ = Range32(1,1); }
}
void SatPreprocessor::cleanUp(bool full) {
	if (ctx_) { seen_.hi = ctx_->numVars() + 1; }
	doCleanUp();
	discardClauses(full);
}

bool SatPreprocessor::addClause(const Literal* lits, uint32 size) {
	if (size > 1) {
		clauses_.push_back( Clause::newClause(lits, size) );
	}
	else if (size == 1) {
		units_.push_back(lits[0]);
	}
	else {
		return false;
	}
	return true;
}	

void SatPreprocessor::freezeSeen() {
	if (!ctx_->validVar(seen_.lo)) { seen_.lo = 1; }
	if (!ctx_->validVar(seen_.hi)) { seen_.hi = ctx_->numVars() + 1; }
	for (Var v = seen_.lo; v != seen_.hi; ++v) {
		if (!ctx_->eliminated(v)) { ctx_->setFrozen(v, true); }
	}
	seen_.lo = seen_.hi;
}

bool SatPreprocessor::preprocess(SharedContext& ctx, Options& opts) {
	ctx_ = &ctx;
	Solver* s = ctx_->master();
	struct OnExit {
		SharedContext*   ctx;
		SatPreprocessor* self;
		SatPreprocessor* rest;
		OnExit(SatPreprocessor* s, SharedContext* c) : ctx(c), self(s), rest(0) {
			if (ctx && ctx->satPrepro.get() == s) { rest = ctx->satPrepro.release(); }
		}
		~OnExit() { 
			if (self) self->cleanUp(); 
			if (rest) ctx->satPrepro.reset(rest);
		}
	} onExit(this, &ctx);
	for (LitVec::const_iterator it = units_.begin(), end = units_.end(); it != end; ++it) {
		if (!ctx.addUnary(*it)) return false;
	}
	units_.clear();
	// skip preprocessing if other constraints are UNSAT
	if (!s->propagate()) return false;
	if (ctx.preserveModels() || opts.mode == Options::prepro_preserve_models) {
		opts.mode = Options::prepro_preserve_models;
		opts.disableBce();
	}
	// preprocess only if not too many vars are frozen or not too many clauses
	bool limFrozen= false;
	if (opts.limFrozen != 0 && ctx_->stats().vars_frozen) {
		uint32 varFrozen = ctx_->stats().vars_frozen;
		for (LitVec::const_iterator it = s->trail().begin(), end = s->trail().end(); it != end; ++it) {
 			varFrozen -= (ctx_->varInfo(it->var()).frozen());
 		}
		limFrozen = ((varFrozen / double(s->numFreeVars())) * 100.0) > double(opts.limFrozen);
	}
	// 1. remove SAT-clauses, strengthen clauses w.r.t false literals, attach
	if (opts.type != 0 && !opts.clauseLimit(numClauses()) && !limFrozen && initPreprocess(opts)) {
		ClauseList::size_type j = 0; 
		for (ClauseList::size_type i = 0; i != clauses_.size(); ++i) {
			Clause* c   = clauses_[i]; assert(c);
			clauses_[i] = 0;
			c->simplify(*s);
			Literal x   = (*c)[0];
			if (s->value(x.var()) == value_free) {
				clauses_[j++] = c; 
			}
			else {
				c->destroy();
				if (!ctx.addUnary(x)) { return false; }	
			}
		}
		clauses_.erase(clauses_.begin()+j, clauses_.end());
		// 2. run preprocessing
		freezeSeen();
		if (!s->propagate() || !doPreprocess()) {
			return false;
		}
	}
	// simplify other constraints w.r.t any newly derived top-level facts
	if (!s->simplify()) return false;
	// 3. move preprocessed clauses to ctx
	for (ClauseList::size_type i = 0; i != clauses_.size(); ++i) {
		if (Clause* c = clauses_[i]) {
			if (!ClauseCreator::create(*s, ClauseRep::create(&(*c)[0], c->size()), 0)) {
				return false;
			}
			clauses_[i] = 0;
			c->destroy();
		}
	}
	ClauseList().swap(clauses_);
	return true;
}
bool SatPreprocessor::preprocess(SharedContext& ctx) {
	SatPreParams opts = ctx.configuration()->context().satPre;
	return preprocess(ctx, opts);
}
void SatPreprocessor::extendModel(ValueVec& m, LitVec& open) {
	if (!open.empty()) {
		// flip last unconstraint variable to get "next" model
		open.back() = ~open.back();
	}
	doExtendModel(m, open);
	// remove unconstraint vars already flipped
	while (!open.empty() && open.back().sign()) {
		open.pop_back();
	}
}
SatPreprocessor::Clause* SatPreprocessor::Clause::newClause(const Literal* lits, uint32 size) {
	assert(size > 0);
	void* mem = ::operator new( sizeof(Clause) + (size-1)*sizeof(Literal) );
	return new (mem) Clause(lits, size);
}
SatPreprocessor::Clause::Clause(const Literal* lits, uint32 size) : size_(size), inQ_(0), marked_(0) {
	std::memcpy(lits_, lits, size*sizeof(Literal));
}
void SatPreprocessor::Clause::strengthen(Literal p) {
	uint64 a = 0;
	uint32 i, end;
	for (i   = 0; lits_[i] != p; ++i) { a |= Clause::abstractLit(lits_[i]); }
	for (end = size_-1; i < end; ++i) { lits_[i] = lits_[i+1]; a |= Clause::abstractLit(lits_[i]); }
	--size_;
	data_.abstr = a;
}
void SatPreprocessor::Clause::simplify(Solver& s) {
	uint32 i;
	for (i = 0; i != size_ && s.value(lits_[i].var()) == value_free; ++i) {;}
	if      (i == size_)        { return; }
	else if (s.isTrue(lits_[i])){ std::swap(lits_[i], lits_[0]); return;  }
	uint32 j = i++;
	for (; i != size_; ++i) {
		if (s.isTrue(lits_[i]))   { std::swap(lits_[i], lits_[0]); return;  }
		if (!s.isFalse(lits_[i])) { lits_[j++] = lits_[i]; }
	}
	size_ = j;
}
void SatPreprocessor::Clause::destroy() {
	void* mem = this;
	this->~Clause();
	::operator delete(mem);
}
/////////////////////////////////////////////////////////////////////////////////////////
// SharedContext
/////////////////////////////////////////////////////////////////////////////////////////
static BasicSatConfig config_def_s;

EventHandler::~EventHandler() {}
uint32 Event::nextId() { static uint32 id = 0; return id++; }
SharedContext::SharedContext() 
	: symTabPtr_(new SharedSymTab()), progress_(0), lastTopLevel_(0) {
	Antecedent::checkPlatformAssumptions();
	init();
}

void SharedContext::init() {
	Var sentinel  = addVar(Var_t::atom_var); // sentinel always present
	setFrozen(sentinel, true);
	problem_.vars = 0;
	config_       = &config_def_s;
	config_.release();
	addSolver();
}

bool SharedContext::ok() const { return master()->decisionLevel() || !master()->hasConflict() || master()->hasStopConflict(); }
void SharedContext::enableStats(uint32 lev) {
	if (lev > 0) { master()->stats.enableExtended(); }
	if (lev > 1) { master()->stats.enableJump();     }
}
void SharedContext::cloneVars(const SharedContext& other, InitMode m) {
	problem_.vars            = other.problem_.vars;
	problem_.vars_eliminated = other.problem_.vars_eliminated;
	problem_.vars_frozen     = other.problem_.vars_frozen;
	varInfo_                 = other.varInfo_;
	if (&symbolTable() != &other.symbolTable()) {
		if (m == init_copy_symbols) { other.symbolTable().copyTo(symbolTable()); }
		else { 
			++other.symTabPtr_->refs;
			if (--symTabPtr_->refs == 0) { delete symTabPtr_; }
			symTabPtr_ = other.symTabPtr_;
		}
	}
}

SharedContext::~SharedContext() {
	while (!solvers_.empty()) { delete solvers_.back(); solvers_.pop_back(); }
	while (!accu_.empty())    { delete accu_.back(); accu_.pop_back(); }
	if (--symTabPtr_->refs == 0) delete symTabPtr_;
}

void SharedContext::reset() {
	this->~SharedContext();
	new (this) SharedContext();	
}

void SharedContext::setConcurrency(uint32 n, ResizeMode mode) {
	if (n <= 1) { share_.count = 1; }
	else        { share_.count = n; solvers_.reserve(n); }
	while (solvers_.size() < share_.count && (mode & mode_add) != 0u) {
		addSolver();
	}
	while (solvers_.size() > share_.count && (mode & mode_remove) != 0u) {
		delete solvers_.back(); 
		solvers_.pop_back();
	}
	if ((share_.shareM & ContextParams::share_auto) != 0) {
		setShareMode(ContextParams::share_auto);
	}
}

void SharedContext::setShareMode(ContextParams::ShareMode m) {
	if ( (share_.shareM = static_cast<uint32>(m)) == ContextParams::share_auto && share_.count > 1) {
		share_.shareM |= static_cast<uint32>(ContextParams::share_all);
	}
}
void SharedContext::setShortMode(ContextParams::ShortMode m) {
	share_.shortM = static_cast<uint32>(m);
}

Solver& SharedContext::addSolver() {
	uint32 id    = (uint32)solvers_.size();
	share_.count = std::max(share_.count, id + 1);
	Solver* s    = new Solver(this, id);
	solvers_.push_back(s);
	return *s;
}

void SharedContext::setConfiguration(Configuration* c, bool own) {
	if (c == 0) { c = &config_def_s; own = false; }
	if (config_.get() != c) {
		config_ = c;
		if (!own) config_.release();
		config_->prepare(*this);
		const ContextParams& opts = config_->context();
		setShareMode(static_cast<ContextParams::ShareMode>(opts.shareMode));
		setShortMode(static_cast<ContextParams::ShortMode>(opts.shortMode));
		share_.seed   = opts.seed;
		share_.satPreM= opts.satPre.mode;
		if (satPrepro.get() == 0 && opts.satPre.type != SatPreParams::sat_pre_no) {
			satPrepro.reset(SatPreParams::create(opts.satPre));
		}
		enableStats(opts.stats);
		// force update on next call to Solver::startInit()
		for (uint32 i = 0; i != solvers_.size(); ++i) {
			solvers_[i]->resetConfig();
		}
	}
	else if (own != config_.is_owner()) {
		if (own) config_.acquire();
		else     config_.release();
	}
}

bool SharedContext::unfreeze() {
	if (frozen()) {
		share_.frozen = 0;
		share_.winner = 0;
		btig_.markShared(false);
		return master()->popRootLevel(master()->rootLevel())
		  &&   btig_.propagate(*master(), posLit(0)) // any newly learnt facts
		  &&   unfreezeStep();
	}
	return true;
}

bool SharedContext::unfreezeStep() {
	for (SolverVec::size_type i = solvers_.size(); i-- ; ) {
		Solver& s = *solvers_[i];
		if (!s.validVar(step_.var())) { continue; }
		s.endStep(lastTopLevel_);
		const SolverParams& params = configuration()->solver(s.id());
		if (params.forgetLearnts())   { s.reduceLearnts(1.0f); }
		if (params.forgetHeuristic()) { s.setHeuristic(0); }
		if (params.forgetSigns())     { s.resetPrefs(); }
		if (params.forgetActivities()){ s.resetLearntActivities(); }
	}
	return !master()->hasConflict();
}

Var SharedContext::addVar(VarType t, bool eq) {
	VarInfo nv;
	if (t == Var_t::body_var) { nv.set(VarInfo::BODY); }
	if (eq)                   { nv.set(VarInfo::EQ);   }
	varInfo_.push_back(nv);
	++problem_.vars;
	return numVars();
}

void SharedContext::requestStepVar()  { if (step_ == posLit(0)) { step_= negLit(0); } }
void SharedContext::requestData(Var v){ master()->requestData(v); }

void SharedContext::setFrozen(Var v, bool b) {
	assert(validVar(v)); 
	if (v && b != varInfo_[v].has(VarInfo::FROZEN)) {
		varInfo_[v].toggle(VarInfo::FROZEN);
		b ? ++problem_.vars_frozen : --problem_.vars_frozen;
	}
}
bool SharedContext::eliminated(Var v) const {
	assert(validVar(v)); 
	return !master()->assign_.valid(v);
}

void SharedContext::eliminate(Var v) {
	assert(validVar(v) && !frozen() && master()->decisionLevel() == 0); 
	if (!eliminated(v)) {
		++problem_.vars_eliminated;
		// eliminate var from assignment - no longer a decision variable!
		master()->assign_.eliminate(v);
	}
}

Literal SharedContext::addAuxLit() {
	VarInfo nv; nv.set(VarInfo::FROZEN);
	varInfo_.push_back(nv);
	return posLit(numVars());
}
Solver& SharedContext::startAddConstraints(uint32 constraintGuess) {
	if (!unfreeze())              { return *master(); }
	if (master()->isFalse(step_)) { step_= addAuxLit(); }
	btig_.resize((numVars()+1)<<1);
	master()->startInit(constraintGuess, configuration()->solver(0));
	return *master();
}
bool SharedContext::addUnary(Literal x) {
	CLASP_ASSERT_CONTRACT(!frozen() || !isShared());
	return master()->force(x);
}
bool SharedContext::addBinary(Literal x, Literal y) {
	CLASP_ASSERT_CONTRACT(allowImplicit(Constraint_t::static_constraint));
	Literal lits[2] = {x, y};
	return ClauseCreator::create(*master(), ClauseRep(lits, 2), ClauseCreator::clause_force_simplify);
}
bool SharedContext::addTernary(Literal x, Literal y, Literal z) {
	CLASP_ASSERT_CONTRACT(allowImplicit(Constraint_t::static_constraint));
	Literal lits[3] = {x, y, z};
	return ClauseCreator::create(*master(), ClauseRep(lits, 3), ClauseCreator::clause_force_simplify);
}
void SharedContext::add(Constraint* c) {
	CLASP_ASSERT_CONTRACT(!frozen());
	master()->add(c);
}
int SharedContext::addImp(ImpGraph::ImpType t, const Literal* lits, ConstraintType ct) {
	if (!allowImplicit(ct)) { return -1; }
	bool learnt = ct != Constraint_t::static_constraint;
	if (!learnt && !frozen() && satPrepro.get()) {
		satPrepro->addClause(lits, static_cast<uint32>(t));
		return 1;
	}
	return int(btig_.add(t, learnt, lits));
}

uint32 SharedContext::numConstraints() const { return numBinary() + numTernary() + master()->constraints_.size(); }

bool SharedContext::endInit(bool attachAll) {
	assert(!frozen());
	report(message(Event::subsystem_prepare, "Preprocessing"));
	initStats(*master());
	SatPrePtr temp;
	satPrepro.swap(temp);
	bool ok = !master()->hasConflict() && master()->preparePost() && (!temp.get() || temp->preprocess(*this)) && master()->endInit();
	satPrepro.swap(temp);
	btig_.markShared(concurrency() > 1);
	master()->dbIdx_            = (uint32)master()->constraints_.size();
	lastTopLevel_               = (uint32)master()->assign_.front;
	problem_.constraints        = master()->constraints_.size();
	problem_.constraints_binary = btig_.numBinary();
	problem_.constraints_ternary= btig_.numTernary();
	problem_.complexity         = std::max(problem_.complexity, problemComplexity());
	share_.frozen               = 1;
	for (uint32 i = ok && attachAll ? 1 : concurrency(); i != concurrency(); ++i) {
		if (!hasSolver(i)) { addSolver(); }
		if (!attach(i))    { return false; }
	}
	return ok || (detach(*master(), false), false);
}

bool SharedContext::attach(Solver& other) {
	assert(frozen() && other.shared_ == this);
	if (other.validVar(step_.var())) {
		if (!other.popRootLevel(other.rootLevel())){ return false; }
		if (&other == master())                    { return true;  }
	}
	initStats(other);
	// 1. clone vars & assignment
	Var lastVar = other.numVars();
	other.startInit(static_cast<uint32>(master()->constraints_.size()), configuration()->solver(other.id()));
	other.assign_.requestData(master()->assign_.numData());
	Antecedent null;
	for (LitVec::size_type i = 0, end = master()->trail().size(); i != end; ++i) {
		if (!other.force(master()->trail()[i], null)) { return false; }
	}
	for (Var v = satPrepro.get() ? lastVar+1 : varMax, end = master()->numVars(); v <= end; ++v) {
		if (eliminated(v) && other.value(v) == value_free) {
			other.assign_.eliminate(v);
		}
	}
	if (other.constraints_.empty()) { other.lastSimp_ = master()->lastSimp_; }
	// 2. clone & attach constraints
	if (!other.cloneDB(master()->constraints_)) {
		return false;
	}
	Constraint* c = master()->enumerationConstraint();
	other.setEnumerationConstraint( c ? c->cloneAttach(other) : 0 );
	// 3. endInit
	return (other.preparePost() && other.endInit()) || (detach(other, false), false);
}

void SharedContext::detach(Solver& s, bool reset) {
	assert(s.shared_ == this);
	if (reset) { s.reset(); }
	s.setEnumerationConstraint(0);
	s.popAuxVar();
}
void SharedContext::initStats(Solver& s) const {
	s.stats.enableStats(master()->stats);
	s.stats.reset();
}
const SolverStats& SharedContext::stats(const Solver& s, bool accu)  const {
	return !accu || s.id() >= accu_.size() || !accu_[s.id()] ? s.stats : *accu_[s.id()];
}
void SharedContext::accuStats() {
	accu_.resize(std::max(accu_.size(), solvers_.size()), 0);
	for (uint32 i = 0; i != solvers_.size(); ++i) {
		if (!accu_[i]) { accu_[i] = new SolverStats(); }
		accu_[i]->enableStats(solvers_[i]->stats);
		accu_[i]->accu(solvers_[i]->stats);
	}
	if (sccGraph.get()) { sccGraph->accuStats(); }
}

void SharedContext::simplify(bool shuffle) {
	Solver::ConstraintDB& db = master()->constraints_;
	if (concurrency() == 1 || master()->dbIdx_ == 0) {
		Clasp::simplifyDB(*master(), db, shuffle);
	}
	else {
		uint32 rem = 0;
		for (Solver::ConstraintDB::size_type i = 0, end = db.size(); i != end; ++i) {
			Constraint* c = db[i];
			if (c->simplify(*master(), shuffle)) { c->destroy(master(), false); db[i] = 0; ++rem; }
		}
		if (rem) {
			for (SolverVec::size_type s = 1; s != solvers_.size(); ++s) {
				Solver& x = *solvers_[s];
				CLASP_FAIL_IF(x.dbIdx_ > db.size(), "Invalid DB idx!");
				if      (x.dbIdx_ == db.size()) { x.dbIdx_ -= rem; }
				else if (x.dbIdx_ != 0)         { x.dbIdx_ -= (uint32)std::count_if(db.begin(), db.begin()+x.dbIdx_, IsNull()); }
			}
			db.erase(std::remove_if(db.begin(), db.end(), IsNull()), db.end());
		}
	}
	master()->dbIdx_ = db.size();
}
void SharedContext::removeConstraint(uint32 idx, bool detach) {
	Solver::ConstraintDB& db = master()->constraints_;
	CLASP_ASSERT_CONTRACT(idx < db.size());
	Constraint* c = db[idx];
	for (SolverVec::size_type s = 1; s != solvers_.size(); ++s) {
		Solver& x = *solvers_[s];
		x.dbIdx_ -= (idx < x.dbIdx_);
	}
	db.erase(db.begin()+idx);
	master()->dbIdx_ = db.size();
	c->destroy(master(), detach);
}

void SharedContext::simplifyShort(const Solver& s, Literal p) {
	if (!isShared() && p.index() < btig_.size()) { btig_.removeTrue(s, p); }
}

uint32 SharedContext::problemComplexity() const {
	if (isExtended()) {
		uint32 r = numBinary() + numTernary();
		for (uint32 i = 0; i != master()->constraints_.size(); ++i) {
			r += master()->constraints_[i]->estimateComplexity(*master());
		}
		return r;
	}
	return numConstraints();
}
/////////////////////////////////////////////////////////////////////////////////////////
// Distributor
/////////////////////////////////////////////////////////////////////////////////////////
Distributor::Distributor(const Policy& p) : policy_(p)  {}
Distributor::~Distributor() {}

}
