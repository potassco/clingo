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

#include "gringo/input/programbuilder.hh"
#include "gringo/input/literals.hh"
#include "gringo/input/aggregates.hh"
#include "gringo/input/program.hh"
#include "gringo/input/theory.hh"
#include "gringo/output/output.hh"
#include "gringo/scripts.hh"

namespace Gringo { namespace Input {

// {{{1 definition of NongroundProgramBuilder

NongroundProgramBuilder::NongroundProgramBuilder(Scripts &scripts, Program &prg, Output::OutputBase &out, Defines &defs, bool rewriteMinimize)
: scripts_(scripts)
, prg_(prg)
, out(out)
, defs_(defs)
, rewriteMinimize_(rewriteMinimize) { }

// {{{2 terms

TermUid NongroundProgramBuilder::term(Location const &loc, Symbol val) {
    return terms_.insert(make_locatable<ValTerm>(loc, val));
}

TermUid NongroundProgramBuilder::term(Location const &loc, String name) {
    if (name == "_") { return terms_.insert(make_locatable<VarTerm>(loc, name, nullptr)); }
    else {
        auto &ret(vals_[name]);
        if (!ret) { ret = std::make_shared<Symbol>(); }
        return terms_.insert(make_locatable<VarTerm>(loc, name, ret));
    }
}

TermUid NongroundProgramBuilder::term(Location const &loc, UnOp op, TermUid a) {
    return term(loc, op, termvec(termvec(), a));
}

TermUid NongroundProgramBuilder::term(Location const &loc, UnOp op, TermVecUid a) {
    UTermVec vec(termvecs_.erase(a));
    if (vec.size() == 1) { return terms_.insert(make_locatable<UnOpTerm>(loc, op, std::move(vec.front()))); }
    else {
        UTermVec pool;
        for (auto &terms : vec) { pool.emplace_back(make_locatable<UnOpTerm>(loc, op, std::move(terms))); }
        return terms_.insert(make_locatable<PoolTerm>(loc, std::move(pool)));
    }
}

TermUid NongroundProgramBuilder::term(Location const &loc, BinOp op, TermUid a, TermUid b) {
    return terms_.insert(make_locatable<BinOpTerm>(loc, op, terms_.erase(a), terms_.erase(b)));
}

TermUid NongroundProgramBuilder::term(Location const &loc, TermUid a, TermUid b) {
    return terms_.insert(make_locatable<DotsTerm>(loc, terms_.erase(a), terms_.erase(b)));
}

TermUid NongroundProgramBuilder::term(Location const &loc, String name, TermVecVecUid a, bool lua) {
    assert(name != "");
    auto create = [&lua, &name, &loc](UTermVec &&vec) -> UTerm {
        // lua terms
        if (lua) { return make_locatable<LuaTerm>(loc, name, std::move(vec)); }
        // constant symbols
        else if (vec.empty()) { return make_locatable<ValTerm>(loc, Symbol::createId(name)); }
        // function terms
        else { return make_locatable<FunctionTerm>(loc, name, std::move(vec)); }
    };
    TermVecVecs::ValueType vec(termvecvecs_.erase(a));
    // no pooling
    if (vec.size() == 1) { return terms_.insert(create(std::move(vec.front()))); }
    // pooling
    else {
        UTermVec pool;
        for (auto &terms : vec) { pool.emplace_back(create(std::move(terms))); }
        return terms_.insert(make_locatable<PoolTerm>(loc, std::move(pool)));
    }
}

TermUid NongroundProgramBuilder::term(Location const &loc, TermVecUid args, bool forceTuple) {
    UTermVec a(termvecs_.erase(args));
    UTerm ret(!forceTuple && a.size() == 1 ? std::move(a.front()) : make_locatable<FunctionTerm>(loc, "", std::move(a)));
    return terms_.insert(std::move(ret));
}

TermUid NongroundProgramBuilder::pool(Location const &loc, TermVecUid args) {
    return terms_.insert(make_locatable<PoolTerm>(loc, termvecs_.erase(args)));
}

// {{{2 id vectors

IdVecUid NongroundProgramBuilder::idvec() {
    return idvecs_.emplace();
}
IdVecUid NongroundProgramBuilder::idvec(IdVecUid uid, Location const &loc, String id) {
    idvecs_[uid].emplace_back(loc, id);
    return uid;
}

// {{{2 csp

CSPMulTermUid NongroundProgramBuilder::cspmulterm(Location const &, TermUid coe, TermUid var) {
    return cspmulterms_.emplace(terms_.erase(var), terms_.erase(coe));
}
CSPMulTermUid NongroundProgramBuilder::cspmulterm(Location const &, TermUid coe) {
    return cspmulterms_.emplace(nullptr, terms_.erase(coe));
}
CSPAddTermUid NongroundProgramBuilder::cspaddterm(Location const &loc, CSPAddTermUid a, CSPMulTermUid b, bool add) {
    if (add) {
        cspaddterms_[a].append(cspmulterms_.erase(b));
    }
    else {
        CSPMulTerm mul(cspmulterms_.erase(b));
        mul.coe = make_locatable<UnOpTerm>(loc, UnOp::NEG, std::move(mul.coe));
        cspaddterms_[a].append(std::move(mul));
    }

    return a;
}
CSPAddTermUid NongroundProgramBuilder::cspaddterm(Location const &, CSPMulTermUid a) {
    return cspaddterms_.emplace(cspmulterms_.erase(a));
}
LitUid NongroundProgramBuilder::csplit(CSPLitUid a) {
    return lits_.emplace(csplits_.erase(a));
}
CSPLitUid NongroundProgramBuilder::csplit(Location const &loc, CSPLitUid a, Relation rel, CSPAddTermUid b) {
    csplits_[a]->append(rel, cspaddterms_.erase(b));
    csplits_[a]->loc(csplits_[a]->loc() + loc);
    return a;
}
CSPLitUid NongroundProgramBuilder::csplit(Location const &loc, CSPAddTermUid a, Relation rel, CSPAddTermUid b) {
    return csplits_.insert(make_locatable<CSPLiteral>(loc, rel, cspaddterms_.erase(a), cspaddterms_.erase(b)));
}

// {{{2 termvecs

TermVecUid NongroundProgramBuilder::termvec() {
    return termvecs_.emplace();
}

TermVecUid NongroundProgramBuilder::termvec(TermVecUid uid, TermUid term) {
    termvecs_[uid].emplace_back(terms_.erase(term));
    return uid;
}

// {{{2 termvecvecs

TermVecVecUid NongroundProgramBuilder::termvecvec() {
    return termvecvecs_.emplace();
}

TermVecVecUid NongroundProgramBuilder::termvecvec(TermVecVecUid uid, TermVecUid termvecUid) {
    termvecvecs_[uid].emplace_back(termvecs_.erase(termvecUid));
    return uid;
}

// {{{2 literals

LitUid NongroundProgramBuilder::boollit(Location const &loc, bool type) {
    return rellit(loc, type ? Relation::EQ : Relation::NEQ, term(loc, Symbol::createNum(0)), term(loc, Symbol::createNum(0)));
}

LitUid NongroundProgramBuilder::predlit(Location const &loc, NAF naf, TermUid term) {
    return lits_.insert(make_locatable<PredicateLiteral>(loc, naf, terms_.erase(term)));
}

LitUid NongroundProgramBuilder::rellit(Location const &loc, Relation rel, TermUid termUidLeft, TermUid termUidRight) {
    return lits_.insert(make_locatable<RelationLiteral>(loc, rel, terms_.erase(termUidLeft), terms_.erase(termUidRight)));
}

// {{{2 literal vectors

LitVecUid NongroundProgramBuilder::litvec() {
    return litvecs_.emplace();
}

LitVecUid NongroundProgramBuilder::litvec(LitVecUid uid, LitUid literalUid) {
    litvecs_[uid].emplace_back(lits_.erase(literalUid));
    return uid;
}

// {{{2 body aggregate element vectors

BdAggrElemVecUid NongroundProgramBuilder::bodyaggrelemvec() {
    return bodyaggrelemvecs_.emplace();
}

BdAggrElemVecUid NongroundProgramBuilder::bodyaggrelemvec(BdAggrElemVecUid uid, TermVecUid termvec, LitVecUid litvec) {
    bodyaggrelemvecs_[uid].emplace_back(termvecs_.erase(termvec), litvecs_.erase(litvec));
    return uid;
}

CondLitVecUid NongroundProgramBuilder::condlitvec() {
    return condlitvecs_.emplace();
}

CondLitVecUid NongroundProgramBuilder::condlitvec(CondLitVecUid uid, LitUid lit, LitVecUid litvec) {
    condlitvecs_[uid].emplace_back(lits_.erase(lit), litvecs_.erase(litvec));
    return uid;
}

// {{{2 head aggregate element vectors

HdAggrElemVecUid NongroundProgramBuilder::headaggrelemvec() {
    return headaggrelemvecs_.emplace();
}

HdAggrElemVecUid NongroundProgramBuilder::headaggrelemvec(HdAggrElemVecUid uid, TermVecUid termvec, LitUid lit, LitVecUid litvec) {
    headaggrelemvecs_[uid].emplace_back(termvecs_.erase(termvec), lits_.erase(lit), litvecs_.erase(litvec));
    return uid;
}

// {{{2 bounds

BoundVecUid NongroundProgramBuilder::boundvec() {
    return bounds_.emplace();
}

BoundVecUid NongroundProgramBuilder::boundvec(BoundVecUid uid, Relation rel, TermUid term) {
    bounds_[uid].emplace_back(rel, terms_.erase(term));
    return uid;
}

// {{{2 rule bodies

BdLitVecUid NongroundProgramBuilder::body() {
    return bodies_.emplace();
}

BdLitVecUid NongroundProgramBuilder::bodylit(BdLitVecUid body, LitUid bodylit) {
    bodies_[body].push_back(gringo_make_unique<SimpleBodyLiteral>(lits_.erase(bodylit)));
    return body;
}

BdLitVecUid NongroundProgramBuilder::bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, BdAggrElemVecUid bodyaggrelemvec) {
    bodies_[body].push_back(make_locatable<TupleBodyAggregate>(loc, naf, fun, bounds_.erase(bounds), bodyaggrelemvecs_.erase(bodyaggrelemvec)));
    return body;
}

BdLitVecUid NongroundProgramBuilder::bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid condlitvecuid) {
    bodies_[body].push_back(make_locatable<LitBodyAggregate>(loc, naf, fun, bounds_.erase(bounds), condlitvecs_.erase(condlitvecuid)));
    return body;
}

BdLitVecUid NongroundProgramBuilder::conjunction(BdLitVecUid body, Location const &loc, LitUid head, LitVecUid litvec) {
    bodies_[body].push_back(make_locatable<Conjunction>(loc, lits_.erase(head), litvecs_.erase(litvec)));
    return body;
}

