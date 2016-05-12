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

#include "gringo/term.hh"
#include "gringo/logger.hh"
#include "gringo/graph.hh"
#include <cmath>

namespace Gringo {

// {{{ definition of Defines

Defines::Defines() = default;

Defines::Defines(Defines &&) = default;

Defines::DefMap const &Defines::defs() const {
    return defs_;
}

void Defines::add(Location const &loc, FWString name, UTerm &&value, bool defaultDef) {
    auto it = defs_.find(name);
    if (it == defs_.end())                           { defs_.emplace(name, make_tuple(defaultDef, loc, std::move(value))); }
    else if (std::get<0>(it->second) && !defaultDef) { it->second = make_tuple(defaultDef, loc, std::move(value)); }
    else if (std::get<0>(it->second) || !defaultDef) {
        GRINGO_REPORT(E_ERROR)
            << loc << ": error: redefinition of constant:\n"
            << "  #const " << *name << "=" << *value << ".\n"
            << std::get<1>(it->second) << ": note: constant also defined here\n";
    }
}

void Defines::init() {
    using DefineGraph = Graph<Defines::DefMap::iterator>;
    using NodeMap     = std::unordered_map<FWString, DefineGraph::Node*>;

    DefineGraph graph;
    NodeMap nodes;
    for (auto it = defs_.begin(), end = defs_.end(); it != end; ++it) {
        nodes.emplace(it->first, &graph.insertNode(it));
    }
    for (auto &x : nodes) {
        Term::VarSet vals;
        std::get<2>(x.second->data->second)->collectIds(vals);
        for (auto &y : vals) {
            auto it = nodes.find(y);
            if (it != nodes.end()) { x.second->insertEdge(*it->second); }
        }
    }
    for (auto &scc : graph.tarjan()) {
        if (scc.size() > 1) {
            std::ostringstream msg;
            msg
                << std::get<1>(scc.back()->data->second) << ": error: cyclic constant definition:\n"
                << "  #const " << *scc.back()->data->first << "=" << *std::get<2>(scc.back()->data->second) << ".\n";
            scc.pop_back();
            for (auto &x : scc) {
                msg
                    << std::get<1>(x->data->second) << ": note: cycle involves definition:\n"
                    << "  #const " << *x->data->first << "=" << *std::get<2>(x->data->second) << ".\n";
            }
            GRINGO_REPORT(E_ERROR) << msg.str();
        }
        for (auto &x : scc) { Term::replace(std::get<2>(x->data->second), std::get<2>(x->data->second)->replace(*this, true)); }
    }
}

bool Defines::empty() const { return defs_.empty(); }

void Defines::apply(Value x, Value &retVal, UTerm &retTerm, bool replace) {
    switch (x.type())  {
        case Value::FUNC: {
            ValVec args;
            bool changed = true;
            for (unsigned i = 0, e = x.args().size(); i != e; ++i) {
                UTerm rt;
                args.emplace_back();
                apply(x.args()[i], args.back(), rt, true);
                if (rt) {
                    Location loc{rt->loc()};
                    UTermVec tArgs;
                    args.pop_back();
                    for (auto &y : args) { tArgs.emplace_back(make_locatable<ValTerm>(rt->loc(), y)); }
                    tArgs.emplace_back(std::move(rt));
                    for (++i; i != e; ++i) {
                        Value rv;
                        tArgs.emplace_back();
                        apply(x.args()[i], rv, tArgs.back(), true);
                        if (!tArgs.back()) {
                            if (rv.type() == Value::SPECIAL) { rv = x.args()[i]; }
                            tArgs.back() = make_locatable<ValTerm>(loc, rv);
                        }
                    }
                    retTerm = make_locatable<FunctionTerm>(loc, x.name(), std::move(tArgs));
                    return;
                }
                else if (args.back().type() == Value::SPECIAL) { args.back() = x.args()[i]; }
                else                                           { changed = true; }
            }
            if (changed) { retVal = Value::createFun(x.name(), args); }
            break;
        }
        case Value::ID: {
            if (replace) {
                auto it(defs_.find(x.string()));
                if (it != defs_.end()) {
                    retVal = std::get<2>(it->second)->isEDB();
                    if (retVal.type() == Value::SPECIAL) {
                        retTerm = get_clone(std::get<2>(it->second));
                    }
                }
            }
            break;
        }
        default: { break; }
    }
}

Defines::~Defines() { }

// }}}

// {{{ definition of GRef

GRef::GRef(UTerm &&name)
    : type(EMPTY)
    , name(std::move(name))
    , value(Value::createNum(0))
    , term(0) { }

GRef::operator bool() const { return type != EMPTY; }

void GRef::reset() { type = EMPTY; }

GRef &GRef::operator=(Value const &x) {
    type  = VALUE;
    value = x;
    return *this;
}

GRef &GRef::operator=(GTerm &x) {
    type = TERM;
    term = &x;
    return *this;
}

bool GRef::occurs(GRef &x) const {
    switch (type) {
        case VALUE: { return false; }
        case TERM:  { return term->occurs(x); }
        case EMPTY: { return this == &x; }
    }
    assert(false);
    return false;
}

bool GRef::match(Value const &x) {
    switch (type) {
        case VALUE: { return value == x; }
        case TERM:  { return term->match(x); }
        case EMPTY: { assert(false); }
    }
    return false;
}

template <class T>
bool GRef::unify(T &x) {
switch (type) {
    case VALUE: { return x.match(value); }
    case TERM:  { return term->unify(x); }
    case EMPTY: { assert(false); }
}
return false;

}

// }}}

// {{{1 definition of GValTerm

GValTerm::GValTerm(Value val) : val(val) { }

bool GValTerm::operator==(GTerm const &x) const {
    auto t = dynamic_cast<GValTerm const*>(&x);
    return t && val == t->val;
}

size_t GValTerm::hash() const { return get_value_hash(typeid(GValTerm).hash_code(), val); }

void GValTerm::print(std::ostream &out) const { out << val; }

FWSignature GValTerm::sig() const { return val.sig(); }

GTerm::EvalResult GValTerm::eval() const { return EvalResult(true, val); }

bool GValTerm::occurs(GRef &) const { return false; }

void GValTerm::reset() { }

bool GValTerm::match(Value const &x) { return val == x; }

bool GValTerm::unify(GTerm &x) { return x.match(val); }

bool GValTerm::unify(GFunctionTerm &x) { return x.match(val); }

bool GValTerm::unify(GLinearTerm &x) { return x.match(val); }

bool GValTerm::unify(GVarTerm &x) { return x.match(val); }

GValTerm::~GValTerm() { }

// {{{1 definition of GFunctionTerm

GFunctionTerm::GFunctionTerm(FWString name, UGTermVec &&args) : sign(false), name(name), args(std::move(args)) { }

// Note: uses structural comparisson of names of variable terms (VarTerm/LinearTerm)
bool GFunctionTerm::operator==(GTerm const &x) const {
    auto t = dynamic_cast<GFunctionTerm const*>(&x);
    return t && sig() == x.sig() && is_value_equal_to(args, t->args);
}

size_t GFunctionTerm::hash() const { return get_value_hash(typeid(GFunctionTerm).hash_code(), sig(), args); }

void GFunctionTerm::print(std::ostream &out) const {
    if ((*sig()).sign()) {
        out << "-";
    }
    out << name;
    out << "(";
    print_comma(out, args, ",", [](std::ostream &out, UGTerm const &x) { out << *x; });
    out << ")";
}

FWSignature GFunctionTerm::sig() const { return FWSignature(name, args.size(), sign); }

GTerm::EvalResult GFunctionTerm::eval() const { return EvalResult(false, Value()); }

bool GFunctionTerm::occurs(GRef &x) const {
    for (auto &y : args) {
        if (y->occurs(x)) { return true; }
    }
    return false;
}

void GFunctionTerm::reset() {
    for (auto &y : args) { y->reset(); }
}

bool GFunctionTerm::match(Value const &x) {
    if (x.type() != Value::FUNC || sig() != x.sig()) { return false; }
    else {
        auto i = 0;
        for (auto &y : args) {
            if (!y->match(x.args()[i++])) { return false; }
        }
        return true;
    }
}

bool GFunctionTerm::unify(GTerm &x) { return x.unify(*this); }

bool GFunctionTerm::unify(GFunctionTerm &x) {
    if (sig() != x.sig()) { return false; }
    else {
        for (auto it = args.begin(), jt = x.args.begin(), ie = args.end(); it != ie; ++it, ++jt) {
            if (!(*it)->unify(**jt)) { return false; }
        }
        return true;
    }
}

bool GFunctionTerm::unify(GLinearTerm &) { return false; }

bool GFunctionTerm::unify(GVarTerm &x) {
    if (*x.ref) { return x.ref->unify(*this); }
    else if (!occurs(*x.ref)) {
        *x.ref = *this;
        return true;
    }
    else { return false; }
}

GFunctionTerm::~GFunctionTerm() { }

// {{{1 definition of GLinearTerm

GLinearTerm::GLinearTerm(SGRef ref, int m, int n) : ref(ref), m(m), n(n) { assert(ref); }

bool GLinearTerm::operator==(GTerm const &x) const {
    auto t = dynamic_cast<GLinearTerm const*>(&x);
    return t && *ref->name == *t->ref->name && m == t->m && n == t->n;
}

size_t GLinearTerm::hash() const   { return get_value_hash(typeid(GLinearTerm).hash_code(), ref->name, m, n); }

void GLinearTerm::print(std::ostream &out) const { out << "(" << m << "*" << *ref->name << "+" << n << ")"; }

FWSignature GLinearTerm::sig() const   { throw std::logic_error("must not be called"); }

GTerm::EvalResult GLinearTerm::eval() const   { return EvalResult(false, Value()); }

bool GLinearTerm::occurs(GRef &x) const { return ref->occurs(x); }

void GLinearTerm::reset() { ref->reset(); }

bool GLinearTerm::match(Value const &x) {
    if (x.type() != Value::NUM) { return false; }
    else {
        int y = x.num();
        y-= n;
        if (y % m != 0) { return false; }
        else {
            y /= m;
            if (*ref) { return ref->match(Value::createNum(y)); }
            else {
                *ref = Value::createNum(y);
                return true;
            }
        }
    }
}

bool GLinearTerm::unify(GTerm &x)   { return x.unify(*this); }

bool GVarTerm::unify(GFunctionTerm &x) {
    if (*ref) { return ref->unify(x); }
    else if (!x.occurs(*ref)) {
        *ref = x;
        return true;
    }
    else { return false; }
}

bool GLinearTerm::unify(GLinearTerm &) {
    // Note: more could be done but this would be somewhat involved
    //       because this would require rational numbers
    //       as of now this simply unifies too much
    return true;
}

bool GLinearTerm::unify(GFunctionTerm &) { return false; }

bool GLinearTerm::unify(GVarTerm &x) {
    if (*x.ref) { return x.ref->unify(*this); }
    else {
        // see not at: GLinearTerm::unify(GLinearTerm &x)
        return true;
    }
}

GLinearTerm::~GLinearTerm() { }

// {{{1 definition of GVarTerm

GVarTerm::GVarTerm(SGRef ref) : ref(ref) { assert(ref); }

bool GVarTerm::operator==(GTerm const &x) const {
    auto t = dynamic_cast<GVarTerm const*>(&x);
    return t && *ref->name == *t->ref->name;
}

size_t GVarTerm::hash() const      { return get_value_hash(typeid(GVarTerm).hash_code(), ref->name); }

void GVarTerm::print(std::ostream &out) const    { out << *ref->name; }

FWSignature GVarTerm::sig() const      { throw std::logic_error("must not be called"); }

GTerm::EvalResult GVarTerm::eval() const      { return EvalResult(false, Value()); }

bool GVarTerm::occurs(GRef &x) const { return ref->occurs(x); }

void GVarTerm::reset()    { ref->reset(); }

bool GVarTerm::match(Value const &x) {
    if (*ref) { return ref->match(x); }
    else {
        *ref = x;
        return true;
    }
}

bool GVarTerm::unify(GTerm &x)      { return x.unify(*this); }

bool GVarTerm::unify(GLinearTerm &x) {
    if (*ref) { return ref->unify(x); }
    else {
        // see note at: GLinearTerm::unify(GLinearTerm &x)
        return true;
    }
}

bool GVarTerm::unify(GVarTerm &x) {
    if (*ref)        { return ref->unify(x); }
    else if (*x.ref) { return x.ref->unify(*this); }
    else if (ref->name != x.ref->name) {
        *ref = x;
        return true;
    }
    else { return true; }
}

GVarTerm::~GVarTerm() { }

// }}}1

// {{{1 definition of operator<< for BinOp and UnOp

int eval(UnOp op, int x) {
    switch (op) {
        case UnOp::NEG: { return -x; }
        case UnOp::ABS: { return std::abs(x); }
        case UnOp::NOT: { return ~x; }
    }
    assert(false);
    return 0;
}

std::ostream &operator<<(std::ostream &out, UnOp op) {
    switch (op) {
        case UnOp::ABS: { out << "#abs"; break; }
        case UnOp::NOT: { out << "~"; break; }
        case UnOp::NEG: { out << "-"; break; }
    }
    return out;
}

int eval(BinOp op, int x, int y) {
    switch (op) {
        case BinOp::XOR: { return x ^ y; }
        case BinOp::OR:  { return x | y; }
        case BinOp::AND: { return x & y; }
        case BinOp::ADD: { return x + y; }
        case BinOp::SUB: { return x - y; }
        case BinOp::MUL: { return x * y; }
        case BinOp::MOD: { return x % y; }
        case BinOp::POW: { return ipow(x, y); }
        case BinOp::DIV: {
            assert(y != 0 && "must be checked before call");
            return x / y;
        }
    }
    assert(false);
    return 0;
}

int Term::toNum(bool &undefined) {
    Value y(eval(undefined));
	if (y.type() == Value::NUM) { return y.num(); }
	else {
        undefined = true;
		GRINGO_REPORT(W_OPERATION_UNDEFINED)
			<< loc() << ": info: number expected:\n"
			<< "  " << *this << "\n";
		return 0;
	}
}

std::ostream &operator<<(std::ostream &out, BinOp op) {
    switch (op) {
        case BinOp::AND: { out << "&"; break; }
        case BinOp::OR:  { out << "?"; break; }
        case BinOp::XOR: { out << "^"; break; }
        case BinOp::POW: { out << "**"; break; }
        case BinOp::ADD: { out << "+"; break; }
        case BinOp::SUB: { out << "-"; break; }
        case BinOp::MUL: { out << "*"; break; }
        case BinOp::DIV: { out << "/"; break; }
        case BinOp::MOD: { out << "\\"; break; }
    }
    return out;
}

// {{{1 definition of Term and auxiliary functions

namespace {

UTerm wrap(UTerm &&x) {
    UTermVec args;
    args.emplace_back(std::move(x));
    return make_locatable<FunctionTerm>(args.front()->loc(), "#b", std::move(args));
}

} // namespace

UTermVec unpool(UTerm const &x) {
    UTermVec pool;
    x->unpool(pool);
    return pool;
}

SGRef Term::_newRef(RenameMap &names, Term::ReferenceMap &refs) const {
    UTerm x(renameVars(names));
    auto &ref = refs[x.get()];
    if (!ref) { ref = std::make_shared<GRef>(std::move(x)); }
    return ref;
}

UGTerm Term::gterm() const {
    RenameMap names;
    ReferenceMap refs;
    return gterm(names, refs);
}


UGFunTerm Term::gfunterm(RenameMap &, ReferenceMap &) const {
    return nullptr;
}

void Term::collect(VarTermSet &x) const {
    VarTermBoundVec vars;
    collect(vars, false);
    for (auto &y : vars) { x.emplace(*y.first); }
}


bool Term::isZero() const {
    bool undefined;
    return getInvertibility() == Term::CONSTANT && eval(undefined) == Value::createNum(0);
}

bool Term::bind(VarSet &bound) {
    VarTermBoundVec occs;
    collect(occs, false);
    bool ret = false;
    for (auto &x : occs) {
        if ((x.first->bindRef = bound.insert(x.first->name).second)) { ret = true; }
    }
    return ret;
}

UTerm Term::insert(ArithmeticsMap &arith, AuxGen &auxGen, UTerm &&term, bool eq) {
    unsigned level = term->getLevel();
    assert(level < arith.size());
    auto ret = arith[level].emplace(std::move(term), nullptr);
    if (ret.second) { ret.first->second = auxGen.uniqueVar(ret.first->first->loc(), level, "#Arith"); }
    if (eq) {
        auto ret2 = arith[level].emplace(get_clone(ret.first->second), nullptr);
        if (ret2.second) { ret2.first->second = get_clone(ret.first->first); }
    }
    return get_clone(ret.first->second);
}

// {{{1 definition of AuxGen

FWString AuxGen::uniqueName(char const *prefix) {
    return FWString(prefix + std::to_string((*auxNum)++));
}

UTerm AuxGen::uniqueVar(Location const &loc, unsigned level, const char *prefix) {
    return make_locatable<VarTerm>(loc, uniqueName(prefix), std::make_shared<Value>(), level);
}

// {{{1 definition of SimplifyState

std::unique_ptr<LinearTerm> SimplifyState::createScript(Location const &loc, FWString name, UTermVec &&args) {
    scripts.emplace_back(gen.uniqueVar(loc, 0, "#Script"), name, std::move(args));
    return make_locatable<LinearTerm>(loc, static_cast<VarTerm&>(*std::get<0>(scripts.back())), 1, 0);
}

std::unique_ptr<LinearTerm> SimplifyState::createDots(Location const &loc, UTerm &&left, UTerm &&right) {
    dots.emplace_back(gen.uniqueVar(loc, 0, "#Range"), std::move(left), std::move(right));
    return make_locatable<LinearTerm>(loc, static_cast<VarTerm&>(*std::get<0>(dots.back())), 1, 0);
}


// {{{1 definition of Term::SimplifyRet

Term::SimplifyRet::SimplifyRet(SimplifyRet &&x) : type(x.type) {
    switch(type) {
        case LINEAR:
        case REPLACE:   { x.type = UNTOUCHED; }
        case UNTOUCHED: {
            term = x.term;
            break;
        }
        case UNDEFINED:
        case CONSTANT: {
            val = x.val;
            break;
        }
    }
}
//! Reference to untouched term.
Term::SimplifyRet::SimplifyRet(Term &x, bool project) : type(UNTOUCHED), project(project), term(&x) { }
//! Indicate replacement with linear term.
Term::SimplifyRet::SimplifyRet(std::unique_ptr<LinearTerm> &&x) : type(LINEAR), project(false), term(x.release()) { }
//! Indicate replacement with arbitrary term.
Term::SimplifyRet::SimplifyRet(UTerm &&x) : type(REPLACE), project(false), term(x.release()) { }
//! Indicate replacement with value.
Term::SimplifyRet::SimplifyRet(Value const &x) : type(CONSTANT), project(false), val(x) { }
Term::SimplifyRet::SimplifyRet() : type(UNDEFINED), project(false), val({}) { }
bool Term::SimplifyRet::notNumeric() const {
    switch (type) {
        case UNDEFINED: { return true; }
        case LINEAR:    { return false; }
        case CONSTANT:  { return val.type() != Value::NUM; }
        case REPLACE:
        case UNTOUCHED: { return term->isNotNumeric(); }
    }
    assert(false);
    return false;
}
bool Term::SimplifyRet::constant() const  { return type == CONSTANT; }
bool Term::SimplifyRet::isZero() const    { return constant() && val.type() == Value::NUM && val.num() == 0; }
LinearTerm &Term::SimplifyRet::lin()      { return static_cast<LinearTerm&>(*term); }
Term::SimplifyRet &Term::SimplifyRet::update(UTerm &x) {
    switch (type) {
        case CONSTANT: {
            x = make_locatable<ValTerm>(x->loc(), val);
            return *this;
        }
        case LINEAR: {
            if (lin().m == 1 && lin().n == 0) {
                type = UNTOUCHED;
                x = std::move(lin().var);
                delete term;
                return *this;
            }
        }
        case REPLACE:  {
            type = UNTOUCHED;
            x.reset(term);
            return *this;
        }
        case UNDEFINED:
        case UNTOUCHED: { return *this; }
    }
    throw std::logic_error("Term::SimplifyRet::update: must not happen");
}
bool Term::SimplifyRet::undefined() const {
    return type == UNDEFINED;
}
bool Term::SimplifyRet::notFunction() const {
    switch (type) {
        case UNDEFINED: { return true; }
        case LINEAR:    { return true; }
        case CONSTANT:  { return val.type() != Value::ID && val.type() != Value::FUNC; }
        case REPLACE:
        case UNTOUCHED: { return term->isNotFunction(); }
    }
    assert(false);
    return false;
}
Term::SimplifyRet::~SimplifyRet() {
    if (type == LINEAR || type == REPLACE) { delete term; }
}

// }}}1

// {{{1 definition of PoolTerm

PoolTerm::PoolTerm(UTermVec &&terms)
    : args(std::move(terms)) { }

void PoolTerm::rename(FWString) {
    throw std::logic_error("must not be called");
}

unsigned PoolTerm::getLevel() const {
    unsigned level = 0;
    for (auto &x : args) { level = std::max(x->getLevel(), level); }
    return level;
}

unsigned PoolTerm::projectScore() const {
    throw std::logic_error("Term::projectScore must be called after Term::unpool");
}

bool PoolTerm::isNotNumeric() const { return false; }
bool PoolTerm::isNotFunction() const { return false; }

Term::Invertibility PoolTerm::getInvertibility() const     { return Term::NOT_INVERTIBLE; }

void PoolTerm::print(std::ostream &out) const    { print_comma(out, args, ";", [](std::ostream &out, UTerm const &y) { out << *y; }); }

Term::SimplifyRet PoolTerm::simplify(SimplifyState &, bool, bool) {
    throw std::logic_error("Term::simplify must be called after Term::unpool");
}

Term::ProjectRet PoolTerm::project(bool, AuxGen &) {
    throw std::logic_error("Term::project must be called after Term::unpool");
}

bool PoolTerm::hasVar() const {
    for (auto &x : args) {
        if (x->hasVar()) { return true; }
    }
    return false;
}

bool PoolTerm::hasPool() const   { return true; }

void PoolTerm::collect(VarTermBoundVec &vars, bool bound) const {
    for (auto &y : args) { y->collect(vars, bound); }
}

void PoolTerm::collect(VarSet &vars, unsigned minLevel , unsigned maxLevel) const {
    for (auto &y : args) { y->collect(vars, minLevel, maxLevel); }
}

Value PoolTerm::eval(bool &) const { throw std::logic_error("Term::unpool must be called before Term::eval"); }

bool PoolTerm::match(Value const &) const { throw std::logic_error("Term::unpool must be called before Term::match"); }

void PoolTerm::unpool(UTermVec &x) const {
    for (auto &t : args) { t->unpool(x); }
}

UTerm PoolTerm::rewriteArithmetics(Term::ArithmeticsMap &, AuxGen &, bool) {
    throw std::logic_error("Term::rewriteArithmetics must be called before Term::rewriteArithmetics");
}

bool PoolTerm::operator==(Term const &x) const {
    auto t = dynamic_cast<PoolTerm const*>(&x);
    return t && is_value_equal_to(args, t->args);
}

size_t PoolTerm::hash() const {
    return get_value_hash(typeid(PoolTerm).hash_code(), args);
}

PoolTerm *PoolTerm::clone() const {
    return make_locatable<PoolTerm>(loc(), get_clone(args)).release();
}

FWSignature PoolTerm::getSig() const { assert(false); throw std::logic_error("Term::getSig must not be called on PoolTerm"); }

UTerm PoolTerm::renameVars(RenameMap &names) const {
    UTermVec args;
    for (auto &x : this->args) { args.emplace_back(x->renameVars(names)); }
    return make_locatable<PoolTerm>(loc(), std::move(args));
}

UGTerm PoolTerm::gterm(RenameMap &names, ReferenceMap &refs) const   { return gringo_make_unique<GVarTerm>(_newRef(names, refs)); }

void PoolTerm::collectIds(VarSet &x) const {
    for (auto &y : args) { y->collectIds(x); }
}

UTerm PoolTerm::replace(Defines &x, bool replace) {
    for (auto &y : args) { Term::replace(y, y->replace(x, replace)); }
    return nullptr;
}

double PoolTerm::estimate(double, VarSet const &) const {
    return 0;
}

Value PoolTerm::isEDB() const { return {}; }

PoolTerm::~PoolTerm() { }

// {{{1 definition of ValTerm

ValTerm::ValTerm(Value value)
    : value(value) { }

unsigned ValTerm::projectScore() const {
    return 0;
}

void ValTerm::rename(FWString x) {
    value = Value::createId(x);
}

unsigned ValTerm::getLevel() const {
    return 0;
}

bool ValTerm::isNotNumeric() const { return value.type() != Value::NUM; }
bool ValTerm::isNotFunction() const { return value.type() != Value::ID && value.type() != Value::FUNC; }

Term::Invertibility ValTerm::getInvertibility() const      { return Term::CONSTANT; }

void ValTerm::print(std::ostream &out) const     { out << value; }

Term::SimplifyRet ValTerm::simplify(SimplifyState &, bool, bool) { return {value}; }

Term::ProjectRet ValTerm::project(bool rename, AuxGen &) {
    assert(!rename); (void)rename;
    return std::make_tuple(nullptr, UTerm(clone()), UTerm(clone()));
}

bool ValTerm::hasVar() const {
    return false;
}

bool ValTerm::hasPool() const { return false; }

void ValTerm::collect(VarTermBoundVec &, bool) const { }

void ValTerm::collect(VarSet &, unsigned, unsigned) const { }

Value ValTerm::eval(bool &) const { return value; }

bool ValTerm::match(Value const &x) const { return value == x; }

void ValTerm::unpool(UTermVec &x) const {
    x.emplace_back(UTerm(clone()));
}

UTerm ValTerm::rewriteArithmetics(Term::ArithmeticsMap &, AuxGen &, bool) { return nullptr; }

bool ValTerm::operator==(Term const &x) const {
    auto t = dynamic_cast<ValTerm const*>(&x);
    return t && value == t->value;
}

size_t ValTerm::hash() const {
    return get_value_hash(typeid(ValTerm).hash_code(), value);
}

ValTerm *ValTerm::clone() const {
    return make_locatable<ValTerm>(loc(), value).release();
}

FWSignature ValTerm::getSig() const {
    switch (value.type()) {
        case Value::ID:
        case Value::FUNC: { return value.sig(); }
        default:          { throw std::logic_error("Term::getSig must not be called on ValTerm"); }
    }
}

UTerm ValTerm::renameVars(RenameMap &) const { return UTerm(clone()); }

UGTerm ValTerm::gterm(RenameMap &, ReferenceMap &) const { return gringo_make_unique<GValTerm>(value); }

void ValTerm::collectIds(VarSet &x) const {
    if (value.type() == Value::ID) { x.emplace(value.string()); }
}

UTerm ValTerm::replace(Defines &x, bool replace) {
    Value retVal;
    UTerm retTerm;
    x.apply(value, retVal, retTerm, replace);
    if (retVal.type() != Value::SPECIAL) { value = retVal; }
    else                                 { return retTerm; }
    return nullptr;
}

double ValTerm::estimate(double, VarSet const &) const {
    return 0;
}

Value ValTerm::isEDB() const { return value; }

ValTerm::~ValTerm() { }

// {{{1 definition of VarTerm

VarTerm::VarTerm(FWString name, SVal ref, unsigned level, bool bindRef)
    : name(name)
    , ref(ref)
    , bindRef(bindRef)
    , level(level) { assert(ref || *name == "_"); }

unsigned VarTerm::projectScore() const {
    return 0;
}

void VarTerm::rename(FWString) {
    throw std::logic_error("must not be called");
}

unsigned VarTerm::getLevel() const {
    return level;
}

bool VarTerm::isNotNumeric() const { return false; }
bool VarTerm::isNotFunction() const { return false; }

Term::Invertibility VarTerm::getInvertibility() const { return Term::INVERTIBLE; }

void VarTerm::print(std::ostream &out) const { out << *(name); }

Term::SimplifyRet VarTerm::simplify(SimplifyState &state, bool positional, bool arithmetic) {
    if (name == "_") {
        ref = std::make_shared<Value>();
        if (positional) { return {*this, true}; }
        else { name = state.gen.uniqueName("#Anon"); }
    }
    if (arithmetic) { return {make_locatable<LinearTerm>(loc(), *this, 1, 0)}; }
    else            { return {*this, false}; }
}

Term::ProjectRet VarTerm::project(bool rename, AuxGen &auxGen) {
    assert(!rename); (void)rename;
    if (*name == "_") {
        UTerm r(make_locatable<ValTerm>(loc(), Value::createId("#p")));
        UTerm x(r->clone());
        UTerm y(auxGen.uniqueVar(loc(), 0, "#P"));
        return std::make_tuple(std::move(r), std::move(x), std::move(y));
    }
    else {
        UTerm y(auxGen.uniqueVar(loc(), 0, "#X"));
        UTerm x(wrap(UTerm(y->clone())));
        return std::make_tuple(wrap(UTerm(clone())), std::move(x), std::move(y));
    }
}

bool VarTerm::hasVar() const { return true; }

bool VarTerm::hasPool() const    { return false; }

void VarTerm::collect(VarTermBoundVec &vars, bool bound) const {
    vars.emplace_back(const_cast<VarTerm*>(this), bound);
}

void VarTerm::collect(VarSet &vars, unsigned minLevel , unsigned maxLevel) const {
    if (minLevel <= level && level <= maxLevel) { vars.emplace(name); }
}

Value VarTerm::eval(bool &) const { return *ref; }

bool VarTerm::match(Value const &x) const {
    if (bindRef) {
        *ref = x;
        return true;
    }
    else { return x == *ref; }
}

void VarTerm::unpool(UTermVec &x) const { x.emplace_back(UTerm(clone())); }

UTerm VarTerm::rewriteArithmetics(Term::ArithmeticsMap &, AuxGen &, bool) { return nullptr; }

bool VarTerm::operator==(Term const &x) const {
    auto t = dynamic_cast<VarTerm const*>(&x);
    return t && *name == *t->name && level == t->level;
}

size_t VarTerm::hash() const {
    return get_value_hash(typeid(VarTerm).hash_code(), *name, level);
}

VarTerm *VarTerm::clone() const {
    return make_locatable<VarTerm>(loc(), name, ref, level, bindRef).release();
}

FWSignature VarTerm::getSig() const { throw std::logic_error("Term::getSig must not be called on VarTerm"); }

UTerm VarTerm::renameVars(RenameMap &names) const {
    auto ret(names.emplace(name, std::make_pair(name, nullptr)));
    if (ret.second) {
        ret.first->second.first  = (bindRef ? "X" : "Y") + std::to_string(names.size() - 1);
        ret.first->second.second = std::make_shared<Value>();
    }
    return make_locatable<VarTerm>(loc(), ret.first->second.first, ret.first->second.second, 0, bindRef);
}

UGTerm VarTerm::gterm(RenameMap &names, ReferenceMap &refs) const { return gringo_make_unique<GVarTerm>(_newRef(names, refs)); }

void VarTerm::collectIds(VarSet &) const { }

UTerm VarTerm::replace(Defines &, bool) { return nullptr; }

double VarTerm::estimate(double size, VarSet const &bound) const {
    return bound.find(name) == bound.end() ? size : 0.0;
}

Value VarTerm::isEDB() const { return {}; }

VarTerm::~VarTerm() { }

// {{{1 definition of LinearTerm

LinearTerm::LinearTerm(VarTerm const &var, int m, int n)
    : var(static_cast<VarTerm*>(var.clone()))
    , m(m)
    , n(n) { }

LinearTerm::LinearTerm(UVarTerm &&var, int m, int n)
    : var(std::move(var))
    , m(m)
    , n(n) { }

unsigned LinearTerm::projectScore() const {
    return 0;
}

void LinearTerm::rename(FWString) {
    throw std::logic_error("must not be called");
}

bool LinearTerm::hasVar() const { return true; }

bool LinearTerm::hasPool() const { return false; }

void LinearTerm::collect(VarTermBoundVec &vars, bool bound) const {
    var->collect(vars, bound);
}

void LinearTerm::collect(VarSet &vars, unsigned minLevel , unsigned maxLevel) const {
    var->collect(vars, minLevel, maxLevel);
}

unsigned LinearTerm::getLevel() const {
    return var->getLevel();
}

bool LinearTerm::isNotNumeric() const   { return false; }
bool LinearTerm::isNotFunction() const   { return true; }

Term::Invertibility LinearTerm::getInvertibility() const   { return Term::INVERTIBLE; }

void LinearTerm::print(std::ostream &out) const  { out << "(" << m << "*" << *var << "+" << n << ")"; }

Term::SimplifyRet LinearTerm::simplify(SimplifyState &, bool, bool) { return {*this, false}; }

Term::ProjectRet LinearTerm::project(bool rename, AuxGen &auxGen) {
    assert(!rename); (void)rename;
    UTerm y(auxGen.uniqueVar(loc(), 0, "#X"));
    UTerm x(wrap(UTerm(y->clone())));
    return std::make_tuple(wrap(make_locatable<LinearTerm>(loc(), std::move(var), m, n)), std::move(x), std::move(y));
}

Value LinearTerm::eval(bool &undefined) const {
	Value value = var->eval(undefined);
	if (value.type() == Value::NUM) { return Value::createNum(m * value.num() + n); }
	else {
        undefined = true;
		GRINGO_REPORT(W_OPERATION_UNDEFINED)
			<< loc() << ": info: operation undefined:\n"
			<< "  " << *this << "\n";
		return Value::createNum(0);
	}
}

bool LinearTerm::match(Value const &x) const {
	if (x.type() == Value::NUM) {
		assert(m != 0);
		int c(x.num() - n);
		if (c % m == 0) { return var->match(Value::createNum(c/m)); }
	}
	return false;
}

void LinearTerm::unpool(UTermVec &x) const {
    x.emplace_back(UTerm(clone()));
}

UTerm LinearTerm::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen, bool forceDefined) {
    if (forceDefined) {
        return Term::insert(arith, auxGen, make_locatable<LinearTerm>(loc(), *var, m, n), true);
    }
    return nullptr;
}

