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
#include <gringo/unique_list.hh>
#include <gringo/output/literal.hh>

namespace Gringo { namespace Output {

using ULitValVec        = std::vector<std::pair<ULit, Value>>;
using BdAggrElemSet     = unique_list<std::pair<FWValVec, std::vector<ULitVec>>, extract_first<FWValVec>>;
using Interval          = IntervalSet<Value>::Interval;
using DisjunctiveBounds = IntervalSet<Value>;
using ConjunctiveBounds = std::vector<std::pair<Interval, Interval>>;
using ULitUintVec       = std::vector<std::pair<ULit,unsigned>>;
using IntVec            = std::vector<int>;

struct AggregateAnalyzer {
    enum Monotonicity { MONOTONE, ANTIMONOTONE, CONVEX, NONMONOTONE };
    enum WeightType { MIXED, POSITIVE, NEGATIVE };
    enum Truth { TRUE, FALSE, OPEN };
    using ConjunctiveBounds = std::vector<std::pair<Interval, Interval>>;

    AggregateAnalyzer(NAF naf, DisjunctiveBounds const &disjunctiveBounds, AggregateFunction fun, Interval range, BdAggrElemSet const &elems);
    void print(std::ostream &out);
    ULitValVec translateElems(LparseTranslator &x, AggregateFunction fun, BdAggrElemSet const &bdElems, bool incomplete);

    Monotonicity monotonicity;
    WeightType weightType;
    Truth truth;
    ConjunctiveBounds bounds;
    Interval range;
};

struct MinMaxTranslator {
    ULit translate(LparseTranslator &x, AggregateAnalyzer &res, bool isMin, ULitValVec &&elems, bool incomplete);
};

struct SumTranslator {
    SumTranslator() { }
    void addLiteral(LparseTranslator &x, ULit const &lit, int weight, bool recursive);
    void translate(LparseTranslator &x, ULit const &head, int bound, ULitUintVec const &litsPosRec, ULitUintVec const &litsNegRec, ULitUintVec const &litsPosStrat, ULitUintVec const &litsNegStrat);
    ULit translate(LparseTranslator &x, ConjunctiveBounds &bounds, bool convex, bool invert);

    ULitUintVec litsPosRec;
    ULitUintVec litsNegRec;
    ULitUintVec litsPosStrat;
    ULitUintVec litsNegStrat;
};

ULit getEqualClause(LparseTranslator &x, ULitVec &&clause, bool conjunctive, bool equivalence);
ULit getEqualClause(LparseTranslator &x, ULitVec const &clause, bool conjunctive, bool equivalence);
ULit getEqualFormula(LparseTranslator &x, std::vector<ULitVec> const &clauses, bool conjunctive, bool equivalence);
ULit getEqualAggregate(LparseTranslator &x, AggregateFunction fun, NAF naf, DisjunctiveBounds const &bounds, Interval const &range, BdAggrElemSet const &bdElems, bool incomplete);

} } // namespace Output Gringo

#endif // _GRINGO_OUTPUT_AGGREGATES_HH
