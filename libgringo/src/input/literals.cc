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

#include "gringo/input/literals.hh"
#include "gringo/ground/literals.hh"
#include "gringo/output/output.hh"
#include "gringo/utility.hh"

namespace Gringo { namespace Input {

// {{{1 definition of Literal::print

inline void RangeLiteral::print(std::ostream &out) const     { out << "#range(" << *assign << "," << *lower << "," << *upper << ")"; }
inline void VoidLiteral::print(std::ostream &out) const     { out << "#void"; }
inline void ScriptLiteral::print(std::ostream &out) const    {
    out << "#script(" << *assign << "," << name << "(";
    print_comma(out, args, ",", [](std::ostream &out, UTerm const &term) { out << *term; });
    out << ")";
}

// {{{1 definition of Literal::clone

RangeLiteral *RangeLiteral::clone() const {
    return make_locatable<RangeLiteral>(loc(), get_clone(assign), get_clone(lower), get_clone(upper)).release();
}
VoidLiteral *VoidLiteral::clone() const {
    return make_locatable<VoidLiteral>(loc()).release();
}
ScriptLiteral *ScriptLiteral::clone() const {
    return make_locatable<ScriptLiteral>(loc(), get_clone(assign), name, get_clone(args)).release();
}

// {{{1 definition of Literal::simplify

bool RangeLiteral::simplify(Logger &, Projections &, SimplifyState &, bool, bool) {
    throw std::logic_error("RangeLiteral::simplify should never be called  if used properly");
}
bool VoidLiteral::simplify(Logger &, Projections &, SimplifyState &, bool, bool) { return true; }
bool ScriptLiteral::simplify(Logger &, Projections &, SimplifyState &, bool, bool) {
    throw std::logic_error("ScriptLiteral::simplify should never be called  if used properly");
}

// {{{1 definition of Literal::collect

void RangeLiteral::collect(VarTermBoundVec &vars, bool bound) const {
    assign->collect(vars, bound);
    lower->collect(vars, false);
    upper->collect(vars, false);
}
void VoidLiteral::collect(VarTermBoundVec &, bool) const { }
void ScriptLiteral::collect(VarTermBoundVec &vars, bool bound) const {
    assign->collect(vars, bound);
    for (auto &x : args) { x->collect(vars, false); }
}

// {{{1 definition of Literal::operator==

bool RangeLiteral::operator==(Literal const &x) const {
    auto t = dynamic_cast<RangeLiteral const *>(&x);
    return t && is_value_equal_to(assign, t->assign) && is_value_equal_to(lower, t->lower) && is_value_equal_to(upper, t->upper);
}
bool VoidLiteral::operator==(Literal const &x) const {
    return dynamic_cast<VoidLiteral const *>(&x) != nullptr;
}
bool ScriptLiteral::operator==(Literal const &x) const {
    auto t = dynamic_cast<ScriptLiteral const *>(&x);
    return t && is_value_equal_to(assign, t->assign) && name == t->name && is_value_equal_to(args, t->args);
}

// {{{1 definition of Literal::rewriteArithmetics

void RangeLiteral::rewriteArithmetics(Term::ArithmeticsMap &arith, RelationVec &, AuxGen &auxGen) {
    Term::replace(this->assign, this->assign->rewriteArithmetics(arith, auxGen));
}
void VoidLiteral::rewriteArithmetics(Term::ArithmeticsMap &, RelationVec &, AuxGen &) { }
void ScriptLiteral::rewriteArithmetics(Term::ArithmeticsMap &arith, RelationVec &, AuxGen &auxGen) {
    Term::replace(this->assign, this->assign->rewriteArithmetics(arith, auxGen));
}

// {{{1 definition of Literal::hash

size_t RangeLiteral::hash() const {
    return get_value_hash(typeid(RangeLiteral).hash_code(), assign, lower, upper);
}
size_t VoidLiteral::hash() const {
    return get_value_hash(typeid(VoidLiteral).hash_code());
}
size_t ScriptLiteral::hash() const {
    return get_value_hash(typeid(RangeLiteral).hash_code(), assign, name, args);
}

// {{{1 definition of Literal::unpool

ULitVec RangeLiteral::unpool(bool, bool) const {
    ULitVec value;
    value.emplace_back(ULit(clone()));
    return value;
}
ULitVec VoidLiteral::unpool(bool, bool) const {
    ULitVec value;
    value.emplace_back(ULit(clone()));
    return value;
}
ULitVec ScriptLiteral::unpool(bool, bool) const {
    ULitVec value;
    value.emplace_back(ULit(clone()));
    return value;
}

// {{{1 definition of Literal::toTuple

void RangeLiteral::toTuple(UTermVec &, int &) const {
    throw std::logic_error("RangeLiteral::toTuple should never be called if used properly");
}
void VoidLiteral::toTuple(UTermVec &tuple, int &id) const {
    tuple.emplace_back(make_locatable<ValTerm>(loc(), Symbol::createNum(id+3)));
    ++id;
}
void ScriptLiteral::toTuple(UTermVec &, int &) const {
    throw std::logic_error("ScriptLiteral::toTuple should never be called if used properly");
}

// {{{1 definition of Literal::isEDB

Symbol Literal::isEDB() const          { return {}; }

// {{{1 definition of Literal::hasPool

bool RangeLiteral::hasPool(bool, bool) const             { return false; }
bool VoidLiteral::hasPool(bool, bool) const             { return false; }
bool ScriptLiteral::hasPool(bool, bool) const            { return false; }

// {{{1 definition of Literal::replace

void RangeLiteral::replace(Defines &x) {
    Term::replace(assign, assign->replace(x, true));
    Term::replace(lower, lower->replace(x, true));
    Term::replace(upper, upper->replace(x, true));
}
void VoidLiteral::replace(Defines &) { }
void ScriptLiteral::replace(Defines &x) {
    Term::replace(assign, assign->replace(x, true));
    for (auto &y : args) { Term::replace(y, y->replace(x, true)); }
}

// {{{1 definition of Literal::toGround

Ground::ULit RangeLiteral::toGround(DomainData &, bool) const {
    return gringo_make_unique<Ground::RangeLiteral>(get_clone(assign), get_clone(lower), get_clone(upper));
}
Ground::ULit VoidLiteral::toGround(DomainData &, bool) const {
    throw std::logic_error("VoidLiteral::toGround: must not happen");
}
Ground::ULit ScriptLiteral::toGround(DomainData &, bool) const {
    return gringo_make_unique<Ground::ScriptLiteral>(get_clone(assign), name, get_clone(args));
}

// {{{1 definition of Literal::shift

ULit RangeLiteral::shift(bool)  { throw std::logic_error("RangeLiteral::shift should never be called  if used properly"); }
ULit VoidLiteral::shift(bool)  { return nullptr; }
ULit ScriptLiteral::shift(bool) { throw std::logic_error("ScriptLiteral::shift should never be called  if used properly"); }

// {{{1 definition of Literal::headRepr

UTerm RangeLiteral::headRepr() const {
    throw std::logic_error("RangeLiteral::toTuple should never be called if used properly");
}
UTerm VoidLiteral::headRepr() const {
    return nullptr;
}
UTerm ScriptLiteral::headRepr() const {
    throw std::logic_error("ScriptLiteral::toTuple should never be called if used properly");
}

// }}}1

// {{{1 definition of PredicateLiteral

PredicateLiteral::PredicateLiteral(NAF naf, UTerm &&repr, bool auxiliary)
: naf(naf)
, auxiliary_(auxiliary)
, repr(std::move(repr)) {
    if (!this->repr->isAtom()) {
        throw std::runtime_error("atom expected");
    }
}

PredicateLiteral::~PredicateLiteral() { }

unsigned PredicateLiteral::projectScore() const {
    return 1 + repr->projectScore();
}

inline void PredicateLiteral::print(std::ostream &out) const {
    if (auxiliary()) { out << "["; }
    out << naf << *repr;
    if (auxiliary()) { out << "]"; }
}

PredicateLiteral *PredicateLiteral::clone() const {
    return make_locatable<PredicateLiteral>(loc(), naf, get_clone(repr)).release();
}

bool PredicateLiteral::simplify(Logger &log, Projections &project, SimplifyState &state, bool positional, bool singleton) {
    if (singleton && positional && naf == NAF::POS) {
        positional = false;
    }
    auto ret(repr->simplify(state, positional, false, log));
    ret.update(repr, false);
    if (ret.undefined()) { return false; }
    if (repr->simplify(state, positional, false, log).update(repr, false).project()) {
        auto rep(project.add(*repr));
        Term::replace(repr, std::move(rep));
    }
    return true;
}

void PredicateLiteral::collect(VarTermBoundVec &vars, bool bound) const {
    repr->collect(vars, bound && naf == NAF::POS);
}

inline bool PredicateLiteral::operator==(Literal const &x) const {
    auto t = dynamic_cast<PredicateLiteral const *>(&x);
    return t && naf == t->naf && is_value_equal_to(repr, t->repr) && (auxiliary_ == t->auxiliary_);
}

void PredicateLiteral::rewriteArithmetics(Term::ArithmeticsMap &arith, RelationVec &, AuxGen &auxGen) {
    if (naf == NAF::POS) { Term::replace(repr, repr->rewriteArithmetics(arith, auxGen)); }
}

inline size_t PredicateLiteral::hash() const {
    return get_value_hash(typeid(PredicateLiteral).hash_code(), size_t(naf), repr);
}

ULitVec PredicateLiteral::unpool(bool, bool) const {
    ULitVec value;
    auto f = [&](UTerm &&y){ value.emplace_back(make_locatable<PredicateLiteral>(loc(), naf, std::move(y))); };
    Term::unpool(repr, Gringo::unpool, f);
    return  value;
}

void PredicateLiteral::toTuple(UTermVec &tuple, int &) const {
    int id = 0;
    switch (naf) {
        case NAF::POS:    { id = 0; break; }
        case NAF::NOT:    { id = 1; break; }
        case NAF::NOTNOT: { id = 2; break; }
    }
    tuple.emplace_back(make_locatable<ValTerm>(loc(), Symbol::createNum(id)));
    tuple.emplace_back(get_clone(repr));
}

Symbol PredicateLiteral::isEDB() const { return naf == NAF::POS ? repr->isEDB() : Symbol(); }

inline bool PredicateLiteral::hasPool(bool, bool) const { return repr->hasPool(); }

inline void PredicateLiteral::replace(Defines &x) { Term::replace(repr, repr->replace(x, false)); }

inline Ground::ULit PredicateLiteral::toGround(DomainData &x, bool auxiliary) const {
    return gringo_make_unique<Ground::PredicateLiteral>(auxiliary_ || auxiliary, x.add(repr->getSig()), naf, get_clone(repr));
}

ULit PredicateLiteral::shift(bool negate) {
    if (naf == NAF::POS) { return nullptr; }
    else {
        NAF inv = (naf == NAF::NOT) == negate ? NAF::NOTNOT : NAF::NOT;
        return make_locatable<PredicateLiteral>(loc(), inv, std::move(repr));
    }
}

UTerm PredicateLiteral::headRepr() const {
    assert(naf == NAF::POS);
    return get_clone(repr);
}

// {{{1 definition of ProjectionLiteral

ProjectionLiteral::ProjectionLiteral(UTerm &&repr)
    : PredicateLiteral(NAF::POS, std::move(repr)), initialized_(false) { }

ProjectionLiteral::~ProjectionLiteral() { }

ProjectionLiteral *ProjectionLiteral::clone() const {
    throw std::logic_error("ProjectionLiteral::clone must not be called!!!");
}

ULitVec ProjectionLiteral::unpool(bool, bool) const {
    throw std::logic_error("ProjectionLiteral::unpool must not be called!!!");
}

inline Ground::ULit ProjectionLiteral::toGround(DomainData &x, bool auxiliary) const {
    bool initialized = initialized_;
    initialized_ = true;
    return gringo_make_unique<Ground::ProjectionLiteral>(auxiliary_ || auxiliary, x.add(repr->getSig()), get_clone(repr), initialized);
}

ULit ProjectionLiteral::shift(bool) {
    throw std::logic_error("ProjectionLiteral::shift must not be called!!!");
}

// {{{1 definition of RangeLiteral

RangeLiteral::RangeLiteral(UTerm &&assign, UTerm &&lower, UTerm &&upper)
    : assign(std::move(assign))
    , lower(std::move(lower))
    , upper(std::move(upper)) { }

ULit RangeLiteral::make(SimplifyState::DotsMap::value_type &dot) {
    Location loc(std::get<0>(dot)->loc());
    return make_locatable<RangeLiteral>(loc, std::move(std::get<0>(dot)), std::move(std::get<1>(dot)), std::move(std::get<2>(dot)));
}

RangeLiteral::~RangeLiteral() { }

// {{{1 definition of VoidLiteral

VoidLiteral::VoidLiteral() { }
VoidLiteral::~VoidLiteral() { }

// {{{1 definition of ScriptLiteral

ScriptLiteral::ScriptLiteral(UTerm &&assign, String name, UTermVec &&args)
    : assign(std::move(assign))
    , name(name)
    , args(std::move(args)) { }

ULit ScriptLiteral::make(SimplifyState::ScriptMap::value_type &script) {
    Location loc(std::get<0>(script)->loc());
    return make_locatable<ScriptLiteral>(loc, std::move(std::get<0>(script)), std::move(std::get<1>(script)), std::move(std::get<2>(script)));
}

ScriptLiteral::~ScriptLiteral() { }


// {{{ definition of RelationLiteral

RelationLiteral::RelationLiteral(Relation rel, UTerm &&left, UTerm &&right)
: left_{std::move(left)}
, naf_{NAF::POS} {
    right_.emplace_back(rel, std::move(right));
}

RelationLiteral::RelationLiteral(NAF naf, Relation rel, UTerm &&left, UTerm &&right)
: left_{std::move(left)}
, naf_{NAF::POS} {
    right_.emplace_back(naf == NAF::NOT ? neg(rel) : rel, std::move(right));
}

RelationLiteral::RelationLiteral(NAF naf, UTerm &&left, Terms &&right)
: left_(std::move(left))
, right_(std::move(right))
, naf_{naf == NAF::NOT ? NAF::NOT : NAF::POS} {
    if (naf_ == NAF::NOT && right_.size() == 1) {
        naf_ = NAF::POS;
        right_.front().first = neg(right_.front().first);
    }
}

ULit RelationLiteral::make(Term::LevelMap::value_type &x) {
    Location loc(x.first->loc());
    return make_locatable<RelationLiteral>(loc, NAF::POS, Relation::EQ, std::move(x.second), get_clone(x.first));
}

ULit RelationLiteral::make(Literal::RelationVec::value_type &x) {
    Location loc(std::get<1>(x)->loc() + std::get<2>(x)->loc());
    return make_locatable<RelationLiteral>(loc, NAF::POS, std::get<0>(x), std::move(std::get<1>(x)), get_clone(std::get<2>(x)));
}

RelationLiteral::~RelationLiteral() noexcept = default;

bool RelationLiteral::needSetShift() const {
    return true;
}

unsigned RelationLiteral::projectScore() const {
    auto score = left_->projectScore();
    for (auto &term : right_) {
        score += term.second->projectScore();
    }
    return score;
}

inline void RelationLiteral::print(std::ostream &out) const {
    assert(!right_.empty());
    out << naf_ << *left_;
    for (auto &term: right_) {
        out << term.first << *term.second;
    }
}

RelationLiteral *RelationLiteral::clone() const {
    return make_locatable<RelationLiteral>(loc(), naf_, get_clone(left_), get_clone(right_)).release();
}

bool RelationLiteral::simplify(Logger &log, Projections &project, SimplifyState &state, bool positional, bool singleton) {
    static_cast<void>(project);
    static_cast<void>(positional);
    static_cast<void>(singleton);
    auto handle_fail = [&]() {
        if (naf_ == NAF::NOT) {
            // the relation literal is trivially satisfied in this case
            naf_ = NAF::POS;
            left_ = make_locatable<ValTerm>(loc(), Symbol::createNum(0));
            right_.clear();
            right_.emplace_back(Relation::EQ, make_locatable<ValTerm>(loc(), Symbol::createNum(0)));
            return true;
        }
        return false;
    };
    if (left_->simplify(state, false, false, log).update(left_, false).undefined()) {
        return handle_fail();
    }
    for (auto &term : right_) {
        if (term.second->simplify(state, false, false, log).update(term.second, false).undefined()) {
            return handle_fail();
        }
    }
    return true;
}

void RelationLiteral::collect(VarTermBoundVec &vars, bool bound) const {
    left_->collect(vars, bound && naf_ == NAF::POS && right_.front().first == Relation::EQ);
    for (auto &term : right_) {
        term.second->collect(vars, false);
    }
}

bool RelationLiteral::operator==(Literal const &x) const {
    auto t = dynamic_cast<RelationLiteral const *>(&x);
    return t != nullptr && naf_ == t->naf_ && is_value_equal_to(left_, t->left_) && is_value_equal_to(right_, t->right_);
}

void RelationLiteral::rewriteArithmetics(Term::ArithmeticsMap &arith, RelationVec &assign, AuxGen &auxGen) {
    assert(naf_ == NAF::POS);
    UTerm *prev = &left_;
    for (auto &term : right_) {
        if (term.first == Relation::EQ) {
            if (term.second->hasVar()) {
                assign.emplace_back(Relation::EQ, get_clone(term.second), get_clone(*prev));
                Term::replace(std::get<1>(assign.back()), std::get<1>(assign.back())->rewriteArithmetics(arith, auxGen));
            }
            Term::replace(*prev, prev->get()->rewriteArithmetics(arith, auxGen));
        }
        prev = &term.second;
    }
    while (right_.size() > 1) {
        assign.emplace_back(right_.back().first, get_clone((right_.end() - 2)->second), std::move(right_.back().second));
        right_.pop_back();
    }
}

size_t RelationLiteral::hash() const {
    return get_value_hash(typeid(RelationLiteral).hash_code(), naf_, left_, right_);
}

ULitVec RelationLiteral::unpool(bool beforeRewrite, bool head) const {
    static_cast<void>(beforeRewrite);
    // unpool a term with a relation
    auto unpoolTerm = [&](Terms::value_type const &term) -> Terms {
        Terms ret;
        for (auto &x : Gringo::unpool(term.second)) {
            ret.emplace_back(term.first, std::move(x));
        }
        return ret;
    };
    // unpool a vector of terms with relations
    auto unpoolTerms = [&](Terms const &terms) -> std::vector<Terms> {
        std::vector<Terms> termsPool;
        Term::unpool(terms.begin(), terms.end(), unpoolTerm, [&termsPool](Terms &&unpooled) {
            termsPool.emplace_back(std::move(unpooled));
        });
        return termsPool;
    };
    // unpool the left hand side together with the terms and relations
    ULitVec unpooled;
    auto appendRelN = [&](UTerm &&left, Terms &&right) {
        unpooled.emplace_back(make_locatable<RelationLiteral>(loc(), naf_, std::move(left), std::move(right)));
    };
    Term::unpool(left_, right_, Gringo::unpool, unpoolTerms, appendRelN);
    return unpooled;
}

ULitVecVec RelationLiteral::unpoolComparison() const {
    ULitVecVec ret;
    if (naf_ != NAF::NOT) {
        ret.emplace_back();
        UTerm prev = get_clone(left_);
        for (auto &term : right_) {
            ret.back().emplace_back(make_locatable<RelationLiteral>(loc(), NAF::POS, term.first, std::move(prev), get_clone(term.second)));
            prev = get_clone(term.second);
        }
    }
    else {
        UTerm prev = get_clone(left_);
        for (auto &term : right_) {
            ret.emplace_back();
            ret.back().emplace_back(make_locatable<RelationLiteral>(loc(), NAF::POS, neg(term.first), std::move(prev), get_clone(term.second)));
            prev = get_clone(term.second);
        }
    }
    return ret;
}

bool RelationLiteral::hasUnpoolComparison() const {
    return right_.size() > 1;
}

void RelationLiteral::toTuple(UTermVec &tuple, int &id) const {
    VarTermBoundVec vars;
    left_->collect(vars, false);
    for (auto const &term : right_) {
        term.second->collect(vars, false);
    }
    tuple.emplace_back(make_locatable<ValTerm>(loc(), Symbol::createNum(id+3)));
    for (auto const &var : vars) {
        tuple.emplace_back(std::unique_ptr<VarTerm>(var.first->clone()));
    }
    ++id;
}

inline bool RelationLiteral::hasPool(bool beforeRewrite, bool head) const  {
    static_cast<void>(beforeRewrite);
    static_cast<void>(head);
    if (left_->hasPool()) {
        return true;
    }
    for (auto &term : right_) {
        if (term.second->hasPool()) {
            return true;
        }
    }
    return  false;
}

inline void RelationLiteral::replace(Defines &x) {
    Term::replace(left_, left_->replace(x, true));
    for (auto &term : right_) {
        Term::replace(term.second, term.second->replace(x, true));
    }
}

inline Ground::ULit RelationLiteral::toGround(DomainData &data, bool auxiliary) const {
    static_cast<void>(data);
    static_cast<void>(auxiliary);
    assert(right_.size() == 1 && naf_ == NAF::POS);
    return gringo_make_unique<Ground::RelationLiteral>(right_.front().first, get_clone(left_), get_clone(right_.front().second));
}

ULit RelationLiteral::shift(bool negate) {
    if (negate) {
        if (naf_ == NAF::NOT) {
            naf_ = NAF::POS;
        }
        else if (right_.size() == 1) {
            naf_ = NAF::POS;
            right_.front().first = neg(right_.front().first);
        }
        else {
            naf_ = NAF::NOT;
        }
    }
    return make_locatable<RelationLiteral>(loc(), std::move(*this));
}

UTerm RelationLiteral::headRepr() const {
    throw std::logic_error("RelationLiteral::toTuple: a relation literal always has to be shifted");
}
// }}}


// {{{ definition of BooleanSetLiteral

BooleanSetLiteral::BooleanSetLiteral(UTerm repr, bool value)
: repr_{std::move(repr)}
, value_{value} { }

BooleanSetLiteral::BooleanSetLiteral(BooleanSetLiteral &&other) noexcept = default;
BooleanSetLiteral &BooleanSetLiteral::operator=(BooleanSetLiteral &&other) noexcept = default;
BooleanSetLiteral::~BooleanSetLiteral() noexcept = default;

unsigned BooleanSetLiteral::projectScore() const {
    return 0;
}

void BooleanSetLiteral::collect(VarTermBoundVec &vars, bool bound) const {
    static_cast<void>(bound);
    repr_->collect(vars, false);
}

void BooleanSetLiteral::toTuple(UTermVec &tuple, int &id) const {
    static_cast<void>(id);
    tuple.emplace_back(make_locatable<ValTerm>(loc(), Symbol::createNum(3)));
    tuple.emplace_back(get_clone(repr_));
}

BooleanSetLiteral *BooleanSetLiteral::clone() const {
    return make_locatable<BooleanSetLiteral>(loc(), get_clone(repr_), value_).release();
}

void BooleanSetLiteral::print(std::ostream &out) const {
    out << "#" << (value_ ? "true" : "false") << "(" << *repr_ << ")";
}

bool BooleanSetLiteral::operator==(Literal const &other) const {
    auto t = dynamic_cast<BooleanSetLiteral const *>(&other);
    return t != nullptr && value_ == t->value_ && is_value_equal_to(repr_, t->repr_);
}

size_t BooleanSetLiteral::hash() const {
    return get_value_hash(typeid(BooleanSetLiteral).hash_code(), size_t(value_), repr_);
}

bool BooleanSetLiteral::simplify(Logger &log, Projections &project, SimplifyState &state, bool positional, bool singleton) {
    static_cast<void>(positional);
    static_cast<void>(singleton);
    // Note: we can always get rid of a false boolean set literal
    return value_ && !repr_->simplify(state, false, false, log).update(repr_, false).undefined();
}

void BooleanSetLiteral::rewriteArithmetics(Term::ArithmeticsMap &arith, RelationVec &assign, AuxGen &auxGen) {
    Term::replace(repr_, repr_->rewriteArithmetics(arith, auxGen));
}

ULitVec BooleanSetLiteral::unpool(bool beforeRewrite, bool head) const {
    static_cast<void>(beforeRewrite);
    ULitVec res;
    for (auto &&repr : Gringo::unpool(get_clone(repr_))) {
        res.emplace_back(make_locatable<BooleanSetLiteral>(loc(), std::move(repr), value_));
    }
    return res;
}

bool BooleanSetLiteral::hasPool(bool beforeRewrite, bool head) const {
    return beforeRewrite && repr_->hasPool();
}

void BooleanSetLiteral::replace(Defines &dx) {
    Term::replace(repr_, repr_->replace(dx, true));
}

Ground::ULit BooleanSetLiteral::toGround(DomainData &x, bool auxiliary) const {
    return gringo_make_unique<Ground::RelationLiteral>(
        Relation::EQ,
        make_locatable<ValTerm>(loc(), Symbol::createNum(0)),
        make_locatable<ValTerm>(loc(), Symbol::createNum(value_ ? 0 : 1)));
}

ULit BooleanSetLiteral::shift(bool negate) {
    return make_locatable<BooleanSetLiteral>(loc(), get_clone(repr_), negate ? !value_ : value_);
}

UTerm BooleanSetLiteral::headRepr() const {
    throw std::runtime_error("BooleanSetLiteral::headRepr must not be called");
}

bool BooleanSetLiteral::auxiliary() const {
    return true;
}

void BooleanSetLiteral::auxiliary(bool aux) {
    static_cast<void>(aux);
}

// }}}

} } // namespace Input Gringo

