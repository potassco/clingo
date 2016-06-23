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

#ifndef _GRINGO_INPUT_PROGRAMBUILDER_HH
#define _GRINGO_INPUT_PROGRAMBUILDER_HH

#include <gringo/locatable.hh>
#include <gringo/symbol.hh>
#include <gringo/indexed.hh>

#include <gringo/base.hh>
#include <gringo/control.hh>

#include <vector>
#include <memory>
#include <forward_list>
#include <clingo_ast.h>

namespace Gringo {

struct Scripts;
struct CSPMulTerm;
struct CSPAddTerm;
class TheoryOpDef;
class TheoryTermDef;
class TheoryAtomDef;
class TheoryDef;

namespace Input {

class Program;
struct Statement;
struct BodyAggregate;
struct HeadAggregate;
struct Literal;
struct CSPLiteral;
struct CSPElem;
class TheoryElement;
class TheoryAtom;

} // namespace Input

namespace Output {

class TheoryTerm;
class RawTheoryTerm;
class OutputBase;
using UTheoryTerm = std::unique_ptr<TheoryTerm>;

} } // namespace Output Gringo

namespace Gringo { namespace Input {

// {{{1 declaration of unique ids of program elements

enum IdVecUid           : unsigned { };
enum CSPAddTermUid      : unsigned { };
enum CSPMulTermUid      : unsigned { };
enum CSPLitUid          : unsigned { };
enum TermUid            : unsigned { };
enum TermVecUid         : unsigned { };
enum TermVecVecUid      : unsigned { };
enum LitUid             : unsigned { };
enum LitVecUid          : unsigned { };
enum CondLitVecUid      : unsigned { };
enum BdAggrElemVecUid   : unsigned { };
enum HdAggrElemVecUid   : unsigned { };
enum HdLitUid           : unsigned { };
enum BdLitVecUid        : unsigned { };
enum BoundVecUid        : unsigned { };
enum CSPElemVecUid      : unsigned { };
enum TheoryOpVecUid     : unsigned { };
enum TheoryTermUid      : unsigned { };
enum TheoryOptermUid    : unsigned { };
enum TheoryOptermVecUid : unsigned { };
enum TheoryElemVecUid   : unsigned { };
enum TheoryAtomUid      : unsigned { };
enum TheoryOpDefUid     : unsigned { };
enum TheoryOpDefVecUid  : unsigned { };
enum TheoryTermDefUid   : unsigned { };
enum TheoryAtomDefUid   : unsigned { };
enum TheoryDefVecUid    : unsigned { };

// {{{1 declaration of INongroundProgramBuilder

class INongroundProgramBuilder {
public:
    // {{{2 terms
    virtual TermUid term(Location const &loc, Symbol val) = 0;                                // constant
    virtual TermUid term(Location const &loc, String name) = 0;                            // variable
    virtual TermUid term(Location const &loc, UnOp op, TermUid a) = 0;                       // unary operation
    virtual TermUid term(Location const &loc, UnOp op, TermVecUid a) = 0;                    // unary operation
    virtual TermUid term(Location const &loc, BinOp op, TermUid a, TermUid b) = 0;           // binary operation
    virtual TermUid term(Location const &loc, TermUid a, TermUid b) = 0;                     // dots
    virtual TermUid term(Location const &loc, String name, TermVecVecUid b, bool lua) = 0; // function or lua function
    virtual TermUid term(Location const &loc, TermVecUid args, bool forceTuple) = 0;         // a tuple term (or simply a term)
    virtual TermUid pool(Location const &loc, TermVecUid args) = 0;                          // a pool term
    // {{{2 csp
    virtual CSPMulTermUid cspmulterm(Location const &loc, TermUid coe, TermUid var) = 0;
    virtual CSPMulTermUid cspmulterm(Location const &loc, TermUid coe) = 0;
    virtual CSPAddTermUid cspaddterm(Location const &loc, CSPAddTermUid a, CSPMulTermUid b, bool add) = 0;
    virtual CSPAddTermUid cspaddterm(Location const &loc, CSPMulTermUid a) = 0;
    virtual LitUid csplit(CSPLitUid a) = 0;
    virtual CSPLitUid csplit(Location const &loc, CSPLitUid a, Relation rel, CSPAddTermUid b) = 0;
    virtual CSPLitUid csplit(Location const &loc, CSPAddTermUid a, Relation rel, CSPAddTermUid b) = 0;
    // {{{2 id vectors
    virtual IdVecUid idvec() = 0;
    virtual IdVecUid idvec(IdVecUid uid, Location const &loc, String id) = 0;
    // {{{2 term vectors
    virtual TermVecUid termvec() = 0;
    virtual TermVecUid termvec(TermVecUid uid, TermUid term) = 0;
    // {{{2 term vector vectors
    virtual TermVecVecUid termvecvec() = 0;
    virtual TermVecVecUid termvecvec(TermVecVecUid uid, TermVecUid termvecUid) = 0;
    // {{{2 literals
    virtual LitUid boollit(Location const &loc, bool type) = 0;
    virtual LitUid predlit(Location const &loc, NAF naf, bool neg, String name, TermVecVecUid argvecvecUid) = 0;
    virtual LitUid rellit(Location const &loc, Relation rel, TermUid termUidLeft, TermUid termUidRight) = 0;
    // {{{2 literal vectors
    virtual LitVecUid litvec() = 0;
    virtual LitVecUid litvec(LitVecUid uid, LitUid literalUid) = 0;
    // {{{2 conditional literals
    virtual CondLitVecUid condlitvec() = 0;
    virtual CondLitVecUid condlitvec(CondLitVecUid uid, LitUid lit, LitVecUid litvec) = 0;
    // {{{2 body aggregate elements
    virtual BdAggrElemVecUid bodyaggrelemvec() = 0;
    virtual BdAggrElemVecUid bodyaggrelemvec(BdAggrElemVecUid uid, TermVecUid termvec, LitVecUid litvec) = 0;
    // {{{2 head aggregate elements
    virtual HdAggrElemVecUid headaggrelemvec() = 0;
    virtual HdAggrElemVecUid headaggrelemvec(HdAggrElemVecUid uid, TermVecUid termvec, LitUid lit, LitVecUid litvec) = 0;
    // {{{2 bounds
    virtual BoundVecUid boundvec() = 0;
    virtual BoundVecUid boundvec(BoundVecUid uid, Relation rel, TermUid term) = 0;
    // {{{2 heads
    virtual HdLitUid headlit(LitUid lit) = 0;
    virtual HdLitUid headaggr(Location const &loc, TheoryAtomUid atom) = 0;
    virtual HdLitUid headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, HdAggrElemVecUid headaggrelemvec) = 0;
    virtual HdLitUid headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid headaggrelemvec) = 0;
    virtual HdLitUid disjunction(Location const &loc, CondLitVecUid condlitvec) = 0;
    // {{{2 bodies
    virtual BdLitVecUid body() = 0;
    virtual BdLitVecUid bodylit(BdLitVecUid body, LitUid bodylit) = 0;
    virtual BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, TheoryAtomUid atom) = 0;
    virtual BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, BdAggrElemVecUid bodyaggrelemvec) = 0;
    virtual BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid bodyaggrelemvec) = 0;
    virtual BdLitVecUid conjunction(BdLitVecUid body, Location const &loc, LitUid head, LitVecUid litvec) = 0;
    virtual BdLitVecUid disjoint(BdLitVecUid body, Location const &loc, NAF naf, CSPElemVecUid elem) = 0;
    // {{{2 csp constraint elements
    virtual CSPElemVecUid cspelemvec() = 0;
    virtual CSPElemVecUid cspelemvec(CSPElemVecUid uid, Location const &loc, TermVecUid termvec, CSPAddTermUid addterm, LitVecUid litvec) = 0;
    // {{{2 statements
    virtual void rule(Location const &loc, HdLitUid head) = 0;
    virtual void rule(Location const &loc, HdLitUid head, BdLitVecUid body) = 0;
    virtual void define(Location const &loc, String name, TermUid value, bool defaultDef, Logger &log) = 0;
    virtual void optimize(Location const &loc, TermUid weight, TermUid priority, TermVecUid cond, BdLitVecUid body) = 0;
    virtual void showsig(Location const &loc, Sig, bool csp) = 0;
    virtual void show(Location const &loc, TermUid t, BdLitVecUid body, bool csp) = 0;
    virtual void python(Location const &loc, String code) = 0;
    virtual void lua(Location const &loc, String code) = 0;
    virtual void block(Location const &loc, String name, IdVecUid args) = 0;
    virtual void external(Location const &loc, LitUid head, BdLitVecUid body) = 0;
    virtual void edge(Location const &loc, TermVecVecUid edges, BdLitVecUid body) = 0;
    virtual void heuristic(Location const &loc, bool neg, String name, TermVecVecUid tvvUid, BdLitVecUid body, TermUid a, TermUid b, TermUid mod) = 0;
    virtual void project(Location const &loc, bool neg, String name, TermVecVecUid tvvUid, BdLitVecUid body) = 0;
    virtual void project(Location const &loc, Sig sig) = 0;
    // {{{2 theory atoms

    virtual TheoryTermUid theorytermset(Location const &loc, TheoryOptermVecUid args) = 0;
    virtual TheoryTermUid theoryoptermlist(Location const &loc, TheoryOptermVecUid args) = 0;
    virtual TheoryTermUid theorytermopterm(Location const &loc, TheoryOptermUid opterm) = 0;
    virtual TheoryTermUid theorytermtuple(Location const &loc, TheoryOptermVecUid args) = 0;
    virtual TheoryTermUid theorytermfun(Location const &loc, String name, TheoryOptermVecUid args) = 0;
    virtual TheoryTermUid theorytermvalue(Location const &loc, Symbol val) = 0;
    virtual TheoryTermUid theorytermvar(Location const &loc, String var) = 0;

    virtual TheoryOptermUid theoryopterm(TheoryOpVecUid ops, TheoryTermUid term) = 0;
    virtual TheoryOptermUid theoryopterm(TheoryOptermUid opterm, TheoryOpVecUid ops, TheoryTermUid term) = 0;

    virtual TheoryOpVecUid theoryops() = 0;
    virtual TheoryOpVecUid theoryops(TheoryOpVecUid ops, String op) = 0;

    virtual TheoryOptermVecUid theoryopterms() = 0;
    virtual TheoryOptermVecUid theoryopterms(TheoryOptermVecUid opterms, TheoryOptermUid opterm) = 0;
    virtual TheoryOptermVecUid theoryopterms(TheoryOptermUid opterm, TheoryOptermVecUid opterms) = 0;

    virtual TheoryElemVecUid theoryelems() = 0;
    virtual TheoryElemVecUid theoryelems(TheoryElemVecUid elems, TheoryOptermVecUid opterms, LitVecUid cond) = 0;

    virtual TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems) = 0;
    virtual TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems, String op, TheoryOptermUid opterm) = 0;

    // {{{2 theory definitions

    virtual TheoryOpDefUid theoryopdef(Location const &loc, String op, unsigned priority, TheoryOperatorType type) = 0;
    virtual TheoryOpDefVecUid theoryopdefs() = 0;
    virtual TheoryOpDefVecUid theoryopdefs(TheoryOpDefVecUid defs, TheoryOpDefUid def) = 0;

    virtual TheoryTermDefUid theorytermdef(Location const &loc, String name, TheoryOpDefVecUid defs, Logger &log) = 0;
    virtual TheoryAtomDefUid theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type) = 0;
    virtual TheoryAtomDefUid theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type, TheoryOpVecUid ops, String guardDef) = 0;

    virtual TheoryDefVecUid theorydefs() = 0;
    virtual TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryTermDefUid def) = 0;
    virtual TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryAtomDefUid def) = 0;

    virtual void theorydef(Location const &loc, String name, TheoryDefVecUid defs, Logger &log) = 0;

    // }}}2

    virtual ~INongroundProgramBuilder() { }
};

