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
#include <gringo/output/lparseoutputter.hh>
#include <gringo/logger.hh>
#include <gringo/control.hh>
#include <gringo/output/aggregates.hh>

namespace Gringo { namespace Output {

// {{{ definition of AuxLiteral

AuxAtom::AuxAtom(unsigned name) : name(name) { }
int AuxAtom::lparseUid(LparseOutputter &out) {
    if (!uid) { uid = out.newUid(); }
    return uid;
}

std::ostream &operator<<(std::ostream &out, AuxAtom const &x) { 
    out << "#aux(" << x.name << ")";
    return out;
}

ULit AuxLiteral::negateLit(LparseTranslator &x) const { 
    ULit lit(gringo_make_unique<AuxLiteral>(atom, inv(naf)));
    Term::replace(lit, lit->toLparse(x));
    return lit;
}

AuxLiteral::AuxLiteral(SAuxAtom atom, NAF naf) : atom(atom), naf(naf) { }

AuxLiteral *AuxLiteral::clone() const { return new AuxLiteral(*this); }

SAuxAtom AuxLiteral::isAuxAtom() const { return naf == NAF::POS ? atom : nullptr; }

ULit AuxLiteral::toLparse(LparseTranslator &trans) { 
    if (naf == NAF::NOTNOT) {
        ULit aux = trans.makeAux();
        LRC().addHead(aux).addBody(negateLit(trans)).toLparse(trans);
        return aux->negateLit(trans);
    }
    return nullptr; 
}

void AuxLiteral::printPlain(std::ostream &out) const { out << naf << *atom; }

bool AuxLiteral::isIncomplete() const { return false; }

int AuxLiteral::lparseUid(LparseOutputter &out) const { 
    switch (naf) {
        case NAF::POS:    { return +atom->lparseUid(out); }
        case NAF::NOT:    { return -atom->lparseUid(out); }
        case NAF::NOTNOT: { throw std::runtime_error("AuxLiteral::lparseUid: toLparse must be called before!"); }
    }
    throw std::logic_error("AuxLiteral::lparseUid: must not happen");
}

size_t AuxLiteral::hash() const { return get_value_hash(typeid(AuxLiteral).hash_code(), atom->name); }

bool AuxLiteral::operator==(Literal const &x) const { 
    AuxLiteral const *t{dynamic_cast<AuxLiteral const*>(&x)};
    return naf == t->naf && atom->name == t->atom->name;
}

bool AuxLiteral::invertible() const { return naf != NAF::POS; }

void AuxLiteral::invert() { naf = inv(naf); }

AuxLiteral::~AuxLiteral() { }

// }}}
// {{{ definition of BooleanLiteral

BooleanLiteral::BooleanLiteral(bool value) : value(value)         { }
BooleanLiteral *BooleanLiteral::clone() const                     { return new BooleanLiteral(*this); }
BooleanLiteral::~BooleanLiteral()                                 { }
ULit BooleanLiteral::toLparse(LparseTranslator &)                 { return nullptr; }
void BooleanLiteral::printPlain(std::ostream &out) const          { out << (value ? "#true" : "#false"); }
bool BooleanLiteral::isIncomplete() const                         { return false; }
int BooleanLiteral::lparseUid(LparseOutputter &out) const         { return value ? -out.falseUid() : out.falseUid(); }
size_t BooleanLiteral::hash() const                               { return get_value_hash(typeid(BooleanLiteral).hash_code(), value); }
bool BooleanLiteral::operator==(Literal const &x) const           { 
    BooleanLiteral const *t{dynamic_cast<BooleanLiteral const*>(&x)};
    return value == t->value;
}

// }}}
// {{{ definition of PredicateLiteral

PredicateLiteral::PredicateLiteral() = default;
PredicateLiteral::PredicateLiteral(NAF naf, PredicateDomain::element_type &repr) : naf(naf), repr(&repr) { }
PredicateDomain::element_type *PredicateLiteral::isAtom() const {
    return naf == NAF::POS ? repr : nullptr;
}
void PredicateLiteral::printPlain(std::ostream &out) const {
    out << naf << repr->first;
}
bool PredicateLiteral::isIncomplete() const         { return false; }
PredicateLiteral *PredicateLiteral::clone() const   { return new PredicateLiteral(*this); }
ULit PredicateLiteral::toLparse(LparseTranslator &trans) { 
    if (naf == NAF::NOTNOT) {
        ULit aux = trans.makeAux();
        LRC().addHead(aux).addBody(gringo_make_unique<PredicateLiteral>(NAF::NOT, *repr)).toLparse(trans);
        return aux->negateLit(trans);
    }
    return nullptr;

}
ULit PredicateLiteral::negateLit(LparseTranslator &x) const {
    ULit lit(gringo_make_unique<PredicateLiteral>(inv(naf), *repr));
    Term::replace(lit, lit->toLparse(x));
    return lit;
}
int PredicateLiteral::lparseUid(LparseOutputter &out) const {
    if (!repr->second.hasUid()) { repr->second.uid(out.newUid()); }
    switch (naf) {
        case NAF::POS:    { return +repr->second.uid(); }
        case NAF::NOT:    { return -int(repr->second.uid()); }
        case NAF::NOTNOT: { throw std::runtime_error("PredicateLiteral::lparseUid: toLparse must be called before!"); }
    }
    assert(false && "cannot happen");
    return 0;
}
bool PredicateLiteral::invertible() const {
    return naf != NAF::POS;
}
void PredicateLiteral::invert() {
    naf = inv(naf);
}
size_t PredicateLiteral::hash() const { return get_value_hash(typeid(PredicateLiteral).hash_code(), naf, repr->first); }
bool PredicateLiteral::operator==(Literal const &x) const { 
    PredicateLiteral const *t{dynamic_cast<PredicateLiteral const*>(&x)};
    return naf == t->naf && repr == t->repr;
}
std::pair<bool, TruthValue> PredicateLiteral::getTruth(AssignmentLookup assignment) {
    if (repr->second.hasUid()) {
        auto value = assignment(repr->second.uid());
        if (naf == NAF::NOT) {
            switch (value.second) {
                case TruthValue::True: {
                    value.second = TruthValue::False;
                    break;
                }
                case TruthValue::False: {
                    value.second = TruthValue::True;
                    break;
                }
                default: { break; }
            }
        }
        return value;
    }
    return {false, TruthValue::Open};
}
PredicateLiteral::~PredicateLiteral() { }

// }}}
// {{{ definition of BodyAggregateState

int clamp(int64_t x) {
    if (x > std::numeric_limits<int>::max()) { return std::numeric_limits<int>::max(); }
    if (x < std::numeric_limits<int>::min()) { return std::numeric_limits<int>::min(); }
    return int(x);
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

bool BodyAggregateState::fact(bool recursive) const { return _fact && (_positive || !recursive); }
unsigned BodyAggregateState::generation() const { return _generation; }
bool BodyAggregateState::isFalse() { return !_defined; }
BodyAggregateState::element_type &BodyAggregateState::ignore() {
    static element_type x{std::piecewise_construct, std::forward_as_tuple(Value::createId("#false")), std::forward_as_tuple()};
    return x;
}
void BodyAggregateState::accumulate(ValVec const &tuple, AggregateFunction fun, bool fact, bool remove) {
    switch (fun) {
        case AggregateFunction::MIN: {
            Value val = tuple.front();
            if (fact) { valMax = std::min<Value>(valMax, val); }
            valMin = std::min<Value>(valMin, val);
            break;
        }
        case AggregateFunction::MAX: {
            Value val = tuple.front();
            if (fact) { valMin = std::max<Value>(valMin, val); }
            valMax = std::max<Value>(valMax, val);
            break;
        }
        default: {
            int val = fun == AggregateFunction::COUNT ? 1 : tuple.front().num();
            if (fact) {
                if (remove) {
                    if (val < 0) { intMax+= val; }
                    else         { intMin+= val; }
                }
                else {
                    intMin+= val;
                    intMax+= val;
                }
            }
            else {
                if (val < 0) { intMin+= val; }
                else         { intMax+= val; }
            }
            break;
        }
    }
}
void BodyAggregateState::init(AggregateFunction fun) {
    switch (fun) {
        case AggregateFunction::MIN: {
            valMin = Value::createSup();
            valMax = Value::createSup();
            break;
        }
        case AggregateFunction::MAX: {
            valMin = Value::createInf();
            valMax = Value::createInf();
            break;
        }
        default: {
            intMin = 0;
            intMax = 0;
            break;
        }
    }
}
bool BodyAggregateState::defined() const { return _defined; }
void BodyAggregateState::generation(unsigned x) { _generation = x; }
BodyAggregateState::Bounds::Interval BodyAggregateState::range(AggregateFunction fun) const { 
    if (fun != AggregateFunction::MIN && fun != AggregateFunction::MAX) {
        return {{Value::createNum(clamp(intMin)), true}, {Value::createNum(clamp(intMax)), true}};
    }
    else { return {{valMin, true}, {valMax, true}}; }
}
BodyAggregateState::~BodyAggregateState() { }

// }}}
// {{{ definition of BodyAggregate

BodyAggregate::BodyAggregate() { }

namespace {

void print_elem(std::ostream &out, BdAggrElemSet::value_type const &x) {
    if (x.second.empty()) { print_comma(out, x.first, ","); }
    else {
        auto print_cond = [&x](std::ostream &out, BdAggrElemSet::value_type::second_type::value_type const &y) -> void { 
            print_comma(out, x.first, ",");
            out << ":";
            using namespace std::placeholders;
            print_comma(out, y, ",", std::bind(&Literal::printPlain, _2, _1));
        };
        print_comma(out, x.second, ";", print_cond);
    }
}

} // namespace

void BodyAggregate::printPlain(std::ostream &out) const {
    out << naf;
    auto it = bounds.begin(), ie = bounds.end();
    if (it != ie) { out << it->second << inv(it->first); ++it; }
    out << fun << "{";
    print_comma(out, repr->second.elems, ";", print_elem);
    out << "}";
    for (; it != ie; ++it) { out << it->first << it->second; }

}

ULit BodyAggregate::toLparse(LparseTranslator &x) {
    if (repr->second.defined()) {
        return getEqualAggregate(x, fun, naf, repr->second.bounds, repr->second.range(fun), repr->second.elems, incomplete);
    }
    else {
        switch (naf) {
            case NAF::POS:    return x.getTrueLit()->negateLit(x);
            case NAF::NOT:    return x.getTrueLit();
            case NAF::NOTNOT: return x.getTrueLit()->negateLit(x);
        }
    }
    throw std::logic_error("BodyAggregate::toLparse: must not happen");
}
int BodyAggregate::lparseUid(LparseOutputter &) const {
    throw std::runtime_error("BodyAggregate::lparseUid must be called after BodyAggregate::toLparse");
}
bool BodyAggregate::isIncomplete() const { return incomplete; }
BodyAggregate *BodyAggregate::clone() const { return new BodyAggregate(*this); }
size_t BodyAggregate::hash() const { throw std::runtime_error("BodyAggregate::hash: implement me if necessary!"); }
bool BodyAggregate::operator==(Literal const &) const { throw std::runtime_error("BodyAggregate::operator==: implement me if necessary!"); }
BodyAggregate::~BodyAggregate() { }

// }}}
// {{{ definition of AssignmentAggregateState

AssignmentAggregateState::AssignmentAggregateState(Data *data, unsigned generation)
    : data(data)
    , _generation(generation)   { }
bool AssignmentAggregateState::fact(bool recursive) const { return data->fact && !recursive; }
unsigned AssignmentAggregateState::generation() const     { return _generation; }
void AssignmentAggregateState::generation(unsigned x)     { _generation = x; }
bool AssignmentAggregateState::isFalse()                  { return data; }
AssignmentAggregateState::element_type &AssignmentAggregateState::ignore()   { throw std::logic_error("AssignmentAggregateState::ignore must not be called"); }
bool AssignmentAggregateState::defined() const            { return true; }

// }}}
// {{{ definition of AssignmentAggregate

namespace {

Interval getRange(AggregateFunction fun, BdAggrElemSet const &elems) {
    switch (fun) {
        case AggregateFunction::MIN: {
            Value valMin = Value::createSup();
            Value valMax = valMin;
            for (auto &x : elems) {
                if (!x.second.empty()) {
                    Value val = x.first.front();
                    if (x.second.front().empty()) { valMax = std::min<Value>(valMax, val); }
                    valMin = std::min<Value>(valMin, val);
                }
            }
            return Interval{{valMin,true},{valMax,true}};
        }
        case AggregateFunction::MAX: {
            Value valMin = Value::createInf();
            Value valMax = valMin;
            for (auto &x : elems) {
                if (!x.second.empty()) {
                    Value val = x.first.front();
                    if (x.second.front().empty()) { valMin = std::max<Value>(valMin, val); }
                    valMax = std::max<Value>(valMax, val);
                }
            }
            return Interval{{valMin,true},{valMax,true}};
        }
        default: {
            int64_t intMin = 0;
            int64_t intMax = intMin;
            for (auto &x : elems) {
                if (!x.second.empty()) {
                    int val = fun == AggregateFunction::COUNT ? 1 : x.first.front().num();
                    if (x.second.front().empty()) { 
                        intMin+= val;
                        intMax+= val;
                    }
                    else {
                        if (val < 0) { intMin+= val; }
                        else         { intMax+= val; }
                    }
                }
            }
            // NOTE: nonsense, proper handling is not achieved like this...
            return {{Value::createNum(clamp(intMin)), true}, {Value::createNum(clamp(intMax)), true}};
        }
    }
}

} // namespace

AssignmentAggregate::AssignmentAggregate() { }
void AssignmentAggregate::printPlain(std::ostream &out) const {
    out << *(repr->first.args().end()-1) << "=" << fun << "{";
    print_comma(out, repr->second.data->elems, ";", print_elem);
    out << "}";
}

ULit AssignmentAggregate::toLparse(LparseTranslator &x) {
    Value assign(repr->first.args().back());
    DisjunctiveBounds bounds;
    bounds.add({{assign,true},{assign,true}});
    Interval range = getRange(fun, repr->second.data->elems);
    return getEqualAggregate(x, fun, NAF::POS, bounds, range, repr->second.data->elems, incomplete);
}
int AssignmentAggregate::lparseUid(LparseOutputter &) const            { throw std::runtime_error("AssignmentAggregate::lparseUid must be called after AssignmentAggregate::toLparse"); }
bool AssignmentAggregate::isIncomplete() const                         { return incomplete; }
AssignmentAggregate *AssignmentAggregate::clone() const                { return new AssignmentAggregate(*this); }
size_t AssignmentAggregate::hash() const                               { throw std::runtime_error("AssignmentAggregate::hash: implement me if necessary!"); }
bool AssignmentAggregate::operator==(Literal const &) const            { throw std::runtime_error("AssignmentAggregate::operator==: implement me if necessary!"); }
AssignmentAggregate::~AssignmentAggregate()                            { }

// }}}
// {{{ definition of ConjunctionElem

ConjunctionElem::ConjunctionElem(Value repr)
: repr(repr) { }

ConjunctionElem::operator Value const & () const {
    return repr;
}

bool ConjunctionElem::isSimple() const {
    return (heads.empty() && bodies.size() == 1 && bodies.front().size() == 1 && bodies.front().front()->invertible()) || 
           (bodies.size() == 1 && bodies.front().empty() && heads.size() <= 1);
}

bool ConjunctionElem::needsSemicolon() const {
    return !bodies.empty() && !bodies.front().empty();
}

void ConjunctionElem::print(std::ostream &out) const {
    using namespace std::placeholders;
    if (bodies.empty()) {
        out << "#true";
    }
    else {
        if (heads.empty()) {
            out << "#false";
        }
        else {
            auto print_conjunction = [](std::ostream &out, ULitVec const &lits) {
                if (lits.empty()) {
                    out << "#true";
                }
                else {
                    print_comma(out, lits, "&", std::bind(&Literal::printPlain, _2, _1));
                }
            };
            print_comma(out, heads, "|", print_conjunction);
        }
        if (!bodies.front().empty()) {
            out << ":";
            auto print_conjunction = [](std::ostream &out, ULitVec const &lits) {
                if (lits.empty()) {
                    out << "#true";
                }
                else {
                    print_comma(out, lits, ",", std::bind(&Literal::printPlain, _2, _1));
                }
            };
            print_comma(out, bodies, "|", print_conjunction);
        }
    }
}

// }}}
// {{{ definition of ConjunctionState

bool ConjunctionState::isFalse()                           { throw std::logic_error("ConjunctionState::isFalse must not be called"); }
ConjunctionState::element_type &ConjunctionState::ignore() { throw std::logic_error("ConjunctionState::ignore must not be called"); }

// }}}
// {{{ definition of Conjunction

void Conjunction::printPlain(std::ostream &out) const {
    if (!repr->second.elems.empty()) {
        int sep = 0;
        for (auto &x : repr->second.elems) {
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
ULit Conjunction::toLparse(LparseTranslator &x) {
    if (!repr->second.bdLit) {
        LparseRuleCreator bd;
        for (ConjunctionState::Elem &y : repr->second.elems) {
            if ((y.heads.size() == 1 && y.heads.front().empty()) || y.bodies.empty()) {
                // this part of the conditional literal is a fact
                continue;
            }
            else if (y.isSimple()) {
                if (y.bodies.size() == 1 && y.bodies.front().empty()) {
                    assert(y.heads.size() <= 1);
                    if (y.heads.empty()) { 
                        repr->second.bdLit = x.makeAux();
                        return get_clone(repr->second.bdLit);
                    }
                    else { bd.addBody(y.heads.front().front()); }
                }
                else { bd.addBody(y.bodies.front().front()->negateLit(x)); }
            }
            else {
                ULit aux = x.makeAux();
                ULit auxHead = x.makeAux();
                for (auto &head : y.heads) {
                    // auxHead :- y.head.
                    LRC().addHead(auxHead).addBody(get_clone(head)).toLparse(x);
                    LRC().addHead(aux).addBody(auxHead).toLparse(x);
                }
                // chk <=> body.
                std::vector<ULitVec> bodies;
                for (auto &body : y.bodies) {
                    bodies.emplace_back(get_clone(body));
                }
                ULit chk = getEqualFormula(x, std::move(bodies), false, repr->second.incomplete && !y.heads.empty());
                LRC().addHead(aux).addBody(chk->negateLit(x)).toLparse(x);
                if (repr->second.incomplete && !y.heads.empty()) {
                    // aux | chk | ~x.first.
                    LRC().addHead(aux).addHead(chk).addHead(auxHead->negateLit(x)).toLparse(x);
                }
                // body += aux
                bd.addBody(aux);
            }
        }
        // bdLit :- body.
        if (bd.body.size() == 1) {
            repr->second.bdLit = std::move(bd.body.front());
            Term::replace(repr->second.bdLit, repr->second.bdLit->toLparse(x));
        }
        else {
            repr->second.bdLit = x.makeAux();
            bd.addHead(repr->second.bdLit).toLparse(x);
        }
    }
    return get_clone(repr->second.bdLit);
}
int Conjunction::lparseUid(LparseOutputter &) const { throw std::logic_error("Conjunction::toLparse: must be called before Conjunction::lparseUid"); }
bool Conjunction::isIncomplete() const                    { return repr->second.incomplete; }
Conjunction *Conjunction::clone() const                   { return new Conjunction(*this); }
size_t Conjunction::hash() const                          { throw std::runtime_error("Conjunction::hash: implement me if necessary!"); }
bool Conjunction::operator==(Literal const &) const       { throw std::runtime_error("Conjunction::operator==: implement me if necessary!"); }
bool Conjunction::needsSemicolon() const {
    return !repr->second.elems.empty() && repr->second.elems.back().needsSemicolon();
}
Conjunction::~Conjunction()                               { }

// }}}
// {{{ definition of DisjointState

DisjointElem::DisjointElem(CSPGroundAdd &&value, int fixed, ULitVec &&lits)
    : value(std::move(value))
    , fixed(fixed)
    , lits(std::move(lits)) { }
DisjointElem::DisjointElem(DisjointElem &&) = default;
DisjointElem DisjointElem::clone() const { return DisjointElem(get_clone(value), fixed, get_clone(lits)); }
DisjointElem::~DisjointElem() { }

bool DisjointState::fact(bool recursive) const { return !recursive && elems.size() <= 1; }
unsigned DisjointState::generation() const     { return _generation - 1; }
bool DisjointState::isFalse()                  { return false; }
DisjointState::element_type &DisjointState::ignore() {
    static element_type x{std::piecewise_construct, std::forward_as_tuple(Value::createId("#false")), std::forward_as_tuple()};
    return x;
}
bool DisjointState::defined() const            { return _generation > 0; }
void DisjointState::generation(unsigned x)     { _generation = x + 1; }
DisjointState::~DisjointState()                { }

// }}}
// {{{ definition of DisjointLiteral

DisjointLiteral::DisjointLiteral(NAF naf) : naf(naf) { }

void DisjointLiteral::printPlain(std::ostream &out) const {
    auto print_elem = [](std::ostream &out, DisjointElemSet::value_type const &x) {
        assert(!x.second.empty());
        auto print_cond = [&x](std::ostream &out, DisjointElem const &y) {
            print_comma(out, x.first, ",");
            out << ":";
            auto print_add = [](std::ostream &out, CSPGroundAdd::value_type const &z) {
                if (z.first == 1) { out              << "$" << z.second; }
                else              { out << z.first << "$*$" << z.second; }
            };
            if (y.value.empty()) { out << y.fixed; }
            else {
                print_comma(out, y.value, "$+", print_add);
                if (y.fixed > 0)      { out << "$+" << y.fixed; }
                else if (y.fixed < 0) { out << "$-" << -y.fixed; }
            }
            if (!y.lits.empty()) {
                out << ":";
                using namespace std::placeholders;
                print_comma(out, y.lits, ",", std::bind(&Literal::printPlain, _2, _1));
            }
        };
        print_comma(out, x.second, ";", print_cond);
    };
    out << naf;
    out << "#disjoint{";
    print_comma(out, repr->second.elems, ";", print_elem);
    out << "}";
}
ULit DisjointLiteral::toLparse(LparseTranslator &x) {
    SAuxAtom aux(std::make_shared<AuxAtom>(x.auxAtom()));
    DisjointCons cons;
    for (auto &y : repr->second.elems) { 
        cons.emplace_back(std::piecewise_construct, std::forward_as_tuple(y.first), std::forward_as_tuple());
        for (auto &z : y.second) {
            ULitVec cond;
            for (auto &lit : z.lits) {
                cond.emplace_back(lit->toLparse(x));
                if (!cond.back()) { cond.back() = std::move(lit); }
            }
            cons.back().second.emplace_back(std::move(z.value), z.fixed, std::move(cond));
        }
    }
    x.addDisjointConstraint(aux, std::move(cons));
    ULit auxLit = gringo_make_unique<AuxLiteral>(aux, NAF::NOT);
    if (naf != NAF::NOT) {
        auxLit = auxLit->negateLit(x);
    }
    Term::replace(auxLit, auxLit->toLparse(x));
    return auxLit;
}
int DisjointLiteral::lparseUid(LparseOutputter &) const {
    throw std::runtime_error("DisjointLiteral::lparseUid must be called after DisjointLiteral::toLparse");
}
bool DisjointLiteral::isIncomplete() const { return incomplete; }
DisjointLiteral *DisjointLiteral::clone() const { return new DisjointLiteral(*this); }
size_t DisjointLiteral::hash() const { throw std::runtime_error("DisjointLiteral::hash: implement me if necessary!"); }
bool DisjointLiteral::operator==(Literal const &) const { throw std::runtime_error("DisjointLiteral::operator==: implement me if necessary!"); }
DisjointLiteral::~DisjointLiteral() { }

// }}}
// {{{ definition of CSPLiteral

CSPLiteral::CSPLiteral() = default;

void CSPLiteral::reset(CSPGroundLit &&ground) {
    this->ground = std::move(ground);
}
void CSPLiteral::printPlain(std::ostream &out) const {
    if (!std::get<1>(ground).empty()) {
        auto f = [](std::ostream &out, std::pair<int, Value> mul) { out << mul.first << "$*$" << mul.second; };
        print_comma(out, std::get<1>(ground), "$+", f);
    }
    else { out << 0; }
    out << "$" << std::get<0>(ground);
    out << std::get<2>(ground);
}
bool CSPLiteral::isIncomplete() const {
    return false;
}
CSPLiteral *CSPLiteral::clone() const {
    return new CSPLiteral(*this);
}
size_t CSPLiteral::hash() const {
    return get_value_hash(typeid(CSPLiteral).hash_code(), ground);
}
bool CSPLiteral::operator==(Literal const &x) const {
    CSPLiteral const *t = dynamic_cast<CSPLiteral const*>(&x);
    return t && is_value_equal_to(ground, t->ground);
}
bool CSPLiteral::isBound(Value &value, bool negate) const {
    Relation rel = std::get<0>(ground);
    if (negate) { rel = neg(rel); }
    if (std::get<1>(ground).size() != 1) { return false; }
    if (rel == Relation::NEQ)            { return false; }
    if (value.type() == Value::SPECIAL)  { value = std::get<1>(ground).front().second; }
    return value == std::get<1>(ground).front().second;
}
void CSPLiteral::updateBound(CSPBound &bound, bool negate) const {
    Relation rel = std::get<0>(ground);
    if (negate) { rel = neg(rel); }
    int coef  = std::get<1>(ground).front().first;
    int fixed = std::get<2>(ground);
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
ULit CSPLiteral::toLparse(LparseTranslator &x) {
    Relation rel = std::get<0>(ground);
    int bound    = std::get<2>(ground);
    auto addInv = [&x](SAuxAtom aux, CoefVarVec &&vars, int bound) {
        for (auto &x : vars) { x.first *= -1; }
        x.addLinearConstraint(aux, std::move(vars), -bound);
    };
    SAuxAtom aux(std::make_shared<AuxAtom>(x.auxAtom()));
    switch (rel) {
        case Relation::EQ:
        case Relation::NEQ: {
            x.addLinearConstraint(aux, CoefVarVec(std::get<1>(ground)), bound-1);
            addInv(aux, std::move(std::get<1>(ground)), bound + 1);
            return gringo_make_unique<AuxLiteral>(aux, rel != Relation::NEQ ? NAF::NOT : NAF::POS);
        }
        case Relation::LT:  { bound--; }
        case Relation::LEQ: {
            x.addLinearConstraint(aux, CoefVarVec(std::get<1>(ground)), bound);
            return gringo_make_unique<AuxLiteral>(aux, NAF::POS);
        }
        case Relation::GT:  { bound++; }
        case Relation::GEQ: {
            addInv(aux, std::move(std::get<1>(ground)), bound);
            return gringo_make_unique<AuxLiteral>(aux, NAF::POS);
        }
    }
    assert(false);
    return nullptr;
}
int CSPLiteral::lparseUid(LparseOutputter &) const {
    throw std::logic_error("CSPLiteral::lparseUid: must not be called");
}
bool CSPLiteral::invertible() const {
    return true;
}
void CSPLiteral::invert() {
    std::get<0>(ground) = neg(std::get<0>(ground));
}

CSPLiteral::~CSPLiteral() { }

// }}}

} } // namespace Output Gringo
