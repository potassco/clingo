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
#pragma warning (disable : 4996) // 'std::_Fill_n' was declared deprecated
#endif
#include <clasp/logic_program_types.h>
#include <clasp/logic_program.h>
#include <clasp/solver.h>
#include <clasp/clause.h>
#include <clasp/weight_constraint.h>
#include <clasp/util/misc_types.h>

#include <deque>

namespace Clasp { namespace Asp {

/////////////////////////////////////////////////////////////////////////////////////////
// class Rule
//
// Code for handling a rule of a logic program
/////////////////////////////////////////////////////////////////////////////////////////
void Rule::clear() {
	heads.clear();
	body.clear();
	bound_  = 0;
	type_   = ENDRULE;
}

// Adds atomId as head to this rule.
Rule& Rule::addHead(Var atomId) {
	assert(type_ != ENDRULE && "Invalid operation - Start rule not called");
	heads.push_back(atomId);
	return *this;
}

// Adds atomId to the body of this rule. If pos is true, atomId is added to B+
// otherwise to B-. The weight is ignored (set to 1) unless the rule is a weight/optimize rule
Rule& Rule::addToBody(Var atomId, bool pos, weight_t weight) {
	assert(type_ != ENDRULE && "Invalid operation - Start rule not called");
	assert( weight >= 0 && "Invalid weight - negative weights are not supported");
	if (weight == 0) return *this;  // ignore weightless atoms
	if (weight != 1 && !bodyHasWeights()) {
		weight = 1;
	}
	body.push_back(WeightLiteral(Literal(atomId, pos == false), weight));
	return *this;
}

uint32 BodyInfo::findLit(Literal x) const {
	for (WeightLitVec::const_iterator it = lits.begin(), end = lits.end(); it != end; ++it) {
		if (it->first == x) { return static_cast<uint32>(it - lits.begin()); }
	}
	return UINT32_MAX;
}

weight_t BodyInfo::sum() const  {
	wsum_t s = 0;
	for (WeightLitVec::const_iterator it = lits.begin(), end = lits.end(); it != end; ++it) {
		s += it->second;
	}
	assert(s <= std::numeric_limits<weight_t>::max());
	return static_cast<weight_t>(s);
}
/////////////////////////////////////////////////////////////////////////////////////////
// class RuleTransform
//
// class for transforming extended rules to normal rules
/////////////////////////////////////////////////////////////////////////////////////////
Var RuleTransform::AdaptBuilder::newAtom() {
	return prg_->newAtom();
}
void RuleTransform::AdaptBuilder::addRule(Rule& rule) {
	prg_->addRule(rule);
}

class RuleTransform::Impl {
public:
	Impl(ProgramAdapter& prg, Rule& r);
	~Impl();
	uint32 transform();
private:
	Impl(const Impl&);
	Impl& operator=(const Impl&);
	struct TodoItem {
		TodoItem(uint32 i, weight_t w, Var v) : idx(i), bound(w), var(v) {}
		uint32   idx;
		weight_t bound;
		Var      var;
	};
	typedef std::deque<TodoItem> TodoList;
	bool   isBogusRule() const { return rule_.bound() > sumW_[0]; }
	bool   isFact()      const { return rule_.bound() <= 0; }
	void   createRule(Var head, Literal* bodyFirst, Literal* bodyEnd);
	uint32 addRule(Var head, bool addLit, const TodoItem& aux);
	Var    getAuxVar(const TodoItem& i) {
		assert(i.bound > 0);
		uint32 k = i.bound-1;
		if (aux_[k] == 0) {
			todo_.push_back(i);
			aux_[k]          = prg_.newAtom();
			todo_.back().var = aux_[k];
		}
		return aux_[k];
	}
	TodoList        todo_; // heads todo
	ProgramAdapter& prg_;  // program to which rules are added
	Rule&           rule_; // rule to translate
	Rule            out_;  // transformed rule
	Var*            aux_;  // newly created atoms for one level of the tree
	weight_t*       sumW_; // achievable weight for individual literals
};

RuleTransform::RuleTransform() {}

uint32 RuleTransform::transform(ProgramAdapter& prg, Rule& rule) {
	if (rule.type() == CHOICERULE) {
		return transformChoiceRule(prg, rule);
	}
	else if (rule.type() == DISJUNCTIVERULE) {
		return transformDisjunctiveRule(prg, rule);
	}
	return Impl(prg, rule).transform();
}

RuleTransform::Impl::Impl(ProgramAdapter& prg, Rule& r)
	: prg_(prg)
	, rule_(r) {
	aux_     = new Var[r.bound()];
	sumW_    = new weight_t[r.body.size()+1];
	std::memset(aux_ , 0, r.bound()*sizeof(Var));
	RuleTransform::prepareRule(r, sumW_);
}
RuleTransform::Impl::~Impl() {
	delete [] aux_;
	delete [] sumW_;
}

// Quadratic transformation of cardinality and weight constraint.
// Introduces aux atoms. 
// E.g. a rule h = 2 {a,b,c,d} is translated into the following eight rules:
// h       :- a, aux_1_1.
// h       :- aux_1_2.
// aux_1_1 :- b.
// aux_1_1 :- aux_2_1.
// aux_1_2 :- b, aux_2_1.
// aux_1_2 :- c, d.
// aux_2_1 :- c.
// aux_2_1 :- d.
uint32 RuleTransform::Impl::transform() {
	if (isBogusRule()) { 
		return 0;
	}
	if (isFact()) {
		createRule(rule_.heads[0], 0, 0);
		return 1;
	}
	todo_.push_back(TodoItem(0, rule_.bound(), rule_.heads[0]));
	uint32 normalRules = 0;
	uint32 level = 0;
	while (!todo_.empty()) {
		TodoItem i = todo_.front();
		todo_.pop_front();
		if (i.idx > level) {
			// We are about to start a new level of the tree.
			// Reset the aux_ array
			level = i.idx;
			std::memset(aux_ , 0, rule_.bound()*sizeof(Var));
		}
		// For a todo item i with var v create at most two rules:
		// r1: v :- lit(i.idx), AuxLit(i.idx+1, i.bound-weight(lit(i.idx)))
		// r2: v :- AuxLit(i.idx+1, i.bound).
		// The first rule r1 represents the case where lit(i.idx) is true, while
		// the second rule encodes the case where the literal is false.
		normalRules += addRule(i.var, true,  TodoItem(i.idx+1, i.bound - rule_.body[i.idx].second, 0));
		normalRules += addRule(i.var, false, TodoItem(i.idx+1, i.bound, 0));
	}
	return normalRules;
}

uint32 RuleTransform::Impl::addRule(Var head, bool addLit, const TodoItem& aux) {
	// create rule head :- posLit(aux.var) resp. head :- posLit(aux.var), ruleLit(aux.idx-1)
	//
	// Let B be the bound of aux, 
	//  - skip rule, iff sumW(aux.idx) < B, i.e. rule is not applicable.
	//  - replace rule with list of body literals if sumW(aux.idx)-minW < B or B <= 0
	weight_t minW = rule_.body.back().second;
	if (aux.bound <= 0 || sumW_[aux.idx] >= aux.bound) {
		if (aux.bound <= 0) {
			assert(addLit);
			Literal body = rule_.body[aux.idx-1].first;
			createRule(head, &body, &body+1);
		}
		else if ((sumW_[aux.idx] - minW) < aux.bound) {
			LitVec nb;
			if (addLit) {
				nb.push_back(rule_.body[aux.idx-1].first);
			}
			for (uint32 r = aux.idx; r != rule_.body.size(); ++r) {
				nb.push_back(rule_.body[r].first);
			}
			createRule(head, &nb[0], &nb[0]+nb.size());
		}
		else {
			Var auxVar      = getAuxVar(aux);
			Literal body[2] = { rule_.body[aux.idx-1].first, posLit(auxVar) };
			createRule(head, body+!addLit, body+2);
		}
		return 1;
	}
	return 0;
}

void RuleTransform::Impl::createRule(Var head, Literal* bodyFirst, Literal* bodyEnd) {
	out_.clear();
	out_.setType(BASICRULE);		
	out_.addHead(head);
	while (bodyFirst != bodyEnd) {
		out_.addToBody(bodyFirst->var(), !bodyFirst->sign());
		++bodyFirst;
	}
	prg_.addRule(out_);
}

weight_t RuleTransform::prepareRule(Rule& r, weight_t* sumVec) {
	if (r.type() != CONSTRAINTRULE && r.type() != WEIGHTRULE) { return 0; }
	weight_t sum = 0;
	if (r.type() == WEIGHTRULE) {
		std::stable_sort(r.body.begin(), r.body.end(), compose22(
				std::greater<weight_t>(),
				select2nd<WeightLiteral>(),
				select2nd<WeightLiteral>()));
		for (uint32 i = r.body.size(); i--; ) {
			sum      += r.body[i].second;
			sumVec[i] = sum;
		}
	}
	else { // no weights allowed!
		for (uint32 i = r.body.size(); i--; ) {
			sum      += (r.body[i].second = 1);
			sumVec[i] = sum;
		}
	}
	return sum;
}

// Exponential transformation of cardinality and weight constraint.
// Creates minimal subsets, no aux atoms.
// E.g. a rule h = 2 {a,b,c,d} is translated into the following six rules:
// h :- a, b.
// h :- a, c.
// h :- a, d.
// h :- b, c.
// h :- b, d.
// h :- c, d.
uint32 RuleTransform::transformNoAux(ProgramAdapter& prg, Rule& rule) {
	assert(rule.type() == WEIGHTRULE || rule.type() == CONSTRAINTRULE);
	WeightVec sumWeights(rule.body.size() + 1, 0);
	prepareRule(rule, &sumWeights[0]);
	uint32 newRules = 0;
	VarVec    nextStack;
	WeightVec weights;
	Rule r(BASICRULE);
	r.addHead(rule.heads[0]);
	uint32    end   = (uint32)rule.body.size();
	weight_t  cw    = 0;
	uint32    next  = 0;
	if (next == end) { prg.addRule(r); return 1; }
	while (next != end) {
		r.addToBody(rule.body[next].first.var(), rule.body[next].first.sign() == false);
		weights.push_back( rule.body[next].second );
		cw += weights.back();
		++next;
		nextStack.push_back(next);
		if (cw >= rule.bound()) {
			prg.addRule(r);
			r.setType(BASICRULE);
			++newRules;
			r.body.pop_back();
			cw -= weights.back();
			nextStack.pop_back();
			weights.pop_back();
		}
		while (next == end && !nextStack.empty()) {
			r.body.pop_back();
			cw -= weights.back();
			weights.pop_back();
			next = nextStack.back();
			nextStack.pop_back();
			if (next != end && (cw + sumWeights[next]) < rule.bound()) {
				next = end;
			}
		}
	}
	return newRules;
}

// A choice rule {h1,...hn} :- BODY
// is replaced with:
// h1   :- BODY, not aux1.
// aux1 :- not h1.
// ...
// hn   :- BODY, not auxN.
// auxN :- not hn.
// If n is large or BODY contains many literals BODY is replaced with auxB and
// auxB :- BODY.
uint32 RuleTransform::transformChoiceRule(ProgramAdapter& prg, Rule& rule) const {
	uint32 newRules = 0;
	Var extraHead = ((rule.heads.size() * (rule.body.size()+1)) + rule.heads.size()) > (rule.heads.size()*3)+rule.body.size()
			? prg.newAtom()
			: varMax;
	Rule r1, r2;
	r1.setType(BASICRULE); r2.setType(BASICRULE);
	if (extraHead != varMax) { r1.addToBody( extraHead, true, 1 ); }
	else { r1.body.swap(rule.body); }
	for (VarVec::iterator it = rule.heads.begin(), end = rule.heads.end(); it != end; ++it) {
		r1.heads.clear(); r2.heads.clear();
		Var aux = prg.newAtom();
		r1.heads.push_back(*it);  r1.addToBody(aux, false, 1);
		r2.heads.push_back(aux);  r2.addToBody(*it, false, 1);
		prg.addRule(r1);  // h    :- body, not aux
		prg.addRule(r2);  // aux  :- not h
		r1.body.pop_back();
		r2.body.pop_back();
		newRules += 2;
	}
	if (extraHead != varMax) {
		r1.heads.clear();
		r1.body.clear();
		r1.body.swap(rule.body);
		r1.heads.push_back(extraHead);
		prg.addRule(r1);
		++newRules;
	}
	rule.body.swap(r1.body);
	return newRules;
}

// A disjunctiive rule h1|...|hn :- BODY
// is replaced with:
// hi   :- BODY, {not hj | 1 <= j != i <= n}.
// If BODY contains more than one literal, BODY is replaced with auxB and
// auxB :- BODY.
uint32 RuleTransform::transformDisjunctiveRule(ProgramAdapter& prg, Rule& rule) const {
	uint32 newRules = 0;
	Rule temp; temp.setType(BASICRULE);
	if (rule.body.size() > 1) {
		Rule bodyR;
		bodyR.setType(BASICRULE);
		bodyR.body.swap(rule.body);
		Var auxB = prg.newAtom();
		bodyR.addHead(auxB);
		prg.addRule(bodyR);
		++newRules;
		rule.body.swap(bodyR.body);
		temp.addToBody(auxB, true);
	}
	else {
		temp.body = rule.body;
	}
	for (VarVec::const_iterator it = rule.heads.begin(), end = rule.heads.end(); it != end; ++it) {
		temp.heads.assign(1, *it);
		temp.body.erase(temp.body.begin()+1, temp.body.end());
		for (VarVec::const_iterator j = rule.heads.begin(); j != end; ++j) {
			if (j != it) { temp.addToBody(*j, false); }
		}
		prg.addRule(temp);
		++newRules;
	}
	return newRules;
}

/////////////////////////////////////////////////////////////////////////////////////////
// class SccChecker
// 
// SCC/cycle checking
/////////////////////////////////////////////////////////////////////////////////////////
SccChecker::SccChecker(LogicProgram& prg, AtomList& sccAtoms, uint32 startScc)
	: prg_(&prg), sccAtoms_(&sccAtoms), count_(0), sccs_(startScc) {
	for (uint32 i = 0; i != prg.numAtoms(); ++i) {
		visit(prg.getAtom(i));
	}
	for (uint32 i = 0; i != prg.numBodies(); ++i) {
		visit(prg.getBody(i));
	}
}

void SccChecker::visitDfs(PrgNode* node, NodeType t) {
	if (!prg_ || !doVisit(node)) {
		return;
	}
	callStack_.clear();
	nodeStack_.clear();
	count_ = 0;
	addCall(node, t, 0);
	while (!callStack_.empty()) {
		Call c = callStack_.back();
		callStack_.pop_back();
		if (!recurse(c)) {
			node = unpackNode(c.node);
			if (c.min < node->id()) {
				node->resetId( c.min, true );
			}
			else if (c.node == nodeStack_.back()) {
				// node is trivially-connected; all such nodes are in the same Pseudo-SCC
				if (isNode(nodeStack_.back(), PrgEdge::ATOM_NODE)) {
					static_cast<PrgAtom*>(node)->setScc(PrgNode::noScc);
				}
				node->resetId(PrgNode::maxVertex, true);
				nodeStack_.pop_back();
			}
			else { // non-trivial SCC
				PrgNode* succVertex;
				do {
					succVertex = unpackNode(nodeStack_.back());
					if (isNode(nodeStack_.back(), PrgEdge::ATOM_NODE)) {
						static_cast<PrgAtom*>(succVertex)->setScc(sccs_);
						sccAtoms_->push_back(static_cast<PrgAtom*>(succVertex));
					}
					nodeStack_.pop_back();
					succVertex->resetId(PrgNode::maxVertex, true);
				} while (succVertex != node);
				++sccs_;
			}
		}
	}
}

bool SccChecker::recurse(Call& c) {
	PrgNode* n = unpackNode(c.node);
	if (!n->seen()) {
		nodeStack_.push_back(c.node);
		c.min = count_++;
		n->resetId(c.min, true);
	}
	if (isNode(c.node, PrgEdge::BODY_NODE)) {
		PrgBody* b = static_cast<PrgBody*>(n);
		PrgHead* h = 0; NodeType t;
		for (PrgBody::head_iterator it = b->heads_begin() + c.next, end = b->heads_end(); it != end; ++it) {
			if   (it->isAtom()){ h = prg_->getAtom(it->node()); t = PrgEdge::ATOM_NODE; }
			else               { h = prg_->getDisj(it->node()); t = PrgEdge::DISJ_NODE; }
			if (doVisit(h, false) && onNode(h, t, c, static_cast<uint32>(it-b->heads_begin()))) {
				return true;
			}
		}
	}
	else if (isNode(c.node, PrgEdge::ATOM_NODE)) {
		PrgAtom* a = static_cast<PrgAtom*>(n);
		for (PrgAtom::dep_iterator it = a->deps_begin() + c.next, end = a->deps_end(); it != end; ++it) {
			if (it->sign()) continue;
			PrgBody* bn = prg_->getBody(it->var());
			if (doVisit(bn, false) && onNode(bn, PrgEdge::BODY_NODE, c, static_cast<uint32>(it-a->deps_begin()))) {
				return true;
			}
		}
	}
	else if (isNode(c.node, PrgEdge::DISJ_NODE)) {
		PrgDisj* d = static_cast<PrgDisj*>(n);
		for (PrgDisj::atom_iterator it = d->begin() + c.next, end = d->end(); it != end; ++it) {
			PrgAtom* a = prg_->getAtom(it->node());
			if (doVisit(a, false) && onNode(a, PrgEdge::ATOM_NODE, c, static_cast<uint32>(it-d->begin()))) {
				return true;
			}
		}
	}
	return false;
}

bool SccChecker::onNode(PrgNode* n, NodeType t, Call& c, uint32 data) {
	if (!n->seen()) {
		Call rec = {c.node, c.min, data};
		callStack_.push_back(rec);
		addCall(n, t, 0);
		return true;
	}
	if (n->id() < c.min) {
		c.min = n->id();
	}
	return false;
}
/////////////////////////////////////////////////////////////////////////////////////////
PrgNode::PrgNode(uint32 id, bool checkScc) 
	: litIdx_(noIdx), noScc_(uint32(!checkScc)), id_(id), val_(value_free), eq_(0), seen_(0) {
	static_assert(sizeof(PrgNode) == sizeof(uint64), "Unsupported Alignment");
}
/////////////////////////////////////////////////////////////////////////////////////////
// class PrgHead
/////////////////////////////////////////////////////////////////////////////////////////
PrgHead::PrgHead(uint32 id, NodeType t, uint32 data, bool checkScc)
	: PrgNode(id, checkScc)
	, data_(data), upper_(0), dirty_(0), freeze_(0), isAtom_(t == PrgEdge::ATOM_NODE)  {
	struct X { uint64 x; EdgeVec y; uint32 z; };
	static_assert(sizeof(PrgHead) == sizeof(X), "Unsupported Alignment");
}
// Adds the node with given id as a support to this head
// and marks the head as dirty so that any duplicates/false/eq
// supports are removed once simplify() is called.
void PrgHead::addSupport(PrgEdge r, Simplify s) {
	supports_.push_back(r);
	if (s == force_simplify) { dirty_ = (supports_.size() > 1); }
}
// Removes the given node from the set of supports of this head.
void PrgHead::removeSupport(PrgEdge r) {
	if (relevant()) {
		supports_.erase(std::remove(supports_.begin(), supports_.end(), r), supports_.end());
	}
	dirty_ = 1;
}
	
void PrgHead::clearSupports() {
	supports_.clear(); 
	upper_  = 0;
	dirty_  = 0;
}
// Simplifies the set of predecessors supporting this head.
// Removes false/eq supports and returns the number of 
// different supporting literals in numDiffSupps.
bool PrgHead::simplifySupports(LogicProgram& prg, bool strong, uint32* numDiffSupps) {
	uint32 numLits = supports();
	uint32 choices = 0;
	if (dirty_ == 1) {
		dirty_                  = 0;
		numLits                 = 0;
		SharedContext& ctx      = *prg.ctx();
		EdgeVec::iterator it,n,j= supports_.begin();
		for (it = supports_.begin(); it != supports_.end(); ++it) {
			PrgNode* x = prg.getSupp(*it);
			if (x->relevant() && x->value() != value_false && (!strong || x->hasVar())) {
				if      (!x->seen()) { *j++ = *it; x->setSeen(true); }
				else if (!choices)   { continue; }
				else                 {
					for (n = supports_.begin(); n != it && n->node() != it->node(); ++n) {;}
					if (*it < *n)      { *n = *it; }
					else               { continue; }
				}
				choices += (it->isBody() && it->isChoice());
				if (strong && !ctx.marked(x->literal())) {
					++numLits;
					ctx.mark(x->literal());
				}
			}
		}
		supports_.erase(j, supports_.end());
		uint32 dis = 0;
		choices    = 0;
		for (it = supports_.begin(); it != supports_.end(); ++it) {
			PrgNode* x = prg.getSupp(*it);
			x->setSeen(false);
			if (strong && ctx.marked(x->literal())) { ctx.unmark(x->var()); }
			if (it->isChoice()) {
				++choices;
				dis += it->isDisj();
			}
		}
		numLits += choices;
	}
	if (numDiffSupps) { *numDiffSupps = numLits; }
	return supports() > 0 || prg.assignValue(this, value_false);
}

// Assigns a variable to this head.
// No support: no var and value false
// More than one support or type not normal: new var
// Exactly one support that is normal: use that 
void PrgHead::assignVar(LogicProgram& prg, PrgEdge support) {
	if (hasVar() || !relevant()) { return; }
	uint32 numB = supports();
	if (numB == 0 && support == PrgEdge::noEdge()) {
		prg.assignValue(this, value_false);
	}
	else {
		PrgNode* sup = prg.getSupp(support);
		if (support.isNormal() && (numB == 1 || sup->value() == value_true)) {
			// head is equivalent to sup
			setLiteral(sup->literal());
			prg.ctx()->setVarEq(var(), true);
			prg.incEqs(Var_t::atom_body_var);
		}
		else {
			setLiteral(posLit(prg.ctx()->addVar(Var_t::atom_var)));
		}
	}
}

// Backpropagates the value of this head to its supports.
bool PrgHead::backpropagate(LogicProgram& prg, ValueRep val, bool bpFull) {
	bool ok = true;
	if (val == value_false) {
		// a false head can't have supports
		EdgeVec temp; temp.swap(supports_);
		markDirty();
		for (EdgeIterator it = temp.begin(), end = temp.end(); it != end && ok; ++it) {
			if (it->isBody()) {
				ok = prg.getBody(it->node())->propagateAssigned(prg, this, it->type());
			}
			else { assert(it->isDisj());
				ok = prg.getDisj(it->node())->propagateAssigned(prg, this, it->type());
			}
		}
	}
	else if (val != value_free && supports() == 1 && bpFull) {
		// head must be true and only has one support, thus the only support must
		// be true, too.
		PrgBody* b   = 0;
		if (supports_[0].isBody()) {
			b   = prg.getBody(supports_[0].node());
		}
		else if (supports_[0].isDisj()) {
			PrgDisj* d = prg.getDisj(supports_[0].node());
			if (d->supports() == 1) { b = prg.getBody(d->supps_begin()->node()); }
		}
		ok = !b || b->value() == val || (b->assignValue(val) && b->propagateValue(prg, bpFull));
	}
	return ok;
}

/////////////////////////////////////////////////////////////////////////////////////////
// class PrgAtom
/////////////////////////////////////////////////////////////////////////////////////////
PrgAtom::PrgAtom(uint32 id, bool checkScc) 
	: PrgHead(id, PrgEdge::ATOM_NODE, PrgNode::noScc, checkScc) {
	static_assert(sizeof(PrgAtom) == sizeof(PrgHead) + sizeof(LitVec), "Unsupported Alignment");
}		

void PrgAtom::setEqGoal(Literal x) {
	if (eq()) {
		data_ = x.sign() ? x.var() : noScc;
	}
}
Literal PrgAtom::eqGoal(bool sign) const {
	if (!eq() || sign || data_ == noScc) {
		return Literal(id(), sign);
	}
	return negLit(data_);
}

// Adds a dependency between this atom and the body with
// the given id. If pos is true, atom appears positively
// in body, otherwise negatively.
void PrgAtom::addDep(Var bodyId, bool pos) {
	deps_.push_back(Literal(bodyId, !pos));	
}

// Removes a dependency between this atom and the body with
// the given id. If pos is true, atom appears positively
// in body, otherwise negatively.
void PrgAtom::removeDep(Var bodyId, bool pos) {
	LitVec::iterator it = std::find(deps_.begin(), deps_.end(), Literal(bodyId, !pos));
	if (it != deps_.end()) { deps_.erase(it); }
}

// Removes the subset of dependencies given by d
void PrgAtom::clearDeps(Dependency d) {
	if (d == dep_all) {
		deps_.clear();
	}
	else {
		bool sign = d == dep_neg;
		LitVec::iterator j = deps_.begin();
		for (LitVec::iterator it = deps_.begin(), end = deps_.end(); it != end; ++it) {
			if (it->sign() != sign) { *j++ = *it; }
		}
		deps_.erase(j, deps_.end());
	}
}

bool PrgAtom::hasDep(Dependency d) const {
	if (d == dep_all) { return !deps_.empty(); }
	for (LitVec::const_iterator it = deps_.begin(), end = deps_.end(); it != end; ++it) {
		if (static_cast<Dependency>(it->sign()) == d) { return true; }
	}
	return false;
}

bool PrgAtom::inDisj() const {
	for (EdgeIterator it= supports_.begin(), end = supports_.end(); it != end; ++it) {
		if (it->isDisj()) { return true; }
	}
	return false;
}

// Propagates the value of this atom to its depending bodies
// and, if backpropagation is enabled, to its supporting bodies/disjunctions.
// PRE: value() != value_free
bool PrgAtom::propagateValue(LogicProgram& prg, bool backprop) {
	ValueRep val = value();
	assert(val != value_free);
	// propagate value forward
	Literal dep = posLit(id());
	for (dep_iterator it = deps_.begin(), end = deps_end(); it != end; ++it) {
		if (!prg.getBody(it->var())->propagateAssigned(prg, dep ^ it->sign(), val)) {
			return false;
		}
	}
	if (prg.isFact(this) && inDisj()) {
		// - atom is true, thus all disjunctive rules containing it are superfluous
		EdgeVec temp; temp.swap(supports_);
		EdgeVec::iterator j = temp.begin();
		EdgeType t          = PrgEdge::CHOICE_EDGE;
		for (EdgeIterator it= temp.begin(), end = temp.end(); it != end; ++it) {
			if      (!it->isDisj())                                            { *j++ = *it; }
			else if (!prg.getDisj(it->node())->propagateAssigned(prg, this, t)){ return false; }
		}
		temp.erase(j, temp.end());
		supports_.swap(temp);
	}
	return backpropagate(prg, val, backprop);
}

// Adds the atom-oriented nogoods for this atom in form of clauses.
// Adds the support clause [~a S1...Sn] (where each Si is a supporting node of a)
// representing the tableau-rules BTA and FFA.
// Furthermore, adds the clause [a ~Bj] representing tableau-rules FTA and BFA
// if Bj supports a via a "normal" edge. 
bool PrgAtom::addConstraints(const LogicProgram& prg, ClauseCreator& gc) {
	SharedContext& ctx  = *prg.ctx();
	EdgeVec::iterator j = supports_.begin();
	bool           nant = false;
	gc.start().add(~literal());
	for (EdgeVec::iterator it = supports_.begin(); it != supports_.end(); ++it) {
		PrgNode* n = prg.getSupp(*it);
		Literal  B = n->literal();
		// consider only bodies which are part of the simplified program, i.e.
		// are associated with a variable in the solver.
		if (n->relevant() && n->hasVar()) {
			*j++ = *it;
			nant = nant || it->isChoice();
			if (!it->isDisj()) { gc.add(B); }
			if (it->isNormal() && !ctx.addBinary(literal(), ~B)) { // FTA/BFA
				return false;
			}
		}
	}
	supports_.erase(j, supports_.end());
	if (nant ||	hasDep(PrgAtom::dep_neg)) { ctx.setNant(var(), true); }
	return gc.end(ClauseCreator::clause_force_simplify);
}

/////////////////////////////////////////////////////////////////////////////////////////
// class PrgBody
/////////////////////////////////////////////////////////////////////////////////////////
PrgBody::PrgBody(LogicProgram& prg, uint32 id, const BodyInfo& body, bool addDeps)
	: PrgNode(id, true)
	, size_(body.size()), extHead_(0), type_(body.type()), sBody_(0), sHead_(0), unsupp_(0) {
	Literal* lits  = goals_begin();
	Literal* p[2]  = {lits, lits + body.posSize()};
	weight_t sw[2] = {0,0}; // sum of positive/negative weights
	const bool W   = type() == BodyInfo::SUM_BODY;
	if (W) {
		data_.ext[0] = SumExtra::create(body.size());
	}
	weight_t   w   = 1;
	// store B+ followed by B- followed by optional weights
	for (WeightLitVec::const_iterator it = body.lits.begin(), end = body.lits.end(); it != end; ++it) {
		Literal x    = it->first;
		*p[x.sign()] = x;
		if (W) {   w = it->second; data_.ext[0]->weights[p[x.sign()] - lits] = w; }
		++p[x.sign()];
		sw[x.sign()] += w;
		if (addDeps) { prg.getAtom(x.var())->addDep(id, !x.sign()); }
	}
	if (body.type() == BodyInfo::COUNT_BODY) {
		data_.lits[0] = body.bound();
	}
	else if (W) {
		data_.ext[0]->bound = body.bound();
		data_.ext[0]->sumW  = sw[0] + sw[1];
	}
	unsupp_ = static_cast<weight_t>(this->bound() - sw[1]);
	if (bound() == 0) {
		assignValue(value_true);
		markDirty();
	}
}

PrgBody* PrgBody::create(LogicProgram& prg, uint32 id, const BodyInfo& body, bool addDeps) {
	uint32 bytes = sizeof(PrgBody) + (body.size() * sizeof(Literal));
	if (body.type() != BodyInfo::NORMAL_BODY) {
		bytes += (SumExtra::LIT_OFFSET * sizeof(uint32));
	}
	return new (::operator new(bytes)) PrgBody(prg, id, body, addDeps);
}

PrgBody::~PrgBody() {
	clearHeads();
	if (hasWeights()) {
		data_.ext[0]->destroy();
	}
}

void PrgBody::destroy() {
	this->~PrgBody();
	::operator delete(this);
}

PrgBody::SumExtra* PrgBody::SumExtra::create(uint32 size) {
	uint32 bytes = sizeof(SumExtra) + (size * sizeof(weight_t));
	return new (::operator new(bytes)) SumExtra;
}
void PrgBody::SumExtra::destroy() {
	::operator delete(this);
}

uint32 PrgBody::findLit(const LogicProgram& prg, Literal p) const {
	for (const Literal* it = goals_begin(), *end = it + size(); it != end; ++it) {
		Literal x = prg.getAtom(it->var())->literal();
		if (it->sign()) x = ~x;
		if (x == p) return static_cast<uint32>(it - goals_begin());
	}
	return varMax;
}

// Sets the unsupported counter back to
// bound() - negWeight()
bool PrgBody::resetSupported() {
	unsupp_ = bound();
	for (uint32 x = size(); x && goal(--x).sign(); ) {
		unsupp_ -= weight(x);
	}
	return isSupported();
}

// Removes all heads from this body *without* notifying them 
void PrgBody::clearHeads() {
	if (extHead()) { delete heads_.ext; }
	extHead_ = 0;
}

// Makes h a head-successor of this body and adds this
// body as a support for h.
void PrgBody::addHead(PrgHead* h, EdgeType t) {
	assert(relevant() && h->relevant());
	PrgEdge fwdEdge = PrgEdge::newEdge(h->id(), t, h->isAtom() ? PrgEdge::ATOM_NODE : PrgEdge::DISJ_NODE);
	PrgEdge bwdEdge = PrgEdge::newEdge(id(), t, PrgEdge::BODY_NODE);
	addHead(fwdEdge);
	h->addSupport(bwdEdge);
	// mark head-set as dirty
	if (extHead_ > 1) {  sHead_ = 1; }
}

void PrgBody::addHead(PrgEdge h) {
	if      (extHead_ < 2u) { heads_.simp[extHead_++] = h; }
	else if (extHead())     { heads_.ext->push_back(h);    }
	else                    { 
		EdgeVec* t  = new EdgeVec(heads_.simp, heads_.simp+2);
		t->push_back(h);
		heads_.ext  = t;
		extHead_    = 3u;
	}
}

void PrgBody::removeHead(PrgHead* h, EdgeType t) {
	PrgEdge x = PrgEdge::newEdge(h->id(), t, h->isAtom() ? PrgEdge::ATOM_NODE : PrgEdge::DISJ_NODE);
	if (eraseHead(x)) {
		h->removeSupport(PrgEdge::newEdge(id(), t, PrgEdge::BODY_NODE)); // also remove back edge
	}
}

bool PrgBody::hasHead(PrgHead* h, EdgeType t) const {
	if (!hasHeads()) { return false;  }
	PrgEdge x = PrgEdge::newEdge(h->id(), t, h->isAtom() ? PrgEdge::ATOM_NODE : PrgEdge::DISJ_NODE);
	head_iterator it = sHead_ != 0 || !extHead() ? std::find(heads_begin(), heads_end(), x) : std::lower_bound(heads_begin(), heads_end(), x);
	return it != heads_end() && *it == x;
}

bool PrgBody::eraseHead(PrgEdge h) {
	PrgEdge* it = const_cast<PrgEdge*>(std::find(heads_begin(), heads_end(), h));
	if (it != heads_end()) {
		if (extHead()) { heads_.ext->erase(it); }
		else           { *it = heads_.simp[1]; --extHead_; }
		return true;
	}
	return false;
}

// Simplifies the body by removing assigned atoms & replacing eq atoms.
// Checks whether simplified body must be false (CONTRA) or is
// structurally equivalent to some other body.
// prg    The program containing this body
// strong If true, treats atoms that have no variable associated as false. 
// eqId   The id of a body in prg that is equivalent to this body	 
bool PrgBody::simplifyBody(LogicProgram& prg, bool strong, uint32* eqId) {
	if (eqId)        { *eqId  = id(); }
	if (sBody_ == 0) { return true;   }
	// update body - compute old hash value
	SharedContext& ctx = *prg.ctx();
	uint32 oldHash     = 0;
	weight_t bound     = this->bound();
	weight_t w         = 1, *jw = hasWeights() ? data_.ext[0]->weights : 0;
	Literal* lits      = goals_begin();
	Literal* j         = lits;
	RuleState& todo    = prg.ruleState();
	Var a;
	bool mark, isEq;
	int todos = 0;
	for (Literal* it = j, *end = j + size(); it != end; ++it) {
		a       = it->var();
		isEq    = a != prg.getEqAtom(a);
		oldHash+= hashLit(*it);
		if (isEq) {
			prg.getAtom(a)->removeDep(id(), !it->sign()); // remove old edge
			*it = prg.getAtom(a)->eqGoal(it->sign());     // replace with eq goal
			a   = it->var();                              // and check it
		}
		Literal aLit = it->sign() ? ~prg.getAtom(a)->literal() : prg.getAtom(a)->literal();
		ValueRep v   = prg.getAtom(a)->value();
		mark         = strong || prg.getAtom(a)->hasVar();
		if (strong && !prg.getAtom(a)->hasVar()) {
			v = value_false;
		}
		if (v == value_weak_true && it->sign()) {
			v = value_true;
		}
		if (v == value_true || v == value_false) { // truth value is known - remove subgoal
			if (v == trueValue(*it)) {
				// subgoal is true: decrease necessary lower bound
				bound -= weight(uint32(it - lits));
			}
			prg.getAtom(a)->removeDep(id(), !it->sign());
		}
		else if (!mark || !ctx.marked(aLit)) {
			if (mark) { ctx.mark(aLit); }
			if (isEq) { // remember to add edge for new goal later
				todo.addToBody(Literal(*it));
				++todos;
			}
			*j++ = *it;  // copy literal and optionally weight
			if (jw) { *jw++ = weight(uint32(it - lits)); }
		}
		else { // body contains aLit more than once 
			if (type() != BodyInfo::NORMAL_BODY) { // merge subgoal
				if (!jw) {
					SumExtra* extra = SumExtra::create(size());
					extra->bound    = this->bound();
					extra->sumW     = this->sumW();
					type_           = BodyInfo::SUM_BODY;
					w               = 1;
					std::fill_n(extra->weights, size(), w);
					data_.ext[0]    = extra;
					jw              = extra->weights + (it - lits);
				}
				else { w = weight(uint32(it - lits)); }
				uint32 pos = findLit(prg, aLit);
				data_.ext[0]->weights[pos] += w;
			}
			else { // ignore if normal
				--bound;
				if (!isEq) { // remove edge
					if (todo.inBody(*it)) { todo.clearBody(*it); --todos; }
					else                  { prg.getAtom(it->var())->removeDep(id(), !it->sign()); }
				}
			} 
		}
	}
	// unmark atoms, compute new hash value,
	// and restore pos | neg partition in case
	// we changed some positive goals to negative ones 
	size_          = j - lits;
	if (jw) jw     = data_.ext[0]->weights;
	uint32 newHash = 0;
	weight_t sumW  = 0, reachW = 0;
	for (uint32 p = 0, n = size_, i, h; p < n;) {
		if      (!lits[p].sign())      { h = hashLit(lits[i = p++]);  }
		else if (lits[n-1].sign())     { h = hashLit(lits[i = --n]);  }
		else /* restore pos|neg order */ {
			std::swap(lits[p], lits[n-1]);
			if (jw) { std::swap(jw[p], jw[n-1]); }
			continue;
		}
		a = lits[i].var();
		if (todos && todo.inBody(lits[i])) {
			prg.getAtom(a)->addDep(id(), !lits[i].sign());
			todo.clearBody(lits[i]);
			--todos;
		}
		Var v   = prg.getAtom(a)->var();
		w       = !jw ? 1 : jw[i];
		sumW   += w;
		reachW += w;
		if (ctx.marked(posLit(v)) && ctx.marked(negLit(v))) {
			// body contains aLit and ~aLit
			if  (type() != BodyInfo::SUM_BODY) { reachW -= 1; }
			else {
				Literal other = prg.getAtom(a)->literal() ^ !goal(i).sign();
				uint32 pos    = findLit(prg, other);
				assert(pos != varMax && pos != i);
				reachW       -= std::min(w, jw[pos]);
			}
		}
		ctx.unmark( v );
		newHash += h;
	}
	bool ok = normalize(prg, bound, sumW, reachW, newHash);
	if (ok) {
		Var xId = id();
		if (oldHash != newHash) {
			xId = prg.update(this, oldHash, newHash);
		}
		if (eqId) { *eqId = xId != varMax ? xId : id(); }
	}
	if (strong) sBody_ = 0;
	return ok && (value() == value_free || propagateValue(prg, prg.options().backprop));	
}

bool PrgBody::normalize(const LogicProgram& prg, weight_t bound, weight_t sumW, weight_t reachW, uint32& hashOut) {
	BodyInfo::BodyType nt = (sumW == bound || size() == 1) ? BodyInfo::NORMAL_BODY : type();
	bool ok = true;
	if (sumW >= bound && type() != BodyInfo::NORMAL_BODY) {
		if (type() == BodyInfo::SUM_BODY) {
			data_.ext[0]->bound   = bound;
			data_.ext[0]->sumW    = sumW;
		}
		else if (type() == BodyInfo::COUNT_BODY) {
			data_.lits[0] = bound;
		}
	}
	if (bound <= 0) {
		for (uint32 i = 0, myId = id(); i != size_; ++i) {
			prg.getAtom(goal(i).var())->removeDep(myId, !goal(i).sign());
		}
		size_  = 0; hashOut = 0, unsupp_ = 0;
		nt     = BodyInfo::NORMAL_BODY;
		ok     = assignValue(value_true);
	}
	else if (reachW < bound) {
		ok     = assignValue(value_false);
		sHead_ = 1;
		markRemoved();
	}
	if (nt != type()) {
		assert(nt == BodyInfo::NORMAL_BODY);
		if (type() == BodyInfo::SUM_BODY) {
			data_.ext[0]->destroy();
		}
		Literal* to   = reinterpret_cast<Literal*>(data_.lits);
		Literal* from = goals_begin();
		std::copy(from, from+size(), to);
		type_         = nt;
	}
	return ok;
}

// Marks the set of heads in rs and removes
// any duplicate heads.
void PrgBody::prepareSimplifyHeads(LogicProgram& prg, RuleState& rs) {
	head_iterator end = heads_end();
	uint32 size       = 0;
	for (PrgEdge*  j  = const_cast<PrgEdge*>(heads_begin()); j != end;) {
		if (!rs.inHead(*j)) {
			rs.addToHead(*j);
			++j; ++size;
		}	
		else { 
			prg.getHead(*j)->markDirty();
			*j = *--end; 
		}
	}
	if (extHead()) { shrinkVecTo(*heads_.ext, size); }
	else           { extHead_ = size; }
}

// Simplifies the heads of this body wrt target.
// Removes superfluous/eq/unsupported heads and checks for self-blocking
// situations.
// PRE: prepareSimplifyHeads was called
bool PrgBody::simplifyHeadsImpl(LogicProgram& prg, PrgBody& target, RuleState& rs, bool strong) {
	PrgHead* cur;
	PrgEdge* j     = const_cast<PrgEdge*>(heads_begin());
	uint32 newSize = 0;
	bool merge     = this != &target;
	bool block     = value() == value_false || (merge && target.value() == value_false);
	for (head_iterator it = heads_begin(), end = heads_end(); it != end; ++it) {
		cur  = prg.getHead(*it);
		block= block || target.blockedHead(*it, rs);
		if (!cur->relevant() || (strong && !cur->hasVar()) 
			|| block || target.superfluousHead(prg, cur, *it, rs) || cur->value() == value_false) {
			// remove superfluous and unsupported heads
			cur->removeSupport(PrgEdge::newEdge(id(), it->type(), PrgEdge::BODY_NODE));
			rs.clearHead(*it);
			block = block || (cur->value() == value_false && it->type() == PrgEdge::NORMAL_EDGE);
		}
		else { 
			*j++ = *it; 
			++newSize; 
			if (merge) { target.addHead(cur, it->type()); }
		}
	}
	if (extHead()) { shrinkVecTo(*heads_.ext, newSize); }
	else           { extHead_ = (j - heads_.simp); }
	return !block;
}

bool PrgBody::simplifyHeads(LogicProgram& prg, bool strong) {
	if (sHead_ == 0) { return true; }
	return PrgBody::mergeHeads(prg, *this, strong);
}

bool PrgBody::mergeHeads(LogicProgram& prg, PrgBody& heads, bool strong, bool simplify) {
	RuleState& rs = prg.ruleState();
	bool       ok = true;
	assert((this == &heads || heads.sHead_ == 0) && "Heads to merge not simplified!");
	if (simplify || &heads == this) {
		// mark the body literals so that we can easily detect superfluous atoms
		// and selfblocking situations.	
		for (const Literal* it = goals_begin(), *end = it + size(); it != end; ++it) {
			rs.addToBody(*it);
		}
		// remove duplicate/superfluous heads & check for blocked atoms
		prepareSimplifyHeads(prg, rs);
		if (this == &heads) {
			ok = simplifyHeadsImpl(prg, *this, rs, strong);
		}
		else {
			assert(heads.sHead_ == 0 && "Heads to merge not simplified!");
			heads.prepareSimplifyHeads(prg, rs);
			if (!simplifyHeadsImpl(prg, *this, rs, strong) && !assignValue(value_false)) {
				rs.clearAll();
				return false;
			}
			ok = heads.simplifyHeadsImpl(prg, *this, rs, strong);
			assert(ok || heads.heads_begin() == heads.heads_end());
		}
		// clear temporary flags & reestablish ordering
		std::sort(const_cast<PrgEdge*>(heads_begin()), const_cast<PrgEdge*>(heads_end()));
		for (head_iterator it = heads_begin(), end = heads_end(); it != end; ++it) {	
			rs.clear(it->node());
		}
		for (const Literal* it = goals_begin(), *end = it + size(); it != end; ++it) {
			rs.clear(it->var());
		}	
		sHead_ = 0;
	}
	else if (relevant()) {
		for (head_iterator it = heads.heads_begin(), end = heads.heads_end(); it != end; ++it) {
			PrgHead* h = prg.getHead(*it);
			if (h->relevant()) { addHead(h, it->type()); }
		}
	}
	return ok || (assignValue(value_false) && propagateValue(prg, prg.options().backprop));
}

// Checks whether the head is superfluous w.r.t this body, i.e.
//  - is needed to satisfy the body
//  - it appears in the body and is a choice
//  - it is a disjunction and one of the atoms is needed to satisfy the body
bool PrgBody::superfluousHead(const LogicProgram& prg, const PrgHead* head, PrgEdge it, const RuleState& rs) const {
	if (it.isAtom()) {
		// the head is an atom
		uint32 atomId = it.node();	
		weight_t    w = 1;
		if (rs.inBody(posLit(atomId))) {
			if (type() == BodyInfo::SUM_BODY) {
				const Literal* lits = goals_begin();
				const Literal* x    = std::find(lits, lits + size(), posLit(atomId));	
				assert(x != lits + size());
				w                   = data_.ext[0]->weights[ x - lits ];
			}
			if (it.isChoice() || (sumW() - w) < bound()) {
				return true;
			}
		}
		return it.isChoice() 
			&& (rs.inBody(negLit(atomId)) || rs.inHead(PrgEdge::newEdge(atomId, PrgEdge::NORMAL_EDGE, PrgEdge::ATOM_NODE)));
	}
	else { assert(it.isDisj()); 
		// check each contained atom
		const PrgDisj* dis = static_cast<const PrgDisj*>(head);
		for (PrgDisj::atom_iterator aIt = dis->begin(), aEnd = dis->end(); aIt != aEnd; ++aIt) {
			if (rs.inBody(posLit(aIt->node())) || rs.inHead(PrgEdge::newEdge(aIt->node(), PrgEdge::NORMAL_EDGE, PrgEdge::ATOM_NODE))) {
				return true;
			}
			if (prg.isFact(prg.getAtom(aIt->node()))) {
				return true;
			}
		}
		// check for subsumption
		if (prg.options().iters == LogicProgram::AspOptions::MAX_EQ_ITERS) {
			for (head_iterator hIt = heads_begin(), hEnd = heads_end(); hIt != hEnd; ++hIt) {
				if (hIt->isDisj() && prg.getDisj(hIt->node())->size() < dis->size()) {
					const PrgDisj* other = prg.getDisj(hIt->node());
					for (PrgDisj::atom_iterator a = other->begin(), aEnd = other->end(); a != aEnd && other; ++a) {
						if (std::find(dis->begin(), dis->end(), *a) == dis->end()) {
							other = 0;
						}
					}
					if (other && other->size() > 0) { 
						return true; 
					}
				}
			}
		}
	}
	return false;
}

// Checks whether the rule it.node() :- *this is selfblocking, i.e. 
// from TB follows conflict
bool PrgBody::blockedHead(PrgEdge it, const RuleState& rs) const {
	if (it.isAtom() && it.isNormal() && rs.inBody(negLit(it.node()))) {
		weight_t w = 1;
		if (type() == BodyInfo::SUM_BODY) {
			const Literal* lits = goals_begin();
			const Literal* x    = std::find(lits, lits + size(), negLit(it.node()));	
			assert(x != lits + size());
			w                   = data_.ext[0]->weights[ x - lits ];
		}
		return (sumW() - w) < bound();
	}
	return false;
}

void PrgBody::assignVar(LogicProgram& prg) {
	if (hasVar() || !relevant()) { return; }
	uint32 size = this->size();
	if (size == 0 || value() == value_true) {
		setLiteral(posLit(0));
	}
	else if (size == 1 && prg.getAtom(goal(0).var())->hasVar()) {
		Literal x = prg.getAtom(goal(0).var())->literal();
		setLiteral(goal(0).sign() ? ~x : x);
		prg.ctx()->setVarEq(var(), true);
		prg.incEqs(Var_t::atom_body_var);
	}
	else if (value() != value_false) {
		setLiteral(posLit(prg.ctx()->addVar(Var_t::body_var)));	
	}
	else {
		setLiteral(negLit(0));
	}
}

bool PrgBody::eqLits(WeightLitVec& vec, bool& sorted) const {
	if (!sorted && vec.size() <= 10) {
		for (WeightLitVec::const_iterator it = vec.begin(), end = vec.end(); it != end; ++it) {
			const Literal* x = std::find(goals_begin(), goals_end(), it->first);
			if (x == goals_end() || weight((uint32)std::distance(goals_begin(), x)) != it->second) {
				return false;
			}
		}
	}
	else {
		if (!sorted) { std::stable_sort(vec.begin(), vec.end()); sorted = true; }
		for (uint32 x = 0, end = size(); x != end; ++x) {
			WeightLiteral w(goal(x), weight(x));
			if (!std::binary_search(vec.begin(), vec.end(), w)) {
				return false;
			}
		}
	}
	return true;
}
bool PrgBody::propagateSupported(Var v) {
	weight_t w = 1;
	if (hasWeights()) {
		uint32 pos = (uint32)std::distance(goals_begin(), std::find(goals_begin(), goals_end(), posLit(v)));
		w          = weight(pos);
	}
	return (unsupp_ -= w) <= 0;
}

bool PrgBody::propagateAssigned(LogicProgram& prg, Literal p, ValueRep v) {
	if (!relevant()) return true;
	assert(std::find(goals_begin(), goals_end(), p) != goals_end());
	markDirty();
	ValueRep x = v == value_weak_true ? value_true : v;
	weight_t w = 1; // TODO: find weight of p for weight rule
	if (x == falseValue(p) && (sumW() - w) < bound() && value() != value_false) {
		return assignValue(value_false) && propagateValue(prg, prg.options().backprop);
	}
	else if (x == trueValue(p) && (bound() - w) <= 0 && value() != value_weak_true) {
		return assignValue(value_weak_true) && propagateValue(prg, prg.options().backprop);
	}
	return true;
}

bool PrgBody::propagateAssigned(LogicProgram& prg, PrgHead* h, EdgeType t) {
	if (!relevant()) return true;
	markHeadsDirty();
	if (h->value() == value_false && eraseHead(PrgEdge::newEdge(h->id(), t, h->isAtom() ? PrgEdge::ATOM_NODE : PrgEdge::DISJ_NODE)) && t == PrgEdge::NORMAL_EDGE) {
		return value() == value_false || (assignValue(value_false) && propagateValue(prg, prg.options().backprop));
	}
	return true;
}

bool PrgBody::propagateValue(LogicProgram& prg, bool backprop) {
	ValueRep val = value();
	assert(value() != value_free);
	// propagate value forward
	for (head_iterator h = heads_begin(), end = heads_end(); h != end; ++h) {
		PrgHead* head = prg.getHead(*h);
		if (val == value_false) {
			head->removeSupport(PrgEdge::newEdge(id(), h->type(), PrgEdge::BODY_NODE));
		}
		else if (!h->isChoice() && head->value() != val && !prg.assignValue(head, val)) {
			return false;
		}
	}
	if (val == value_false) { clearHeads(); }
	// propagate value backward
	if (backprop && relevant()) {
		const uint32 W = type() == BodyInfo::SUM_BODY;
		weight_t MAX_W = 1;
		weight_t* wPos = W == 0 ? &MAX_W : data_.ext[0]->weights;
		MAX_W          = *std::max_element(wPos, wPos + (size() * W));
		weight_t bound = value()==value_false ? this->bound() : (sumW() - this->bound())+1;
		if (MAX_W >= bound) {
			ValueRep goalVal;
			for (const Literal* it = goals_begin(), *end = goals_end(); it != end; ++it) {
				if ((bound - *wPos) <= 0) {
					if (!it->sign()) { goalVal = val; }
					else             { goalVal = val == value_false ? value_weak_true : value_false; }
					if (!prg.assignValue(prg.getAtom(it->var()), goalVal)) {
						return false;
					}
				}
				wPos += W;
			}
		}
	}
	return true;
}

// Adds nogoods for the tableau-rules FFB and BTB as well as FTB, BFB.
// For normal bodies, clauses are used, i.e:
//   FFB and BTB:
//     - a binary clause [~b s] for every positive subgoal of b
//     - a binary clause [~b ~n] for every negative subgoal of b
//   FTB and BFB:
//     - a clause [b ~s1...~sn n1..nn] where si is a positive and ni a negative subgoal
// For count/sum bodies, a weight constraint is created
bool PrgBody::addConstraints(const LogicProgram& prg, ClauseCreator& gc) {
	if (type() == BodyInfo::NORMAL_BODY) {
		bool    taut= false;
		Literal negB= ~literal();
		gc.start().add(literal()); 
		for (const Literal* it = goals_begin(), *end = goals_end(); it != end; ++it) {
			Literal li = prg.getAtom(it->var())->literal() ^ it->sign();
			if (li == literal()) { taut = true; continue; }
			if (!prg.ctx()->addBinary(negB, li)) { // [~B li]
				return false;
			}
			if (li.var() != negB.var()) { gc.add(~li); }  // [B v ~l1 v ... v ~ln]
		}
		return taut || gc.end();
	}
	WeightLitVec lits;
	for (uint32 i = 0, end = size_; i != end; ++i) {
		Literal eq = prg.getAtom(goal(i).var())->literal() ^ goal(i).sign();
		lits.push_back(WeightLiteral(eq, weight(i)));
	}
	return WeightConstraint::create(*prg.ctx()->master(), literal(), lits, bound()).ok();
}

// Returns the SCC of body B, i.e.
// - scc if exist atom a in B.heads(), a' in B+, s.th. a.scc == a'.scc
// - noScc otherwise
uint32 PrgBody::scc(const LogicProgram& prg) const {
	uint64 sccMask = 0;
	uint32 end     = size();
	uint32 scc     = PrgNode::noScc;
	bool   large   = false;
	for (uint32 i  = 0; i != end; ++i) {
		if      (goal(i).sign()) { 
			end = i; 
			break; 
		}
		else if ((scc = prg.getAtom(goal(i).var())->scc()) != PrgNode::noScc) {
			sccMask |= uint64(1) << (scc & 63);
			large   |= scc > 63;
		}
	}
	if (sccMask != 0) {
		PrgDisj::atom_iterator aIt = 0, aEnd = 0;
		Var atom;
		for (head_iterator h = heads_begin(), hEnd = heads_end(); h != hEnd; ++h) {
			if (h->isAtom()) { aIt = h; aEnd = h+1; }
			else             { PrgDisj* d = prg.getDisj(h->node()); aIt = d->begin(), aEnd = d->end(); }
			for (; aIt != aEnd; ++aIt) {
				atom = aIt->node();
				scc  = prg.getAtom(atom)->scc();
				if (scc != PrgNode::noScc && (sccMask & (uint64(1) << (scc&63))) != 0) {
					if (!large) { return scc; }
					for (uint32 j = 0; j != end; ++j) {
						if (scc == prg.getAtom(goal(j).var())->scc()) { return scc; }
					}
				}
			}
		}
	}
	return PrgNode::noScc;
}

/////////////////////////////////////////////////////////////////////////////////////////
// class PrgDisj
//
// Head of a disjunctive rule
/////////////////////////////////////////////////////////////////////////////////////////
PrgDisj* PrgDisj::create(uint32 id, const VarVec& heads) {
	void* m = ::operator new(sizeof(PrgDisj) + (heads.size()*sizeof(Var)));
	return new (m) PrgDisj(id, heads);
}

PrgDisj::PrgDisj(uint32 id, const VarVec& atoms) : PrgHead(id, PrgEdge::DISJ_NODE, (uint32)atoms.size()) {
	PrgEdge* a  = atoms_;
	for (VarVec::const_iterator it = atoms.begin(), end = atoms.end(); it != end; ++it) {
		*a++ = PrgEdge::newEdge(*it, PrgEdge::CHOICE_EDGE, PrgEdge::ATOM_NODE);
	}
	std::sort(atoms_, atoms_+size());
}
PrgDisj::~PrgDisj() {}
void PrgDisj::destroy() {
	this->~PrgDisj();
	::operator delete(this);
}

void PrgDisj::detach(LogicProgram& prg) {
	PrgEdge edge = PrgEdge::newEdge(id(), PrgEdge::CHOICE_EDGE, PrgEdge::DISJ_NODE);
	for (atom_iterator it = begin(), end = this->end(); it != end; ++it) {
		prg.getAtom(it->node())->removeSupport(edge);
	}
	EdgeVec temp; temp.swap(supports_);
	for (PrgDisj::sup_iterator it = temp.begin(), end = temp.end(); it != end; ++it) {
		prg.getBody(it->node())->removeHead(this, PrgEdge::NORMAL_EDGE);
	}
	setInUpper(false);
	markRemoved();
}

bool PrgDisj::propagateAssigned(LogicProgram& prg, PrgHead* head, EdgeType t) {
	assert(head->isAtom() && t == PrgEdge::CHOICE_EDGE);
	if (prg.isFact(static_cast<PrgAtom*>(head)) || head->value() == value_false) {
		atom_iterator it = std::find(begin(), end(), PrgEdge::newEdge(head->id(), t, PrgEdge::ATOM_NODE));
		if (it != end()) {
			if      (head->value() == value_true) { detach(prg); }
			else if (head->value() == value_false){
				head->removeSupport(PrgEdge::newEdge(id(), t, PrgEdge::DISJ_NODE));
				std::copy(it+1, end(), (PrgEdge*)it);
				if (--data_ == 1) { 
					PrgAtom* last = prg.getAtom(begin()->node());
					EdgeVec temp;
					clearSupports(temp);
					for (EdgeVec::const_iterator it = temp.begin(), end = temp.end(); it != end; ++it) {
						prg.getBody(it->node())->removeHead(this, PrgEdge::NORMAL_EDGE);
						prg.getBody(it->node())->addHead(last, PrgEdge::NORMAL_EDGE);
					}
					detach(prg);
				}
			}
		}
	}
	return true;
}

} }
