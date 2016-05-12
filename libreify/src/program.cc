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

#include "reify/program.hh"
#include <iostream>
#include <algorithm>
#include <cassert>

// {{{1 Reifier

Reify::Reifier::Reifier(std::ostream &out, bool calculateSCCs)
: calculateSCCs(calculateSCCs)
, out_(out) { }

Reify::Reifier::~Reifier() = default;

unsigned Reify::Reifier::printLits(LitWeightVec const &wlits) {
    LitVec key;
    for (auto &elem : wlits) { key.emplace_back(elem.first); }
    std::sort(key.begin(), key.end());
    key.erase(std::unique(key.begin(), key.end()), key.end());
    auto res = lits_.emplace(std::move(key), 0);
    if (res.second) {
        res.first->second = lits_.size();
        for (auto &lit : res.first->first) { printFact("lits", res.first->second, lit); }
    }
    return res.first->second;
}

unsigned Reify::Reifier::printWLits(LitWeightVec const &wlits) {
    LitWeightVec key = wlits;
    std::sort(key.begin(), key.end());
    // Note: relies on unspecified behavior
    auto cmp = [](LitWeight &a, LitWeight &b) -> bool {
        if (a.first == b.first) {
            a.second += b.second;
            return true;
        }
        return false;
    };
    key.erase(std::unique(key.begin(), key.end(), cmp), key.end());
    auto res = wlits_.emplace(std::move(key), 0);
    if (res.second) {
        res.first->second = wlits_.size();
        for (auto &elem : res.first->first) { printFact("wlits", res.first->second, elem.first, elem.second); }
    }
    return res.first->second;

}

unsigned Reify::Reifier::printAtoms(AtomVec const &atoms) {
    AtomVec key = atoms;
    std::sort(key.begin(), key.end());
    key.erase(std::unique(key.begin(), key.end()), key.end());
    auto res = atoms_.emplace(std::move(key), 0);
    if (res.second) {
        res.first->second = atoms_.size();
        for (auto &atom : res.first->first) { printFact("atoms", res.first->second, atom); }
    }
    return res.first->second;
}

void Reify::Reifier::printSCCs() {
    assert(calculateSCCs);
    unsigned i = 1;
    for (auto &scc : graph_.tarjan()) {
        if (scc.size() > 1) {
            for (auto &node : scc) {
                printFact("scc", i, node->data);
            }
        }
        ++i;
    }
}

// {{{1 Rule

Reify::Rule::Rule(Rule &&) = default;

Reify::Rule::Rule(AtomVec const &head, LitWeightVec const &body, uint32_t bound)
: type_(RuleType::Weight)
, bound_(bound)
, head_(head)
, body_(body) { }

Reify::Rule::Rule(AtomVec const &head, LitVec const &body, bool choice)
: type_(choice ? RuleType::Choice : (head.size() == 1 ? RuleType::Normal : RuleType::Disjunctive))
, bound_(0)
, head_(head) {
    body_.reserve(body.size());
    for (auto &atom : body) {
        body_.emplace_back(atom, 1);
        bound_ += 1;
    }
}

void Reify::Rule::printLparse(std::ostream &out) {
    bool card = true;
    switch (type_) {
        case RuleType::Choice:      { out << "3 " << head_.size(); break; }
        case RuleType::Normal:      { out << "1"; break; }
        case RuleType::Disjunctive: { out << "8 " << head_.size(); break; }
        case RuleType::Weight:      {
            for (auto &x : body_) {
                if (x.second != 1) {
                    card = false;
                    break;
                }
            }
            out << (card ? "2" : "5"); break;
        }
    }
    for (auto &atom : head_) { out << " " << atom; }
    if (type_ == RuleType::Weight && !card) { out << " " << bound_; }
    out << " " << body_.size() << " " << std::count_if(body_.begin(), body_.end(), [](LitWeight const &elem) { return elem.first < 0; });
    if (type_ == RuleType::Weight && card) { out << " " << bound_; }
    for (auto &elem : body_) {
        if (elem.first < 0) { out << " " << -elem.first; }
    }
    for (auto &elem : body_) {
        if (elem.first > 0) { out << " " << elem.first; }
    }
    if (type_ == RuleType::Weight && !card) {
        for (auto &elem : body_) {
            if (elem.first < 0) { out << " " << elem.second; }
        }
        for (auto &elem : body_) {
            if (elem.first > 0) { out << " " << elem.second; }
        }
    }
    out << "\n";
}