bool LinearTerm::operator==(Term const &x) const {
    auto t = dynamic_cast<LinearTerm const*>(&x);
    return t && m == t->m && n == t->n && is_value_equal_to(var, t->var);
}

size_t LinearTerm::hash() const {
    return get_value_hash(typeid(LinearTerm).hash_code(), m, n, var->hash());
}

LinearTerm *LinearTerm::clone() const {
    return make_locatable<LinearTerm>(loc(), *var, m, n).release();
}

FWSignature LinearTerm::getSig() const   { throw std::logic_error("Term::getSig must not be called on LinearTerm"); }

UTerm LinearTerm::renameVars(RenameMap &names) const {
    return make_locatable<LinearTerm>(loc(), UVarTerm(static_cast<VarTerm*>(var->renameVars(names).release())), m, n);
}

UGTerm LinearTerm::gterm(RenameMap &names, ReferenceMap &refs) const { return gringo_make_unique<GLinearTerm>(var->_newRef(names, refs), m, n); }

void LinearTerm::collectIds(VarSet &) const {
}

UTerm LinearTerm::replace(Defines &, bool) {
    return nullptr;
}

double LinearTerm::estimate(double size, VarSet const &bound) const {
    return var->estimate(size, bound);
}

Value LinearTerm::isEDB() const { return {}; }

