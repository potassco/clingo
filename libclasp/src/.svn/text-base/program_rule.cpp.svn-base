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
#pragma warning (disable : 4146) // unary minus operator applied to unsigned type, result still unsigned
#pragma warning (disable : 4996) // 'std::_Fill_n' was declared deprecated
#endif

#include <clasp/program_rule.h>
#include <clasp/program_builder.h>
#include <clasp/util/misc_types.h>
#include <deque>
#include <numeric>
#include <limits.h>

namespace Clasp {

/////////////////////////////////////////////////////////////////////////////////////////
// class PrgRule
//
// Code for handling a rule of a logic program
/////////////////////////////////////////////////////////////////////////////////////////
void PrgRule::clear() {
	heads.clear();
	body.clear();
	bound_  = 0;
	type_   = ENDRULE;
}

// Adds atomId as head to this rule.
PrgRule& PrgRule::addHead(Var atomId) {
	assert(type_ != ENDRULE && "Invalid operation - Start rule not called");
	heads.push_back(atomId);
	return *this;
}

// Adds atomId to the body of this rule. If pos is true, atomId is added to B+
// otherwise to B-. The weight is ignored (set to 1) unless the rule is a weight/optimize rule
PrgRule& PrgRule::addToBody(Var atomId, bool pos, weight_t weight) {
	assert(type_ != ENDRULE && "Invalid operation - Start rule not called");
	assert( weight >= 0 && "Invalid weight - negative weights are not supported");
	if (weight == 0) return *this;  // ignore weightless atoms
	if (weight != 1 && !bodyHasWeights()) {
		weight = 1;
	}
	body.push_back(WeightLiteral(Literal(atomId, pos == false), weight));
	return *this;
}



// Simplifies the rule:
//  - simplify the body
//  - drop duplicate head atoms
// - TAUT: check for tautologies
//  - atoms that appear in the head and the positive part of the body are removed
//    from the head if they are strictly needed to satisfy the body.
//  - SIMP-CR: atoms that appear both in the head and the body of a choice rule are removed
//    from the head.
PrgRule::RData PrgRule::simplify(RuleState& rs) {
	RData r = { 0, 0, 0, 0 };
	if (!(bodyIsSet()?simplifyNormal(r,rs):simplifyWeight(r,rs))) {
		type_    = ENDRULE;
		r.value  = value_false;
		return r;
	}
	else if (type_ != OPTIMIZERULE && bound() <= 0) {
		r.value = value_true;
	}
	Weights w(*this);
	// NORMALIZE head atoms, check for TAUT and SIMP-CR
	if (type_ != ENDRULE) {
		VarVec::iterator j = heads.begin();
		for (VarVec::iterator it = heads.begin(), end = heads.end();  it != end; ++it) {
			if ( !rs.inHead(*it) && !rs.superfluousHead(type()==CHOICERULE, r.sumWeight, bound(), *it, w)) {
				rs.addToHead(*it);  // mark as seen
				if (rs.selfblocker(type()==CHOICERULE, r.sumWeight, bound(), *it, w)) {
					r.value = value_false;
				}
				*j++ = *it;
			}
		}
		heads.erase(j, heads.end());
		if (heads.empty() && type_ != OPTIMIZERULE) type_ = ENDRULE;
	}
	return r;
}

// Simplifies a normal body and computes its hash:
//   - removes duplicate literals: {a,b,a} -> {a, b}.
//   - checks for contradictions : {a, not a}
// Post: All literals in body are marked
bool PrgRule::simplifyNormal(RData& rd, RuleState& rs) {
	bool contra = false;
	WeightLitVec::iterator j = body.begin();
	for (WeightLitVec::iterator it = body.begin(), bEnd = body.end(); it != bEnd; ++it) {
		if (!rs.inBody(it->first)) {  // a not seen yet
			*j++  = *it;                // keep it and 
			rs.addToBody(it->first);    // mark as seen
			if (it->first.sign()) { rd.hash += hashId(-it->first.var()); }
			else                  { ++rd.posSize; rd.hash += hashId(it->first.var()); }
		}
		if (rs.inBody(~it->first)) {
			contra = true;
		}
	}
	body.erase(j, body.end());
	rd.sumWeight = (weight_t)body.size();
	bound_       = (weight_t)body.size();
	return !contra;
}

// Simplifies a weight body and computes its hash:
//   - removes literals with weight 0    : LB [a = 0, b = 2, c = 0, ...] -> LB [b = 2, ...]
//   - reduces weights > bound() to bound:  2 [a=1, b=3] -> 2 [a=1, b=2]
//   - merges duplicate literals         : LB [a=w1, b=w2, a=w3] -> LB [a=w1+w3, b=w2]
//   - checks for contradiction, i.e.
//     rule body contains both p and not p and both are needed
//   - replaces weight constraint with cardinality constraint 
//     if all body weights are equal.
//   - replaces weight/cardinality constraint with normal body 
//     if sumW - minW < bound()
bool PrgRule::simplifyWeight(RData& rd, RuleState& rs) {
	if (bodyHasBound() && bound_ <= 0) {
		body.clear();
	}
	weight_t minW    = std::numeric_limits<weight_t>::max();
	weight_t maxW    = 0;
	weight_t oldW    = 0;
	weight_t w       = 0;
	weight_t BOUND   = bodyHasBound() ? bound() : std::numeric_limits<weight_t>::max();
	weight_t realSum = 0;
	WeightLitVec::iterator j = body.begin();
	for (WeightLitVec::iterator it = body.begin(), bEnd = body.end(); it != bEnd; ++it) {
		if (it->second == 0) continue;            // skip irrelevant lits
		it->second = std::min(it->second, BOUND); // reduce weights to bound
		w          = it->second;
		if (!rs.inBody(it->first)) {  // a not seen yet
			*j++  = *it;                // keep it and 
			rs.addToBody(it->first);    // mark as seen
			if (it->first.sign()) { rd.hash += hashId(-it->first.var()); }
			else                  { ++rd.posSize; rd.hash += hashId(it->first.var()); }
			if (w > maxW) { maxW = w; }
			if (w < minW) { minW = w; }
		}
		else { // Merge duplicate lits
			if (type_ == CONSTRAINTRULE) type_ = WEIGHTRULE;
			WeightLitVec::iterator lit = std::find_if(body.begin(), j, compose1(
				std::bind2nd(std::equal_to<Literal>(), it->first),
				select1st<WeightLiteral>()));
			assert(lit != j);
			oldW         = lit->second;
			assert((INT_MAX-oldW) >= it->second && "Integer overflow!");
			lit->second  = std::min(it->second + lit->second, BOUND);
			it->second   = lit->second;     // store merged weight
			w            = lit->second-oldW;// and "new" part
			if (lit->second > maxW) { maxW = lit->second; }
			if (oldW == minW && bodyHasBound()) {
				// we changed the weight of a literal that previously had the minimal weight
				minW    = lit->second;
				for (WeightLitVec::iterator t = body.begin(); t != j && minW > 1; ++t) {
					if (t->second < minW) { minW = t->second; }
				}
			}
		}
		rd.sumWeight += w;
		if (!rs.inBody(~it->first)) {
			realSum    += w;
		}
		else {
			// body contains x and ~x: we can achieve at most max(weight(x), weight(~x))
			// it->second is the weight of the current (potentially merged) literal,
			// the weight of ~it was already added. Increase realSum only if
			// it->second > ~it.second
			weight_t diff = it->second - weight(it->first.var(), it->first.sign());
			if (diff > 0) realSum += diff;
		}
	}
	body.erase(j, body.end());
	if (bodyHasBound()) {
		if (realSum < bound()) {
			return false;
		}
		else if (rd.sumWeight - minW < bound()) {
			type_        = BASICRULE;
			bound_       = (weight_t)body.size();
			rd.sumWeight = (weight_t)body.size();
		}
		else if (type_ == WEIGHTRULE && minW == maxW) {
			type_       = CONSTRAINTRULE;
			bound_      = (bound_+(minW-1))/minW;
			rd.sumWeight= (weight_t)body.size();
		}
	}
	return true;
}


weight_t PrgRule::weight(Var id, bool pos) const {
	if (!bodyHasWeights()) {
		return 1;
	}
	return std::find_if(body.begin(), body.end(), compose1(
		std::bind2nd(std::equal_to<Literal>(), Literal(id, pos==false)),
		select1st<WeightLiteral>()
		))->second;
}

/////////////////////////////////////////////////////////////////////////////////////////
// class PrgRuleTransform
//
// class for transforming extended rules to normal rules
/////////////////////////////////////////////////////////////////////////////////////////
class PrgRuleTransform::Impl {
public:
	Impl(ProgramBuilder& prg, PrgRule& r);
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
	void   createRule(Var head, Literal* bodyFirst, Literal* bodyEnd) const;
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
	TodoList        todo_;    // heads todo
	ProgramBuilder& prg_;     // program to which rules are headed
	PrgRule&        rule_;    // rule to translate
	Var*            aux_;     // newly created atoms for one level of the tree
	weight_t*       sumW_;    // achievable weight for individual literals
};

PrgRuleTransform::PrgRuleTransform() : impl_(0) {}
PrgRuleTransform::~PrgRuleTransform() { delete impl_; }

uint32 PrgRuleTransform::transform(ProgramBuilder& prg, PrgRule& rule) {
	if (rule.type() == CHOICERULE) {
		return transformChoiceRule(prg, rule);
	}
	delete impl_;
	impl_ = new Impl(prg, rule);
	return impl_->transform();
}

PrgRuleTransform::Impl::Impl(ProgramBuilder& prg, PrgRule& r)
	: prg_(prg)
	, rule_(r) {
	aux_     = new Var[r.bound()];
	sumW_    = new weight_t[r.body.size()+1];
	std::memset(aux_ , 0, r.bound()*sizeof(Var));
	
	if (r.type() == WEIGHTRULE) {
		std::stable_sort(r.body.begin(), r.body.end(), compose22(
				std::greater<weight_t>(),
				select2nd<WeightLiteral>(),
				select2nd<WeightLiteral>()));
	}
	uint32 i      = (uint32)r.body.size();
	weight_t wSum = 0;
	sumW_[i]      = 0;
	while (i-- != 0) {
		wSum += r.body[i].second;
		sumW_[i] = wSum;
	}
}
PrgRuleTransform::Impl::~Impl() {
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
uint32 PrgRuleTransform::Impl::transform() {
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

uint32 PrgRuleTransform::Impl::addRule(Var head, bool addLit, const TodoItem& aux) {
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

void PrgRuleTransform::Impl::createRule(Var head, Literal* bodyFirst, Literal* bodyEnd) const {
	prg_.startRule(BASICRULE);
	prg_.addHead(head);
	while (bodyFirst != bodyEnd) {
		prg_.addToBody(bodyFirst->var(), !bodyFirst->sign());
		++bodyFirst;
	}
	prg_.endRule();
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
uint32 PrgRuleTransform::transformNoAux(ProgramBuilder& prg, PrgRule& rule) {
	assert(rule.type() == WEIGHTRULE || rule.type() == CONSTRAINTRULE);
	if (rule.type() == WEIGHTRULE) {
		std::stable_sort(rule.body.begin(), rule.body.end(), compose22(
			std::greater<weight_t>(),
			select2nd<WeightLiteral>(),
			select2nd<WeightLiteral>()));
	}
	WeightVec sumWeights(rule.body.size() + 1, 0);
	LitVec::size_type i = (uint32)rule.body.size();
	weight_t sum = 0;
	while (i != 0) {
		--i;
		sum += rule.body[i].second;
		sumWeights[i] = sum;
	}

	uint32 newRules = 0;
	VarVec    nextStack;
	WeightVec weights;
	PrgRule r(BASICRULE);
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
uint32 PrgRuleTransform::transformChoiceRule(ProgramBuilder& prg, PrgRule& rule) const {
	uint32 newRules = 0;
	Var extraHead = ((rule.heads.size() * (rule.body.size()+1)) + rule.heads.size()) > (rule.heads.size()*3)+rule.body.size()
			? prg.newAtom()
			: varMax;
	PrgRule r1, r2;
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


}