// {{{1 declaration of NongroundProgramBuilder

using UCSPLit = std::unique_ptr<CSPLiteral>;
using ULit = std::unique_ptr<Literal>;
using ULitVec = std::vector<ULit>;
using UHeadAggr = std::unique_ptr<HeadAggregate>;
using UBodyAggr = std::unique_ptr<BodyAggregate>;
using UStm = std::unique_ptr<Statement>;
using BoundVec = std::vector<Bound>;
using BodyAggrElem = std::pair<UTermVec, ULitVec>;
using BodyAggrElemVec = std::vector<BodyAggrElem>;
using CondLit = std::pair<ULit, ULitVec>;
using CondLitVec = std::vector<CondLit>;
using HeadAggrElem = std::tuple<UTermVec, ULit, ULitVec>;
using HeadAggrElemVec = std::vector<HeadAggrElem>;
using UBodyAggrVec = std::vector<UBodyAggr>;
using CSPElemVec = std::vector<CSPElem>;
using IdVec = std::vector<std::pair<Location, String>>;

class NongroundProgramBuilder : public INongroundProgramBuilder {
public:
    NongroundProgramBuilder(Scripts &scripts, Program &prg, Output::OutputBase &out, Defines &defs, bool rewriteMinimize = false);
    // {{{2 terms
    TermUid term(Location const &loc, Symbol val) override;                               // constant
    TermUid term(Location const &loc, String name) override;                           // variable
    TermUid term(Location const &loc, UnOp op, TermUid a) override;                      // unary operation
    TermUid term(Location const &loc, UnOp op, TermVecUid a) override;                   // unary operation
    TermUid term(Location const &loc, BinOp op, TermUid a, TermUid b) override;          // binary operation
    TermUid term(Location const &loc, TermUid a, TermUid b) override;                    // assignment
    TermUid term(Location const &loc, String name, TermVecVecUid b, bool lua) override;// function or lua function
    TermUid term(Location const &loc, TermVecUid args, bool forceTuple) override;        // a tuple term (or simply a term)
    TermUid pool(Location const &loc, TermVecUid args) override;                         // a pool term