BdLitVecUid NongroundProgramBuilder::disjoint(BdLitVecUid body, Location const &loc, NAF naf, CSPElemVecUid elem) {
    bodies_[body].push_back(make_locatable<DisjointAggregate>(loc, naf, cspelems_.erase(elem)));
    return body;
}


// {{{2 rule heads

HdLitUid NongroundProgramBuilder::headlit(LitUid lit) {
    return heads_.insert(gringo_make_unique<SimpleHeadLiteral>(lits_.erase(lit)));
}

HdLitUid NongroundProgramBuilder::headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, HdAggrElemVecUid headaggrelemvec) {
    return heads_.insert(make_locatable<TupleHeadAggregate>(loc, fun, bounds_.erase(bounds), headaggrelemvecs_.erase(headaggrelemvec)));
}

HdLitUid NongroundProgramBuilder::headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid condlitvec) {
    return heads_.insert(make_locatable<LitHeadAggregate>(loc, fun, bounds_.erase(bounds), condlitvecs_.erase(condlitvec)));
}

HdLitUid NongroundProgramBuilder::disjunction(Location const &loc, CondLitVecUid condlitvec) {
    return heads_.insert(make_locatable<Disjunction>(loc, condlitvecs_.erase(condlitvec)));
}

// {{{2 csp constraint elements

CSPElemVecUid NongroundProgramBuilder::cspelemvec() {
    return cspelems_.emplace();
}

CSPElemVecUid NongroundProgramBuilder::cspelemvec(CSPElemVecUid uid, Location const &loc, TermVecUid termvec, CSPAddTermUid addterm, LitVecUid litvec) {
    cspelems_[uid].emplace_back(loc, termvecs_.erase(termvec), cspaddterms_.erase(addterm), litvecs_.erase(litvec));
    return uid;
}

// {{{2 statements

void NongroundProgramBuilder::rule(Location const &loc, HdLitUid head) {
    rule(loc, head, body());
}

void NongroundProgramBuilder::rule(Location const &loc, HdLitUid head, BdLitVecUid body) {
    prg_.add(make_locatable<Statement>(loc, heads_.erase(head), bodies_.erase(body), StatementType::RULE));
}

void NongroundProgramBuilder::define(Location const &loc, String name, TermUid value, bool defaultDef, Logger &log) {
    defs_.add(loc, name, terms_.erase(value), defaultDef, log);
}

void NongroundProgramBuilder::optimize(Location const &loc, TermUid weight, TermUid priority, TermVecUid cond, BdLitVecUid body) {
    if (rewriteMinimize_) {
        auto argsUid = termvec(termvec(termvec(), priority), weight);
        termvec(argsUid, term(loc, cond, true));
        auto predUid = predlit(loc, NAF::POS, term(loc, "_criteria", termvecvec(termvecvec(), argsUid), false));
        rule(loc, headlit(predUid), body);
        out.outPredsForce.emplace_back(loc, Sig("_criteria", 3, false), false);
    }
    else {
        prg_.add(make_locatable<Statement>(loc, make_locatable<MinimizeHeadLiteral>(loc, terms_.erase(weight), terms_.erase(priority), termvecs_.erase(cond)), bodies_.erase(body), StatementType::WEAKCONSTRAINT));
    }
}

void NongroundProgramBuilder::showsig(Location const &loc, Sig sig, bool csp) {
    out.outPreds.emplace_back(loc, sig, csp);
}

void NongroundProgramBuilder::show(Location const &loc, TermUid t, BdLitVecUid body, bool csp) {
    prg_.add(make_locatable<Statement>(loc, make_locatable<ShowHeadLiteral>(loc, terms_.erase(t), csp), bodies_.erase(body), StatementType::RULE));
}

void NongroundProgramBuilder::lua(Location const &loc, String code) {
    scripts_.luaExec(loc, code);
}

void NongroundProgramBuilder::python(Location const &loc, String code) {
    scripts_.pyExec(loc, code);
}

void NongroundProgramBuilder::block(Location const &loc, String name, IdVecUid args) {
    prg_.begin(loc, name, idvecs_.erase(args));
}

void NongroundProgramBuilder::external(Location const &loc, TermUid head, BdLitVecUid body) {
    prg_.add(make_locatable<Statement>(loc, heads_.erase(headlit(predlit(loc, NAF::POS, head))), bodies_.erase(body), StatementType::EXTERNAL));
}

void NongroundProgramBuilder::edge(Location const &loc, TermVecVecUid edgesUid, BdLitVecUid body) {
    auto edges = termvecvecs_.erase(edgesUid);
    for (auto it = edges.begin(), end = edges.end(), last = end-1; it != end; ++it) {
        prg_.add(make_locatable<Statement>(
            loc,
            make_locatable<EdgeHeadAtom>(
                loc,
                std::move(it->front()),
                std::move(it->back())
            ),
            it == last ? bodies_.erase(body) : get_clone(bodies_[body]),
            StatementType::RULE
        ));
    }
}

void NongroundProgramBuilder::heuristic(Location const &loc, TermUid termUid, BdLitVecUid body, TermUid a, TermUid b, TermUid mod) {
    prg_.add(make_locatable<Statement>(loc, make_locatable<HeuristicHeadAtom>(loc, terms_.erase(termUid), terms_.erase(a), terms_.erase(b), terms_.erase(mod)), bodies_.erase(body), StatementType::RULE));
}

void NongroundProgramBuilder::project(Location const &loc, TermUid termUid, BdLitVecUid body) {
    prg_.add(make_locatable<Statement>(loc, make_locatable<ProjectHeadAtom>(loc, terms_.erase(termUid)), bodies_.erase(body), StatementType::RULE));
}

void NongroundProgramBuilder::project(Location const &loc, Sig sig) {
    Sig s = sig;
    auto tv = termvec();
    for (unsigned i = 0; i < s.arity(); ++i) {
        std::ostringstream ss;
        ss << "X" << i;
        tv = termvec(tv, term(loc, ss.str().c_str()));
    }
    project(loc, predRep(loc, s.sign(), s.name(), termvecvec(termvecvec(), tv)), body());
}

// {{{2 theory

TheoryTermUid NongroundProgramBuilder::theorytermset(Location const &loc, TheoryOptermVecUid args) {
    (void)loc;
    return theoryTerms_.emplace(gringo_make_unique<Output::TupleTheoryTerm>(Output::TupleTheoryTerm::Type::Brace, theoryOptermVecs_.erase(args)));
}

TheoryTermUid NongroundProgramBuilder::theoryoptermlist(Location const &loc, TheoryOptermVecUid args) {
    (void)loc;
    return theoryTerms_.emplace(gringo_make_unique<Output::TupleTheoryTerm>(Output::TupleTheoryTerm::Type::Bracket, theoryOptermVecs_.erase(args)));
}

TheoryTermUid NongroundProgramBuilder::theorytermtuple(Location const &loc, TheoryOptermVecUid args) {
    (void)loc;
    return theoryTerms_.emplace(gringo_make_unique<Output::TupleTheoryTerm>(Output::TupleTheoryTerm::Type::Paren, theoryOptermVecs_.erase(args)));
}

TheoryTermUid NongroundProgramBuilder::theorytermopterm(Location const &loc, TheoryOptermUid opterm) {
    (void)loc;
    return theoryTerms_.emplace(gringo_make_unique<Output::RawTheoryTerm>(theoryOpterms_.erase(opterm)));
}

TheoryTermUid NongroundProgramBuilder::theorytermfun(Location const &loc, String name, TheoryOptermVecUid args) {
    (void)loc;
    return theoryTerms_.emplace(gringo_make_unique<Output::FunctionTheoryTerm>(name, theoryOptermVecs_.erase(args)));
}

TheoryTermUid NongroundProgramBuilder::theorytermvalue(Location const &loc, Symbol val) {
    return theoryTerms_.emplace(gringo_make_unique<Output::TermTheoryTerm>(make_locatable<ValTerm>(loc, val)));
}

TheoryTermUid NongroundProgramBuilder::theorytermvar(Location const &loc, String var) {
    auto &ret(vals_[var]);
    if (!ret) { ret = std::make_shared<Symbol>(); }
    return theoryTerms_.emplace(gringo_make_unique<Output::TermTheoryTerm>(make_locatable<VarTerm>(loc, var, ret)));
}

TheoryOptermUid NongroundProgramBuilder::theoryopterm(TheoryOpVecUid ops, TheoryTermUid term) {
    auto ret = theoryOpterms_.emplace();
    theoryOpterms_[ret].append(theoryOpVecs_.erase(ops), theoryTerms_.erase(term));
    return ret;
}

TheoryOptermUid NongroundProgramBuilder::theoryopterm(TheoryOptermUid opterm, TheoryOpVecUid ops, TheoryTermUid term) {
    theoryOpterms_[opterm].append(theoryOpVecs_.erase(ops), theoryTerms_.erase(term));
    return opterm;
}

TheoryOpVecUid NongroundProgramBuilder::theoryops() {
    return theoryOpVecs_.emplace();
}

TheoryOpVecUid NongroundProgramBuilder::theoryops(TheoryOpVecUid ops, String op) {
    theoryOpVecs_[ops].emplace_back(op);
    return ops;
}

TheoryOptermVecUid NongroundProgramBuilder::theoryopterms() {
    return theoryOptermVecs_.emplace();
}
TheoryOptermVecUid NongroundProgramBuilder::theoryopterms(TheoryOptermVecUid opterms, Location const &, TheoryOptermUid opterm) {
    theoryOptermVecs_[opterms].emplace_back(gringo_make_unique<Output::RawTheoryTerm>(theoryOpterms_.erase(opterm)));
    return opterms;
}
TheoryOptermVecUid NongroundProgramBuilder::theoryopterms(Location const &, TheoryOptermUid opterm, TheoryOptermVecUid opterms) {
    theoryOptermVecs_[opterms].insert(theoryOptermVecs_[opterms].begin(), gringo_make_unique<Output::RawTheoryTerm>(theoryOpterms_.erase(opterm)));
    return opterms;
}

TheoryElemVecUid NongroundProgramBuilder::theoryelems() {
    return theoryElems_.emplace();
}
TheoryElemVecUid NongroundProgramBuilder::theoryelems(TheoryElemVecUid elems, TheoryOptermVecUid opterms, LitVecUid cond) {
    theoryElems_[elems].emplace_back(theoryOptermVecs_.erase(opterms), litvecs_.erase(cond));
    return elems;
}

TheoryAtomUid NongroundProgramBuilder::theoryatom(TermUid term, TheoryElemVecUid elems) {
    return theoryAtoms_.emplace(terms_.erase(term), theoryElems_.erase(elems));
}
TheoryAtomUid NongroundProgramBuilder::theoryatom(TermUid term, TheoryElemVecUid elems, String op, Location const &, TheoryOptermUid opterm) {
    return theoryAtoms_.emplace(terms_.erase(term), theoryElems_.erase(elems), op, gringo_make_unique<Output::RawTheoryTerm>(theoryOpterms_.erase(opterm)));
}

