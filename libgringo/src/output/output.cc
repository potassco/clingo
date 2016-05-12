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

#include "gringo/output/output.hh"
#include "gringo/logger.hh"
#include "gringo/output/aggregates.hh"
#include <cstring>

namespace Gringo { namespace Output {

namespace {

struct DefaultLparseTranslator;

// {{{ declaration of Bound

struct Bound {
    using const_iterator = enum_interval_set<int>::const_iterator;
    using atom_vec = std::vector<std::pair<int, SAuxAtom>>;
    Bound(Value var) 
        : modified(true)
        , var(var) {
        _range.add(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    }
    operator Value const & () const { return var; }
    bool init(DefaultLparseTranslator &x);
    int getLower(int coef) const { 
        if (_range.empty()) { return 0; }
        return coef * (coef < 0 ? _range.back() : _range.front()); 
    }
    int getUpper(int coef) const { 
        if (_range.empty()) { return -coef; }
        return coef * (coef < 0 ? _range.front() : _range.back()); 
    }
    void remove(int l, int r) {
        _range.remove(l, r);
        modified = true;
    }
    void add(int l, int r) {
        _range.add(l, r);
        modified = true;
    }
    void clear() {
        _range.clear();
        modified = true;
    }
    void intersect(enum_interval_set<int> &x) {
        _range.intersect(x);
        modified = true;
    }
    const_iterator begin() const { return _range.begin(); }
    const_iterator end() const { return _range.end(); }

    bool                   modified;
    Value                  var;
    
    atom_vec               atoms;
    enum_interval_set<int> _range;
};

// }}}
// {{{ auxiliary functions

bool showSig(OutputPredicates const &outPreds, Signature const &sig, bool csp = false) {
    if (outPreds.empty()) { return true; }
    auto le = [](OutputPredicates::value_type const &x, OutputPredicates::value_type const &y) -> bool { 
        if (std::get<1>(x) != std::get<1>(y)) { return std::get<1>(x) < std::get<1>(y); }
        return std::get<2>(x) < std::get<2>(y);
    };
    static Location loc("",1,1,"",1,1);
    return std::binary_search(outPreds.begin(), outPreds.end(), OutputPredicates::value_type(loc, sig, csp), le);
}
bool showBound(OutputPredicates const &outPreds, Bound const &bound) {
    return outPreds.empty() || ((bound.var.type() == Value::FUNC || bound.var.type() == Value::ID) && showSig(outPreds, *bound.var.sig(), true));
}

// }}}
// {{{ declaration of LinearConstraint

struct LinearConstraint {
    struct State {
        using Coef = CoefVarVec::value_type;
        State(Bound &bound, Coef &coef) 
            : bound(bound)
            , coef(coef.first) { reset(); }
        int upper()  { return bound.getUpper(coef); }
        int lower()  { return bound.getLower(coef); }
        bool valid() { return atom != (coef < 0 ? bound.atoms.begin() : bound.atoms.end()); }
        void next(int total, int &ret) {
            if (coef < 0) {
                --atom;
                --current;
                auto it = current;
                --it;
                ret = total + coef * (*it);
            }
            else {
                ++atom;
                ++current;
                ret = total + coef * (*current);
            }
        }
        void reset() {
            current = coef < 0 ? bound.end() : bound.begin();
            atom    = coef < 0 ? bound.atoms.end() : bound.atoms.begin();
        }
        Bound                                 &bound;
        enum_interval_set<int>::const_iterator current;
        Bound::atom_vec::iterator              atom;
        int                                    coef;
    };
    using StateVec = std::vector<State>;
    struct Generate {
        Generate(LinearConstraint &cons, DefaultLparseTranslator &trans)
            : cons(cons)
            , trans(trans) { }
        bool init();
        int getBound() { return cons.bound; }
        void generate(StateVec::iterator it, int current, int remainder);
        StateVec               states;
        LinearConstraint            &cons;
        DefaultLparseTranslator &trans;
        SAuxAtomVec            aux;
    };
    LinearConstraint(SAuxAtom atom, CoefVarVec &&coefs, int bound)
        : atom(atom)
        , coefs(std::move(coefs))
        , bound(bound) { }
    bool encode(DefaultLparseTranslator &x);
    SAuxAtom   atom;
    CoefVarVec coefs;
    int        bound;
};

// }}}
// {{{ declaration of DisjointConstraint

struct DisjointConstraint {
    DisjointConstraint(SAuxAtom atom, DisjointCons &&cons)
        : atom(atom)
        , cons(std::move(cons)) { }
    bool encode(DefaultLparseTranslator &x);
    SAuxAtom     atom;
    DisjointCons cons;
};

// }}}
// {{{ declaration of DefaultLparseTranslator

struct DefaultLparseTranslator : LparseTranslator {
    using StmPrinter      = std::function<void (Statement const &)>;
    using MinimizeList    = std::vector<std::pair<FWValVec, ULitVec>>;
    using BoundMap        = unique_list<Bound, identity<Value>>;
    using ConstraintVec   = std::vector<LinearConstraint>;
    using DisjointConsVec = std::vector<DisjointConstraint>;

