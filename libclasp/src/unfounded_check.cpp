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
#ifdef _MSC_VER
#pragma warning(disable : 4996) // std::copy was declared deprecated
#endif

#include <clasp/unfounded_check.h>
#include <clasp/clause.h>
#include <algorithm>

namespace Clasp { 

/////////////////////////////////////////////////////////////////////////////////////////
// class UfsNormal
//
// Body object for handling normal/choice rules
/////////////////////////////////////////////////////////////////////////////////////////
// Choice rules are handled like normal rules with one exception:
//  Since BFA does not apply to choice rules, we manually trigger the removal of source pointers
//  whenever an atom source by a choice rule becomes false.
//
class UfsNormal : public DefaultUnfoundedCheck::UfsBodyNode {
public:
	typedef DefaultUnfoundedCheck::UfsBodyNode UfsBodyNode;
	typedef DefaultUnfoundedCheck::UfsAtomNode UfsAtomNode;
	UfsNormal(PrgBodyNode* b, bool choice) : UfsBodyNode(b->literal(), b->scc()), circWeight_(b->posSize()), choice_(choice) {}
	bool init(const PrgBodyNode&, const AtomList&, DefaultUnfoundedCheck& ufs, UfsAtomNode** initList, uint32 len) {
		if (initList == preds) { circWeight_ = len; }
		else if (choice_ == 1) {
			// If a head of a choice rule becomes false, force removal of its source pointer.
			for (uint32 i = 0; succs[i] != 0; ++i) {
				ufs.addWatch(~succs[i]->lit, this, i);
			}
		}
		return circWeight_ == 0;
	}
	void onWatch(DefaultUnfoundedCheck& ufs, Literal, uint32 data) {
		assert(choice_ == 1);
		if (succs[data]->hasSource() && succs[data]->watch() == this) {
			assert(ufs.solver().isFalse(succs[data]->lit));
			ufs.enqueuePropagateUnsourced(succs[data]);
		}
	}
	bool isValidSource(const DefaultUnfoundedCheck&) { return circWeight_ == 0; }
	bool atomSourced(const UfsAtomNode&)             { return --circWeight_ == 0; }
	bool atomUnsourced(const UfsAtomNode&)           { return ++circWeight_ == 1; }
	void enqueueUnsourced(DefaultUnfoundedCheck& ufs){
		for (UfsAtomNode** p = preds; *p; ++p) {
			if ( !(*p)->hasSource() ) {
				ufs.enqueueFindSource(*p);
			}
		}
	}
	void doAddIfReason(DefaultUnfoundedCheck& ufs)   {
		if (ufs.solver().isFalse(lit)) {
			// body is only a reason if it does not depend on the atoms from the unfounded set
			for (UfsAtomNode** it = preds; *it; ++it) {
				if ( (*it)->typeOrUnfounded && !ufs.solver().isFalse((*it)->lit)) { return; }
			}
			ufs.addReasonLit(lit, this);
		}
	}
private:
	uint32  circWeight_ : 31;
	uint32  choice_     : 1;
};


/////////////////////////////////////////////////////////////////////////////////////////
// class UfsAggregate
//
// Body object for handling cardinality and weight rules.
/////////////////////////////////////////////////////////////////////////////////////////
// The major problems with card/weight-rules are:
//  1. subgoals can circularly depend on the body
//  2. subgoal false -> body false, does not hold
// 
// Regarding the first point, consider: {b}. a:- 1{a,b}.
// Since b is external to 1{a,b}, the body is a valid source for a. Therefore, 1{a,b} can source a.
// After a's source pointer is set to 1{a,b}, both subgoals of 1{a,b} have a source. Nevertheless,
// we must not count a because it (circularly) depends on the body. I.e. as soon as b 
// becomes false, a is unfounded, because the loop {a} no longer has external bodies.
//
// The second point means that we have to watch *all* subgoals because we 
// may need to trigger source pointer removal whenever one of the subgoals becomes false.
// Consider: {a,b,c}. t :- 2 {b,t,x}. x :- t,c. x :- a.
// Now assume {t,c} is true and a becomes false. In this case, both x and t lose their
// source pointer and we get the (conflicting) unfounded set {x, t}.
// Further assume that after some backtracking we have that both {t,c} and a
// become false. Therefore x is false, too. Since we do not update source pointers on
// conflicts, x and t still have no source. Thus no removal of source pointers is triggered.
// If we would not watch x in 2 {b,t,x}, we could not add t to the todo queue and 
// we would therefore miss the unfounded set {t}.
//
// The implementation works as follows:
// - It distinguishes between internal literals, those that are in the same SCC as B
// and external literals, those that are not.
// - Let W(l) be the weight of literal l in B and W(WS) be the sum of W(l) for each l in a set WS. 
// - The goal is to maintain a set WS of literals l, s.th l in Body and hasSource(l) AND
// W(WS) >= bound.
// - Initially WS contains all non-false external literals of B.
// - Whenever one of the internal literals of B becomes sourced, it is added to WS 
// *only if* W(WS) < bound. In that case, it is guaranteed that the literal
// currently does not circularly depend on the body.
// - As soon as W(WS) >= bound, we declare the body as valid source for its heads.
// - Whenever one of the literals l in WS loses its source, it is removed from WS.
// If l is an external literal, new valid external literals are searched and added to WS 
// until the source condition holds again.
// If the condition cannot be restored, the body is marked as invalid source.
class UfsAggregate : public DefaultUnfoundedCheck::UfsBodyNode {
public:
	typedef DefaultUnfoundedCheck::UfsBodyNode UfsBodyNode;
	typedef DefaultUnfoundedCheck::UfsAtomNode UfsAtomNode;
	UfsAggregate(PrgBodyNode* b);
	~UfsAggregate();
	bool init(const PrgBodyNode& self, const AtomList& atoms, DefaultUnfoundedCheck& ufs, UfsAtomNode** initList, uint32 len);	
	bool isValidSource(const DefaultUnfoundedCheck&);
	bool atomSourced(const UfsAtomNode&);
	bool atomUnsourced(const UfsAtomNode&);
	void doAddIfReason(DefaultUnfoundedCheck& ufs);
	void onWatch(DefaultUnfoundedCheck& ufs, Literal, uint32 data);
	void enqueueUnsourced(DefaultUnfoundedCheck& ufs){
		for (UfsAtomNode** p = preds; *p; ++p) {
			if ( !predMarked(*p) && !(*p)->hasSource() ) {
				ufs.enqueueFindSource(*p);
			}
		}
	}
private:
	// struct for holding external literals and (optionally) weights.
	// if weights_ == 0, data_ corresponds to Literal lits[size_];
	// Otherwise, data_ resembles
	// struct WeightExtra { 
	//   WeightLiteral lits[size_];
	//   weight_t      predWeights[preds.size()];
	// };
	struct External {
		External(uint32 size, bool weights) : size_(size), weights_(weights) {}
		Literal  lit(uint32 idx)    const { return Literal::fromRep(data_[idx<<weights_]); }
		// External literals in WS have their watch flag set.
		void     watchLiteral(uint32 idx) { store_set_bit(data_[idx<<weights_], 0); }
		void     clearWatch(uint32 idx)   { store_clear_bit(data_[idx<<weights_], 0);  }
		weight_t weight(uint32 idx) const {   // returns the weight of external literal idx
			return weights_ != 0 
				? weight_t(data_[(idx<<1)+1])
				: weight_t(1);
		}
		weight_t predWeight(uint32 idx) const {// returns the weight of internal literal idx
			return weights_ != 0
				? weight_t(data_[(size_<<1)+idx])
				: weight_t(1);
		}
		uint32 size_   : 31; // number of external literals
		uint32 weights_:  1; // does the body contain weights > 1?
    uint32 data_[0];     // external literals and (optionally) weights
	};
	void initExternal(const PrgBodyNode& self, const AtomList& atoms, DefaultUnfoundedCheck& ufs);
	// Adds the literal at index litIdx to WS and updates W(WS).
	// If litIdx is an external literal, its watch-flag is set.
	// If it is an internal literal, we mark the literal by setting the LSB
	// of the corresponding UfsAtomNode pointer.
	void addToWS(uint32 litIdx, weight_t w, bool external) {
		if (external) {
			assert(ext_->lit(litIdx).watched() == false);
			ext_->watchLiteral(litIdx);
		}
		else {
			assert(predMarked(preds[litIdx]) == false);
			preds[litIdx] = markPred(preds[litIdx]);
		}
		lower_ -= w;
	}
	// Removes the literal at index litIdx from WS and updates W(WS).
	void removeFromWS(uint32 litIdx, weight_t w, bool external) {
		if (external) {
			assert(ext_->lit(litIdx).watched() == true);
			ext_->clearWatch(litIdx);
		}
		else {
			assert(predMarked(preds[litIdx]) == true);
			preds[litIdx] = unmarkPred(preds[litIdx]);
		}
		lower_ += w;
	}
	// Returns the index of a in the preds array
	// Pre: a is a predecessor of this body
	uint32 findPred(const UfsAtomNode& a) const {
		uint32 idx = 0;
		// a *must* be there. Otherwise something is horribly wrong!
		while (unmarkPred(preds[idx]) != &a) { ++idx; }
		return idx;
	}
	bool         predMarked(UfsAtomNode* p) const { return test_bit(uintp(p), 0); }
	UfsAtomNode* markPred(UfsAtomNode* p)   const { return (UfsAtomNode*)set_bit(uintp(p), 0); }
	UfsAtomNode* unmarkPred(UfsAtomNode* p) const { return (UfsAtomNode*)clear_bit(uintp(p), 0);} 
	weight_t  lower_; // Stores bound - W(WS); if <= 0, body can source its heads
	External* ext_;   // external literals and (optionally) weights
};

UfsAggregate::UfsAggregate(PrgBodyNode* b)
	: UfsBodyNode(b->literal(), b->scc())
	, lower_(b->bound())
	, ext_(0) {
	typeOrUnfounded = 1;
}
UfsAggregate::~UfsAggregate() {
	if (ext_) { 
		::operator delete(ext_); ext_ = 0; 
	}
}

bool UfsAggregate::init(const PrgBodyNode& self, const AtomList& prgAtoms, DefaultUnfoundedCheck& ufs, UfsAtomNode** initList, uint32 len) {
	if (initList == preds) {
		// compute and allocate necessary storage to hold external literals
		uint32 otherScc = 0;
		for (uint32 p = 0; p != self.posSize(); ++p) {
			PrgAtomNode* pred = prgAtoms[self.pos(p)];
			if (pred->scc() != self.scc() && !ufs.solver().isFalse(pred->literal())) {
				++otherScc;
			}
		}
		for (uint32 n = 0; n != self.negSize(); ++n) {
			otherScc += !ufs.solver().isTrue(prgAtoms[self.neg(n)]->literal());
		}
		uint32 space = sizeof(External) + otherScc*sizeof(Literal);
		if (self.rtype() == WEIGHTRULE) {
			space += (otherScc*sizeof(weight_t));
			space += (len*sizeof(weight_t));
		}
		void* m = ::operator new(space);
		ext_ = new (m) External(otherScc, self.rtype() == WEIGHTRULE);
		// init aggregate
		initExternal(self, prgAtoms, ufs);
	}
	return lower_ <= 0;
}

// Initialize external and populate WS with external literals.
// Set lower_ to body.bound() - W(WS)
void UfsAggregate::initExternal(const PrgBodyNode& body, const AtomList& prgAtoms, DefaultUnfoundedCheck& ufs) {
	uint32 idx = 0, wIdx = 0;
	lower_ = body.bound();
	for (uint32 p = 0; p != body.posSize(); ++p) {
		PrgAtomNode* pred = prgAtoms[body.pos(p)];
		if (ufs.solver().isFalse(pred->literal())) { continue; }
		weight_t w = body.weight(p, true);
		ufs.addWatch(~pred->literal(), this, (idx<<1) + (pred->scc()!=body.scc()));
		if (pred->scc() != body.scc()) {
			if (ext_->weights_ != 0) {
				ext_->data_[idx << 1]   = pred->literal().asUint();
				ext_->data_[(idx<<1)+1] = (uint32)w;
			}
			else {
				ext_->data_[idx]        = pred->literal().asUint();
			}
			addToWS(idx,w,true);
			++idx;
		}
		else if (ext_->weights_!=0) {
			ext_->data_[(ext_->size_<<1)+wIdx++] = (uint32)w;
		}
	}
	for (uint32 n = 0; n != body.negSize(); ++n) {
		PrgAtomNode* pred = prgAtoms[body.neg(n)];
		if (pred->hasVar() && !ufs.solver().isTrue(pred->literal())) {
			weight_t w = body.weight(n, false);
			if (ext_->weights_) {
				ext_->data_[idx<<1]     = (~pred->literal()).asUint();
				ext_->data_[(idx<<1)+1] = (uint32)w;
			}
			else {
				ext_->data_[idx]        = (~pred->literal()).asUint();
			}
			addToWS(idx,w,true);
			ufs.addWatch(pred->literal(), this, (idx<<1)+1);
			++idx;
		}
	}
}

bool UfsAggregate::atomSourced(const UfsAtomNode& a) {
	if (lower_ <= 0) {
		// body is already a valid source: do not add further atoms
		// to WS because we cannot be sure that they don't
		// depend on this body.
		return false;
	}
	uint32 idx = findPred(a);
	addToWS(idx, ext_->predWeight(idx), false);
	return lower_ <= 0;
}

bool UfsAggregate::atomUnsourced(const UfsAtomNode& a)  { 
	bool wasSource = lower_ <= 0;
	uint32 idx = findPred(a);
	if (predMarked(preds[idx])) {
		removeFromWS(idx, ext_->predWeight(idx), false);
		// At this point we could try to extend WS with
		// some external literals. Our internal literals, on the other hand,
		// must be assumed as unsourced because they may now depend on the body.
	}
	return wasSource && lower_ > 0;
}

bool UfsAggregate::isValidSource(const DefaultUnfoundedCheck& ufs) {
	if (lower_ <= 0) return true;
	// Since *this is currently not a source, 
	// we here know that no literal with a source can depend on this body.
	// Hence, we can safely add all those literals to WS.
	
	// We check all external literals here because we do not update
	// the body on backtracking. Therefore some external literals that were false
	// may now be true/free.
	for (uint32 i = 0; i != ext_->size_; ++i) {
		Literal li = ext_->lit(i);
		if (!li.watched() && !ufs.solver().isFalse(li)) {
			addToWS(i, ext_->weight(i), true);
		}
	}
	
	// We check all internal literals here because there may be atoms
	// that were sourced *after* we established the watch set.
	for (uint32 i = 0; preds[i] != 0; ++i) {
		UfsAtomNode* a = preds[i];
		if (!predMarked(a) && a->hasSource() && !ufs.solver().isFalse(a->lit)) {
			addToWS(i, ext_->predWeight(i), false);
		}
	}
	return lower_ <= 0;
}

void UfsAggregate::doAddIfReason(DefaultUnfoundedCheck& ufs) {
	if (ufs.solver().isFalse(lit)) {
		// The body is false and therefore possibly part of the reason for an unfounded set.
		// Now check if the body depends on the atoms from the unfounded set. I.e.
		// would the body still be false if all but its unfounded literals would be true?
		weight_t tempLower = lower_;
		for (uint32 i = 0; i != ext_->size_; ++i) {
			Literal li = ext_->lit(i);
			if (!li.watched() && (tempLower-=ext_->weight(i)) <= 0) {
				ufs.addReasonLit(lit, this);
				return;
			}
		}
		for (uint32 i = 0; preds[i] != 0; ++i) {
			UfsAtomNode* a = preds[i];
			if (!predMarked(a) && (!a->typeOrUnfounded || ufs.solver().isFalse(a->lit))) { 
				if ( (tempLower-=ext_->predWeight(i)) <= 0 ) {
					ufs.addReasonLit(lit, this);
					return;
				}
			}
		}
	}
	else {  // body is neither false nor a valid source - add all false lits to reason set
		for (uint32 i = 0; i != ext_->size_; ++i) {
			if (ufs.solver().isFalse(ext_->lit(i))) {  ufs.addReasonLit(ext_->lit(i), this); }
		}
		for (UfsAtomNode** z = preds; *z; ++z) {
			assert(!predMarked(*z) || !ufs.solver().isFalse(unmarkPred(*z)->lit));
			if (!predMarked(*z) && ufs.solver().isFalse((*z)->lit)) { ufs.addReasonLit((*z)->lit,this); }
		}
	}
}

void UfsAggregate::onWatch(DefaultUnfoundedCheck& ufs, Literal l, uint32 idx) {
	assert((idx&1) == 0 || ~l == ext_->lit(idx>>1)); (void)l;
	// If l is external and false...
	if (test_bit(idx, 0) && ext_->lit(idx=(idx>>1)).watched()) {
		// ...and currently part of our watch set: remove it.
		removeFromWS(idx, ext_->weight(idx), true);
		// If the set can no longer source our heads: try to extend it.
		for (uint32 i = 0; i != ext_->size_ && lower_ > 0; ++i) {
			if (!ext_->lit(i).watched() && !ufs.solver().isFalse(ext_->lit(i))) {
				addToWS(i, ext_->weight(i), true);
			}
		}
	}
	if (lower_ > 0 && watches > 0) {
		// The body is not a valid source but at least one head atom 
		// still depends on it:  mark body as invalid source
		ufs.enqueueInvalid(this);	
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// DefaultUnfoundedCheck - Construction/Destruction
/////////////////////////////////////////////////////////////////////////////////////////
DefaultUnfoundedCheck::DefaultUnfoundedCheck(ReasonStrategy r)
	: solver_(0) 
	, reasons_(0)
	, activeClause_(0)
	, strategy_(r) {
}
DefaultUnfoundedCheck::~DefaultUnfoundedCheck() { clear(); }

void DefaultUnfoundedCheck::clear() {
	std::for_each( bodies_.begin(), bodies_.end(), DeleteObject());
	std::for_each( atoms_.begin(), atoms_.end(), DeleteObject() );

	atoms_.clear();
	bodies_.clear();
	invalid_.clear();
	watches_.clear();
	delete activeClause_;
	delete [] reasons_;
	reasons_ = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
// DefaultUnfoundedCheck - Base interface
/////////////////////////////////////////////////////////////////////////////////////////
void DefaultUnfoundedCheck::reset() {
	assert(loopAtoms_.empty());
	while (!todo_.empty()) {
		todo_.back()->pickedOrTodo = 0;
		todo_.pop_back();
	}
	while (!unfounded_.empty()) {
		unfounded_.back()->typeOrUnfounded = 0;
		unfounded_.pop_back();
	}
	while (!sourceQueue_.empty()) {
		sourceQueue_.back()->resurrectSource();
		sourceQueue_.pop_back();
	}
	invalid_.clear();
	activeClause_->clear();
}

bool DefaultUnfoundedCheck::propagateFixpoint(Solver&) {
	return solver_->strategies().search != SolverStrategies::no_learning 
		? assertAtom()
		: assertSet();
}

/////////////////////////////////////////////////////////////////////////////////////////
// DefaultUnfoundedCheck - Constraint interface
/////////////////////////////////////////////////////////////////////////////////////////
// a (relevant) literal was assigned. Check which node is affected and update it accordingly
Constraint::PropResult DefaultUnfoundedCheck::propagate(const Literal& p, uint32& data, Solver& s) {
	assert(&s == solver_); (void)s;
	uint32 index  = data >> 1;
	uint32 type   = data & 1;
	if (type == 0) {
		// a normal body became false - mark as invalid if necessary
		assert(index < bodies_.size());
		if (bodies_[index]->watches > 0) { invalid_.push_back(bodies_[index]); }
	}
	else {
		// literal is associated with a watch - notify the object 
		assert(index < watches_.size());
		watches_[index].notify(p, this);
	}
	return PropResult(true, true);  // always keep the watch
}
// only used if strategy_ == only_reason
void DefaultUnfoundedCheck::reason(const Literal& p, LitVec& r) {
	assert(strategy_ == only_reason && reasons_);
	r.assign(reasons_[p.var()-1].begin(), reasons_[p.var()-1].end());
}

/////////////////////////////////////////////////////////////////////////////////////////
// DefaultUnfoundedCheck / Dependency graph initialization
/////////////////////////////////////////////////////////////////////////////////////////
void DefaultUnfoundedCheck::startInit(Solver& s) {
	assert(!solver_ || solver_ == &s);
	solver_ = &s;
	if (!activeClause_) { activeClause_ = new ClauseCreator(solver_); }
}

// Creates a positive-body-atom-dependency graph (PBADG) for the previously added SCCs.
// The PBADG contains a node for each atom A of a non-trivial SCC and
// a node for each body B, s.th. there is a non-trivially connected atom A with
// B in body(A).
// Pre : a->ignore  = 0 for all new and relevant atoms a
// Pre : b->visited = 1 for all new and relevant bodies b
// Post: a->ignore  = 1 for all atoms that were added to the PBADG
// Post: b->visited = 0 for all bodies that were added to the PBADG
bool DefaultUnfoundedCheck::endInit(const AtomList& sccAtoms, const AtomList& prgAtoms, const BodyList& prgBodies) {
	assert(solver_ && "DefaultUnfoundedCheck::startInit() not called!");
	assert(sccAtoms.size() <= prgAtoms.size());
	atoms_.reserve(atoms_.size()+sccAtoms.size()+1);
	// Pass 1: Create ufs atom nodes
	for (AtomList::size_type i = 0, end = sccAtoms.size(); i != end; ++i) {
		PrgAtomNode* a = sccAtoms[i];
		if (relevantPrgAtom(a)) {
			// create corresponding UfsAtomNode
			atoms_.push_back( new UfsAtomNode(a->literal(), a->scc()) );
			// store link between program node and ufs node for later lookup
			a->setUfsNode((uint32)atoms_.size()-1, true);
			// atom is defined by more than just a bunch of clauses
			solver_->setFrozen(a->var(), true);
		}
	}
	// Pass 2: Init atom nodes and create body nodes
	for (AtomList::size_type i = 0, end = sccAtoms.size(); i != end; ++i) {
		PrgAtomNode* a = sccAtoms[i];
		if (relevantPrgAtom(a)) { initAtom(*atoms_[a->ufsNode()], *a, prgAtoms, prgBodies); }
	}
	// Pass 3: Check for initially unfounded atoms
	propagateSource();
	for (AtomList::size_type i = 0, end = sccAtoms.size(); i != end; ++i) {
		PrgAtomNode* a = sccAtoms[i];
		if (relevantPrgAtom(a)) { 
			UfsAtomNode* u = atoms_[a->ufsNode()];
			if (!solver_->isFalse(u->lit)
				&& !u->hasSource()
				&& !solver_->force(~u->lit, 0)) {
				return false;
			}
		}
	}
	return true;
}


DefaultUnfoundedCheck::UfsAtomNode* DefaultUnfoundedCheck::getAtom(PrgAtomNode* h) {
	assert(h->ufsNode() < (uint32)atoms_.size());
	return atoms_[ h->ufsNode() ]; 
}

void DefaultUnfoundedCheck::initAtom(UfsAtomNode& an, const PrgAtomNode& a, const AtomList& prgAtoms, const BodyList& prgBodies) {
	BodyVec bodies;
	for (VarVec::const_iterator it = a.preds.begin(), end = a.preds.end(); it != end; ++it) {
		PrgBodyNode* prgBody = prgBodies[*it];
		if (relevantPrgBody(prgBody)) {
			UfsBodyNode* bn = addBody(prgBody, prgAtoms);
			BodyVec::iterator insPos = bn->scc == an.scc ? bodies.end() : bodies.begin();
			bodies.insert(insPos, bn);
		}
	}
	an.preds = new UfsBodyNode*[bodies.size()+1];
	std::copy(bodies.begin(), bodies.end(), an.preds);
	an.preds[bodies.size()] = 0;
	bodies.clear();
	for (VarVec::const_iterator it = a.posDep.begin(), end = a.posDep.end(); it != end; ++it) {
		PrgBodyNode* prgBody = prgBodies[*it];
		if (prgBody->scc() == a.scc() && relevantPrgBody(prgBody)) {
			bodies.push_back(addBody(prgBody, prgAtoms));
		}
	}
	an.succs = new UfsBodyNode*[bodies.size()+1];
	std::copy(bodies.begin(), bodies.end(), an.succs);
	an.succs[bodies.size()] = 0;
	bodies.clear();
}

// if necessary, creates and initializes a new body node and adds it to the PBADG
DefaultUnfoundedCheck::UfsBodyNode* DefaultUnfoundedCheck::addBody(PrgBodyNode* b, const AtomList& prgAtoms) {
	if (b->visited()) {       // first time we see this body - 
		b->setVisited(false);   // mark as visited and create node of
		RuleType rt = b->rtype();// suitable type
		UfsBodyNode* bn;
		if (b->scc() == PrgNode::noScc || rt == BASICRULE || rt == CHOICERULE) {
			bn = new UfsNormal(b, rt == CHOICERULE);
		}
		else if (rt == CONSTRAINTRULE || rt == WEIGHTRULE)  { 
			bn = new UfsAggregate(b); 
		}
		else { assert(false && "Unknown body type!"); bn = 0; }
		bodies_.push_back( bn );
		b->setUfsNode((uint32)bodies_.size() - 1, false);
		solver_->addWatch(~b->literal(), this, ((uint32)bodies_.size() - 1)<<1);
		// Init node
		AtomVec atoms;
		for (uint32 p = 0; p != b->posSize(); ++p) {
			PrgAtomNode* pred = prgAtoms[b->pos(p)];
			if (relevantPrgAtom(pred) && pred->scc() == bn->scc) {
				atoms.push_back( getAtom(pred) );
			}
		}
		atoms.push_back(0);
		bn->preds       = new UfsAtomNode*[atoms.size()];
		std::copy(atoms.begin(), atoms.end(), bn->preds);
		bool ext = bn->init(*b, prgAtoms, *this, bn->preds, (uint32)atoms.size()-1);  // init preds
		atoms.clear();
		for (VarVec::const_iterator it = b->heads.begin(), end = b->heads.end(); it != end; ++it) {
			PrgAtomNode* a = prgAtoms[*it];
			if (relevantPrgAtom(a)) {
				UfsAtomNode* an = getAtom(a);
				AtomVec::iterator insPos = bn->scc == an->scc ? atoms.begin() : atoms.end();
				atoms.insert(insPos, an);
				if (!an->hasSource() && (a->scc() != b->scc() || ext) ) {
					an->updateSource(bn);
					sourceQueue_.push_back(an);
				}
			}
		}
		atoms.push_back(0);
		bn->succs       = new UfsAtomNode*[atoms.size()];
		std::copy(atoms.begin(), atoms.end(), bn->succs);
		bn->init(*b, prgAtoms, *this, bn->succs, (uint32)atoms.size()-1); // init succs
		// exclude body from SAT-based preprocessing
		solver_->setFrozen(b->var(), true);
	}
	return bodies_[ b->ufsNode() ];
}

/////////////////////////////////////////////////////////////////////////////////////////
// DefaultUnfoundedCheck - finding unfounded sets
/////////////////////////////////////////////////////////////////////////////////////////
bool DefaultUnfoundedCheck::findUnfoundedSet() {
	// first: remove all sources that were recently falsified
	if (!sourceQueue_.empty()) {
		propagateSource(true);
	}
	for (VarVec::size_type i = 0; i != invalid_.size(); ++i) { 
		removeSource(invalid_[i]); 
	}
	invalid_.clear();
	assert(sourceQueue_.empty() && unfounded_.empty());
	// second: try to re-establish sources.
	while (!todo_.empty()) {
		UfsAtomNode* head = dequeueTodo();
		assert(head->scc != PrgNode::noScc );
		if (!head->hasSource() && !solver_->isFalse(head->lit) && !findSource(head)) {
			return true;  // found an unfounded set - contained in unfounded_
		}
		assert(sourceQueue_.empty());
	}
	return false;     // no unfounded sets
}

// searches a new source for the atom node head.
// If a new source is found the function returns true.
// Otherwise the function returns false and unfounded_ contains head
// as well as atoms with no source that circularly depend on head.
bool DefaultUnfoundedCheck::findSource(UfsAtomNode* head) {
	assert(unfounded_.empty());
	enqueueFindSource(head);  // unfounded, unless we find a new source
	Queue noSourceYet;
	bool changed = false;
	while (!unfounded_.empty()) {
		head = unfounded_.front();
		if (!head->hasSource()) { // no source
			unfounded_.pop_front(); // note: current atom is still marked 
			UfsBodyNode** it  = head->preds;
			for (; *it != 0; ++it) {
				if (!solver_->isFalse((*it)->lit )) {
					if ((*it)->scc != head->scc || (*it)->isValidSource(*this)) {
						head->typeOrUnfounded = 0;  // found a new source,    
						setSource(*it, head);       // set the new source
						propagateSource();          // and propagate it forward
						changed = true;             // may source atoms in noSourceYet!
						break;
					}
					else {
						(*it)->enqueueUnsourced(*this);
					}
				}
			}
			if (!head->hasSource()) {
				noSourceYet.push_back(head);// no source found
			}
		}
		else {  // head has a source
			dequeueUnfounded();
		}
	} // while unfounded_.emtpy() == false
	std::swap(noSourceYet, unfounded_);
	if (changed) {
		// remove all atoms that have a source as they are not unfounded
		Queue::iterator it, j = unfounded_.begin();
		for (it = unfounded_.begin(); it != unfounded_.end(); ++it) {
			if ( (*it)->hasSource() )   { (*it)->typeOrUnfounded = 0; }
			else                        { *j++ = *it; }
		}
		unfounded_.erase(j, unfounded_.end());
	}
	return unfounded_.empty();
}

// sets body as source for head if necessary.
// PRE: value(body) != value_false
// POST: source(head) != 0
void DefaultUnfoundedCheck::setSource(UfsBodyNode* body, UfsAtomNode* head) {
	assert(!solver_->isFalse(body->lit));
	// For normal rules from not false B follows not false head, but
	// for choice rules this is not the case. Therefore, the 
	// check for isFalse(head) is needed so that we do not inadvertantly
	// source a head that is currently false.
	if (!head->hasSource() && !solver_->isFalse(head->lit)) {
		head->updateSource(body);
		sourceQueue_.push_back(head);
	}
}

// For each successor h of body sets source(h) to 0 if source(h) == body.
// If source(h) == 0, adds h to the todo-Queue.
// This function is called for each body that became invalid during propagation.
void DefaultUnfoundedCheck::removeSource(UfsBodyNode* body) {
	for (UfsAtomNode** it = body->succs; *it; ++it) {
		UfsAtomNode* head = *it;
		if (head->watch() == body) {
			if (head->hasSource()) {
				head->markSourceInvalid();
				sourceQueue_.push_back(head);
			}
			enqueueTodo(head);
		}
	}
	propagateSource();
} 


// propagates recently set source pointers within one strong component.
void DefaultUnfoundedCheck::propagateSource(bool forceTodo) {
	for (LitVec::size_type i = 0; i < sourceQueue_.size(); ++i) {
		UfsAtomNode* head = sourceQueue_[i];
		UfsBodyNode** it  = head->succs;
		if (head->hasSource()) {
			// propagate a newly added source-pointer
			for (; *it; ++it) {
				if ((*it)->atomSourced(*head) && !solver_->isFalse((*it)->lit)) {
					for (UfsAtomNode** aIt = (*it)->succs; *aIt; ++aIt) {
						setSource(*it, *aIt);
					}
				}
			}
		}
		else {
			// propagate the removal of a source-pointer within this scc
			for (; *it; ++it) {
				// For a normal body there are two cases whenever it ceases to be valid:
				// a) all predecessor have their source restored and the body becomes a valid source again
				// b) at least one predecessor is unfounded, hence false. In that case, the body
				// becomes false, too and all its successors having the body as source are enqueued.
				// Alas, case b) does not necessarily hold for extended rules. I.e. a false predecessor
				// does not automatically make the body false. Hence, we eagerly enqueue all successors.
                bool addTodo = forceTodo || ((*it)->typeOrUnfounded == 1 && !solver_->isFalse((*it)->lit));

				if ((*it)->atomUnsourced(*head)) {
					for (UfsAtomNode** aIt = (*it)->succs; *aIt && (*aIt)->scc == (*it)->scc; ++aIt) {
						if ((*aIt)->hasSource() && (*aIt)->watch() == *it) {
							(*aIt)->markSourceInvalid();
							sourceQueue_.push_back(*aIt);
						}
						if ( addTodo && (*aIt)->watch() == *it) {
							enqueueTodo(*aIt);
						}
					}
				}
			}
		}
	}
	sourceQueue_.clear();
}


/////////////////////////////////////////////////////////////////////////////////////////
// DefaultUnfoundedCheck - Propagating unfounded sets
/////////////////////////////////////////////////////////////////////////////////////////
// asserts all atoms of the unfounded set, then propagates
bool DefaultUnfoundedCheck::assertSet() {
	activeClause_->clear();
	while (findUnfoundedSet()) {
		while (!unfounded_.empty() && solver_->force(~unfounded_[0]->lit, 0)) {
			dequeueUnfounded();
		}
		if (!unfounded_.empty() || !solver_->propagateUntil(this)) {
			return false;
		}
	}
	return true;
}

// as long as an unfounded set U is not empty,
// - asserts the first non-false atom
// - propagates
// - removes the atom from U
bool DefaultUnfoundedCheck::assertAtom() {
	while(findUnfoundedSet()) {
		activeClause_->clear();
		while (!unfounded_.empty()) {
			if (!solver_->isFalse(unfounded_[0]->lit) && !assertAtom(unfounded_[0]->lit)) {
				return false;
			}
			dequeueUnfounded();
		}
		if (!loopAtoms_.empty()) {
			createLoopFormula();
		}
	}
	return true;
}

// asserts an unfounded atom using the selected reason strategy
bool DefaultUnfoundedCheck::assertAtom(Literal a) {
	bool conflict = solver_->isTrue(a);
	if (strategy_ == distinct_reason || activeClause_->empty()) {
		// first atom of unfounded set or distinct reason for each atom requested.
		activeClause_->startAsserting(Constraint_t::learnt_loop, conflict ? a : ~a);
		invSign_ = conflict || strategy_ == only_reason;
		computeReason();
		if (conflict) {
			LitVec cfl;
			activeClause_->swap(cfl);
			assert(!cfl.empty());
			solver_->setConflict( cfl );
			return false;
		}
		else if (strategy_ == only_reason) {
			if (reasons_ == 0) reasons_ = new LitVec[solver_->numVars()];
			reasons_[a.var()-1].assign(activeClause_->lits().begin()+1, activeClause_->lits().end());
			solver_->force( ~a, this );
		}
		else if (strategy_ != shared_reason || activeClause_->size() < 4) {
			activeClause_->end();
		}
		else {
			loopAtoms_.push_back(~a);
			solver_->force(~a, 0);
		}
	}
	// subsequent atoms
	else if (conflict) {
		if (!loopAtoms_.empty()) {
			createLoopFormula();
		}
		if (strategy_ == only_reason) {
			// we already have the reason for the conflict, only add a
			LitVec cfl;
			(*activeClause_)[0] = a;
			activeClause_->swap(cfl);
			solver_->setConflict( cfl );
			return false;
		}
		else {
			// we have a clause - create reason for conflict by inverting literals
			LitVec cfl;
			cfl.push_back(a);
			for (LitVec::size_type i = 1; i < activeClause_->size(); ++i) {
				if (~(*activeClause_)[i] != a) {
					cfl.push_back( ~(*activeClause_)[i] );
				}
			}
			solver_->setConflict( cfl );
			return false;
		}
	}
	else if (strategy_ == only_reason) {
		reasons_[a.var()-1].assign(activeClause_->lits().begin()+1, activeClause_->lits().end());
		solver_->force( ~a, this );
	}
	else if (strategy_ != shared_reason || activeClause_->size() < 4) {
		(*activeClause_)[0] = ~a;
		activeClause_->end();
	}
	else {
		solver_->force(~a, 0);
		loopAtoms_.push_back(~a);
	}
	if ( (conflict = !solver_->propagateUntil(this)) == true && !loopAtoms_.empty()) {
		createLoopFormula();
	}
	return !conflict;
}

void DefaultUnfoundedCheck::createLoopFormula() {
	assert(activeClause_->size() > 3);
	if (loopAtoms_.size() == 1) {
		(*activeClause_)[0] = loopAtoms_[0];
		Constraint* ante;
		activeClause_->end(&ante);
		assert(ante != 0 && solver_->isTrue(loopAtoms_[0]) && solver_->reason(loopAtoms_[0]).isNull());
		const_cast<Antecedent&>(solver_->reason(loopAtoms_[0])) = ante;
	}
	else {
		LoopFormula* lf = LoopFormula::newLoopFormula(*solver_, &(*activeClause_)[1], (uint32)activeClause_->size() - 1, (uint32)activeClause_->secondWatch()-1, (uint32)loopAtoms_.size()); 
		solver_->addLearnt(lf, lf->size());
		for (VarVec::size_type i = 0; i < loopAtoms_.size(); ++i) {
			assert(solver_->isTrue(loopAtoms_[i]) && solver_->reason(loopAtoms_[i]).isNull());
			const_cast<Antecedent&>(solver_->reason(loopAtoms_[i])) = lf;
			lf->addAtom(loopAtoms_[i], *solver_);
		}
		lf->updateHeuristic(*solver_);
	}
	loopAtoms_.clear();
}


// computes the reason why a set of atoms is unfounded
void DefaultUnfoundedCheck::computeReason() {
	uint32 ufsScc = unfounded_[0]->scc;
	for (LitVec::size_type i = 0; i < unfounded_.size(); ++i) {
		assert(ufsScc == unfounded_[i]->scc);
		if (!solver_->isFalse(unfounded_[i]->lit)) {
			UfsAtomNode* a = unfounded_[i];
			for (UfsBodyNode** it = a->preds; *it != 0; ++it) {
				if ((*it)->pickedOrTodo == 0) {
					picked_.push_back(*it);
					(*it)->pickedOrTodo = 1;
					(*it)->addIfReason(*this, ufsScc);
				}
			}
		}
	} 
	assert( (activeClause_->size() > 1
		? solver_->level((*activeClause_)[activeClause_->secondWatch()].var()) == solver_->decisionLevel()
		: solver_->decisionLevel() == 0)
		&& "Loop nogood must contain a literal from current DL!");
	for (BodyVec::iterator it = picked_.begin(); it != picked_.end(); ++it) {
		(*it)->pickedOrTodo = false;
		if (solver_->level((*it)->lit.var()) > 0) {
			solver_->clearSeen((*it)->lit.var());
		}
	}
	for (LitVec::iterator it = pickedAtoms_.begin(); it != pickedAtoms_.end(); ++it) {
		solver_->clearSeen(it->var());
	}
	picked_.clear();
	pickedAtoms_.clear();
	double ratio = activeClause_->size()/double(solver_->decisionLevel()+1);
	if (ratio > 10 && activeClause_->size() > 100 && !solver_->isFalse((*activeClause_)[0])) {
		Literal a = (*activeClause_)[0];
		activeClause_->startAsserting(Constraint_t::learnt_loop, a);
		for (uint32 i = 1; i <= solver_->decisionLevel(); ++i) {
			activeClause_->add(~solver_->decision(i));
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// DefaultUnfoundedCheck - Helpers
/////////////////////////////////////////////////////////////////////////////////////////
void DefaultUnfoundedCheck::addWatch(Literal p, UfsBodyNode* b, uint32 data) {
	uint32 d = (static_cast<uint32>(watches_.size())<<1) + 1;
	watches_.push_back(Watch(b, data));
	solver_->addWatch(p, this, d);
}

void DefaultUnfoundedCheck::addReasonLit(Literal p, const UfsBodyNode* reason) {
	if (!solver_->seen(p)) {
		solver_->markSeen(p);
		if (p != reason->lit) { pickedAtoms_.push_back(p); }
		if (!invSign_ || (p = ~p) != activeClause_->lits().front()) {
			activeClause_->add(p);
		}
	}
}
}