BdLitVecUid NongroundProgramBuilder::bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, TheoryAtomUid atom) {
    bodies_[body].emplace_back(make_locatable<BodyTheoryLiteral>(loc, naf, theoryAtoms_.erase(atom)));
    return body;
}

HdLitUid NongroundProgramBuilder::headaggr(Location const &loc, TheoryAtomUid atom) {
    return heads_.emplace(make_locatable<HeadTheoryLiteral>(loc, theoryAtoms_.erase(atom)));
}

// {{{2 theory definitions

TheoryOpDefUid NongroundProgramBuilder::theoryopdef(Location const &loc, String op, unsigned priority, TheoryOperatorType type) {
    return theoryOpDefs_.emplace(loc, op, priority, type);
}

TheoryOpDefVecUid NongroundProgramBuilder::theoryopdefs() {
    return theoryOpDefVecs_.emplace();
}

TheoryOpDefVecUid NongroundProgramBuilder::theoryopdefs(TheoryOpDefVecUid defs, TheoryOpDefUid def) {
    theoryOpDefVecs_[defs].emplace_back(theoryOpDefs_.erase(def));
    return defs;
}

TheoryTermDefUid NongroundProgramBuilder::theorytermdef(Location const &loc, String name, TheoryOpDefVecUid defs, Logger &log) {
    TheoryTermDef def(loc, name);
    for (auto &opDef : theoryOpDefVecs_.erase(defs)) {
        def.addOpDef(std::move(opDef), log);
    }
    return theoryTermDefs_.emplace(std::move(def));
}

TheoryAtomDefUid NongroundProgramBuilder::theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type) {
    return theoryAtomDefs_.emplace(loc, name, arity, termDef, type);
}

TheoryAtomDefUid NongroundProgramBuilder::theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type, TheoryOpVecUid ops, String guardDef) {
    return theoryAtomDefs_.emplace(loc, name, arity, termDef, type, theoryOpVecs_.erase(ops), guardDef);
}

TheoryDefVecUid NongroundProgramBuilder::theorydefs() {
    return theoryDefVecs_.emplace();
}

TheoryDefVecUid NongroundProgramBuilder::theorydefs(TheoryDefVecUid defs, TheoryTermDefUid def) {
    theoryDefVecs_[defs].first.emplace_back(theoryTermDefs_.erase(def));
    return defs;
}

TheoryDefVecUid NongroundProgramBuilder::theorydefs(TheoryDefVecUid defs, TheoryAtomDefUid def) {
    theoryDefVecs_[defs].second.emplace_back(theoryAtomDefs_.erase(def));
    return defs;
}

void NongroundProgramBuilder::theorydef(Location const &loc, String name, TheoryDefVecUid defs, Logger &log) {
    TheoryDef def(loc, name);
    auto defsVec = theoryDefVecs_.erase(defs);
    for (auto &termDef : defsVec.first) {
        def.addTermDef(std::move(termDef), log);
    }
    for (auto &atomDef : defsVec.second) {
        def.addAtomDef(std::move(atomDef), log);
    }
    prg_.add(std::move(def), log);
}

// }}}2

NongroundProgramBuilder::~NongroundProgramBuilder() { }

// {{{1 definition of ASTBuilder

clingo_location convertLoc(Location const &loc) {
    return clingo_location {
        loc.beginFilename.c_str(), loc.endFilename.c_str(),
        loc.beginLine, loc.endLine,
        loc.beginColumn, loc.endColumn
    };
}

Location convertLoc(clingo_location loc) {
    return {loc.begin_file, static_cast<unsigned>(loc.begin_line), static_cast<unsigned>(loc.begin_column)
           ,loc.end_file, static_cast<unsigned>(loc.end_line), static_cast<unsigned>(loc.end_column)};
}

template <class T>
T *ASTBuilder::create_() {
    data_.emplace_back(operator new(sizeof(T)));
    return reinterpret_cast<T*>(data_.back());
};
template <class T>
T *ASTBuilder::create_(T x) {
    auto *r = create_<T>();
    *r = x;
    return r;
};
template <class T>
T *ASTBuilder::createArray_(size_t size) {
    arrdata_.emplace_back(operator new[](sizeof(T) * size));
    return reinterpret_cast<T*>(arrdata_.back());
};
template <class T>
T *ASTBuilder::createArray_(std::vector<T> const &vec) {
    auto *r = createArray_<T>(vec.size());
    std::copy(vec.begin(), vec.end(), reinterpret_cast<T*>(r));
    return r;
};

void ASTBuilder::clear_() noexcept {
    for (auto &x : data_) { operator delete(x); }
    for (auto &x : arrdata_) { operator delete[](x); }
    data_.clear();
    arrdata_.clear();
}

void ASTBuilder::statement_(Location loc, clingo_ast_statement_type_t type, clingo_ast_statement_t &stm) {
    stm.location = convertLoc(loc);
    stm.type = type;
    cb_(stm);
    clear_();
}

ASTBuilder::ASTBuilder(Callback cb) : cb_(cb) { }
ASTBuilder::~ASTBuilder() noexcept {
    clear_();
}

// {{{2 terms

TermUid ASTBuilder::term(Location const &loc, Symbol val) {
    clingo_ast_term_t term;
    term.location = convertLoc(loc);
    term.type     = clingo_ast_term_type_symbol;
    term.symbol   = val.rep();
    return terms_.insert(std::move(term));
}

TermUid ASTBuilder::term(Location const &loc, String name) {
    clingo_ast_term_t term;
    term.location = convertLoc(loc);
    term.type     = clingo_ast_term_type_variable;
    term.variable = name.c_str();
    return terms_.insert(std::move(term));
}

TermUid ASTBuilder::term(Location const &loc, UnOp op, TermUid a) {
    auto unop = create_<clingo_ast_unary_operation_t>();
    unop->unary_operator = static_cast<clingo_ast_unary_operator_t>(op);
    unop->argument       = terms_.erase(a);
    clingo_ast_term_t term;
    term.location        = convertLoc(loc);
    term.type            = clingo_ast_term_type_unary_operation;
    term.unary_operation = unop;
    return terms_.insert(std::move(term));
}

TermUid ASTBuilder::pool_(Location const &loc, TermVec &&vec) {
    if (vec.size() == 1) {
        return terms_.insert(std::move(vec.front()));
    }
    else {
        auto pool = create_<clingo_ast_pool_t>();
        pool->size      = vec.size();
        pool->arguments = createArray_(vec);
        clingo_ast_term_t term;
        term.location = convertLoc(loc);
        term.type     = clingo_ast_term_type_pool;
        term.pool     = pool;
        return terms_.insert(std::move(term));
    }
}

TermUid ASTBuilder::term(Location const &loc, UnOp op, TermVecUid a) {
    return term(loc, op, pool_(loc, termvecs_.erase(a)));
}

TermUid ASTBuilder::term(Location const &loc, BinOp op, TermUid a, TermUid b) {
    auto *binop = create_<clingo_ast_binary_operation_t>();
    binop->binary_operator = static_cast<clingo_ast_binary_operator_t>(op);
    binop->left            = terms_.erase(a);
    binop->right           = terms_.erase(b);
    clingo_ast_term_t term;
    term.location         = convertLoc(loc);
    term.type             = clingo_ast_term_type_binary_operation;
    term.binary_operation = binop;
    return terms_.insert(std::move(term));
}

TermUid ASTBuilder::term(Location const &loc, TermUid a, TermUid b) {
    auto interval = create_<clingo_ast_interval_t>();
    interval->left  = terms_.erase(a);
    interval->right = terms_.erase(b);
    clingo_ast_term_t term;
    term.location = convertLoc(loc);
    term.type     = clingo_ast_term_type_interval;
    term.interval = interval;
    return terms_.insert(std::move(term));
}

clingo_ast_term_t ASTBuilder::fun_(Location const &loc, String name, TermVec &&vec, bool external) {
    auto fun = create_<clingo_ast_function_t>();
    fun->name      = name.c_str();
    fun->size      = vec.size();
    fun->arguments = createArray_(vec);
    clingo_ast_term_t term;
    term.location = convertLoc(loc);
    term.type     = external ? clingo_ast_term_type_external_function : clingo_ast_term_type_function;
    term.function = fun;
    return term;
}

TermUid ASTBuilder::term(Location const &loc, String name, TermVecVecUid a, bool lua) {
    TermVec pool;
    for (auto &args : termvecvecs_.erase(a)) {
        pool.emplace_back(fun_(loc, name, std::move(args), lua));
    }
    return pool_(loc, std::move(pool));
}

TermUid ASTBuilder::term(Location const &loc, TermVecUid a, bool forceTuple) {
    return terms_.insert((termvecs_[a].size() == 1 && !forceTuple)
        ? std::move(termvecs_.erase(a).front())
        : fun_(loc, "", termvecs_.erase(a), false));
}

TermUid ASTBuilder::pool(Location const &loc, TermVecUid a) {
    return pool_(loc, termvecs_.erase(a));
}

// {{{2 csp

CSPMulTermUid ASTBuilder::cspmulterm(Location const &loc, TermUid coe, TermUid var) {
    clingo_ast_csp_product_term_t term;
    term.location    = convertLoc(loc);
    term.coefficient = terms_.erase(coe);
    term.variable    = create_(terms_.erase(var));
    return cspmulterms_.insert(std::move(term));
}

CSPMulTermUid ASTBuilder::cspmulterm(Location const &loc, TermUid coe) {
    clingo_ast_csp_product_term_t term;
    term.location    = convertLoc(loc);
    term.coefficient = terms_.erase(coe);
    term.variable    = nullptr;
    return cspmulterms_.insert(std::move(term));
}

CSPAddTermUid ASTBuilder::cspaddterm(Location const &loc, CSPAddTermUid a, CSPMulTermUid b, bool add) {
    auto &addterm = cspaddterms_[a];
    addterm.first = loc;
    addterm.second.emplace_back(cspmulterms_.erase(b));
    if (!add) {
        auto unop = create_<clingo_ast_unary_operation_t>();
        unop->unary_operator = clingo_ast_unary_operator_minus;
        unop->argument       = addterm.second.back().coefficient;
        clingo_ast_term_t term;
        term.type            = clingo_ast_term_type_unary_operation;
        term.location        = addterm.second.back().coefficient.location;
        term.unary_operation = unop;
        addterm.second.back().coefficient = term;
    }
    return a;
}

CSPAddTermUid ASTBuilder::cspaddterm(Location const &loc, CSPMulTermUid b) {
    return cspaddterms_.emplace(loc, std::initializer_list<clingo_ast_csp_product_term_t>{cspmulterms_.erase(b)});
}

