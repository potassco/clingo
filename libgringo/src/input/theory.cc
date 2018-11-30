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

#include <gringo/input/theory.hh>
#include <gringo/input/literals.hh>
#include <gringo/ground/statements.hh>
#include <gringo/logger.hh>

namespace Gringo { namespace Input {

// {{{1 definition of TheoryElement

TheoryElement::TheoryElement(Output::UTheoryTermVec &&tuple, ULitVec &&cond)
: tuple_(std::move(tuple))
, cond_(std::move(cond))
{ }

TheoryElement::TheoryElement(TheoryElement &&) = default;

TheoryElement &TheoryElement::operator=(TheoryElement &&) = default;

TheoryElement::~TheoryElement() noexcept = default;

bool TheoryElement::operator==(TheoryElement const &other) const {
    return is_value_equal_to(tuple_, other.tuple_) && is_value_equal_to(cond_, other.cond_);
}

void TheoryElement::print(std::ostream &out) const {
    if (tuple_.empty() && cond_.empty()) {
        out << " : ";
    }
    else {
        print_comma(out, tuple_, ",", [](std::ostream &out, Output::UTheoryTerm const &term) { term->print(out); });
        if (!cond_.empty()) {
            out << ": ";
            print_comma(out, cond_, ",", [](std::ostream &out, ULit const &lit) { lit->print(out); });
        }
    }
}

size_t TheoryElement::hash() const {
    return get_value_hash(tuple_, cond_);
}

void TheoryElement::unpool(TheoryElementVec &elems, bool beforeRewrite) {
    Term::unpool(cond_.begin(), cond_.end(), [beforeRewrite](ULit &lit) {
        return lit->unpool(beforeRewrite);
    }, [&](ULitVec &&cond) {
        elems.emplace_back(get_clone(tuple_), std::move(cond));
    });
}

TheoryElement TheoryElement::clone() const {
    return {get_clone(tuple_), get_clone(cond_)};
}

bool TheoryElement::hasPool(bool beforeRewrite) const {
    for (auto &lit : cond_) {
        if (lit->hasPool(beforeRewrite)) { return true; }
    }
    return false;
}

void TheoryElement::replace(Defines &x) {
    for (auto &term : tuple_) {
        term->replace(x);
    }
    for (auto &lit : cond_) {
        lit->replace(x);
    }
}

void TheoryElement::collect(VarTermBoundVec &vars) const {
    for (auto &term : tuple_) { term->collect(vars); }
    for (auto &lit : cond_) { lit->collect(vars, false); }
}

void TheoryElement::assignLevels(AssignLevel &lvl) {
    AssignLevel &local(lvl.subLevel());
    VarTermBoundVec vars;
    for (auto &term : tuple_) { term->collect(vars); }
    for (auto &lit : cond_) { lit->collect(vars, true); }
    local.add(vars);
}

void TheoryElement::check(Location const &loc, Printable const &p, ChkLvlVec &levels, Logger &log) const {
    levels.emplace_back(loc, p);
    for (auto &lit : cond_) {
        levels.back().current = &levels.back().dep.insertEnt();
        VarTermBoundVec vars;
        lit->collect(vars, true);
        addVars(levels, vars);
    }
    VarTermBoundVec vars;
    levels.back().current = &levels.back().dep.insertEnt();
    for (auto &term : tuple_) {
        term->collect(vars);
    }
    addVars(levels, vars);
    levels.back().check(log);
    levels.pop_back();
}

bool TheoryElement::simplify(Projections &project, SimplifyState &state, Logger &log) {
    for (auto &lit : cond_) {
        if (!lit->simplify(log, project, state, true, true)) {
            return false;
        }
    }
    for (auto &dot : state.dots) { cond_.emplace_back(RangeLiteral::make(dot)); }
    for (auto &script : state.scripts) { cond_.emplace_back(ScriptLiteral::make(script)); }
    return true;
}

void TheoryElement::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) {
    Literal::AssignVec assign;
    arith.emplace_back(gringo_make_unique<Term::LevelMap>());
    for (auto &lit : cond_) { lit->rewriteArithmetics(arith, assign, auxGen); }
    for (auto &y : *arith.back()) { cond_.emplace_back(RelationLiteral::make(y)); }
    for (auto &y : assign) { cond_.emplace_back(RelationLiteral::make(y)); }
    arith.pop_back();
}

void TheoryElement::initTheory(Output::TheoryParser &p, Logger &log) {
    for (auto &term : tuple_) {
        Term::replace(term, term->initTheory(p, log));
    }
}

std::unique_ptr<Ground::TheoryAccumulate> TheoryElement::toGround(ToGroundArg &x, Ground::TheoryComplete &completeRef, Ground::ULitVec &&lits) const {
    for (auto &z : cond_) { lits.emplace_back(z->toGround(x.domains, false)); }
    return gringo_make_unique<Ground::TheoryAccumulate>(completeRef, get_clone(tuple_), std::move(lits));
}

// {{{1 definition of TheoryAtom

TheoryAtom::TheoryAtom(UTerm &&name, TheoryElementVec &&elems)
: name_(std::move(name))
, elems_(std::move(elems))
, op_("")
, guard_(nullptr)
{ }

TheoryAtom::TheoryAtom(TheoryAtom &&) = default;

TheoryAtom &TheoryAtom::operator=(TheoryAtom &&) = default;

TheoryAtom::TheoryAtom(UTerm &&name, TheoryElementVec &&elems, String op, Output::UTheoryTerm &&guard, TheoryAtomType type)
: name_(std::move(name))
, elems_(std::move(elems))
, op_(op)
, guard_(std::move(guard))
, type_(type)
{ }

TheoryAtom TheoryAtom::clone() const {
    return { get_clone(name_), get_clone(elems_), op_, hasGuard() ? get_clone(guard_) : nullptr, type_ };
}

bool TheoryAtom::hasGuard() const {
    return static_cast<bool>(guard_);
}

bool TheoryAtom::operator==(TheoryAtom const &other) const {
    bool ret = is_value_equal_to(name_, other.name_) && is_value_equal_to(elems_, other.elems_) && hasGuard() == other.hasGuard();
    if (ret && hasGuard()) {
        ret = op_ == other.op_ && is_value_equal_to(guard_, other.guard_);
    }
    return ret;
}

void TheoryAtom::print(std::ostream &out) const {
    out << "&";
    name_->print(out);
    out << "{";
    print_comma(out, elems_, ";");
    out << "}";
    if (hasGuard()) {
        out << op_;
        guard_->print(out);
    }
}

size_t TheoryAtom::hash() const {
    size_t hash = get_value_hash(name_, elems_);
    if (hasGuard()) {
        hash = get_value_hash(hash, op_, guard_);
    }
    return hash;
}

template <class T>
void TheoryAtom::unpool(T f, bool beforeRewrite) {
    TheoryElementVec elems;
    for (auto &elem : elems_) {
        elem.unpool(elems, beforeRewrite);
    }
    UTermVec names;
    name_->unpool(names);
    for (auto &name : names) {
        f(TheoryAtom(std::move(name), get_clone(elems), op_, guard_ ? get_clone(guard_) : nullptr));
    }
}

bool TheoryAtom::hasPool(bool beforeRewrite) const {
    if (beforeRewrite && name_->hasPool()) { return true; }
    for (auto &elem : elems_) {
        if (elem.hasPool(beforeRewrite)) { return true; }
    }
    return false;
}

void TheoryAtom::replace(Defines &x) {
    Term::replace(name_, name_->replace(x, true));
    for (auto &elem : elems_) {
        elem.replace(x);
    }
    if (hasGuard()) { guard_->replace(x); }
}

void TheoryAtom::collect(VarTermBoundVec &vars) const {
    name_->collect(vars, false);
    if (hasGuard()) { guard_->collect(vars); }
    for (auto &elem : elems_) { elem.collect(vars); }
}

void TheoryAtom::assignLevels(AssignLevel &lvl) {
    VarTermBoundVec vars;
    name_->collect(vars, false);
    if (hasGuard()) { guard_->collect(vars); }
    lvl.add(vars);
    for (auto &elem : elems_) {
        elem.assignLevels(lvl);
    }
}

void TheoryAtom::check(Location const &loc, Printable const &p, ChkLvlVec &levels, Logger &log) const {
    levels.back().current = &levels.back().dep.insertEnt();

    VarTermBoundVec vars;
    name_->collect(vars, false);
    if (hasGuard()) { guard_->collect(vars); }
    addVars(levels, vars);

    for (auto &elem : elems_) {
        elem.check(loc, p, levels, log);
    }
}

bool TheoryAtom::simplify(Projections &project, SimplifyState &state, Logger &log) {
    if (name_->simplify(state, false, false, log).update(name_).undefined()) {
        return false;
    }
    for (auto &elem : elems_) {
        SimplifyState elemState(state);
        if (!elem.simplify(project, elemState, log)) {
            return false;
        }
    }
    return true;
}

void TheoryAtom::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) {
    for (auto &elem : elems_) {
        elem.rewriteArithmetics(arith, auxGen);
    }
}

