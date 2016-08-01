// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Author: Roland Kaminski

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

#include "reify/program.hh"
#include "gringo/symbol.hh"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <cassert>

namespace Reify {

// {{{1 Reifier

Reifier::Reifier(std::ostream &out, bool calculateSCCs, bool reifyStep)
: out_(out)
, calculateSCCs_(calculateSCCs)
, reifyStep_(reifyStep) { }

Reifier::~Reifier() noexcept = default;

template<typename... T>
void Reifier::printFact(char const *name, T const &...args) {
    out_ << name << "(";
    printComma(out_, args...);
    out_ << ").\n";
}
template<typename... T>
void Reifier::printStepFact(char const *name, T const &...args) {
    if (reifyStep_) { printFact(name, args..., step_); }
    else            { printFact(name, args...); }
}

template <class M, class T>
size_t Reifier::tuple(M &map, char const *name, T const &args) {
    return tuple(map, name, toVec(args));
}

template <class M, class T>
size_t Reifier::tuple(M &map, char const *name, std::vector<T> &&args) {
    auto ret = map.emplace(std::move(args), map.size());
    if (ret.second) {
        printStepFact(name, ret.first->second);
        for (auto &x : ret.first->first) {
            printStepFact(name, ret.first->second, x);
        }
    }
    return ret.first->second;
}

size_t Reifier::theoryTuple(IdSpan const &args) {
    return tuple(stepData_.theoryTuples, "theory_tuple", args);
}

size_t Reifier::theoryElementTuple(IdSpan const &args) {
    return tuple(stepData_.theoryElementTuples, "theory_element_tuple", args);
}

size_t Reifier::litTuple(LitSpan const &args) {
    return tuple(stepData_.litTuples, "literal_tuple", args);
}

size_t Reifier::weightLitTuple(WeightLitSpan const &args) {
    WLVec lits;
    lits.reserve(args.size);
    for (auto &x : args) { lits.emplace_back(x.lit, x.weight); }
    return tuple(stepData_.weightLitTuples, "weighted_literal_tuple", std::move(lits));
}

size_t Reifier::atomTuple(AtomSpan const &args) {
    return tuple(stepData_.atomTuples, "atom_tuple", args);
}

Reifier::Graph::Node &Reifier::addNode(Atom_t atom) {
    auto & node = stepData_.nodes_[atom];
    if (!node) { node = &stepData_.graph_.insertNode(atom); }
    return *node;
}

void Reifier::initProgram(bool incremental) {
    if (incremental) { printFact("tag", "incremental"); }
}

void Reifier::beginStep() { }

void Reifier::rule(Head_t ht, const AtomSpan& head, const LitSpan& body) {
    char const *h = ht == Potassco::Head_t::Disjunctive ? "disjunction" : "choice";
    std::ostringstream hss, bss;
    hss << h << "(" << atomTuple(head) << ")";
    bss << "normal(" << litTuple(body) << ")";
    printStepFact("rule", hss.str(), bss.str());
    if (calculateSCCs_) { calculateSCCs(head, body); }
}

void Reifier::rule(Head_t ht, const AtomSpan& head, Weight_t bound, const WeightLitSpan& body) {
    char const *h = ht == Potassco::Head_t::Disjunctive ? "disjunction" : "choice";
    std::ostringstream hss, bss;
    hss << h << "(" << atomTuple(head) << ")";
    bss << "sum(" << weightLitTuple(body) << "," << bound << ")";
    printStepFact("rule", hss.str(), bss.str());
    if (calculateSCCs_) { calculateSCCs(head, body); }
}

template <class L>
void Reifier::calculateSCCs(const AtomSpan& head, const Potassco::Span<L>& body) {
    for (auto &atom : head) {
        Graph::Node &u = addNode(atom);
        for (auto &elem : body) {
            if (Potassco::lit(elem) > 0) {
                Graph::Node &v = addNode(Potassco::lit(elem));
                u.insertEdge(v);
            }
        }
    }
}

void Reifier::minimize(Weight_t prio, const WeightLitSpan& lits) {
    printStepFact("minimize", prio, weightLitTuple(lits));
}

void Reifier::project(const AtomSpan& atoms) {
    for (auto &x : atoms) { printStepFact("project", x); }
}

void Reifier::output(const StringSpan& str, const LitSpan& condition) {
    printStepFact("output", str, litTuple(condition));
}

void Reifier::external(Atom_t a, Value_t v) {
    char const *type = "";
    switch (v) {
        case Value_t::Free:    { type = "free";    break; }
        case Value_t::False:   { type = "false";   break; }
        case Value_t::True:    { type = "true";    break; }
        case Value_t::Release: { type = "release"; break; }
    }
    printStepFact("external", a, type);
}

void Reifier::assume(const LitSpan& lits) {
    for (auto &x : lits) { printStepFact("assume", x); }
}

void Reifier::heuristic(Atom_t a, Heuristic_t t, int bias, unsigned prio, const LitSpan& condition) {
    char const *type = "";
    switch (t) {
        case Heuristic_t::Level:  { type = "level";  break; }
        case Heuristic_t::Sign:   { type = "sign";   break; }
        case Heuristic_t::Factor: { type = "factor"; break; }
        case Heuristic_t::Init:   { type = "init";   break; }
        case Heuristic_t::True:   { type = "true";   break; }
        case Heuristic_t::False:  { type = "false";  break; }
    }
    printStepFact("heuristic", a, type, bias, prio, litTuple(condition));
}

void Reifier::acycEdge(int s, int t, const LitSpan& condition) {
    printStepFact("edge", s, t, litTuple(condition));
}

void Reifier::theoryTerm(Id_t termId, int number) {
    printStepFact("theory_number", termId, number);
}

void Reifier::theoryTerm(Id_t termId, const StringSpan& name) {
    auto s = Gringo::quote(name);
    s.insert(s.begin(), '"');
    s.push_back('"');
    printStepFact("theory_string", termId, s);
}

void Reifier::theoryTerm(Id_t termId, int cId, IdSpan const &args) {
    if (cId >= 0) {
        printStepFact("theory_function", termId, cId, theoryTuple(args));
    }
    else {
        char const *type = "";
        switch (cId) {
            case -1: { type = "tuple"; break; }
            case -2: { type = "set";   break; }
            case -3: { type = "list";  break; }
        }
        printStepFact("theory_sequence", termId, type, theoryTuple(args));
    }
}

void Reifier::theoryElement(Id_t elementId, IdSpan const &terms, const LitSpan& cond) {
    printStepFact("theory_element", elementId, theoryTuple(terms), litTuple(cond));
}

void Reifier::theoryAtom(Id_t atomOrZero, Id_t termId, IdSpan const &elements) {
    printStepFact("theory_atom", atomOrZero, termId, theoryElementTuple(elements));
}

void Reifier::theoryAtom(Id_t atomOrZero, Id_t termId, IdSpan const &elements, Id_t op, Id_t rhs) {
    printStepFact("theory_atom", atomOrZero, termId, theoryElementTuple(elements), op, rhs);
}

void Reifier::endStep() {
    size_t i = 0;
    for (auto &scc : stepData_.graph_.tarjan()) {
        if (scc.size() > 1) {
            for (auto &node : scc) {
                printStepFact("scc", i, node->data);
            }
        }
        ++i;
    }
    if (reifyStep_) {
        stepData_ = StepData();
        ++step_;
    }
}

void Reifier::parse(std::istream &in) {
    Potassco::readAspif(in, *this);
}

} // namespace Reify