LinearTerm::~LinearTerm() { }

// {{{1 definition of UnOpTerm

// TODO: NEG has to be handled specially now - maybe I should add a new kind of term?

UnOpTerm::UnOpTerm(UnOp op, UTerm &&arg)
    : op(op)
    , arg(std::move(arg)) { }

unsigned UnOpTerm::projectScore() const {
    return arg->projectScore();
}

void UnOpTerm::rename(FWString) { throw std::logic_error("must not be called"); }

unsigned UnOpTerm::getLevel() const { return arg->getLevel(); }

bool UnOpTerm::isNotNumeric() const { return false; }
bool UnOpTerm::isNotFunction() const { return op != UnOp::NEG; }

Term::Invertibility UnOpTerm::getInvertibility() const {
    return op == UnOp::NEG ? Term::INVERTIBLE : Term::NOT_INVERTIBLE;
}

void UnOpTerm::print(std::ostream &out) const {
    if (op == UnOp::ABS) {
        out << "|" << *arg << "|";
    }
    else {
        // TODO: parenthesis are problematic if I want to use this term for predicates
        out << "(" << op << *arg << ")";
    }
}

Term::SimplifyRet UnOpTerm::simplify(SimplifyState &state, bool, bool arithmetic) {
    bool multiNeg = !arithmetic && op == UnOp::NEG;
    SimplifyRet ret(arg->simplify(state, false, !multiNeg));
    if (ret.undefined()) {
        return {};
    }
    else if ((multiNeg && ret.notNumeric() && ret.notFunction()) || (!multiNeg && ret.notNumeric())) {
        GRINGO_REPORT(W_OPERATION_UNDEFINED)
            << loc() << ": info: operation undefined:\n"
            << "  " << *this << "\n";
        return {};
    }
    switch (ret.type) {
        case SimplifyRet::CONSTANT: {
            if (ret.val.type() == Value::NUM) {
                return {Value::createNum(Gringo::eval(op, ret.val.num()))};
            }
            else {
                assert(ret.val.type() == Value::FUNC || ret.val.type() == Value::ID);
                return {ret.val.flipSign()};
            }
        }
        case SimplifyRet::LINEAR: {
            if (op == UnOp::NEG) {
                ret.lin().m *= -1;
                ret.lin().n *= -1;
                return ret;
            }
        }
        default: {
            ret.update(arg);
            return {*this, false};
        }
    }
}
Term::ProjectRet UnOpTerm::project(bool rename, AuxGen &auxGen) {
    assert(!rename); (void)rename;
    UTerm y(auxGen.uniqueVar(loc(), 0, "#X"));
    UTerm x(wrap(UTerm(y->clone())));
    return std::make_tuple(wrap(make_locatable<UnOpTerm>(loc(), op, std::move(arg))), std::move(x), std::move(y));
}
bool UnOpTerm::hasVar() const {
    return arg->hasVar();
}
bool UnOpTerm::hasPool() const   { return arg->hasPool(); }
void UnOpTerm::collect(VarTermBoundVec &vars, bool bound) const {
    arg->collect(vars, bound && op == UnOp::NEG);
}
void UnOpTerm::collect(VarSet &vars, unsigned minLevel , unsigned maxLevel) const {
    arg->collect(vars, minLevel, maxLevel);
}
Value UnOpTerm::eval(bool &undefined) const {
	Value value = arg->eval(undefined);
	if (value.type() == Value::NUM) {
		int num = value.num();
		switch (op) {
            case UnOp::NEG: { return Value::createNum(-num); }
			case UnOp::ABS: { return Value::createNum(std::abs(num)); }
			case UnOp::NOT: { return Value::createNum(~num); }
		}
		assert(false);
		return Value::createNum(0);
	}
    else if (op == UnOp::NEG && (value.type() == Value::ID || value.type() == Value::FUNC)) {
        return value.flipSign();
    }
	else {
        undefined = true;
		GRINGO_REPORT(W_OPERATION_UNDEFINED)
			<< loc() << ": info: operation undefined:\n"
			<< "  " << *this << "\n";
		return Value::createNum(0);
	}
}
bool UnOpTerm::match(Value const &x) const  {
    if (op != UnOp::NEG) {
        throw std::logic_error("Term::rewriteArithmetics must be called before Term::match");
    }
	if (x.type() == Value::NUM) {
		return arg->match(Value::createNum(-x.num()));
	}
    else if (x.type() == Value::ID || x.type() == Value::FUNC) {
        return arg->match(x.flipSign());
    }
	return false;
}
void UnOpTerm::unpool(UTermVec &x) const {
    auto f = [&](UTerm &&y) { x.emplace_back(make_locatable<UnOpTerm>(loc(), op, std::move(y))); };
    Term::unpool(arg, Gringo::unpool, f);
}
UTerm UnOpTerm::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen, bool forceDefined) {
    if (!forceDefined && op == UnOp::NEG) {
        Term::replace(arg, arg->rewriteArithmetics(arith, auxGen, false));
        return nullptr;
    }
    else {
        return Term::insert(arith, auxGen, make_locatable<UnOpTerm>(loc(), op, std::move(arg)), forceDefined && op == UnOp::NEG);
    }
}
bool UnOpTerm::operator==(Term const &x) const {
    auto t = dynamic_cast<UnOpTerm const*>(&x);
    return t && op == t->op && is_value_equal_to(arg, t->arg);
}
size_t UnOpTerm::hash() const {
    return get_value_hash(typeid(UnOpTerm).hash_code(), size_t(op), arg);
}
UnOpTerm *UnOpTerm::clone() const {
    return make_locatable<UnOpTerm>(loc(), op, get_clone(arg)).release();
}
FWSignature UnOpTerm::getSig() const {
    if (op == UnOp::NEG) {
        Signature sig = *arg->getSig();
        return sig.flipSign();
    }
    throw std::logic_error("Term::getSig must not be called on UnOpTerm");
}
UTerm UnOpTerm::renameVars(RenameMap &names) const { return make_locatable<UnOpTerm>(loc(), op, arg->renameVars(names)); }
UGTerm UnOpTerm::gterm(RenameMap &names, ReferenceMap &refs) const {
    if (op == UnOp::NEG) {
        UGFunTerm fun(arg->gfunterm(names, refs));
        if (fun) {
            fun->sign = not fun->sign;
            return std::move(fun);
        }
    }
    return gringo_make_unique<GVarTerm>(_newRef(names, refs));
}