CSPLitUid ASTBuilder::csplit(Location const &loc, CSPLitUid a, Relation rel, CSPAddTermUid b) {
    auto &lit = csplits_[a];
    auto rep = cspaddterms_.erase(b);
    clingo_ast_csp_sum_term_t term;
    term.size     = rep.second.size();
    term.location = convertLoc(rep.first);
    term.terms    = createArray_(rep.second);
    lit.first = loc;
    lit.second.emplace_back(rel, std::move(term));
    return a;
}

CSPLitUid ASTBuilder::csplit(Location const &loc, CSPAddTermUid a, Relation rel, CSPAddTermUid b) {
    auto repa = cspaddterms_.erase(a), repb = cspaddterms_.erase(b);
    clingo_ast_csp_sum_term terma, termb;
    terma.location = convertLoc(repa.first);
    terma.size     = repa.second.size();
    terma.terms    = createArray_(repa.second);
    termb.location = convertLoc(repb.first);
    termb.size     = repb.second.size();
    termb.terms    = createArray_(repb.second);
    CSPLits::ValueType vec = { loc, {{rel, terma}, {rel, termb}} };
    return csplits_.insert(std::move(vec));
}

// {{{2 id vectors

IdVecUid ASTBuilder::idvec() {
    return idvecs_.emplace();
}

IdVecUid ASTBuilder::idvec(IdVecUid uid, Location const &loc, String id) {
    idvecs_[uid].emplace_back(clingo_ast_id_t{convertLoc(loc), id.c_str()});
    return uid;
}

// {{{2 term vectors
TermVecUid ASTBuilder::termvec() {
    return termvecs_.emplace();
}

TermVecUid ASTBuilder::termvec(TermVecUid uid, TermUid termUid) {
    termvecs_[uid].emplace_back(terms_.erase(termUid));
    return uid;
}

// {{{2 term vector vectors
TermVecVecUid ASTBuilder::termvecvec() {
    return termvecvecs_.emplace();
}

TermVecVecUid ASTBuilder::termvecvec(TermVecVecUid uid, TermVecUid termvecUid) {
    termvecvecs_[uid].emplace_back(termvecs_.erase(termvecUid));
    return uid;
}

// {{{2 literals

LitUid ASTBuilder::boollit(Location const &loc, bool type) {
    clingo_ast_literal_t lit;
    lit.location = convertLoc(loc);
    lit.sign     = clingo_ast_sign_none;
    lit.type     = clingo_ast_literal_type_boolean;
    lit.boolean  = type;
    return lits_.insert(std::move(lit));
}

LitUid ASTBuilder::predlit(Location const &loc, NAF naf, TermUid termUid) {
    clingo_ast_literal_t lit;
    lit.location = convertLoc(loc);
    lit.sign     = static_cast<clingo_ast_sign_t>(naf);
    lit.type     = clingo_ast_literal_type_symbolic;
    lit.symbol   = create_(terms_.erase(termUid));
    return lits_.insert(std::move(lit));
}

LitUid ASTBuilder::rellit(Location const &loc, Relation rel, TermUid termUidLeft, TermUid termUidRight) {
    auto comp = create_<clingo_ast_comparison_t>();
    comp->comparison = static_cast<clingo_ast_comparison_operator_t>(rel);
    comp->left       = terms_.erase(termUidLeft);
    comp->right      = terms_.erase(termUidRight);
    clingo_ast_literal_t lit;
    lit.location   = convertLoc(loc);
    lit.sign       = clingo_ast_sign_none;
    lit.type       = clingo_ast_literal_type_comparison;
    lit.comparison = comp;
    return lits_.insert(std::move(lit));
}

LitUid ASTBuilder::csplit(CSPLitUid a) {
    auto rep = csplits_.erase(a);
    assert(rep.second.size() > 1);
    auto guards = createArray_<clingo_ast_csp_guard_t>(rep.second.size()-1);
    auto guardsit = guards;
    for (auto it = rep.second.begin() + 1, ie = rep.second.end(); it != ie; ++it, ++guardsit) {
        guardsit->comparison = static_cast<clingo_ast_comparison_operator_t>(it->first);
        guardsit->term = it->second;
    }
    auto csp = create_<clingo_ast_csp_literal_t>();
    csp->term   = rep.second.front().second;
    csp->size   = rep.second.size() - 1;
    csp->guards = guards;
    clingo_ast_literal_t lit;
    lit.location = convertLoc(rep.first);
    lit.sign        = clingo_ast_sign_none;
    lit.type        = clingo_ast_literal_type_csp;
    lit.csp_literal = csp;
    return lits_.insert(std::move(lit));
}

// {{{2 literal vectors

LitVecUid ASTBuilder::litvec() {
    return litvecs_.emplace();
}

LitVecUid ASTBuilder::litvec(LitVecUid uid, LitUid literalUid) {
    litvecs_[uid].emplace_back(lits_.erase(literalUid));
    return uid;
}

// {{{2 conditional literals

CondLitVecUid ASTBuilder::condlitvec() {
    return condlitvecs_.emplace();
}

CondLitVecUid ASTBuilder::condlitvec(CondLitVecUid uid, LitUid litUid, LitVecUid litvecUid) {
    auto cond = litvecs_.erase(litvecUid);
    clingo_ast_conditional_literal_t lit;
    lit.size      = cond.size();
    lit.condition = createArray_(cond);
    lit.literal   = lits_.erase(litUid);
    condlitvecs_[uid].emplace_back(lit);
    return uid;
}

// {{{2 body aggregate elements

BdAggrElemVecUid ASTBuilder::bodyaggrelemvec() {
    return bodyaggrelemvecs_.emplace();
}

BdAggrElemVecUid ASTBuilder::bodyaggrelemvec(BdAggrElemVecUid uid, TermVecUid termvec, LitVecUid litvec) {
    clingo_ast_body_aggregate_element_t elem;
    auto cond = litvecs_.erase(litvec);
    auto tuple = termvecs_.erase(termvec);
    elem.condition_size = cond.size();
    elem.condition      = createArray_(cond);
    elem.tuple_size     = tuple.size();
    elem.tuple          = createArray_(tuple);
    bodyaggrelemvecs_[uid].emplace_back(elem);
    return uid;
}

// {{{2 head aggregate elements

HdAggrElemVecUid ASTBuilder::headaggrelemvec() {
    return headaggrelemvecs_.emplace();
}

HdAggrElemVecUid ASTBuilder::headaggrelemvec(HdAggrElemVecUid uid, TermVecUid termvec, LitUid litUid, LitVecUid litvec) {
    clingo_ast_head_aggregate_element_t elem;
    auto cond = litvecs_.erase(litvec);
    clingo_ast_conditional_literal_t lit;
    lit.size      = cond.size();
    lit.condition = createArray_(cond);
    lit.literal   = lits_.erase(litUid);
    auto tuple = termvecs_.erase(termvec);
    elem.conditional_literal = lit;
    elem.tuple_size          = tuple.size();
    elem.tuple               = createArray_(tuple);
    headaggrelemvecs_[uid].emplace_back(elem);
    return uid;
}

// {{{2 bounds

BoundVecUid ASTBuilder::boundvec() {
    return bounds_.emplace();
}

BoundVecUid ASTBuilder::boundvec(BoundVecUid uid, Relation rel, TermUid term) {
    clingo_ast_aggregate_guard_t guard;
    guard.term = terms_.erase(term);
    guard.comparison = static_cast<clingo_ast_comparison_operator_t>(rel);
    bounds_[uid].emplace_back(guard);
    return uid;
}

// {{{2 heads

HdLitUid ASTBuilder::headlit(LitUid litUid) {
    clingo_ast_head_literal_t head;
    head.type     = clingo_ast_head_literal_type_literal;
    head.literal  = create_(lits_.erase(litUid));
    head.location = head.literal->location;
    return heads_.insert(std::move(head));
}

HdLitUid ASTBuilder::headaggr(Location const &loc, TheoryAtomUid atomUid) {
    clingo_ast_head_literal_t head;
    head.location    = convertLoc(loc);
    head.type        = clingo_ast_head_literal_type_theory_atom;
    head.theory_atom = create_(theoryAtoms_.erase(atomUid));
    return heads_.insert(std::move(head));
}

HdLitUid ASTBuilder::headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, HdAggrElemVecUid headaggrelemvec) {
    auto guards = bounds_.erase(bounds);
    auto elems = headaggrelemvecs_.erase(headaggrelemvec);
    assert(guards.size() < 3);
    if (!guards.empty()) {
        guards.front().comparison = static_cast<clingo_ast_comparison_operator_t>(inv(static_cast<Relation>(guards.front().comparison)));
    }
    clingo_ast_head_aggregate_t aggr;
    aggr.function    = static_cast<clingo_ast_aggregate_function>(fun);
    aggr.size        = elems.size();
    aggr.elements    = createArray_(elems);
    aggr.left_guard  = guards.size() > 0 ? create_(guards[0]) : nullptr;
    aggr.right_guard = guards.size() > 1 ? create_(guards[1]) : nullptr;
    clingo_ast_head_literal_t head;
    head.location       = convertLoc(loc);
    head.type           = clingo_ast_head_literal_type_head_aggregate;
    head.head_aggregate = create_(aggr);
    return heads_.insert(std::move(head));
}

HdLitUid ASTBuilder::headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid headaggrelemvec) {
    (void)fun;
    assert(fun == AggregateFunction::COUNT);
    auto guards = bounds_.erase(bounds);
    auto elems = condlitvecs_.erase(headaggrelemvec);
    assert(guards.size() < 3);
    if (!guards.empty()) {
        guards.front().comparison = static_cast<clingo_ast_comparison_operator_t>(inv(static_cast<Relation>(guards.front().comparison)));
    }
    clingo_ast_aggregate_t aggr;
    aggr.size        = elems.size();
    aggr.elements    = createArray_(elems);
    aggr.left_guard  = guards.size() > 0 ? create_(guards[0]) : nullptr;
    aggr.right_guard = guards.size() > 1 ? create_(guards[1]) : nullptr;
    clingo_ast_head_literal_t head;
    head.location  = convertLoc(loc);
    head.type      = clingo_ast_head_literal_type_aggregate;
    head.aggregate = create_(aggr);
    return heads_.insert(std::move(head));
}

HdLitUid ASTBuilder::disjunction(Location const &loc, CondLitVecUid condlitvec) {
    auto elems = condlitvecs_.erase(condlitvec);
    clingo_ast_disjunction_t disj;
    disj.size     = elems.size();
    disj.elements = createArray_(elems);
    clingo_ast_head_literal_t head;
    head.location    = convertLoc(loc);
    head.type        = clingo_ast_head_literal_type_disjunction;
    head.disjunction = create_(disj);
    return heads_.insert(std::move(head));
}