Reify::Graph::Node &Reify::Reifier::addNode(Atom atom) {
    assert(calculateSCCs);
    auto & node = nodes_[atom];
    if (!node) { node = &graph_.insertNode(atom); }
    return *node;
}

void Reify::Rule::printReified(Reifier &reifier) {
    switch (type_) {
        case RuleType::Choice: {
            unsigned head = reifier.printAtoms(head_);
            unsigned body = reifier.printLits(body_);
            reifier.printFact("choice", head, body);
            break;
        }
        case RuleType::Normal: {
            unsigned body = reifier.printLits(body_);
            reifier.printFact("normal", head_.front(), body);
            break;
        }
        case RuleType::Disjunctive: {
            unsigned head = reifier.printAtoms(head_);
            unsigned body = reifier.printLits(body_);
            reifier.printFact("disjunction", head, body);
            break;
        }
        case RuleType::Weight: {
            unsigned body = reifier.printWLits(body_);
            reifier.printFact("weight", head_.front(), bound_, body);
            break;
        }
    }
    if (reifier.calculateSCCs) {
        for (auto &atom : head_) {
            Reify::Graph::Node & head = reifier.addNode(atom);
            for (auto &elem : body_) {
                if (elem.first > 0) {
                    Reify::Graph::Node & body = reifier.addNode(elem.first);
                    head.insertEdge(body);
                }
            }
        }
    }
}

 // {{{1 Program

void Reify::Program::addRule(Rule &&rule) {
    rules_.emplace_back(std::move(rule));
}

void Reify::Program::addMinimize(LitWeightVec const &body) {
    minimize_.emplace_back(body);
}

void Reify::Program::addCompute(Lit lit) {
    compute_.emplace_back(lit);
}

void Reify::Program::showAtom(Atom atom, std::string &&name) {
    symbols_.emplace_back(atom, std::move(name));
}

void Reify::Program::printLparse(std::ostream &out) {
    for (auto &rule : rules_) {
        rule.printLparse(out);
    }
    for (auto &minimize : minimize_) {
        out << "6 0 " << minimize.size() << " " << std::count_if(minimize.begin(), minimize.end(), [](LitWeight const &elem) { return elem.first < 0; });
        for (auto &elem : minimize) {
            if (elem.first < 0) { out << " " << -elem.first; }
        }
        for (auto &elem : minimize) {
            if (elem.first > 0) { out << " " << elem.first; }
        }
        for (auto &elem : minimize) {
            if (elem.first < 0) { out << " " << elem.second; }
        }
        for (auto &elem : minimize) {
            if (elem.first > 0) { out << " " << elem.second; }
        }
        out << "\n";
    }
    out << "0\n";
    for (auto &elem : symbols_) {
        out << elem.first << " " << elem.second << "\n";
    }
    out << "0\n";
    out << "B+\n";
    for (auto &lit : compute_) {
        if (lit > 0) { out << lit << "\n"; }
    }
    out << "0\n";
    out << "B-\n";
    for (auto &lit : compute_) {
        if (lit < 0) { out << -lit << "\n"; }
    }
    out << "0\n";
    out << models_ << "\n";
}

void Reify::Program::printReified(Reifier &reifier) {
    for (auto &rule : rules_) {
        rule.printReified(reifier);
    }
    if (reifier.calculateSCCs) {
        reifier.printSCCs();
    }
    for (auto &lit : compute_) {
        reifier.printFact("compute", lit);
    }
    for (auto &entry : symbols_) {
        reifier.printFact("show", entry.first, entry.second);
    }
    Weight priority = 0;
    for (auto &minimize : minimize_) {
        Size body = reifier.printWLits(minimize);
        reifier.printFact("minimize", priority, body);
        ++priority;
    }
    reifier.printFact("models", models_);
}