UGFunTerm UnOpTerm::gfunterm(RenameMap &names, ReferenceMap &refs) const {
    if (op != UnOp::NEG) { return nullptr; }
    UGFunTerm fun(arg->gfunterm(names, refs));
    if (!fun) { return nullptr; }
    fun->sign = not fun->sign;
    return fun;
}

void UnOpTerm::collectIds(VarSet &x) const {
    arg->collectIds(x);
}
UTerm UnOpTerm::replace(Defines &x, bool) {
    Term::replace(arg, arg->replace(x, true));
    return nullptr;
}
double UnOpTerm::estimate(double, VarSet const &) const {
    return 0;
}
Value UnOpTerm::isEDB() const { return {}; }
UnOpTerm::~UnOpTerm() { }

// {{{1 definition of BinOpTerm

BinOpTerm::BinOpTerm(BinOp op, UTerm &&left, UTerm &&right)
    : op(op)
    , left(std::move(left))
    , right(std::move(right)) { }

unsigned BinOpTerm::projectScore() const {
    return left->projectScore() + right->projectScore();
}

void BinOpTerm::rename(FWString) {
    throw std::logic_error("must not be called");
}

unsigned BinOpTerm::getLevel() const {
    return std::max(left->getLevel(), right->getLevel());
}

bool BinOpTerm::isNotNumeric() const    { return false; }
bool BinOpTerm::isNotFunction() const    { return true; }

