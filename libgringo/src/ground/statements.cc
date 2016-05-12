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

#include "gringo/ground/statements.hh"
#include "gringo/safetycheck.hh"
#include "gringo/output/literals.hh"
#include "gringo/output/output.hh"
#include "gringo/ground/binders.hh"
#include "gringo/logger.hh"
#include <limits>

namespace Gringo { namespace Ground {

// FIXME: too much c&p similarities in the implementations below

// {{{1 Helpers

namespace {

// {{{2 definition of _initBounds

void _initBounds(BoundVec const &bounds, IntervalSet<Value> &set) {
    set.add({{Value::createInf(),true}, {Value::createSup(),true}});
    for (auto &x : bounds) {
        bool undefined = false;
        Value v(x.bound->eval(undefined));
        assert(!undefined);
        switch (x.rel) {
            case Relation::GEQ: { set.remove({{Value::createInf(),true},{v,false}          }); break; }
            case Relation::GT:  { set.remove({{Value::createInf(),true},{v,true}           }); break; }
            case Relation::LEQ: { set.remove({{v,false},                {Value::createSup(),true}}); break; }
            case Relation::LT:  { set.remove({{v,true},                 {Value::createSup(),true}}); break; }
            case Relation::NEQ: { set.remove({{v,true},                 {v,true}           }); break; }
            case Relation::EQ: {
                set.remove({{v,false},                {Value::createSup(),true}});
                set.remove({{Value::createInf(),true},{v,false}          });
                break;
            }
        }
    }
}

// {{{2 definition of _analyze (Note: could be moved into AbstractStatement)

void _analyze(Statement::Dep::Node &node, Statement::Dep &dep, ULitVec const &lits, bool forceNegative = false) {
    for (auto &x : lits) {
        auto occ(x->occurrence());
        if (occ) { dep.depends(node, *occ, forceNegative); }
    }
}

// {{{2 definition of _linearize (Note: could be moved into AbstractStatement)

struct Ent {
    Ent(BinderType type, Literal &lit) : type(type), lit(lit) { }
    std::vector<unsigned> depends;
    std::vector<unsigned> vars;
    BinderType type;
    Literal &lit;
};
using SC  = SafetyChecker<unsigned, Ent>;

InstVec _linearize(Scripts &scripts, bool positive, SolutionCallback &cb, Term::VarSet &&important, ULitVec const &lits, ULitVec const &aux) {
    InstVec insts;
    std::vector<unsigned> rec;
    std::vector<std::vector<std::pair<BinderType,Literal*>>> todo{1};
    unsigned i{0};
    for (auto &x : lits) {
        todo.back().emplace_back(BinderType::ALL, x.get());
        if (x->isRecursive() && x->occurrence() && !x->occurrence()->isNegative()) { rec.emplace_back(i); }
        ++i;
    }
    for (auto &x : aux) {
        todo.back().emplace_back(BinderType::ALL, x.get());
        if (x->isRecursive() && x->occurrence() && !x->occurrence()->isNegative()) { rec.emplace_back(i); }
        ++i;
    }
    todo.reserve(std::max(std::vector<unsigned>::size_type(1), rec.size()));
    insts.reserve(todo.capacity()); // Note: preserve references
    for (auto i : rec) {
        todo.back()[i].first = BinderType::NEW;
        if (i != rec.back()) {
            todo.emplace_back(todo.back());
            todo.back()[i].first = BinderType::OLD;
        }
    }
    if (!positive) {
        for (auto &lit : lits) { lit->collectImportant(important); }
    }
    for (auto &x : todo) {
        insts.emplace_back(cb);
        SC s;
        std::unordered_map<FWString, SC::VarNode*> varMap;
        std::vector<std::pair<FWString, std::vector<unsigned>>> boundBy;
        for (auto &lit : x) {
            auto &entNode(s.insertEnt(lit.first, *lit.second));
            VarTermBoundVec vars;
            lit.second->collect(vars);
            for (auto &occ : vars) {
                auto &varNode(varMap[occ.first->name]);
                if (!varNode)   {
                    varNode = &s.insertVar(boundBy.size());
                    boundBy.emplace_back(occ.first->name, std::vector<unsigned>{});
                }
                if (occ.second) { s.insertEdge(entNode, *varNode); }
                else            { s.insertEdge(*varNode, entNode); }
                entNode.data.vars.emplace_back(varNode->data);
            }
        }
        Term::VarSet bound;
        Instantiator::DependVec depend;
        unsigned uid = 0;
        auto pred = [&bound](Ent const &x, Ent const &y) -> bool {
            double sx(x.lit.score(bound));
            double sy(y.lit.score(bound));
            //std::cerr << "  " << x.lit << "@" << sx << " < " << y.lit << "@" << sy << " with " << bound.size() << std::endl;
            if (sx < 0 || sy < 0) { return sx < sy; }
            if ((x.type == BinderType::NEW || y.type == BinderType::NEW) && x.type != y.type) { return x.type < y.type; }
            return sx < sy;
        };

        SC::EntVec open;
        s.init(open);
        while (!open.empty()) {
            for (auto it = open.begin(), end = open.end() - 1; it != end; ++it) {
                if (pred((*it)->data, open.back()->data)) { std::swap(open.back(), *it); }
            }
            auto y = open.back();
            for (auto &var : y->data.vars) {
                auto &bb(boundBy[var]);
                if (bound.find(bb.first) == bound.end()) {
                    bb.second.emplace_back(uid);
                    if ((depend.empty() || depend.back() != uid) && important.find(bb.first) != important.end()) { depend.emplace_back(uid); }
                }
                else { y->data.depends.insert(y->data.depends.end(), bb.second.begin(), bb.second.end()); }
            }
            auto index(y->data.lit.index(scripts, y->data.type, bound));
            if (auto update = index->getUpdater()) {
                if (BodyOcc *occ = y->data.lit.occurrence()) {
                    for (HeadOccurrence &x : occ->definedBy()) { x.defines(*update, y->data.type == BinderType::NEW ? &insts.back() : nullptr); }
                }
            }
            std::sort(y->data.depends.begin(), y->data.depends.end());
            y->data.depends.erase(std::unique(y->data.depends.begin(), y->data.depends.end()), y->data.depends.end());
            insts.back().add(std::move(index), std::move(y->data.depends));
            uid++;
            open.pop_back();
            s.propagate(y, open);
        }
        insts.back().finalize(std::move(depend));
    }
    return insts;
}

// {{{2 definition of completeRepr_

UTerm completeRepr_(UTerm const &repr) {
    UTermVec ret;
    ret.emplace_back(get_clone(repr));
    return make_locatable<FunctionTerm>(repr->loc(), FWString{"#complete"}, std::move(ret));
}

// {{{2 definition of BindOnce

struct BindOnce : Binder, IndexUpdater {
    IndexUpdater *getUpdater() { return this; }
    bool update() { return true; }
    void match() { once = true; }
    bool next() {
        bool ret = once;
        once = false;
        return ret;
    }
    void print(std::ostream &out) const { out << "#once"; }
    bool once;
};

// {{{2 operator <<

template <class T>
std::ostream &operator <<(std::ostream &out, std::unique_ptr<T> const &x) {
    out << *x;
    return out;
}

template <class T>
std::ostream &operator <<(std::ostream &out, std::vector<T> const &x) {
    if (!x.empty()) {
        for (auto it = x.begin(), ie = x.end(); ; ) {
            out << *it;
            if (++it != ie) { out << ","; }
            else { break; }
        }
    }
    return out;
}
std::ostream &operator <<(std::ostream &out, OccurrenceType x) {
    switch (x) {
        case OccurrenceType::POSITIVELY_STRATIFIED: { break; }
        case OccurrenceType::STRATIFIED:            { out << "!"; break; }
        case OccurrenceType::UNSTRATIFIED:          { out << "?"; break; }
    }
    return out;
}

// }}}2

} // namespace

// }}}1
// {{{1 definition of HeadDefinition

HeadDefinition::HeadDefinition(UTerm &&repr, Domain *domain) : repr(std::move(repr)), domain(domain) { }
UGTerm HeadDefinition::getRepr() const { return repr->gterm(); }
void HeadDefinition::defines(IndexUpdater &update, Instantiator *inst) {
    auto ret(offsets.emplace(&update, enqueueVec.size()));
    if (ret.second)     { enqueueVec.emplace_back(&update, RInstVec{}); }
    if (active && inst) { enqueueVec[ret.first->second].second.emplace_back(*inst); }
}
void HeadDefinition::enqueue(Queue &queue) {
    if (domain) { queue.enqueue(*domain); }
    for (auto &x : enqueueVec) {
        if (x.first->update()) {
            for (Instantiator &y : x.second) { y.enqueue(queue); }
        }
    }
}
void HeadDefinition::collectImportant(Term::VarSet &vars) {
    if (repr) {
        VarTermBoundVec x;
        repr->collect(x, false);
        for (auto &occ : x) { vars.emplace(occ.first->name); }
    }
}
HeadDefinition::~HeadDefinition() { }

//{{{1 definition of AbstractStatement

AbstractStatement::AbstractStatement(UTerm &&repr, Domain *domain, ULitVec &&lits, ULitVec &&auxLits)
: def(std::move(repr), domain)
, lits(std::move(lits))
, auxLits(std::move(auxLits)) { }

AbstractStatement::~AbstractStatement() { }

bool AbstractStatement::isNormal() const { return false; }

void AbstractStatement::analyze(Dep::Node &node, Dep &dep) {
  if (def.repr) { dep.provides(node, def, def.getRepr()); }
  _analyze(node, dep, lits);
  _analyze(node, dep, auxLits);
}

void AbstractStatement::startLinearize(bool active) {
    if (def.repr) { def.active = active; }
    if (active) { insts.clear(); }
}

void AbstractStatement::collectImportant(Term::VarSet &vars) {
    if (def.repr) { def.collectImportant(vars); }
}
void AbstractStatement::linearize(Scripts &scripts, bool positive) {
    Term::VarSet important;
    collectImportant(important);
    insts = _linearize(scripts, positive, *this, std::move(important), lits, auxLits);
}

void AbstractStatement::enqueue(Queue &q) {
    if (def.domain) { def.domain->init(); }
    for (auto &x : insts) { x.enqueue(q); }
}

void AbstractStatement::printHead(std::ostream &out) const {
    if (def.repr) { out << *def.repr; }
    else          { out << "#false"; }
}
void AbstractStatement::printBody(std::ostream &out) const {
    out << lits;
    if (!auxLits.empty()) {
        out << ":-" << auxLits;
    }
}
void AbstractStatement::print(std::ostream &out) const {
    printHead(out);
    if (!lits.empty() || !auxLits.empty()) {
        out << ":-";
        printBody(out);
    }
    out << ".";
}

void AbstractStatement::propagate(Queue &queue) {
    def.enqueue(queue);
}

// {{{1 definition of Rule

Rule::Rule(PredicateDomain *domain, UTerm &&repr, ULitVec &&lits, RuleType type)
: AbstractStatement(std::move(repr), domain, std::move(lits), {})
, type(type) { }

Rule::~Rule() { }

void Rule::report(Output::OutputBase &out) {
    switch (type) {
        case RuleType::EXTERNAL: {
            if (def.repr) {
                // just insert the atom into the domain and report it to the output
                bool undefined = false;
                Value val(def.repr->eval(undefined));
                if (!undefined) {
                    auto ret(static_cast<PredicateDomain*>(def.domain)->insert(val, false));
                    out.createExternal(*std::get<0>(ret));
                }
            }
            break;
        }
        case RuleType::NORMAL: {
            Output::RuleRef &rule(out.tempRule);
            rule.body.clear();
            bool fact = true;
            for (auto &x : lits) {
                auto ret = x->toOutput();
                if (ret.first && (out.keepFacts || !ret.second)) {
                    rule.body.emplace_back(*ret.first);
                }
                if (!ret.second) { fact = false; }
            }
            if (def.repr) {
                bool undefined = false;
                Value val = def.repr->eval(undefined);
                if (!undefined) {
                    auto ret(static_cast<PredicateDomain*>(def.domain)->insert(val, fact));
                    if (!std::get<2>(ret)) {
                        rule.head = std::get<0>(ret);
                        out.output(rule);
                    }
                }
            }
            else {
                rule.head = nullptr;
                out.output(rule);
            }
            break;
        }
    }
}

bool Rule::isNormal() const { return def.repr && type != RuleType::EXTERNAL; }

void Rule::printHead(std::ostream &out) const {
    if (type == RuleType::EXTERNAL) { out << "#external "; }
    AbstractStatement::printHead(out);
}

void Rule::print(std::ostream &out) const {
    assert(auxLits.empty());
    printHead(out);
    if (!lits.empty()) {
        out << (type == RuleType::EXTERNAL ? ":" : ":-");
        printBody(out);
    }
    out << ".";
}

// {{{1 WeakConstraint

WeakConstraint::WeakConstraint(UTermVec &&tuple, ULitVec &&lits)
: AbstractStatement(nullptr, nullptr, std::move(lits), {})
, tuple(std::move(tuple)) { }

void WeakConstraint::report(Output::OutputBase &out) {
    out.tempVals.clear();
    bool undefined = false;
    for (auto &x : tuple) { out.tempVals.emplace_back(x->eval(undefined)); }
    if (!undefined && out.tempVals.front().type() == Value::NUM) {
        Output::ULitVec cond;
        for (auto &x : lits) {
            auto lit = x->toOutput();
            if (!lit.second) { cond.emplace_back(lit.first->clone()); }
        }
        Output::Minimize min;
        min.elems.emplace_back(
                std::piecewise_construct,
                std::forward_as_tuple(out.tempVals),
                std::forward_as_tuple(std::move(cond)));
        out.output(min);
    }
    else if (!undefined) {
        GRINGO_REPORT(W_OPERATION_UNDEFINED)
            << tuple.front()->loc() << ": info: tuple ignored:\n"
            << "  " << out.tempVals.front() << "\n";
    }
}

void WeakConstraint::collectImportant(Term::VarSet &vars) {
    for (auto &x : tuple) { x->collect(vars); }
}

void WeakConstraint::printHead(std::ostream &out) const {
    assert(tuple.size() > 1);
    out << "[";
    out << tuple.front() << "@" << tuple[1];
    for (auto it(tuple.begin() + 2), ie(tuple.end()); it != ie; ++it) { out << "," << *it; }
    out << "]";
}
void WeakConstraint::print(std::ostream &out) const {
    out << ":~" << lits << ".";
    printHead(out);
}
WeakConstraint::~WeakConstraint() { }

// {{{1 ExternalRule

ExternalRule::ExternalRule()
    : defines(make_locatable<ValTerm>(Location("#external", 1, 1, "#external", 1, 1), Value::createId("#external")), nullptr) { }
    bool ExternalRule::isNormal() const                  { return false; }
    void ExternalRule::analyze(Dep::Node &node, Dep &dep) {
        dep.provides(node, defines, defines.getRepr());
    }
void ExternalRule::startLinearize(bool active) {
    defines.active = active;
}
void ExternalRule::linearize(Scripts &, bool) { }
void ExternalRule::enqueue(Queue &) {
}
void ExternalRule::print(std::ostream &out) const {
    out << "#external.";
}
ExternalRule::~ExternalRule()                                   { }

// {{{1 Body Aggregates
// {{{2 BodyAggregate
// {{{3 definition of BodyAggregateComplete

BodyAggregateComplete::BodyAggregateComplete(UTerm &&repr, AggregateFunction fun, BoundVec &&bounds)
: def(std::move(repr), &domain)
, accuRepr(completeRepr_(def.repr))
, fun(fun)
, bounds(std::move(bounds))
, inst(*this) {
    switch (fun) {
        case AggregateFunction::COUNT:
        case AggregateFunction::SUMP:
        case AggregateFunction::MAX: {
            for (auto &x : this->bounds) {
                if (x.rel != Relation::GT && x.rel != Relation::GEQ) { positive = false; break; }
            }
            break;
        }
        case AggregateFunction::MIN: {
            for (auto &x : this->bounds) {
                if (x.rel != Relation::LT && x.rel != Relation::LEQ) { positive = false; break; }
            }
            break;
        }
        default: { positive = false; break; }
    }
}

bool BodyAggregateComplete::isNormal() const {
    return true;
}
void BodyAggregateComplete::analyze(Dep::Node &node, Dep &dep) {
    dep.depends(node, *this);
    dep.provides(node, def, def.getRepr());
}
void BodyAggregateComplete::startLinearize(bool active) {
    def.active = active;
    if (active) { inst = Instantiator(*this); }
}
void BodyAggregateComplete::linearize(Scripts &, bool) {
    auto binder  = gringo_make_unique<BindOnce>();
    for (HeadOccurrence &x : defBy) { x.defines(*binder->getUpdater(), &inst); }
    inst.add(std::move(binder), Instantiator::DependVec{});
    inst.finalize(Instantiator::DependVec{});
}
void BodyAggregateComplete::enqueue(Queue &q) {
    domain.init();
    q.enqueue(inst);
}
void BodyAggregateComplete::printHead(std::ostream &out) const {
    out << *def.repr;
}
void BodyAggregateComplete::propagate(Queue &queue) {
    def.enqueue(queue);
}
void BodyAggregateComplete::report(Output::OutputBase &) {
    for (DomainElement &x : todo) {
        if (x.second.bounds.intersects(x.second.range(fun))) {
            x.second.generation(domain.exports.size());
            x.second._defined  = true;
            x.second._positive = positive;
            domain.exports.append(x);
        }
        x.second.enqueued = false;
    }
    todo.clear();
}

void BodyAggregateComplete::print(std::ostream &out) const {
    printHead(out);
    out << ":-";
    print_comma(out, accuDoms, ",", [this](std::ostream &out, BodyAggregateAccumulate const &x) -> void { x.printHead(out); out << occType; });
    out << ".";
}

UGTerm BodyAggregateComplete::getRepr() const {
    return accuRepr->gterm();
}

bool BodyAggregateComplete::isPositive() const {
    return true;
}

bool BodyAggregateComplete::isNegative() const {
    return false;
}

void BodyAggregateComplete::setType(OccurrenceType x) {
    occType = x;
}

OccurrenceType BodyAggregateComplete::getType() const {
    return occType;
}

BodyAggregateComplete::DefinedBy &BodyAggregateComplete::definedBy() {
    return defBy;
}

void BodyAggregateComplete::checkDefined(LocSet &, SigSet const &, UndefVec &) const {

}

BodyAggregateComplete::~BodyAggregateComplete() { }

// {{{3 definition of BodyAggregateAccumulate

BodyAggregateAccumulate::BodyAggregateAccumulate(BodyAggregateComplete &complete, UTermVec &&tuple, ULitVec &&lits, ULitVec &&auxLits)
: AbstractStatement(get_clone(complete.accuRepr), nullptr, std::move(lits), std::move(auxLits))
, complete(complete)
, tuple(std::move(tuple)) {}

BodyAggregateAccumulate::~BodyAggregateAccumulate() { }

void BodyAggregateAccumulate::collectImportant(Term::VarSet &vars) {
    VarTermBoundVec bound;
    def.repr->collect(bound, false);
    for (auto &x : tuple) { x->collect(bound, false); }
    for (auto &x : bound) { vars.emplace(x.first->name); }
}

void BodyAggregateAccumulate::report(Output::OutputBase &out) {
    out.tempVals.clear();
    bool undefined = false;
    for (auto &x : tuple) { out.tempVals.emplace_back(x->eval(undefined)); }
    Value repr(complete.def.repr->eval(undefined));
    if (!undefined) {
        out.tempLits.clear();
        for (auto &x : lits) {
            auto lit = x->toOutput();
            if (!lit.second) { out.tempLits.emplace_back(*lit.first); }
        }
        auto state = complete.domain.domain.find(repr);
        if (state == complete.domain.domain.end()) {
            state = complete.domain.domain.emplace(std::piecewise_construct, std::forward_as_tuple(repr), std::forward_as_tuple()).first;
        }
        if (!state->second.initialized) {
            state->second.initialized = true;
            state->second.init(complete.fun);
            _initBounds(complete.bounds, state->second.bounds);
            assert(!undefined);
        }
        if (!Output::neutral(out.tempVals, complete.fun, tuple.empty() ? def.repr->loc() : tuple.front()->loc())) {
            auto ret(state->second.elems.emplace_back(std::piecewise_construct, std::forward_as_tuple(out.tempVals), std::forward_as_tuple()));
            auto &elem(ret.first->second);
            if (ret.second || elem.size() != 1 || !elem.front().empty()) {
                if (out.tempLits.empty()) {
                    elem.clear();
                    elem.emplace_back();
                    state->second.accumulate(out.tempVals, complete.fun, true, !ret.second);
                }
                else {
                    if (ret.second) { state->second.accumulate(out.tempVals, complete.fun, false, false); }
                    elem.emplace_back();
                    for (Output::Literal &x : out.tempLits) { elem.back().emplace_back(x.clone()); }
                }
            }
        }
        state->second._fact = state->second.bounds.contains(state->second.range(complete.fun));
        if (!state->second._defined && !state->second.enqueued) {
            state->second.enqueued = true;
            complete.todo.emplace_back(*state);
        }
    }
}

void BodyAggregateAccumulate::printHead(std::ostream &out) const {
    out << "#accu(" << *complete.def.repr << ",tuple(" << tuple << "))";
}

// {{{3 definition of BodyAggregateLiteral

BodyAggregateLiteral::BodyAggregateLiteral(BodyAggregateComplete &complete, NAF naf)
: complete(complete) {
    gLit.naf = naf;
}
BodyAggregateLiteral::~BodyAggregateLiteral() { }
UGTerm BodyAggregateLiteral::getRepr() const { return complete.def.repr->gterm(); }
bool BodyAggregateLiteral::isPositive() const { return gLit.naf == NAF::POS && complete.positive; }
bool BodyAggregateLiteral::isNegative() const { return gLit.naf != NAF::POS; }
void BodyAggregateLiteral::setType(OccurrenceType x) { type = x; }
OccurrenceType BodyAggregateLiteral::getType() const { return type; }
BodyOcc::DefinedBy &BodyAggregateLiteral::definedBy() { return defs; }
void BodyAggregateLiteral::checkDefined(LocSet &, SigSet const &, UndefVec &) const { }
void BodyAggregateLiteral::print(std::ostream &out) const {
    out << gLit.naf;
    auto it = std::begin(complete.bounds), ie = std::end(complete.bounds);
    if (it != ie) {
        it->bound->print(out);
        out << inv(it->rel);
        ++it;
    }
    out << complete.fun << "{" << *complete.def.repr << type << "}";
    if (it != ie) {
        out << it->rel;
        it->bound->print(out);
    }
}
bool BodyAggregateLiteral::isRecursive() const { return type == OccurrenceType::UNSTRATIFIED; }
BodyOcc *BodyAggregateLiteral::occurrence() { return this; }
void BodyAggregateLiteral::collect(VarTermBoundVec &vars) const { complete.def.repr->collect(vars, gLit.naf == NAF::POS); }
UIdx BodyAggregateLiteral::index(Scripts &, BinderType type, Term::VarSet &bound) {
    return make_binder(complete.domain, gLit.naf, *complete.def.repr, gLit.repr, type, isRecursive(), bound, 0);
}
Literal::Score BodyAggregateLiteral::score(Term::VarSet const &bound) { return gLit.naf == NAF::POS ? estimate(complete.domain.exports.size(), *complete.def.repr, bound) : 0; }
std::pair<Output::Literal*,bool> BodyAggregateLiteral::toOutput() {
    gLit.incomplete = isRecursive();
    gLit.fun        = complete.fun;
    gLit.bounds.clear();
    bool undefined = false;
    for (auto &x : complete.bounds) { gLit.bounds.emplace_back(x.rel, x.bound->eval(undefined)); }
    assert(!undefined);
    switch (gLit.naf) {
        case NAF::POS:
        case NAF::NOTNOT: { return {&gLit, gLit.repr->second.fact(gLit.incomplete)}; }
        case NAF::NOT:    { return {&gLit, !gLit.incomplete && gLit.repr->second.isFalse()}; }
    }
    assert(false);
    return {nullptr, true};
}

// {{{2 AssignmentAggregate

// {{{3 definition of AssignmentAggregateComplete

AssignmentAggregateComplete::AssignmentAggregateComplete(UTerm &&repr, UTerm &&dataRepr, AggregateFunction fun)
: def(std::move(repr), &domain)
, dataRepr(std::move(dataRepr))
  , fun(fun)
  , inst(*this) { }

