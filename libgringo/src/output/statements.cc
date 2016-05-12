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
#include <gringo/output/aggregates.hh>

namespace Gringo { namespace Output {

// {{{ definition of helpers to print interval sets

namespace Debug {

std::ostream &operator<<(std::ostream &out, IntervalSet<Value>::LBound const &x) {
    out << (x.inclusive ? "[" : "(") << x.bound;
    return out;
}

std::ostream &operator<<(std::ostream &out, IntervalSet<Value>::RBound const &x) {
    out << x.bound << (x.inclusive ? "]" : ")");
    return out;
}

std::ostream &operator<<(std::ostream &out, IntervalSet<Value>::Interval const &x) {
    out << x.left << "," << x.right;
    return out;
}

std::ostream &operator<<(std::ostream &out, IntervalSet<Value> const &x) {
    out << "{";
    print_comma(out, x.vec, ",", [](std::ostream &out, IntervalSet<Value>::Interval const &x) { out << x; });
    out << "}";
    return out;
}

} // namespace Debug

namespace {

template <typename B, typename F>
void printPlainBody(std::ostream &out, B const &body, F get) {
    int sep = 0;
    for (auto &x : body) {
        switch (sep) { 
        case 1: { out << ","; break; }
        case 2: { out << ";"; break; }
        }
        get(x).printPlain(out);
        sep = static_cast<bool>(get(x).needsSemicolon()) + 1;
    }
}

template <class U>
struct deref {
    auto operator ()(U const &x) const -> decltype(*x) { return *x; }
    auto operator ()(U &x) const -> decltype(*x) { return *x; }
};

template <class U>
struct id {
    U const &operator ()(U const &x) const { return x; }
    U &operator ()(U &x) const { return x; }
};

} // namespace


// }}}

// {{{ definition of Rule

Rule::Rule() { }
Rule::Rule(PredicateDomain::element_type *head, ULitVec &&body) : head(head), body(std::move(body)) { }
void Rule::printPlain(std::ostream &out) const {
    bool isShow = head && head->first.sig() == Signature("#show", 2);
    if (isShow)    { out << "#show " << ((*(head->first.args().begin()+1)).num() == 1 ? "$" : "") << head->first.args().front(); }
    else if (head) { out << head->first; }
    if (!body.empty() || !head) { out << (isShow ? ":" : ":-"); }
    printPlainBody(out, body, deref<ULit>());
    out << ".\n";
}
void Rule::toLparse(LparseTranslator &x) {
    if (!head) {
        Value value;
        auto isBound = [&]() {
            if (body.empty())     { return false; }
            for (auto &x : body) {
                if (!x->isBound(value, true)) { return false; }
            }
            return true;
        };
        if (isBound()) {
            std::vector<CSPBound> bounds;
            for (auto &y : body) {
                bounds.emplace_back(std::numeric_limits<int>::min(), std::numeric_limits<int>::max()-1);
                y->updateBound(bounds.back(), true);
            }
            x.addBounds(value, bounds);
            return;
        }
    }
    for (auto &y : body) { Term::replace(y, y->toLparse(x)); }
    x(*this);
}
void Rule::printLparse(LparseOutputter &out) const {
    unsigned headUid;
    if (!head)                       { headUid = out.falseUid(); }
    else if (!head->second.hasUid()) { headUid = out.newUid(); head->second.uid(headUid); }
    else                             { headUid = head->second.uid(); }
    LparseOutputter::LitVec lits;
    for (auto &x : body) { lits.emplace_back(x->lparseUid(out)); }
    out.printBasicRule(headUid, lits);
}
Rule *Rule::clone() const {
    auto ret(gringo_make_unique<Rule>());
    ret->body = get_clone(body);
    ret->head = head;
    return ret.release();
}
bool Rule::isIncomplete() const {
    for (auto &x : body) {
        if (x->isIncomplete()) {
            return true;
        }
    }
    return false;
}
Rule::~Rule() { }

void RuleRef::printPlain(std::ostream &out) const {
    bool isShow = head && head->first.sig() == Signature("#show", 2);
    if (isShow)    { out << "#show " << ((*(head->first.args().begin()+1)).num() == 1 ? "$" : "") << head->first.args().front(); }
    else if (head) { out << head->first; }
    if (!body.empty() || !head) { out << (isShow ? ":" : ":-"); }
    printPlainBody(out, body, id<Literal>());
    out << ".\n";
}
Rule *RuleRef::clone() const {
    auto ret(gringo_make_unique<Rule>());
    for (Literal &x : body) { ret->body.emplace_back(ULit(x.clone())); }
    ret->head = head;
    return ret.release();
}
void RuleRef::toLparse(LparseTranslator &x) {
    ULitVec bd;
    for (auto &y : body) {
        // TODO: stupid copy and paste
        if (!head) {
            Value value;
            auto isBound = [&]() {
                if (body.empty())     { return false; }
                for (auto &x : body) {
                    if (!x.get().isBound(value, true)) { return false; }
                }
                return true;
            };
            if (isBound()) {
                std::vector<CSPBound> bounds;
                for (auto &y : body) {
                    bounds.emplace_back(std::numeric_limits<int>::min(), std::numeric_limits<int>::max()-1);
                    y.get().updateBound(bounds.back(), true);
                }
                x.addBounds(value, bounds);
                return;
            }
        }
        if (ULit z = y.get().toLparse(x)) {
            bd.emplace_back(std::move(z));
            y = *bd.back();
        }
    }
    x(*this);
}
void RuleRef::printLparse(LparseOutputter &out) const {
    unsigned headUid;
    if (!head)                       { headUid = out.falseUid(); }
    else if (!head->second.hasUid()) { headUid = out.newUid(); head->second.uid(headUid); }
    else                             { headUid = head->second.uid(); }
    LparseOutputter::LitVec lits;
    for (Literal &x : body) { lits.emplace_back(x.lparseUid(out)); }
    out.printBasicRule(headUid, lits);
}
bool RuleRef::isIncomplete() const {
    for (Literal &x : body) {
        if (x.isIncomplete()) {
            return true;
        }
    }
    return false;
}
RuleRef::~RuleRef() { }

// }}}
// {{{ definition of LparseRule

LparseRule::LparseRule(HeadVec &&head, SAuxAtomVec &&auxHead, ULitVec &&body, bool choice) : choice(choice), head(std::move(head)), auxHead(std::move(auxHead)), body(std::move(body)) { }
void LparseRule::printPlain(std::ostream &out) const {
    if (choice) { out << "{"; }
    print_comma(out, head, choice ? ";" : "|", [](std::ostream &out, PredicateDomain::element_type const &x) { out << x.first;  });
    if (!head.empty() && !auxHead.empty()) { out << (choice ? ";" : "|"); }
    print_comma(out, auxHead, "|", [](std::ostream &out, SAuxAtom const &x) { out << *x; });
    if (choice) { out << "}"; }
    if (!body.empty()) {
        out << ":-";
        printPlainBody(out, body, deref<ULit>());
    }
    out << ".\n";
}
void LparseRule::toLparse(LparseTranslator &x) {
    // TODO: stupid copy and paste
    if (head.empty() && auxHead.empty()) {
        Value value;
        auto isBound = [&]() {
            if (body.empty())     { return false; }
            for (auto &x : body) {
                if (!x->isBound(value, true)) { return false; }
            }
            return true;
        };
        if (isBound()) {
            std::vector<CSPBound> bounds;
            for (auto &y : body) {
                bounds.emplace_back(std::numeric_limits<int>::min(), std::numeric_limits<int>::max()-1);
                y->updateBound(bounds.back(), true);
            }
            x.addBounds(value, bounds);
            return;
        }
    }
    for (auto &y : body) { Term::replace(y, y->toLparse(x)); }
    x(*this);
}
void LparseRule::printLparse(LparseOutputter &out) const {
    LparseOutputter::AtomVec atoms;
    for (PredicateDomain::element_type &x : head) {
        if (!x.second.hasUid()) { x.second.uid(out.newUid()); }
        atoms.emplace_back(x.second.uid());
    }
    for (auto &x : auxHead) {
        if (!x->uid) { x->uid = out.newUid(); }
        atoms.emplace_back(x->uid);
    }
    LparseOutputter::LitVec lits;
    for (auto &x : body) { lits.emplace_back(x->lparseUid(out)); }

    if (atoms.empty())          { out.printBasicRule(out.falseUid(), lits); }
    else if (choice)            { out.printChoiceRule(atoms, lits); }
    else if (atoms.size() == 1) { out.printBasicRule(atoms.front(), lits); }
    else                        { out.printDisjunctiveRule(atoms, lits); }
}
LparseRule *LparseRule::clone() const {
    auto ret(gringo_make_unique<LparseRule>(get_clone(head), get_clone(auxHead), get_clone(body), choice));
    return ret.release();
}
bool LparseRule::isIncomplete() const { return false; }
LparseRule::~LparseRule() { }

// }}}
// {{{ definition of WeightRule

WeightRule::WeightRule(SAuxAtom head, unsigned lower, ULitBoundVec &&body) : head(std::move(head)), body(std::move(body)), lower(lower) { }
void WeightRule::printPlain(std::ostream &out) const {
    out << *head << ":-" << lower << "{";
    if (!body.empty()) {
        auto it(body.begin()), ie(body.end());
        it->first->printPlain(out);
        out << "=" << it->second;
        for (++it; it != ie; ++it) { 
            out << ",";
            it->first->printPlain(out);
            out << "=" << it->second;
        }
    }
    out << "}.\n";
}
void WeightRule::toLparse(LparseTranslator &x) {
    for (auto &y : body) { Term::replace(y.first, y.first->toLparse(x)); }
    x(*this);
}
void WeightRule::printLparse(LparseOutputter &out) const {
    if(!head->uid) { head->uid = out.newUid(); }
    bool card = true;
    for (auto &x : body) {
        if (x.second != 1) { 
            card = false; 
            break;
        }
    }
    if (card) {
        LparseOutputter::LitVec lits;
        for (auto &x : body) { lits.emplace_back(x.first->lparseUid(out)); }
        out.printCardinalityRule(head->uid, lower, lits);
    }
    else {
        LparseOutputter::LitWeightVec lits;
        for (auto &x : body) { lits.emplace_back(x.first->lparseUid(out), x.second); }
        out.printWeightRule(head->uid, lower, lits);
    }
}
WeightRule *WeightRule::clone() const {
    auto ret(gringo_make_unique<WeightRule>(head, lower, get_clone(body)));
    return ret.release();
}
bool WeightRule::isIncomplete() const { return false; }
WeightRule::~WeightRule() { }

// }}}
// {{{ definition of HeadAggregateState

HeadAggregateElement::Cond::Cond(PredicateDomain::element_type *head, unsigned headNum, Output::ULitVec &&lits)
    : head(head)
    , headNum(headNum)
    , lits(std::move(lits)) { }

HeadAggregateState::HeadAggregateState() { throw std::runtime_error("must not happen"); }
HeadAggregateState::HeadAggregateState(AggregateFunction fun, unsigned generation) 
    : _generation(generation) {
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
void HeadAggregateState::accumulate(ValVec const &tuple, AggregateFunction fun, PredicateDomain::element_type *head, unsigned headNum, LitVec const &lits, Location const &loc) {
    bool ignore = neutral(tuple, fun, loc);
    if (!ignore || head) {
        bool fact(lits.empty() && (!head || (head->second.defined() && head->second.fact(false))));
        auto ret(elems.emplace_back(std::piecewise_construct, std::forward_as_tuple(tuple), std::forward_as_tuple()));
        bool remove(fact && !ret.first->second.fact && !ret.second);
        if (!ret.first->second.fact || head) { // even if the condition is a fact it has to be accumulated to capture choices
            auto &elem(ret.first->second);
            elem.fact = elem.fact || fact;
            Output::ULitVec uLits;
            for (Output::Literal &x : lits) { uLits.emplace_back(x.clone()); }
            elem.conds.emplace_back(head, headNum, std::move(uLits));
            if ((ret.second || remove) && !ignore) {
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
        }
    }
}
bool HeadAggregateState::defined() const { return true; }
HeadAggregateState::Bounds::Interval HeadAggregateState::range(AggregateFunction fun) const {
    if (fun != AggregateFunction::MIN && fun != AggregateFunction::MAX) {
        return {{Value::createNum(clamp(intMin)), true}, {Value::createNum(clamp(intMax)), true}};
    }
    else { return {{valMin, true}, {valMax, true}}; }
}
unsigned HeadAggregateState::generation() const { return _generation; }
bool HeadAggregateState::fact(bool) const { return false; }
HeadAggregateState::element_type &HeadAggregateState::ignore() { throw std::logic_error("HeadAggregateState::ignore must not be called"); }

// }}}
// {{{ definition of HeadAggregateRule

void HeadAggregateRule::toLparse(LparseTranslator &x) {
    auto range(repr->range(fun));
    if (range.empty()) {
        LRC().addBody(std::move(body)).toLparse(x);
        return;
    }
    ULit bodyLit;
    if (!body.empty()) {
        // b :- body.
        bodyLit = getEqualClause(x, std::move(body), true, false);
    }
    using GroupByCond = unique_list<
        std::pair<std::reference_wrapper<ULitVec>, std::vector<std::pair<FWValVec, HeadAggregateElement::Cond&>>>, 
        extract_first<ULitVec>, 
        value_hash<ULitVec>, 
        value_equal_to<ULitVec>
    >;
    BdAggrElemSet bdElems;
    GroupByCond groupByCond;
    for (auto &y : repr->elems) {
        for (auto &z : y.second.conds) {
            auto ret(groupByCond.emplace_back(std::piecewise_construct, std::forward_as_tuple(z.lits), std::forward_as_tuple()));
            ret.first->second.emplace_back(y.first, z);
        }
    }
    for (auto &cond : groupByCond) {
        ULit condLit;
        if (!cond.first.get().empty()) { 
            // c :- C.
            condLit = getEqualClause(x, get_clone(cond.first.get()), true, false);
        }
        // { heads } :- c, b.
        LRC choice(true);
        for (auto &elem : cond.second) {
            ULit head = elem.second.head ? gringo_make_unique<PredicateLiteral>(NAF::POS, *elem.second.head) : nullptr;
            if (head) { choice.addHead(head); }
            // e :- h, c.
            auto bdElem = bdElems.emplace_back(std::piecewise_construct, std::forward_as_tuple(elem.first), std::forward_as_tuple());
            bdElem.first->second.emplace_back();
            if (condLit) { bdElem.first->second.back().emplace_back(get_clone(condLit)); }
            if (head)    { bdElem.first->second.back().emplace_back(std::move(head)); }
        }
        if (!choice.head.empty()) {
            if (bodyLit) { choice.addBody(bodyLit); }
            if (condLit) { choice.addBody(condLit); }
            choice.toLparse(x);
        }
    }

    // :- b, not aggr.
    if (!repr->bounds.contains(range)) {
        ULit aggr = getEqualAggregate(x, fun, NAF::NOT, repr->bounds, range, bdElems, false);
        LRC check;
        if (bodyLit) { check.addBody(std::move(bodyLit)); }
        check.addBody(std::move(aggr)).toLparse(x);
    }
}
void HeadAggregateRule::printLparse(LparseOutputter &) const {
    throw std::runtime_error("HeadAggregateRule::printLparse must be called after HeadAggregateRule::toLparse");
}
void HeadAggregateRule::printElem(std::ostream &out, HeadAggregateState::ElemSet::value_type const &x) {
    bool comma = false;
    for (auto &y : x.second.conds) {
        if (comma) { out << ";"; }
        else       { comma = true; }
        print_comma(out, x.first, ",");
        out << ":";
        if (y.head) { out << y.head->first; }
        else        { out << "#true"; }
        if (!y.lits.empty()) {
            out << ":";
            using namespace std::placeholders;
            print_comma(out, y.lits, ",", std::bind(&Literal::printPlain, _2, _1));
        }
    }
}
void HeadAggregateRule::printPlain(std::ostream &out) const {
    auto it = bounds.begin(), ie = bounds.end();
    if (it != ie) { out << it->second << inv(it->first); ++it; }
    out << fun << "{";
    print_comma(out, repr->elems, ";", &HeadAggregateRule::printElem);
    out << "}";
    for (; it != ie; ++it) { out << it->first << it->second; }
    if (!body.empty()) {
        out << ":-";
        printPlainBody(out, body, deref<ULit>());
    }
    out << ".\n";
}
bool HeadAggregateRule::isIncomplete() const { return true; }
HeadAggregateRule *HeadAggregateRule::clone() const { 
    auto ret(gringo_make_unique<HeadAggregateRule>());
    ret->body   = get_clone(body);
    ret->bounds = bounds;
    ret->repr   = repr;
    ret->fun    = fun;
    return ret.release();
}
HeadAggregateRule::~HeadAggregateRule() { }

// }}}
// {{{ definition of DisjunctionState

DisjunctionElement::DisjunctionElement(Value repr)
: repr(repr) { }

DisjunctionElement::operator Value const & () const {
    return repr;
}

DisjunctionElement::DisjunctionElement(DisjunctionElement &&) = default;
DisjunctionElement &DisjunctionElement::operator=(DisjunctionElement &&) = default;

void DisjunctionElement::print(std::ostream &out) const {
    using namespace std::placeholders;
    if (bodies.empty()) {
        out << "#false";
    }
    else { 
        if (heads.empty()) {
            out << "#true";
        }
        else {
            auto print_conjunction = [](std::ostream &out, ULitVec const &lits) {
                if (lits.empty()) {
                    out << "#false";
                }
                else {
                    print_comma(out, lits, "|", std::bind(&Literal::printPlain, _2, _1));
                }
            };
            print_comma(out, heads, "&", print_conjunction);
        }
        if (!bodies.front().empty()) {
            out << ":";
            auto print_disjunction = [](std::ostream &out, ULitVec const &lits) {
                if (lits.empty()) {
                    out << "#true";
                }
                else {
                    print_comma(out, lits, "&", std::bind(&Literal::printPlain, _2, _1));
                }
            };
            print_comma(out, bodies, "|", print_disjunction);
        }
    }
}

DisjunctionState::DisjunctionState() = default;

bool DisjunctionState::fact(bool) const { return false; }

DisjunctionState::element_type &DisjunctionState::ignore() { throw std::logic_error("DisjunctionState::ignore must not be called"); }

// }}}
// {{{ definition of DisjunctionRule

void DisjunctionRule::toLparse(LparseTranslator &x) {
    bool isFact = false;
    repr->elems.erase(std::remove_if(repr->elems.begin(), repr->elems.end(), [&isFact](DisjunctionState::Elem &elem){
        if (elem.heads.empty() && elem.bodies.size() == 1 && elem.bodies.front().empty()) { 
            isFact = true;
        }
        return elem.bodies.empty() || (elem.heads.size() == 1 && elem.heads.front().empty());
        return false;
    }), repr->elems.end());
    if (isFact) { return; }
    if (body.empty()) {
        Value value;
        auto isBound = [&]() -> bool {
            if (repr->elems.empty()) { return false; }
            for (auto &y : repr->elems) {
                if (y.bodies.size() != 1 && !y.bodies.front().empty()) { return false; }
                for (auto &z : y.heads) {
                    if (z.size() != 1) { return false; }
                    for (auto &u : z) {
                        if (!u->isBound(value, false)) { return false; }
                    }
                }
            }
            return true;
        };
        if (isBound()) {
            std::vector<CSPBound> bounds;
            for (auto &y : repr->elems) {
                bounds.emplace_back(std::numeric_limits<int>::min(), std::numeric_limits<int>::max()-1);
                for (auto &z : y.heads) {
                    for (auto &u : z) { u->updateBound(bounds.back(), false); }
                }
            }
            x.addBounds(value, bounds);
            return;
        }
    }
    LRC dj;
    for (auto &elem : repr->elems) {
        assert (!elem.bodies.empty());
        ULit cond;
        if (elem.bodies.size() == 1 && elem.bodies.front().size() == 1) {
            // condition is singleton
            cond = std::move(elem.bodies.front().front());
        }
        else if (elem.bodies.size() != 1 || !elem.bodies.front().empty()) {
            // condition needs auxiliary variable
            cond = x.makeAux();
            for (auto &body : elem.bodies) {
                LRC().addHead(cond).addBody(get_clone(body)).toLparse(x);
            }
        }
        if (elem.heads.empty()) {
            assert(cond);
            // head is a fact
            body.emplace_back(cond->negateLit(x));
        }
        else if (elem.heads.size() == 1) {
            if (cond) {
                ULit headAtom = x.makeAux();
                for (auto &lit : elem.heads.front()) {
                    LRC().addHead(headAtom).addBody(lit).addBody(cond).toLparse(x);
                }
                LRC().addHead(get_clone(elem.heads.front())).addBody(cond).addBody(headAtom).toLparse(x);
                LRC().addBody(cond->negateLit(x)).addBody(headAtom).toLparse(x);
                dj.addHead(headAtom);
            }
            else {
                // the head literals can be pushed directly into djHead
                dj.addHead(get_clone(elem.heads.front()));
            }
        }
        else {
            ULit conjunctionAtom = x.makeAux();
            ULitVec conjunctionBody;
            for (auto &head : elem.heads) {
                if (head.size() == 1) {
                    LRC().addHead(head.front()).addBody(conjunctionAtom).toLparse(x);
                    conjunctionBody.emplace_back(get_clone(head.front()));
                }
                else {
                    ULit disjunctionAtom = x.makeAux();
                    LRC().addHead(get_clone(head)).addBody(disjunctionAtom).toLparse(x);
                    for (auto &lit : head) {
                        LRC().addHead(disjunctionAtom).addBody(lit).toLparse(x);
                    }
                    conjunctionBody.emplace_back(std::move(disjunctionAtom));
                }
            }
            LRC().addHead(conjunctionAtom).addBody(std::move(conjunctionBody)).toLparse(x);
            if (cond) {
                ULit headAtom = x.makeAux();
                LRC().addHead(headAtom).addBody(conjunctionAtom).addBody(cond).toLparse(x);
                LRC().addHead(conjunctionAtom).addBody(headAtom).addBody(cond).toLparse(x);
                LRC().addBody(cond->negateLit(x)).addBody(headAtom).toLparse(x);
                dj.addHead(std::move(headAtom));
            }
            else {
                dj.addHead(std::move(conjunctionAtom));
            }
        }
    }
    dj.addBody(std::move(body)).toLparse(x);
}
void DisjunctionRule::printLparse(LparseOutputter &) const {
    throw std::logic_error("DisjunctionRule::toLparse must be called before DisjunctionRule::printLparse");
}
void DisjunctionRule::printPlain(std::ostream &out) const {
    print_comma(out, repr->elems, ";");
    if (!body.empty()) {
        out << ":-";
        printPlainBody(out, body, deref<ULit>());
    }
    out << ".\n";
}
bool DisjunctionRule::isIncomplete() const { return true; }
DisjunctionRule *DisjunctionRule::clone() const { 
    auto ret(gringo_make_unique<DisjunctionRule>());
    ret->repr = repr;
    ret->body = get_clone(body);
    return ret.release();
}
DisjunctionRule::~DisjunctionRule() { }

// }}}
// {{{ definition of Minimize

void Minimize::toLparse(LparseTranslator &x) {
    for (auto &y : elems) {
        for (auto &z : y.second) { Term::replace(z, z->toLparse(x)); }
    }
    x.addMinimize(std::move(elems));
}
void Minimize::printPlain(std::ostream &out) const {
    for (auto &x : elems) {
        out << ":~";
        printPlainBody(out, x.second, deref<ULit>());
        out << ".[";
        auto it(x.first.begin());
        out << *it++ << "@";
        out << *it++;
        for (auto ie(x.first.end()); it != ie; ++it) { out << "," << *it; }
        out << "]\n";
    }
}
void Minimize::printLparse(LparseOutputter &) const {
    throw std::runtime_error("Minimize::printLparse: must not be called");
}
bool Minimize::isIncomplete() const { return false; }
Minimize *Minimize::clone() const { throw std::logic_error("Minimize::clone must not be called."); }
Minimize::~Minimize() { }

// }}}
// {{{ definition of LparseMinimize

LparseMinimize::LparseMinimize(Value prio, ULitWeightVec &&lits)
    : prio(prio)
    , lits(std::move(lits)) { }
void LparseMinimize::toLparse(LparseTranslator &x) {
    for (auto &y : lits) { Term::replace(y.first, y.first->toLparse(x)); }
    x(*this);
}
void LparseMinimize::printPlain(std::ostream &out) const {
    int i = 0;
    out << "#minimize{";
    auto f = [&i, this](std::ostream &out, ULitWeightVec::value_type const &x) { out << x.second << "@" << prio << "," << i++ << ":"; x.first->printPlain(out); };
    print_comma(out, lits, ";", f);
    out << "}.\n";
}
void LparseMinimize::printLparse(LparseOutputter &x) const {
    LparseOutputter::LitWeightVec body;
    for (auto &y : lits) { body.emplace_back(y.first->lparseUid(x), y.second); }
    x.printMinimize(body);
}
bool LparseMinimize::isIncomplete() const { return false; }
LparseMinimize *LparseMinimize::clone() const { throw std::logic_error("LparseMinimize::clone must not be called."); }
LparseMinimize::~LparseMinimize() { }

// }}}

} } // namespace Output Gringo
