// {{{ GPL License 

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Roland Kaminski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// }}}

#ifndef _GRINGO_OUTPUT_STATEMENTS_HH
#define _GRINGO_OUTPUT_STATEMENTS_HH

#include <gringo/output/statement.hh>
#include <gringo/output/literals.hh>
#include <gringo/unique_list.hh>

namespace Gringo { namespace Output {

// {{{ declaration of Rule

struct Rule : Statement {
    Rule();
    Rule(PredicateDomain::element_type *head, ULitVec &&body);
    virtual void printPlain(std::ostream &out) const;
    virtual void toLparse(LparseTranslator &x);
    virtual void printLparse(LparseOutputter &out) const;
    virtual Rule *clone() const;
    virtual bool isIncomplete() const;
    virtual ~Rule();
    
    PredicateDomain::element_type *head = nullptr;
    ULitVec body;
};

struct RuleRef : Statement {
    virtual void printPlain(std::ostream &out) const;
    virtual Rule *clone() const;
    virtual void toLparse(LparseTranslator &x);
    virtual void printLparse(LparseOutputter &out) const;
    virtual bool isIncomplete() const;
    virtual ~RuleRef();
    
    PredicateDomain::element_type *head = nullptr;
    LitVec body;
};
// }}}
// {{{ declaration of LparseRule

struct LparseRule : Statement {
    using HeadVec = std::vector<std::reference_wrapper<PredicateDomain::element_type>>;

    LparseRule(HeadVec &&head, SAuxAtomVec &&auxHead, ULitVec &&body, bool choice);
    virtual void printPlain(std::ostream &out) const;
    virtual void toLparse(LparseTranslator &x);
    virtual void printLparse(LparseOutputter &out) const;
    virtual LparseRule *clone() const;
    virtual bool isIncomplete() const;
    virtual ~LparseRule();
    
    bool        choice;
    HeadVec     head;
    SAuxAtomVec auxHead;
    ULitVec     body;
};

// }}}
// {{{ declaration of WeightRule

struct WeightRule : Statement {
    using ULitBoundVec = std::vector<std::pair<ULit, unsigned>>;

    WeightRule(SAuxAtom head, unsigned lower, ULitBoundVec &&body);
    virtual void printPlain(std::ostream &out) const;
    virtual void toLparse(LparseTranslator &x);
    virtual void printLparse(LparseOutputter &out) const;
    virtual WeightRule *clone() const;
    virtual bool isIncomplete() const;
    virtual ~WeightRule();
    
    SAuxAtom     head;
    ULitBoundVec body;
    unsigned     lower;
};

// }}}
// {{{ declaration of HeadAggregateState

struct HeadAggregateElement {
    struct Cond {
        Cond(PredicateDomain::element_type *head, unsigned headNum, Output::ULitVec &&lits);
        size_t hash() const;
        bool operator==(Cond const &x) const;
        PredicateDomain::element_type *head;
        unsigned                       headNum;
        Output::ULitVec                lits;
    };
    using CondVec = std::vector<Cond>;

    CondVec        conds;
    unsigned       imported = 0;
    bool           fact     = false;
};

struct HeadAggregateState {
    using Bounds       = IntervalSet<Value>;
    using ElemSet      = unique_list<std::pair<FWValVec, HeadAggregateElement>, extract_first<FWValVec>>;
    using element_type = std::pair<Value const, HeadAggregateState>;

    // Note has much in common with BodyAggregateState
    HeadAggregateState();
    HeadAggregateState(AggregateFunction fun, unsigned generation);
    void accumulate(ValVec const &tuple, AggregateFunction fun, PredicateDomain::element_type *head, unsigned headNum, LitVec const &lits, Location const &loc);
    bool defined() const;
    Bounds::Interval range(AggregateFunction fun) const;
    unsigned generation() const;
    bool fact(bool) const;
    static element_type &ignore();

    Bounds  bounds;
    ElemSet elems;
    union {
        int64_t    intMin;
        Value::POD valMin;
    };
    union {
        int64_t    intMax;
        Value::POD valMax;
    };
    unsigned _generation;
    bool     todo        = false;
};

// }}}
// {{{ declaration of HeadAggregateRule

struct HeadAggregateRule : Statement {
    using BoundsVec = std::vector<std::pair<Relation, Value>>;

    static void printElem(std::ostream &out, HeadAggregateState::ElemSet::value_type const &x);
    virtual void toLparse(LparseTranslator &x);
    virtual void printPlain(std::ostream &out) const;
    virtual void printLparse(LparseOutputter &out) const;
    virtual bool isIncomplete() const;
    virtual HeadAggregateRule *clone() const;
    virtual ~HeadAggregateRule();

