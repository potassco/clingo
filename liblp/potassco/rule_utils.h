// 
// Copyright (c) 2016, Benjamin Kaufmann
// 
// This file is part of Potassco. See http://potassco.sourceforge.net/
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// 
#ifndef LIBLP_RULE_UTILS_H_INCLUDED
#define LIBLP_RULE_UTILS_H_INCLUDED
#include <potassco/match_basic_types.h>
#include <new>
namespace Potassco {

//! A sum aggregate with a lower bound.
struct Sum_t {
	WeightLitSpan lits;
	Weight_t      bound;
};
//! A type that can represent an aspif rule.
struct Rule_t {
	Head_t   ht;
	AtomSpan head;
	Body_t   bt;
	union {
		LitSpan cond;
		Sum_t   agg;
	};
	static Rule_t normal(Head_t ht, const AtomSpan& head, const LitSpan& body);
	static Rule_t sum(Head_t ht, const AtomSpan& head, const Sum_t& sum);
	static Rule_t sum(Head_t ht, const AtomSpan& head, Weight_t bound, const WeightLitSpan& lits);
	bool normal() const { return bt == Body_t::Normal; }
	bool sum()    const { return bt != Body_t::Normal; }
};

//! A builder class for creating a rule.
class RuleBuilder {
public:
	RuleBuilder();
	/*!
	 * \name Start functions
	 * Functions for starting the definition of a rule's head or body.
	 * If the active rule is frozen (i.e. end() was called), the active
	 * rule is discarded.
	 * \note The body of a rule can be defined before or after its head is defined
	 * but definitions of head and body must not be mixed.
	 */
	//@{
	//! Start definition of the rule's head, which can be either disjunctive or a choice.
	RuleBuilder& start(Head_t ht = Head_t::Disjunctive);
	//! Start definition of a minimize rule. No head allowed.
	RuleBuilder& startMinimize(Weight_t prio);
	//! Start definition of a conjunction to be used as the rule's body.
	RuleBuilder& startBody();
	//! Start definition of a sum aggregate to be used as the rule's body.
	RuleBuilder& startSum(Weight_t bound);
	//@}

	/*!
	 * \name Update functions
	 * Functions for adding elements to the active rule.
	 * \note Update function shall not be called once a rule is frozen.
	 * \note Calling an update function implicitly starts the definition of the
	 * corresponding rule part.
	 */
	//@{
	//! Add a to the rule's head.
	RuleBuilder& addHead(Atom_t a);
	//! Add lit to the rule's body.
	RuleBuilder& addGoal(Lit_t lit);
	//! Add lit with given weight to rule's body if body is a sum aggregate or rule is a minimize rule.
	RuleBuilder& addGoal(Lit_t lit, Weight_t w);
	RuleBuilder& addGoal(WeightLit_t lit);
	//! Update lower bound of sum aggregate.
	RuleBuilder& setBound(Weight_t bound);
	//@}
	
	//! Stop definition of rule and add rule to out if given.
	/*!
	 * Once end() was called, the active rule is considered frozen.
   */
	RuleBuilder& end(AbstractProgram* out = 0);
	//! Discard active rule.
	RuleBuilder& clear();
	//! Discard body of active rule but keep head if any.
	RuleBuilder& clearBody();
	//! Weaken active sum aggregate body to a normal body or count aggregate.
	RuleBuilder& weaken(Body_t to, bool resetWeights = true);
	
	/*!
	 * \name Query functions
	 * Functions for accessing parts of the active rule.
	 * \note The result of these functions is only valid until the next call to
	 * an update function.
	 */
	//@{
	uint32_t     headSize() const;
	Atom_t*      head()     const;
	uint32_t     bodySize() const;
	Body_t       bodyType() const;
	Lit_t*       body()     const;
	WeightLit_t* sum()      const;
	Weight_t     bound()    const;
	Rule_t       rule()     const;
	//@}
private:
	struct RuleInfo;
	struct Data : RawStack {
		template <class T>
		T* push() { return new (this->get(this->push_(sizeof(T)))) T(); }
	};
	void require(bool, const char*) const;
	void endHead();
	void endBody();
	RuleInfo* startBody(Body_t bt, Weight_t bnd);
	RuleInfo* init();
	RuleInfo* info() const;
	Data data_;
};

}
#endif