  bool AssignmentAggregateComplete::isNormal() const {
      return true;
  }

void AssignmentAggregateComplete::analyze(Dep::Node &node, Dep &dep) {
    dep.depends(node, *this);
    dep.provides(node, def, def.getRepr());
}

void AssignmentAggregateComplete::startLinearize(bool active) {
    def.active = active;
    if (active) { inst = Instantiator(*this); }
}

void AssignmentAggregateComplete::linearize(Scripts &, bool) {
    auto binder  = gringo_make_unique<BindOnce>();
    for (HeadOccurrence &x : defBy) { x.defines(*binder->getUpdater(), &inst); }
    inst.add(std::move(binder), Instantiator::DependVec{});
    inst.finalize(Instantiator::DependVec{});
}

void AssignmentAggregateComplete::enqueue(Queue &q) {
    domain.init();
    q.enqueue(inst);
}

void AssignmentAggregateComplete::printHead(std::ostream &out) const {
    out << *def.repr;
}

void AssignmentAggregateComplete::propagate(Queue &queue) {
    def.enqueue(queue);
}

void AssignmentAggregateComplete::insert_(ValVec const &values, StateDataMap::value_type &x) {
    ValVec valsCache;
    Value as(x.first);
    if (as.type() == Value::FUNC) { valsCache.assign(as.args().begin(), as.args().end()); }
    valsCache.emplace_back();
    for (auto &y : values) {
        valsCache.back() = y;
        auto ret(domain.domain.emplace(std::piecewise_construct, std::forward_as_tuple(Value::createFun(as.name(), valsCache)), std::forward_as_tuple(&x.second, domain.exports.size())));
        if (ret.second) { domain.exports.append(*ret.first); }
    }
    x.second.fact = values.size() == 1;
}
void AssignmentAggregateComplete::report(Output::OutputBase &) {
    for (StateDataMap::value_type &data : todo) {
        switch (fun) {
            case AggregateFunction::MIN: {
                ValVec values;
                values.clear();
                Value min(Value::createSup());
                for (auto &x : data.second.elems) {
                    if (x.second.size() == 1 && x.second.front().empty()) {
                        Value weight(x.first.front());
                        if (weight < min) { min = weight; }
                    }
                }
                values.emplace_back(min);
                for (auto &x : data.second.elems) {
                    Value weight(x.first.front());
                    if (weight < min) { values.emplace_back(weight); }
                }
                std::sort(values.begin(), values.end());
                values.erase(std::unique(values.begin(), values.end()), values.end());
                insert_(values, data);
                break;
            }
            case AggregateFunction::MAX: {
                ValVec values;
                values.clear();
                Value max(Value::createInf());
                for (auto &x : data.second.elems) {
                    if (x.second.size() == 1 && x.second.front().empty()) {
                        Value weight(x.first.front());
                        if (max < weight) { max = weight; }
                    }
                }
                values.emplace_back(max);
                for (auto &x : data.second.elems) {
                    Value weight(x.first.front());
                    if (max < weight) { values.emplace_back(weight); }
                }
                std::sort(values.begin(), values.end());
                values.erase(std::unique(values.begin(), values.end()), values.end());
                insert_(values, data);
                break;
            }
            default: {
                // Note: Count case could be optimized
                std::vector<int> values;
                values.emplace_back(0);
                for (auto &x : data.second.elems) {
                    int weight = fun == AggregateFunction::COUNT ? 1 : x.first.front().num();
                    if (x.second.size() == 1 && x.second.front().empty()) {
                        for (auto &x : values) { x+= weight; }
                    }
                    else {
                        values.reserve(values.size() * 2);
                        for (auto &x : values) { values.emplace_back(x + weight); }
                        std::sort(values.begin(), values.end());
                        values.erase(std::unique(values.begin(), values.end()), values.end());
                    }
                }
                ValVec vals;
                vals.reserve(values.size());
                for (auto &x : values) { vals.emplace_back(Value::createNum(x)); }
                insert_(vals, data);
                break;
            }
        }
        data.second.enqueued = false;
    }
    todo.clear();
}

void AssignmentAggregateComplete::print(std::ostream &out) const {
    printHead(out);
    out << ":-";
    print_comma(out, accuDoms, ";", [this](std::ostream &out, AssignmentAggregateAccumulate const &x) -> void { x.printHead(out); out << occType; });
    out << ".";
}

UGTerm AssignmentAggregateComplete::getRepr() const {
    return dataRepr->gterm();
}

bool AssignmentAggregateComplete::isPositive() const {
    return true;
}

bool AssignmentAggregateComplete::isNegative() const {
    return false;
}

void AssignmentAggregateComplete::setType(OccurrenceType x) {
    occType = x;
}

OccurrenceType AssignmentAggregateComplete::getType() const {
    return occType;
}

AssignmentAggregateComplete::DefinedBy &AssignmentAggregateComplete::definedBy() {
    return defBy;
}

void AssignmentAggregateComplete::checkDefined(LocSet &, SigSet const &, UndefVec &) const {

}

AssignmentAggregateComplete::~AssignmentAggregateComplete() { }

// {{{3 Definition of AssignmentAggregateAccumulate

