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

#include <gringo/output/aggregates.hh>
#include <gringo/output/statements.hh>

namespace Gringo { namespace Output {

// {{{1 definition of AggregateAnalyzer

void AggregateAnalyzer::print(std::ostream &out) {
    auto printInterval = [&](Interval const &x) {
        out << (x.left.inclusive ? "[" : "(");
        out << x.left.bound;
        out << ",";
        out << x.right.bound;
        out << (x.right.inclusive ? "]" : ")");
    };
    out << "analyze result: " << std::endl;
    out << "  range: ";
    printInterval(range);
    std::cerr << std::endl;
    std::cerr << "  bounds:" << std::endl;
    for (auto &x : bounds) {
        out << "    ";
        printInterval(x.first);
        out << " ";
        printInterval(x.second);
        out << std::endl;
    }
    out << "  monotonicity: ";
    switch (monotonicity) {
        case Monotonicity::MONOTONE:     { out << "monotone"; break; }
        case Monotonicity::ANTIMONOTONE: { out << "antimonotone"; break; }
        case Monotonicity::CONVEX:       { out << "convex"; break; }
        case Monotonicity::NONMONOTONE:  { out << "nonmonotone"; break; }
    }
    out << std::endl;
    out << "  weights: ";
    switch (weightType) {
        case WeightType::POSITIVE: { out << "positive"; break; }
        case WeightType::NEGATIVE: { out << "negative"; break; }
        case WeightType::MIXED:    { out << "mixed"; break; }
    }
    out << std::endl;
    out << "  truth: ";
    switch (truth) {
        case Truth::TRUE:  { out << "true"; break; }
        case Truth::OPEN:  { out << "open"; break; }
        case Truth::FALSE: { out << "false"; break; }
    }
    out << std::endl;
}

AggregateAnalyzer::AggregateAnalyzer(NAF naf, DisjunctiveBounds const &disjunctiveBounds, AggregateFunction fun, Interval range, BdAggrElemSet const &elems)
: range(range) {
    // NOTE: considers everything that is fixed in a reduct as ANTIMONOTONE
    weightType = POSITIVE;
    IntervalSet<Value> numBounds;
    for (auto &y : disjunctiveBounds) {
        Interval x = y;
        if (!x.right.inclusive && x.right.bound.type() == Value::NUM) {
            x.right.inclusive = true;
            x.right.bound = Value::createNum(x.right.bound.num() - 1);
        }
        if (!x.left.inclusive && x.left.bound.type() == Value::NUM) {
            x.left.inclusive = true;
            x.left.bound = Value::createNum(x.left.bound.num() + 1);
        }
        numBounds.add(x);
    }
    IntervalSet<Value> complement(IntervalSet<Value>(range).difference(numBounds));
    bool nonMonotone = false;
    for (auto &y : complement) {
        Interval x = y;
        if (x.right.inclusive && x.right.bound.type() == Value::NUM) {
            x.right.inclusive = false;
            x.right.bound = Value::createNum(x.right.bound.num() + 1);
        }
        if (x.left.inclusive && x.left.bound.type() == Value::NUM) {
            x.left.inclusive = false;
            x.left.bound = Value::createNum(x.left.bound.num() - 1);
        }
        IntervalSet<Value>::Interval a, b;
        a.left  = range.left;
        a.right = x.left;
        b.left  = x.right;
        b.right = range.right;
        if (a.empty() && b.empty()) {
            truth        = FALSE;
            monotonicity = ANTIMONOTONE;
            bounds.clear();
            bounds.emplace_back(a, b);
            return;
        }
        bounds.emplace_back(a, b);
        if (!a.empty() && !b.empty()) {
            nonMonotone = true;
        }
    }
    if (bounds.empty()) {
        truth        = TRUE;
        monotonicity = MONOTONE;
    }
    else {
        truth = OPEN;
        bool hasPositive       = false;
        bool hasPositiveWeight = false;
        bool hasNegativeWeight = false;
        for (auto &x : elems) {
            bool hasPositiveLiteral = false;
            for (auto &y : x.second) {
                for (auto &z : y) {
                    if (z->isPositive()) {
                        hasPositive        = true;
                        hasPositiveLiteral = true;
                        break;
                    }
                }
                if (hasPositiveLiteral && !x.first.empty() && x.first.front().type() == Value::NUM) {
                    if (x.first.front().num() < 0) { hasNegativeWeight = true; }
                    if (x.first.front().num() > 0) { hasPositiveWeight = true; }
                }
            }
        }
        if (fun == AggregateFunction::SUM) {
            if (hasNegativeWeight && hasPositiveWeight) {
                weightType = MIXED;
                nonMonotone = true;
            }
            if (hasNegativeWeight && !hasPositiveWeight) {
                weightType = NEGATIVE;
            }
        }
        if (naf != NAF::POS || !hasPositive)  { monotonicity = ANTIMONOTONE; }
        else if (nonMonotone) { monotonicity = NONMONOTONE; }
        else if (bounds.size() == 1) {
            bool flip = fun == AggregateFunction::MIN || (fun == AggregateFunction::SUM && weightType == NEGATIVE);
            if (bounds.front().first.empty()) { flip = !flip; }
            monotonicity = flip ? MONOTONE : ANTIMONOTONE;;
        }
        else {
            monotonicity = CONVEX;
        }
    }
}

ULitValVec AggregateAnalyzer::translateElems(LparseTranslator &x, AggregateFunction fun, BdAggrElemSet const &bdElems, bool incomplete) {
    ULitValVec elems;
    for (auto &y : bdElems) {
        Value weight(getWeight(fun, y.first));
        std::vector<ULitVec> formula = get_clone(y.second);
        ULit lit = getEqualFormula(x, std::move(formula), false, monotonicity == AggregateAnalyzer::NONMONOTONE && incomplete);
        elems.emplace_back(std::move(lit), weight);
    }

    return elems;
}

// {{{1 definition of MinMaxTranslator

ULit MinMaxTranslator::translate(LparseTranslator &x, AggregateAnalyzer &res, bool isMin, ULitValVec &&elems, bool incomplete) {
    // NOTE: passing the elems vec as a list of weighted formulas
    //       could be exploited to add fewer disjunctions than translateElems adds at the moment
    //       (same goes for sum aggregates)
    ULitVec conjunction;
    for (auto &bound : res.bounds) {
        // translation overview (min):
        // |--A--|  B  |--C--|
        // (B => A)
        // note that B is true if C is empty!
        // if not incomplete or not nonmonotone:
        // (~B | A)
        bool hasAntecedent = isMin ? !bound.second.empty() : !bound.first.empty();
        ULitVec antecedent;
        ULitVec consequent;
        for (auto &elem : elems) {
            if (res.range.contains(elem.second)) {
                if (bound.first.contains(elem.second)) {
                    if (isMin) { consequent.emplace_back(get_clone(elem.first)); }
                }
                else if (bound.second.contains(elem.second)) {
                    if (!isMin) { consequent.emplace_back(get_clone(elem.first)); }
                }
                else if (hasAntecedent) { antecedent.emplace_back(get_clone(elem.first)); }
            }
        }
        if (hasAntecedent) {
            assert(!antecedent.empty());
            if (consequent.empty()) {
                for (auto &lit : antecedent) { conjunction.emplace_back(lit->negateLit(x)); }
            }
            else {
                if (incomplete && res.monotonicity == AggregateAnalyzer::NONMONOTONE) {
                    // (a | b | c => x | y | z) => q
                    // is equivalent to
                    //   a | b | c <=> a1.
                    //   x | y | z  => a2.
                    //     a2 => q
                    //    ~a1 => q
                    //   ~~a2 => a1 | q
                    ULit q  = x.makeAux();
                    ULit a1 = getEqualClause(x, std::move(antecedent), false, true);
                    ULit a2 = getEqualClause(x, std::move(consequent), false, false);
                    LRC().addHead(q).addBody(a2).toLparse(x);
                    LRC().addHead(q).addBody(a1->negateLit(x)).toLparse(x);
                    LRC().addHead(q).addHead(a1).addBody(a2->negateLit(x)->negateLit(x)).toLparse(x);
                    conjunction.emplace_back(std::move(q));
                }
                else {
                    ULit aux = x.makeAux();
                    for (auto &lit : consequent) { LRC().addHead(aux).addBody(std::move(lit)).toLparse(x); }
                    LRC negated;
                    for (auto &lit : antecedent) { negated.addBody(lit->negateLit(x)); }
                    negated.addHead(aux).toLparse(x);
                    conjunction.emplace_back(std::move(aux));
                }
            }
        }
        else {
            assert(!consequent.empty());
            conjunction.emplace_back(getEqualClause(x, std::move(consequent), false, false));
        }
    }
    ULit ret = getEqualClause(x, std::move(conjunction), true, false);
    Term::replace(ret, ret->toLparse(x));
    return ret;
}

// {{{1 definition of SumTranslator

void SumTranslator::addLiteral(LparseTranslator &x, ULit const &lit, int weight, bool recursive) {
    if (weight > 0) {
        if (!recursive || lit->invertible() || x.isAtomFromPreviousStep(lit)) {
            litsPosStrat.emplace_back(get_clone(lit), weight);
        }
        else {
            litsPosRec.emplace_back(get_clone(lit), weight);
        }
    }
    else if (weight < 0) {
        if (!recursive || lit->invertible() || x.isAtomFromPreviousStep(lit)) {
            litsNegStrat.emplace_back(get_clone(lit), -weight);
        }
        else {
            litsNegRec.emplace_back(get_clone(lit), -weight);
        }
    }
}

void SumTranslator::translate(LparseTranslator &x, ULit const &head, int bound, ULitUintVec const &litsPosRec, ULitUintVec const &litsNegRec, ULitUintVec const &litsPosStrat, ULitUintVec const &litsNegStrat) {
    ULitUintVec elems;
    for (auto &wLit : litsPosRec)   { elems.emplace_back(get_clone(wLit.first), wLit.second); }
    for (auto &wLit : litsPosStrat) { elems.emplace_back(get_clone(wLit.first), wLit.second); }
    for (auto &wLit : litsNegStrat) {
        bound+= wLit.second;
        elems.emplace_back(wLit.first->negateLit(x), wLit.second);
    }
    for (auto &wLit : litsNegRec) {
        bound+= wLit.second;
        ULit aux = x.makeAux();
        elems.emplace_back(get_clone(aux), wLit.second);
        LRC().addHead(aux).addBody(wLit.first->negateLit(x)).toLparse(x);
        LRC().addHead(aux).addBody(head).toLparse(x);
        LRC().addHead(aux).addHead(wLit.first).addHead(head->negateLit(x)).toLparse(x);
    }
    WeightRule(head->isAuxAtom(), bound, get_clone(elems)).toLparse(x);
}

ULit SumTranslator::translate(LparseTranslator &x, BodyAggregate::ConjunctiveBounds &bounds, bool convex, bool invert) {
    ULitVec clause;
    for (auto &bound : bounds) {
        assert(!bound.first.empty() || !bound.second.empty());
        ULit pos, neg;
        if (!bound.second.empty()) {
            if (invert && convex) {
                if (!neg) { neg = x.makeAux(); }
                int lower = 1 - bound.second.left.bound.num();
                translate(x, neg, lower, litsNegRec, litsPosRec, litsNegStrat, litsPosStrat);
            }
            else {
                if (!pos) { pos = x.makeAux(); }
                int lower = bound.second.left.bound.num();
                translate(x, pos, lower, litsPosRec, litsNegRec, litsPosStrat, litsNegStrat);
            }
        }
        if (!bound.first.empty()) {
            if (!invert && convex) {
                if (!neg) { neg = x.makeAux(); }
                int lower = 1 + bound.first.right.bound.num();
                translate(x, neg,  lower, litsPosRec, litsNegRec, litsPosStrat, litsNegStrat);
            }
            else {
                if (!pos) { pos = x.makeAux(); }
                int lower = -bound.first.right.bound.num();
                translate(x, pos, lower, litsNegRec, litsPosRec, litsNegStrat, litsPosStrat);
            }
        }
        ULitVec disjunction;
        if (pos) { disjunction.emplace_back(std::move(pos)); }
        if (neg) { disjunction.emplace_back(neg->negateLit(x)); }
        clause.emplace_back(getEqualClause(x, std::move(disjunction), false, false));
    }
    return getEqualClause(x, std::move(clause), true, false);
}

// {{{1 definition of translation functions

ULit getEqualClause(LparseTranslator &x, ULitVec &&clause, bool conjunctive, bool equivalence) {
    if (clause.empty()) {
        return conjunctive ? x.getTrueLit() : x.getTrueLit()->negateLit(x);
    }
    else if (clause.size() == 1) {
        if (equivalence && x.isAtomFromPreviousStep(clause.front())) {
            clause.front()->invert();
            clause.front()->invert();
        }
        return std::move(clause.front());
    }
    else {
        ULit aux = x.makeAux();
        if (conjunctive) {
            if (equivalence) {
                for (auto &lit : clause) { LRC().addHead(lit).addBody(aux).toLparse(x, true); }
            }
            LRC().addHead(aux).addBody(std::move(clause)).toLparse(x);
        }
        else {
            for (auto &lit : clause) { LRC().addHead(aux).addBody(lit).toLparse(x); }
            if (equivalence) {
                LRC().addHead(std::move(clause)).addBody(aux).toLparse(x, true);
            }
        }
        return aux;
    }
}

ULit getEqualClause(LparseTranslator &x, ULitVec const &clause, bool conjunctive, bool equivalence) {
    return getEqualClause(x, get_clone(clause), conjunctive, equivalence);
}

ULit getEqualFormula(LparseTranslator &x, std::vector<ULitVec> const &clauses, bool conjunctive, bool equivalence) {
    ULitVec disjunction;
    for (auto &conjunction : clauses) {
        disjunction.emplace_back(getEqualClause(x, conjunction, !conjunctive, equivalence));
    }
    return getEqualClause(x, disjunction, conjunctive, equivalence);
}

Value getNeutral(AggregateFunction fun) {
    switch (fun) {
        case AggregateFunction::COUNT:
        case AggregateFunction::SUMP:
        case AggregateFunction::SUM: { return Value::createNum(0); }
        case AggregateFunction::MIN: { return Value::createSup(); }
        case AggregateFunction::MAX: { return Value::createInf(); }
    }
    assert(false);
    return {};
}

ULit getEqualAggregate(LparseTranslator &x, AggregateFunction fun, NAF naf, DisjunctiveBounds const &bounds, Interval const &range, BdAggrElemSet const &bdElems, bool incomplete) {
    AggregateAnalyzer res(naf, bounds, fun, range, bdElems);
    //res.print(std::cerr);
    ULit aux;
    if (res.truth == AggregateAnalyzer::TRUE) {
        aux = x.getTrueLit();
    }
    else if (res.truth == AggregateAnalyzer::FALSE) {
        aux = x.getTrueLit()->negateLit(x);
    }
    else {
        bool recursive = res.monotonicity == AggregateAnalyzer::NONMONOTONE && incomplete;
        ULitValVec elems = res.translateElems(x, fun, bdElems, incomplete);
        if (elems.size() == 1) {
            bool hasElem  = bounds.contains({{elems.front().second, true}, {elems.front().second, true}});
            bool hasEmpty = bounds.contains({{getNeutral(fun), true}, {getNeutral(fun), true}});
            if (hasElem && hasEmpty) { aux = x.getTrueLit(); }
            else if (hasElem)        { aux = std::move(elems.front().first); }
            else if (hasEmpty)       { aux = elems.front().first->negateLit(x); }
            else                     { aux = x.getTrueLit()->negateLit(x); }
        }
        else {
            switch (fun) {
                case AggregateFunction::COUNT:
                case AggregateFunction::SUMP:
                case AggregateFunction::SUM: {
                    SumTranslator trans;
                    for (auto &elem : elems) {
                        trans.addLiteral(x, elem.first, elem.second.num(), recursive);
                    }
                    aux = trans.translate(x, res.bounds, !recursive, res.weightType == AggregateAnalyzer::NEGATIVE);
                    break;
                }
                case AggregateFunction::MIN:
                case AggregateFunction::MAX: {
                    MinMaxTranslator trans;
                    aux = trans.translate(x, res, fun == AggregateFunction::MIN, std::move(elems), incomplete);
                    break;
                }
            }
        }
    }
    switch (naf) {
        case NAF::NOTNOT:
            aux = aux->negateLit(x);
        case NAF::NOT:
            aux = aux->negateLit(x);
        default:
            ULit ret = aux->toLparse(x);
            return ret ? std::move(ret) : std::move(aux);
    }
}

// }}}1

} } // namespace Output Gringo
