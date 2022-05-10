// {{{ MIT License

// Copyright 2017 Roland Kaminski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// }}}

#include "gringo/bug.hh"
#include "gringo/logger.hh"
#include "gringo/input/aggregate.hh"

namespace Gringo { namespace Input {

// {{{ definition of AssignLevel

AssignLevel::AssignLevel() = default;
AssignLevel::~AssignLevel() noexcept = default;

void AssignLevel::add(VarTermBoundVec &vars) {
    for (auto &occ : vars) { occurr[occ.first->ref].emplace_back(occ.first); }
}
AssignLevel &AssignLevel::subLevel() {
    childs.emplace_front();
    return childs.front();
}
void AssignLevel::assignLevels() {
    BoundSet bound;
    assignLevels(0, bound);
}
void AssignLevel::assignLevels(unsigned level, BoundSet const &parent) {
    BoundSet bound(parent);
    for (auto &occs : occurr) {
        unsigned l = bound.emplace(occs.first, level).first->second;
        for (auto &occ : occs.second) { occ->level = l; }
    }
    for (auto &child : childs) { child.assignLevels(level + 1, bound); }
}

// }}}
// {{{ definition of CheckLevel

bool CheckLevel::Ent::operator<(Ent const &) const { return false; }

CheckLevel::CheckLevel(Location const &loc, Printable const &p) : loc(loc), p(p) { }

CheckLevel::CheckLevel(CheckLevel &&other) noexcept = default;

CheckLevel::~CheckLevel() noexcept = default;

CheckLevel::SC::VarNode &CheckLevel::var(VarTerm &var) {
    auto &node = vars[var.name];
    if (node == nullptr) {
        node = &dep.insertVar(&var);
    }
    return *node;
}

void CheckLevel::check(Logger &log) {
    dep.order();
    auto vars(dep.open());
    if (!vars.empty()) {
        auto cmp = [](SC::VarNode const *x, SC::VarNode const *y) -> bool{
            if (x->data->name != y->data->name) { return x->data->name < y->data->name; }
            return x->data->loc() < y->data->loc();
        };
        std::sort(vars.begin(), vars.end(), cmp);
        std::ostringstream msg;
        msg << loc << ": error: unsafe variables in:\n  " << p << "\n";
        for (auto &x : vars) { msg << x->data->loc() << ": note: '" << x->data->name << "' is unsafe\n"; }
        GRINGO_REPORT(log, Warnings::RuntimeError) << msg.str();
    }
}

void addVars(ChkLvlVec &levels, VarTermBoundVec &vars) {
    for (auto &x: vars) {
        auto &lvl(levels[x.first->level]);
        bool bind = x.second && levels.size() == x.first->level + 1;
        if (bind) { lvl.dep.insertEdge(*lvl.current, lvl.var(*x.first)); }
        else      { lvl.dep.insertEdge(lvl.var(*x.first), *lvl.current); }
    }
}

// }}}
// {{{ declaration of ToGroundArg

ToGroundArg::ToGroundArg(unsigned &auxNames, DomainData &domains)
    : auxNames(auxNames)
    , domains(domains) { }
String ToGroundArg::newId(bool increment) {
    auxNames+= increment;
    return ("#d" + std::to_string(auxNames-increment)).c_str();
}
UTermVec ToGroundArg::getGlobal(VarTermBoundVec const &vars) {
    std::unordered_set<String> seen;
    UTermVec global;
    for (auto &occ : vars) {
        if (occ.first->level == 0 && seen.emplace(occ.first->name).second) {
            global.emplace_back(occ.first->clone());
        }
    }
    return global;
}
UTermVec ToGroundArg::getLocal(VarTermBoundVec const &vars) {
    std::unordered_set<String> seen;
    UTermVec local;
    for (auto &occ : vars) {
        if (occ.first->level != 0 && seen.emplace(occ.first->name).second) {
            local.emplace_back(occ.first->clone());
        }
    }
    return local;
}
UTerm ToGroundArg::newId(UTermVec &&global, Location const &loc, bool increment) {
    if (!global.empty()) { return make_locatable<FunctionTerm>(loc, newId(increment), std::move(global)); }
    else                 { return make_locatable<ValTerm>(loc, Symbol::createId(newId(increment))); }
}
ToGroundArg::~ToGroundArg() { }

// }}}

void HeadAggregate::printWithCondition(std::ostream &out, UBodyAggrVec const &condition) const {
    out << *this;
    if (!condition.empty()) {
        out << ":-";
        auto f = [](std::ostream &out, UBodyAggr const &x) { out << *x; };
        print_comma(out, condition, ";", f);
    }
    out << ".";
}

} } // namespace Input Gringo
