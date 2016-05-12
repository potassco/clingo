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

#include <gringo/output/literal.hh>
#include <gringo/output/statements.hh>
#include <gringo/logger.hh>
#include <gringo/control.hh>
#include <gringo/output/aggregates.hh>
#include <gringo/output/theory.hh>

namespace Gringo { namespace Output {

// {{{1 definition of functions to work with BodyAggregates

namespace {

void printLit(PrintPlain out, LiteralId lit) {
    call(out.domain, lit, &Literal::printPlain, out);
}

void printCond(PrintPlain out, FWValVec tuple, Formula::value_type cond) {
    print_comma(out, tuple, ",");
    out << ":";
    auto rng = out.domain.clause(cond.first, cond.second);
    print_comma(out, rng, ",", [](PrintPlain out, LiteralId lit) { printLit(out, lit); });
}

void printCond(PrintPlain out, FWValVec tuple, HeadFormula::value_type const &cond) {
    print_comma(out, tuple, ",");
    out << ":";
    if (cond.first.valid()) { printLit(out, cond.first); }
    else { out << "#true"; }
    if (cond.second.second > 0) {
        out << ":";
        auto rng = out.domain.clause(cond.second.first, cond.second.second);
        print_comma(out, rng, ",", [](PrintPlain out, LiteralId lit) { printLit(out, lit); });
    }
}

void printBodyElem(PrintPlain out, BodyAggregateElements::ValueType const &x) {
    if (x.second.empty()) { print_comma(out, x.first, ","); }
    else { print_comma(out, x.second, ";", [&x](PrintPlain out, Formula::value_type cond) { printCond(out, x.first, cond); }); }
}

void printHeadElem(PrintPlain out, HeadAggregateElements::ValueType const &x) {
    print_comma(out, x.second, ";", [&x](PrintPlain out, HeadFormula::value_type const &cond) { printCond(out, x.first, cond); });
}

template <class Atom>
void makeFalse(DomainData &data, Translator &trans, NAF naf, Atom &atm) {
    LiteralId atomlit;
    switch (naf) {
        case NAF::POS:    { atomlit = data.getTrueLit().negate(); break; }
        case NAF::NOT:    { atomlit = data.getTrueLit(); break; }
        case NAF::NOTNOT: { atomlit = data.getTrueLit().negate(); break; }
    }
    auto lit = atm.lit();
    if (lit) {
        assert(lit.sign() == NAF::POS);
        Rule().addHead(lit).addBody(atomlit).translate(data, trans);
    }
    else { atm.setLit(atomlit); }
}

} // namespace

int clamp(int64_t x) {
    if (x > std::numeric_limits<int>::max()) { return std::numeric_limits<int>::max(); }
    if (x < std::numeric_limits<int>::min()) { return std::numeric_limits<int>::min(); }
    return int(x);
}

bool defined(ValVec const &tuple, AggregateFunction fun, Location const &loc) {
    if (tuple.empty()) {
        if (fun == AggregateFunction::COUNT) { return true; }
        else {
            GRINGO_REPORT(W_OPERATION_UNDEFINED)
                << loc << ": info: empty tuple ignored\n";
            return false;
        }
    }
    if (tuple.front().type() == Value::SPECIAL) { return true; }
    switch (fun) {
        case AggregateFunction::MIN:
        case AggregateFunction::MAX:
        case AggregateFunction::COUNT: { return true; }
        case AggregateFunction::SUM:
        case AggregateFunction::SUMP: {
            if (tuple.front().type() == Value::NUM) { return true; }
            else {
                std::ostringstream s;
                print_comma(s, tuple, ",");
                GRINGO_REPORT(W_OPERATION_UNDEFINED)
                    << loc << ": info: tuple ignored:\n"
                    << "  " << s.str() << "\n";
                return false;
            }
        }
    }
    return true;
}


bool neutral(ValVec const &tuple, AggregateFunction fun, Location const &loc) {
    if (tuple.empty()) {
        if (fun == AggregateFunction::COUNT) { return false; }
        else {
            GRINGO_REPORT(W_OPERATION_UNDEFINED)
                << loc << ": info: empty tuple ignored\n";
            return true;
        }
    }
    else if (tuple.front().type() != Value::SPECIAL) {
        bool ret = true;
        switch (fun) {
            case AggregateFunction::MIN:   { return tuple.front() == Value::createSup(); }
            case AggregateFunction::MAX:   { return tuple.front() == Value::createInf(); }
            case AggregateFunction::COUNT: { return false; }
            case AggregateFunction::SUM:   { ret = tuple.front().type() != Value::NUM || tuple.front() == Value::createNum(0); break; }
            case AggregateFunction::SUMP:  { ret = tuple.front().type() != Value::NUM || tuple.front() <= Value::createNum(0); break; }
        }
        if (ret && tuple.front() != Value::createNum(0)) {
            std::ostringstream s;
            print_comma(s, tuple, ",");
            GRINGO_REPORT(W_OPERATION_UNDEFINED)
                << loc << ": info: tuple ignored:\n"
                << "  " << s.str() << "\n";
        }
        return ret;
    }
    return true;
}

int toInt(IntervalSet<Value>::LBound const &x) {
    if (x.bound.type() == Value::NUM) {
        return x.inclusive ? x.bound.num() : x.bound.num() + 1;
    }
    else {
        if (x.bound < Value::createNum(0)) { return std::numeric_limits<int>::min(); }
        else             { return std::numeric_limits<int>::max(); }
    }
}

int toInt(IntervalSet<Value>::RBound const &x) {
    if (x.bound.type() == Value::NUM) {
        return x.inclusive ? x.bound.num() : x.bound.num() - 1;
    }
    else {
        if (x.bound < Value::createNum(0)) { return std::numeric_limits<int>::min(); }
        else             { return std::numeric_limits<int>::max(); }
    }
}

Value getWeight(AggregateFunction fun, FWValVec const &x) {
    return fun == AggregateFunction::COUNT ? Value::createNum(1) : x.front();
}

// {{{1 definition of AggregateAtomRange

void AggregateAtomRange::init(AggregateFunction fun, DisjunctiveBounds &&bounds) {
    switch (fun) {
        case AggregateFunction::MIN: {
            valMin() = Value::createSup();
            valMax() = Value::createSup();
            break;
        }
        case AggregateFunction::MAX: {
            valMin() = Value::createInf();
            valMax() = Value::createInf();
            break;
        }
        default: {
            intMin() = 0;
            intMax() = 0;
            break;
        }
    }
    this->fun = fun;
    this->bounds = std::move(bounds);
}

Interval AggregateAtomRange::range() const {
    if (fun != AggregateFunction::MIN && fun != AggregateFunction::MAX) {
        return {{Value::createNum(clamp(intMin())), true}, {Value::createNum(clamp(intMax())), true}};
    }
    else { return {{valMin(), true}, {valMax(), true}}; }
}

PlainBounds AggregateAtomRange::plainBounds() {
    // There are 3 cases:
    // (a) disjunctive bounds are open to the left: > or >=
    // (b) disjunctive bounds are open to the right: < or <=
    // (c) < and > follow each other: !=
    // Case (c) can furthermore correspond to one or two != occurrences.
    PlainBounds cBounds;
    auto rng = range();
    for (auto part : bounds) {
        if (rng.left < part.left) {
            // (a)
            if (cBounds.empty()) {
                cBounds.emplace_back(part.left.inclusive ? Relation::GEQ : Relation::GT, part.left.bound);
            }
            // (c)
            else {
                assert(cBounds.back().first == Relation::LT || cBounds.back().first == Relation::LEQ);
                Value left = cBounds.back().second;
                if (cBounds.back().first == Relation::LEQ) {
                    assert(left.type() == Value::NUM);
                    left = Value::createNum(left.num() + 1);
                }
                Value right = part.left.bound;
                if (part.left.inclusive) {
                    assert(right.type() == Value::NUM);
                    right = Value::createNum(right.num() - 1);
                }
                cBounds.back() = {Relation::NEQ, left};
                if (left != right) {
                    cBounds.back() = {Relation::NEQ, right};
                }
            }
        }
        if (part.right < rng.right) {
            // (b)
            cBounds.emplace_back(part.right.inclusive ? Relation::LEQ : Relation::LT, part.right.bound);
        }
    }
    assert(cBounds.size() < 3);
    return cBounds;

}

void AggregateAtomRange::accumulate(FWValVec tuple, bool fact, bool remove) {
    switch (fun) {
        case AggregateFunction::MIN: {
            Value val = tuple.front();
            if (fact) { valMax() = std::min<Value>(valMax(), val); }
            valMin() = std::min<Value>(valMin(), val);
            break;
        }
        case AggregateFunction::MAX: {
            Value val = tuple.front();
            if (fact) { valMin() = std::max<Value>(valMin(), val); }
            valMax() = std::max<Value>(valMax(), val);
            break;
        }
        default: {
            int64_t val = fun == AggregateFunction::COUNT ? 1 : tuple.front().num();
            if (fact) {
                if (remove) {
                    if (val < 0) { intMax()+= val; }
                    else         { intMin()+= val; }
                }
                else {
                    intMin()+= val;
                    intMax()+= val;
                }
            }
            else {
                if (val < 0) { intMin()+= val; }
                else         { intMax()+= val; }
            }
            break;
        }
    }
}

// {{{1 definition of BodyAggregateAtom

class BodyAggregateElements_::TupleOffset {
public:
    TupleOffset(Id_t offset, Id_t size, bool fact)
    : repr_(fact | (static_cast<uint64_t>(size) << 1) | (static_cast<uint64_t>(offset) << 32)) { }
    TupleOffset(uint64_t repr)
    : repr_(repr) { }
    bool fact() const { return repr_ & 1; }
    Id_t size() const { return static_cast<uint32_t>(repr_) >> 1; }
    Id_t offset() const { return static_cast<uint32_t>(repr_ >> 32); }
    uint64_t repr() const { return repr_; }
    operator uint64_t() const { return repr(); }
private:
    uint64_t repr_;
};

class BodyAggregateElements_::ClauseOffset {
public:
    ClauseOffset(Id_t offset, Id_t size)
    : repr_(std::max(size-1, Id_t(3)) | (offset << 2)) {
        assert(size > 0);
    }
    ClauseOffset(uint32_t repr)
    : repr_(repr) { }
    Id_t size() const { return (repr_ & 3) < 3 ? (repr_ & 3) + 1 : InvalidId; }
    Id_t offset() const { return static_cast<uint32_t>(repr_ >> 2); }
    uint32_t repr() const { return repr_; }
    operator uint32_t() const { return repr(); }
private:
    uint32_t repr_;
};

std::pair<uint64_t &, bool> BodyAggregateElements_::insertTuple(uint64_t to) {
    return tuples_.insert(
        [](uint64_t a) { return std::hash<uint64_t>()(a >> 1); },
        [](uint64_t a, uint64_t b){ return (a >> 1) == (b >> 1); },
        to);
}

template <class F>
void BodyAggregateElements_::visitClause(F f) {
    for (auto it = conditions_.begin(), ie = conditions_.end(); it != ie; ) {
        auto &to = *it++;
        Id_t size = 0;
        Id_t offset = 0;
        if (to & 1) {
            ClauseOffset co(*it++);
            offset = co.offset();
            size = co.size();
            if (size == InvalidId) { size = *it++; }
        }
        f(to, ClauseId(offset, size));
    }
}

void BodyAggregateElements_::accumulate(DomainData &data, FWValVec tuple, LitVec &lits, bool &inserted, bool &fact, bool &remove) {
    if (tuples_.reserveNeedsRebuild(tuples_.size() + 1)) {
        // remap tuple offsets if rebuild is necessary
        HashSet<uint64_t> tuples(tuples_.size() + 1, tuples_.reserved());
        tuples.swap(tuples_);
        if (tuples.reserved() >= tuples_.reserved()) {
            std::cerr << tuples.size() << " / " << tuples.load() << " / " << tuples.reserved() << " < " << tuples_.reserved() << std::endl;
        }
        assert(tuples.reserved() < tuples_.reserved());
        visitClause([&](uint32_t &to, ClauseId) {
            to = (tuples_.offset(insertTuple(tuples.at(to >> 1)).first) << 1) | (to & 1);
        });
    }
    TupleOffset newTO(tuple.offset(), tuple.size(), lits.empty());
    auto ret = insertTuple(newTO);
    inserted = ret.second;
    remove = false;
    if (!inserted) {
        TupleOffset oldTO = ret.first;
        if (!oldTO.fact() && newTO.fact()) {
            ret.first = newTO;
            remove = true;
        }
        else { newTO = oldTO; }
    }
    fact = newTO.fact();
    if (!fact || inserted || remove) {
        auto clause = data.clause(std::move(lits));
        auto size = clause.second;
        conditions_.emplace_back((tuples_.offset(ret.first) << 1) | (size > 0));
        if (size > 0) {
            ClauseOffset co(clause.first, size);
            conditions_.emplace_back(co);
            if (co.size() == InvalidId) { conditions_.emplace_back(size); }
        }
    }
}
BodyAggregateElements BodyAggregateElements_::elems() const {
    BodyAggregateElements elems;
    const_cast<BodyAggregateElements_*>(this)->visitClause([&](uint32_t const &to, ClauseId cond) {
        TupleOffset fo(tuples_.at(to >> 1));
        FWValVec tuple(FWValVec::fromOffset, fo.size(), fo.offset());
        auto ret = elems.push(std::piecewise_construct, std::forward_as_tuple(tuple), std::forward_as_tuple());
        if (fo.fact()) { ret.first->second.clear(); }
        ret.first->second.emplace_back(cond);
    });
    return elems;
}

void BodyAggregateAtom::init(AggregateFunction fun, DisjunctiveBounds &&bounds, bool monotone) {
    data_->range.init(fun, std::move(bounds));
    data_->fact = data_->range.fact();
    data_->monotone = monotone;
    data_->initialized = true;
}

BodyAggregateElements BodyAggregateAtom::elems() const {
    return data_->elems.elems();
}

void BodyAggregateAtom::accumulate(DomainData &data, Location const &loc, ValVec const &tuple, LitVec &lits) {
    if (neutral(tuple, data_->range.fun, loc)) { return; }
    bool inserted, fact, remove;
    data_->elems.accumulate(data, tuple, lits, inserted, fact, remove);
    if (!fact || inserted || remove) {
        data_->range.accumulate(tuple, fact, remove);
        data_->fact = data_->range.fact();
    }
}

BodyAggregateAtom::~BodyAggregateAtom() noexcept = default;

// {{{1 definition of AssignmentAggregateAtom

void AssignmentAggregateData::accumulate(DomainData &data, Location const &loc, ValVec const &tuple, LitVec &lits) {
    if (neutral(tuple, fun_, loc)) { return; }
    auto ret(elems_.push(std::piecewise_construct, std::forward_as_tuple(tuple), std::forward_as_tuple()));
    auto &elem = ret.first->second;
    // the tuple was fact
    if (elem.size() == 1 && elem.front().second == 0) { return; }
    bool fact = false;
    bool remove = false;
    if (lits.empty()) {
        elem.clear();
        fact = true;
        remove = !ret.second;
    }
    elem.emplace_back(data.clause(lits));
    if (!ret.second && !remove) { return; }
    switch (fun_) {
        case AggregateFunction::MIN: {
            Value val = tuple.front();
            if (fact) {
                values_.erase(std::remove_if(values_.begin() + 1, values_.end(), [val](Value x) { return x >= val; }), values_.end());
                if (values_.front() > val) { values_.front() = val; }
            }
            else if (values_.front() > val) { values_.push_back(val); }
            break;
        }
        case AggregateFunction::MAX: {
            Value val = tuple.front();
            if (fact) {
                values_.erase(std::remove_if(values_.begin() + 1, values_.end(), [val](Value x) { return x <= val; }), values_.end());
                if (values_.front() < val) { values_.front() = val; }
            }
            else if (values_.front() < val) { values_.push_back(val); }
            break;
        }
        default: {
            Value val = fun_ == AggregateFunction::COUNT ? Value::createNum(1) : tuple.front();
            if (fact) {
                if (remove) { values_.erase(std::find(values_.begin() + 1, values_.end(), val)); }
                values_.front() = Value::createNum(values_.front().num() + val.num());
            }
            else { values_.push_back(val); }
            break;
        }
    }
}

AssignmentAggregateData::Values AssignmentAggregateData::values() const {
    switch (fun_) {
        case AggregateFunction::MIN:
        case AggregateFunction::MAX: {
            Values values = values_;
            sort_unique(values);
            return values;
        }
        default: {
            UniqueVec<Value> values;
            auto it = values_.begin();
            values.push(*it++);
            for (auto ie = values_.end(); it != ie; ++it) {
                for (Id_t jt = 0, je = values.size(); jt != je; ++jt) { values.push(Value::createNum(values[jt].num() + it->num())); }
            }
            return {values.begin(), values.end()};
        }
    }
}

Interval AssignmentAggregateData::range() const {
    switch (fun_) {
        case AggregateFunction::MAX:
        case AggregateFunction::MIN: {
            Value valMin = values_.front();
            Value valMax = valMin;
            for (auto it = values_.begin() + 1, ie = values_.end(); it != ie; ++it) {
                valMin = std::min(valMin, *it);
                valMax = std::max(valMax, *it);
            }
            return Interval{{valMin,true},{valMax,true}};
        }
        default: {
            int64_t intMin = values_.front().num();
            int64_t intMax = intMin;
            for (auto it = values_.begin() + 1, ie = values_.end(); it != ie; ++it) {
                int val = it->num();
                if (val < 0) { intMin+= val; }
                else         { intMax+= val; }
            }
            // NOTE: nonsense, proper handling is not achieved like this...
            return {{Value::createNum(clamp(intMin)), true}, {Value::createNum(clamp(intMax)), true}};
        }
    }
}

// {{{1 definition of ConjunctionAtom

// Example:
//   H :- p(G), q(G,X,Y) : r(G,X,Z).
//   global variables: G
//   local variables : X, Y, Z
//   value of the conjunction: c(G)
//   value of the element    : e(X)
//   disjunction of bodies is spanned by Z
//   disjunction of heads is spanned by Y

bool ConjunctionElement::isSimple(DomainData &data) const {
    return (heads_.empty() && bodies_.size() == 1 && bodies_.front().second == 1 && data.clause(bodies_.front()).front().invertible()) ||
           (bodies_.size() == 1 && bodies_.front().second == 0 && heads_.size() <= 1);
}

bool ConjunctionElement::needsSemicolon() const {
    return !bodies_.empty() && bodies_.front().second > 0;
}

void ConjunctionElement::print(PrintPlain out) const {
    using namespace std::placeholders;
    if (bodies_.empty()) {
        out << "#true";
    }
    else {
        if (heads_.empty()) {
            out << "#false";
        }
        else {
            auto pc = [](PrintPlain out, ClauseId id) {
                if (id.second == 0) {
                    out << "#true";
                }
                else {
                    print_comma(out, out.domain.clause(id), "&", printLit);
                }
            };
            print_comma(out, heads_, "|", pc);
        }
        if (bodies_.front().second > 0) {
            out << ":";
            auto pc = [](PrintPlain out, ClauseId id) {
                if (id.second == 0) {
                    out << "#true";
                }
                else {
                    print_comma(out, out.domain.clause(id), ",", printLit);
                }
            };
            print_comma(out, bodies_, "|", pc);
        }
    }
}
void ConjunctionElement::accumulateCond(DomainData &data, LitVec &lits, Id_t &blocked, Id_t &fact) {
    if (bodies_.empty()) {
         // there can only be a head if there is at least one body
        assert(heads_.empty());
        ++fact;
    }
    if (bodies_.size() != 1 || bodies_.front().second > 0) {
        if (lits.empty()) {
            bodies_.clear();
            if (heads_.empty()) { ++blocked; }
        }
        bodies_.emplace_back(data.clause(lits));
    }
}

void ConjunctionElement::accumulateHead(DomainData &data, LitVec &lits, Id_t &blocked, Id_t &fact) {
    // NOTE: returns true for newly satisfiable elements
    if (heads_.empty() && bodies_.size() == 1 && bodies_.front().second == 0) {
        assert(blocked > 0);
        --blocked;
    }
    if (heads_.size() != 1 || heads_.front().second > 0) {
        if (lits.empty()) {
            heads_.clear();
            assert(fact > 0);
            --fact;
        }
        heads_.emplace_back(data.clause(lits));
    }
}

void ConjunctionAtom::accumulateCond(DomainData &data, Value elem, LitVec &lits) {
    elems_.findPush(elem, elem).first->accumulateCond(data, lits, blocked_, fact_);
}

void ConjunctionAtom::accumulateHead(DomainData &data, Value elem, LitVec &lits) {
    elems_.findPush(elem, elem).first->accumulateHead(data, lits, blocked_, fact_);
}

bool ConjunctionAtom::recursive() const {
    return (headRecursive_ || condRecursive_) && !fact();
}

bool ConjunctionAtom::nonmonotone() const {
    return !fact() && condRecursive_;
}

void ConjunctionAtom::init(bool headRecursive, bool condRecursive) {
    headRecursive_ = headRecursive;
    condRecursive_ = condRecursive;
}

// {{{1 definition of DisjointAtom

DisjointElement::DisjointElement(CSPGroundAdd &&value, int fixed, ClauseId cond)
: value_(std::move(value))
, cond_(cond)
, fixed_(fixed) { }

void DisjointElement::printPlain(PrintPlain out) const {
    if (value_.empty()) { out << fixed_; }
    else {
        auto print_add = [](PrintPlain out, CSPGroundAdd::value_type const &z) {
            if (z.first == 1) { out              << "$" << z.second; }
            else              { out << z.first << "$*$" << z.second; }
        };
        print_comma(out, value_, "$+", print_add);
        if (fixed_ > 0)      { out << "$+" << fixed_; }
        else if (fixed_ < 0) { out << "$-" << -fixed_; }
    }
    if (cond_.second > 0) {
        out << ":";
        print_comma(out, out.domain.clause(cond_), ",", [](PrintPlain out, LiteralId lit) { printLit(out, lit); });
    }
}

void DisjointAtom::accumulate(DomainData &data, ValVec const &tuple, CSPGroundAdd &&value, int fixed, LitVec const &lits) {
    auto elem = elems_.push(std::piecewise_construct, std::forward_as_tuple(tuple), std::forward_as_tuple());
    elem.first->second.emplace_back(std::move(value), fixed, data.clause(get_clone(lits)));
}

bool DisjointAtom::translate(DomainData &data, Translator &x) {
    std::set<int> values;
    std::vector<std::map<int, LiteralId>> layers;
    for (auto &elem : elems_) {
        layers.emplace_back();
        auto &current = layers.back();
        auto encodeSingle = [&data, &current, &x, &values](ClauseId cond, int coef, Value var, int fixed) -> void {
            auto &bound = x.findBound(var);
            auto it = bound.atoms.begin() + 1;
            for(auto i : bound) {
                auto &aux = current[i*coef + fixed];
                if (!aux) { aux = data.newAux(); }
                Rule rule;
                if (cond.second > 0) { rule.addBody(getEqualClause(data, x, cond, true, false)); }
                if ((it-1)->second)          { rule.addBody(LiteralId{NAF::NOT, AtomType::Aux, (it-1)->second, 0}); }
                if (it != bound.atoms.end()) { rule.addBody(LiteralId{NAF::POS, AtomType::Aux, it->second, 0}); }
                rule.addHead(aux).translate(data, x);
                values.insert(i*coef + fixed);
                ++it;
            }
        };
        for (auto &condVal : elem.second) {
            switch(condVal.value().size()) {
                case 0: {
                    auto &aux = current[condVal.fixed()];
                    if (!aux) { aux = data.newAux(); }
                    Rule().addHead(aux).addBody(getEqualClause(data, x, condVal.cond(), true, false)).translate(data, x);
                    values.insert(condVal.fixed());
                    break;
                }
                case 1: {
                    encodeSingle(condVal.cond(), condVal.value().front().first, condVal.value().front().second, condVal.fixed());
                    break;
                }
                default: {
                    // create a bound possibly with holes
                    auto b = x.addBound(Value::createId("#aux" + std::to_string(data.newAtom())));
                    b->clear();
                    std::set<int> values;
                    values.emplace(0);
                    for (auto &mul : condVal.value()) {
                        std::set<int> nextValues;
                        for (auto i : x.findBound(mul.second)) {
                            for (auto j : values) { nextValues.emplace(j + i * mul.first); }
                        }
                        values = std::move(nextValues);
                    }
                    for (auto i : values) { b->add(i, i+1); }
                    b->init(data, x);

                    // create a new variable with the respective bound
                    // b = value + fixed
                    // b - value = fixed
                    // aux :- b - value <=  fixed - 1
                    // aux :- value - b <= -fixed - 1
                    // :- aux.
                    LiteralId aux = data.newAux();
                    auto value = condVal.value();
                    value.emplace_back(-1, b->var);
                    x.addLinearConstraint(aux, get_clone(value), -condVal.fixed()-1);
                    for (auto &add : value) { add.first *= -1; }
                    x.addLinearConstraint(aux, std::move(value), condVal.fixed()-1);
                    Rule().addBody(aux).translate(data, x);
                    // then proceed as in case one
                    encodeSingle(condVal.cond(), 1, b->var, 0);
                    break;
                }
            }
        }
    }
    LitVec checks;
    for (auto &value : values) {
        LitUintVec card;
        for (auto &layer : layers) {
            auto it = layer.find(value);
            if (it != layer.end()) { card.emplace_back(it->second, 1); }
        }
        LiteralId aux = data.newAux();
        checks.emplace_back(aux.negate());
        WeightRule wr(std::move(aux), 2, std::move(card));
        x.output(data, wr);
    }
    Rule().addHead(lit()).addBody(std::move(checks)).translate(data, x);
    return true;
}


// {{{1 definition of DisjunctionAtom

// Example:
//   p(1..10,X) : q(X,Y,G) :- r(G).
//   global variables: G
//   local variables : X, Y
//   value of the disjunction: c(G)
//   value of the element    : e(X)
//   disjunction of bodies is spanned by Y
//   conjunction of heads is spanned by 1..10

void DisjunctionElement::print(PrintPlain out) const {
    using namespace std::placeholders;
    if (bodies_.empty()) {
        out << "#false";
    }
    else {
        if (heads_.empty()) {
            out << "#true";
        }
        else {
            auto pc = [](PrintPlain out, ClauseId id) {
                if (id.second == 0) {
                    out << "#false";
                }
                else {
                    print_comma(out, out.domain.clause(id), "|", printLit);
                }
            };
            print_comma(out, heads_, "&", pc);
        }
        if (bodies_.front().second > 0) {
            out << ":";
            auto pc = [](PrintPlain out, ClauseId id) {
                if (id.second == 0) {
                    out << "#true";
                }
                else {
                    print_comma(out, out.domain.clause(id), "&", printLit);
                }
            };
            print_comma(out, bodies_, "|", pc);
        }
    }
}

// Note: this are general functions on formulas
bool DisjunctionElement::bodyIsTrue() const {
    return bodies_.size() == 1 && bodies_.front().second == 0;
}

bool DisjunctionElement::headIsTrue() const {
    return heads_.empty();
}

bool DisjunctionElement::bodyIsFalse() const {
    return bodies_.empty();
}

bool DisjunctionElement::headIsFalse() const {
    return heads_.size() == 1 && heads_.front().second == 0;
}

void DisjunctionElement::accumulateCond(DomainData &data, LitVec &lits, Id_t &fact) {
    if (!bodyIsTrue()) {
        if (lits.empty()) {
            bodies_.clear();
            if (headIsTrue()) { ++fact; }
        }
        bodies_.emplace_back(data.clause(lits));
    }
}

void DisjunctionElement::accumulateHead(DomainData &data, LitVec &lits, Id_t &fact) {
    if (!headIsFalse()) {
        if (bodyIsTrue() && headIsTrue()) { --fact; }
        if (lits.empty()) { heads_.clear(); }
        heads_.emplace_back(data.clause(lits));
    }
}

void DisjunctionAtom::simplify(bool &headFact) {
    headFact_ = 0;
    elems_.erase([this](DisjunctionElement &elem) {
        if (elem.headIsTrue() && elem.bodyIsTrue()) { ++headFact_; }
        return elem.bodyIsFalse() || elem.headIsFalse();
    });
    headFact = headFact_ > 0;
}

void DisjunctionAtom::accumulateCond(DomainData &data, Value elem, LitVec &lits) {
    elems_.findPush(elem, elem).first->accumulateCond(data, lits, headFact_);
}

void DisjunctionAtom::accumulateHead(DomainData &data, Value elem, LitVec &lits) {
    elems_.findPush(elem, elem).first->accumulateHead(data, lits, headFact_);
}

// {{{1 declaration of TheoryAtom

void TheoryAtom::simplify(TheoryData const &) {
    if (!simplified_) {
        // NOTE: tuples with non-factual conditions can be removed if there
        //       is an element with the same tuple and a factual condition
        std::sort(elems_.begin(), elems_.end());
        elems_.erase(std::unique(elems_.begin(), elems_.end()), elems_.end());
        elems_.shrink_to_fit();
        simplified_ = true;
    }
}

// {{{1 definition of HeadAggregateAtom

void HeadAggregateAtom::init(AggregateFunction fun, DisjunctiveBounds &&bounds) {
    range_.init(fun, std::move(bounds));
    fact_ = range_.fact();
    initialized_ = true;
}

void HeadAggregateAtom::accumulate(DomainData &data, Location const &loc, ValVec const &tuple, LiteralId head, LitVec &lits) {
    // Elements are grouped by their tuples.
    // Each tuple is associated with a vector of pairs of head literals and a condition.
    // If the head is a fact, this is represented with an invalid literal.
    // If a tuple is a fact, this is represented with the first element of the vector being having an invalid head and an empty condition.
    if (!Gringo::Output::defined(tuple, range_.fun, loc)) { return; }
    auto ret(elems_.push(std::piecewise_construct, std::forward_as_tuple(tuple), std::forward_as_tuple()));
    auto &elem = ret.first->second;
    bool fact = lits.empty() && !head.valid();
    bool wasFact = !elem.empty() && !elem.front().first.valid() && elem.front().second.second == 0;
    if (wasFact && fact) { return; }
    elem.emplace_back(head, data.clause(lits));
    bool remove = false;
    if (fact) {
        std::swap(elem.front(), elem.back());
        remove = !ret.second;
    }
    if ((!ret.second && !remove) || neutral(tuple, range_.fun, loc)) { return; }
    range_.accumulate(tuple, fact, remove);
    fact_ = range_.fact();
}

// }}}1

// {{{1 definition of AuxLiteral

AuxLiteral::AuxLiteral(DomainData &data, LiteralId id)
: data_(data)
, id_(id) { }

bool AuxLiteral::isHeadAtom() const { return id_.sign() == NAF::POS; }

LiteralId AuxLiteral::translate(Translator &trans) {
    return id_.sign() != NAF::NOTNOT ? id_ : trans.removeNotNot(data_, id_);
}

void AuxLiteral::printPlain(PrintPlain out) const { out << id_.sign() << (id_.domain() == 0 ? "#aux" : "#delayed") << "(" << id_.offset() << ")"; }

bool AuxLiteral::isIncomplete() const { return false; }

int AuxLiteral::uid() const {
    switch (id_.sign()) {
        case NAF::POS:    { return +static_cast<Potassco::Lit_t>(id_.offset()); }
        case NAF::NOT:    { return -static_cast<Potassco::Lit_t>(id_.offset()); }
        case NAF::NOTNOT: { throw std::logic_error("AuxLiteral::uid: translate must be called before!"); }
    }
    throw std::logic_error("AuxLiteral::uid: must not happen");
}

LiteralId AuxLiteral::simplify(Mappings &, AssignmentLookup assignment) const {
    auto value = assignment(id_.offset());
    if (value.second == Potassco::Value_t::Free) { return id_; }
    auto ret = data_.getTrueLit();
    if (value.second == Potassco::Value_t::False) { ret = ret.negate(); }
    if (id_.sign() == NAF::NOT){ ret = ret.negate(); }
    return id_;
}

bool AuxLiteral::isTrue(IsTrueLookup lookup) const {
    assert(id_.offset() > 0);
    return (id_.sign() == NAF::NOT) ^ lookup(id_.offset());
}

LiteralId AuxLiteral::toId() const {
    return id_;
}

AuxLiteral::~AuxLiteral() noexcept = default;

// {{{1 definition of PredicateLiteral

PredicateLiteral::PredicateLiteral(DomainData &data, LiteralId id)
: data_(data)
, id_(id) { }

bool PredicateLiteral::isHeadAtom() const {
    return id_.sign() == NAF::POS;
}

bool PredicateLiteral::isAtomFromPreviousStep() const {
    auto &domain = *data_.predDoms()[id_.domain()];
    return id_.offset() < domain.incOffset();
}

void PredicateLiteral::printPlain(PrintPlain out) const {
    auto &atom = data_.predDoms()[id_.domain()]->operator[](id_.offset());
    out << id_.sign() << static_cast<Value>(atom);
}

bool PredicateLiteral::isIncomplete() const { return false; }

LiteralId PredicateLiteral::toId() const {
    return id_;
}

LiteralId PredicateLiteral::translate(Translator &trans) {
    return id_.sign() != NAF::NOTNOT ? id_ : trans.removeNotNot(data_, id_);
}

int PredicateLiteral::uid() const {
    auto &atom = data_.predDoms()[id_.domain()]->operator[](id_.offset());
    if (!atom.hasUid()) { atom.setUid(data_.newAtom()); }
    switch (id_.sign()) {
        case NAF::POS:    { return +static_cast<Potassco::Lit_t>(atom.uid()); }
        case NAF::NOT:    { return -static_cast<Potassco::Lit_t>(atom.uid()); }
        case NAF::NOTNOT: { throw std::logic_error("PredicateLiteral::uid: translate must be called before!"); }
    }
    assert(false);
    return 0;
}

LiteralId PredicateLiteral::simplify(Mappings &mappings, AssignmentLookup assignment) const {
    auto offset = mappings[id_.domain()].get(id_.offset());
    if (offset == InvalidId) {
        auto ret = data_.getTrueLit();
        if (id_.sign() != NAF::NOT){ ret = ret.negate(); }
        return ret;
    }
    else {
        auto &atom = data_.predDoms()[id_.domain()]->operator[](offset);
        if (!atom.defined()) { return data_.getTrueLit().negate(); }
        if (atom.hasUid()) {
            auto value = assignment(atom.uid());
            if (value.second != Potassco::Value_t::Free) {
                auto ret = data_.getTrueLit();
                if (value.second == Potassco::Value_t::False) { ret = ret.negate(); }
                if (id_.sign() == NAF::NOT){ ret = ret.negate(); }
                return ret;
            }
        }
        return id_.withOffset(offset);
    }
}

bool PredicateLiteral::isTrue(IsTrueLookup lookup) const {
    auto &atom = data_.predDoms()[id_.domain()]->operator[](id_.offset());
    assert(atom.hasUid());
    return (id_.sign() == NAF::NOT) ^ lookup(atom.uid());
}

PredicateLiteral::~PredicateLiteral() noexcept = default;

// {{{1 definition of TheoryLiteral

TheoryLiteral::TheoryLiteral(DomainData &data, LiteralId id)
: data_(data)
, id_(id) { }

void TheoryLiteral::printPlain(PrintPlain out) const {
    auto &atm = data_.getAtom<TheoryDomain>(id_);
    if (atm.defined()) {
        atm.simplify(data_.theory());
        out << id_.sign();
        out << "&";
        out.printTerm(atm.name());
        out << "{";
        print_comma(out, atm.elems(), "; ", [](PrintPlain out, Potassco::Id_t elemId) { out.printElem(elemId); });
        out << "}";
        if (atm.hasGuard()) {
            out.printTerm(atm.op());
            out << "(";
            out.printTerm(atm.guard());
            out << ")";
        }
    }
    else { out << (id_.sign() == NAF::NOT ? "#true" : "#false"); }
}

bool TheoryLiteral::isHeadAtom() const {
    auto &atm = data_.getAtom<TheoryDomain>(id_);
    return atm.defined() && atm.type() != TheoryAtomType::Body;
}

bool TheoryLiteral::isIncomplete() const {
    return data_.getAtom<TheoryDomain>(id_).recursive();
}

std::pair<LiteralId,bool> TheoryLiteral::delayedLit() {
    auto &atm = data_.getAtom<TheoryDomain>(id_);
    bool found = atm.lit();
    if (!found) { atm.setLit(data_.newDelayed()); }
    return {atm.lit().withSign(id_.sign()), !found};
}

LiteralId TheoryLiteral::toId() const {
    return id_;
}

LiteralId TheoryLiteral::translate(Translator &trans) {
    auto &atm = data_.getAtom<TheoryDomain>(id_);
    if (!atm.translated()) {
        atm.setTranslated();
        if (atm.defined()) {
            atm.simplify(data_.theory());
            for (auto &elemId : atm.elems()) {
                auto &cond = data_.theory().getCondition(elemId);
                Gringo::Output::translate(data_, trans, cond);
            }
            auto newAtom = [&]() -> Atom_t {
                if (atm.type() == TheoryAtomType::Directive) { return 0; }
                if (!atm.lit()) { atm.setLit(data_.newAux()); }
                assert(atm.lit().type() == AtomType::Aux);
                return atm.lit().offset();
            };
            TheoryData &data = data_.theory();
            auto ret = atm.hasGuard()
                ? data.addAtom(newAtom, atm.name(), Potassco::toSpan(atm.elems()), atm.op(), atm.guard())
                : data.addAtom(newAtom, atm.name(), Potassco::toSpan(atm.elems()));
            // output newly inserted theory atoms
            if (ret.second) {
                backendLambda(data_, trans, [&ret](DomainData &data, Backend &out){
                    auto getCond = [&data](Id_t elem) {
                        TheoryData &td = data.theory();
                        Backend::LitVec bc;
                        for (auto &lit : td.getCondition(elem)) {
                            bc.emplace_back(call(data, lit, &Literal::uid));
                        }
                        return bc;
                    };
                    out.printTheoryAtom(ret.first, getCond);
                });
            }
            if (ret.first.atom() != 0) {
                // assign the literal of the theory atom
                if (!atm.lit()) { atm.setLit({NAF::POS, AtomType::Aux, ret.first.atom(), 0}); }
                // connect the theory atom with an existing one
                else if (ret.first.atom() != atm.lit().offset()) {
                    LiteralId head = atm.lit();
                    LiteralId body{NAF::POS, AtomType::Aux, ret.first.atom(), 0};
                    if (atm.type() == TheoryAtomType::Head) { std::swap(head, body); }
                    Rule().addHead(head).addBody(body).translate(data_, trans);
                }
            }
        }
        else { makeFalse(data_, trans, id_.sign(), atm); }
    }
    auto lit = atm.lit();
    return !lit ? lit : trans.removeNotNot(data_, lit.withSign(id_.sign()));
}

int TheoryLiteral::uid() const { throw std::logic_error("TheoryLiteral::uid: translate must be called before!"); }

TheoryLiteral::~TheoryLiteral() noexcept = default;

// {{{1 definition of CSPLiteral

CSPLiteral::CSPLiteral(DomainData &data, LiteralId id)
: data_(data)
, id_(id) { }

CSPGroundLit const &CSPLiteral::atom() const {
    return data_.cspAtom(id_.offset());

}
void CSPLiteral::printPlain(PrintPlain out) const {
    auto &atm = atom();
    out << id_.sign();
    if (!std::get<1>(atm).empty()) {
        auto f = [](PrintPlain out, std::pair<int, Value> mul) { out << mul.first << "$*$" << mul.second; };
        print_comma(out, std::get<1>(atm), "$+", f);
    }
    else { out << 0; }
    out << "$" << std::get<0>(atm);
    out << std::get<2>(atm);
}

bool CSPLiteral::isIncomplete() const {
    return false;
}

bool CSPLiteral::isBound(Value &value, bool negate) const {
    auto &atm = atom();
    Relation rel = std::get<0>(atm);
    if (id_.sign() == NAF::NOT) { negate = !negate; }
    if (negate) { rel = neg(rel); }
    if (std::get<1>(atm).size() != 1) { return false; }
    if (rel == Relation::NEQ)            { return false; }
    if (value.type() == Value::SPECIAL)  { value = std::get<1>(atm).front().second; }
    return value == std::get<1>(atm).front().second;
}

void CSPLiteral::updateBound(std::vector<CSPBound> &bounds, bool negate) const {
    auto &atm = atom();
    bounds.emplace_back(std::numeric_limits<int>::min(), std::numeric_limits<int>::max()-1);
    auto &bound = bounds.back();
    Relation rel = std::get<0>(atm);
    if (id_.sign() == NAF::NOT) { negate = !negate; }
    if (negate) { rel = neg(rel); }
    int coef  = std::get<1>(atm).front().first;
    int fixed = std::get<2>(atm);
    if (coef < 0) {
        coef  = -coef;
        fixed = -fixed;
        rel   = inv(rel);
    }
    switch (rel) {
        case Relation::LT:  { fixed--; }
        case Relation::LEQ: {
            fixed /= coef;
            bound.second = std::min(bound.second, fixed);
            break;
        }
        case Relation::GT:  { fixed++; }
        case Relation::GEQ: {
            fixed = (fixed+coef-1) / coef;
            bound.first  = std::max(bound.first,  fixed);
            break;
        }
        case Relation::EQ: {
            if (fixed % coef == 0) {
                fixed /= coef;
                bound.first  = std::max(bound.first,  fixed);
                bound.second = std::min(bound.second, fixed);
            }
            else {
                bound.first  = 0;
                bound.second = -1;
            }
            break;
        }
        case Relation::NEQ: { assert(false); }
    }
}

LiteralId CSPLiteral::translate(Translator &x) {
    // NOTE: consider replacing this with a theory atom
    auto &atm = atom();
    auto term = std::get<1>(atm);
    Relation rel = std::get<0>(atm);
    if (id_.sign() == NAF::NOT) { rel = neg(rel); }
    int bound = std::get<2>(atm);
    auto addInv = [&x](Atom_t aux, CoefVarVec &&vars, int bound) {
        for (auto &x : vars) { x.first *= -1; }
        x.addLinearConstraint(aux, std::move(vars), -bound);
    };
    Atom_t aux = data_.newAtom();
    switch (rel) {
        case Relation::EQ:
        case Relation::NEQ: {
            x.addLinearConstraint(aux, CoefVarVec(term), bound-1);
            addInv(aux, std::move(term), bound + 1);
            return LiteralId{rel != Relation::NEQ ? NAF::NOT : NAF::POS, AtomType::Aux, aux, 0};
        }
        case Relation::LT:  { bound--; }
        case Relation::LEQ: {
            x.addLinearConstraint(aux, std::move(term), bound);
            return LiteralId{NAF::POS, AtomType::Aux, aux, 0};
        }
        case Relation::GT:  { bound++; }
        case Relation::GEQ: {
            addInv(aux, std::move(term), bound);
            return LiteralId{NAF::POS, AtomType::Aux, aux, 0};
        }
    }
    assert(false);
    throw std::logic_error("cannot happen");
}

int CSPLiteral::uid() const {
    throw std::logic_error("CSPLiteral::uid must not be called");
}

CSPLiteral::~CSPLiteral() noexcept = default;

// {{{1 definition of BodyAggregateLiteral

BodyAggregateLiteral::BodyAggregateLiteral(DomainData &data, LiteralId id)
: data_(data)
, id_(id) { }

void BodyAggregateLiteral::printPlain(PrintPlain out) const {
    auto &dom = data_.getDom<BodyAggregateDomain>(id_.domain());
    auto &atm = dom[id_.offset()];
    if (atm.defined()) {
        auto bounds = atm.plainBounds();
        out << id_.sign();
        auto it = bounds.begin(), ie = bounds.end();
        if (it != ie) { out << it->second << inv(it->first); ++it; }
        out << atm.fun() << "{";
        print_comma(out, atm.elems(), ";", printBodyElem);
        out << "}";
        for (; it != ie; ++it) { out << it->first << it->second; }
    }
    else {
        out << (id_.sign() == NAF::NOT ? "#true" : "#false");
    }
}

LiteralId BodyAggregateLiteral::toId() const {
    return id_;
}

LiteralId BodyAggregateLiteral::translate(Translator &x) {
    auto &dom = data_.getDom<BodyAggregateDomain>(id_.domain());
    auto &atm = dom[id_.offset()];
    if (!atm.translated()) {
        atm.setTranslated();
        if (atm.defined()) {
            auto aggrLit = getEqualAggregate(data_, x, atm.fun(), id_.sign(), atm.bounds(), atm.range(), atm.elems(), atm.recursive());
            auto lit = atm.lit();
            if (lit) { Rule().addHead(lit).addBody(aggrLit).translate(data_, x); }
            else { atm.setLit(aggrLit); }
        }
        else { makeFalse(data_, x, id_.sign(), atm); }
    }
    return atm.lit();
}
int BodyAggregateLiteral::uid() const {
    throw std::logic_error("BodyAggregateLiteral::uid must be called after BodyAggregateLiteral::translate");
}

bool BodyAggregateLiteral::isIncomplete() const {
    return data_.getAtom<BodyAggregateDomain>(id_.domain(), id_.offset()).recursive();
}

std::pair<LiteralId,bool> BodyAggregateLiteral::delayedLit() {
    auto &atm = data_.getAtom<BodyAggregateDomain>(id_.domain(), id_.offset());
    bool found = atm.lit();
    if (!found) { atm.setLit(data_.newDelayed()); }
    return {atm.lit(), !found};
}

BodyAggregateLiteral::~BodyAggregateLiteral() noexcept = default;

// {{{1 definition of AssignmentAggregateLiteral

AssignmentAggregateLiteral::AssignmentAggregateLiteral(DomainData &data, LiteralId id)
: data_(data)
, id_(id) {
    assert(id_.sign() == NAF::POS);
}

AssignmentAggregateDomain &AssignmentAggregateLiteral::dom() const {
    return data_.getDom<AssignmentAggregateDomain>(id_.domain());
}

void AssignmentAggregateLiteral::printPlain(PrintPlain out) const {
    auto &dom = this->dom();
    auto &atm = dom[id_.offset()];
    auto &data = dom.data(atm.data());
    Value repr = atm;
    out << id_.sign();
    out << data.fun() << "{";
    print_comma(out, data.elems(), ";", printBodyElem);
    out << "}=" << repr.args().back();
}

LiteralId AssignmentAggregateLiteral::toId() const {
    return id_;
}

LiteralId AssignmentAggregateLiteral::translate(Translator &x) {
    auto &dom = this->dom();
    auto &atm = dom[id_.offset()];
    auto &data = dom.data(atm.data());
    if (!atm.translated()) {
        atm.setTranslated();
        assert(atm.defined());
        // NOTE: for assignment aggregates with many values a better translation could be implemented
        Value repr = atm;
        DisjunctiveBounds bounds;
        bounds.add(repr.args().back(), true, repr.args().back(), true);
        auto aggrLit = getEqualAggregate(data_, x, data.fun(), id_.sign(), bounds, data.range(), data.elems(), atm.recursive());
        auto lit = atm.lit();
        if (lit) { Rule().addHead(lit).addBody(aggrLit).translate(data_, x); }
        else { atm.setLit(aggrLit); }
    }
    return atm.lit();
}
int AssignmentAggregateLiteral::uid() const {
    throw std::logic_error("AssignmentAggregateLiteral::uid must be called after AssignmentAggregateLiteral::translate");
}

bool AssignmentAggregateLiteral::isIncomplete() const {
    return dom()[id_.offset()].recursive();
}

std::pair<LiteralId,bool> AssignmentAggregateLiteral::delayedLit() {
    auto &atm = data_.getAtom<AssignmentAggregateDomain>(id_.domain(), id_.offset());
    bool found = atm.lit();
    if (!found) { atm.setLit(data_.newDelayed()); }
    return {atm.lit(), !found};
}

AssignmentAggregateLiteral::~AssignmentAggregateLiteral() noexcept = default;

// {{{1 definition of DisjunctionLiteral

DisjunctionLiteral::DisjunctionLiteral(DomainData &data, LiteralId id)
: data_(data)
, id_(id) { }

LiteralId DisjunctionLiteral::toId() const {
    return id_;
}

void DisjunctionLiteral::printPlain(PrintPlain out) const {
    auto &atm = dom()[id_.offset()];
    if (!atm.elems().empty()) { print_comma(out, atm.elems(), ";"); }
    else { out << "#false"; }
}

bool DisjunctionLiteral::isBound(Value &value, bool negate) const {
    assert(!negate);
    auto &atm = dom()[id_.offset()];
    for (auto &y : atm.elems()) {
        if (y.bodies().size() != 1 && y.bodies().front().second > 0) { return false; }
        for (auto &z : y.heads()) {
            if (z.second != 1) { return false; }
            for (auto &u : data_.clause(z)) {
                if (!call(data_, u, &Literal::isBound, value, negate)) { return false; }
            }
        }
    }
    return true;
}

void DisjunctionLiteral::updateBound(std::vector<CSPBound> &bounds, bool negate) const {
    assert(!negate);
    auto &atm = dom()[id_.offset()];
    for (auto &y : atm.elems()) {
        for (auto &z : y.heads()) {
            for (auto &u : data_.clause(z)) {
                call(data_, u, &Literal::updateBound, bounds, negate);
            }
        }
    }
}

LiteralId DisjunctionLiteral::translate(Translator &x) {
    auto &atm = dom()[id_.offset()];
    if (!atm.translated()) {
        atm.setTranslated();
        if (!atm.lit()) { atm.setLit(data_.newAux()); }
        Value value;
        if (atm.fact() && isBound(value, false) && value.type() != Value::SPECIAL) {
            std::vector<CSPBound> bounds;
            updateBound(bounds, false);
            x.addBounds(value, bounds);
            return atm.lit();
        }
        bool headFact;
        atm.simplify(headFact);
        if (headFact) { return atm.lit(); }
        Rule dj;
        dj.addBody(atm.lit());
        for (auto &elem : atm.elems()) {
            assert (!elem.bodies().empty());
            LiteralId cond; // cond :- elem.bodies()
            if (!elem.bodyIsTrue()) { cond = getEqualFormula(data_, x, elem.bodies(), false, false); }
            if (elem.heads().empty()) {
                assert(cond);
                // head is a fact
                dj.addBody(cond.negate());
            }
            else if (elem.heads().size() == 1) {
                auto headClause = data_.clause(elem.heads().front());
                if (cond) {
                    LiteralId headAtom = data_.newAux();
                    for (auto &lit : headClause) {
                        Rule().addHead(headAtom).addBody(lit).addBody(cond).translate(data_, x);
                    }
                    Rule().addHead(headClause).addBody(cond).addBody(headAtom).translate(data_, x);
                    Rule().addBody(cond.negate()).addBody(headAtom).translate(data_, x);
                    dj.addHead(headAtom);
                }
                else {
                    // the head literals can be pushed directly into djHead
                    dj.addHead(headClause.front());
                }
            }
            else {
                LiteralId conjunctionAtom = data_.newAux();
                Rule conjunction;
                for (auto &headId : elem.heads()) {
                    auto head = data_.clause(headId);
                    if (head.size() == 1) {
                        Rule().addHead(head.front()).addBody(conjunctionAtom).translate(data_, x);
                        conjunction.addBody(head.front());
                    }
                    else {
                        LiteralId disjunctionAtom = data_.newAux();
                        Rule().addHead(head).addBody(disjunctionAtom).translate(data_, x);
                        for (auto &lit : head) {
                            Rule().addHead(disjunctionAtom).addBody(lit).translate(data_, x);
                        }
                        conjunction.addBody(disjunctionAtom);
                    }
                }
                conjunction.addHead(conjunctionAtom).translate(data_, x);
                if (cond) {
                    LiteralId headAtom = data_.newAux();
                    Rule().addHead(headAtom).addBody(conjunctionAtom).addBody(cond).translate(data_, x);
                    Rule().addHead(conjunctionAtom).addBody(headAtom).addBody(cond).translate(data_, x);
                    Rule().addBody(cond.negate()).addBody(headAtom).translate(data_, x);
                    dj.addHead(headAtom);
                }
                else {
                    dj.addHead(conjunctionAtom);
                }
            }
        }
        dj.translate(data_, x);
    }
    return atm.lit();
}

Lit_t DisjunctionLiteral::uid() const {
    throw std::logic_error("DisjunctionLiteral::uid: must be called before DisjunctionLiteral::translate");
}

bool DisjunctionLiteral::isIncomplete() const {
    return dom()[id_.offset()].recursive();
}

DisjunctionDomain &DisjunctionLiteral::dom() const {
    return data_.getDom<DisjunctionDomain>(id_.domain());
}

std::pair<LiteralId,bool> DisjunctionLiteral::delayedLit() {
    auto &atm = data_.getAtom<DisjunctionDomain>(id_.domain(), id_.offset());
    bool found = atm.lit();
    if (!found) { atm.setLit(data_.newDelayed()); }
    return {atm.lit(), !found};
}

DisjunctionLiteral::~DisjunctionLiteral() noexcept = default;

// {{{1 definition of ConjunctionLiteral

ConjunctionLiteral::ConjunctionLiteral(DomainData &data, LiteralId id)
: data_(data)
, id_(id) { }

LiteralId ConjunctionLiteral::toId() const {
    return id_;
}

void ConjunctionLiteral::printPlain(PrintPlain out) const {
    auto &atm = dom()[id_.offset()];
    if (!atm.elems().empty()) {
        int sep = 0;
        for (auto &x : atm.elems()) {
            switch (sep) {
                case 1: { out << ","; break; }
                case 2: { out << ";"; break; }
            }
            out << x;
            sep = static_cast<bool>(x.needsSemicolon()) + 1;
        }
    }
    else { out << "#true"; }
}

LiteralId ConjunctionLiteral::translate(Translator &x) {
    auto &atm = dom()[id_.offset()];
    if (!atm.translated()) {
        atm.setTranslated();
        LitVec bd;
        for (auto &y : atm.elems()) {
            if ((y.heads().size() == 1 && y.heads().front().second == 0) || y.bodies().empty()) {
                // this part of the conditional literal is a fact
                continue;
            }
            else if (y.isSimple(data_)) {
                if (y.bodies().size() == 1 && y.bodies().front().second == 0) {
                    assert(y.heads().size() <= 1);
                    if (y.heads().empty()) {
                        if (!atm.lit()) { atm.setLit(data_.newAux()); }
                        return atm.lit();
                    }
                    else { bd.emplace_back(data_.clause(y.heads().front()).front()); }
                }
                else { bd.emplace_back(data_.clause(y.bodies().front()).front().negate()); }
            }
            else {
                LiteralId aux = data_.newAux();
                LiteralId auxHead = data_.newAux();
                for (auto &head : y.heads()) {
                    // auxHead :- y.head.
                    Rule().addHead(auxHead).addBody(data_.clause(head)).translate(data_, x);
                    Rule().addHead(aux).addBody(auxHead).translate(data_, x);
                }
                // chk <=> body.
                LiteralId chk = getEqualFormula(data_, x, y.bodies(), false, atm.nonmonotone() && !y.heads().empty());
                Rule().addHead(aux).addBody(chk.negate()).translate(data_, x);
                if (atm.nonmonotone() && !y.heads().empty()) {
                    // aux | chk | ~x.first.
                    Rule().addHead(aux).addHead(chk).addHead(auxHead.negate()).translate(data_, x);
                }
                // body += aux
                bd.emplace_back(aux);
            }
        }
        // bdLit :- body.
        if (bd.size() == 1 && !atm.lit()) {
            bd.front() = call(data_, bd.front(), &Literal::translate, x);
            atm.setLit(bd.front());
        }
        else {
            if (!atm.lit()) { atm.setLit(data_.newAux()); }
            Rule().addHead(atm.lit()).addBody(bd).translate(data_, x);
        }
    }
    return atm.lit();
}

Lit_t ConjunctionLiteral::uid() const {
    throw std::logic_error("ConjunctionLiteral::uid: must be called before ConjunctionLiteral::translate");
}

bool ConjunctionLiteral::isIncomplete() const {
    return dom()[id_.offset()].recursive();
}

ConjunctionDomain &ConjunctionLiteral::dom() const {
    return data_.getDom<ConjunctionDomain>(id_.domain());
}

std::pair<LiteralId,bool> ConjunctionLiteral::delayedLit() {
    auto &atm = data_.getAtom<ConjunctionDomain>(id_.domain(), id_.offset());
    bool found = atm.lit();
    if (!found) { atm.setLit(data_.newDelayed()); }
    return {atm.lit(), !found};
}

bool ConjunctionLiteral::needsSemicolon() const {
    auto &atm = dom()[id_.offset()];
    return !atm.elems().empty() && atm.elems().back().needsSemicolon();
}

ConjunctionLiteral::~ConjunctionLiteral() noexcept = default;

// {{{1 definition of DisjointLiteral

DisjointLiteral::DisjointLiteral(DomainData &data, LiteralId id)
: data_(data)
, id_(id) { }

DisjointDomain &DisjointLiteral::dom() const {
    return data_.getDom<DisjointDomain>(id_.domain());
}

void DisjointLiteral::printPlain(PrintPlain out) const {
    auto &atm = data_.getAtom<DisjointDomain>(id_.domain(), id_.offset());
    if (atm.defined()) {
        auto print_elems = [](PrintPlain out, DisjointElemSet::ValueType const &x) {
            print_comma(out, x.first, ",");
            out << ":";
            using namespace std::placeholders;
            print_comma(out, x.second, ",", std::bind(&DisjointElement::printPlain, _2, _1));
        };
        out << id_.sign();
        out << "#disjoint{";
        print_comma(out, atm.elems(), ";", print_elems);
        out << "}";
    }
    else {
        out << (id_.sign() == NAF::NOT ? "#true" : "#false");
    }
}

LiteralId DisjointLiteral::toId() const {
    return id_;
}

LiteralId DisjointLiteral::translate(Translator &x) {
    auto &atm = data_.getAtom<DisjointDomain>(id_.domain(), id_.offset());
    if (!atm.translated()) {
        atm.setTranslated();
        if (atm.defined()) {
            if (!atm.lit()) { atm.setLit(data_.newAux()); }
            x.addDisjointConstraint(data_, id_);
        }
        else { makeFalse(data_, x, id_.sign(), atm); }
    }
    return id_.sign() != NAF::NOT ? atm.lit() : atm.lit().negate();
}

int DisjointLiteral::uid() const {
    throw std::logic_error("DisjointLiteral::uid must be called after DisjointLiteral::translate");
}

bool DisjointLiteral::isIncomplete() const {
    return data_.getAtom<DisjointDomain>(id_.domain(), id_.offset()).recursive();
}

std::pair<LiteralId,bool> DisjointLiteral::delayedLit() {
    auto &atm = data_.getAtom<DisjointDomain>(id_.domain(), id_.offset());
    bool found = atm.lit();
    if (!found) { atm.setLit(data_.newDelayed()); }
    return {id_.sign() != NAF::NOT ? atm.lit() : atm.lit().negate(), !found};
}

DisjointLiteral::~DisjointLiteral() noexcept = default;

// {{{1 definition of HeadAggregateLiteral

HeadAggregateLiteral::HeadAggregateLiteral(DomainData &data, LiteralId id)
: data_(data)
, id_(id) { }

LiteralId HeadAggregateLiteral::toId() const {
    return id_;
}

LiteralId HeadAggregateLiteral::translate(Translator &x) {
    auto &atm = dom()[id_.offset()];
    if (!atm.translated()) {
        atm.setTranslated();
        if (!atm.lit()) { atm.setLit(data_.newAux()); }
        auto range(atm.range());
        if (!atm.satisfiable()) {
            Rule().addBody(atm.lit()).translate(data_, x);
            return atm.lit();
        }
        using GroupedByCond = std::vector<std::pair<ClauseId, std::pair<FWValVec, LiteralId>>>;
        GroupedByCond groupedByCond;
        for (auto &y : atm.elems()) {
            for (auto &z : y.second) { groupedByCond.emplace_back(z.second, std::make_pair(y.first, z.first)); }
        }
        sort_unique(groupedByCond);
        BodyAggregateElements bdElems;
        for (auto it = groupedByCond.begin(), ie = groupedByCond.end(); it != ie; ) {
            auto condId = it->first;
            LiteralId condLit;
            if (condId.second > 0) {
                // c :- C.
                condLit = getEqualClause(data_, x, condId, true, false);
            }
            Rule choice(true);
            do {
                // { heads } :- c, b.
                LiteralId head = it->second.second;
                if (head) { choice.addHead(head); }
                // e :- h, c.
                auto bdElem = bdElems.push(std::piecewise_construct, std::forward_as_tuple(it->second.first), std::forward_as_tuple());
                LitVec cond;
                if (condLit) { cond.emplace_back(condLit); }
                if (head)    { cond.emplace_back(head); }
                bdElem.first->second.emplace_back(data_.clause(cond));
                ++it;
            }
            while (it != ie && it->first == condId);
            if (choice.numHeads() > 0) {
                choice.addBody(atm.lit());
                if (condLit) { choice.addBody(condLit); }
                choice.translate(data_, x);
            }
        }
        // :- b, not aggr.
        if (!atm.bounds().contains(range)) {
            LiteralId aggr = getEqualAggregate(data_, x, atm.fun(), NAF::NOT, atm.bounds(), range, bdElems, false);
            Rule check;
            check.addBody(atm.lit());
            check.addBody(aggr).translate(data_, x);
        }
    }
    return atm.lit();
}

void HeadAggregateLiteral::printPlain(PrintPlain out) const {
    auto &atm = dom()[id_.offset()];
    auto bounds = atm.plainBounds();
    out << id_.sign();
    auto it = bounds.begin(), ie = bounds.end();
    if (it != ie) { out << it->second << inv(it->first); ++it; }
    out << atm.fun() << "{";
    print_comma(out, atm.elems(), ";", printHeadElem);
    out << "}";
    for (; it != ie; ++it) { out << it->first << it->second; }
}

bool HeadAggregateLiteral::isIncomplete() const {
    return true;
}

HeadAggregateDomain &HeadAggregateLiteral::dom() const {
    return data_.getDom<HeadAggregateDomain>(id_.domain());
}

int HeadAggregateLiteral::uid() const {
    throw std::logic_error("HeadAggregateLiteral::uid must be called after BodyAggregateLiteral::translate");
}

std::pair<LiteralId,bool> HeadAggregateLiteral::delayedLit() {
    auto &atm = data_.getAtom<HeadAggregateDomain>(id_.domain(), id_.offset());
    bool found = atm.lit();
    if (!found) { atm.setLit(data_.newDelayed()); }
    return {atm.lit(), !found};
}

HeadAggregateLiteral::~HeadAggregateLiteral() noexcept = default;

// {{{1 definition of DomainData

Gringo::TheoryData::TermType DomainData::termType(Id_t value) const {
    auto &term = theory_.data().getTerm(value);
    switch (term.type()) {
        case Potassco::Theory_t::Number: { return Gringo::TheoryData::TermType::Number; }
        case Potassco::Theory_t::Symbol: { return Gringo::TheoryData::TermType::Symbol; }
        case Potassco::Theory_t::Compound: {
            if (term.isFunction()) { return Gringo::TheoryData::TermType::Function; }
            switch (term.tuple()) {
                case Potassco::Tuple_t::Paren:   { return Gringo::TheoryData::TermType::Tuple; }
                case Potassco::Tuple_t::Bracket: { return Gringo::TheoryData::TermType::List; }
                case Potassco::Tuple_t::Brace:   { return Gringo::TheoryData::TermType::Set; }
            }
            return Gringo::TheoryData::TermType::Number;
        }
    }
    throw std::logic_error("must not happen");
}

int DomainData::termNum(Id_t value) const {
    return theory_.data().getTerm(value).number();
}

char const *DomainData::termName(Id_t value) const {
    if (theory_.data().getTerm(value).isFunction()) {
        return theory_.data().getTerm(theory_.data().getTerm(value).function()).symbol();
    }
    return theory_.data().getTerm(value).symbol();
}

Potassco::IdSpan DomainData::termArgs(Id_t value) const {
    return theory_.data().getTerm(value).terms();
}

Potassco::IdSpan DomainData::elemTuple(Id_t value) const {
    return theory_.data().getElement(value).terms();
}

Potassco::LitSpan DomainData::elemCond(Id_t value) const {
    auto &data = const_cast<DomainData&>(*this);
    data.tempLits_.clear();
    for (auto &lit : theory_.getCondition(value)) {
        data.tempLits_.emplace_back(call(data, lit, &Literal::uid));
    }
    return Potassco::toSpan(data.tempLits_);
}

Potassco::Lit_t DomainData::elemCondLit(Id_t value) const {
    return theory_.data().getElement(value).condition();
}

Potassco::IdSpan DomainData::atomElems(Id_t value) const {
    return theory_.getAtom(value).elements();
}

Potassco::Id_t DomainData::atomTerm(Id_t value) const {
    return theory_.getAtom(value).term();
}

bool DomainData::atomHasGuard(Id_t value) const {
    return theory_.getAtom(value).guard();
}

Potassco::Lit_t DomainData::atomLit(Id_t value) const {
    return theory_.getAtom(value).atom();
}

std::pair<char const *, Id_t> DomainData::atomGuard(Id_t value) const {
    return {termName(*theory_.getAtom(value).guard()), *theory_.getAtom(value).rhs()};
}

Potassco::Id_t DomainData::numAtoms() const {
    return theory_.data().numAtoms();
}

std::string DomainData::termStr(Id_t value) const {
    std::ostringstream oss;
    theory_.printTerm(oss, value);
    return oss.str();
}

std::string DomainData::elemStr(Id_t value) const {
    std::ostringstream oss;
    auto &data = const_cast<DomainData&>(*this);
    theory_.printElem(oss, value, [&data](std::ostream &out, LiteralId lit) { call(data, lit, &Literal::printPlain, PrintPlain{data, out}); });
    return oss.str();
}

std::string DomainData::atomStr(Id_t value) const {
    std::ostringstream oss;
    oss << "&";
    auto &atom = theory_.getAtom(value);
    theory_.printTerm(oss, atom.term());
    oss << "{";
    bool comma = false;
    for (auto &elem : atom.elements()) {
        if (comma) { oss << ";"; }
        else       { comma = true; }
        auto &data = const_cast<DomainData&>(*this);
        theory_.printElem(oss, elem, [&data](std::ostream &out, LiteralId lit) { call(data, lit, &Literal::printPlain, PrintPlain{data, out}); });
    }
    oss << "}";
    if (atom.guard()) {
        theory_.printTerm(oss, *atom.guard());
        theory_.printTerm(oss, *atom.rhs());
    }
    return oss.str();

}

// }}}1

} } // namespace Output Gringo