// {{{2 bodies

BdLitVecUid ASTBuilder::body() {
    return bodies_.emplace();
}

BdLitVecUid ASTBuilder::bodylit(BdLitVecUid body, LitUid bodylit) {
    auto lit = lits_.erase(bodylit);
    clingo_ast_body_literal_t bd;
    bd.location = lit.location;
    bd.sign     = static_cast<clingo_ast_sign_t>(clingo_ast_sign_none);
    bd.type     = clingo_ast_body_literal_type_literal;;
    bd.literal  = create_(lit);
    bodies_[body].emplace_back(bd);
    return body;
}

BdLitVecUid ASTBuilder::bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, TheoryAtomUid atomUid) {
    clingo_ast_body_literal_t bd;
    bd.location    = convertLoc(loc);
    bd.sign        = static_cast<clingo_ast_sign_t>(naf);
    bd.type        = clingo_ast_body_literal_type_theory_atom;;
    bd.theory_atom = create_(theoryAtoms_.erase(atomUid));
    bodies_[body].emplace_back(bd);
    return body;
}

BdLitVecUid ASTBuilder::bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, BdAggrElemVecUid bodyaggrelemvec) {
    auto guards = bounds_.erase(bounds);
    auto elems = bodyaggrelemvecs_.erase(bodyaggrelemvec);
    assert(guards.size() < 3);
    if (!guards.empty()) {
        guards.front().comparison = static_cast<clingo_ast_comparison_operator_t>(inv(static_cast<Relation>(guards.front().comparison)));
    }
    clingo_ast_body_aggregate_t aggr;
    aggr.function    = static_cast<clingo_ast_aggregate_function>(fun);
    aggr.size        = elems.size();
    aggr.elements    = createArray_(elems);
    aggr.left_guard  = guards.size() > 0 ? create_(guards[0]) : nullptr;
    aggr.right_guard = guards.size() > 1 ? create_(guards[1]) : nullptr;
    clingo_ast_body_literal_t bd;
    bd.location       = convertLoc(loc);
    bd.sign           = static_cast<clingo_ast_sign_t>(naf);
    bd.type           = clingo_ast_body_literal_type_body_aggregate;
    bd.body_aggregate = create_(aggr);
    bodies_[body].emplace_back(bd);
    return body;
}

BdLitVecUid ASTBuilder::bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid bodyaggrelemvec) {
    (void)fun;
    assert(fun == AggregateFunction::COUNT);
    auto guards = bounds_.erase(bounds);
    auto elems = condlitvecs_.erase(bodyaggrelemvec);
    assert(guards.size() < 3);
    if (!guards.empty()) {
        guards.front().comparison = static_cast<clingo_ast_comparison_operator_t>(inv(static_cast<Relation>(guards.front().comparison)));
    }
    clingo_ast_aggregate_t aggr;
    aggr.size        = elems.size();
    aggr.elements    = createArray_(elems);
    aggr.left_guard  = guards.size() > 0 ? create_(guards[0]) : nullptr;
    aggr.right_guard = guards.size() > 1 ? create_(guards[1]) : nullptr;
    clingo_ast_body_literal_t bd;
    bd.location  = convertLoc(loc);
    bd.sign      = static_cast<clingo_ast_sign_t>(naf);
    bd.type      = clingo_ast_body_literal_type_aggregate;
    bd.aggregate = create_(aggr);
    bodies_[body].emplace_back(bd);
    return body;
}

BdLitVecUid ASTBuilder::conjunction(BdLitVecUid body, Location const &loc, LitUid head, LitVecUid litvec) {
    auto cond = litvecs_.erase(litvec);
    clingo_ast_conditional_literal_t lit;
    lit.literal   = lits_.erase(head);
    lit.size      = cond.size();
    lit.condition = createArray_(cond);
    clingo_ast_body_literal_t bd;
    bd.location    = convertLoc(loc);
    bd.sign        = clingo_ast_sign_none;
    bd.type        = clingo_ast_body_literal_type_conditional;
    bd.conditional = create_(lit);
    bodies_[body].emplace_back(bd);
    return body;
}

BdLitVecUid ASTBuilder::disjoint(BdLitVecUid body, Location const &loc, NAF naf, CSPElemVecUid elem) {
    auto elems = cspelems_.erase(elem);
    clingo_ast_disjoint_t disj;
    disj.size     = elems.size();
    disj.elements = createArray_(elems);
    clingo_ast_body_literal_t bd;
    bd.location    = convertLoc(loc);
    bd.sign        = static_cast<clingo_ast_sign_t>(naf);
    bd.type        = clingo_ast_body_literal_type_disjoint;
    bd.disjoint    = create_(disj);
    bodies_[body].emplace_back(bd);
    return body;
}

// {{{2 csp constraint elements

CSPElemVecUid ASTBuilder::cspelemvec() {
    return cspelems_.emplace();
}

CSPElemVecUid ASTBuilder::cspelemvec(CSPElemVecUid uid, Location const &loc, TermVecUid termvec, CSPAddTermUid addterm, LitVecUid litvec) {
    auto tuple = termvecs_.erase(termvec);
    auto rep = cspaddterms_.erase(addterm);
    auto cond = litvecs_.erase(litvec);
    clingo_ast_csp_sum_term_t term;
    term.size     = rep.second.size();
    term.location = convertLoc(rep.first);
    term.terms    = createArray_(rep.second);
    clingo_ast_disjoint_element_t elem;
    elem.term           = term;
    elem.location       = convertLoc(loc);
    elem.tuple          = createArray_(tuple);
    elem.tuple_size     = tuple.size();
    elem.condition      = createArray_(cond);
    elem.condition_size = cond.size();
    cspelems_[uid].emplace_back(elem);
    return uid;
}

// {{{2 statements

void ASTBuilder::rule(Location const &loc, HdLitUid head) {
    rule(loc, head, body());
}

void ASTBuilder::rule(Location const &loc, HdLitUid head, BdLitVecUid body) {
    auto lits = bodies_.erase(body);
    clingo_ast_rule_t rule;
    rule.head = heads_.erase(head);
    rule.size = lits.size();
    rule.body = createArray_(lits);
    clingo_ast_statement stm;
    stm.rule = create_(rule);
    statement_(loc, clingo_ast_statement_type_rule, stm);
}

void ASTBuilder::define(Location const &loc, String name, TermUid value, bool defaultDef, Logger &) {
    clingo_ast_definition_t def;
    def.name = name.c_str();
    def.value = terms_.erase(value);
    def.is_default = defaultDef;
    clingo_ast_statement stm;
    stm.definition = create_(def);
    statement_(loc, clingo_ast_statement_type_const, stm);
}

void ASTBuilder::optimize(Location const &loc, TermUid weight, TermUid priority, TermVecUid cond, BdLitVecUid body) {
    auto bd = bodies_.erase(body);
    auto tuple = termvecs_.erase(cond);
    clingo_ast_minimize_t min;
    min.weight     = terms_.erase(weight);
    min.priority   = terms_.erase(priority);
    min.body_size  = bd.size();
    min.body       = createArray_(bd);
    min.tuple_size = tuple.size();
    min.tuple      = createArray_(tuple);
    clingo_ast_statement stm;
    stm.minimize = create_(min);
    statement_(loc, clingo_ast_statement_type_minimize, stm);
}

void ASTBuilder::showsig(Location const &loc, Sig sig, bool csp) {
    clingo_ast_show_signature_t show;
    show.signature = sig.rep();
    show.csp = csp;
    clingo_ast_statement stm;
    stm.show_signature = create_(show);
    statement_(loc, clingo_ast_statement_type_show_signature, stm);
}

void ASTBuilder::show(Location const &loc, TermUid t, BdLitVecUid body, bool csp) {
    auto bd = bodies_.erase(body);
    clingo_ast_show_term_t show;
    show.term = terms_.erase(t);
    show.csp  = csp;
    show.body = createArray_(bd);
    show.size = bd.size();
    clingo_ast_statement stm;
    stm.show_term = create_(show);
    statement_(loc, clingo_ast_statement_type_show_term, stm);
}

void ASTBuilder::python(Location const &loc, String code) {
    clingo_ast_script_t script;
    script.code = code.c_str();
    script.type = clingo_ast_script_type_python;
    clingo_ast_statement stm;
    stm.script = create_(script);
    statement_(loc, clingo_ast_statement_type_script, stm);
}

void ASTBuilder::lua(Location const &loc, String code) {
    clingo_ast_script_t script;
    script.code = code.c_str();
    script.type = clingo_ast_script_type_lua;
    clingo_ast_statement stm;
    stm.script = create_(script);
    statement_(loc, clingo_ast_statement_type_script, stm);
}

void ASTBuilder::block(Location const &loc, String name, IdVecUid args) {
    auto params = idvecs_.erase(args);
    clingo_ast_program_t program;
    program.name       = name.c_str();
    program.parameters = createArray_(params);
    program.size       = params.size();
    clingo_ast_statement stm;
    stm.program = create_(program);
    statement_(loc, clingo_ast_statement_type_program, stm);
}

void ASTBuilder::external(Location const &loc, TermUid head, BdLitVecUid body) {
    auto bd = bodies_.erase(body);
    clingo_ast_external_t ext;
    ext.atom = terms_.erase(head);
    ext.body = createArray_(bd);
    ext.size = bd.size();
    clingo_ast_statement stm;
    stm.external = create_(ext);
    statement_(loc, clingo_ast_statement_type_external, stm);
}

void ASTBuilder::edge(Location const &loc, TermVecVecUid edges, BdLitVecUid body) {
    auto bd = bodies_.erase(body);
    for (auto &x : termvecvecs_.erase(edges)) {
        assert(x.size() == 2);
        clingo_ast_edge_t edge;
        edge.u = x[0];
        edge.v = x[1];
        edge.size = bd.size();
        edge.body = createArray_(bd);
        clingo_ast_statement stm;
        stm.location = convertLoc(loc);
        stm.type     = clingo_ast_statement_type_edge;
        stm.edge     = create_(edge);
        cb_(stm);
    }
    clear_();
}

void ASTBuilder::heuristic(Location const &loc, TermUid termUid, BdLitVecUid body, TermUid a, TermUid b, TermUid mod) {
    auto bd = bodies_.erase(body);
    clingo_ast_heuristic_t heu;
    heu.atom     = terms_.erase(termUid);
    heu.bias     = terms_.erase(a);
    heu.priority = terms_.erase(b);
    heu.modifier = terms_.erase(mod);
    heu.body     = createArray_(bd);
    heu.size     = bd.size();
    clingo_ast_statement stm;
    stm.heuristic = create_(heu);
    statement_(loc, clingo_ast_statement_type_heuristic, stm);
}