    // {{{2 term vectors
    TermVecUid termvec() override;
    TermVecUid termvec(TermVecUid uid, TermUid term) override;
    // {{{2 id vectors
    IdVecUid idvec() override;
    IdVecUid idvec(IdVecUid uid, Location const &loc, String id) override;
    // {{{2 csp
    CSPMulTermUid cspmulterm(Location const &loc, TermUid coe, TermUid var) override;
    CSPMulTermUid cspmulterm(Location const &loc, TermUid coe) override;
    CSPAddTermUid cspaddterm(Location const &loc, CSPAddTermUid a, CSPMulTermUid b, bool add) override;
    CSPAddTermUid cspaddterm(Location const &loc, CSPMulTermUid a) override;
    LitUid csplit(CSPLitUid a) override;
    CSPLitUid csplit(Location const &loc, CSPLitUid a, Relation rel, CSPAddTermUid b) override;
    CSPLitUid csplit(Location const &loc, CSPAddTermUid a, Relation rel, CSPAddTermUid b) override;
    // {{{2 term vector vectors
    TermVecVecUid termvecvec() override;
    TermVecVecUid termvecvec(TermVecVecUid uid, TermVecUid termvecUid) override;
    // {{{2 literals
    LitUid boollit(Location const &loc, bool type) override;
    LitUid predlit(Location const &loc, NAF naf, bool neg, String name, TermVecVecUid argvecvecUid) override;
    LitUid rellit(Location const &loc, Relation rel, TermUid termUidLeft, TermUid termUidRight) override;
    // {{{2 literal vectors
    LitVecUid litvec() override;
    LitVecUid litvec(LitVecUid uid, LitUid literalUid) override;
    // {{{2 conditional literal vectors
    CondLitVecUid condlitvec() override;
    CondLitVecUid condlitvec(CondLitVecUid uid, LitUid lit, LitVecUid litvec) override;
    // {{{2 body aggregate elements
    BdAggrElemVecUid bodyaggrelemvec() override;
    BdAggrElemVecUid bodyaggrelemvec(BdAggrElemVecUid uid, TermVecUid termvec, LitVecUid litvec) override;
    // {{{2 head aggregate elements
    HdAggrElemVecUid headaggrelemvec() override;
    HdAggrElemVecUid headaggrelemvec(HdAggrElemVecUid uid, TermVecUid termvec, LitUid lit, LitVecUid litvec) override;
    // {{{2 bounds
    BoundVecUid boundvec() override;
    BoundVecUid boundvec(BoundVecUid uid, Relation rel, TermUid term) override;
    // {{{2 heads
    HdLitUid headlit(LitUid lit) override;
    HdLitUid headaggr(Location const &loc, TheoryAtomUid atom) override;
    HdLitUid headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, HdAggrElemVecUid headaggrelemvec) override;
    HdLitUid headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid headaggrelemvec) override;
    HdLitUid disjunction(Location const &loc, CondLitVecUid condlitvec) override;
    // {{{2 bodies
    BdLitVecUid body() override;
    BdLitVecUid bodylit(BdLitVecUid body, LitUid bodylit) override;
    BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, TheoryAtomUid atom) override;
    BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, BdAggrElemVecUid bodyaggrelemvec) override;
    BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid bodyaggrelemvec) override;
    BdLitVecUid conjunction(BdLitVecUid body, Location const &loc, LitUid head, LitVecUid litvec) override;
    BdLitVecUid disjoint(BdLitVecUid body, Location const &loc, NAF naf, CSPElemVecUid elem) override;
    // {{{2 csp constraint elements
    CSPElemVecUid cspelemvec() override;
    CSPElemVecUid cspelemvec(CSPElemVecUid uid, Location const &loc, TermVecUid termvec, CSPAddTermUid addterm, LitVecUid litvec) override;
    // {{{2 statements
    void rule(Location const &loc, HdLitUid head) override;
    void rule(Location const &loc, HdLitUid head, BdLitVecUid body) override;
    void define(Location const &loc, String name, TermUid value, bool defaultDef, Logger &log) override;
    void optimize(Location const &loc, TermUid weight, TermUid priority, TermVecUid cond, BdLitVecUid body) override;
    void showsig(Location const &loc, Sig sig, bool csp) override;
    void show(Location const &loc, TermUid t, BdLitVecUid body, bool csp) override;
    void python(Location const &loc, String code) override;
    void lua(Location const &loc, String code) override;
    void block(Location const &loc, String name, IdVecUid args) override;
    void external(Location const &loc, LitUid head, BdLitVecUid body) override;
    void edge(Location const &loc, TermVecVecUid edges, BdLitVecUid body) override;
    void heuristic(Location const &loc, bool neg, String name, TermVecVecUid tvvUid, BdLitVecUid body, TermUid a, TermUid b, TermUid mod) override;
    void project(Location const &loc, bool neg, String name, TermVecVecUid tvvUid, BdLitVecUid body) override;
    void project(Location const &loc, Sig sig) override;
    // }}}2
    // {{{2 theory atoms
    TheoryTermUid theorytermset(Location const &loc, TheoryOptermVecUid args) override;
    TheoryTermUid theoryoptermlist(Location const &loc, TheoryOptermVecUid args) override;
    TheoryTermUid theorytermopterm(Location const &loc, TheoryOptermUid opterm) override;
    TheoryTermUid theorytermtuple(Location const &loc, TheoryOptermVecUid args) override;
    TheoryTermUid theorytermfun(Location const &loc, String name, TheoryOptermVecUid args) override;
    TheoryTermUid theorytermvalue(Location const &loc, Symbol val) override;
    TheoryTermUid theorytermvar(Location const &loc, String var) override;

    TheoryOptermUid theoryopterm(TheoryOpVecUid ops, TheoryTermUid term) override;
    TheoryOptermUid theoryopterm(TheoryOptermUid opterm, TheoryOpVecUid ops, TheoryTermUid term) override;

    TheoryOpVecUid theoryops() override;
    TheoryOpVecUid theoryops(TheoryOpVecUid ops, String op) override;

    TheoryOptermVecUid theoryopterms() override;
    TheoryOptermVecUid theoryopterms(TheoryOptermVecUid opterms, TheoryOptermUid opterm) override;
    TheoryOptermVecUid theoryopterms(TheoryOptermUid opterm, TheoryOptermVecUid opterms) override;

    TheoryElemVecUid theoryelems() override;
    TheoryElemVecUid theoryelems(TheoryElemVecUid elems, TheoryOptermVecUid opterms, LitVecUid cond) override;

    TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems) override;
    TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems, String op, TheoryOptermUid opterm) override;

    // {{{2 theory definitions

    TheoryOpDefUid theoryopdef(Location const &loc, String op, unsigned priority, TheoryOperatorType type) override;
    TheoryOpDefVecUid theoryopdefs() override;
    TheoryOpDefVecUid theoryopdefs(TheoryOpDefVecUid defs, TheoryOpDefUid def) override;

    TheoryTermDefUid theorytermdef(Location const &loc, String name, TheoryOpDefVecUid defs, Logger &log) override;
    TheoryAtomDefUid theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type) override;
    TheoryAtomDefUid theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type, TheoryOpVecUid ops, String guardDef) override;

    TheoryDefVecUid theorydefs() override;
    TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryTermDefUid def) override;
    TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryAtomDefUid def) override;

    void theorydef(Location const &loc, String name, TheoryDefVecUid defs, Logger &log) override;

    // }}}2
    virtual ~NongroundProgramBuilder();

