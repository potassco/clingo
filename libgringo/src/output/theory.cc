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

#include "gringo/output/theory.hh"
#include "gringo/output/literal.hh"
#include "gringo/logger.hh"
#include <cstring>

namespace Gringo { namespace Output {

namespace {

// {{{1 definition of comparison operators

bool termEqual(Potassco::TheoryTerm const &term, int number) {
    return term.type() == Potassco::Theory_t::Number && term.number() == number;
}

bool termEqual(Potassco::TheoryTerm const &term, char const *symbol) {
    return term.type() == Potassco::Theory_t::Symbol && strcmp(term.symbol(), symbol) == 0;
}

bool termEqual(Potassco::TheoryTerm const &term, Potassco::Tuple_t type, Potassco::IdSpan const &terms) {
    return term.type() == Potassco::Theory_t::Compound && term.isTuple() && term.tuple() == type && term.size() == terms.size && std::equal(term.begin(), term.end(), Potassco::begin(terms));
}

bool termEqual(Potassco::TheoryTerm const &term, Potassco::Id_t name, Potassco::IdSpan const &terms) {
    return term.type() == Potassco::Theory_t::Compound && term.isFunction() && term.function() == name && term.size() == terms.size && std::equal(term.begin(), term.end(), Potassco::begin(terms));
}

bool elementEqual(Potassco::TheoryElement const &a, LitVec const &condA, Potassco::IdSpan const &tuple, LitVec const &condB) {
    return is_value_equal_to(condA, condB) && a.size() == tuple.size && std::equal(a.begin(), a.end(), Potassco::begin(tuple));
}

bool atomEqual(Potassco::TheoryAtom const &a, Potassco::Id_t termId, Potassco::IdSpan const &elems) {
    return !a.guard() && a.term() == termId && a.size() == elems.size && std::equal(a.begin(), a.end(), Potassco::begin(elems));
}

bool atomEqual(Potassco::TheoryAtom const &a, Potassco::Id_t termId, Potassco::IdSpan const &elems, Potassco::Id_t guard, Potassco::Id_t rhs) {
    return a.guard() && *a.guard() == guard && *a.rhs() == rhs && a.term() == termId && a.size() == elems.size && std::equal(a.begin(), a.end(), Potassco::begin(elems));
}

bool atomEqual(Potassco::TheoryAtom const &a, Potassco::TheoryAtom const &b) {
    return b.guard()
        ? atomEqual(a, b.term(), b.elements(), *b.guard(), *b.rhs())
        : atomEqual(a, b.term(), b.elements());
}

// }}}1
// {{{1 definition of hash functions

size_t termHash(int number) {
    return Gringo::get_value_hash(static_cast<unsigned>(Potassco::Theory_t::Number), number);
}

size_t termHash(char const *symbol) {
    return Gringo::get_value_hash(static_cast<unsigned>(Potassco::Theory_t::Symbol), strhash(symbol));
}

size_t termHash(Potassco::Id_t name, Potassco::IdSpan const &terms) {
    size_t seed = Gringo::get_value_hash(static_cast<unsigned>(Potassco::Theory_t::Compound), name);
    for (auto&& t : terms) {
        Gringo::hash_combine(seed, t);
    }
    return seed;
}

size_t termHash(Potassco::Tuple_t type, Potassco::IdSpan const &terms) {
    size_t seed = Gringo::get_value_hash(static_cast<unsigned>(Potassco::Theory_t::Compound), static_cast<unsigned>(type));
    for (auto&& t : terms) {
        Gringo::hash_combine(seed, t);
    }
    return seed;
}

size_t termHash(Potassco::TheoryTerm const &term) {
    switch (term.type()) {
        case Potassco::Theory_t::Number:   { return termHash(term.number()); }
        case Potassco::Theory_t::Symbol:   { return termHash(term.symbol()); }
        case Potassco::Theory_t::Compound: { return term.isTuple() ? termHash(term.tuple(), term.terms()) : termHash(term.function(), term.terms()); }
    }
    assert(false);
    return 0;
}

size_t elementHash(Potassco::IdSpan const &tuple, LitVec const &cond) {
    size_t seed = get_value_hash(cond);
    for (auto&& t : tuple) { Gringo::hash_combine(seed, t); }
    return seed;
}

size_t elementHash(Potassco::TheoryElement const &elem, LitVec const &cond) {
    return elementHash(elem.terms(), cond);
}

size_t atomHash(Potassco::Id_t termId, Potassco::IdSpan const &elems) {
    size_t seed = 0;
    Gringo::hash_combine(seed, termId);
    for (auto&& t : elems) {
        Gringo::hash_combine(seed, t);
    }
    return seed;
}

size_t atomHash(Potassco::Id_t termId, Potassco::IdSpan const &elems, Potassco::Id_t guard, Potassco::Id_t rhs) {
    size_t seed = atomHash(termId, elems);
    Gringo::hash_combine(seed, guard);
    Gringo::hash_combine(seed, rhs);
    return seed;
}

size_t atomHash(Potassco::TheoryAtom const &atom) {
    return atom.guard()
        ? atomHash(atom.term(), atom.elements(), *atom.guard(), *atom.rhs())
        : atomHash(atom.term(), atom.elements());
}

// }}}1

} // namespace

// {{{1 definition of TheoryParser

TheoryParser::Elem::Elem(String op, bool unary)
: tokenType(Op)
, op(op, unary) { }

TheoryParser::Elem::Elem(UTheoryTerm &&term)
: tokenType(Id)
, term(std::move(term))  {}

TheoryParser::Elem::Elem(Elem &&elem)
: tokenType(elem.tokenType) {
    if (elem.tokenType == Id) {
        new (&term) UTheoryTerm(std::move(elem.term));
    }
    else {
        new (&op) std::pair<String,bool>(std::move(elem.op));
    }
}

TheoryParser::Elem::~Elem() noexcept {
    if (tokenType == Id) {
        term.~UTheoryTerm();
    }
    else {
        op.~pair();
    }
}

TheoryParser::TheoryParser(Location const &loc, TheoryTermDef const &def)
: loc_(loc)
, def_(def) { }

bool TheoryParser::check(String op) {
    if (stack_.size() < 2) { return false; }
    assert(stack_.back().tokenType == Id);
    auto current  = def_.getPrioAndAssoc(op);
    auto previous = def_.getPrio(stack_[stack_.size() - 2].op.first, stack_[stack_.size() - 2].op.second);
    return previous > current.first || (previous == current.first && current.second);
}

void TheoryParser::reduce() {
    assert(stack_.back().tokenType == Id);
    auto b = std::move(stack_.back().term);
    stack_.pop_back();
    assert(stack_.back().tokenType == Op);
    auto op = stack_.back().op;
    stack_.pop_back();
    if (!op.second) {
        assert(stack_.back().tokenType == Id);
        auto a = std::move(stack_.back().term);
        stack_.pop_back();
        stack_.emplace_back(gringo_make_unique<BinaryTheoryTerm>(std::move(a), op.first, std::move(b)));
    }
    else {
        stack_.emplace_back(gringo_make_unique<UnaryTheoryTerm>(op.first, std::move(b)));
    }
}

UTheoryTerm TheoryParser::parse(RawTheoryTerm::ElemVec &&elems, Logger &log) {
    stack_.clear();
    bool unary = true;
    for (auto &elem : elems) {
        for (auto &op : elem.first) {
            if (!def_.hasOp(op, unary)) {
                GRINGO_REPORT(log, Warnings::RuntimeError)
                    << loc_ << ": error: missing definition for operator:" << "\n"
                    << "  " << op << "\n";
            }
            while (!unary && check(op)) {
                reduce();
            }
            stack_.emplace_back(op, unary);
            unary = true;
        }
        stack_.emplace_back(std::move(elem.second));
        unary = false;
    }
    while (stack_.size() > 1) {
        reduce();
    }
    return std::move(stack_.front().term);
}

TheoryParser::~TheoryParser() noexcept = default;

// {{{1 definition of RawTheoryTerm

RawTheoryTerm::RawTheoryTerm() { }

RawTheoryTerm::RawTheoryTerm(ElemVec &&elems)
: elems_(std::move(elems)) { }

RawTheoryTerm::RawTheoryTerm(RawTheoryTerm &&) = default;

RawTheoryTerm &RawTheoryTerm::operator=(RawTheoryTerm &&) = default;

RawTheoryTerm::~RawTheoryTerm() noexcept = default;

void RawTheoryTerm::append(StringVec &&ops, UTheoryTerm &&term) {
    assert(elems_.empty() || !ops.empty());
    elems_.emplace_back(std::move(ops), std::move(term));
}

size_t RawTheoryTerm::hash() const {
    return get_value_hash(typeid(RawTheoryTerm).hash_code(), elems_);
}

bool RawTheoryTerm::operator==(TheoryTerm const &other) const {
    auto t = dynamic_cast<RawTheoryTerm const*>(&other);
    return t && is_value_equal_to(elems_, t->elems_);
}

void RawTheoryTerm::print(std::ostream &out) const {
    out << "(";
    print_comma(out, elems_, ",", [](std::ostream &out, ElemVec::value_type const &elem) {
        print_comma(out, elem.first, " ");
        out << *elem.second;
    });
    out << ")";
}

RawTheoryTerm *RawTheoryTerm::clone() const {
    return gringo_make_unique<RawTheoryTerm>(get_clone(elems_)).release();
}

Potassco::Id_t RawTheoryTerm::eval(TheoryData &, Logger &) const {
    throw std::logic_error("RawTheoryTerm::eval must not be called!");
}

void RawTheoryTerm::collect(VarTermBoundVec &vars) {
    for (auto &elem : elems_) {
        elem.second->collect(vars);
    }
}

void RawTheoryTerm::replace(Defines &defs) {
    for (auto &elem : elems_) {
        elem.second->replace(defs);
    }
}

UTheoryTerm RawTheoryTerm::initTheory(TheoryParser &p, Logger &log) {
    for (auto &elem : elems_) {
        Term::replace(elem.second, elem.second->initTheory(p, log));
    }
    return p.parse(std::move(elems_), log);
}

// {{{1 definition of UnaryTheoryTerm

UnaryTheoryTerm::UnaryTheoryTerm(String op, UTheoryTerm &&arg)
: arg_(std::move(arg))
, op_(std::move(op))
{ }

UnaryTheoryTerm::~UnaryTheoryTerm() noexcept = default;

size_t UnaryTheoryTerm::hash() const {
    return get_value_hash(typeid(UnaryTheoryTerm).hash_code(), arg_, op_);
}

bool UnaryTheoryTerm::operator==(TheoryTerm const &other) const {
    auto t = dynamic_cast<UnaryTheoryTerm const*>(&other);
    return t && is_value_equal_to(arg_, t->arg_) && op_ == t->op_;
}

void UnaryTheoryTerm::print(std::ostream &out) const {
    out << "(" << op_ << *arg_ << ")";
}

UnaryTheoryTerm *UnaryTheoryTerm::clone() const {
    return gringo_make_unique<UnaryTheoryTerm>(op_, get_clone(arg_)).release();
}

Potassco::Id_t UnaryTheoryTerm::eval(TheoryData &data, Logger &log) const {
    auto op = data.addTerm(op_.c_str());
    Potassco::Id_t args[] = { arg_->eval(data, log) };
    return data.addTermFun(op, Potassco::toSpan(args, 1));
}

void UnaryTheoryTerm::collect(VarTermBoundVec &vars) {
    arg_->collect(vars);
}

void UnaryTheoryTerm::replace(Defines &defs) {
    arg_->replace(defs);
}

UTheoryTerm UnaryTheoryTerm::initTheory(TheoryParser &p, Logger &log) {
    Term::replace(arg_, arg_->initTheory(p, log));
    return nullptr;
}

// {{{1 definition of BinaryTheoryTerm

BinaryTheoryTerm::BinaryTheoryTerm(UTheoryTerm &&left, String op, UTheoryTerm &&right)
: left_(std::move(left))
, right_(std::move(right))
, op_(op)
{ }

BinaryTheoryTerm::~BinaryTheoryTerm() noexcept = default;

size_t BinaryTheoryTerm::hash() const {
    return get_value_hash(typeid(BinaryTheoryTerm).hash_code(), left_, right_, op_);
}

bool BinaryTheoryTerm::operator==(TheoryTerm const &other) const {
    auto t = dynamic_cast<BinaryTheoryTerm const*>(&other);
    return t && is_value_equal_to(left_, t->left_) && is_value_equal_to(right_, t->right_) && op_ == t->op_;
}

void BinaryTheoryTerm::print(std::ostream &out) const {
    out << "(" << *left_ << op_ << *right_ << ")";
}

BinaryTheoryTerm *BinaryTheoryTerm::clone() const {
    return gringo_make_unique<BinaryTheoryTerm>(get_clone(left_), op_, get_clone(right_)).release();
}

Potassco::Id_t BinaryTheoryTerm::eval(TheoryData &data, Logger &log) const {
    auto op = data.addTerm(op_.c_str());
    Potassco::Id_t args[] = { left_->eval(data, log), right_->eval(data, log) };
    return data.addTermFun(op, Potassco::toSpan(args, 2));
}

void BinaryTheoryTerm::collect(VarTermBoundVec &vars) {
    left_->collect(vars);
    right_->collect(vars);
}

void BinaryTheoryTerm::replace(Defines &defs) {
    left_->replace(defs);
    right_->replace(defs);
}

UTheoryTerm BinaryTheoryTerm::initTheory(TheoryParser &p, Logger &log) {
    Term::replace(left_, left_->initTheory(p, log));
    Term::replace(right_, right_->initTheory(p, log));
    return nullptr;
}

// {{{1 definition of TupleTheoryTerm

TupleTheoryTerm::TupleTheoryTerm(Type type, UTheoryTermVec &&args)
: args_(std::move(args))
, type_(type)
{ }

TupleTheoryTerm::~TupleTheoryTerm() noexcept = default;

size_t TupleTheoryTerm::hash() const {
    return get_value_hash(typeid(TupleTheoryTerm).hash_code(), static_cast<unsigned>(type_), args_);
}

bool TupleTheoryTerm::operator==(TheoryTerm const &other) const {
    auto t = dynamic_cast<TupleTheoryTerm const*>(&other);
    return t && is_value_equal_to(args_, t->args_) && type_ == t->type_;
}

void TupleTheoryTerm::print(std::ostream &out) const {
    auto parens = Potassco::toString(type_);
    out << parens[0];
    print_comma(out, args_, ",", [](std::ostream &out, UTheoryTerm const &term) { out << *term; });
    if (args_.size() == 1 && type_ == Potassco::Tuple_t::Paren) { out << ","; }
    out << parens[1];
}

TupleTheoryTerm *TupleTheoryTerm::clone() const {
    return gringo_make_unique<TupleTheoryTerm>(type_, get_clone(args_)).release();
}

Potassco::Id_t TupleTheoryTerm::eval(TheoryData &data, Logger &log) const {
    std::vector<Potassco::Id_t> args;
    for (auto &arg : args_) {
        args.emplace_back(arg->eval(data, log));
    }
    return data.addTermTup(type_, Potassco::toSpan(args));
}

void TupleTheoryTerm::collect(VarTermBoundVec &vars) {
    for (auto &term : args_) {
        term->collect(vars);
    }
}

void TupleTheoryTerm::replace(Defines &defs) {
    for (auto &term : args_) {
        term->replace(defs);
    }
}

UTheoryTerm TupleTheoryTerm::initTheory(TheoryParser &p, Logger &log) {
    for (auto &term : args_) {
        Term::replace(term, term->initTheory(p, log));
    }
    return nullptr;
}

// {{{1 definition of FunctionTheoryTerm

FunctionTheoryTerm::FunctionTheoryTerm(String name, UTheoryTermVec &&args)
: args_(std::move(args))
, name_(name)
{ }

FunctionTheoryTerm::~FunctionTheoryTerm() noexcept = default;

size_t FunctionTheoryTerm::hash() const {
    return get_value_hash(typeid(FunctionTheoryTerm).hash_code(), name_, args_);
}

bool FunctionTheoryTerm::operator==(TheoryTerm const &other) const {
    auto t = dynamic_cast<FunctionTheoryTerm const*>(&other);
    return t && is_value_equal_to(args_, t->args_) && name_ == t->name_;
}

void FunctionTheoryTerm::print(std::ostream &out) const {
    out << name_ << "(";
    print_comma(out, args_, ",", [](std::ostream &out, UTheoryTerm const &term) { out << *term; });
    out << ")";
}

FunctionTheoryTerm *FunctionTheoryTerm::clone() const {
    return gringo_make_unique<FunctionTheoryTerm>(name_, get_clone(args_)).release();
}

Potassco::Id_t FunctionTheoryTerm::eval(TheoryData &data, Logger &log) const {
    auto name = data.addTerm(name_.c_str());
    std::vector<Potassco::Id_t> args;
    for (auto &arg : args_) {
        args.emplace_back(arg->eval(data, log));
    }
    return data.addTermFun(name, Potassco::toSpan(args));
}

void FunctionTheoryTerm::collect(VarTermBoundVec &vars) {
    for (auto &term : args_) {
        term->collect(vars);
    }
}

void FunctionTheoryTerm::replace(Defines &defs) {
    for (auto &term : args_) {
        term->replace(defs);
    }
}

UTheoryTerm FunctionTheoryTerm::initTheory(TheoryParser &p, Logger &log) {
    for (auto &term : args_) {
        Term::replace(term, term->initTheory(p, log));
    }
    return nullptr;
}

// {{{1 definition of TermTheoryTerm

TermTheoryTerm::TermTheoryTerm(UTerm &&term)
: term_(std::move(term)) {
    assert(dynamic_cast<VarTerm*>(term_.get()) || dynamic_cast<ValTerm*>(term_.get()));
}

TermTheoryTerm::~TermTheoryTerm() noexcept = default;

size_t TermTheoryTerm::hash() const {
    return get_value_hash(typeid(TermTheoryTerm).hash_code(), term_);
}

bool TermTheoryTerm::operator==(TheoryTerm const &other) const {
    auto t = dynamic_cast<TermTheoryTerm const*>(&other);
    return t && is_value_equal_to(term_, t->term_);
}

void TermTheoryTerm::print(std::ostream &out) const {
    out << "(" << *term_ << ")";
}

TermTheoryTerm *TermTheoryTerm::clone() const {
    return gringo_make_unique<TermTheoryTerm>(get_clone(term_)).release();
}

Potassco::Id_t TermTheoryTerm::eval(TheoryData &data, Logger &log) const {
    bool undefined = false;
    return data.addTerm(term_->eval(undefined, log));
}

void TermTheoryTerm::collect(VarTermBoundVec &vars) {
    term_->collect(vars, false);
}

void TermTheoryTerm::replace(Defines &defs) {
    Term::replace(term_, term_->replace(defs, true));
}

UTheoryTerm TermTheoryTerm::initTheory(TheoryParser &, Logger &) {
    return nullptr;
}

// {{{1 definition of TheoryData

TheoryData::TheoryData(Potassco::TheoryData &data)
: data_(data)
{ }

TheoryData::~TheoryData() noexcept = default;

template <typename ...Args>
Potassco::Id_t TheoryData::addTerm_(Args ...args) {
    auto size = terms_.size();
    auto ret = terms_.insert([&](Potassco::Id_t const &a) {
        assert(a != std::numeric_limits<Potassco::Id_t>::max());
        return a == size ? termHash(args...) : termHash(data_.getTerm(a));
    }, [&](Potassco::Id_t const &a, Potassco::Id_t const &b) {
        assert(a < size);
        return b == size ? termEqual(data_.getTerm(a), args...) : a == b;
    }, size);
    if (ret.second) { data_.addTerm(size, args...); }
    return ret.first;
}

Potassco::Id_t TheoryData::addTerm(int number) {
    return addTerm_(number);
}

Potassco::Id_t TheoryData::addTerm(char const *name) {
    return addTerm_(name);
}

Potassco::Id_t TheoryData::addTermFun(Potassco::Id_t funcSym, Potassco::IdSpan const& terms) {
    return addTerm_(funcSym, terms);
}

Potassco::Id_t TheoryData::addTermTup(Potassco::Tuple_t type, Potassco::IdSpan const& terms) {
    return addTerm_(type, terms);
}

Potassco::Id_t TheoryData::addTerm(Symbol value) {
    switch (value.type()) {
        case SymbolType::Num: {
            auto num = value.num();
            if (num < 0) {
                auto f = addTerm("-");
                auto ret = addTerm(-num);
                Potassco::Id_t args[] = { ret };
                return addTermFun(f, Potassco::toSpan(args, 1));
            }
            else {
                return addTerm(num);
            }
        }
        case SymbolType::Str: {
            std::string s;
            s.push_back('"');
            s.append(quote(value.string().c_str()));
            s.push_back('"');
            return addTerm(s.c_str());
        }
        case SymbolType::Sup: {
            return addTerm("#sup");
        }
        case SymbolType::Inf: {
            return addTerm("#inf");
        }
        case SymbolType::Fun: {
            std::vector<Potassco::Id_t> args;
            for (auto &arg : value.args()) {
                args.emplace_back(addTerm(arg));
            }
            if (value.name().empty()) {
                return addTermTup(Potassco::Tuple_t::Paren, Potassco::toSpan(args));
            }
            else {
                Potassco::Id_t name = addTerm(value.name().c_str());
                auto ret = args.empty()
                    ? addTerm(value.name().c_str())
                    : addTermFun(name, Potassco::toSpan(args));
                if (value.sign()) {
                    Potassco::Id_t f = addTerm("-");
                    ret = addTermFun(f, Potassco::toSpan(&ret, 1));
                }
                return ret;
            }
        }
        case SymbolType::Special: {
            assert(false);
            break;
        }
    }
    assert(false);
    return 0;
}

Potassco::Id_t TheoryData::addElem(Potassco::IdSpan const &tuple, LitVec &&cond) {
    assert(conditions_.size() == elems_.size());
    auto size = elems_.size();
    auto ret = elems_.insert([&](Potassco::Id_t const &a) {
        assert(a != std::numeric_limits<Potassco::Id_t>::max());
        return a == size ? elementHash(tuple, cond) : elementHash(data_.getElement(a), conditions_[a]);
    }, [&](Potassco::Id_t const &a, Potassco::Id_t const &b) {
        assert(a < size);
        return b == size ? elementEqual(data_.getElement(a), conditions_[a], tuple, cond) : a == b;
    }, numeric_cast<unsigned>(conditions_.size()));
    if (ret.second) {
        data_.addElement(size, tuple, cond.empty() ? 0 : Potassco::TheoryData::COND_DEFERRED);
        conditions_.emplace_back(std::move(cond));
    }
    return ret.first;
}

template <typename ...Args>
std::pair<Potassco::TheoryAtom const&, bool> TheoryData::addAtom_(std::function<Potassco::Id_t()> newAtom, Args ...args) {
    auto **ret = reinterpret_cast<Potassco::TheoryAtom const **>(atoms_.find([&]() {
        return atomHash(args...);
    }, [&](uintptr_t a) {
        return atomEqual(*reinterpret_cast<Potassco::TheoryAtom const *>(a), args...);
    }));
    if (!ret) {
        auto &atom = data_.addAtom(newAtom(), args...);
        atoms_.insert([&](uintptr_t a) {
            return atomHash(*reinterpret_cast<Potassco::TheoryAtom const *>(a));
        }, [&](uintptr_t a, uintptr_t b) {
            return atomEqual(*reinterpret_cast<Potassco::TheoryAtom const *>(a), *reinterpret_cast<Potassco::TheoryAtom const *>(b));
        }, reinterpret_cast<uintptr_t>(&atom));
        return {atom, true};
    }
    return {**ret, false};
}

std::pair<Potassco::TheoryAtom const &, bool> TheoryData::addAtom(std::function<Potassco::Id_t()> newAtom, Potassco::Id_t termId, Potassco::IdSpan const &elems) {
    return addAtom_(newAtom, termId, elems);
}

std::pair<Potassco::TheoryAtom const &, bool> TheoryData::addAtom(std::function<Potassco::Id_t()> newAtom, Potassco::Id_t termId, Potassco::IdSpan const &elems, Potassco::Id_t op, Potassco::Id_t rhs) {
    return addAtom_(newAtom, termId, elems, op, rhs);
}

void TheoryData::printTerm(std::ostream &out, Potassco::Id_t termId) const {
    auto &term = data_.getTerm(termId);
    switch (term.type()) {
        case Potassco::Theory_t::Number: {
            if (term.number() < 0) { out << "("; }
            out << term.number();
            if (term.number() < 0) { out << ")"; }
            break;
        }
        case Potassco::Theory_t::Symbol: { out << term.symbol(); break; }
        case Potassco::Theory_t::Compound: {
            auto parens = Potassco::toString(term.isTuple() ? term.tuple() : Potassco::Tuple_t::Paren);
            bool isOp = false;
            if (term.isFunction()) {
                auto &name = data_.getTerm(term.function());
                char buf[2] = { *name.symbol(), 0 };
                isOp = term.size() <= 2 && std::strpbrk(buf, "/!<=>+-*\\?&@|:;~^.");
                if (!isOp) { printTerm(out, term.function()); }
            }
            out << parens[0];
            if (isOp && term.size() <= 1) {
                printTerm(out, term.function());
            }
            print_comma(out, term, isOp ? data_.getTerm(term.function()).symbol() : ",", [this](std::ostream &out, Potassco::Id_t termId){ printTerm(out, termId); });
            if (term.isTuple() && term.tuple() == Potassco::Tuple_t::Paren && term.size() == 1) { out << ","; }
            out << parens[1];
            break;
        }
    }
}

void TheoryData::printElem(std::ostream &out, Potassco::Id_t elemId, PrintLit printLit) const {
    auto &elem = data_.getElement(elemId);
    auto &cond = getCondition(elemId);
    print_comma(out, elem, ",", [this](std::ostream &out, Potassco::Id_t termId){ printTerm(out, termId); });
    if (elem.size() == 0 && cond.empty()) {
        out << ": ";
    }
    else if (!cond.empty()) {
        out << ": ";
        print_comma(out, cond, ",", [this, &printLit](std::ostream &out, LiteralId const &lit){ printLit(out, lit); });
    }
}

bool TheoryData::empty() const {
    return data_.numAtoms() == 0;
}

Potassco::TheoryData const &TheoryData::data() const {
    return data_;
}

LitVec &TheoryData::getCondition(Potassco::Id_t elemId) {
    assert(elemId < conditions_.size());
    return conditions_[elemId];
}

LitVec const &TheoryData::getCondition(Potassco::Id_t elemId) const {
    assert(elemId < conditions_.size());
    return conditions_[elemId];
}

void TheoryData::setCondition(Potassco::Id_t elementId, Potassco::Id_t newCond) {
    data_.setCondition(elementId, newCond);
}

void TheoryData::reset(bool resetData) {
    TIdSet().swap(terms_);
    TIdSet().swap(elems_);
    AtomSet().swap(atoms_);
    ConditionVec().swap(conditions_);
    if (resetData) { data_.reset(); }
}

bool TheoryData::hasConditions() const {
    return !conditions_.empty();
}


// }}}1

} } // namspace Gringo