Term::Invertibility BinOpTerm::getInvertibility() const    { return Term::NOT_INVERTIBLE; }

void BinOpTerm::print(std::ostream &out) const {
    out << "(" << *left << op << *right << ")";
}

Term::SimplifyRet BinOpTerm::simplify(SimplifyState &state, bool, bool) {
    auto retLeft(left->simplify(state, false, true));
    auto retRight(right->simplify(state, false, true));
    if (retLeft.undefined() || retRight.undefined()) {
        return {};
    }
    else if (retLeft.notNumeric() || retRight.notNumeric() || (op == BinOp::DIV && retRight.isZero())) {
        GRINGO_REPORT(W_OPERATION_UNDEFINED)
            << loc() << ": info: operation undefined:\n"
            << "  " << *this << "\n";
        return {};
    }
    else if (op == BinOp::MUL && (retLeft.isZero() || retRight.isZero())) {
        // NOTE: keep binary operation untouched
    }
    else if (retLeft.type == SimplifyRet::CONSTANT && retRight.type == SimplifyRet::CONSTANT) {
        return {Value::createNum(Gringo::eval(op, retLeft.val.num(), retRight.val.num()))};
    }
    else if (retLeft.type == SimplifyRet::CONSTANT && retRight.type == SimplifyRet::LINEAR) {
        if (op == BinOp::ADD) {
            retRight.lin().n += retLeft.val.num();
            return retRight;
        }
        else if (op == BinOp::SUB) {
            retRight.lin().n = retLeft.val.num() - retRight.lin().n;
            retRight.lin().m = -retRight.lin().m;
            return retRight;
        }
        else if (op == BinOp::MUL) {
            retRight.lin().n *= retLeft.val.num();
            retRight.lin().m *= retLeft.val.num();
            return retRight;
        }
    }
    else if (retLeft.type == SimplifyRet::LINEAR && retRight.type == SimplifyRet::CONSTANT) {
        if (op == BinOp::ADD) {
            retLeft.lin().n += retRight.val.num();
            return retLeft;
        }
        else if (op == BinOp::SUB) {
            retLeft.lin().n-= retRight.val.num();
            return retLeft;
        }
        else if (op == BinOp::MUL) {
            retLeft.lin().n *= retRight.val.num();
            retLeft.lin().m *= retRight.val.num();
            return retLeft;
        }
    }
    retLeft.update(left);
    retRight.update(right);
    return {*this, false};
}