void TheoryAtom::initTheory(Location const &loc, TheoryDefs &defs, bool inBody, bool hasBody, Logger &log) {
    Sig sig = name_->getSig();
    for (auto &def : defs) {
        if (auto atomDef = def.getAtomDef(sig)) {
            type_ = atomDef->type();
            if (inBody) {
                if (type_ == TheoryAtomType::Head) {
                    GRINGO_REPORT(log, Warnings::RuntimeError)
                        << loc << ": error: theory body atom used in head:" << "\n"
                        << "  " << sig << "\n";
                    return;
                }
                else if (type_ == TheoryAtomType::Directive) {
                    GRINGO_REPORT(log, Warnings::RuntimeError)
                        << loc << ": error: theory directive used in body:" << "\n"
                        << "  " << sig << "\n";
                    return;
                }
            }
            else {
                if (type_ == TheoryAtomType::Body) {
                    GRINGO_REPORT(log, Warnings::RuntimeError)
                        << loc << ": error: theory head atom used in body:" << "\n"
                        << "  " << sig << "\n";
                    return;
                }
                if (type_ == TheoryAtomType::Directive && hasBody) {
                    GRINGO_REPORT(log, Warnings::RuntimeError)
                        << loc << ": error: theory directive used with body:" << "\n"
                        << "  " << sig << "\n";
                    return;
                }
            }
            if (inBody) {
                type_ = TheoryAtomType::Body;
            }
            else if (type_ != TheoryAtomType::Directive) {
                type_ = TheoryAtomType::Head;
            }
            if (auto termDef = def.getTermDef(atomDef->elemDef())) {
                Output::TheoryParser p(loc, *termDef);
                for (auto &elem : elems_) {
                    elem.initTheory(p, log);
                }
            }
            else {
                GRINGO_REPORT(log, Warnings::RuntimeError)
                    << loc << ": error: missing definition for term:" << "\n"
                    << "  " << atomDef->elemDef() << "\n";
            }
            if (hasGuard()) {
                if (!atomDef->hasGuard()) {
                    GRINGO_REPORT(log, Warnings::RuntimeError)
                        << loc << ": error: unexpected guard:" << "\n"
                        << "  " << sig << "\n";
                }
                else if (auto termDef = def.getTermDef(atomDef->guardDef())) {
                    if (std::find(atomDef->ops().begin(), atomDef->ops().end(), op_) != atomDef->ops().end()) {
                        Output::TheoryParser p(loc, *termDef);
                        Term::replace(guard_, guard_->initTheory(p, log));
                    }
                    else {
                        std::stringstream ss;
                        print_comma(ss, atomDef->ops(), ",");
                        GRINGO_REPORT(log, Warnings::RuntimeError)
                            << loc << ": error: unexpected operator:" << "\n"
                            << "  " << op_ << "\n"
                            << loc << ": note: expected one of:\n"
                            << "  " << ss.str() << "\n";
                    }
                }
                else {
                    GRINGO_REPORT(log, Warnings::RuntimeError)
                        << loc << ": error: missing definition for term:" << "\n"
                        << "  " << atomDef->guardDef() << "\n";
                }
            }
            return;
        }
    }
    GRINGO_REPORT(log, Warnings::RuntimeError)
        << loc << ": error: no definition found for theory atom:" << "\n"
        << "  " << sig << "\n";
}