    DefaultLparseTranslator(PredDomMap &domains, StmPrinter const &printer)
        : domains(domains)
        , printer(printer) { }
    BoundMap::value_type &addBound(Value x) {
        auto it = boundMap.find(x);
        return it != boundMap.end() ? *it : *boundMap.emplace_back(x).first;
    }
    virtual bool isAtomFromPreviousStep(ULit const &lit) {
        PredicateDomain::element_type *atom = lit->isAtom();
        if (!atom) { return false; }
        if (!atom->first.hasSig()) { return false; }
        auto it = domains.find(atom->first.sig());
        return it != domains.end() && atom->second.generation() < it->second.exports.incOffset;
    }
    static int min() { return std::numeric_limits<int>::min(); }
    static int max() { return std::numeric_limits<int>::max(); }
    virtual void addLowerBound(Value x, int bound) {
        auto &y = addBound(x);
        y.remove(std::numeric_limits<int>::min(), bound);
    }
    virtual void addUpperBound(Value x, int bound) {
        auto &y = addBound(x);
        y.remove(bound+1, std::numeric_limits<int>::max());
    }
    virtual void addBounds(Value value, std::vector<CSPBound> bounds) {
        std::map<Value, enum_interval_set<int>> boundUnion;
        for (auto &x : bounds) {
            boundUnion[value].add(x.first, x.second+1);
        }
        for (auto &x : boundUnion) {
            auto &z = addBound(x.first);
            z.intersect(x.second);
        }
    }
    virtual void addLinearConstraint(SAuxAtom head, CoefVarVec &&vars, int bound) {
        for (auto &x : vars) { addBound(x.second); }
        constraints.emplace_back(head, std::move(vars), bound);
    }
    virtual void addDisjointConstraint(SAuxAtom head, DisjointCons &&elem) {
        for (auto &x : elem) { 
            for (auto &y : x.second) {
                for (auto z : y.value) { addBound(z.second); }
            }
        }
        disjointCons.emplace_back(head, std::move(elem));
    }
    virtual unsigned auxAtom() {
        return ++auxUid;
    }
    virtual void operator()(Statement &x) { printer(x); }
    virtual void addMinimize(MinimizeList &&x) {
        minimizeChanged_ = minimizeChanged_ || !x.empty();
        std::move(x.begin(), x.end(), std::back_inserter(minimize));
    }
    virtual bool minimizeChanged() const {
        return minimizeChanged_;
    }
    virtual void translate() {
        for (auto &x : boundMap) { 
            if (!x.init(*this)) { return; }
        }
        for (auto &x : disjointCons) { x.encode(*this); }
        for (auto &x : constraints)  { x.encode(*this); }
        disjointCons.clear();
        constraints.clear();
        if (minimizeChanged_) {
            translateMinimize();
            minimizeChanged_ = false;
        }
    }
    virtual void outputSymbols(LparseOutputter &out, OutputPredicates const &outPreds) {
        std::vector<std::tuple<unsigned, Value, int>> symtab;
        auto show = [&](Bound const &bound, AtomState *cond) -> void {
            std::string const *name = nullptr;
            if (bound.var.type() == Value::FUNC || bound.var.type() == Value::ID) { 
                name = &*bound.var.name();
            }
            if ((!name || name->empty() || name->front() != '#')) {
                auto assign = [&](int i, SAuxAtom a, SAuxAtom b) {
                    LparseOutputter::LitVec body;
                    if (a) { body.emplace_back(a->lparseUid(out)); }
                    if (b) { body.emplace_back(-b->lparseUid(out)); }
                    if (cond && cond->uid() && !cond->fact(false)) { body.emplace_back(cond->uid()); }
                    unsigned head = out.newUid();
                    out.printBasicRule(head, body);
                    symtab.emplace_back(head, bound.var, i);
                };
                auto it = bound.begin();
                for (auto jt = bound.atoms.begin() + 1; jt != bound.atoms.end(); ++jt) { assign(*it++, jt->second, (jt-1)->second); }
                assign(*it++, nullptr, bound.atoms.back().second);
            }
        };
        // show csp varibles
        {
            auto it = incBoundOffset, ie = boundMap.end();
            if (it == ie) { it = boundMap.begin(); }
            else          { ++it; }
            for (; it != ie; ++it) {
                if (it->var.type() == Value::FUNC || it->var.type() == Value::ID) { seenSigs.emplace(it->var.sig()); }
                if (showBound(outPreds, *it)) { show(*it, nullptr); }
                incBoundOffset = it;
            }
        }
        // show explicitely shown csp variables (if not shown above)
        auto it(domains.find(Signature("#show", 2)));
        if (it != domains.end()) {
            for (auto jt = it->second.exports.begin() + it->second.exports.showOffset, je = it->second.exports.end(); jt != je; ++jt) {
                if ((jt->get().first.args().begin()+1)->num() == 1) {
                    Value val = jt->get().first.args().front();
                    auto bound = boundMap.find(val);
                    if (bound != boundMap.end()) {
                        if (!showBound(outPreds, *bound)) { show(*bound, &jt->get().second); }
                    }
                    else {
                        GRINGO_REPORT(W_ATOM_UNDEFINED) 
                            << "info: constraint variable does not occur in program:\n"
                            << "  $" << val << "\n";
                    }
                }
            }
            // Note: showNext is called in LparseHandler::finish
        }
        // check for signatures that did not occur in the program
        for (auto &x : outPreds) {
            if (std::get<1>(x) != Signature("", 0) && std::get<2>(x)) {
                auto it(seenSigs.find(std::get<1>(x)));
                if (it == seenSigs.end()) {
                    GRINGO_REPORT(W_ATOM_UNDEFINED) 
                        << std::get<0>(x) << ": info: no constraint variables over signature occur in program:\n"
                        << "  $" << *std::get<1>(x) << "\n";
                    seenSigs.emplace(std::get<1>(x));
                }
            }
        }
        out.finishRules();
        for (auto &y : symtab) {
            std::ostringstream oss;
            oss 
                << std::get<1>(y)
                << "="
                << std::get<2>(y);
            out.printSymbol(std::get<0>(y), Value::createId(oss.str()));
        }
        if (!outPreds.empty()) {
            // show what was requested
            for (auto &x : outPreds) {
                if (!std::get<2>(x)) {
                    auto it(domains.find(std::get<1>(x)));
                    if (it != domains.end()) {
                        for (auto jt = it->second.exports.begin() + it->second.exports.showOffset, je = it->second.exports.end(); jt != je; ++jt) {
                            if (!jt->get().second.hasUid()) { jt->get().second.uid(out.newUid()); }
                            out.printSymbol(jt->get().second.uid(), jt->get().first);
                        }
                        it->second.exports.showNext();
                    }
                }
            }
        }
        else {
            // show everything non-internal
            for (auto &x : domains) {
                std::string const &name(*(*x.first).name());
                if ((name.empty() || name.front() != '#')) {
                    for (auto jt = x.second.exports.begin() + x.second.exports.showOffset, je = x.second.exports.end(); jt != je; ++jt) {
                        if (!jt->get().second.hasUid()) { jt->get().second.uid(out.newUid()); }
                        out.printSymbol(jt->get().second.uid(), jt->get().first);
                    }
                    x.second.exports.showNext();
                }
            }
        }
        // show terms
        it = domains.find(Signature("#show", 2));
        if (it != domains.end()) {
            for (auto jt = it->second.exports.begin() + it->second.exports.showOffset, je = it->second.exports.end(); jt != je; ++jt) {
                if ((jt->get().first.args().begin()+1)->num() == 0) {
                    if (!jt->get().second.hasUid()) { jt->get().second.uid(out.newUid()); }
                    out.printSymbol(jt->get().second.uid(), jt->get().first.args().front());
                }
            }
            it->second.exports.showNext();
        }

        // NOTE: otherwise clasp will complain about redefinition errors...
        trueLit = nullptr;
    }
    void atoms(int atomset, std::function<bool(unsigned)> const &isTrue, ValVec &atoms, OutputPredicates const &outPreds) {
        if (atomset & (Model::ATOMS | Model::SHOWN)) {
            for (auto &x : domains) {
                Signature sig = *x.first;
                std::string const &name = *sig.name();
                if (((atomset & Model::ATOMS || (atomset & Model::SHOWN && showSig(outPreds, *x.first))) && !name.empty() && name.front() != '#')) {
                    for (auto &y: x.second.domain) {
                        if (y.second.defined() && y.second.hasUid() && isTrue(y.second.uid())) { atoms.emplace_back(y.first); }
                    }
                }
            }
        }
        if ((atomset & (Model::TERMS | Model::SHOWN))) {
            auto it(domains.find(Signature("#show", 2)));
            if (it != domains.end()) {
                for (auto &y: it->second.domain) {
                    if (y.first.args().back() == Value::createNum(0) && y.second.defined() && y.second.hasUid() && isTrue(y.second.uid())) { atoms.emplace_back(y.first.args().front()); }
                }
            }
        }
        auto showCsp = [&isTrue, &atoms](Bound const &bound) {
            assert(!bound.atoms.empty());
            int prev = bound.atoms.front().first;
            for (auto it = bound.atoms.begin()+1; it != bound.atoms.end() && !isTrue(it->second->uid); ++it);
            atoms.emplace_back(Value::createFun("$", {bound.var, Value::createNum(prev)}));
        };
        if (atomset & (Model::CSP | Model::SHOWN)) {
            for (auto &x : boundMap) {
                if (atomset & Model::CSP || (atomset & Model::SHOWN && showBound(outPreds, x))) { showCsp(x); }
            }
        }
        if (atomset & Model::SHOWN) {
            auto it(domains.find(Signature("#show", 2)));
            if (it != domains.end()) {
                for (auto &y: it->second.domain) {
                    if (y.first.args().back() == Value::createNum(1) && y.second.defined() && y.second.hasUid() && isTrue(y.second.uid())) {
                        auto bound = boundMap.find(y.first.args().front());
                        if (bound != boundMap.end() && !showBound(outPreds, *bound)) { showCsp(*bound); }
                    }
                }
            }
        }
    }
    void translateMinimize();

