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

#ifndef REIFY_PROGRAM_HH
#define REIFY_PROGRAM_HH

#include <reify/util.hh>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include "gringo/graph.hh"

namespace Reify {

using Lit = int32_t;
using Atom = uint32_t;
using Weight = uint32_t;
using Size = uint32_t;
using LitWeight = std::pair<Lit, Weight>;
using AtomVec = std::vector<Atom>;
using LitVec = std::vector<Lit>;
using LitWeightVec = std::vector<LitWeight>;
using Graph = Gringo::Graph<Atom>;

enum class RuleType { Weight, Normal, Disjunctive, Choice };

class Reifier {
public:
    Reifier(std::ostream &out, bool calculateSCCs);
    ~Reifier();
    unsigned printLits(LitWeightVec const &wlits);
    unsigned printWLits(LitWeightVec const &wlits);
    unsigned printAtoms(AtomVec const &atoms);
    Graph::Node &addNode(Atom atom);
    void printSCCs();
    template<typename... T>
    void printFact(char const *name, T const &...args) {
        out_ << name << "(";
        printComma(out_, args...);
        out_ << ").\n";
    }
public:
    bool const calculateSCCs = true;
private:
    Graph graph_;
    std::unordered_map<Atom, Graph::Node*> nodes_;
    std::unordered_map<AtomVec, Size> atoms_;
    std::unordered_map<LitVec, Size> lits_;
    std::unordered_map<LitWeightVec, Size> wlits_;
    std::ostream &out_;
};

class Rule {
public:
    Rule(Rule &&rule);
    Rule(AtomVec const &head, LitWeightVec const &body, Weight bound);
    Rule(AtomVec const &head, LitVec const &body, bool choice = false);
    void printLparse(std::ostream &out);
    void printReified(Reifier &reifier);
private:
    RuleType type_;
    Weight bound_;
    AtomVec head_;
    LitWeightVec body_;
};
using RuleVec = std::vector<Rule>;

class Minimize {
    LitWeightVec body;
};

class Program {
    using MinimizeVec = std::vector<LitWeightVec>;
    using SymbolTable = std::vector<std::pair<Atom,std::string>>;
public:
    void setModels(uint32_t models) { models_ = models; }
    void addRule(Rule &&rule);
    void addMinimize(LitWeightVec const &minimize);
    void showAtom(Atom atom, std::string &&name);
    void addCompute(Lit lit);
    void printLparse(std::ostream &out);
    void printReified(Reifier &reifier);
private:
private:
    int models_ = 1;
    RuleVec rules_;
    LitVec compute_;    
    MinimizeVec minimize_;
    SymbolTable symbols_;
};

} // namespace Reify

#endif // REIFY_PROGRAM_HH