    BoundsVec           bounds;
    AggregateFunction   fun        = AggregateFunction::COUNT;
    HeadAggregateState *repr       = nullptr;
    ULitVec             body;
};

// }}}
// {{{ declaration of DisjunctionState

struct DisjunctionElement {
    using ULitVecList = unique_list<ULitVec, identity<ULitVec>, value_hash<ULitVec>, value_equal_to<ULitVec>>;

    DisjunctionElement(Value repr);
    DisjunctionElement(DisjunctionElement &&);
    DisjunctionElement &operator=(DisjunctionElement &&);
    operator Value const & () const;
    void print(std::ostream &out) const;

    Value repr;
    ULitVecList heads;
    ULitVecList bodies;
};

inline std::ostream &operator<<(std::ostream &out, DisjunctionElement const &x) {
    x.print(out);
    return out;
}

struct DisjunctionState {
    using element_type = std::pair<Value const, DisjunctionState>;
    using Elem         = DisjunctionElement;
    using ElemSet      = unique_list<Elem, identity<Value>>;

    DisjunctionState();
    unsigned generation() const { return _generation - 2; }
    void generation(unsigned x) { _generation = x + 2; }
    bool defined() const  { return _generation > 1; }
    bool enqueued() const { return _generation == 1; }
    void enqueue() {
        assert(_generation == 0);
        _generation = 1;
    }
    void dequeue() {
        assert(_generation == 1);
        _generation = 0;
    }
    bool fact(bool) const;
    static element_type &ignore();

    ElemSet  elems;
    unsigned _generation = 0;
    unsigned imported    = 0;
    bool     todo        = false;
};

// }}}
// {{{ declaration of DisjunctionRule

struct DisjunctionRule : Statement {
    virtual void toLparse(LparseTranslator &x);
    virtual void printPlain(std::ostream &out) const;
    virtual void printLparse(LparseOutputter &out) const;
    virtual bool isIncomplete() const;
    virtual DisjunctionRule *clone() const;
    virtual ~DisjunctionRule();

    DisjunctionState *repr = nullptr;
    ULitVec           body;
};

// }}}
// {{{ declaration of Minimize

struct Minimize : Statement {
    virtual void toLparse(LparseTranslator &x);
    virtual void printPlain(std::ostream &out) const;
    virtual void printLparse(LparseOutputter &out) const;
    virtual bool isIncomplete() const;
    virtual Minimize *clone() const;
    virtual ~Minimize();

    MinimizeList elems;
};

// }}}
// {{{ declaration of LparseMinimize

using ULitWeightVec = std::vector<std::pair<ULit, unsigned>>;
struct LparseMinimize : Statement {
    LparseMinimize(Value prio, ULitWeightVec &&lits);
    virtual void toLparse(LparseTranslator &x);
    virtual void printPlain(std::ostream &out) const;
    virtual void printLparse(LparseOutputter &out) const;
    virtual bool isIncomplete() const;
    virtual LparseMinimize *clone() const;
    virtual ~LparseMinimize();

    Value prio;
    ULitWeightVec lits;
};

// }}}
// {{{ declaration of LparseRuleCreator

struct LparseRuleCreator {
    LparseRuleCreator(bool choice = false) : choice(choice) { }
    LparseRuleCreator &addHead(ULitVec &&x) {
        std::move(x.begin(), x.end(), std::back_inserter(head));
        return *this;
    }
    LparseRuleCreator &addHead(ULit &&x) {
        head.emplace_back(std::move(x));
        return *this;
    }
    LparseRuleCreator &addHead(ULit const &x) {
        head.emplace_back(get_clone(x));
        return *this;
    }
    LparseRuleCreator &addBody(ULitVec &&x) {
        std::move(x.begin(), x.end(), std::back_inserter(body));
        return *this;
    }
    LparseRuleCreator &addBody(ULit &&x) {
        body.emplace_back(std::move(x));
        return *this;
    }
    LparseRuleCreator &addBody(ULit const &x) {
        body.emplace_back(get_clone(x));
        return *this;
    }
    void toLparse(LparseTranslator &x, bool shift=false) {
        if (shift) {
            for (auto &lit : head) {
                if (x.isAtomFromPreviousStep(lit)) {
                    lit->invert();
                    lit->invert();
                }
            }
        }
        LparseRule::HeadVec predHead;
        SAuxAtomVec auxHead;
        for (auto &lit : head) {
            if (PredicateDomain::element_type *atom = lit->isAtom()) {
                predHead.emplace_back(*atom);
            }
            else if (SAuxAtom atom = lit->isAuxAtom()) {
                auxHead.emplace_back(atom);;
            }
            else {
                body.emplace_back(lit->negateLit(x));
            }
        }
        LparseRule(std::move(predHead), std::move(auxHead), std::move(body), choice).toLparse(x);;
        head.clear();
        body.clear();
    }

    bool choice;
    ULitVec head;
    ULitVec body;
};

using LRC = LparseRuleCreator;

// }}}

} } // namespace Output Gringo

#endif // _GRINGO_OUTPUT_STATEMENTS_HH