Term::ProjectRet BinOpTerm::project(bool rename, AuxGen &auxGen) {
    assert(!rename); (void)rename;
    UTerm y(auxGen.uniqueVar(loc(), 0, "#X"));
    UTerm x(wrap(UTerm(y->clone())));
    return std::make_tuple(wrap(make_locatable<BinOpTerm>(loc(), op, std::move(left), std::move(right))), std::move(x), std::move(y));
}

bool BinOpTerm::hasVar() const {
    return left->hasVar() || right->hasVar();
}

bool BinOpTerm::hasPool() const  { return left->hasPool() || right->hasPool(); }

void BinOpTerm::collect(VarTermBoundVec &vars, bool) const {
    left->collect(vars, false);
    right->collect(vars, false);
}

void BinOpTerm::collect(VarSet &vars, unsigned minLevel , unsigned maxLevel) const {
    left->collect(vars, minLevel, maxLevel);
    right->collect(vars, minLevel, maxLevel);
}

Value BinOpTerm::eval(bool &undefined) const {
	Value l(left->eval(undefined));
	Value r(right->eval(undefined));
	if (l.type() == Value::NUM && r.type() == Value::NUM && (op != BinOp::DIV || r.num() != 0)) { return Value::createNum(Gringo::eval(op, l.num(), r.num())); }
	else {
        undefined = true;
		GRINGO_REPORT(W_OPERATION_UNDEFINED)
			<< loc() << ": info: operation undefined:\n"
			<< "  " << *this << "\n";
		return Value::createNum(0);
	}
}

bool BinOpTerm::match(Value const &) const { throw std::logic_error("Term::rewriteArithmetics must be called before Term::match"); }

void BinOpTerm::unpool(UTermVec &x) const {
    auto f = [&](UTerm &&l, UTerm &&r) { x.emplace_back(make_locatable<BinOpTerm>(loc(), op, std::move(l), std::move(r))); };
    Term::unpool(left, right, Gringo::unpool, Gringo::unpool, f);
}

