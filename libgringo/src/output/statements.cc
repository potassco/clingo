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

#include <gringo/output/statements.hh>
#include <gringo/output/theory.hh>
#include <gringo/output/aggregates.hh>
#include <gringo/logger.hh>
#include <gringo/control.hh>

namespace Gringo { namespace Output {

// {{{1 definition of helpers to print interval sets

namespace Debug {

std::ostream &operator<<(std::ostream &out, IntervalSet<Symbol>::LBound const &x) {
    out << (x.inclusive ? "[" : "(") << x.bound;
    return out;
}

std::ostream &operator<<(std::ostream &out, IntervalSet<Symbol>::RBound const &x) {
    out << x.bound << (x.inclusive ? "]" : ")");
    return out;
}

std::ostream &operator<<(std::ostream &out, IntervalSet<Symbol>::Interval const &x) {
    out << x.left << "," << x.right;
    return out;
}

std::ostream &operator<<(std::ostream &out, IntervalSet<Symbol> const &x) {
    out << "{";
    print_comma(out, x.vec, ",", [](std::ostream &out, IntervalSet<Symbol>::Interval const &x) { out << x; });
    out << "}";
    return out;
}

} // namespace Debug

namespace {

void printPlainBody(PrintPlain out, LitVec const &body) {
    int sep = 0;
    for (auto &x : body) {
        switch (sep) {
        case 1: { out << ","; break; }
        case 2: { out << ";"; break; }
        }
        call(out.domain, x, &Literal::printPlain, out);
        sep = call(out.domain, x, &Literal::needsSemicolon) + 1;
    }
}

void printPlainHead(PrintPlain out, LitVec const &body, bool choice) {
    bool sep = false;
    if (choice) { out << "{"; }
    for (auto &x : body) {
        if (sep) { out << ";"; }
        call(out.domain, x, &Literal::printPlain, out);
        sep = true;
    }
    if (choice) { out << "}"; }
}

bool updateBound(DomainData &data, LitVec const &head, LitVec const &body, Translator &trans) {
    Symbol value;
    for (auto &y : head) {
        if (!call(data, y, &Literal::isBound, value, false)) { return false; }
    }
    for (auto &y : body) {
        if (!call(data, y, &Literal::isBound, value, true)) { return false; }
    }
    if (value.type() == SymbolType::Special) { return false; }
    std::vector<CSPBound> bounds;
    for (auto &y : body) {
        call(data, y, &Literal::updateBound, bounds, true);
    }
    trans.addBounds(value, bounds);
    return true;
}

} // namespace


// }}}1

// {{{1 definition of Rule

Rule::Rule(bool choice)
: choice_(choice) { }

Rule &Rule::reset(bool choice) {
    choice_ = choice;
    head_.clear();
    body_.clear();
    return *this;
}

Rule &Rule::addHead(LiteralId lit) {
    head_.emplace_back(lit);
    return *this;
}

Rule &Rule::addBody(LiteralId lit) {
    body_.emplace_back(lit);
    return *this;
}

Rule &Rule::negatePrevious(DomainData &data) {
    for (auto &lit : head_) {
        if (call(data, lit, &Literal::isAtomFromPreviousStep)) {
            lit = lit.negate();
            lit = lit.negate();
        }
    }
    return *this;
}

void Rule::print(PrintPlain out, char const *prefix) const {
    out << prefix;
    printPlainHead(out, head_, choice_);
    if (!body_.empty() || head_.empty()) { out << ":-"; }
    printPlainBody(out, body_);
    out << ".\n";
}

void Rule::translate(DomainData &data, Translator &x) {
    head_.erase(std::remove_if(head_.begin(), head_.end(), [&](LiteralId &lit) {
        if (!call(data, lit, &Literal::isHeadAtom)) {
            if (!choice_) { body_.emplace_back(lit.negate()); }
            return true;
        }
        return false;
    }), head_.end());
    if (!updateBound(data, head_, body_, x)) {
        Gringo::Output::translate(data, x, head_);
        Gringo::Output::translate(data, x, body_);
        x.output(data, *this);
    }
}
void Rule::output(DomainData &data, Backend &out) const {
    BackendAtomVec &hd = data.tempAtoms();
    for (auto &x : head_) {
        Potassco::Lit_t lit = call(data, x, &Literal::uid);
        assert(lit > 0);
        hd.emplace_back(static_cast<Potassco::Atom_t>(lit));
    }
    BackendLitVec &bd = data.tempLits();
    for (auto &x : body_) { bd.emplace_back(call(data, x, &Literal::uid)); }
    outputRule(out, choice_, hd, bd);
}

void Rule::replaceDelayed(DomainData &data, LitVec &delayed) {
    Gringo::Output::replaceDelayed(data, head_, delayed);
    Gringo::Output::replaceDelayed(data, body_, delayed);
}

Rule::~Rule() { }

// {{{1 definition of External

External::External(LiteralId head, Potassco::Value_t type)
: head_(head)
, type_(type) { }

void External::print(PrintPlain out, char const *prefix) const {
    out << prefix << "#external ";
    call(out.domain, head_, &Literal::printPlain, out);
    switch (type_) {
        case Potassco::Value_t::False:   { out << ".\n"; break; }
        case Potassco::Value_t::True:    { out << "=true.\n"; break; }
        case Potassco::Value_t::Free:    { out << "=free.\n"; break; }
        case Potassco::Value_t::Release: { out << "=release.\n"; break; }
    }
}

void External::translate(DomainData &data, Translator &x) {
    x.output(data, *this);
}

void External::output(DomainData &data, Backend &out) const {
    Atom_t head = call(data, head_, &Literal::uid);
    out.external(head, type_);
}

void External::replaceDelayed(DomainData &data, LitVec &) {
    (void)data;
    assert(!call(data, head_, &Literal::isIncomplete));
}

External::~External() { }

// {{{1 definition of ShowStatement

ShowStatement::ShowStatement(Symbol term, bool csp, LitVec const &body)
: term_(term)
, body_(body)
, csp_(csp) { }

void ShowStatement::print(PrintPlain out, char const *prefix) const {
    out << prefix;
    out << "#show " << (csp_ ? "$" : "") << term_;
    if (!body_.empty()) { out << ":"; }
    printPlainBody(out, body_);
    out << ".\n";
}

void ShowStatement::replaceDelayed(DomainData &data, LitVec &delayed) {
    Gringo::Output::replaceDelayed(data, body_, delayed);
}

void ShowStatement::translate(DomainData &data, Translator &x) {
    Gringo::Output::translate(data, x, body_);
    x.showTerm(data, term_, csp_, std::move(body_));
}

void ShowStatement::output(DomainData &, Backend &) const {
    // Show statements are taken care of in the translator.
}

// {{{1 definition of ProjectStatement

ProjectStatement::ProjectStatement(LiteralId atom)
: atom_(atom) {
    assert(atom.sign() == NAF::POS);
    assert(atom.type() == AtomType::Predicate);
}

void ProjectStatement::print(PrintPlain out, char const *prefix) const {
    out << prefix << "#project ";
    call(out.domain, atom_, &Literal::printPlain, out);
    out << ".\n";
}

void ProjectStatement::translate(DomainData &data, Translator &x) {
    x.output(data, *this);
}

void ProjectStatement::output(DomainData &data, Backend &out) const {
    BackendAtomVec atoms;
    atoms.emplace_back(call(data, atom_, &Literal::uid));
    out.project(Potassco::toSpan(atoms));
}

void ProjectStatement::replaceDelayed(DomainData &, LitVec &) {
}

// {{{1 definition of HeuristicStatement

HeuristicStatement::HeuristicStatement(LiteralId atom, int value, int priority, Potassco::Heuristic_t mod, LitVec const &body)
: atom_(atom)
, value_(value)
, priority_(priority)
, mod_(mod)
, body_(body) {
    assert(atom.sign() == NAF::POS);
    assert(atom.type() == AtomType::Predicate);
}

void HeuristicStatement::print(PrintPlain out, char const *prefix) const {
    out << prefix << "#heuristic ";
    call(out.domain, atom_, &Literal::printPlain, out);
    if (!body_.empty()) { out << ":"; }
    printPlainBody(out, body_);
    out << ".[" << value_ << "@" << priority_ << "," << toString(mod_) << "]\n";
}

void HeuristicStatement::translate(DomainData &data, Translator &x) {
    Gringo::Output::translate(data, x, body_);
    x.output(data, *this);
}

void HeuristicStatement::output(DomainData &data, Backend &out) const {
    auto uid = call(data, atom_, &Literal::uid);
    BackendLitVec bd;
    for (auto &lit : body_) {
        bd.emplace_back(call(data, lit, &Literal::uid));
    }
    out.heuristic(uid, mod_, value_, priority_, Potassco::toSpan(bd));
}

void HeuristicStatement::replaceDelayed(DomainData &data, LitVec &delayed) {
    Gringo::Output::replaceDelayed(data, body_, delayed);
}

// {{{1 definition of EdgeStatement

EdgeStatement::EdgeStatement(Symbol u, Symbol v, LitVec const &body)
: u_(u)
, v_(v)
, uidU_(0)
, uidV_(0)
, body_(body)
{ }

void EdgeStatement::print(PrintPlain out, char const *prefix) const {
    out << prefix;
    out << "#edge(" << u_ << "," << v_ << ")";
    if (!body_.empty()) { out << ":"; }
    printPlainBody(out, body_);
    out << ".\n";
}

void EdgeStatement::translate(DomainData &data, Translator &x) {
    Gringo::Output::translate(data, x, body_);
    uidU_ = x.nodeUid(u_);
    uidV_ = x.nodeUid(v_);
    x.output(data, *this);
}

void EdgeStatement::output(DomainData &data, Backend &out) const {
    BackendLitVec bd;
    for (auto &x : body_) { bd.emplace_back(call(data, x, &Literal::uid)); }
    out.acycEdge(uidU_, uidV_, Potassco::toSpan(bd));
}

void EdgeStatement::replaceDelayed(DomainData &data, LitVec &delayed) {
    Gringo::Output::replaceDelayed(data, body_, delayed);
}

// {{{1 definition of TheoryDirective

TheoryDirective::TheoryDirective(LiteralId theoryLit)
: theoryLit_(theoryLit) {
    assert(theoryLit_.type() == AtomType::Theory);
}

void TheoryDirective::print(PrintPlain out, char const *prefix) const {
    out << prefix;
    call(out.domain, theoryLit_, &Literal::printPlain, out);
    out << ".\n";
}

void TheoryDirective::translate(DomainData &data, Translator &x) {
    x.output(data, *this);
    assert(!data.getAtom<TheoryDomain>(theoryLit_).recursive() && data.getAtom<TheoryDomain>(theoryLit_).type() == TheoryAtomType::Directive);
    call(data, theoryLit_, &Literal::translate, x);
}

void TheoryDirective::output(DomainData &, Backend &) const {
    // Note: taken care of in translate
}

void TheoryDirective::replaceDelayed(DomainData &, LitVec &) {
}

// {{{1 definition of Minimize

void WeakConstraint::translate(DomainData &data, Translator &x) {
    for (auto &z : lits_) { z = call(data, z, &Literal::translate, x); }
    x.addMinimize(data.tuple(tuple_), getEqualClause(data, x, data.clause(std::move(lits_)), true, false));
}

void WeakConstraint::print(PrintPlain out, char const *prefix) const {
    out << prefix;
    out << ":~";
    printPlainBody(out, lits_);
    out << ".[";
    auto it(tuple_.begin());
    out << *it++ << "@";
    out << *it++;
    for (auto ie(tuple_.end()); it != ie; ++it) { out << "," << *it; }
    out << "]\n";
}

void WeakConstraint::output(DomainData &, Backend &) const {
    throw std::logic_error("WeakConstraint::output: must not be called");
}

void WeakConstraint::replaceDelayed(DomainData &data, LitVec &delayed) {
    Gringo::Output::replaceDelayed(data, lits_, delayed);
}

// }}}


// {{{1 definition of Bound

bool Bound::init(DomainData &data, Translator &x, Logger &log) {
    if (modified) {
        modified = false;
        if (range_.empty()) { Rule().translate(data, x); }
        else {
            if (range_.front() == std::numeric_limits<int>::min() || range_.back()+1 == std::numeric_limits<int>::max()) {
                if      (range_.front()  != std::numeric_limits<int>::min()) { range_.remove(range_.front()+1, std::numeric_limits<int>::max()); }
                else if (range_.back()+1 != std::numeric_limits<int>::max()) { range_.remove(std::numeric_limits<int>::min(), range_.back()); }
                else                                                         { range_.clear(), range_.add(0, 1); }
                GRINGO_REPORT(log, clingo_warning_variable_unbounded)
                    << "warning: unbounded constraint variable:\n"
                    << "  domain of '" << var << "' is set to [" << range_.front() << "," << range_.back() << "]\n"
                    ;
            }
            if (atoms.empty()) {
                auto assign = [&](Potassco::Atom_t a, Potassco::Atom_t b) {
                    if (b) {
                        Rule rule(true);
                        if (a) { rule.addBody({NAF::POS, AtomType::Aux, a, 0}); }
                        rule.addHead({NAF::POS, AtomType::Aux, b, 0}).translate(data, x);
                    }
                };
                for (auto y : range_) {
                    if (y == range_.front()) { atoms.emplace_back(y, 0); }
                    else                     { atoms.emplace_back(y, data.newAtom()); }
                }
                for (auto jt = atoms.begin() + 1; jt != atoms.end(); ++jt) { assign(jt->second, (jt-1)->second); }
                assign(0, atoms.back().second);
            }
            else { // incremental update of bounds
                AtomVec next;
                int l = range_.front(), r = range_.back();
                for (auto jt = atoms.begin() + 1; jt != atoms.end(); ++jt) {
                    int w = (jt - 1)->first;
                    if (w < l)                           { Rule().addBody({NAF::POS, AtomType::Aux, jt->second, 0}).translate(data, x); }
                    else if (w >= r)                     {
                        Rule().addBody({NAF::NOT, AtomType::Aux, jt->second, 0}).translate(data, x);
                        if (w == r) { next.emplace_back(*(jt - 1)); }
                    }
                    else if (!range_.contains(w, w + 1)) { Rule().addBody({NAF::NOT, AtomType::Aux, (jt-1)->second, 0}).addBody({NAF::POS, AtomType::Aux, jt->second, 0}).translate(data, x); }
                    else                                 { next.emplace_back(*(jt - 1)); }
                }
                if (atoms.back().first <= r) { next.emplace_back(atoms.back()); }
                next.front().second = 0;
                atoms = std::move(next);
            }
        }
    }
    return !range_.empty();
}

// {{{1 definition of LinearConstraint

void LinearConstraint::Generate::generate(StateVec::iterator it, int current, int remainder) {
    if (current + remainder > getBound()) {
        if (it == states.end()) {
            assert(remainder == 0);
            aux.emplace_back(data.newAtom());
            for (auto &x : states) {
                if (x.atom != x.bound.atoms.end() && x.atom->second) {
                    Rule()
                        .addHead({NAF::POS, AtomType::Aux, aux.back(), 0})
                        .addBody({x.coef < 0 ? NAF::NOT : NAF::POS, AtomType::Aux, x.atom->second, 0})
                        .translate(data, trans);
                }
            }
        }
        else {
            remainder -= it->upper() - it->lower();
            int total = current - it->lower();
            for (it->reset(); it->valid(); it->next(total, current)) {
                generate(it+1, current, remainder);
                if (current > getBound()) { break; }
            }
        }
    }
}

bool LinearConstraint::Generate::init() {
    int current   = 0;
    int remainder = 0;
    for (auto &y : cons.coefs) {
        states.emplace_back(trans.findBound(y.second), y);
        current   += states.back().lower();
        remainder += states.back().upper() - states.back().lower();
    }
    if (current <= getBound()) {
        generate(states.begin(), current, remainder);
        Rule rule;
        for (auto &x : aux) { rule.addBody({NAF::POS, AtomType::Aux, x, 0}); }
        rule.addHead({NAF::POS, AtomType::Aux, cons.atom, 0}).translate(data, trans);
    }
    return current <= cons.bound;
}

bool LinearConstraint::translate(DomainData &data, Translator &x) {
    Generate gen(*this, data, x);
    return gen.init();
}

// }}}1

// {{{1 definition of Translator

Translator::Translator(UAbstractOutput &&out)
: out_(std::move(out))
{ }

Translator::BoundMap::Iterator Translator::addBound(Symbol x) {
    auto it = boundMap_.find(x);
    return it != boundMap_.end() ? it : boundMap_.push(x).first;
}

Bound &Translator::findBound(Symbol x) {
    auto it = boundMap_.find(x);
    assert(it != boundMap_.end());
    return *it;
}

void Translator::addLowerBound(Symbol x, int bound) {
    auto &y = *addBound(x);
    y.remove(std::numeric_limits<int>::min(), bound);
}

void Translator::addUpperBound(Symbol x, int bound) {
    auto &y = *addBound(x);
    y.remove(bound+1, std::numeric_limits<int>::max());
}

void Translator::addBounds(Symbol value, std::vector<CSPBound> bounds) {
    std::map<Symbol, enum_interval_set<int>> boundUnion;
    for (auto &x : bounds) {
        boundUnion[value].add(x.first, x.second+1);
    }
    for (auto &x : boundUnion) {
        auto &z = *addBound(x.first);
        z.intersect(x.second);
    }
}
void Translator::addLinearConstraint(Potassco::Atom_t head, CoefVarVec &&vars, int bound) {
    for (auto &x : vars) { addBound(x.second); }
    constraints_.emplace_back(head, std::move(vars), bound);
}
void Translator::addDisjointConstraint(DomainData &data, LiteralId lit) {
    auto &atm = data.getAtom<DisjointDomain>(lit.domain(), lit.offset());
    for (auto &x : atm.elems()) {
        for (auto &y : x.second) {
            for (auto z : y.value()) { addBound(z.second); }
        }
    }
    disjointCons_.emplace_back(lit);
}
void Translator::addMinimize(TupleId tuple, LiteralId cond) {
    minimize_.emplace_back(tuple, cond);
}
void Translator::translate(DomainData &data, OutputPredicates const &outPreds, Logger &log) {
    for (auto &x : boundMap_) {
        if (!x.init(data, *this, log)) { return; }
    }
    for (auto &lit : disjointCons_) {
        auto &atm = data.getAtom<DisjointDomain>(lit.domain(), lit.offset());
        atm.translate(data, *this, log);
    }
    for (auto &x : constraints_)  { x.translate(data, *this); }
    disjointCons_.clear();
    constraints_.clear();
    translateMinimize(data);
    outputSymbols(data, outPreds, log);
}

bool Translator::showBound(OutputPredicates const &outPreds, Bound const &bound) {
    return outPreds.empty() || (bound.var.type() == SymbolType::Fun && showSig(outPreds, bound.var.sig(), true));
}

void Translator::outputSymbols(DomainData &data, OutputPredicates const &outPreds, Logger &log) {
    { // show csp varibles
        for (auto it = boundMap_.begin() + incBoundOffset_, ie = boundMap_.end(); it != ie; ++it, ++incBoundOffset_) {
            if (it->var.type() == SymbolType::Fun) { seenSigs_.insert(std::hash<uint64_t>(), std::equal_to<uint64_t>(), it->var.sig().rep()); }
            if (showBound(outPreds, *it)) { showValue(data, *it, LitVec{}); }
        }
    }
    // check for signatures that did not occur in the program
    for (auto &x : outPreds) {
        if (!std::get<1>(x).match("", 0, false) && std::get<2>(x)) {
            auto it(seenSigs_.find(std::hash<uint64_t>(), std::equal_to<uint64_t>(), std::get<1>(x).rep()));
            if (!it) {
                GRINGO_REPORT(log, clingo_warning_atom_undefined)
                    << std::get<0>(x) << ": info: no constraint variables over signature occur in program:\n"
                    << "  $" << std::get<1>(x) << "\n";
                seenSigs_.insert(std::hash<uint64_t>(), std::equal_to<uint64_t>(), std::get<1>(x).rep());
            }
        }
    }
    // show what was requested
    if (!outPreds.empty()) {
        for (auto &x : outPreds) {
            if (!std::get<2>(x)) {
                auto it(data.predDoms().find(std::get<1>(x)));
                if (it != data.predDoms().end()) {
                    showAtom(data, it);
                }
            }
        }
    }
    // show everything non-internal
    else {
        for (auto it = data.predDoms().begin(), ie = data.predDoms().end(); it != ie; ++it) {
            Sig sig = **it;
            auto name(sig.name());
            if (!name.startsWith("#")) { showAtom(data, it); }
        }
    }
    // show terms
    for (auto &todo : termOutput_.todo) {
        if (todo.cond.empty()) { continue; }
        showValue(data, todo.term, updateCond(data, termOutput_.table, todo));
    }
    termOutput_.todo.clear();
    // show csp variables
    for (auto &todo : cspOutput_.todo) {
        auto bound = boundMap_.find(todo.term);
        if (bound == boundMap_.end()) {
            // TODO: clingo_warning_atom_undefined???
            GRINGO_REPORT(log, clingo_warning_atom_undefined)
                << "info: constraint variable does not occur in program:\n"
                << "  $" << todo.term << "\n";
            continue;
        }
        if (todo.cond.empty() || showBound(outPreds, *bound)) { continue; }
        showValue(data, *bound, updateCond(data, cspOutput_.table, todo));
    }
    cspOutput_.todo.clear();
}

LitVec Translator::updateCond(DomainData &data, OutputTable::Table &table, OutputTable::Todo::ValueType &todo) {
    LiteralId excludeOldCond;
    auto entry = table.push(todo.term, LiteralId{});
    if (!entry.second) {
        LiteralId oldCond = entry.first->cond;
        LiteralId newCond = getEqualFormula(data, *this, todo.cond, false, false);
        LiteralId includeOldCond = getEqualClause(data, *this, data.clause(LitVec{oldCond, newCond}), false, false);
        excludeOldCond = getEqualClause(data, *this, data.clause(LitVec{oldCond.negate(), newCond}), true, false);
        entry.first->cond = includeOldCond;
    }
    else {
        excludeOldCond = getEqualFormula(data, *this, todo.cond, false, false);
        entry.first->cond = excludeOldCond;
    }
    return {excludeOldCond};
}

bool Translator::showSig(OutputPredicates const &outPreds, Sig sig, bool csp) {
    if (outPreds.empty()) { return true; }
    auto le = [](OutputPredicates::value_type const &x, OutputPredicates::value_type const &y) -> bool {
        if (std::get<1>(x) != std::get<1>(y)) { return std::get<1>(x) < std::get<1>(y); }
        return std::get<2>(x) < std::get<2>(y);
    };
    static Location loc("",1,1,"",1,1);
    return std::binary_search(outPreds.begin(), outPreds.end(), OutputPredicates::value_type(loc, sig, csp), le);
}

void Translator::showCsp(Bound const &bound, IsTrueLookup isTrue, SymVec &atoms) {
    assert(!bound.atoms.empty());
    int prev = bound.atoms.front().first;
    for (auto it = bound.atoms.begin()+1; it != bound.atoms.end() && !isTrue(it->second); ++it);
    atoms.emplace_back(Symbol::createFun("$", Potassco::toSpan(SymVec{bound.var, Symbol::createNum(prev)})));
}

void Translator::atoms(DomainData &data, unsigned atomset, IsTrueLookup isTrue, SymVec &atoms, OutputPredicates const &outPreds) {
    if (atomset & (clingo_show_type_csp | clingo_show_type_shown)) {
        for (auto &x : boundMap_) {
            if (atomset & clingo_show_type_csp || (atomset & clingo_show_type_shown && showBound(outPreds, x))) { showCsp(x, isTrue, atoms); }
        }
    }
    if (atomset & (clingo_show_type_atoms | clingo_show_type_shown)) {
        for (auto &x : data.predDoms()) {
            Sig sig = *x;
            auto name = sig.name();
            if (((atomset & clingo_show_type_atoms || (atomset & clingo_show_type_shown && showSig(outPreds, sig, false))) && !name.empty() && !name.startsWith("#"))) {
                for (auto &y: *x) {
                    if (y.defined() && y.hasUid() && isTrue(y.uid())) { atoms.emplace_back(y); }
                }
            }
        }
    }
    if (atomset & clingo_show_type_shown) {
        for (auto &entry : cspOutput_.table) {
            auto bound = boundMap_.find(entry.term);
            if (bound != boundMap_.end() && !showBound(outPreds, *bound) && call(data, entry.cond, &Literal::isTrue, isTrue)) {
                showCsp(*bound, isTrue, atoms);
            }
        }
    }
    if (atomset & (clingo_show_type_terms | clingo_show_type_shown)) {
        for (auto &entry : termOutput_.table) {
            if (isTrue(call(data, entry.cond, &Literal::uid))) {
                atoms.emplace_back(entry.term);
            }
        }
    }
}

void Translator::simplify(DomainData &data, Mappings &mappings, AssignmentLookup assignment) {
    minimize_.erase(std::remove_if(minimize_.begin(), minimize_.end(), [&](MinimizeList::value_type &elem) {
        elem.second = call(data, elem.second, &Literal::simplify, mappings, assignment);
        return elem.second != data.getTrueLit().negate();
    }), minimize_.end());
    tuples_.erase([&](TupleLitMap::ValueType &elem) {
        elem.second = call(data, elem.second, &Literal::simplify, mappings, assignment);
        return elem.second != data.getTrueLit().negate();
    });
    termOutput_.table.erase([&](OutputTable::Table::ValueType &elem) {
        elem.cond = call(data, elem.cond, &Literal::simplify, mappings, assignment);
        return elem.cond != data.getTrueLit().negate();
    });
    cspOutput_.table.erase([&](OutputTable::Table::ValueType &elem) {
        elem.cond = call(data, elem.cond, &Literal::simplify, mappings, assignment);
        return elem.cond != data.getTrueLit().negate();
    });
}

void Translator::output(DomainData &data, Statement &stm) {
    out_->output(data, stm);
}

void Translator::showAtom(DomainData &data, PredDomMap::Iterator it) {
    for (auto jt = (*it)->begin() + (*it)->showOffset(), je = (*it)->end(); jt != je; ++jt) {
        if (jt->defined()) {
            LitVec cond;
            if (!jt->fact()) {
                Potassco::Id_t domain = it - data.predDoms().begin();
                Potassco::Id_t offset = jt - (*it)->begin();
                cond.emplace_back(NAF::POS, AtomType::Predicate, offset, domain);
            }
            showValue(data, *jt, cond);
        }
    }
    (*it)->showNext();
}

void Translator::showValue(DomainData &data, Symbol value, LitVec const &cond) {
    Symtab(value, get_clone(cond)).translate(data, *this);
}

void Translator::showValue(DomainData &data, Bound const &bound, LitVec const &cond) {
    if (bound.var.type() != SymbolType::Fun || !bound.var.name().startsWith("#")) {
        auto assign = [&](int i, Potassco::Atom_t a, Potassco::Atom_t b) {
            LitVec body = get_clone(cond);
            if (a) { body.emplace_back(NAF::POS, AtomType::Aux, a, 0); }
            if (b) { body.emplace_back(NAF::NOT, AtomType::Aux, b, 0); }
            Symtab(bound.var, i, std::move(body)).translate(data, *this);
        };
        auto it = bound.begin();
        for (auto jt = bound.atoms.begin() + 1; jt != bound.atoms.end(); ++jt) { assign(*it++, jt->second, (jt-1)->second); }
        assign(*it++, 0, bound.atoms.back().second);
    }
}

void Translator::translateMinimize(DomainData &data) {
    sort_unique(minimize_, [&data](TupleLit const &a, TupleLit const &b) {
        auto aa = data.tuple(a.first), ab = data.tuple(b.first);
        if (aa[1] != ab[1]) { return aa[1] < ab[1]; }
        return a < b;
    });
    for (auto it = minimize_.begin(), iE = minimize_.end(); it != iE;) {
        int priority = data.tuple(it->first)[1].num();
        Minimize lm(priority);
        do {
            LitVec condLits;
            auto tuple = it->first;
            do {
                condLits.emplace_back(it++->second);
            }
            while (it != iE && it->first == tuple);
            int weight(data.tuple(tuple).front().num());
            // Note: extends the minimize constraint incrementally
            auto ret = tuples_.push(tuple, LiteralId{});
            if (!ret.second) {
                lm.add(ret.first->second, -weight);
                condLits.emplace_back(ret.first->second);
            }
            LiteralId lit = getEqualClause(data, *this, data.clause(std::move(condLits)), false, false);
            ret.first->second = lit;
            lm.add(lit, weight);
        }
        while (it != iE && data.tuple(it->first)[1].num() == priority);
        out_->output(data, lm);
    }
    minimize_.clear();
}

void Translator::showTerm(DomainData &data, Symbol term, bool csp, LitVec &&cond) {
    if (csp) {
        cspOutput_.todo.push(term, Formula{}).first->cond.emplace_back(data.clause(std::move(cond)));
    }
    else {
        termOutput_.todo.push(term, Formula{}).first->cond.emplace_back(data.clause(std::move(cond)));
    }
}

unsigned Translator::nodeUid(Symbol v) {
    return nodeUids_.offset(nodeUids_.push(v).first);
}

LiteralId Translator::removeNotNot(DomainData &data, LiteralId lit) {
    if (lit.sign() == NAF::NOTNOT) {
        LiteralId aux = data.newAux();
        Rule().addHead(aux).addBody(lit.negate()).translate(data, *this);
        return aux.negate();
    }
    return lit;
}

constexpr Translator::ClauseKey Translator::ClauseKeyLiterals::open;
constexpr Translator::ClauseKey Translator::ClauseKeyLiterals::deleted;

LiteralId Translator::clause(ClauseId id, bool conjunctive, bool equivalence) {
    auto ret = clauses_.find(
        [](ClauseKey const &a) { return a.hash(); },
        [](ClauseKey const &a, ClauseKey const &b) { return a == b; },
        ClauseKey{ id.first, id.second, conjunctive, equivalence, LiteralId().repr() });
    return ret ? LiteralId{ret->literal} : LiteralId{};
}

void Translator::clause(LiteralId lit, ClauseId id, bool conjunctive, bool equivalence) {
    auto ret = clauses_.insert(
        [](ClauseKey const &a) { return a.hash(); },
        [](ClauseKey const &a, ClauseKey const &b) { return a == b; },
        ClauseKey{ id.first, id.second, conjunctive, equivalence, lit.repr() });
    (void)ret;
    assert(ret.second);
}

Translator::~Translator() { }

// }}}1

// {{{1 definition of Symtab

Symtab::Symtab(Symbol symbol, LitVec &&body)
: symbol_(symbol)
, value_(0)
, csp_(false)
, body_(std::move(body)) { }

Symtab::Symtab(Symbol symbol, int value, LitVec &&body)
: symbol_(symbol)
, value_(value)
, csp_(true)
, body_(std::move(body)) { }

void Symtab::print(PrintPlain out, char const *prefix) const {
    out << prefix << "#show " << symbol_;
    if (csp_) { out << "=" << value_; }
    if (!body_.empty()) { out << ":"; }
    printPlainBody(out, body_);
    out << ".\n";
}

void Symtab::translate(DomainData &data, Translator &x) {
    for (auto &y : body_) { y = call(data, y, &Literal::translate, x); }
    x.output(data, *this);
}

void Symtab::output(DomainData &data, Backend &out) const {
    BackendLitVec &bd = data.tempLits();
    for (auto &x : body_) { bd.emplace_back(call(data, x, &Literal::uid)); }
    std::ostringstream oss;
    oss << symbol_;
    if (csp_) { oss << "=" << value_; }
    out.output(Potassco::toSpan(oss.str()), Potassco::toSpan(bd));
}

void Symtab::replaceDelayed(DomainData &data, LitVec &delayed) {
    Gringo::Output::replaceDelayed(data, body_, delayed);
}

Symtab::~Symtab() { }

// {{{1 definition of Minimize

Minimize::Minimize(int priority)
: priority_(priority) { }

Minimize &Minimize::add(LiteralId lit, Potassco::Weight_t weight) {
    lits_.emplace_back(lit, weight);
    return *this;
}

void Minimize::translate(DomainData &data, Translator &x) {
    for (auto &y : lits_) { y.first = call(data, y.first, &Literal::translate, x); }
    x.output(data, *this);
}

void Minimize::print(PrintPlain out, char const *prefix) const {
    int i = 0;
    out << prefix << "#minimize{";
    auto f = [&i, this](PrintPlain &out, LitWeightVec::value_type const &x) {
        out << x.second << "@" << priority_ << "," << i++ << ":";
        call(out.domain, x.first, &Literal::printPlain, out);
    };
    print_comma(out, lits_, ";", f);
    out << "}.\n";
}

void Minimize::output(DomainData &data, Backend &x) const {
    BackendLitWeightVec &body = data.tempWLits();
    for (auto &y : lits_) {
        body.push_back({call(data, y.first, &Literal::uid), y.second});
    }
    x.minimize(priority_, Potassco::toSpan(body));
}

void Minimize::replaceDelayed(DomainData &data, LitVec &delayed) {
    for (auto &x : lits_) { Gringo::Output::replaceDelayed(data, x.first, delayed); }
}

Minimize::~Minimize() { }

// {{{1 definition of WeightRule

WeightRule::WeightRule(LiteralId head, Potassco::Weight_t lower, LitUintVec &&body)
: head_(head)
, body_(std::move(body))
, lower_(lower) { }

void WeightRule::print(PrintPlain out, char const *prefix) const {
    out << prefix;
    call(out.domain, head_, &Literal::printPlain, out);
    out << ":-" << lower_ << "{";
    if (!body_.empty()) {
        auto it(body_.begin()), ie(body_.end());
        call(out.domain, it->first, &Literal::printPlain, out);
        out << "=" << it->second;
        for (++it; it != ie; ++it) {
            out << ",";
            call(out.domain, it->first, &Literal::printPlain, out);
            out << "=" << it->second;
        }
    }
    out << "}.\n";
}

void WeightRule::translate(DomainData &data, Translator &x) {
    head_ = call(data, head_, &Literal::translate, x);
    if (!call(data, head_, &Literal::isHeadAtom)) {
        LiteralId aux = data.newAux();
        Rule().addHead(head_).addBody(aux).translate(data, x);
        head_ = aux;
    }
    for (auto &y : body_) { y.first = call(data, y.first, &Literal::translate, x); }
    x.output(data, *this);
}

void WeightRule::output(DomainData &data, Backend &out) const {
    BackendLitWeightVec lits;
    for (auto &x : body_) { lits.push_back({call(data, x.first, &Literal::uid), static_cast<Potassco::Weight_t>(x.second)}); }
    BackendAtomVec heads({static_cast<Potassco::Atom_t>(call(data, head_, &Literal::uid))});
    outputRule(out, false, heads, lower_, lits);
}

void WeightRule::replaceDelayed(DomainData &data, LitVec &delayed) {
    Gringo::Output::replaceDelayed(data, head_, delayed);
    for (auto &x : body_) { Gringo::Output::replaceDelayed(data, x.first, delayed); }
}

WeightRule::~WeightRule() { }

// }}}1

} } // namespace Output Gringo