CreateHead TheoryAtom::toGroundHead() const {
    return CreateHead([&](Ground::ULitVec &&lits) {
        for (auto &x : lits) {
            auto lit = dynamic_cast<Ground::TheoryLiteral*>(x.get());
            if (lit && lit->auxiliary()) {
                return gringo_make_unique<Ground::TheoryRule>(*lit, std::move(lits));
            }
        }
        throw std::logic_error("must not happen");
    });
}

CreateBody TheoryAtom::toGroundBody(ToGroundArg &x, Ground::UStmVec &stms, NAF naf, UTerm &&id) const {
    if (hasGuard()) {
        stms.emplace_back(gringo_make_unique<Ground::TheoryComplete>(x.domains, std::move(id), type_, get_clone(name_), op_, get_clone(guard_)));
    }
    else {
        stms.emplace_back(gringo_make_unique<Ground::TheoryComplete>(x.domains, std::move(id), type_, get_clone(name_)));
    }
    auto &completeRef = static_cast<Ground::TheoryComplete&>(*stms.back());
    CreateStmVec split;
    split.emplace_back([&completeRef](Ground::ULitVec &&lits) -> Ground::UStm {
        auto ret = gringo_make_unique<Ground::TheoryAccumulate>(completeRef, std::move(lits));
        completeRef.addAccuDom(*ret);
        return std::move(ret);
    });
    for (auto &y : elems_) {
        split.emplace_back([&completeRef,&y,&x](Ground::ULitVec &&lits) -> Ground::UStm {
            auto ret = y.toGround(x, completeRef, std::move(lits));
            completeRef.addAccuDom(*ret);
            return std::move(ret);
        });
    }
    bool aux1 = type_ != TheoryAtomType::Body;
    return CreateBody([&completeRef, naf, aux1](Ground::ULitVec &lits, bool primary, bool aux2) {
        if (primary) {
            auto ret = gringo_make_unique<Ground::TheoryLiteral>(completeRef, naf, aux1 || aux2);
            lits.emplace_back(std::move(ret));
        }
    }, std::move(split));
}