void ASTBuilder::project(Location const &loc, TermUid termUid, BdLitVecUid body) {
    auto bd = bodies_.erase(body);
    clingo_ast_project_t proj;
    proj.size = bd.size();
    proj.body = createArray_(bd);
    proj.atom = terms_.erase(termUid);
    clingo_ast_statement stm;
    stm.project_atom = create_(proj);
    statement_(loc, clingo_ast_statement_type_project_atom, stm);
}

void ASTBuilder::project(Location const &loc, Sig sig) {
    clingo_ast_statement stm;
    stm.project_signature = sig.rep();
    statement_(loc, clingo_ast_statement_type_project_atom_signature, stm);
}

// {{{2 theory atoms

TheoryTermUid ASTBuilder::theorytermarr_(Location const &loc, TheoryOptermVecUid args, clingo_ast_theory_term_type_t type) {
    auto a = theoryOptermVecs_.erase(args);
    clingo_ast_theory_term_array_t arr;
    arr.size  = a.size();
    arr.terms = createArray_(a);
    clingo_ast_theory_term_t term;
    term.type     = type;
    term.location = convertLoc(loc);
    term.set      = create_(arr);
    return theoryTerms_.insert(std::move(term));
}

TheoryTermUid ASTBuilder::theorytermset(Location const &loc, TheoryOptermVecUid args) {
    return theorytermarr_(loc, args, clingo_ast_theory_term_type_set);
}

TheoryTermUid ASTBuilder::theoryoptermlist(Location const &loc, TheoryOptermVecUid args) {
    return theorytermarr_(loc, args, clingo_ast_theory_term_type_list);
}

TheoryTermUid ASTBuilder::theorytermtuple(Location const &loc, TheoryOptermVecUid args) {
    return theorytermarr_(loc, args, clingo_ast_theory_term_type_tuple);
}

clingo_ast_theory_term_t ASTBuilder::opterm_(Location const &loc, TheoryOptermUid opterm) {
    auto terms = theoryOpterms_.erase(opterm);
    clingo_ast_theory_unparsed_term arr;
    arr.size     = terms.size();
    arr.elements = createArray_(terms);
    clingo_ast_theory_term_t term;
    term.location      = convertLoc(loc);
    term.type          = clingo_ast_theory_term_type_unparsed_term;
    term.unparsed_term = create_(arr);
    return term;
}

TheoryTermUid ASTBuilder::theorytermopterm(Location const &loc, TheoryOptermUid opterm) {
    return theoryTerms_.insert(opterm_(loc, opterm));
}

TheoryTermUid ASTBuilder::theorytermfun(Location const &loc, String name, TheoryOptermVecUid args) {
    auto a = theoryOptermVecs_.erase(args);
    clingo_ast_theory_function_t fun;
    fun.name      = name.c_str();
    fun.size      = a.size();
    fun.arguments = createArray_(a);
    clingo_ast_theory_term_t term;
    term.type     = clingo_ast_theory_term_type_function;
    term.location = convertLoc(loc);
    term.function = create_(fun);
    return theoryTerms_.insert(std::move(term));
}

TheoryTermUid ASTBuilder::theorytermvalue(Location const &loc, Symbol val) {
    clingo_ast_theory_term_t term;
    term.type     = clingo_ast_theory_term_type_symbol;
    term.location = convertLoc(loc);
    term.symbol   = val.rep();
    return theoryTerms_.insert(std::move(term));
}

TheoryTermUid ASTBuilder::theorytermvar(Location const &loc, String var) {
    clingo_ast_theory_term_t term;
    term.type     = clingo_ast_theory_term_type_variable;
    term.location = convertLoc(loc);
    term.variable = var.c_str();
    return theoryTerms_.insert(std::move(term));
}

clingo_ast_theory_unparsed_term_element_t ASTBuilder::opterm_(TheoryOpVecUid ops, TheoryTermUid term) {
    auto o = theoryOpVecs_.erase(ops);
    clingo_ast_theory_unparsed_term_element_t t;
    t.size      = o.size();
    t.operators = createArray_(o);
    t.term      = theoryTerms_.erase(term);
    return t;
}

TheoryOptermUid ASTBuilder::theoryopterm(TheoryOpVecUid ops, TheoryTermUid term) {
    return theoryOpterms_.insert({opterm_(ops, term)});
}

TheoryOptermUid ASTBuilder::theoryopterm(TheoryOptermUid opterm, TheoryOpVecUid ops, TheoryTermUid term) {
    theoryOpterms_[opterm].emplace_back(opterm_(ops, term));
    return opterm;
}

TheoryOpVecUid ASTBuilder::theoryops() {
    return theoryOpVecs_.emplace();
}

TheoryOpVecUid ASTBuilder::theoryops(TheoryOpVecUid ops, String op) {
    theoryOpVecs_[ops].emplace_back(op.c_str());
    return ops;
}

TheoryOptermVecUid ASTBuilder::theoryopterms() {
    return theoryOptermVecs_.emplace();
}

TheoryOptermVecUid ASTBuilder::theoryopterms(TheoryOptermVecUid opterms, Location const &loc, TheoryOptermUid opterm) {
    theoryOptermVecs_[opterms].emplace_back(opterm_(loc, opterm));
    return opterms;
}

TheoryOptermVecUid ASTBuilder::theoryopterms(Location const &loc, TheoryOptermUid opterm, TheoryOptermVecUid opterms) {
    theoryOptermVecs_[opterms].emplace(theoryOptermVecs_[opterms].begin(), opterm_(loc, opterm));
    return opterms;
}

TheoryElemVecUid ASTBuilder::theoryelems() {
    return theoryElems_.emplace();
}

TheoryElemVecUid ASTBuilder::theoryelems(TheoryElemVecUid elems, TheoryOptermVecUid opterms, LitVecUid cond) {
    auto ops = theoryOptermVecs_.erase(opterms);
    auto cnd = litvecs_.erase(cond);
    clingo_ast_theory_atom_element_t elem;
    elem.tuple_size     = ops.size();
    elem.tuple          = createArray_(ops);
    elem.condition_size = cnd.size();
    elem.condition      = createArray_(cnd);
    theoryElems_[elems].emplace_back(elem);
    return elems;
}

TheoryAtomUid ASTBuilder::theoryatom(TermUid term, TheoryElemVecUid elems) {
    auto elms = theoryElems_.erase(elems);
    clingo_ast_theory_atom_t atom;
    atom.term     = terms_.erase(term);
    atom.elements = createArray_(elms);
    atom.size     = elms.size();
    atom.guard    = nullptr;
    return theoryAtoms_.insert(std::move(atom));
}

TheoryAtomUid ASTBuilder::theoryatom(TermUid term, TheoryElemVecUid elems, String op, Location const &loc, TheoryOptermUid opterm) {
    auto elms = theoryElems_.erase(elems);
    clingo_ast_theory_guard_t guard;
    guard.term          = opterm_(loc, opterm);
    guard.operator_name = op.c_str();
    clingo_ast_theory_atom_t atom;
    atom.term     = terms_.erase(term);
    atom.elements = createArray_(elms);
    atom.size     = elms.size();
    atom.guard    = create_(guard);
    return theoryAtoms_.insert(std::move(atom));
}

// {{{2 theory definitions

TheoryOpDefUid ASTBuilder::theoryopdef(Location const &loc, String op, unsigned priority, TheoryOperatorType type) {
    clingo_ast_theory_operator_definition_t def;
    def.location = convertLoc(loc);
    def.priority = priority;
    def.name     = op.c_str();
    def.type     = static_cast<clingo_ast_theory_operator_type_t>(type);
    return theoryOpDefs_.insert(std::move(def));
}

TheoryOpDefVecUid ASTBuilder::theoryopdefs() {
    return theoryOpDefVecs_.emplace();
}

TheoryOpDefVecUid ASTBuilder::theoryopdefs(TheoryOpDefVecUid defs, TheoryOpDefUid def) {
    theoryOpDefVecs_[defs].emplace_back(theoryOpDefs_.erase(def));
    return defs;
}

TheoryTermDefUid ASTBuilder::theorytermdef(Location const &loc, String name, TheoryOpDefVecUid defs, Logger &) {
    auto dfs = theoryOpDefVecs_.erase(defs);
    clingo_ast_theory_term_definition_t def;
    def.location  = convertLoc(loc);
    def.name      = name.c_str();
    def.operators = createArray_(dfs);
    def.size      = dfs.size();
    return theoryTermDefs_.insert(std::move(def));
}

TheoryAtomDefUid ASTBuilder::theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type) {
    clingo_ast_theory_atom_definition_t def;
    def.location = convertLoc(loc);
    def.name     = name.c_str();
    def.arity    = arity;
    def.elements = termDef.c_str();
    def.type     = static_cast<clingo_ast_theory_atom_definition_type_t>(type);
    def.guard    = nullptr;
    return theoryAtomDefs_.insert(std::move(def));
}

TheoryAtomDefUid ASTBuilder::theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type, TheoryOpVecUid ops, String guardDef) {
    auto o = theoryOpVecs_.erase(ops);
    clingo_ast_theory_guard_definition_t guard;
    guard.guard     = guardDef.c_str();
    guard.operators = createArray_(o);
    guard.size      = o.size();
    clingo_ast_theory_atom_definition_t def;
    def.location = convertLoc(loc);
    def.name     = name.c_str();
    def.arity    = arity;
    def.elements = termDef.c_str();
    def.type     = static_cast<clingo_ast_theory_atom_definition_type_t>(type);
    def.guard    = create_(guard);
    return theoryAtomDefs_.insert(std::move(def));
}

TheoryDefVecUid ASTBuilder::theorydefs() {
    return theoryDefVecs_.emplace();
}

TheoryDefVecUid ASTBuilder::theorydefs(TheoryDefVecUid defs, TheoryTermDefUid def) {
    theoryDefVecs_[defs].first.emplace_back(theoryTermDefs_.erase(def));
    return defs;
}

TheoryDefVecUid ASTBuilder::theorydefs(TheoryDefVecUid defs, TheoryAtomDefUid def) {
    theoryDefVecs_[defs].second.emplace_back(theoryAtomDefs_.erase(def));
    return defs;
}

void ASTBuilder::theorydef(Location const &loc, String name, TheoryDefVecUid defs, Logger &) {
    auto d = theoryDefVecs_.erase(defs);
    clingo_ast_theory_definition_t def;
    def.name = name.c_str();
    def.terms_size = d.first.size();
    def.terms      = createArray_(d.first);
    def.atoms_size = d.second.size();
    def.atoms      = createArray_(d.second);
    clingo_ast_statement stm;
    stm.theory_definition = create_(def);
    statement_(loc, clingo_ast_statement_type_theory_definition, stm);
}

// }}}2

// {{{1 ast parsing

namespace {

struct ASTParser {
public:
    ASTParser(Logger &log, INongroundProgramBuilder &prg)
    : log_(log), prg_(prg) { }

    // {{{2 statement