UTerm BinOpTerm::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen, bool) {
    return Term::insert(arith, auxGen, make_locatable<BinOpTerm>(loc(), op, std::move(left), std::move(right)));
}

bool BinOpTerm::operator==(Term const &x) const {
    auto t = dynamic_cast<BinOpTerm const*>(&x);
    return t && op == t->op && is_value_equal_to(left, t->left) && is_value_equal_to(right, t->right);
}

size_t BinOpTerm::hash() const {
    return get_value_hash(typeid(BinOpTerm).hash_code(), size_t(op), left, right);
}

BinOpTerm *BinOpTerm::clone() const {
    return make_locatable<BinOpTerm>(loc(), op, get_clone(left), get_clone(right)).release();
}

FWSignature BinOpTerm::getSig() const    { throw std::logic_error("Term::getSig must not be called on DotsTerm"); }

UTerm BinOpTerm::renameVars(RenameMap &names) const {
    UTerm term(left->renameVars(names));
    return make_locatable<BinOpTerm>(loc(), op, std::move(term), right->renameVars(names));
}

UGTerm BinOpTerm::gterm(RenameMap &names, ReferenceMap &refs) const  { return gringo_make_unique<GVarTerm>(_newRef(names, refs)); }

void BinOpTerm::collectIds(VarSet &x) const {
    left->collectIds(x);
    right->collectIds(x);
}

UTerm BinOpTerm::replace(Defines &x, bool) {
    Term::replace(left, left->replace(x, true));
    Term::replace(right, right->replace(x, true));
    return nullptr;
}

double BinOpTerm::estimate(double, VarSet const &) const {
    return 0;
}

Value BinOpTerm::isEDB() const { return {}; }

BinOpTerm::~BinOpTerm() { }

// {{{1 definition of DotsTerm

DotsTerm::DotsTerm(UTerm &&left, UTerm &&right)
    : left(std::move(left))
    , right(std::move(right)) { }

unsigned DotsTerm::projectScore() const {
    return 2;
}

void DotsTerm::rename(FWString) {
    throw std::logic_error("must not be called");
}
unsigned DotsTerm::getLevel() const {
    return std::max(left->getLevel(), right->getLevel());
}

bool DotsTerm::isNotNumeric() const     { return false; }
bool DotsTerm::isNotFunction() const     { return true; }

Term::Invertibility DotsTerm::getInvertibility() const     { return Term::NOT_INVERTIBLE; }

void DotsTerm::print(std::ostream &out) const {
    out << "(" << *left << ".." << *right << ")";
}

Term::SimplifyRet DotsTerm::simplify(SimplifyState &state, bool, bool) {
    if (!left->simplify(state, false, false).update(left).undefined() && !right->simplify(state, false, false).update(right).undefined()) {
        return { state.createDots(loc(), std::move(left), std::move(right)) };
    }
    else {
        return {};
    }
}

Term::ProjectRet DotsTerm::project(bool, AuxGen &) {
    throw std::logic_error("Term::project must be called after Term::simplify");
}

bool DotsTerm::hasVar() const {
    return left->hasVar() || right->hasVar();
}

bool DotsTerm::hasPool() const   { return left->hasPool() || right->hasPool(); }

void DotsTerm::collect(VarTermBoundVec &vars, bool) const {
    left->collect(vars, false);
    right->collect(vars, false);
}

void DotsTerm::collect(VarSet &vars, unsigned minLevel , unsigned maxLevel) const {
    left->collect(vars, minLevel, maxLevel);
    right->collect(vars, minLevel, maxLevel);
}

Value DotsTerm::eval(bool &) const { throw std::logic_error("Term::rewriteDots must be called before Term::eval"); }

bool DotsTerm::match(Value const &) const  { throw std::logic_error("Term::rewriteDots must be called before Term::match"); }

void DotsTerm::unpool(UTermVec &x) const {
    auto f = [&](UTerm &&l, UTerm &&r) { x.emplace_back(make_locatable<DotsTerm>(loc(), std::move(l), std::move(r))); };
    Term::unpool(left, right, Gringo::unpool, Gringo::unpool, f);
}

UTerm DotsTerm::rewriteArithmetics(Term::ArithmeticsMap &, AuxGen &, bool) {
    throw std::logic_error("Term::rewriteDots must be called before Term::rewriteArithmetics");
}

bool DotsTerm::operator==(Term const &) const {
    // Note: each DotsTerm is associated to a unique variable
    return false;
}

size_t DotsTerm::hash() const {
    return get_value_hash(typeid(DotsTerm).hash_code(), left, right);
}

DotsTerm *DotsTerm::clone() const {
    return make_locatable<DotsTerm>(loc(), get_clone(left), get_clone(right)).release();
}

FWSignature DotsTerm::getSig() const     { throw std::logic_error("Term::getSig must not be called on LuaTerm"); }

UTerm DotsTerm::renameVars(RenameMap &names) const {
    UTerm term(left->renameVars(names));
    return make_locatable<DotsTerm>(loc(), std::move(term), right->renameVars(names));
}

UGTerm DotsTerm::gterm(RenameMap &names, ReferenceMap &refs) const   { return gringo_make_unique<GVarTerm>(_newRef(names, refs)); }

void DotsTerm::collectIds(VarSet &x) const {
    left->collectIds(x);
    right->collectIds(x);
}

UTerm DotsTerm::replace(Defines &x, bool) {
    Term::replace(left, left->replace(x, true));
    Term::replace(right, right->replace(x, true));
    return nullptr;
}

double DotsTerm::estimate(double, VarSet const &) const {
    return 0;
}

Value DotsTerm::isEDB() const { return {}; }

DotsTerm::~DotsTerm() { }

// {{{1 definition of LuaTerm

LuaTerm::LuaTerm(FWString name, UTermVec &&args)
    : name(name)
    , args(std::move(args)) { }

unsigned LuaTerm::projectScore() const {
    return 2;
}

void LuaTerm::rename(FWString) {
    throw std::logic_error("must not be called");
}

unsigned LuaTerm::getLevel() const {
    unsigned level = 0;
    for (auto &x : args) { level = std::max(x->getLevel(), level); }
    return level;
}

bool LuaTerm::isNotNumeric() const { return false; }
bool LuaTerm::isNotFunction() const { return false; }

Term::Invertibility LuaTerm::getInvertibility() const      { return Term::NOT_INVERTIBLE; }

void LuaTerm::print(std::ostream &out) const {
    out << "@" << *name << "(";
    print_comma(out, args, ",", [](std::ostream &out, UTerm const &y) { out << *y; });
    out << ")";
}

Term::SimplifyRet LuaTerm::simplify(SimplifyState &state, bool, bool) {
    for (auto &arg : args) {
        if (arg->simplify(state, false, false).update(arg).undefined()) {
            return {};
        }
    }
    return { state.createScript(loc(), std::move(name), std::move(args)) };
}

Term::ProjectRet LuaTerm::project(bool rename, AuxGen &auxGen) {
    assert(!rename); (void)rename;
    UTerm y(auxGen.uniqueVar(loc(), 0, "#X"));
    UTerm x(wrap(UTerm(y->clone())));
    return std::make_tuple(make_locatable<LuaTerm>(loc(), name, std::move(args)), std::move(x), std::move(y));
}

bool LuaTerm::hasVar() const {
    for (auto &x : args) {
        if (x->hasVar()) { return true; }
    }
    return false;
}

bool LuaTerm::hasPool() const {
    for (auto &x : args) { if (x->hasPool()) { return true; } }
    return false;
}

void LuaTerm::collect(VarTermBoundVec &vars, bool) const {
    for (auto &y : args) { y->collect(vars, false); }
}

void LuaTerm::collect(VarSet &vars, unsigned minLevel , unsigned maxLevel) const {
    for (auto &y : args) { y->collect(vars, minLevel, maxLevel); }
}

Value LuaTerm::eval(bool &) const { throw std::logic_error("Term::simplify must be called before Term::eval"); }

bool LuaTerm::match(Value const &) const   { throw std::logic_error("Term::rewriteArithmetics must be called before Term::match"); }