TheoryAtom::~TheoryAtom() noexcept = default;

// {{{1 definition of HeadTheoryLiteral

HeadTheoryLiteral::HeadTheoryLiteral(TheoryAtom &&atom, bool rewritten)
: atom_(std::move(atom))
, rewritten_(rewritten)
{ }

HeadTheoryLiteral::~HeadTheoryLiteral() noexcept = default;

CreateHead HeadTheoryLiteral::toGround(ToGroundArg &, Ground::UStmVec &, Ground::RuleType) const {
    return atom_.toGroundHead();
}

UHeadAggr HeadTheoryLiteral::rewriteAggregates(UBodyAggrVec &lits) {
    rewritten_ = true;
    auto lit = make_locatable<BodyTheoryLiteral>(loc(), NAF::POS, atom_.clone(), rewritten_);
    lits.emplace_back(std::move(lit));
    return nullptr;
}

void HeadTheoryLiteral::collect(VarTermBoundVec &vars) const {
    atom_.collect(vars);
}

void HeadTheoryLiteral::unpool(UHeadAggrVec &x, bool beforeRewrite) {
    atom_.unpool([&](TheoryAtom &&atom) { x.emplace_back(make_locatable<HeadTheoryLiteral>(loc(), std::move(atom))); }, beforeRewrite);
}

bool HeadTheoryLiteral::simplify(Projections &project, SimplifyState &state, Logger &log) {
    return atom_.simplify(project, state, log);
}

void HeadTheoryLiteral::assignLevels(AssignLevel &lvl) {
    atom_.assignLevels(lvl);
}

void HeadTheoryLiteral::rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) {
    atom_.rewriteArithmetics(arith, auxGen);
}

bool HeadTheoryLiteral::hasPool(bool beforeRewrite) const {
    return atom_.hasPool(beforeRewrite);
}