    void parse(clingo_ast_statement_t const &stm) {
        switch (static_cast<enum clingo_ast_statement_type>(stm.type)) {
            case clingo_ast_statement_type_rule: {
                auto &y = *stm.rule;
                return prg_.rule(parseLocation(stm.location), parseHeadLiteral(y.head), parseBodyLiteralVec(y.body, y.size));
            }
            case clingo_ast_statement_type_const: {
                auto &y = *stm.definition;
                return prg_.define(parseLocation(stm.location), y.name, parseTerm(y.value), y.is_default, log_);
            }
            case clingo_ast_statement_type_show_signature: {
                auto &y = *stm.show_signature;
                return prg_.showsig(parseLocation(stm.location), Sig(y.signature), y.csp);
            }
            case clingo_ast_statement_type_show_term: {
                auto &y = *stm.show_term;
                return prg_.show(parseLocation(stm.location), parseTerm(y.term), parseBodyLiteralVec(y.body, y.size), y.csp);
            }
            case clingo_ast_statement_type_minimize: {
                auto &y = *stm.minimize;
                return prg_.optimize(parseLocation(stm.location), parseTerm(y.weight), parseTerm(y.priority), parseTermVec(y.tuple, y.tuple_size), parseBodyLiteralVec(y.body, y.body_size));
            }
            case clingo_ast_statement_type_script: {
                auto &y = *stm.script;
                switch (y.type) {
                    case clingo_ast_script_type_python: { return prg_.python(parseLocation(stm.location), y.code); }
                    case clingo_ast_script_type_lua:    { return prg_.lua(parseLocation(stm.location), y.code); }
                }
                break;
            }
            case clingo_ast_statement_type_program: {
                auto &y = *stm.program;
                return prg_.block(parseLocation(stm.location), y.name, parseIdVec(y.parameters, y.size));
            }
            case clingo_ast_statement_type_external: {
                auto &y = *stm.external;
                return prg_.external(parseLocation(stm.location), parseTerm(y.atom), parseBodyLiteralVec(y.body, y.size));
            }
            case clingo_ast_statement_type_edge: {
                auto &y = *stm.edge;
                return prg_.edge(parseLocation(stm.location), prg_.termvecvec(prg_.termvecvec(), prg_.termvec(prg_.termvec(prg_.termvec(), parseTerm(y.u)), parseTerm(y.v))), parseBodyLiteralVec(y.body, y.size));
            }
            case clingo_ast_statement_type_heuristic: {
                auto &y = *stm.heuristic;
                return prg_.heuristic(parseLocation(stm.location), parseTerm(y.atom), parseBodyLiteralVec(y.body, y.size), parseTerm(y.bias), parseTerm(y.priority), parseTerm(y.modifier));
            }
            case clingo_ast_statement_type_project_atom: {
                auto &y = *stm.project_atom;
                return prg_.project(parseLocation(stm.location), parseTerm(y.atom), parseBodyLiteralVec(y.body, y.size));
            }
            case clingo_ast_statement_type_project_atom_signature: {
                return prg_.project(parseLocation(stm.location), Sig(stm.project_signature));
            }
            case clingo_ast_statement_type_theory_definition: {
                auto &y = *stm.theory_definition;
                return prg_.theorydef(parseLocation(stm.location), y.name, parseTheoryAtomDefinitionVec(parseTheoryTermDefinitionVec(y.terms, y.terms_size), y.atoms, y.atoms_size), log_);
            }
        }
    }

private:
#define ARR(type, f, g) \
    auto g ## Vec(type const *x, size_t size) -> decltype(std::declval<INongroundProgramBuilder>().f()) { \
        return g ## Vec(prg_.f(), x, size);\
    } \
    auto g ## Vec(decltype(std::declval<INongroundProgramBuilder>().f()) ret, type const *x, size_t size) -> decltype(std::declval<INongroundProgramBuilder>().f()) { \
        for (auto it = x, ie = it + size; it != ie; ++it) { \
            prg_.f(ret, g(*it)); \
        } \
        return ret; \
    }

    bool require_(bool cond, char const *message) {
        if (!cond) { fail_<void>(message); }
        return false;
    }

    template <class T>
    T fail_(char const *message) {
        throw std::runtime_error(message);
    }

    Location parseLocation(clingo_location_t const &loc) {
        return Location(loc.begin_file, loc.begin_line, loc.begin_column, loc.end_file, loc.end_line, loc.end_column);
    }

    // {{{2 terms

    IdVecUid parseIdVec(clingo_ast_id_t const *x, size_t size) {
        auto ret = prg_.idvec();
        for (auto it = x, ie = it + size; it != ie; ++it) {
            prg_.idvec(ret, parseLocation(it->location), it->id);
        }
        return ret;
    }

    TermUid parseTerm(clingo_ast_term_t const &x) {
        switch (static_cast<enum clingo_ast_term_type>(x.type)) {
            case clingo_ast_term_type_symbol: {
                return prg_.term(parseLocation(x.location), Symbol(x.symbol));
            }
            case clingo_ast_term_type_variable: {
                return prg_.term(parseLocation(x.location), x.variable);
            }
            case clingo_ast_term_type_unary_operation: {
                auto &y = *x.unary_operation;
                return prg_.term(parseLocation(x.location), static_cast<UnOp>(y.unary_operator), parseTerm(y.argument));
            }
            case clingo_ast_term_type_binary_operation: {
                auto &y = *x.binary_operation;
                return prg_.term(parseLocation(x.location), static_cast<BinOp>(y.binary_operator), parseTerm(y.left), parseTerm(y.right));
            }
            case clingo_ast_term_type_interval: {
                auto &y = *x.interval;
                return prg_.term(parseLocation(x.location), parseTerm(y.left), parseTerm(y.right));
            }
            case clingo_ast_term_type_function:
            case clingo_ast_term_type_external_function: {
                auto &y = *x.function;
                require_(y.name[0] != '\0' || x.type != clingo_ast_term_type_external_function, "external functions must have a name");
                return y.name[0] != '\0'
                    ? prg_.term(parseLocation(x.location), y.name, prg_.termvecvec(prg_.termvecvec(), parseTermVec(y.arguments, y.size)), x.type == clingo_ast_term_type_external_function)
                    : prg_.term(parseLocation(x.location), parseTermVec(y.arguments, y.size), true);
            }
            case clingo_ast_term_type_pool: {
                auto &y = *x.pool;
                return prg_.pool(parseLocation(x.location), parseTermVec(y.arguments, y.size));
            }
        }
        return static_cast<TermUid>(0);
    }
    ARR(clingo_ast_term_t, termvec, parseTerm)

    CSPMulTermUid parseCSPMulTerm(clingo_ast_csp_product_term_t const &x) {
        return x.variable
            ? prg_.cspmulterm(parseLocation(x.location), parseTerm(x.coefficient), parseTerm(*x.variable))
            : prg_.cspmulterm(parseLocation(x.location), parseTerm(x.coefficient));
    }

    CSPAddTermUid parseCSPAddTerm(clingo_ast_csp_sum_term_t const &x) {
        require_(x.size > 0, "csp sums terms must not be empty");
        auto it = x.terms, ie = it + x.size;
        auto ret = prg_.cspaddterm(parseLocation(x.location), parseCSPMulTerm(*it));
        for (++it; it != ie; ++it) {
            ret = prg_.cspaddterm(parseLocation(x.location), ret, parseCSPMulTerm(*it), true);
        }
        return ret;
    }

    TheoryTermUid parseTheoryTerm(clingo_ast_theory_term_t const &x) {
        switch (static_cast<enum clingo_ast_theory_term_type>(x.type)) {
            case clingo_ast_theory_term_type_symbol: {
                return prg_.theorytermvalue(parseLocation(x.location), Symbol(x.symbol));
            }
            case clingo_ast_theory_term_type_variable: {
                return prg_.theorytermvar(parseLocation(x.location), x.variable);
            }
            case clingo_ast_theory_term_type_tuple: {
                auto y = *x.tuple;
                return prg_.theorytermtuple(parseLocation(x.location), parseTheoryOptermVec(y.terms, y.size));
            }
            case clingo_ast_theory_term_type_list: {
                auto y = *x.list;
                return prg_.theoryoptermlist(parseLocation(x.location), parseTheoryOptermVec(y.terms, y.size));
            }
            case clingo_ast_theory_term_type_set: {
                auto y = *x.set;
                return prg_.theorytermset(parseLocation(x.location), parseTheoryOptermVec(y.terms, y.size));
            }
            case clingo_ast_theory_term_type_function: {
                auto y = *x.function;
                return prg_.theorytermfun(parseLocation(x.location), y.name, parseTheoryOptermVec(y.arguments, y.size));
            }
            case clingo_ast_theory_term_type_unparsed_term: {
                return prg_.theorytermopterm(parseLocation(x.location), parseTheoryOpterm(*x.unparsed_term));
            }
        }
        return static_cast<TheoryTermUid>(0);
    }

    TheoryOptermUid parseTheoryOpterm(clingo_ast_theory_unparsed_term_t const &x) {
        require_(x.size > 0, "unparsed term arrays must not be empty");
        auto it = x.elements, ie = it + x.size;
        auto ret = prg_.theoryopterm(parseTheoryOpVec(it->operators, it->size), parseTheoryTerm(it->term));
        for (++it; it != ie; ++it) {
            require_(it->size > 0, "at least one operator necessary on right-hand-side of unparsed theory term");
            ret = prg_.theoryopterm(ret, parseTheoryOpVec(it->operators, it->size), parseTheoryTerm(it->term));
        }
        return ret;
    }

    TheoryOptermUid parseTheoryOpterm(clingo_ast_theory_term_t const &x) {
        return (x.type == clingo_ast_theory_term_type_unparsed_term)
            ? parseTheoryOpterm(*x.unparsed_term)
            : prg_.theoryopterm(prg_.theoryops(), parseTheoryTerm(x));
    }
    TheoryOptermVecUid parseTheoryOptermVec(clingo_ast_theory_term_t const *vec, size_t size) {
        auto ret = prg_.theoryopterms();
        for (auto it = vec, ie = it + size; it != ie; ++it) {
            ret = prg_.theoryopterms(ret, parseLocation(it->location), parseTheoryOpterm(*it));
        }
        return ret;
    }

    // {{{2 literals

    NAF parseSign(enum clingo_ast_sign a, enum clingo_ast_sign b = clingo_ast_sign_none) {
        switch (a) {
            case clingo_ast_sign_none:            { return static_cast<NAF>(b); }
            case clingo_ast_sign_negation:        { return static_cast<NAF>(b == clingo_ast_sign_negation ? clingo_ast_sign_double_negation : clingo_ast_sign_negation); }
            case clingo_ast_sign_double_negation: { return static_cast<NAF>(b == clingo_ast_sign_negation ? clingo_ast_sign_negation : clingo_ast_sign_double_negation); }
        }
        return static_cast<NAF>(clingo_ast_sign_none);
    }

    LitUid parseLiteral(clingo_ast_literal_t const &x, enum clingo_ast_sign extraSign = clingo_ast_sign_none) {
        switch (static_cast<enum clingo_ast_literal_type>(x.type)) {
            case clingo_ast_literal_type_boolean: {
                return prg_.boollit(parseLocation(x.location), extraSign != clingo_ast_sign_negation ? x.boolean : !x.boolean);
            }
            case clingo_ast_literal_type_symbolic: {
                return prg_.predlit(parseLocation(x.location), parseSign(static_cast<enum clingo_ast_sign>(x.sign), extraSign), parseTerm(*x.symbol));
            }
            case clingo_ast_literal_type_comparison: {
                auto &y = *x.comparison;
                Relation rel = static_cast<Relation>(y.comparison);
                if (extraSign == clingo_ast_sign_negation) { rel = neg(rel); }
                if (x.sign == clingo_ast_sign_negation)    { rel = neg(rel); }
                return prg_.rellit(parseLocation(x.location), rel, parseTerm(y.left), parseTerm(y.right));
            }
            case clingo_ast_literal_type_csp: {
                auto &y = *x.csp_literal;
                require_(x.sign == clingo_ast_sign_none && extraSign == clingo_ast_sign_none, "csp literals must not have signs");
                require_(y.size > 0, "csp literals need at least one guard");
                auto it = y.guards, ie = it + y.size;
                auto ret = prg_.csplit(parseLocation(x.location), parseCSPAddTerm(y.term), static_cast<Relation>(it->comparison), parseCSPAddTerm(it->term));
                for (++it; it != ie; ++it) {
                    ret = prg_.csplit(parseLocation(x.location), ret, static_cast<Relation>(it->comparison), parseCSPAddTerm(it->term));
                }
                return prg_.csplit(ret);
            }
        }
        return static_cast<LitUid>(0);
    }
    ARR(clingo_ast_literal_t, litvec, parseLiteral)

    // {{{2 aggregates

    BoundVecUid parseBounds(clingo_ast_aggregate_guard_t const *left, clingo_ast_aggregate_guard_t const *right) {
        auto ret = prg_.boundvec();
        if (left)  { ret = prg_.boundvec(ret, inv(static_cast<Relation>(left->comparison)), parseTerm(left->term)); }
        if (right) { ret = prg_.boundvec(ret, static_cast<Relation>(right->comparison), parseTerm(right->term)); }
        return ret;
    }

    CondLitVecUid parseCondLitVec(clingo_ast_conditional_literal_t const *vec, size_t size) {
        auto ret = prg_.condlitvec();
        for (auto it = vec, ie = it + size; it != ie; ++it) {
            ret = prg_.condlitvec(ret, parseLiteral(it->literal), parseLiteralVec(it->condition, it->size));
        }
        return ret;
    }

    HdAggrElemVecUid parseHdAggrElemVec(clingo_ast_head_aggregate_element_t const *vec, size_t size) {
        auto ret = prg_.headaggrelemvec();
        for (auto it = vec, ie = it + size; it != ie; ++it) {
            auto x = it->conditional_literal;
            ret = prg_.headaggrelemvec(ret, parseTermVec(it->tuple, it->tuple_size), parseLiteral(x.literal), parseLiteralVec(x.condition, x.size));
        }
        return ret;
    }

    BdAggrElemVecUid parseBdAggrElemVec(clingo_ast_body_aggregate_element_t const *vec, size_t size) {
        auto ret = prg_.bodyaggrelemvec();
        for (auto it = vec, ie = it + size; it != ie; ++it) {
            ret = prg_.bodyaggrelemvec(ret, parseTermVec(it->tuple, it->tuple_size), parseLiteralVec(it->condition, it->condition_size));
        }
        return ret;
    }

    TheoryElemVecUid parseTheoryElemVec(clingo_ast_theory_atom_element_t const *vec, size_t size) {
        auto ret = prg_.theoryelems();
        for (auto it = vec, ie = it + size; it != ie; ++it) {
            ret = prg_.theoryelems(ret, parseTheoryOptermVec(it->tuple, it->tuple_size), parseLiteralVec(it->condition, it->condition_size));
        }
        return ret;
    }

    CSPElemVecUid parseCSPElemVec(clingo_ast_disjoint_element_t const *vec, size_t size) {
        auto ret = prg_.cspelemvec();
        for (auto it = vec, ie = it + size; it != ie; ++it) {
            ret = prg_.cspelemvec(ret, parseLocation(it->location), parseTermVec(it->tuple, it->tuple_size), parseCSPAddTerm(it->term), parseLiteralVec(it->condition, it->condition_size));
        }
        return ret;
    }

    // {{{2 heads

    HdLitUid parseHeadLiteral(clingo_ast_head_literal_t const &x) {
        switch (static_cast<enum clingo_ast_head_literal_type>(x.type)) {
            case clingo_ast_head_literal_type_literal: {
                return prg_.headlit(parseLiteral(*x.literal));
            }
            case clingo_ast_head_literal_type_disjunction: {
                auto &y = *x.disjunction;
                return prg_.disjunction(parseLocation(x.location), parseCondLitVec(y.elements, y.size));
            }
            case clingo_ast_head_literal_type_aggregate: {
                auto &y = *x.aggregate;
                return prg_.headaggr(parseLocation(x.location), AggregateFunction::COUNT, parseBounds(y.left_guard, y.right_guard), parseCondLitVec(y.elements, y.size));
            }
            case clingo_ast_head_literal_type_head_aggregate: {
                auto &y = *x.head_aggregate;
                return prg_.headaggr(parseLocation(x.location), static_cast<AggregateFunction>(y.function), parseBounds(y.left_guard, y.right_guard), parseHdAggrElemVec(y.elements, y.size));
            }
            case clingo_ast_head_literal_type_theory_atom: {
                auto &y = *x.theory_atom;
                return y.guard
                    ? prg_.headaggr(parseLocation(x.location), prg_.theoryatom(parseTerm(y.term), parseTheoryElemVec(y.elements, y.size), y.guard->operator_name, parseLocation(x.location), parseTheoryOpterm(y.guard->term)))
                    : prg_.headaggr(parseLocation(x.location), prg_.theoryatom(parseTerm(y.term), parseTheoryElemVec(y.elements, y.size)));
            }
        }
        return static_cast<HdLitUid>(0);
    }

    // {{{2 bodies

    BdLitVecUid parseBodyLiteralVec(clingo_ast_body_literal_t const *lit, size_t size) {
        auto ret = prg_.body();
        for (auto it = lit, ie = lit + size; it != ie; ++it) {
            switch (static_cast<enum clingo_ast_body_literal_type>(it->type)) {
                case clingo_ast_body_literal_type_literal: {
                    ret = prg_.bodylit(ret, parseLiteral(*it->literal, static_cast<enum clingo_ast_sign>(it->sign)));
                    break;
                }
                case clingo_ast_body_literal_type_conditional: {
                    require_(static_cast<NAF>(it->sign) == NAF::POS, "conditional literals must hot have a sign");
                    auto &y = *it->conditional;
                    ret = prg_.conjunction(ret, parseLocation(it->location), parseLiteral(y.literal), parseLiteralVec(y.condition, y.size));
                    break;
                }
                case clingo_ast_body_literal_type_aggregate: {
                    auto &y = *it->aggregate;
                    ret = prg_.bodyaggr(ret, parseLocation(it->location), static_cast<NAF>(it->sign), AggregateFunction::COUNT, parseBounds(y.left_guard, y.right_guard), parseCondLitVec(y.elements, y.size));
                    break;
                }
                case clingo_ast_body_literal_type_body_aggregate: {
                    auto &y = *it->body_aggregate;
                    ret = prg_.bodyaggr(ret, parseLocation(it->location), static_cast<NAF>(it->sign), static_cast<AggregateFunction>(y.function), parseBounds(y.left_guard, y.right_guard), parseBdAggrElemVec(y.elements, y.size));
                    break;
                }
                case clingo_ast_body_literal_type_theory_atom: {
                    auto &y = *it->theory_atom;
                    ret = y.guard
                        ? prg_.bodyaggr(ret, parseLocation(it->location), static_cast<NAF>(it->sign), prg_.theoryatom(parseTerm(y.term), parseTheoryElemVec(y.elements, y.size), y.guard->operator_name, parseLocation(it->location), parseTheoryOpterm(y.guard->term)))
                        : prg_.bodyaggr(ret, parseLocation(it->location), static_cast<NAF>(it->sign), prg_.theoryatom(parseTerm(y.term), parseTheoryElemVec(y.elements, y.size)));
                    break;
                }
                case clingo_ast_body_literal_type_disjoint: {
                    auto &y = *it->disjoint;
                    ret = prg_.disjoint(ret, parseLocation(it->location), static_cast<NAF>(it->sign), parseCSPElemVec(y.elements, y.size));
                    break;
                }
            }
        }
        return ret;
    }

    // {{{2 theory definitions

    String parseTheoryOp(char const *str) { return str; }
    ARR(char const *, theoryops, parseTheoryOp)

    TheoryOpDefUid parseTheoryOpDef(clingo_ast_theory_operator_definition_t const &x) {
        return prg_.theoryopdef(parseLocation(x.location), x.name, x.priority, static_cast<TheoryOperatorType>(x.type));
    }
    ARR(clingo_ast_theory_operator_definition_t, theoryopdefs, parseTheoryOpDef)

    TheoryAtomDefUid parseTheoryAtomDefinition(clingo_ast_theory_atom_definition_t const &x) {
        return x.guard
            ? prg_.theoryatomdef(parseLocation(x.location), x.name, x.arity, x.elements, static_cast<TheoryAtomType>(x.type), parseTheoryOpVec(x.guard->operators, x.guard->size), x.guard->guard)
            : prg_.theoryatomdef(parseLocation(x.location), x.name, x.arity, x.elements, static_cast<TheoryAtomType>(x.type));
    }
    ARR(clingo_ast_theory_atom_definition_t, theorydefs, parseTheoryAtomDefinition)

    TheoryTermDefUid parseTheoryTermDefinition(clingo_ast_theory_term_definition_t const &x) {
        return prg_.theorytermdef(parseLocation(x.location), x.name, parseTheoryOpDefVec(x.operators, x.size), log_);
    }
    ARR(clingo_ast_theory_term_definition_t, theorydefs, parseTheoryTermDefinition)

    // }}}2
#undef ARR
private:
    Logger &log_;
    INongroundProgramBuilder &prg_;
};

}

void parseStatement(INongroundProgramBuilder &prg, Logger &log, clingo_ast_statement_t const &stm) {
    ASTParser p(log, prg);
    p.parse(stm);
}

// }}}1

} } // namespace Input Gringo

