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

#ifndef REIFY_PROGRAM_HH
#define REIFY_PROGRAM_HH

#include <reify/util.hh>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <gringo/graph.hh>
#include <potassco/aspif.h>

namespace Reify {

using Potassco::Id_t;
using Potassco::Lit_t;
using Potassco::Atom_t;
using Potassco::Weight_t;
using Potassco::WeightLit_t;
using Potassco::Heuristic_t;
using Potassco::Value_t;
using Potassco::LitSpan;
using Potassco::HeadView;
using Potassco::BodyView;
using Potassco::IdSpan;
using Potassco::AtomSpan;
using Potassco::WeightLitSpan;
using Potassco::StringSpan;

class Reifier : public Potassco::AbstractProgram {
public:
    Reifier(std::ostream &out, bool calculateSCCs, bool reifyStep);
    virtual ~Reifier() noexcept;

    void parse(std::istream &in);

    void initProgram(bool incremental) override;
    void beginStep() override;
    void rule(const HeadView& head, const BodyView& body) override;
    void minimize(Weight_t prio, const WeightLitSpan& lits) override;
    void project(const AtomSpan& atoms) override;
    void output(const StringSpan& str, const LitSpan& condition) override;
    void external(Atom_t a, Value_t v) override;
    void assume(const LitSpan& lits) override;
    void heuristic(Atom_t a, Heuristic_t t, int bias, unsigned prio, const LitSpan& condition) override;
    void acycEdge(int s, int t, const LitSpan& condition) override;

    void theoryTerm(Id_t termId, int number) override;
    void theoryTerm(Id_t termId, const StringSpan& name) override;
    void theoryTerm(Id_t termId, int cId, IdSpan const &args) override;
    void theoryElement(Id_t elementId, IdSpan const &terms, const LitSpan& cond) override;
    void theoryAtom(Id_t atomOrZero, Id_t termId, IdSpan const &elements) override;
    void theoryAtom(Id_t atomOrZero, Id_t termId, IdSpan const &elements, Id_t op, Id_t rhs) override;

    void endStep() override;

private:
    using Graph = Gringo::Graph<Atom_t>;
    template<typename... T>
    void printFact(char const *name, T const &...args);
    template<typename... T>
    void printStepFact(char const *name, T const &...args);
    template <class M, class T>
    size_t tuple(M &map, char const *name, T const &args);
    template <class M, class T>
    size_t tuple(M &map, char const *name, std::vector<T> &&args);
    size_t theoryTuple(IdSpan const &args);
    size_t litTuple(LitSpan const &args);
    size_t litTuple(WeightLitSpan const &args);
    size_t atomTuple(AtomSpan const &args);
    size_t theoryElementTuple(IdSpan const &args);
    size_t weightLitTuple(WeightLitSpan const &args);
    Graph::Node &addNode(Atom_t atom);

private:
    using WLVec = std::vector<std::pair<Lit_t, Weight_t>>;
    struct StepData {
        std::unordered_map<std::vector<Id_t>, size_t, Hash<std::vector<Id_t>>> theoryTuples;
        std::unordered_map<std::vector<Id_t>, size_t, Hash<std::vector<Id_t>>> theoryElementTuples;
        std::unordered_map<std::vector<Lit_t>, size_t, Hash<std::vector<Lit_t>>> litTuples;
        std::unordered_map<std::vector<Atom_t>, size_t, Hash<std::vector<Atom_t>>> atomTuples;
        std::unordered_map<WLVec, size_t, Hash<WLVec>> weightLitTuples;
        Graph graph_;
        std::unordered_map<Atom_t, Graph::Node*> nodes_;
    } stepData_;
    std::ostream &out_;
    size_t step_ = 0;
    bool calculateSCCs_;
    bool reifyStep_;
};

} // namespace Reify

#endif // REIFY_PROGRAM_HH