void HeadTheoryLiteral::check(ChkLvlVec &lvl, Logger &log) const {
    atom_.check(loc(), *this, lvl, log);
}

void HeadTheoryLiteral::replace(Defines &x) {
    atom_.replace(x);
}

size_t HeadTheoryLiteral::hash() const {
    return atom_.hash();
}

void HeadTheoryLiteral::print(std::ostream &out) const {
    if (rewritten_) { out << "#false"; }
    else            { atom_.print(out); }
}

HeadTheoryLiteral *HeadTheoryLiteral::clone() const {
    auto ret = make_locatable<HeadTheoryLiteral>(loc(), get_clone(atom_), rewritten_).release();
    return ret;
}

bool HeadTheoryLiteral::operator==(HeadAggregate const &other) const {
    auto t = dynamic_cast<HeadTheoryLiteral const*>(&other);
    return t && atom_ == t->atom_;
}

void HeadTheoryLiteral::initTheory(TheoryDefs &defs, bool hasBody, Logger &log) {
    atom_.initTheory(loc(), defs, false, hasBody, log);
}

void HeadTheoryLiteral::getNeg(std::function<void (Sig)>) const { }

// {{{1 definition of BodyTheoryLiteral

BodyTheoryLiteral::BodyTheoryLiteral(NAF naf, TheoryAtom &&atom, bool rewritten)
: atom_(std::move(atom))
, naf_(naf)
, rewritten_(rewritten)
{ }

BodyTheoryLiteral::~BodyTheoryLiteral() noexcept = default;

void BodyTheoryLiteral::unpool(UBodyAggrVec &x, bool beforeRewrite) {
    atom_.unpool([&](TheoryAtom &&atom) { x.emplace_back(make_locatable<BodyTheoryLiteral>(loc(), naf_, std::move(atom))); }, beforeRewrite);
}

bool BodyTheoryLiteral::simplify(Projections &project, SimplifyState &state, bool, Logger &log) {
    return atom_.simplify(project, state, log);
}

void BodyTheoryLiteral::assignLevels(AssignLevel &lvl) {
    atom_.assignLevels(lvl);
}

void BodyTheoryLiteral::check(ChkLvlVec &lvl, Logger &log) const {
    atom_.check(loc(), *this, lvl, log);
}

void BodyTheoryLiteral::rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::AssignVec &, AuxGen &auxGen) {
    atom_.rewriteArithmetics(arith, auxGen);
}

bool BodyTheoryLiteral::rewriteAggregates(UBodyAggrVec &) {
    return true;
}

void BodyTheoryLiteral::removeAssignment() {
}

bool BodyTheoryLiteral::isAssignment() const {
    return false;
}

void BodyTheoryLiteral::collect(VarTermBoundVec &vars) const {
    atom_.collect(vars);
}

bool BodyTheoryLiteral::hasPool(bool beforeRewrite) const {
    return atom_.hasPool(beforeRewrite);
}

void BodyTheoryLiteral::replace(Defines &x) {
    atom_.replace(x);
}

CreateBody BodyTheoryLiteral::toGround(ToGroundArg &x, Ground::UStmVec &stms) const {
    return atom_.toGroundBody(x, stms, naf_, x.newId(*this));
}

size_t BodyTheoryLiteral::hash() const {
    return get_value_hash(typeid(BodyTheoryLiteral).hash_code(), naf_, atom_);
}

void BodyTheoryLiteral::print(std::ostream &out) const {
    if (rewritten_) { out << "not " << atom_; }
    else            { out << naf_ << atom_; }
}

BodyTheoryLiteral *BodyTheoryLiteral::clone() const {
    return make_locatable<BodyTheoryLiteral>(loc(), naf_, get_clone(atom_), rewritten_).release();
}

bool BodyTheoryLiteral::operator==(BodyAggregate const &other) const {
    auto t = dynamic_cast<BodyTheoryLiteral const*>(&other);
    return t && naf_ == t->naf_ && atom_ == t->atom_;
}

void BodyTheoryLiteral::initTheory(TheoryDefs &defs, Logger &log) {
    atom_.initTheory(loc(), defs, true, true, log);
}

// }}}1

} } // namespace Gringo Input

