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

#include "gringo/output/backends.hh"
#include "potassco/theory_data.h"
#include <cstring>

namespace Gringo { namespace Output {

// {{{1 definition of IntermediateFormatBackend

IntermediateFormatBackend::IntermediateFormatBackend(Potassco::TheoryData const &data, std::ostream &out)
: data_(data)
, out_(out)
, cond_(nullptr) { }
void IntermediateFormatBackend::init(bool incremental) {
    out_.initProgram(incremental);
}
void IntermediateFormatBackend::beginStep() {
    out_.beginStep();
}

void IntermediateFormatBackend::printHead(bool choice, AtomVec const &atoms) {
    type_  = choice ? Potassco::Head_t::Choice : Potassco::Head_t::Disjunctive;
    atoms_ = atoms;
}

void IntermediateFormatBackend::printNormalBody(LitVec const &body) {
    wlits_.clear();
    for (auto &lit : body) { wlits_.push_back({lit, 1}); }
    Potassco::Weight_t size = static_cast<Potassco::Weight_t>(wlits_.size());
    out_.rule(Potassco::HeadView{type_, Potassco::toSpan(atoms_)}, Potassco::BodyView{Potassco::Body_t::Normal, size, Potassco::toSpan(wlits_)});
}

void IntermediateFormatBackend::printWeightBody(Potassco::Weight_t lower, LitWeightVec const &body) {
    out_.rule(Potassco::HeadView{type_, Potassco::toSpan(atoms_)}, Potassco::BodyView{Potassco::Body_t::Sum, lower, Potassco::toSpan(body)});
}
void IntermediateFormatBackend::printMinimize(int priority, LitWeightVec const &body) {
    out_.minimize(priority, Potassco::toSpan(body));
}

void IntermediateFormatBackend::printProject(AtomVec const &lits) {
    out_.project(Potassco::toSpan(lits));
}

void IntermediateFormatBackend::printOutput(char const *value, LitVec const &body) {
    out_.output(Potassco::toSpan(value, std::strlen(value)), Potassco::toSpan(body));
}

void IntermediateFormatBackend::printEdge(unsigned u, unsigned v, LitVec const &body) {
    out_.acycEdge(static_cast<int>(u), static_cast<int>(v), Potassco::toSpan(body));
}

void IntermediateFormatBackend::printHeuristic(Potassco::Heuristic_t modifier, Potassco::Atom_t atom, int value, unsigned priority, LitVec const &body) {
    out_.heuristic(atom, modifier, value, priority, Potassco::toSpan(body));
}

void IntermediateFormatBackend::printExternal(Potassco::Atom_t atom, Potassco::Value_t value) {
    out_.external(atom, value);
}

void IntermediateFormatBackend::printAssume(LitVec const &lits) {
    out_.assume(Potassco::toSpan(lits));
}
bool IntermediateFormatBackend::addSeen(std::vector<bool>& vec, Potassco::Id_t id) const {
    if (vec.size() <= id) { vec.resize(id + 1, false); }
    return !vec[id] && (vec[id] = true) == true;
}
void IntermediateFormatBackend::visit(const Potassco::TheoryData& data, Potassco::Id_t termId, const Potassco::TheoryTerm& t) {
    if (addSeen(tSeen_, termId)) { // only visit once
        // visit any subterms then print
        data.accept(t, *this);
        Potassco::print(out_, termId, t);
    }
}
void IntermediateFormatBackend::visit(const Potassco::TheoryData& data, Potassco::Id_t elemId, const Potassco::TheoryElement& e) {
    if (addSeen(eSeen_, elemId)) { // only visit once
        // visit terms then print element
        data.accept(e, *this);
        out_.theoryElement(elemId, e.terms(), Potassco::toSpan(cond_(e.condition())));
    }
}
void IntermediateFormatBackend::visit(const Potassco::TheoryData& data, const Potassco::TheoryAtom& a) {
    // visit elements then print atom
    data.accept(a, *this);
    Potassco::print(out_, a);
}
void IntermediateFormatBackend::endStep() {
    if (cond_) {
        cond_  = nullptr;
        tSeen_ = eSeen_ = std::vector<bool>();
    }
    out_.endStep();
}

void IntermediateFormatBackend::printTheoryAtom(Potassco::TheoryAtom const & a, GetCond getCond) {
    cond_ = std::move(getCond);
    IntermediateFormatBackend::visit(data_, a);
}
IntermediateFormatBackend::~IntermediateFormatBackend() noexcept = default;
// {{{1 definition of SmodelsFormatBackend

SmodelsFormatBackend::SmodelsFormatBackend(std::ostream &out)
: writer_(out, true)
, out_(writer_, true)
{ }

void SmodelsFormatBackend::init(bool incremental) {
    out_.initProgram(incremental);
}

void SmodelsFormatBackend::beginStep() {
    out_.beginStep();
}

void SmodelsFormatBackend::printHead(bool choice, AtomVec const &atoms) {
    type_ = choice ? Potassco::Head_t::Choice : Potassco::Head_t::Disjunctive;
    atoms_ = atoms;
}

void SmodelsFormatBackend::printNormalBody(LitVec const &body) {
    wlits_.clear();
    for (auto &lit : body) { wlits_.push_back({lit, 1}); }
    Potassco::Weight_t size = static_cast<Potassco::Weight_t>(wlits_.size());
    out_.rule(Potassco::HeadView{type_, Potassco::toSpan(atoms_)}, Potassco::BodyView{Potassco::Body_t::Normal, size, Potassco::toSpan(wlits_)});
}

void SmodelsFormatBackend::printWeightBody(Potassco::Weight_t lower, LitWeightVec const &body) {
    out_.rule(Potassco::HeadView{type_, Potassco::toSpan(atoms_)}, Potassco::BodyView{Potassco::Body_t::Sum, lower, Potassco::toSpan(body)});
}

void SmodelsFormatBackend::printMinimize(int priority, LitWeightVec const &body) {
    out_.minimize(priority, toSpan(body));
}

void SmodelsFormatBackend::printProject(AtomVec const &atoms) {
    out_.project(Potassco::toSpan(atoms));
}

void SmodelsFormatBackend::printOutput(char const *value, LitVec const &body) {
    out_.output(Potassco::StringSpan{value, strlen(value)}, Potassco::toSpan(body));
}

void SmodelsFormatBackend::printEdge(unsigned u, unsigned v, LitVec const &body) {
    out_.acycEdge(u, v, Potassco::toSpan(body));
}

void SmodelsFormatBackend::printHeuristic(Potassco::Heuristic_t modifier, Potassco::Atom_t atom, int value, unsigned priority, LitVec const &body) {
    out_.heuristic(atom, modifier, value, priority, Potassco::toSpan(body));
}

void SmodelsFormatBackend::printExternal(Potassco::Atom_t atom, Potassco::Value_t value) {
    out_.external(atom, value);
}

void SmodelsFormatBackend::printAssume(LitVec const &lits) {
    out_.assume(Potassco::toSpan(lits));
}

void SmodelsFormatBackend::printTheoryAtom(Potassco::TheoryAtom const &, GetCond) {
    throw std::runtime_error("smodels format does not support theory atoms");
}

void SmodelsFormatBackend::endStep() {
    out_.endStep();
}

SmodelsFormatBackend::~SmodelsFormatBackend() noexcept = default;

// }}}1

} } // namespace Output Gringo