    ULit makeAux(NAF naf = NAF::POS) {
        return gringo_make_unique<AuxLiteral>(std::make_shared<AuxAtom>(auxAtom()), naf);
    }

    ULit getTrueLit() {
        if (!trueLit) {
            trueLit = makeAux();
            LRC().addHead(trueLit).toLparse(*this);
        }
        return get_clone(trueLit);
    }
    virtual void simplify(AssignmentLookup assignment) {
        minimize.erase(std::remove_if(minimize.begin(), minimize.end(), [&assignment](MinimizeList::value_type &elem) {
            bool remove = false;
            elem.second.erase(std::remove_if(elem.second.begin(), elem.second.end(), [&assignment,&remove](ULit &lit) {
                auto value = lit->getTruth(assignment);
                switch (value.second) {
                    case TruthValue::True:  { return true; }
                    case TruthValue::False: { remove = true; }
                    default:                { return false; }
                }
            }), elem.second.end());
            return remove;
        }), minimize.end());
    }
    virtual ~DefaultLparseTranslator() { }

    PredDomMap        &domains;
    BoundMap           boundMap;
    ConstraintVec      constraints;
    DisjointConsVec    disjointCons;
    MinimizeList       minimize;
    StmPrinter         printer;
    unsigned           auxUid = 0;
    bool               minimizeChanged_ = false;
    std::set<FWSignature> seenSigs;
    BoundMap::iterator incBoundOffset;
    ULit               trueLit;
};

// }}}

// {{{ definition of DefaultLparseTranslator

void DefaultLparseTranslator::translateMinimize() {
    // TODO: should detect if there are changes in the minimize constraint
    // priority -> tuple -> [[lit]]
    using GroupByPriority = std::map<Value, unique_list<std::pair<FWValVec, std::vector<ULitVec>>, extract_first<FWValVec>>>;
    GroupByPriority groupBy;
    for (auto &y : minimize) {
        auto &ret(groupBy.insert(GroupByPriority::value_type(*(y.first.begin() + 1), GroupByPriority::mapped_type())).first->second);
        ret.emplace_back(
            std::piecewise_construct,
            std::forward_as_tuple(y.first),
            std::forward_as_tuple()).first->second.emplace_back(get_clone(y.second));
    }
    for (auto &y : groupBy) {
        ULitWeightVec weightLits;
        for (auto &conds : y.second) {
            ULitVec condLits;
            int weight(conds.first.front().num());
            for (auto &cond : conds.second) {
                condLits.emplace_back(getEqualClause(*this, std::move(cond), true, false));
            }
            ULit lit = getEqualClause(*this, std::move(condLits), false, false);
            weightLits.emplace_back(weight < 0 ? lit->negateLit(*this) : std::move(lit), std::abs(weight));
        }
        printer(LparseMinimize(y.first, std::move(weightLits)));
    }
}
// }}}
// {{{ definition of Bound

bool Bound::init(DefaultLparseTranslator &x) {
    if (modified) {
        modified = false;
        if (_range.empty()) { LRC().toLparse(x); }
        else {
            if (_range.front() == std::numeric_limits<int>::min() || _range.back()+1 == std::numeric_limits<int>::max()) {
                if      (_range.front()  != std::numeric_limits<int>::min()) { _range.remove(_range.front()+1, std::numeric_limits<int>::max()); }
                else if (_range.back()+1 != std::numeric_limits<int>::max()) { _range.remove(std::numeric_limits<int>::min(), _range.back()); }
                else                                                        { _range.clear(), _range.add(0, 1); }
                GRINGO_REPORT(W_VARIABLE_UNBOUNDED)
                    << "warning: unbounded constraint variable:\n"
                    << "  domain of '" << var << "' is set to [" << _range.front() << "," << _range.back() << "]\n"
                    ;
            }
            if (atoms.empty()) {
                auto assign = [&](SAuxAtom a, SAuxAtom b) {
                    if (b) {
                        LRC rule(true);
                        if (a) { rule.addBody(gringo_make_unique<AuxLiteral>(a, NAF::POS)); }
                        rule.addHead(gringo_make_unique<AuxLiteral>(b, NAF::POS)).toLparse(x);
                    }
                };
                for (auto y : _range) { 
                    if (y == _range.front()) { atoms.emplace_back(y, nullptr); } 
                    else                     { atoms.emplace_back(y, std::make_shared<AuxAtom>(x.auxAtom())); }
                }
                for (auto jt = atoms.begin() + 1; jt != atoms.end(); ++jt) { assign(jt->second, (jt-1)->second); }
                assign(nullptr, atoms.back().second);
            }
            else { // incremental update of bounds
                atom_vec next;
                int l = _range.front(), r = _range.back();
                for (auto jt = atoms.begin() + 1; jt != atoms.end(); ++jt) {
                    int w = (jt - 1)->first;
                    if (w < l)                           { LRC().addBody(gringo_make_unique<AuxLiteral>(jt->second, NAF::POS)).toLparse(x); }
                    else if (w >= r)                     { 
                        LRC().addBody(gringo_make_unique<AuxLiteral>(jt->second, NAF::NOT)).toLparse(x);
                        if (w == r) { next.emplace_back(*(jt - 1)); }
                    }
                    else if (!_range.contains(w, w + 1)) { LRC().addBody(gringo_make_unique<AuxLiteral>((jt-1)->second, NAF::NOT)).addBody(gringo_make_unique<AuxLiteral>(jt->second, NAF::POS)).toLparse(x); }
                    else                                 { next.emplace_back(*(jt - 1)); }
                }
                if (atoms.back().first <= r) { next.emplace_back(atoms.back()); }
                next.front().second = nullptr;
                atoms = std::move(next);
            }
        }
    }
    return !_range.empty();
}

// }}}
// {{{ definition of LinearConstraint

int depth = 0;
void LinearConstraint::Generate::generate(StateVec::iterator it, int current, int remainder) {
    depth++;
    if (current + remainder > getBound()) {
        if (it == states.end()) {
            assert(remainder == 0);
            aux.emplace_back(std::make_shared<AuxAtom>(trans.auxAtom()));
            for (auto &x : states) {
                if (x.atom != x.bound.atoms.end() && x.atom->second) { 
                    LRC()
                        .addHead(gringo_make_unique<AuxLiteral>(aux.back(), NAF::POS))
                        .addBody(gringo_make_unique<AuxLiteral>(x.atom->second, x.coef < 0 ? NAF::NOT : NAF::POS))
                        .toLparse(trans);
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
    depth--;
}

bool LinearConstraint::Generate::init() {
    int current   = 0;
    int remainder = 0;
    for (auto &y : cons.coefs) {
        states.emplace_back(*trans.boundMap.find(y.second), y);
        current   += states.back().lower();
        remainder += states.back().upper() - states.back().lower();
    }
    if (current <= getBound()) {
        generate(states.begin(), current, remainder); 
        ULitVec body;
        for (auto &x : aux) { body.emplace_back(gringo_make_unique<AuxLiteral>(x, NAF::POS)); }
        LRC().addHead(gringo_make_unique<AuxLiteral>(cons.atom, NAF::POS)).addBody(std::move(body)).toLparse(trans);
    }
    return current <= cons.bound;
}

bool LinearConstraint::encode(DefaultLparseTranslator &x) {
    Generate gen(*this, x);
    return gen.init();
}

// }}}
// {{{ definition of DisjointConstraint

bool DisjointConstraint::encode(DefaultLparseTranslator &x) {
    std::set<int> values;
    std::vector<std::map<int, SAuxAtom>> layers;
    for (auto &elem : cons) {
        layers.emplace_back();
        auto &current = layers.back();
        auto encodeSingle = [&current, &x, &values](ULitVec const &cond, int coef, Value var, int fixed) -> void {
            auto &bound = *x.boundMap.find(var);
            auto it = bound.atoms.begin() + 1;
            for(auto i : bound) {
                auto &aux = current[i*coef + fixed];
                if (!aux) { aux = std::make_shared<AuxAtom>(x.auxAtom()); }
                ULitVec lits = get_clone(cond);
                if ((it-1)->second)          { lits.emplace_back(gringo_make_unique<AuxLiteral>((it-1)->second, NAF::NOT)); }
                if (it != bound.atoms.end()) { lits.emplace_back(gringo_make_unique<AuxLiteral>(it->second, NAF::POS)); }
                LRC().addHead(gringo_make_unique<AuxLiteral>(aux, NAF::POS)).addBody(std::move(lits)).toLparse(x);
                values.insert(i*coef + fixed);
                ++it;
            }
        };
        for (auto &condVal : elem.second) {
            switch(condVal.value.size()) {
                case 0: {
                    auto &aux = current[condVal.fixed];
                    if (!aux) { aux = std::make_shared<AuxAtom>(x.auxAtom()); }
                    LRC().addHead(gringo_make_unique<AuxLiteral>(aux, NAF::POS)).addBody(std::move(condVal.lits)).toLparse(x);
                    values.insert(condVal.fixed);
                    break;
                }
                case 1: {
                    encodeSingle(condVal.lits, condVal.value.front().first, condVal.value.front().second, condVal.fixed);
                    break;
                }
                default: {
                    // create a bound possibly with holes
                    auto &b = *x.boundMap.emplace_back(Value::createId("#aux" + std::to_string(x.auxAtom()))).first;
                    b.clear();
                    std::set<int> values;
                    values.emplace(0);
                    for (auto &mul : condVal.value) {
                        std::set<int> nextValues;
                        for (auto i : *x.boundMap.find(mul.second)) {
                            for (auto j : values) { nextValues.emplace(j + i * mul.first); }
                        }
                        values = std::move(nextValues);
                    }
                    for (auto i : values) { b.add(i, i+1); }
                    b.init(x);

                    // create a new variable with the respective bound
                    // b = value + fixed
                    // b - value = fixed
                    // aux :- b - value <=  fixed - 1
                    // aux :- value - b <= -fixed - 1
                    // :- aux.
                    SAuxAtom aux(std::make_shared<AuxAtom>(x.auxAtom()));
                    condVal.value.emplace_back(-1, b.var);
                    x.addLinearConstraint(aux, get_clone(condVal.value), -condVal.fixed-1);
                    for (auto &add : condVal.value) { add.first *= -1; }
                    x.addLinearConstraint(aux, get_clone(condVal.value), condVal.fixed-1);
                    LRC().addBody(gringo_make_unique<AuxLiteral>(aux, NAF::POS)).toLparse(x);
                    // then proceed as in case one
                    encodeSingle(condVal.lits, 1, b.var, 0);
                    break;
                }
            }
        }
    }
    ULitVec checks;
    for (auto &value : values) {
        WeightRule::ULitBoundVec card;
        for (auto &layer : layers) {
            auto it = layer.find(value);
            if (it != layer.end()) {
                card.emplace_back(gringo_make_unique<AuxLiteral>(it->second, NAF::POS), 1);
            }
        }
        SAuxAtom aux = std::make_shared<AuxAtom>(x.auxAtom());
        checks.emplace_back(gringo_make_unique<AuxLiteral>(aux, NAF::NOT));
        x.printer(WeightRule(aux, 2, std::move(card)));
    }
    LRC().addHead(gringo_make_unique<AuxLiteral>(atom, NAF::POS)).addBody(std::move(checks)).toLparse(x);
    return true;
}

// }}}

// {{{ definition of LparseHandler

struct LparseHandler : StmHandler {
    LparseHandler(PredDomMap &domains, LparseOutputter &out, LparseDebug debug) 
        : trans(domains, [&out, debug](Statement const &x) {
            if ((unsigned)debug & (unsigned)LparseDebug::LPARSE) { std::cerr << "%%"; x.printPlain(std::cerr); }
            x.printLparse(out);
        }) 
        , out(out)
        , debug(debug) { }
    virtual void operator()(Statement &x) { 
        if ((unsigned)debug & (unsigned)LparseDebug::PLAIN) { std::cerr << "%"; x.printPlain(std::cerr); }
        x.toLparse(trans);
    }
    virtual void incremental() { out.incremental(); }
    virtual void operator()(PredicateDomain::element_type &head, TruthValue type) {
        if (!head.second.hasUid()) { head.second.uid(out.newUid()); }
        out.printExternal(head.second.uid(), type);
    }
    virtual void finish(OutputPredicates &outPreds) {
        out.disposeMinimize() = trans.minimizeChanged();
        trans.translate(); 
        trans.outputSymbols(out, outPreds);
        out.finishSymbols();
    }
    virtual void atoms(int atomset, std::function<bool(unsigned)> const &isTrue, ValVec &atoms, OutputPredicates const &outPreds) {
        trans.atoms(atomset, isTrue, atoms, outPreds);
    }
    virtual void simplify(AssignmentLookup assignment) {
        trans.simplify(assignment);
    }
    virtual ~LparseHandler() { }

    DefaultLparseTranslator trans;
    LparseOutputter &out;
    LparseDebug debug;
};

// }}}
// {{{ definition of LparsePlainHandler

struct LparsePlainHandler : StmHandler {
    LparsePlainHandler(PredDomMap &domains, std::ostream &out)
        : trans(domains, [&out](Statement const &x) { x.printPlain(out); })
        , out(out) { }
    virtual void operator()(Statement &x) { x.toLparse(trans); }
    virtual void operator()(PredicateDomain::element_type &head, TruthValue type) {
        switch (type) {
            case TruthValue::False: { out << "#external " << head.first << ".\n"; break; }
            case TruthValue::True:  { out << "#external " << head.first << "=true.\n"; break; }
            case TruthValue::Free:  { out << "#external " << head.first << "=free.\n"; break; }
            case TruthValue::Open:  { out << "#external " << head.first << "=open.\n"; break; }
        }
    }
    virtual void finish(OutputPredicates &) { trans.translate(); }
    virtual void atoms(int, std::function<bool(unsigned)> const &, ValVec &, OutputPredicates const &) { }
    virtual void simplify(AssignmentLookup assignment) {
        trans.simplify(assignment);
    }
    virtual ~LparsePlainHandler() { }

    DefaultLparseTranslator trans;
    std::ostream           &out;
};

// }}}
// {{{ definition of PlainHandler

struct PlainHandler : StmHandler {
    PlainHandler(std::ostream &out) : out(out)            { }
    virtual ~PlainHandler()                               { }
    virtual void operator()(Statement &x)                 { x.printPlain(out); }
    virtual void operator()(PredicateDomain::element_type &head, TruthValue type) {
        switch (type) {
            case TruthValue::False: { out << "#external " << head.first << ".\n"; break; }
            case TruthValue::True:  { out << "#external " << head.first << "=true.\n"; break; }
            case TruthValue::Free:  { out << "#external " << head.first << "=free.\n"; break; }
            case TruthValue::Open:  { out << "#external " << head.first << "=open.\n"; break; }
        }
    }
    virtual void finish(OutputPredicates &outPreds) { 
        for (auto &x : outPreds) { 
            if (std::get<1>(x) != Signature("", 0)) { std::cout << "#show " << (std::get<2>(x) ? "$" : "") << *std::get<1>(x) << ".\n"; }
            else                                    { std::cout << "#show.\n"; }
        }
    }
    virtual void atoms(int, std::function<bool(unsigned)> const &, ValVec &, OutputPredicates const &) { }
    virtual void simplify(AssignmentLookup) { }
    std::ostream &out;
};

// }}}

} // namespace

// {{{ definition of PlainLparseOutputter

PlainLparseOutputter::PlainLparseOutputter(std::ostream &out) : out(out) { }
void PlainLparseOutputter::incremental() {
    out << "90 0\n";
}
void PlainLparseOutputter::printBasicRule(unsigned head, LitVec const &body) {
    out << "1 " << head << " " << body.size();
    unsigned neg(0);
    for (auto &x : body) { neg+= x < 0; }
    out << " " << neg;
    for (auto &x : body) { if (x < 0) { out << " " << -x; } }
    for (auto &x : body) { if (x > 0) { out << " " << +x; } }
    out << "\n";
}
void PlainLparseOutputter::printChoiceRule(AtomVec const &head, LitVec const &body) {
    out << "3 " << head.size();
    for (auto &x : head) { out << " " << x; }
    out << " " << body.size();
    unsigned neg(0);
    for (auto &x : body) { neg+= x < 0; }
    out << " " << neg;
    for (auto &x : body) { if (x < 0) { out << " " << -x; } }
    for (auto &x : body) { if (x > 0) { out << " " << +x; } }
    out << "\n";
}
void PlainLparseOutputter::printCardinalityRule(unsigned head, unsigned lower, LitVec const &body) {
    out << "2 " << head << " " << body.size();
    unsigned neg(0);
    for (auto &x : body) { neg+= x < 0; }
    out << " " << neg << " " << lower;
    for (auto &x : body) { if (x < 0) { out << " " << -x; } }
    for (auto &x : body) { if (x > 0) { out << " " << +x; } }
    out << "\n";
}
void PlainLparseOutputter::printWeightRule(unsigned head, unsigned lower, LitWeightVec const &body) {
    out << "5 " << head << " " << lower << " " << body.size();
    unsigned neg(0);
    for (auto &x : body) { neg+= x.first < 0; }
    out << " " << neg;
    for (auto &x : body) { if (x.first < 0) { out << " " << -x.first; } }
    for (auto &x : body) { if (x.first > 0) { out << " " << +x.first; } }
    for (auto &x : body) { if (x.first < 0) { out << " " << x.second; } }
    for (auto &x : body) { if (x.first > 0) { out << " " << x.second; } }
    out << "\n";
}
void PlainLparseOutputter::printMinimize(LitWeightVec const &body) {
    out << "6 0 " << body.size();
    unsigned neg(0);
    for (auto &x : body) { neg+= x.first < 0; }
    out << " " << neg;
    for (auto &x : body) { if (x.first < 0) { out << " " << -x.first; } }
    for (auto &x : body) { if (x.first > 0) { out << " " << +x.first; } }
    for (auto &x : body) { if (x.first < 0) { out << " " << x.second; } }
    for (auto &x : body) { if (x.first > 0) { out << " " << x.second; } }
    out << "\n";
}
void PlainLparseOutputter::printDisjunctiveRule(AtomVec const &head, LitVec const &body) {
    out << "8 " << head.size();
    for (auto &x : head) { out << " " << x; }
    out << " " << body.size();
    unsigned neg(0);
    for (auto &x : body) { neg+= x < 0; }
    out << " " << neg;
    for (auto &x : body) { if (x < 0) { out << " " << -x; } }
    for (auto &x : body) { if (x > 0) { out << " " << +x; } }
    out << "\n";
}
void PlainLparseOutputter::printExternal(unsigned atomUid, TruthValue type) { 
    switch (type) {
        case TruthValue::False: { out << "91 " << atomUid << " 0\n"; break; }
        case TruthValue::True:  { out << "91 " << atomUid << " 1\n"; break; }
        case TruthValue::Open:  { out << "91 " << atomUid << " 2\n"; break; }
        case TruthValue::Free:  { out << "92 " << atomUid << "\n"; break; }
    }
}
unsigned PlainLparseOutputter::falseUid()                                   { return 1; }
unsigned PlainLparseOutputter::newUid()                                     { return uids++; }
void PlainLparseOutputter::finishRules()                                    { out << "0\n"; }
void PlainLparseOutputter::printSymbol(unsigned atomUid, Value v)           { out << atomUid << " " << v << "\n"; }
void PlainLparseOutputter::finishSymbols()                                  { out << "0\nB+\n0\nB-\n" << falseUid() << "\n0\n1\n"; }
PlainLparseOutputter::~PlainLparseOutputter()                               { }

// }}}
// {{{ definition of OutputBase

OutputBase::OutputBase(OutputPredicates &&outPreds, std::ostream &out, bool lparse)
    : handler(lparse ? UStmHandler(new LparsePlainHandler(domains, out)) : UStmHandler(new PlainHandler(out))) 
    , outPreds(std::move(outPreds)) { }
OutputBase::OutputBase(OutputPredicates &&outPreds, LparseOutputter &out, LparseDebug debug) 
    : handler(gringo_make_unique<LparseHandler>(domains, out, debug))
    , outPreds(std::move(outPreds)) { }
void OutputBase::output(Value const &val) {
    auto it(domains.find(val.sig()));
    assert(it != domains.end());
    auto ret(it->second.insert(val, true));
    if (!std::get<2>(ret)) {
        tempRule.head = std::get<0>(ret);
        tempRule.body.clear();
        (*handler)(tempRule);
    }
}
void OutputBase::incremental() {
    handler->incremental();
}
void OutputBase::createExternal(PredicateDomain::element_type &head) {
    head.second.setExternal(true);
    (*handler)(head, TruthValue::False);
}
void OutputBase::assignExternal(PredicateDomain::element_type &head, TruthValue type) {
    (*handler)(head, type);
}
void OutputBase::output(UStm &&x) {
    if (!x->isIncomplete()) { (*handler)(*x); }
    else { stms.emplace_back(std::move(x)); }
}
void OutputBase::output(Statement &x) {
    if (!x.isIncomplete()) { (*handler)(x); }
    else { stms.emplace_back(x.clone()); }
}
void OutputBase::flush() {
    for (auto &x : stms) { (*handler)(*x); }
    stms.clear();
}
void OutputBase::finish() { 
    if (!outPreds.empty()) {
        std::move(outPredsForce.begin(), outPredsForce.end(), std::back_inserter(outPreds));
        outPredsForce.clear();
    }
    handler->finish(outPreds);
    std::vector <Gringo::FWSignature> rm;
    for (auto &x : domains) {
        if (std::strncmp((*(*x.first).name()).c_str(), "#d", 2) == 0) {
            rm.emplace_back(x.first);
        }
    }
    for (auto const &x : rm) {
        domains.erase(x);
    }
} 
void OutputBase::checkOutPreds() {
    auto le = [](OutputPredicates::value_type const &x, OutputPredicates::value_type const &y) -> bool { 
        if (std::get<1>(x) != std::get<1>(y)) { return std::get<1>(x) < std::get<1>(y); }
        return std::get<2>(x) < std::get<2>(y);
    };
    auto eq = [](OutputPredicates::value_type const &x, OutputPredicates::value_type const &y) { 
        return std::get<1>(x) == std::get<1>(y) && std::get<2>(x) == std::get<2>(y); 
    };
    std::sort(outPreds.begin(), outPreds.end(), le);
    outPreds.erase(std::unique(outPreds.begin(), outPreds.end(), eq), outPreds.end());
    for (auto &x : outPreds) {
        if (std::get<1>(x) != Signature("", 0) && !std::get<2>(x)) {
            auto it(domains.find(std::get<1>(x)));
            if (it == domains.end()) {
                GRINGO_REPORT(W_ATOM_UNDEFINED) 
                    << std::get<0>(x) << ": info: no atoms over signature occur in program:\n"
                    << "  " << *std::get<1>(x) << "\n";
            }
        }
    }
}
ValVec OutputBase::atoms(int atomset, std::function<bool(unsigned)> const &isTrue) const {
    Gringo::ValVec ret;
    handler->atoms(atomset, isTrue, ret, outPreds);
    return ret;
}
Gringo::AtomState const *OutputBase::find(Gringo::Value val) const {
    if (val.type() == Gringo::Value::ID || val.type() == Gringo::Value::FUNC) {
        auto it = domains.find(val.sig());
        if (it != domains.end()) {
            auto jt = it->second.domain.find(val);
            if (jt != it->second.domain.end() && jt->second.defined()) {
                return &jt->second;
            }
        }
    }
    return nullptr;
}
PredicateDomain::element_type *OutputBase::find2(Gringo::Value val) {
    if (val.type() == Gringo::Value::ID || val.type() == Gringo::Value::FUNC) {
        auto it = domains.find(val.sig());
        if (it != domains.end()) {
            auto jt = it->second.domain.find(val);
            if (jt != it->second.domain.end() && jt->second.defined()) {
                return &*jt;
            }
        }
    }
    return nullptr;
}
std::pair<unsigned, unsigned> OutputBase::simplify(AssignmentLookup assignment) {
    // TODO: would be nice to have this one in the statistics output
    handler->simplify(assignment);
    unsigned facts = 0;
    unsigned deleted = 0;
    for (auto &x : domains) {
        unsigned offset = 0;
        x.second.indices.clear();
        x.second.fullIndices.clear();
        x.second.exports.exports.erase(std::remove_if(x.second.exports.begin(), x.second.exports.end(), [&](Gringo::PredicateDomain::element_type &y) -> bool {
            y.second.generation(offset);
            if (y.second.hasUid()) {
                auto value = assignment(y.second.uid());
                if (!value.first) {
                    switch (value.second) {
                        case TruthValue::True: {
                            // NOTE: externals cannot become facts here
                            //       because they might get new definitions while grounding
                            //       because there is no distinction between true and weak true
                            //       these definitions might be skipped if a weak true external 
                            //       is made a fact here
                            if (!y.second.fact(false)) { ++facts; }
                            y.second.setFact(true);
                            break;
                        }
                        case TruthValue::False: {
                            if (offset < x.second.exports.incOffset)  { --x.second.exports.incOffset; }
                            if (offset < x.second.exports.showOffset) { --x.second.exports.showOffset; }
                            x.second.domain.erase(y.first);
                            ++deleted;
                            return true;
                        }
                        default: { break; }
                    }
                }
            }
            ++offset;
            return false;
        }), x.second.exports.end());
        x.second.exports.generation_     = 0;
        x.second.exports.nextGeneration_ = x.second.exports.size();
    }
    return {facts, deleted};
}

// }}}

} } // namespace Output Gringo