    AssignmentAggregateAccumulate::AssignmentAggregateAccumulate(AssignmentAggregateComplete &complete, UTermVec &&tuple, ULitVec &&lits, ULitVec &&auxLits)
    : AbstractStatement(get_clone(complete.dataRepr), nullptr, std::move(lits), std::move(auxLits))
      , complete(complete)
      , tuple(std::move(tuple)) { }

      AssignmentAggregateAccumulate::~AssignmentAggregateAccumulate() { }

      void AssignmentAggregateAccumulate::collectImportant(Term::VarSet &vars) {
          VarTermBoundVec bound;
          def.repr->collect(bound, false);
          for (auto &x : tuple) { x->collect(bound, false); }
          for (auto &x : bound) { vars.emplace(x.first->name); }
      }

void AssignmentAggregateAccumulate::report(Output::OutputBase &out) {
    out.tempVals.clear();
    bool undefined = false;
    for (auto &x : tuple) { out.tempVals.emplace_back(x->eval(undefined)); }
    Value dataRepr(complete.dataRepr->eval(undefined));
    if (!undefined) {
        out.tempLits.clear();
        for (auto &x : lits) {
            auto lit = x->toOutput();
            if (!lit.second) { out.tempLits.emplace_back(*lit.first); }
        }
        auto &data = *complete.data.emplace(std::piecewise_construct, std::forward_as_tuple(dataRepr), std::forward_as_tuple()).first;
        if (!Output::neutral(out.tempVals, complete.fun, tuple.empty() ? def.repr->loc() : tuple.front()->loc())) {
            auto ret(data.second.elems.emplace_back(std::piecewise_construct, std::forward_as_tuple(out.tempVals), std::forward_as_tuple()));
            auto &elem(ret.first->second);
            if (ret.second || elem.size() != 1 || !elem.front().empty()) {
                if (out.tempLits.empty()) { elem.clear(); }
                elem.emplace_back();
                for (Output::Literal &x : out.tempLits) { elem.back().emplace_back(x.clone()); }
            }
        }
        if (!data.second.enqueued) {
            data.second.enqueued = true;
            complete.todo.emplace_back(data);
        }
    }
}

void AssignmentAggregateAccumulate::printHead(std::ostream &out) const {
    out << "#accu(" << *complete.def.repr << ",tuple(" << tuple << "))";
}

// {{{3 Definition of AssignmentAggregateLiteral

AssignmentAggregateLiteral::AssignmentAggregateLiteral(AssignmentAggregateComplete &complete)
: complete(complete) { }
AssignmentAggregateLiteral::~AssignmentAggregateLiteral() { }
UGTerm AssignmentAggregateLiteral::getRepr() const { return complete.def.repr->gterm(); }
bool AssignmentAggregateLiteral::isPositive() const { return false; }
bool AssignmentAggregateLiteral::isNegative() const { return false; }
void AssignmentAggregateLiteral::setType(OccurrenceType x) { type = x; }
OccurrenceType AssignmentAggregateLiteral::getType() const { return type; }
BodyOcc::DefinedBy &AssignmentAggregateLiteral::definedBy() { return defs; }
void AssignmentAggregateLiteral::checkDefined(LocSet &, SigSet const &, UndefVec &) const { }
void AssignmentAggregateLiteral::print(std::ostream &out) const { out << static_cast<FunctionTerm&>(*complete.def.repr).args.back() << "=" << complete.fun << "{" << *complete.def.repr << "}" << type; }
bool AssignmentAggregateLiteral::isRecursive() const { return type == OccurrenceType::UNSTRATIFIED; }
BodyOcc *AssignmentAggregateLiteral::occurrence() { return this; }
void AssignmentAggregateLiteral::collect(VarTermBoundVec &vars) const { complete.def.repr->collect(vars, true); }
UIdx AssignmentAggregateLiteral::index(Scripts &, BinderType type, Term::VarSet &bound) {
    return make_binder(complete.domain, NAF::POS, *complete.def.repr, gLit.repr, type, isRecursive(), bound, 0);
}
Literal::Score AssignmentAggregateLiteral::score(Term::VarSet const &bound) { return estimate(complete.domain.exports.size(), *complete.def.repr, bound); }
std::pair<Output::Literal*,bool> AssignmentAggregateLiteral::toOutput() {
    gLit.incomplete = isRecursive();
    gLit.fun        = complete.fun;
    return {&gLit, gLit.repr->second.fact(gLit.incomplete)};
}
///}}}3

// {{{2 Conjunction

// {{{3 definition of ConjunctionLiteral

ConjunctionLiteral::ConjunctionLiteral(ConjunctionComplete &complete)
    : complete(complete) { }

ConjunctionLiteral::~ConjunctionLiteral() = default;

UGTerm ConjunctionLiteral::getRepr() const {
    return complete.def.repr->gterm();
}

bool ConjunctionLiteral::isPositive() const {
    return true;
}

bool ConjunctionLiteral::isNegative() const {
    return false;
}

void ConjunctionLiteral::setType(OccurrenceType x) {
    type = x;
}

OccurrenceType ConjunctionLiteral::getType() const {
    return type;
}

BodyOcc::DefinedBy &ConjunctionLiteral::definedBy() {
    return defs;
}

void ConjunctionLiteral::checkDefined(LocSet &, SigSet const &, UndefVec &) const {
}

void ConjunctionLiteral::print(std::ostream &out) const {
    out << *complete.def.repr << type;
}

bool ConjunctionLiteral::isRecursive() const {
    return type == OccurrenceType::UNSTRATIFIED;
}

BodyOcc *ConjunctionLiteral::occurrence() {
    return this;
}

void ConjunctionLiteral::collect(VarTermBoundVec &vars) const {
    complete.def.repr->collect(vars, true);
}

UIdx ConjunctionLiteral::index(Scripts &, BinderType type, Term::VarSet &bound) {
    return make_binder(complete.dom, NAF::POS, *complete.def.repr, gLit.repr, type, isRecursive(), bound, 0);
}

Literal::Score ConjunctionLiteral::score(Term::VarSet const &bound) {
    return estimate(complete.dom.exports.size(), *complete.def.repr, bound);
}

std::pair<Output::Literal*,bool> ConjunctionLiteral::toOutput() {
    return {&gLit, gLit.repr->second.fact(gLit.repr->second.incomplete)};
}

// {{{3 definition of ConjunctionAccumulateEmpty

ConjunctionAccumulateEmpty::ConjunctionAccumulateEmpty(ConjunctionComplete &complete, ULitVec &&auxLits)
: AbstractStatement(complete.emptyRepr(), &complete.domEmpty, {}, std::move(auxLits))
, complete(complete) {
}

ConjunctionAccumulateEmpty::~ConjunctionAccumulateEmpty() = default;

bool ConjunctionAccumulateEmpty::isNormal() const {
    return true;
}

void ConjunctionAccumulateEmpty::report(Output::OutputBase &) {
    bool undefined = false;
    Value repr(complete.def.repr->eval(undefined));
    auto &state = *complete.dom.domain.emplace(std::piecewise_construct, std::forward_as_tuple(repr), std::forward_as_tuple()).first;
    if (state.second.numBlocked == 0 && !state.second.defined() && !state.second.enqueued()) {
        state.second.enqueue();
        complete.todo.emplace_back(state);
    }
    complete.domEmpty.insert(def.repr->eval(undefined), false);
    assert(!undefined);
}

// {{{3 definition of ConjunctionAccumulateCond

ConjunctionAccumulateCond::ConjunctionAccumulateCond(ConjunctionComplete &complete, ULitVec &&lits)
: AbstractStatement(complete.condRepr(), &complete.domCond, std::move(lits), {})
, complete(complete) {
    auxLits.emplace_back(gringo_make_unique<PredicateLiteral>(complete.domEmpty, NAF::POS, complete.emptyRepr()));
}

ConjunctionAccumulateCond::~ConjunctionAccumulateCond() = default;

void ConjunctionAccumulateCond::linearize(Scripts &scripts, bool positive) {
    AbstractStatement::linearize(scripts, positive);
    for (auto &x : lits) { complete.condRecursive = complete.condRecursive || x->isRecursive(); }
}

bool ConjunctionAccumulateCond::isNormal() const {
    return true;
}

void ConjunctionAccumulateCond::report(Output::OutputBase &) {
    bool undefined = false;
    Value litRepr(complete.def.repr->eval(undefined));
    Value condRepr(def.repr->eval(undefined));
    assert(!undefined);

    auto &state = *complete.dom.domain.emplace(std::piecewise_construct, std::forward_as_tuple(litRepr), std::forward_as_tuple()).first;
    Output::ULitVec cond;
    for (auto &x : lits) {
        auto y = x->toOutput();
        if (!y.second) {
            cond.emplace_back(y.first->clone());
        }
    }
    complete.domCond.insert(condRepr, cond.empty());
    auto ret = state.second.elems.emplace_back(condRepr.args()[2]);
    if (ret.first->bodies.size() != 1 || !ret.first->bodies.front().empty()) {
        if (cond.empty()) {
            ++state.second.numBlocked;
            ret.first->bodies.clear();
            ret.first->bodies.emplace_back();
        }
        else {
            ret.first->bodies.emplace_back(std::move(cond));
            if (state.second.numBlocked == 0 && !state.second.defined() && !state.second.enqueued()) {
                complete.todo.emplace_back(state);
                state.second.enqueue();
            }
        }
    }
}

// {{{3 definition of ConjunctionAccumulateHead

ConjunctionAccumulateHead::ConjunctionAccumulateHead(ConjunctionComplete &complete, ULitVec &&lits)
: AbstractStatement(complete.headRepr(), nullptr, std::move(lits), {})
, complete(complete) {
    auxLits.emplace_back(gringo_make_unique<PredicateLiteral>(complete.domCond, NAF::POS, complete.condRepr()));
}

ConjunctionAccumulateHead::~ConjunctionAccumulateHead() = default;

bool ConjunctionAccumulateHead::isNormal() const {
    return true;
}

void ConjunctionAccumulateHead::report(Output::OutputBase &) {
    bool undefined = false;
    Value litRepr(complete.def.repr->eval(undefined));
    Value condRepr(def.repr->eval(undefined));
    assert(!undefined);

    auto &state = *complete.dom.domain.find(litRepr);
    Output::ULitVec head;
    for (auto &x : lits) {
        auto y = x->toOutput();
        if (!y.second) {
            head.emplace_back(y.first->clone());
        }
    }

    auto &elem = *state.second.elems.find(condRepr.args()[2]);
    if (elem.heads.empty() && elem.bodies.size() == 1 && elem.bodies.back().empty()) { --state.second.numBlocked; }
    if (head.empty()) { elem.heads.clear(); }
    elem.heads.emplace_back(std::move(head));
    if (state.second.numBlocked == 0 && !state.second.defined() && !state.second.enqueued()) {
        complete.todo.emplace_back(state);
        state.second.enqueue();
    }
}

// {{{3 definition of ConjunctionComplete

ConjunctionComplete::ConjunctionComplete(UTerm &&repr, UTermVec &&local)
: def(std::move(repr), &dom)
, inst(*this)
, local(std::move(local)) { }

UTerm ConjunctionComplete::emptyRepr() const {
    UTermVec args;
    args.emplace_back(make_locatable<ValTerm>(def.repr->loc(), Value::createId("empty")));
    args.emplace_back(get_clone(def.repr));
    args.emplace_back(make_locatable<FunctionTerm>(def.repr->loc(), "", UTermVec()));
    return make_locatable<FunctionTerm>(def.repr->loc(), "#accu", std::move(args));
}

UTerm ConjunctionComplete::condRepr() const {
    UTermVec args;
    args.emplace_back(make_locatable<ValTerm>(def.repr->loc(), Value::createId("cond")));
    args.emplace_back(get_clone(def.repr));
    args.emplace_back(make_locatable<FunctionTerm>(def.repr->loc(), "", get_clone(local)));
    return make_locatable<FunctionTerm>(def.repr->loc(), "#accu", std::move(args));
}

UTerm ConjunctionComplete::headRepr() const {
    UTermVec args;
    args.emplace_back(make_locatable<ValTerm>(def.repr->loc(), Value::createId("head")));
    args.emplace_back(get_clone(def.repr));
    args.emplace_back(make_locatable<FunctionTerm>(def.repr->loc(), "", get_clone(local)));
    return make_locatable<FunctionTerm>(def.repr->loc(), "#accu", std::move(args));
}

UTerm ConjunctionComplete::accuRepr() const {
    UTermVec args;
    args.emplace_back(make_locatable<VarTerm>(def.repr->loc(), "#Any1", std::make_shared<Value>(Value::createNum(0))));
    args.emplace_back(get_clone(def.repr));
    args.emplace_back(make_locatable<VarTerm>(def.repr->loc(), "#Any2", std::make_shared<Value>(Value::createNum(0))));
    return make_locatable<FunctionTerm>(def.repr->loc(), "#accu", std::move(args));
}

bool ConjunctionComplete::isNormal() const {
    return true;
}

void ConjunctionComplete::analyze(Dep::Node &node, Dep &dep){
    dep.depends(node, *this);
    dep.provides(node, def, def.getRepr());
}

void ConjunctionComplete::startLinearize(bool active){
    def.active = active;
    if (active) { inst = Instantiator(*this); }
}

void ConjunctionComplete::linearize(Scripts &, bool){
    auto binder  = gringo_make_unique<BindOnce>();
    for (HeadOccurrence &x : defBy) { x.defines(*binder->getUpdater(), &inst); }
    inst.add(std::move(binder), Instantiator::DependVec{});
    inst.finalize(Instantiator::DependVec{});
}

void ConjunctionComplete::enqueue(Queue &q){
    dom.init();
    q.enqueue(inst);
}

void ConjunctionComplete::printHead(std::ostream &out) const{
    out << *def.repr;
}

void ConjunctionComplete::propagate(Queue &queue) {
    def.enqueue(queue);
}

void ConjunctionComplete::report(Output::OutputBase &) {
    // NOTE: this code relies on priority set to 2!
    // NOTE about handling incompleteness:
    //   - if both the condition and the head are stratified
    //     then the conjunction is complete
    //   - if the condition is stratified but the head is recursive,
    //     then the conjunction is complete if all heads are fact
    for (DomainElement &x : todo) {
        if (x.second.numBlocked == 0) {
            if (!condRecursive) {
                x.second._fact = true;
                for (auto &y : x.second.elems) {
                    if (y.bodies.size() != 1 || !y.bodies.front().empty() ||
                        y.heads.size()  != 1 || !y.heads.front().empty()
                    ) {
                        x.second._fact = false;
                        break;
                    }
                }
            } else {
                x.second._fact = false;
            }
            x.second.incomplete = occType == OccurrenceType::UNSTRATIFIED && !x.second._fact;
            x.second.generation(dom.exports.size());
            dom.exports.append(x);
        }
        else { x.second.dequeue(); }
    }
    todo.clear();
}

void ConjunctionComplete::print(std::ostream &out) const{
    printHead(out);
    out << ":-" << *accuRepr();
}

UGTerm ConjunctionComplete::getRepr() const {
    return accuRepr()->gterm();
}

bool ConjunctionComplete::isPositive() const {
    return true;
}

bool ConjunctionComplete::isNegative() const {
    return false;
}

void ConjunctionComplete::setType(OccurrenceType x) {
    occType = x;
}

OccurrenceType ConjunctionComplete::getType() const {
    return occType;
}

ConjunctionComplete::DefinedBy &ConjunctionComplete::definedBy() {
    return defBy;
}

void ConjunctionComplete::checkDefined(LocSet &, SigSet const &, UndefVec &) const { }

ConjunctionComplete::~ConjunctionComplete() { }

// }}}3

// {{{2 Disjoint
// {{{3 definition of DisjointComplete

DisjointComplete::DisjointComplete(UTerm &&repr)
: def(std::move(repr), &domain)
, accuRepr(completeRepr_(def.repr))
, inst(*this) { }

bool DisjointComplete::isNormal() const {
    return false;
}
void DisjointComplete::analyze(Dep::Node &node, Dep &dep) {
    dep.depends(node, *this);
    dep.provides(node, def, def.getRepr());
}
void DisjointComplete::startLinearize(bool active) {
    def.active = active;
    if (active) { inst = Instantiator(*this); }
}
void DisjointComplete::linearize(Scripts &, bool) {
    auto binder  = gringo_make_unique<BindOnce>();
    for (HeadOccurrence &x : defBy) { x.defines(*binder->getUpdater(), &inst); }
    inst.add(std::move(binder), Instantiator::DependVec{});
    inst.finalize(Instantiator::DependVec{});
}
void DisjointComplete::enqueue(Queue &q) {
    domain.init();
    q.enqueue(inst);
}
void DisjointComplete::printHead(std::ostream &out) const {
    out << *def.repr;
}
void DisjointComplete::propagate(Queue &queue) {
    def.enqueue(queue);
}
void DisjointComplete::report(Output::OutputBase &) {
    for (DomainElement &x : todo) {
        x.second.generation(domain.exports.size());
        x.second.enqueued = false;
        domain.exports.append(x);
    }
    todo.clear();
}

void DisjointComplete::print(std::ostream &out) const {
    printHead(out);
    out << ":-";
    print_comma(out, accuDoms, ",", [](std::ostream &out, DisjointAccumulate const &x) { x.printHead(out); });
    out << ".";
}

UGTerm DisjointComplete::getRepr() const {
    return accuRepr->gterm();
}

bool DisjointComplete::isPositive() const {
    return true;
}

bool DisjointComplete::isNegative() const {
    return false;
}

void DisjointComplete::setType(OccurrenceType x) {
    occType = x;
}

OccurrenceType DisjointComplete::getType() const {
    return occType;
}

DisjointComplete::DefinedBy &DisjointComplete::definedBy() {
    return defBy;
}

void DisjointComplete::checkDefined(LocSet &, SigSet const &, UndefVec &) const {
}

DisjointComplete::~DisjointComplete() { }

// {{{3 definition of DisjointAccumulate

DisjointAccumulate::DisjointAccumulate(DisjointComplete &complete, ULitVec &&lits, ULitVec &&auxLits)
    : AbstractStatement(get_clone(complete.accuRepr), nullptr, std::move(lits), std::move(auxLits))
    , complete(complete)
    , value({})
    , neutral(true) { }

DisjointAccumulate::DisjointAccumulate(DisjointComplete &complete, UTermVec &&tuple, CSPAddTerm &&value, ULitVec &&lits, ULitVec &&auxLits)
    : AbstractStatement(get_clone(complete.accuRepr), nullptr, std::move(lits), std::move(auxLits))
    , complete(complete)
    , tuple(std::move(tuple))
    , value(std::move(value))
    , neutral(false) { }

DisjointAccumulate::~DisjointAccumulate() { }

void DisjointAccumulate::collectImportant(Term::VarSet &vars) {
    VarTermBoundVec bound;
    def.repr->collect(bound, false);
    value.collect(bound);
    for (auto &x : tuple) { x->collect(bound, false); }
    for (auto &x : bound) { vars.emplace(x.first->name); }
}

void DisjointAccumulate::report(Output::OutputBase &out) {
    bool undefined = false;
    Value repr(complete.def.repr->eval(undefined));
    assert(!undefined);
    auto &state = *complete.domain.domain.emplace(std::piecewise_construct, std::forward_as_tuple(repr), std::forward_as_tuple()).first;
    if (!neutral) {
        out.tempVals.clear();
        for (auto &x : tuple) { out.tempVals.emplace_back(x->eval(undefined)); }
        if (!undefined && value.checkEval()) {
            CSPGroundLit gVal(Relation::EQ, {}, 0);
            value.toGround(gVal, false);
            auto &elem = *state.second.elems.emplace_back(std::piecewise_construct, std::forward_as_tuple(out.tempVals), std::forward_as_tuple()).first;
            Output::ULitVec outLits;
            for (auto &x : lits) {
                auto lit = x->toOutput();
                if (!lit.second) {
                    outLits.emplace_back(lit.first->clone());
                }
            }
            elem.second.emplace_back(std::move(std::get<1>(gVal)), -std::get<2>(gVal), std::move(outLits));
        }
    }
    if (!state.second.enqueued && !state.second.defined()) {
        complete.todo.emplace_back(state);
        state.second.enqueued = true;
    }
}

void DisjointAccumulate::printHead(std::ostream &out) const {
    out << "#accu(" << *complete.def.repr << ",";
    if (!value.terms.empty()) {
        out << value;
    } else {
        out << "#neutral";
    }
    if (!tuple.empty()) {
        out << ",tuple(" << tuple << ")";
    }
    out << ")";
}

// {{{3 definition of DisjointLiteral

DisjointLiteral::DisjointLiteral(DisjointComplete &complete, NAF naf)
: complete(complete)
, gLit(naf) { }
DisjointLiteral::~DisjointLiteral() { }
UGTerm DisjointLiteral::getRepr() const { return complete.def.repr->gterm(); }
bool DisjointLiteral::isPositive() const { return false; }
bool DisjointLiteral::isNegative() const { return gLit.naf != NAF::POS; }
void DisjointLiteral::setType(OccurrenceType x) { type = x; }
OccurrenceType DisjointLiteral::getType() const { return type; }
BodyOcc::DefinedBy &DisjointLiteral::definedBy() { return defs; }
void DisjointLiteral::checkDefined(LocSet &, SigSet const &, UndefVec &) const { }
void DisjointLiteral::print(std::ostream &out) const { out << gLit.naf << "#disjoint{" << *complete.def.repr << type << "}"; }
bool DisjointLiteral::isRecursive() const { return type == OccurrenceType::UNSTRATIFIED; }
BodyOcc *DisjointLiteral::occurrence() { return this; }
void DisjointLiteral::collect(VarTermBoundVec &vars) const { complete.def.repr->collect(vars, gLit.naf == NAF::POS); }
UIdx DisjointLiteral::index(Scripts &, BinderType type, Term::VarSet &bound) {
    return make_binder(complete.domain, gLit.naf, *complete.def.repr, gLit.repr, type, isRecursive(), bound, 0);
}
Literal::Score DisjointLiteral::score(Term::VarSet const &bound) { return gLit.naf == NAF::POS ? estimate(complete.domain.exports.size(), *complete.def.repr, bound) : 0; }
std::pair<Output::Literal*,bool> DisjointLiteral::toOutput() {
    gLit.incomplete = isRecursive();
    return {&gLit, false};
}

// {{{1 Head Aggregates
// {{{2 HeadAggregate
// {{{3 definition of HeadAggregateRule

HeadAggregateRule::HeadAggregateRule(UTerm &&repr, AggregateFunction fun, BoundVec &&bounds, ULitVec &&lits)
    : AbstractStatement(std::move(repr), &domain, std::move(lits), {})
    , fun(fun)
    , bounds(std::move(bounds)) { }

HeadAggregateRule::~HeadAggregateRule() { }

void HeadAggregateRule::report(Output::OutputBase &out) {
    std::unique_ptr<Output::HeadAggregateRule> rule(gringo_make_unique<Output::HeadAggregateRule>());
    rule->fun = fun;
    for (auto &x : lits) {
        auto ret = x->toOutput();
        if (ret.first && (out.keepFacts || !ret.second)) {
            rule->body.emplace_back(Output::ULit(ret.first->clone()));
        }
    }
    bool undefined = false;
    for (auto &x : bounds) { rule->bounds.emplace_back(x.rel, x.bound->eval(undefined)); }
    auto ret(domain.domain.emplace(def.repr->eval(undefined), HeadAggregateState{fun, domain.exports.size()}));
    assert(!undefined);
    if (ret.second) {
        _initBounds(bounds, ret.first->second.bounds);
        domain.exports.append(*ret.first);
    }
    rule->repr = &ret.first->second;
    // NOTE: the head here might be false
    //       it does not make much sense to output the unsatisfiable aggregate head in this case
    //       the delayed output should take this into consideration...
    out.output(std::move(rule));
}

void HeadAggregateRule::print(std::ostream &out) const {
    auto it(bounds.begin()), ie(bounds.end());
    if (it != ie) {
        out << *it->bound;
        out << inv(it->rel);
        ++it;
    }
    out << fun << "(" << *def.repr << ")";
    for (; it != ie; ++it) {
        out << it->rel;
        out << *it->bound;
    }
    if (!lits.empty()) {
        out << ":-";
        auto g = [](std::ostream &out, ULit const &x) { if (x) { out << *x; } else { out << "#null?"; }  };
        print_comma(out, lits, ",", g);
    }
    out << ".";
}

// {{{3 definition of HeadAggregateAccumulate

HeadAggregateAccumulate::HeadAggregateAccumulate(HeadAggregateRule &headRule, unsigned elemIndex, UTermVec &&tuple, PredicateDomain *predDom, UTerm &&predRepr, ULitVec &&lits)
    : AbstractStatement(completeRepr_(headRule.def.repr), nullptr, std::move(lits), {})
    , predDef(predRepr ? gringo_make_unique<HeadDefinition>(std::move(predRepr), predDom) : nullptr)
    , headRule(headRule)
    , elemIndex(elemIndex)
    , tuple(std::move(tuple)) { }

HeadAggregateAccumulate::~HeadAggregateAccumulate() { }

void HeadAggregateAccumulate::collectImportant(Term::VarSet &vars) {
    VarTermBoundVec bound;
    def.repr->collect(bound, false);
    if (predDef) { predDef->repr->collect(bound, false); }
    for (auto &x : tuple) { x->collect(bound, false); }
    for (auto &x : bound) { vars.emplace(x.first->name); }
}

void HeadAggregateAccumulate::report(Output::OutputBase &out) {
    out.tempVals.clear();
    bool undefined = false;
    for (auto &x : tuple) { out.tempVals.emplace_back(x->eval(undefined)); }
    out.tempLits.clear();
    for (auto &x : lits) {
        auto lit = x->toOutput();
        if (!lit.second) { out.tempLits.emplace_back(*lit.first); }
    }
    if (!undefined) {
        Value predVal(predDef ? predDef->repr->eval(undefined) : Value());
        if (!undefined) {
            Value headVal(headRule.def.repr->eval(undefined));
            assert(!undefined);
            assert(headRule.domain.domain.find(headVal) != headRule.domain.domain.end());
            auto &state(headRule.domain.domain.find(headVal)->second);
            state.accumulate(out.tempVals, headRule.fun, predDef ? &static_cast<PredicateDomain*>(predDef->domain)->reserve(predVal) : nullptr, elemIndex, out.tempLits, tuple.empty() ? def.repr->loc() : tuple.front()->loc());
            if (!state.todo) {
                state.todo = true;
                headRule.todo.emplace_back(state);
            }
        }
    }
}

void HeadAggregateAccumulate::printHead(std::ostream &out) const {
    out << "#accu(" << *headRule.def.repr << ",";
    if (predDef) {
        out << *predDef->repr << ",tuple(" << tuple << ")";
    }
    else { out << "#true"; }
    out << ")";
}

// {{{3 definition of HeadAggregateComplete

HeadAggregateComplete::HeadAggregateComplete(HeadAggregateRule &headRule)
    : headRule(headRule)
    , completeRepr(completeRepr_(headRule.def.repr))
    , inst(*this) { }
bool HeadAggregateComplete::isNormal() const { return false; }
void HeadAggregateComplete::analyze(Dep::Node &node, Dep &dep) {
    for (HeadAggregateAccumulate &x : accuDoms) {
        if (x.predDef) { dep.provides(node, *x.predDef, x.predDef->getRepr()); }
    }
    dep.depends(node, *this);
}
void HeadAggregateComplete::startLinearize(bool active) {
    for (HeadAggregateAccumulate &x : accuDoms) {
        if (x.predDef) { x.predDef->active = active; }
    }
    if (active) { inst = Instantiator(*this); }
}
void HeadAggregateComplete::linearize(Scripts &, bool) {
    auto binder  = gringo_make_unique<BindOnce>();
    for (HeadOccurrence &x : defBy) { x.defines(*binder->getUpdater(), &inst); }
    inst.add(std::move(binder), Instantiator::DependVec{});
    inst.finalize(Instantiator::DependVec{});
}
void HeadAggregateComplete::enqueue(Queue &q) {
    for (HeadAggregateAccumulate &x : accuDoms) {
        if (x.predDef) { x.predDef->domain->init(); }
    }
    q.enqueue(inst);
}
void HeadAggregateComplete::printHead(std::ostream &out) const {
    auto it(headRule.bounds.begin()), ie(headRule.bounds.end());
    if (it != ie) {
        out << *it->bound;
        out << inv(it->rel);
        ++it;
    }
    out << headRule.fun << "{";
    print_comma(out, accuDoms, ";", [](std::ostream &out, HeadAggregateAccumulate const &x) -> void {
        out << x.tuple;
        out << ":";
        if (x.predDef) {
            out << x.predDef->repr;
        } else {
            out << "#true";
        }
        out << ":";
        x.printHead(out);
    });

    out << "}";
    for (; it != ie; ++it) {
        out << it->rel;
        out << *it->bound;
    }
}
void HeadAggregateComplete::propagate(Queue &queue) {
    for (HeadAggregateAccumulate &x : accuDoms) {
        if (x.predDef) { x.predDef->enqueue(queue); }
    }
}
void HeadAggregateComplete::print(std::ostream &out) const {
    printHead(out);
    out << ":-" << headRule.def.repr << occType << ".";
}
void HeadAggregateComplete::report(Output::OutputBase &) {
    // NOTE: this implementation might consider aggregate elements out of order of seminaive evaluation
    //       this does not affect the correctness of the grounding and should not have a big effect on performance (either positively or negatively)
    for (HeadAggregateState &x : headRule.todo) {
        if (x.bounds.intersects(x.range(headRule.fun))) {
            for (auto &y : x.elems) {
                for (auto it(y.second.conds.begin() + y.second.imported), ie(y.second.conds.end()); it != ie; ++it) {
                    if (it->head) {
                        static_cast<PredicateDomain*>(accuDoms[it->headNum].get().predDef->domain)->insert(*it->head);
                    }
                }
                y.second.imported = y.second.conds.size();
            }
        }
        x.todo = false;
    }
    headRule.todo.clear();
}
UGTerm HeadAggregateComplete::getRepr() const { return completeRepr->gterm(); }
bool HeadAggregateComplete::isPositive() const { return true; }
bool HeadAggregateComplete::isNegative() const { return false; }
void HeadAggregateComplete::setType(OccurrenceType x) { occType = x; }
OccurrenceType HeadAggregateComplete::getType() const { return occType; }
HeadAggregateComplete::DefinedBy &HeadAggregateComplete::definedBy() { return defBy; }
void HeadAggregateComplete::checkDefined(LocSet &, SigSet const &, UndefVec &) const { }
HeadAggregateComplete::~HeadAggregateComplete() { }

// {{{3 definition of HeadAggregateLiteral

HeadAggregateLiteral::HeadAggregateLiteral(HeadAggregateRule &headRule) : headRule(headRule)                                   { }
HeadAggregateLiteral::~HeadAggregateLiteral() { }
UGTerm HeadAggregateLiteral::getRepr() const { return headRule.def.repr->gterm(); }
bool HeadAggregateLiteral::isPositive() const { return true; }
bool HeadAggregateLiteral::isNegative() const { return false; }
void HeadAggregateLiteral::setType(OccurrenceType x) { type = x; }
OccurrenceType HeadAggregateLiteral::getType() const { return type; }
BodyOcc::DefinedBy &HeadAggregateLiteral::definedBy() { return defs; }
void HeadAggregateLiteral::checkDefined(LocSet &, SigSet const &, UndefVec &) const { }
void HeadAggregateLiteral::print(std::ostream &out) const { out << *headRule.def.repr << type; }
bool HeadAggregateLiteral::isRecursive() const { return type == OccurrenceType::UNSTRATIFIED; }
BodyOcc *HeadAggregateLiteral::occurrence() { return this; }
void HeadAggregateLiteral::collect(VarTermBoundVec &vars) const { headRule.def.repr->collect(vars, true); }
UIdx HeadAggregateLiteral::index(Scripts &, BinderType type, Term::VarSet &bound) {
    return make_binder(headRule.domain, NAF::POS, *headRule.def.repr, gResult, type, isRecursive(), bound, 0);
}
Literal::Score HeadAggregateLiteral::score(Term::VarSet const &bound) { return estimate(headRule.domain.exports.size(), *headRule.def.repr, bound); }
std::pair<Output::Literal*,bool> HeadAggregateLiteral::toOutput() {
    return {nullptr, true};
}

// {{{2 Disjunction

// {{{3 definition of DisjunctionRule

DisjunctionRule::DisjunctionRule(DisjunctionComplete &complete, ULitVec &&lits)
: AbstractStatement(complete.emptyRepr(), &complete.domEmpty, std::move(lits), {})
, complete(complete) {
}

bool DisjunctionRule::isNormal() const {
    return false;
}

DisjunctionRule::~DisjunctionRule() = default;

void DisjunctionRule::report(Output::OutputBase &out) {
    bool undefined = false;
    Value repr(complete.repr->eval(undefined));
    auto &state = *complete.dom.emplace(std::piecewise_construct, std::forward_as_tuple(repr), std::forward_as_tuple()).first;
    if (!state.second.defined() && !state.second.enqueued()) {
        state.second.enqueue();
        complete.todo.emplace_back(state.second);
    }
    complete.domEmpty.insert(def.repr->eval(undefined), false);
    assert(!undefined);

    std::unique_ptr<Output::DisjunctionRule> rule(gringo_make_unique<Output::DisjunctionRule>());
    for (auto &x : lits) {
        auto ret = x->toOutput();
        if (ret.first && (out.keepFacts || !ret.second)) {
            rule->body.emplace_back(Output::ULit(ret.first->clone()));
        }
    }
    rule->repr = &state.second;
    out.output(std::move(rule));
}

// {{{3 definition of DisjunctionAccumulateCond

DisjunctionAccumulateCond::DisjunctionAccumulateCond(DisjunctionComplete &complete, unsigned elemIndex, ULitVec &&lits)
: AbstractStatement(complete.condRepr(elemIndex), &complete.domCond, std::move(lits), {})
, complete(complete) {
    auxLits.emplace_back(gringo_make_unique<PredicateLiteral>(complete.domEmpty, NAF::POS, complete.emptyRepr()));
}

DisjunctionAccumulateCond::~DisjunctionAccumulateCond() = default;

void DisjunctionAccumulateCond::report(Output::OutputBase &) {
    bool undefined = false;
    Value litRepr(complete.repr->eval(undefined));
    Value condRepr(def.repr->eval(undefined));
    assert(!undefined);

    auto &state = *complete.dom.emplace(std::piecewise_construct, std::forward_as_tuple(litRepr), std::forward_as_tuple()).first;
    Output::ULitVec cond;
    for (auto &x : lits) {
        auto y = x->toOutput();
        if (!y.second) {
            cond.emplace_back(y.first->clone());
        }
    }
    complete.domCond.insert(condRepr, cond.empty());
    auto ret = state.second.elems.emplace_back(condRepr.args()[2]);
    if (ret.first->bodies.size() != 1 || !ret.first->bodies.front().empty()) {
        if (cond.empty()) {
            ret.first->bodies.clear();
            ret.first->bodies.emplace_back();
        }
        else {
            ret.first->bodies.emplace_back(std::move(cond));
        }
    }
}

// {{{3 definition of DisjunctionAccumulateHead

DisjunctionAccumulateHead::DisjunctionAccumulateHead(DisjunctionComplete &complete, unsigned elemIndex, int headIndex, ULitVec &&lits)
: AbstractStatement(complete.headRepr(elemIndex, headIndex), nullptr, std::move(lits), {})
, complete(complete)
, headIndex(headIndex) {
    auxLits.emplace_back(gringo_make_unique<PredicateLiteral>(complete.domCond, NAF::POS, complete.condRepr(elemIndex)));
}

DisjunctionAccumulateHead::~DisjunctionAccumulateHead() = default;

void DisjunctionAccumulateHead::report(Output::OutputBase &) {
    bool undefined = false;
    Value litRepr(complete.repr->eval(undefined));
    assert(!undefined);
    Value headRepr(def.repr->eval(undefined));
    if (!undefined) {
        auto &state = *complete.dom.find(litRepr);
        auto &cond = *state.second.elems.find(headRepr.args()[2]);
        if (cond.heads.size() != 1 || !cond.heads.front().empty()) {
            Output::ULitVec head;
            for (auto &x : lits) {
                auto y = x->toOutput();
                if (!y.second) {
                    auto z = get_clone(y.first);
                    assert(z->invertible());
                    z->invert();
                    head.emplace_back(std::move(z));
                }
            }

            auto *repr = headIndex >= 0 ? complete.heads[headIndex].repr.get() : nullptr;
            Value headRepr;
            if (!repr && head.empty()) {
                cond.heads.clear();
                cond.heads.emplace_back();
            }
            else {
                if (repr) { headRepr = repr->eval(undefined); }
                assert(!undefined);
                complete.todoHead.emplace_back(headIndex, cond, headRepr, std::move(head));
            }
        }
    }
}

// }}}3
// {{{3 definition of DisjunctionComplete

DisjunctionComplete::DisjunctionComplete(UTerm &&repr)
: repr(std::move(repr))
, inst(*this) { }

UTerm DisjunctionComplete::emptyRepr() const {
    UTermVec args;
    args.emplace_back(make_locatable<ValTerm>(repr->loc(), Value::createId("empty")));
    args.emplace_back(get_clone(repr));
    args.emplace_back(make_locatable<FunctionTerm>(repr->loc(), "", UTermVec()));
    return make_locatable<FunctionTerm>(repr->loc(), "#accu", std::move(args));
}

UTerm DisjunctionComplete::condRepr(unsigned elemIndex) const {
    UTermVec args;
    args.emplace_back(make_locatable<ValTerm>(repr->loc(), Value::createId("cond")));
    args.emplace_back(get_clone(repr));
    UTermVec condArgs = get_clone(locals[elemIndex]);
    condArgs.emplace_back(make_locatable<ValTerm>(repr->loc(), Value::createNum(elemIndex)));
    args.emplace_back(make_locatable<FunctionTerm>(repr->loc(), "", std::move(condArgs)));
    return make_locatable<FunctionTerm>(repr->loc(), "#accu", std::move(args));
}

UTerm DisjunctionComplete::headRepr(unsigned elemIndex, int headIndex) const {
    UTermVec args;
    if (headIndex >= 0) {
        UTermVec headArgs;
        headArgs.emplace_back(get_clone(heads[headIndex].repr));
        args.emplace_back(make_locatable<FunctionTerm>(repr->loc(), "head", std::move(headArgs)));
    }
    else { args.emplace_back(make_locatable<ValTerm>(repr->loc(), Value::createId("head"))); }
    args.emplace_back(get_clone(repr));
    UTermVec condArgs = get_clone(locals[elemIndex]);
    condArgs.emplace_back(make_locatable<ValTerm>(repr->loc(), Value::createNum(elemIndex)));
    args.emplace_back(make_locatable<FunctionTerm>(repr->loc(), "", std::move(condArgs)));
    return make_locatable<FunctionTerm>(repr->loc(), "#accu", std::move(args));
}

UTerm DisjunctionComplete::accuRepr() const {
    UTermVec args;
    args.emplace_back(make_locatable<VarTerm>(repr->loc(), "#Any1", std::make_shared<Value>(Value::createNum(0))));
    args.emplace_back(get_clone(repr));
    args.emplace_back(make_locatable<VarTerm>(repr->loc(), "#Any2", std::make_shared<Value>(Value::createNum(0))));
    return make_locatable<FunctionTerm>(repr->loc(), "#accu", std::move(args));
}

bool DisjunctionComplete::isNormal() const {
    return true;
}

void DisjunctionComplete::analyze(Dep::Node &node, Dep &dep) {
    dep.depends(node, *this);
    for (auto &def : heads) {
        dep.provides(node, def, def.getRepr());
    }
}

void DisjunctionComplete::startLinearize(bool active) {
    for (auto &def : heads) {
        def.active = active;
    }
    if (active) { inst = Instantiator(*this); }
}

void DisjunctionComplete::linearize(Scripts &, bool) {
    auto binder  = gringo_make_unique<BindOnce>();
    for (HeadOccurrence &x : defBy) { x.defines(*binder->getUpdater(), &inst); }
    inst.add(std::move(binder), Instantiator::DependVec{});
    inst.finalize(Instantiator::DependVec{});
}

void DisjunctionComplete::enqueue(Queue &q) {
    for (HeadDefinition &x : heads) {
        x.domain->init();
    }
    q.enqueue(inst);
}

void DisjunctionComplete::printHead(std::ostream &out) const {
    bool comma = false;
    for (auto &def : heads) {
        if (def.repr) {
            if (comma) { out << "|"; }
            else { comma = true; }
            out << *def.repr;
        }
    }
}

void DisjunctionComplete::propagate(Queue &queue) {
    for (auto &def : heads) {
        def.enqueue(queue);
    }
}

void DisjunctionComplete::report(Output::OutputBase &) {
    // NOTE: in theory I could scan first if there is a fact in the disjunction
    for (auto &todo : todoHead) {
        Output::DisjunctionElement &cond = std::get<1>(todo);
        if (cond.heads.size() != 1 || !cond.heads.front().empty()) {
            int headIndex = std::get<0>(todo);
            Output::ULitVec &head = std::get<3>(todo);
            if (headIndex >= 0) {
                auto h = static_cast<PredicateDomain*>(heads[headIndex].domain)->insert(std::get<2>(todo), false);
                head.emplace_back(gringo_make_unique<Output::PredicateLiteral>(NAF::POS, *std::get<0>(h)));
            }
            if (head.empty()) {
                // NOTE: the head is a conjunction of disjunctions
                cond.heads.clear();
            }
            cond.heads.emplace_back(std::move(head));
        }
    }
    todoHead.clear();
    for (Output::DisjunctionState &x : todo) {
        // NOTE: it is not really meaningful to use a domain here
        x.generation(1);
    }
    todo.clear();
}

void DisjunctionComplete::print(std::ostream &out) const{
    printHead(out);
    out << ":-" << *accuRepr() << occType;
}

UGTerm DisjunctionComplete::getRepr() const {
    return accuRepr()->gterm();
}

bool DisjunctionComplete::isPositive() const {
    return true;
}

bool DisjunctionComplete::isNegative() const {
    return false;
}

void DisjunctionComplete::setType(OccurrenceType x) {
    occType = x;
}

OccurrenceType DisjunctionComplete::getType() const {
    return occType;
}

DisjunctionComplete::DefinedBy &DisjunctionComplete::definedBy() {
    return defBy;
}

void DisjunctionComplete::checkDefined(LocSet &, SigSet const &, UndefVec &) const { }

void DisjunctionComplete::appendLocal(UTermVec &&local) {
    locals.emplace_back(std::move(local));
}

void DisjunctionComplete::appendHead(PredicateDomain &headDom, UTerm &&headRepr) {
    heads.emplace_back(std::move(headRepr), &headDom);
}

DisjunctionComplete::~DisjunctionComplete() { }

// }}}3

// }}}1

} } // namespace Ground Gringo