private:
    TermUid predRep(Location const &loc, bool neg, String name, TermVecVecUid tvvUid);

    using Terms            = Indexed<UTerm, TermUid>;
    using TermVecs         = Indexed<UTermVec, TermVecUid>;
    using TermVecVecs      = Indexed<UTermVecVec, TermVecVecUid>;
    using IdVecs           = Indexed<IdVec, IdVecUid>;
    using Lits             = Indexed<ULit, LitUid>;
    using LitVecs          = Indexed<ULitVec, LitVecUid>;
    using BodyAggrElemVecs = Indexed<BodyAggrElemVec, BdAggrElemVecUid>;
    using CondLitVecs      = Indexed<CondLitVec, CondLitVecUid>;
    using HeadAggrElemVecs = Indexed<HeadAggrElemVec, HdAggrElemVecUid>;
    using Bodies           = Indexed<UBodyAggrVec, BdLitVecUid>;
    using Heads            = Indexed<UHeadAggr, HdLitUid>;
    using CSPLits          = Indexed<UCSPLit, CSPLitUid>;
    using CSPAddTerms      = Indexed<CSPAddTerm, CSPAddTermUid>;
    using CSPMulTerms      = Indexed<CSPMulTerm, CSPMulTermUid>;
    using CSPElems         = Indexed<CSPElemVec, CSPElemVecUid>;
    using Statements       = std::vector<UStm>;
    using Bounds           = Indexed<BoundVec, BoundVecUid>;
    using VarVals          = std::unordered_map<String, Term::SVal>;

    using TheoryOpVecs      = Indexed<std::vector<String>, TheoryOpVecUid>;
    using TheoryTerms       = Indexed<Output::UTheoryTerm, TheoryTermUid>;
    using RawTheoryTerms    = Indexed<Output::RawTheoryTerm, TheoryOptermUid>;
    using RawTheoryTermVecs = Indexed<std::vector<Output::UTheoryTerm>, TheoryOptermVecUid>;
    using TheoryElementVecs = Indexed<std::vector<TheoryElement>, TheoryElemVecUid>;
    using TheoryAtoms       = Indexed<TheoryAtom, TheoryAtomUid>;

    using TheoryOpDefs = Indexed<TheoryOpDef, TheoryOpDefUid>;
    using TheoryOpDefVecs = Indexed<std::vector<TheoryOpDef>, TheoryOpDefVecUid>;
    using TheoryTermDefs = Indexed<TheoryTermDef, TheoryTermDefUid>;
    using TheoryAtomDefs = Indexed<TheoryAtomDef, TheoryAtomDefUid>;
    using TheoryDefVecs = Indexed<std::pair<std::vector<TheoryTermDef>, std::vector<TheoryAtomDef>>, TheoryDefVecUid>;

    Terms               terms_;
    TermVecs            termvecs_;
    TermVecVecs         termvecvecs_;
    IdVecs              idvecs_;
    Lits                lits_;
    LitVecs             litvecs_;
    BodyAggrElemVecs    bodyaggrelemvecs_;
    HeadAggrElemVecs    headaggrelemvecs_;
    CondLitVecs         condlitvecs_;
    Bounds              bounds_;
    Bodies              bodies_;
    Heads               heads_;
    VarVals             vals_;
    CSPLits             csplits_;
    CSPAddTerms         cspaddterms_;
    CSPMulTerms         cspmulterms_;
    CSPElems            cspelems_;
    TheoryOpVecs        theoryOpVecs_;
    TheoryTerms         theoryTerms_;
    RawTheoryTerms      theoryOpterms_;
    RawTheoryTermVecs   theoryOptermVecs_;
    TheoryElementVecs   theoryElems_;
    TheoryAtoms         theoryAtoms_;
    TheoryOpDefs        theoryOpDefs_;
    TheoryOpDefVecs     theoryOpDefVecs_;
    TheoryTermDefs      theoryTermDefs_;
    TheoryAtomDefs      theoryAtomDefs_;
    TheoryDefVecs       theoryDefVecs_;
    Scripts            &scripts_;
    Program            &prg_;
    Output::OutputBase &out;
    Defines            &defs_;
    bool                rewriteMinimize_;
};

