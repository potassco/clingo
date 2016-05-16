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

#ifndef _GRINGO_OUTPUT_AGGREGATES_HH
#define _GRINGO_OUTPUT_AGGREGATES_HH

#include <gringo/terms.hh>
#include <gringo/domain.hh>
#include <gringo/intervals.hh>
#include <gringo/output/literal.hh>

namespace Gringo { namespace Output {

struct TupleId;
using BodyAggregateElements = UniqueVec<std::pair<TupleId, Formula>, HashFirst<TupleId>, EqualToFirst<TupleId>>;
using HeadFormula = std::vector<std::pair<LiteralId, ClauseId>>;
using HeadAggregateElements = UniqueVec<std::pair<TupleId, HeadFormula>, HashFirst<TupleId>, EqualToFirst<TupleId>>;
using DisjunctiveBounds = IntervalSet<Symbol>;
using Interval = DisjunctiveBounds::Interval;
using ConjunctiveBounds = std::vector<std::pair<Interval, Interval>>;
using PlainBounds = std::vector<std::pair<Relation, Symbol>>;
using LitValVec = std::vector<std::pair<LiteralId, Symbol>>;
using LitUintVec = std::vector<std::pair<LiteralId, unsigned>>;

struct AggregateAnalyzer {
    enum Monotonicity { MONOTONE, ANTIMONOTONE, CONVEX, NONMONOTONE };
    enum WeightType { MIXED, POSITIVE, NEGATIVE };
    enum Truth { True, False, Open };
    using ConjunctiveBounds = std::vector<std::pair<Interval, Interval>>;

    AggregateAnalyzer(DomainData &data, NAF naf, DisjunctiveBounds const &disjunctiveBounds, AggregateFunction fun, Interval range, BodyAggregateElements const &elems);
    void print(std::ostream &out);
    LitValVec translateElems(DomainData &data, Translator &x, AggregateFunction fun, BodyAggregateElements const &bdElems, bool incomplete);

    Monotonicity monotonicity;
    WeightType weightType;
    Truth truth;
    ConjunctiveBounds bounds;
    Interval range;
};

inline Symbol getNeutral(AggregateFunction fun) {
    switch (fun) {
        case AggregateFunction::COUNT:
        case AggregateFunction::SUMP:
        case AggregateFunction::SUM: { return Symbol::createNum(0); }
        case AggregateFunction::MIN: { return Symbol::createSup(); }
        case AggregateFunction::MAX: { return Symbol::createInf(); }
    }
    assert(false);
    return {};
}

LiteralId getEqualClause(DomainData &data, Translator &x, std::pair<Id_t, Id_t> clause, bool conjunctive, bool equivalence);
LiteralId getEqualFormula(DomainData &data, Translator &x, Formula const &formula, bool conjunctive, bool equivalence);
LiteralId getEqualAggregate(DomainData &data, Translator &x, AggregateFunction fun, NAF naf, DisjunctiveBounds const &bounds, Interval const &range, BodyAggregateElements const &bdElems, bool recursive);

class MinMaxTranslator {
public:
    LiteralId translate(DomainData &data, Translator &x, AggregateAnalyzer &res, bool isMin, LitValVec &&elems, bool incomplete);
};

struct SumTranslator {
    SumTranslator() { }
    void addLiteral(DomainData &data, LiteralId const &lit, Potassco::Weight_t weight, bool recursive);
    void translate(DomainData &data, Translator &x, LiteralId const &head, Potassco::Weight_t bound, LitUintVec const &litsPosRec, LitUintVec const &litsNegRec, LitUintVec const &litsPosStrat, LitUintVec const &litsNegStrat);
    LiteralId translate(DomainData &data, Translator &x, ConjunctiveBounds &bounds, bool convex, bool invert);

    LitUintVec litsPosRec;
    LitUintVec litsNegRec;
    LitUintVec litsPosStrat;
    LitUintVec litsNegStrat;
};

} } // namespace Output Gringo

#endif // _GRINGO_OUTPUT_AGGREGATES_HH