void LuaTerm::unpool(UTermVec &x) const {
    auto f = [&](UTermVec &&args) { x.emplace_back(make_locatable<LuaTerm>(loc(), name, std::move(args))); };
    Term::unpool(args.begin(), args.end(), Gringo::unpool, f);
}

UTerm LuaTerm::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen, bool) {
    return Term::insert(arith, auxGen, make_locatable<LuaTerm>(loc(), name, std::move(args)));
}

bool LuaTerm::operator==(Term const &x) const {
    auto t = dynamic_cast<LuaTerm const*>(&x);
    return t && *name == *t->name && is_value_equal_to(args, t->args);
}

size_t LuaTerm::hash() const {
    return get_value_hash(typeid(LuaTerm).hash_code(), *name, args);
}

LuaTerm *LuaTerm::clone() const {
    return make_locatable<LuaTerm>(loc(), name, get_clone(args)).release();
}

FWSignature LuaTerm::getSig() const      { return FWSignature(name, args.size()); }

UTerm LuaTerm::renameVars(RenameMap &names) const {
    UTermVec args;
    for (auto &x : this->args) { args.emplace_back(x->renameVars(names)); }
    return make_locatable<LuaTerm>(loc(), name, std::move(args));
}

UGTerm LuaTerm::gterm(RenameMap &names, ReferenceMap &refs) const    { return gringo_make_unique<GVarTerm>(_newRef(names, refs)); }

void LuaTerm::collectIds(VarSet &x) const {
    for (auto &y : args) { y->collectIds(x); }
}

UTerm LuaTerm::replace(Defines &x, bool) {
    for (auto &y : args) { Term::replace(y, y->replace(x, true)); }
    return nullptr;
}

double LuaTerm::estimate(double, VarSet const &) const {
    return 0;
}

Value LuaTerm::isEDB() const { return {}; }

LuaTerm::~LuaTerm() { }

// {{{1 definition of FunctionTerm

FunctionTerm::FunctionTerm(FWString name, UTermVec &&args)
    : name(name)
    , args(std::move(args)) { }

unsigned FunctionTerm::projectScore() const {
    unsigned ret = 0;
    for (auto &x : args) { ret += x->projectScore(); }
    return ret;
}

void FunctionTerm::rename(FWString x) {
    name = x;
}

unsigned FunctionTerm::getLevel() const {
    unsigned level = 0;
    for (auto &x : args) { level = std::max(x->getLevel(), level); }
    return level;
}

bool FunctionTerm::isNotNumeric() const { return true; }
bool FunctionTerm::isNotFunction() const { return false; }

Term::Invertibility FunctionTerm::getInvertibility() const { return Term::INVERTIBLE; }

void FunctionTerm::print(std::ostream &out) const {
    out << *(name) << "(";
    print_comma(out, args, ",", [](std::ostream &out, UTerm const &y) { out << *y; });
    if (*name == "" && args.size() == 1) {
        out << ",";
    }
    out << ")";
}

Term::SimplifyRet FunctionTerm::simplify(SimplifyState &state, bool positional, bool) {
    bool constant  = true;
    bool projected = false;
    for (auto &arg : args) {
        auto ret(arg->simplify(state, positional, false));
        if (ret.undefined()) {
            return {};
        }
        constant  = constant  && ret.constant();
        projected = projected || ret.project;
        ret.update(arg);
    }
    if (constant) {
        bool undefined;
        return {eval(undefined)};
    }
    else          { return {*this, projected}; }
}

Term::ProjectRet FunctionTerm::project(bool rename, AuxGen &auxGen) {
    UTermVec argsProjected;
    UTermVec argsProject;
    for (auto &arg : args) {
        auto ret(arg->project(false, auxGen));
        Term::replace(arg, std::move(std::get<0>(ret)));
        argsProjected.emplace_back(std::move(std::get<1>(ret)));
        argsProject.emplace_back(std::move(std::get<2>(ret)));
    }
    FWString oldName = name;
    if (rename) { name = FWString("#p_" + *name); }
    return std::make_tuple(
            nullptr,
            make_locatable<FunctionTerm>(loc(), name, std::move(argsProjected)),
            make_locatable<FunctionTerm>(loc(), oldName, std::move(argsProject)));
}

bool FunctionTerm::hasVar() const {
    for (auto &x : args) {
        if (x->hasVar()) { return true; }
    }
    return false;
}

bool FunctionTerm::hasPool() const {
    for (auto &x : args) { if (x->hasPool()) { return true; } }
    return false;
}

void FunctionTerm::collect(VarTermBoundVec &vars, bool bound) const {
    for (auto &y : args) { y->collect(vars, bound); }
}

void FunctionTerm::collect(VarSet &vars, unsigned minLevel , unsigned maxLevel) const {
    for (auto &y : args) { y->collect(vars, minLevel, maxLevel); }
}

Value FunctionTerm::eval(bool &undefined) const {
	cache.clear();
	for (auto &term : args) { cache.emplace_back(term->eval(undefined)); }
	return Value::createFun(name, cache);
}

bool FunctionTerm::match(Value const &x) const {
	if (x.type() == Value::FUNC && !(*x.sig()).sign()) {
		Signature s(*x.sig());
		if (s.name() == name && s.length() == args.size()) {
			auto i = 0;
			for (auto &term : args) {
				if (!term->match(x.args()[i++])) { return false; }
			}
			return true;
		}
	}
	return false;
}

void FunctionTerm::unpool(UTermVec &x) const {
    auto f = [&](UTermVec &&args) { x.emplace_back(make_locatable<FunctionTerm>(loc(), name, std::move(args))); };
    Term::unpool(args.begin(), args.end(), Gringo::unpool, f);
}

UTerm FunctionTerm::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen, bool forceDefined) {
    for (auto &arg : args) { Term::replace(arg, arg->rewriteArithmetics(arith, auxGen, forceDefined)); }
    return nullptr;
}

bool FunctionTerm::operator==(Term const &x) const {
    auto t = dynamic_cast<FunctionTerm const*>(&x);
    return t && *name == *t->name && is_value_equal_to(args, t->args);
}

size_t FunctionTerm::hash() const {
    return get_value_hash(typeid(FunctionTerm).hash_code(), *name, args);
}

FunctionTerm *FunctionTerm::clone() const {
    return make_locatable<FunctionTerm>(loc(), name, get_clone(args)).release();
}

FWSignature FunctionTerm::getSig() const { return FWSignature(name, args.size()); }

UTerm FunctionTerm::renameVars(RenameMap &names) const {
    UTermVec args;
    for (auto &x : this->args) { args.emplace_back(x->renameVars(names)); }
    return make_locatable<FunctionTerm>(loc(), name, std::move(args));
}

UGTerm FunctionTerm::gterm(RenameMap &names, ReferenceMap &refs) const {
    return gfunterm(names, refs);
}

UGFunTerm FunctionTerm::gfunterm(RenameMap &names, ReferenceMap &refs) const {
    UGTermVec args;
    for (auto &x : this->args) { args.emplace_back(x->gterm(names, refs)); }
    return gringo_make_unique<GFunctionTerm>(name, std::move(args));
}

void FunctionTerm::collectIds(VarSet &x) const {
    for (auto &y : args) { y->collectIds(x); }
}

UTerm FunctionTerm::replace(Defines &x, bool) {
    for (auto &y : args) { Term::replace(y, y->replace(x, true)); }
    return nullptr;
}

double FunctionTerm::estimate(double size, VarSet const &bound) const {
    double ret = 0.0;
    if (!args.empty()) {
        double root = std::max(1.0, std::pow(((*name).empty() ? size : size/2.0), 1.0/args.size()));
        for (auto &x : args) { ret += x->estimate(root, bound); }
        ret /= args.size();
    }
    return ret;
}

Value FunctionTerm::isEDB() const {
    cache.clear();
    for (auto &x : args) {
        cache.emplace_back(x->isEDB());
        if (cache.back().type() == Value::SPECIAL) { return {}; }
    }
    return Value::createFun(name, cache);
}


FunctionTerm::~FunctionTerm() { }

// }}}1

} // namespace Gringo
