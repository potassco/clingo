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

#ifndef CLASP_PROGRAM_RULE_H_INCLUDED
#define CLASP_PROGRAM_RULE_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif
#include <clasp/literal.h>
namespace Clasp {

class ProgramBuilder;


//! Supported rule-types.
/**
 * \ingroup problem
 */
enum RuleType{
	ENDRULE         = 0,  /**< Not a valid rule, used as sentinel */
	BASICRULE       = 1,  /**< A normal rule, i.e: A0 :- A1,...,Am, not Am+1,...,not An */
	CONSTRAINTRULE  = 2,  /**< A cardinality constraint, i.e. A0 :- L{A1,...,Am,not Am+1,...,not An} */
	CHOICERULE      = 3,  /**< A choice rule, i.e. {A0,...,An} :- BODY */
	WEIGHTRULE      = 5,  /**< A weight constraint, i.e. A0 :- L[A1=W1,...,Am=Wm,not Am+1=Wm+1,...,not An=Wn] */
	OPTIMIZERULE    = 6   /**< A minimize statement */
};

//! Used during rule simplification
/**
 * \ingroup problem
 */
class RuleState {
public:
	//! Does v appear in the head of the active rule?
	bool inHead(Var v)        { return hasFlag(v, head_flag); }
	//! Does p appear in the body of the active rule?
	bool inBody(Literal p)    { return hasFlag(p.var(), pos_flag+(uint8)p.sign()); }
	//! Mark v as a head of the active rule
	void addToHead(Var v)     { setFlag(v, head_flag); }  
	//! Mark p as a literal contained in the active rule
	void addToBody(Literal p) { setFlag(p.var(), pos_flag+(uint8)p.sign()); }
	//! Remove v from the active rule
	void popFromRule(Var v)   { assert(v < vf_.size()); vf_[v] = 0; }
	//! Returns the number of variables known to this object
	uint32 size() const       { return (uint32)vf_.size(); }
	//! Clears all flags
	void clear()              { VarFlags().swap(vf_); }
	
	//! Checks whether the head-atom id is superfluous w.r.t the body
	/*!
	 * A head atom is superfluous if it is needed to satisfy the body (in that case, the
	 * body can't define the head atom) or, if the rule is a choice rule, if it appears
	 * in the body. 
	 * \pre The body literals of the rule to be checked were marked using addToBody
	 */
	template <class W>
	bool superfluousHead(bool choice, weight_t sumW, weight_t bound, Var id, const W& weights) {
		// choice-rule: ignore heads that appear in body
		// other rules: ignore heads that appear in *positive* body and that are needed
		// to make body true.
		return (choice && (inBody(posLit(id)) || inBody(negLit(id))))
			|| (inBody(posLit(id)) && (sumW - weights(posLit(id)) < bound));
	}

	//! Checks whether the rule id :- B is selfblocking, i.e. if the body is false, whenever id is true
	/*!
	 * \pre The body literals of the rule to be checked were marked using addToBody
	 */
	template <class W>
	bool selfblocker(bool choice, weight_t sumW, weight_t bound, Var id, const W& weights) {
		// a is part of B- and needed to make body true
		return !choice
			&& inBody(negLit(id))                 
			&& (sumW - weights(negLit(id)) < bound);
	}
private:
	bool hasFlag(Var v, uint8 f) { grow(v); return (vf_[v] & f) != 0; }
	void setFlag(Var v, uint8 f) { grow(v); vf_[v] |= f; }
	void grow(Var v)             { if (v >= vf_.size()) { vf_.resize(v+1); } }
	typedef PodVector<uint8>::type VarFlags;
	static const uint8 pos_flag   = 0x1u;
	static const uint8 neg_flag   = 0x2u;
	static const uint8 head_flag  = 0x4u;
	VarFlags vf_;
};

//! A rule of a logic program
/*!
 * \ingroup problem
 * Objects of this class represent one rule of a logic program.
 * The task of this class is to simplify a rule and to translate it into an
 * equivalent set of normal rules if requested.
 */
class PrgRule {
public:
	//! Objects of this type store information about a simplified rule
	struct RData {
		uint32   hash;       // hash value of the rule body
		weight_t sumWeight;  // max achievable weight
		uint32   posSize:30; // size of positive body
		uint32   value  : 2; // truth-value of the body
	};
	explicit PrgRule(RuleType t = ENDRULE) 
		: bound_(0)
		, type_(t) {
	}
	//! resets the rule
	void clear();
	
	//! swaps *this with o
	void swap(PrgRule& o) {
		std::swap(heads, o.heads);
		std::swap(body, o.body);
		std::swap(bound_, o.bound_);
		std::swap(type_, o.type_);
	}

	// logic
	//! Sets the type of the rule
	/*! 
	 * \pre t != ENDRULE
	 */
	PrgRule& setType(RuleType t) {
		assert(t != ENDRULE && "Precondition violated\n");
		type_ = t;
		return *this;
	}
	//! returns the rule's type
	/*!
	 * \note if simplification detected the rule to be irrelevant, ENDRULE is returned
	 */
	RuleType type() const { return type_; }
	
	//! Sets the lower bound of the rule
	/*!
	 * \pre bound >= 0
	 * \note the bound is ignored if type() is either a basic, choice, or optimize rule
	 */
	PrgRule& setBound(weight_t bound) {
		assert(bound >= 0 && "Precondition violated!\n");
		bound_ = bound;
		return *this;
	}
	//! returns the lower bound of the rule
	/*!
	 * \note for basic and choice rules, the lower bound is equal to the size of the rule's body.
	 */
	weight_t bound() const { return bound_; }

	//! Adds v as a head of this rule
	PrgRule& addHead(Var v);
	//! Adds v to the positive/negative body of the rule
	PrgRule& addToBody(Var v, bool pos, weight_t w=1);

	//! Simplifies the rule and returns information about the simplified rule
	RData   simplify(RuleState& r);
	
	VarVec        heads;        /**< list of rule heads (note: conjunctive heads!) */
	WeightLitVec  body;         /**< body of this rule */
private:
	PrgRule(const PrgRule&);
	PrgRule& operator=(const PrgRule&);
	struct Weights {
		Weights(const PrgRule& self) : self_(&self) {}
		weight_t operator()(Literal p) const {
			return self_->weight(p.var(), p.sign() == false);
		}
		const PrgRule* self_;
	};
	bool     bodyHasWeights() const { return type_ == WEIGHTRULE || type_ == OPTIMIZERULE;   }
	bool     bodyHasBound()   const { return type_ == WEIGHTRULE || type_ == CONSTRAINTRULE; }
	bool     bodyIsSet()      const { return type_ == BASICRULE  || type_ == CHOICERULE;     }
	bool     selfblocker(RuleState& r, Var aId, weight_t sw) const;
	bool     simplifyNormal(RData& rd, RuleState& rs);
	bool     simplifyWeight(RData& rd, RuleState& rs);

	weight_t weight(Var id, bool pos) const;
	weight_t bound_; // lower bound (i.e. number/weight of lits that must be true before rule fires)
	RuleType type_;  // type of rule
};

class PrgRuleTransform {
public:
	PrgRuleTransform();
	~PrgRuleTransform();
	uint32 transform(ProgramBuilder& prg, PrgRule& rule);
	uint32 transformNoAux(ProgramBuilder& prg, PrgRule& rule);
private:
	PrgRuleTransform(const PrgRuleTransform&);
	PrgRuleTransform& operator=(const PrgRuleTransform&);
	uint32 transformChoiceRule(ProgramBuilder& prg, PrgRule& rule) const;
	class Impl;
	Impl* impl_;
};

}
#endif