// {{{1 declaration of ASTBuilder

class ASTBuilder : public Gringo::Input::INongroundProgramBuilder {
public:
    using Callback = std::function<void (clingo_ast_statement const &ast)>;
    ASTBuilder(Callback cb);
    virtual ~ASTBuilder() noexcept;

    // {{{2 terms
    TermUid term(Location const &loc, Symbol val) override;
    TermUid term(Location const &loc, String name) override;
    TermUid term(Location const &loc, UnOp op, TermUid a) override;
    TermUid term(Location const &loc, UnOp op, TermVecUid a) override;
    TermUid term(Location const &loc, BinOp op, TermUid a, TermUid b) override;
    TermUid term(Location const &loc, TermUid a, TermUid b) override;
    TermUid term(Location const &loc, String name, TermVecVecUid a, bool lua) override;
    TermUid term(Location const &loc, TermVecUid a, bool forceTuple) override;
    TermUid pool(Location const &loc, TermVecUid a) override;
    // {{{2 csp
    CSPMulTermUid cspmulterm(Location const &loc, TermUid coe, TermUid var) override;
    CSPMulTermUid cspmulterm(Location const &loc, TermUid coe) override;
    CSPAddTermUid cspaddterm(Location const &loc, CSPAddTermUid a, CSPMulTermUid b, bool add) override;
    CSPAddTermUid cspaddterm(Location const &, CSPMulTermUid b) override;
    CSPLitUid csplit(Location const &loc, CSPLitUid a, Relation rel, CSPAddTermUid b) override;
    CSPLitUid csplit(Location const &loc, CSPAddTermUid a, Relation rel, CSPAddTermUid b) override;
    // {{{2 id vectors
    IdVecUid idvec() override;
    IdVecUid idvec(IdVecUid uid, Location const &loc, String id) override;
    // {{{2 term vectors
    TermVecUid termvec() override;
    TermVecUid termvec(TermVecUid uid, TermUid termUid) override;
    // {{{2 term vector vectors
    TermVecVecUid termvecvec() override;
    TermVecVecUid termvecvec(TermVecVecUid uid, TermVecUid termvecUid) override;
    // {{{2 literals
    LitUid boollit(Location const &loc, bool type) override;
    LitUid predlit(Location const &loc, NAF naf, bool neg, String name, TermVecVecUid argvecvecUid) override;
    LitUid rellit(Location const &loc, Relation rel, TermUid termUidLeft, TermUid termUidRight) override;
    LitUid csplit(CSPLitUid a) override;
    // {{{2 literal vectors
    LitVecUid litvec() override;
    LitVecUid litvec(LitVecUid uid, LitUid literalUid) override;
    // {{{2 conditional literals
    CondLitVecUid condlitvec() override;
    CondLitVecUid condlitvec(CondLitVecUid uid, LitUid litUid, LitVecUid litvecUid) override;
    // {{{2 body aggregate elements
    BdAggrElemVecUid bodyaggrelemvec() override;
    BdAggrElemVecUid bodyaggrelemvec(BdAggrElemVecUid uid, TermVecUid termvec, LitVecUid litvec) override;
    // {{{2 head aggregate elements
    HdAggrElemVecUid headaggrelemvec() override;
    HdAggrElemVecUid headaggrelemvec(HdAggrElemVecUid uid, TermVecUid termvec, LitUid lit, LitVecUid litvec) override;
    // {{{2 bounds
    BoundVecUid boundvec() override;
    BoundVecUid boundvec(BoundVecUid uid, Relation rel, TermUid term) override;
    // {{{2 heads
    HdLitUid headlit(LitUid litUid) override;
    HdLitUid headaggr(Location const &loc, TheoryAtomUid atomUid) override;
    HdLitUid headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, HdAggrElemVecUid headaggrelemvec) override;
    HdLitUid headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid headaggrelemvec) override;
    HdLitUid disjunction(Location const &loc, CondLitVecUid condlitvec) override;
    // {{{2 bodies
    BdLitVecUid body() override;
    BdLitVecUid bodylit(BdLitVecUid body, LitUid bodylit) override;
    BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, TheoryAtomUid atomUid) override;
    BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, BdAggrElemVecUid bodyaggrelemvec) override;
    BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid bodyaggrelemvec) override;
    BdLitVecUid conjunction(BdLitVecUid body, Location const &loc, LitUid head, LitVecUid litvec) override;
    BdLitVecUid disjoint(BdLitVecUid body, Location const &loc, NAF naf, CSPElemVecUid elem) override;
    // {{{2 csp constraint elements
    CSPElemVecUid cspelemvec() override;
    CSPElemVecUid cspelemvec(CSPElemVecUid uid, Location const &loc, TermVecUid termvec, CSPAddTermUid addterm, LitVecUid litvec) override;
    // {{{2 statements
    void rule(Location const &loc, HdLitUid head) override;
    void rule(Location const &loc, HdLitUid head, BdLitVecUid body) override;
    void define(Location const &loc, String name, TermUid value, bool defaultDef, Logger &log) override;
    void optimize(Location const &loc, TermUid weight, TermUid priority, TermVecUid cond, BdLitVecUid body) override;
    void showsig(Location const &loc, Sig sig, bool csp) override;
    void show(Location const &loc, TermUid t, BdLitVecUid body, bool csp) override;
    void python(Location const &loc, String code) override;
    void lua(Location const &loc, String code) override;
    void block(Location const &loc, String name, IdVecUid args) override;
    void external(Location const &loc, LitUid head, BdLitVecUid body) override;
    void edge(Location const &loc, TermVecVecUid edges, BdLitVecUid body) override;
    void heuristic(Location const &loc, bool neg, String name, TermVecVecUid tvvUid, BdLitVecUid body, TermUid a, TermUid b, TermUid mod) override;
    void project(Location const &loc, bool neg, String name, TermVecVecUid tvvUid, BdLitVecUid body) override;
    void project(Location const &loc, Sig sig) override;
    /*
    // {{{2 theory atoms
    TheoryTermUid theorytermset(Location const &loc, TheoryOptermVecUid args) override;
    TheoryTermUid theoryoptermlist(Location const &loc, TheoryOptermVecUid args) override;
    TheoryTermUid theorytermtuple(Location const &loc, TheoryOptermVecUid args) override;
    TheoryTermUid theorytermopterm(Location const &loc, TheoryOptermUid opterm) override;
    TheoryTermUid theorytermfun(Location const &loc, String name, TheoryOptermVecUid args) override;
    TheoryTermUid theorytermvalue(Location const &loc, Symbol val) override;
    TheoryTermUid theorytermvar(Location const &loc, String var) override;
    TheoryOptermUid theoryopterm(TheoryOpVecUid ops, TheoryTermUid term) override;
    TheoryOptermUid theoryopterm(TheoryOptermUid opterm, TheoryOpVecUid ops, TheoryTermUid term) override;
    TheoryOpVecUid theoryops() override;
    TheoryOpVecUid theoryops(TheoryOpVecUid ops, String op) override;
    TheoryOptermVecUid theoryopterms() override;
    TheoryOptermVecUid theoryopterms(TheoryOptermVecUid opterms, TheoryOptermUid opterm) override;
    TheoryOptermVecUid theoryopterms(TheoryOptermUid opterm, TheoryOptermVecUid opterms) override;
    TheoryElemVecUid theoryelems() override;
    TheoryElemVecUid theoryelems(TheoryElemVecUid elems, TheoryOptermVecUid opterms, LitVecUid cond) override;
    TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems) override;
    TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems, String op, TheoryOptermUid opterm) override;
    // {{{2 theory definitions
    TheoryOpDefUid theoryopdef(Location const &loc, String op, unsigned priority, TheoryOperatorType type) override;
    TheoryOpDefVecUid theoryopdefs() override;
    TheoryOpDefVecUid theoryopdefs(TheoryOpDefVecUid defs, TheoryOpDefUid def) override;
    TheoryTermDefUid theorytermdef(Location const &loc, String name, TheoryOpDefVecUid defs, Logger &log) override;
    TheoryAtomDefUid theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type) override;
    TheoryAtomDefUid theoryatomdef(Location const &loc, String name, unsigned arity, String termDef, TheoryAtomType type, TheoryOpVecUid ops, String guardDef) override;
    TheoryDefVecUid theorydefs() override;
    TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryTermDefUid def) override;
    TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryAtomDefUid def) override;
    void theorydef(Location const &loc, String name, TheoryDefVecUid defs, Logger &log) override;
    */
    // }}}2

private:
    using TermVec = std::vector<clingo_ast_term_t>;
    using TermVecVec = std::vector<TermVec>;

    TermUid pool_(Location const &loc, TermVec &&vec);
    clingo_ast_term_t fun_(Location const &loc, String name, TermVec &&vec, bool external);

    using Terms            = Indexed<clingo_ast_term_t, TermUid>;
    using TermVecs         = Indexed<TermVec, TermVecUid>;
    using TermVecVecs      = Indexed<TermVecVec, TermVecVecUid>;
    using CSPAddTerms      = Indexed<std::pair<Location, std::vector<clingo_ast_csp_multiply_term_t>>, CSPAddTermUid>;
    using CSPMulTerms      = Indexed<clingo_ast_csp_multiply_term_t, CSPMulTermUid>;
    using CSPLits          = Indexed<std::pair<Location, std::vector<std::pair<Relation, clingo_ast_csp_add_term_t>>>, CSPLitUid>;
    using IdVecs           = Indexed<std::vector<clingo_ast_id_t>, IdVecUid>;
    using Lits             = Indexed<clingo_ast_literal_t, LitUid>;
    using LitVecs          = Indexed<std::vector<clingo_ast_literal_t>, LitVecUid>;
    using CondLitVecs      = Indexed<std::vector<clingo_ast_conditional_literal_t>, CondLitVecUid>;
    using BodyAggrElemVecs = Indexed<std::vector<clingo_ast_body_aggregate_element_t>, BdAggrElemVecUid>;
    using HeadAggrElemVecs = Indexed<std::vector<clingo_ast_head_aggregate_element_t>, HdAggrElemVecUid>;
    using Bounds           = Indexed<std::vector<clingo_ast_aggregate_guard_t>, BoundVecUid>;
    using Bodies           = Indexed<std::vector<clingo_ast_body_literal_t>, BdLitVecUid>;
    using Heads            = Indexed<clingo_ast_head_literal_t, HdLitUid>;
    using TheoryAtoms      = Indexed<clingo_ast_theory_atom_t, TheoryAtomUid>;
    using CSPElems         = Indexed<std::vector<clingo_ast_disjoint_element_t>, CSPElemVecUid>;
    /*

    using TheoryOpVecs      = Indexed<NodeVec, TheoryOpVecUid>;
    using TheoryTerms       = Indexed<clingo_ast, TheoryTermUid>;
    using RawTheoryTerms    = Indexed<NodeVec, TheoryOptermUid>;
    using RawTheoryTermVecs = Indexed<NodeVec, TheoryOptermVecUid>;
    using TheoryElementVecs = Indexed<NodeVec, TheoryElemVecUid>;

    using TheoryOpDefs    = Indexed<clingo_ast, TheoryOpDefUid>;
    using TheoryOpDefVecs = Indexed<NodeVec, TheoryOpDefVecUid>;
    using TheoryTermDefs  = Indexed<clingo_ast, TheoryTermDefUid>;
    using TheoryAtomDefs  = Indexed<clingo_ast, TheoryAtomDefUid>;
    using TheoryDefVecs   = Indexed<NodeVec, TheoryDefVecUid>;
    */

    Callback            cb_;
    Terms               terms_;
    TermVecs            termvecs_;
    TermVecVecs         termvecvecs_;
    CSPAddTerms         cspaddterms_;
    CSPMulTerms         cspmulterms_;
    CSPLits             csplits_;
    IdVecs              idvecs_;
    Lits                lits_;
    LitVecs             litvecs_;
    CondLitVecs         condlitvecs_;
    BodyAggrElemVecs    bodyaggrelemvecs_;
    HeadAggrElemVecs    headaggrelemvecs_;
    Bounds              bounds_;
    Bodies              bodies_;
    Heads               heads_;
    TheoryAtoms         theoryAtoms_;
    CSPElems            cspelems_;
    /*
    TheoryOpVecs        theoryOpVecs_;
    TheoryTerms         theoryTerms_;
    RawTheoryTerms      theoryOpterms_;
    RawTheoryTermVecs   theoryOptermVecs_;
    TheoryElementVecs   theoryElems_;
    TheoryOpDefs        theoryOpDefs_;
    TheoryOpDefVecs     theoryOpDefVecs_;
    TheoryTermDefs      theoryTermDefs_;
    TheoryAtomDefs      theoryAtomDefs_;
    TheoryDefVecs       theoryDefVecs_;
    */
    std::vector<void *> data_;
    std::vector<void *> arrdata_;

    template <class T>
    T *create();
    template <class T>
    T *create(T x);
    template <class T>
    T *create_array(size_t size);
    template <class T>
    T *create_array(std::vector<T> const &vec);

    void statement_(Location loc, clingo_ast_statement_type_t type, clingo_ast_statement_t &stm);
    void clear_() noexcept;
};

// {{{1 declaration of ASTParser

// TODO...

// }}}1

} } // namespace Input Gringo

#endif // _GRINGO_PROGRAMBUILDER_HH
