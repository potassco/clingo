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

#include <gringo/flyweight.hh>
#include <gringo/locatable.hh>
#include <gringo/value.hh>

#include <gringo/base.hh>
#include <gringo/control.hh>

#include <vector>
#include <memory>
#include <forward_list>

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
    virtual TermUid term(Location const &loc, Value val) = 0;                                // constant
    virtual TermUid term(Location const &loc, FWString name) = 0;                            // variable
    virtual TermUid term(Location const &loc, UnOp op, TermUid a) = 0;                       // unary operation
    virtual TermUid term(Location const &loc, UnOp op, TermVecUid a) = 0;                    // unary operation
    virtual TermUid term(Location const &loc, BinOp op, TermUid a, TermUid b) = 0;           // binary operation
    virtual TermUid term(Location const &loc, TermUid a, TermUid b) = 0;                     // dots
    virtual TermUid term(Location const &loc, FWString name, TermVecVecUid b, bool lua) = 0; // function or lua function
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
    virtual IdVecUid idvec(IdVecUid uid, Location const &loc, FWString id) = 0;
    // {{{2 term vectors
    virtual TermVecUid termvec() = 0;
    virtual TermVecUid termvec(TermVecUid uid, TermUid term) = 0;
    // {{{2 term vector vectors
    virtual TermVecVecUid termvecvec() = 0;
    virtual TermVecVecUid termvecvec(TermVecVecUid uid, TermVecUid termvecUid) = 0;
    // {{{2 literals
    virtual LitUid boollit(Location const &loc, bool type) = 0;
    virtual LitUid predlit(Location const &loc, NAF naf, bool neg, FWString name, TermVecVecUid argvecvecUid) = 0;
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
    virtual void define(Location const &loc, FWString name, TermUid value, bool defaultDef) = 0;
    virtual void optimize(Location const &loc, TermUid weight, TermUid priority, TermVecUid cond, BdLitVecUid body) = 0;
    virtual void showsig(Location const &loc, FWSignature, bool csp) = 0;
    virtual void show(Location const &loc, TermUid t, BdLitVecUid body, bool csp) = 0;
    virtual void python(Location const &loc, FWString code) = 0;
    virtual void lua(Location const &loc, FWString code) = 0;
    virtual void block(Location const &loc, FWString name, IdVecUid args) = 0;
    virtual void external(Location const &loc, LitUid head, BdLitVecUid body) = 0;
    virtual void edge(Location const &loc, TermVecVecUid edges, BdLitVecUid body) = 0;
    virtual void heuristic(Location const &loc, bool neg, FWString name, TermVecVecUid tvvUid, BdLitVecUid body, TermUid a, TermUid b, TermUid mod) = 0;
    virtual void project(Location const &loc, bool neg, FWString name, TermVecVecUid tvvUid, BdLitVecUid body) = 0;
    virtual void project(Location const &loc, FWSignature sig) = 0;
    // {{{2 theory atoms

    virtual TheoryTermUid theorytermset(Location const &loc, TheoryOptermVecUid args) = 0;
    virtual TheoryTermUid theoryoptermlist(Location const &loc, TheoryOptermVecUid args) = 0;
    virtual TheoryTermUid theorytermopterm(Location const &loc, TheoryOptermUid opterm) = 0;
    virtual TheoryTermUid theorytermtuple(Location const &loc, TheoryOptermVecUid args) = 0;
    virtual TheoryTermUid theorytermfun(Location const &loc, FWString name, TheoryOptermVecUid args) = 0;
    virtual TheoryTermUid theorytermvalue(Location const &loc, Value val) = 0;
    virtual TheoryTermUid theorytermvar(Location const &loc, FWString var) = 0;

    virtual TheoryOptermUid theoryopterm(TheoryOpVecUid ops, TheoryTermUid term) = 0;
    virtual TheoryOptermUid theoryopterm(TheoryOptermUid opterm, TheoryOpVecUid ops, TheoryTermUid term) = 0;

    virtual TheoryOpVecUid theoryops() = 0;
    virtual TheoryOpVecUid theoryops(TheoryOpVecUid ops, FWString op) = 0;

    virtual TheoryOptermVecUid theoryopterms() = 0;
    virtual TheoryOptermVecUid theoryopterms(TheoryOptermVecUid opterms, TheoryOptermUid opterm) = 0;
    virtual TheoryOptermVecUid theoryopterms(TheoryOptermUid opterm, TheoryOptermVecUid opterms) = 0;

    virtual TheoryElemVecUid theoryelems() = 0;
    virtual TheoryElemVecUid theoryelems(TheoryElemVecUid elems, TheoryOptermVecUid opterms, LitVecUid cond) = 0;

    virtual TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems) = 0;
    virtual TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems, FWString op, TheoryOptermUid opterm) = 0;

    // {{{2 theory definitions

    virtual TheoryOpDefUid theoryopdef(Location const &loc, FWString op, unsigned priority, TheoryOperatorType type) = 0;
    virtual TheoryOpDefVecUid theoryopdefs() = 0;
    virtual TheoryOpDefVecUid theoryopdefs(TheoryOpDefVecUid defs, TheoryOpDefUid def) = 0;

    virtual TheoryTermDefUid theorytermdef(Location const &loc, FWString name, TheoryOpDefVecUid defs) = 0;
    virtual TheoryAtomDefUid theoryatomdef(Location const &loc, FWString name, unsigned arity, FWString termDef, TheoryAtomType type) = 0;
    virtual TheoryAtomDefUid theoryatomdef(Location const &loc, FWString name, unsigned arity, FWString termDef, TheoryAtomType type, TheoryOpVecUid ops, FWString guardDef) = 0;

    virtual TheoryDefVecUid theorydefs() = 0;
    virtual TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryTermDefUid def) = 0;
    virtual TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryAtomDefUid def) = 0;

    virtual void theorydef(Location const &loc, FWString name, TheoryDefVecUid defs) = 0;

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
using IdVec = std::vector<std::pair<Location, FWString>>;

class NongroundProgramBuilder : public INongroundProgramBuilder {
public:
    NongroundProgramBuilder(Scripts &scripts, Program &prg, Output::OutputBase &out, Defines &defs, bool rewriteMinimize = false);
    // {{{2 terms
    TermUid term(Location const &loc, Value val) override;                               // constant
    TermUid term(Location const &loc, FWString name) override;                           // variable
    TermUid term(Location const &loc, UnOp op, TermUid a) override;                      // unary operation
    TermUid term(Location const &loc, UnOp op, TermVecUid a) override;                   // unary operation
    TermUid term(Location const &loc, BinOp op, TermUid a, TermUid b) override;          // binary operation
    TermUid term(Location const &loc, TermUid a, TermUid b) override;                    // assignment
    TermUid term(Location const &loc, FWString name, TermVecVecUid b, bool lua) override;// function or lua function
    TermUid term(Location const &loc, TermVecUid args, bool forceTuple) override;        // a tuple term (or simply a term)
    TermUid pool(Location const &loc, TermVecUid args) override;                         // a pool term

    // {{{2 term vectors
    TermVecUid termvec() override;
    TermVecUid termvec(TermVecUid uid, TermUid term) override;
    // {{{2 id vectors
    IdVecUid idvec() override;
    IdVecUid idvec(IdVecUid uid, Location const &loc, FWString id) override;
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
    LitUid predlit(Location const &loc, NAF naf, bool neg, FWString name, TermVecVecUid argvecvecUid) override;
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
    void define(Location const &loc, FWString name, TermUid value, bool defaultDef) override;
    void optimize(Location const &loc, TermUid weight, TermUid priority, TermVecUid cond, BdLitVecUid body) override;
    void showsig(Location const &loc, FWSignature sig, bool csp) override;
    void show(Location const &loc, TermUid t, BdLitVecUid body, bool csp) override;
    void python(Location const &loc, FWString code) override;
    void lua(Location const &loc, FWString code) override;
    void block(Location const &loc, FWString name, IdVecUid args) override;
    void external(Location const &loc, LitUid head, BdLitVecUid body) override;
    void edge(Location const &loc, TermVecVecUid edges, BdLitVecUid body) override;
    void heuristic(Location const &loc, bool neg, FWString name, TermVecVecUid tvvUid, BdLitVecUid body, TermUid a, TermUid b, TermUid mod) override;
    void project(Location const &loc, bool neg, FWString name, TermVecVecUid tvvUid, BdLitVecUid body) override;
    void project(Location const &loc, FWSignature sig) override;
    // }}}2
    // {{{2 theory atoms
    TheoryTermUid theorytermset(Location const &loc, TheoryOptermVecUid args) override;
    TheoryTermUid theoryoptermlist(Location const &loc, TheoryOptermVecUid args) override;
    TheoryTermUid theorytermopterm(Location const &loc, TheoryOptermUid opterm) override;
    TheoryTermUid theorytermtuple(Location const &loc, TheoryOptermVecUid args) override;
    TheoryTermUid theorytermfun(Location const &loc, FWString name, TheoryOptermVecUid args) override;
    TheoryTermUid theorytermvalue(Location const &loc, Value val) override;
    TheoryTermUid theorytermvar(Location const &loc, FWString var) override;

    TheoryOptermUid theoryopterm(TheoryOpVecUid ops, TheoryTermUid term) override;
    TheoryOptermUid theoryopterm(TheoryOptermUid opterm, TheoryOpVecUid ops, TheoryTermUid term) override;

    TheoryOpVecUid theoryops() override;
    TheoryOpVecUid theoryops(TheoryOpVecUid ops, FWString op) override;

    TheoryOptermVecUid theoryopterms() override;
    TheoryOptermVecUid theoryopterms(TheoryOptermVecUid opterms, TheoryOptermUid opterm) override;
    TheoryOptermVecUid theoryopterms(TheoryOptermUid opterm, TheoryOptermVecUid opterms) override;

    TheoryElemVecUid theoryelems() override;
    TheoryElemVecUid theoryelems(TheoryElemVecUid elems, TheoryOptermVecUid opterms, LitVecUid cond) override;

    TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems) override;
    TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems, FWString op, TheoryOptermUid opterm) override;

    // {{{2 theory definitions

    TheoryOpDefUid theoryopdef(Location const &loc, FWString op, unsigned priority, TheoryOperatorType type) override;
    TheoryOpDefVecUid theoryopdefs() override;
    TheoryOpDefVecUid theoryopdefs(TheoryOpDefVecUid defs, TheoryOpDefUid def) override;

    TheoryTermDefUid theorytermdef(Location const &loc, FWString name, TheoryOpDefVecUid defs) override;
    TheoryAtomDefUid theoryatomdef(Location const &loc, FWString name, unsigned arity, FWString termDef, TheoryAtomType type) override;
    TheoryAtomDefUid theoryatomdef(Location const &loc, FWString name, unsigned arity, FWString termDef, TheoryAtomType type, TheoryOpVecUid ops, FWString guardDef) override;

    TheoryDefVecUid theorydefs() override;
    TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryTermDefUid def) override;
    TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryAtomDefUid def) override;

    void theorydef(Location const &loc, FWString name, TheoryDefVecUid defs) override;

    // }}}2
    virtual ~NongroundProgramBuilder();

private:
    TermUid predRep(Location const &loc, bool neg, FWString name, TermVecVecUid tvvUid);

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
    using VarVals          = std::unordered_map<FWString, Term::SVal>;

    using TheoryOpVecs      = Indexed<std::vector<FWString>, TheoryOpVecUid>;
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
    // about ownership: the children of nodes are owned by the builder
    // such nodes are deleted when the next directive is parsed
    using NodeList = std::forward_list<AST>;
    using NodeVec = std::vector<AST>;
    using NodeVecList = std::forward_list<NodeVec>;
    using TermUidVec = std::vector<TermUid>;
    using TermUidVecVec = std::vector<TermVecUid>;
    using Callback = std::function<void (AST &ast)>;
public:
    ASTBuilder(Callback cb);
    virtual ~ASTBuilder() noexcept;

    // {{{2 terms
    TermUid term(Location const &loc, Value val) override;
    TermUid term(Location const &loc, FWString name) override;
    TermUid term(Location const &loc, UnOp op, TermUid a) override;
    TermUid term(Location const &loc, UnOp op, TermVecUid a) override;
    TermUid term(Location const &loc, BinOp op, TermUid a, TermUid b) override;
    TermUid term(Location const &loc, TermUid a, TermUid b) override;
    TermUid term(Location const &loc, FWString name, TermVecVecUid a, bool lua) override;
    TermUid term(Location const &loc, TermVecUid a, bool forceTuple) override;
    TermUid pool(Location const &loc, TermVecUid a) override;
    // {{{2 csp
    CSPMulTermUid cspmulterm(Location const &loc, TermUid coe, TermUid var) override;
    CSPMulTermUid cspmulterm(Location const &loc, TermUid coe) override;
    CSPAddTermUid cspaddterm(Location const &loc, CSPAddTermUid a, CSPMulTermUid b, bool add) override;
    CSPAddTermUid cspaddterm(Location const &, CSPMulTermUid b) override;
    LitUid csplit(CSPLitUid a) override;
    CSPLitUid csplit(Location const &loc, CSPLitUid a, Relation rel, CSPAddTermUid b) override;
    CSPLitUid csplit(Location const &loc, CSPAddTermUid a, Relation rel, CSPAddTermUid b) override;
    // {{{2 id vectors
    IdVecUid idvec() override;
    IdVecUid idvec(IdVecUid uid, Location const &loc, FWString id) override;
    // {{{2 term vectors
    TermVecUid termvec() override;
    TermVecUid termvec(TermVecUid uid, TermUid termUid) override;
    // {{{2 term vector vectors
    TermVecVecUid termvecvec() override;
    TermVecVecUid termvecvec(TermVecVecUid uid, TermVecUid termvecUid) override;
    // {{{2 literals
    LitUid boollit(Location const &loc, bool type) override;
    LitUid predlit(Location const &loc, NAF naf, bool neg, FWString name, TermVecVecUid argvecvecUid) override;
    LitUid rellit(Location const &loc, Relation rel, TermUid termUidLeft, TermUid termUidRight) override;
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
    void define(Location const &loc, FWString name, TermUid value, bool defaultDef) override;
    void optimize(Location const &loc, TermUid weight, TermUid priority, TermVecUid cond, BdLitVecUid body) override;
    void showsig(Location const &loc, FWSignature sig, bool csp) override;
    void show(Location const &loc, TermUid t, BdLitVecUid body, bool csp) override;
    void python(Location const &loc, FWString code) override;
    void lua(Location const &loc, FWString code) override;
    void block(Location const &loc, FWString name, IdVecUid args) override;
    void external(Location const &loc, LitUid head, BdLitVecUid body) override;
    void edge(Location const &loc, TermVecVecUid edges, BdLitVecUid body) override;
    void heuristic(Location const &loc, bool neg, FWString name, TermVecVecUid tvvUid, BdLitVecUid body, TermUid a, TermUid b, TermUid mod) override;
    void project(Location const &loc, bool neg, FWString name, TermVecVecUid tvvUid, BdLitVecUid body) override;
    void project(Location const &loc, FWSignature sig) override;
    // {{{2 theory atoms
    TheoryTermUid theorytermset(Location const &loc, TheoryOptermVecUid args) override;
    TheoryTermUid theoryoptermlist(Location const &loc, TheoryOptermVecUid args) override;
    TheoryTermUid theorytermtuple(Location const &loc, TheoryOptermVecUid args) override;
    TheoryTermUid theorytermopterm(Location const &loc, TheoryOptermUid opterm) override;
    TheoryTermUid theorytermfun(Location const &loc, FWString name, TheoryOptermVecUid args) override;
    TheoryTermUid theorytermvalue(Location const &loc, Value val) override;
    TheoryTermUid theorytermvar(Location const &loc, FWString var) override;
    TheoryOptermUid theoryopterm(TheoryOpVecUid ops, TheoryTermUid term) override;
    TheoryOptermUid theoryopterm(TheoryOptermUid opterm, TheoryOpVecUid ops, TheoryTermUid term) override;
    TheoryOpVecUid theoryops() override;
    TheoryOpVecUid theoryops(TheoryOpVecUid ops, FWString op) override;
    TheoryOptermVecUid theoryopterms() override;
    TheoryOptermVecUid theoryopterms(TheoryOptermVecUid opterms, TheoryOptermUid opterm) override;
    TheoryOptermVecUid theoryopterms(TheoryOptermUid opterm, TheoryOptermVecUid opterms) override;
    TheoryElemVecUid theoryelems() override;
    TheoryElemVecUid theoryelems(TheoryElemVecUid elems, TheoryOptermVecUid opterms, LitVecUid cond) override;
    TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems) override;
    TheoryAtomUid theoryatom(TermUid term, TheoryElemVecUid elems, FWString op, TheoryOptermUid opterm) override;
    // {{{2 theory definitions
    TheoryOpDefUid theoryopdef(Location const &loc, FWString op, unsigned priority, TheoryOperatorType type) override;
    TheoryOpDefVecUid theoryopdefs() override;
    TheoryOpDefVecUid theoryopdefs(TheoryOpDefVecUid defs, TheoryOpDefUid def) override;
    TheoryTermDefUid theorytermdef(Location const &loc, FWString name, TheoryOpDefVecUid defs) override;
    TheoryAtomDefUid theoryatomdef(Location const &loc, FWString name, unsigned arity, FWString termDef, TheoryAtomType type) override;
    TheoryAtomDefUid theoryatomdef(Location const &loc, FWString name, unsigned arity, FWString termDef, TheoryAtomType type, TheoryOpVecUid ops, FWString guardDef) override;
    TheoryDefVecUid theorydefs() override;
    TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryTermDefUid def) override;
    TheoryDefVecUid theorydefs(TheoryDefVecUid defs, TheoryAtomDefUid def) override;
    void theorydef(Location const &loc, FWString name, TheoryDefVecUid defs) override;
    // }}}2

private:
    void initNode(AST &node, ASTLocation loc, Value val);
    void initNode(AST &node, ASTLocation loc, Value val, NodeVec &children);
    void initNode(AST &node, Location const &loc, Value val);
    void initNode(AST &node, Location const &loc, Value val, NodeVec &children);
    AST newNode(ASTLocation loc, Value val);
    AST newNode(Location const &loc, Value val);
    AST newNode(ASTLocation loc, Value val, NodeVec &children);
    AST newNode(Location const &loc, Value val, NodeVec &children);
    AST newNode(Location const &loc,  NAF naf);
    AST newNode(Location const &loc,  AggregateFunction fun);
    AST newNode(Location const &loc,  Relation rel);
    AST newNode(Location const &loc, TheoryAtomType type);
    AST newNode(Location const &loc, char const *value);
    AST newNode(Location const &loc, FWString value);
    AST newNode(Location const &loc, unsigned length);
    AST newNode(Location const &loc, char const *value, NodeVec &children);
    AST typedNode(char const *type, AST node);
    NodeVec &newNodeVec();
    Location dummyloc_();
    AST littuple_(Location const &loc, LitVecUid a);
    AST littuple_(Location const &loc, BdLitVecUid a);
    AST condlit_(Location const &loc, LitUid litUid, LitVecUid litvecUid);
    TermUid fun_(Location const &loc, FWString name, TermVecUid a, bool lua);
    TermUid pool_(Location const &loc, TermUidVec const &vec);
    void directive_(Location const &loc, char const *name, NodeVec &nodeVec);

private:
    using Terms            = Indexed<AST, TermUid>;
    using TermVecs         = Indexed<TermUidVec, TermVecUid>;
    using TermVecVecs      = Indexed<TermUidVecVec, TermVecVecUid>;
    using IdVecs           = Indexed<NodeVec, IdVecUid>;
    using Lits             = Indexed<AST, LitUid>;
    using LitVecs          = Indexed<NodeVec, LitVecUid>;
    using BodyAggrElemVecs = Indexed<NodeVec, BdAggrElemVecUid>;
    using CondLitVecs      = Indexed<NodeVec, CondLitVecUid>;
    using HeadAggrElemVecs = Indexed<NodeVec, HdAggrElemVecUid>;
    using Bodies           = Indexed<NodeVec, BdLitVecUid>;
    using Heads            = Indexed<AST, HdLitUid>;
    using CSPLits          = Indexed<std::pair<Location, std::pair<AST, NodeVec>>, CSPLitUid>;
    using CSPAddTerms      = Indexed<NodeVec, CSPAddTermUid>;
    using CSPMulTerms      = Indexed<AST, CSPMulTermUid>;
    using CSPElems         = Indexed<NodeVec, CSPElemVecUid>;
    using Bounds           = Indexed<NodeVec, BoundVecUid>;

    using TheoryOpVecs      = Indexed<NodeVec, TheoryOpVecUid>;
    using TheoryTerms       = Indexed<AST, TheoryTermUid>;
    using RawTheoryTerms    = Indexed<NodeVec, TheoryOptermUid>;
    using RawTheoryTermVecs = Indexed<NodeVec, TheoryOptermVecUid>;
    using TheoryElementVecs = Indexed<NodeVec, TheoryElemVecUid>;
    using TheoryAtoms       = Indexed<std::tuple<TermUid, TheoryElemVecUid, AST>, TheoryAtomUid>;

    using TheoryOpDefs    = Indexed<AST, TheoryOpDefUid>;
    using TheoryOpDefVecs = Indexed<NodeVec, TheoryOpDefVecUid>;
    using TheoryTermDefs  = Indexed<AST, TheoryTermDefUid>;
    using TheoryAtomDefs  = Indexed<AST, TheoryAtomDefUid>;
    using TheoryDefVecs   = Indexed<NodeVec, TheoryDefVecUid>;

    Callback            cb_;
    NodeVecList         nodeVecs_;
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
};

// {{{1 declaration of ASTParser

class ASTParser {
public:
    ASTParser(Scripts &scripts, Program &prg, Output::OutputBase &out, Defines &defs, bool rewriteMinimize = false);
    void parse(AST const &node);
private:
    UnOp parseUnOp(AST const &node);
    BinOp parseBinOp(AST const &node);
    Value parseValue(AST const &node);
    TermUid parseTerm(AST const &node);
    TermVecUid parseTermVec(AST const &node);
    TermVecVecUid parseArgs(AST const &node);
    FWString parseID(AST const &node);
    NAF parseNAF(AST const &node);
    bool parseNEG(AST const &node);
    void parseProgram(AST const &node);
    LitUid parseLit(AST const &node);
    LitVecUid parseLitVec(AST const &node);
    CondLitVecUid parseConditional(CondLitVecUid uid, AST const &node);
    HdLitUid parseHead(AST const &node);
    BdLitVecUid parseBodyLit(BdLitVecUid uid, AST const &node);
    BdLitVecUid parseBody(AST const &node);
    void parseRule(AST const &node);
    bool require_(bool cond, char const *message = "parse error");
    template <class T>
    T fail_(char const *message = "parse error");

private:
    NongroundProgramBuilder prg_;
    Value directive_rule              = Value::createId("directive_rule");
    Value directive_const             = Value::createId("directive_const");
    Value directive_minimize          = Value::createId("directive_minimize");
    Value directive_show_signature    = Value::createId("directive_show_signature");
    Value directive_show              = Value::createId("directive_show");
    Value directive_python            = Value::createId("directive_python");
    Value directive_lua               = Value::createId("directive_lua");
    Value directive_program           = Value::createId("directive_program");
    Value directive_external          = Value::createId("directive_external");
    Value directive_edge              = Value::createId("directive_edge");
    Value directive_heuristic         = Value::createId("directive_heuristic");
    Value directive_project           = Value::createId("directive_project");
    Value directive_project_signature = Value::createId("directive_project_signature");
    Value directive_theory            = Value::createId("directive_theory ");
    Value literal_boolean             = Value::createId("literal_boolean");
    Value literal_predicate           = Value::createId("literal_predicate");
    Value literal_relation            = Value::createId("literal_relation");
    Value literal_csp                 = Value::createId("literal_csp");
    Value literal_conditional         = Value::createId("literal_conditional");
    Value tuple_literal               = Value::createId("tuple_literal");
    Value tuple_id                    = Value::createId("tuple_id");
    Value theory_atom                 = Value::createId("theory_atom");
    Value disjoint                    = Value::createId("disjoint");
    Value aggregate_head              = Value::createId("aggregate_head");
    Value aggregate_body              = Value::createId("aggregate_body");
    Value aggregate_lparse            = Value::createId("aggregate_lparse");
    Value id                          = Value::createId("id");
    Value naf_pos                     = Value::createId("naf_pos");
    Value naf_not                     = Value::createId("naf_not");
    Value naf_not_not                 = Value::createId("naf_not_not");
    Value neg_pos                     = Value::createId("neg_pos");
    Value neg_not                     = Value::createId("neg_not");
    Value tuple_term                  = Value::createId("tuple_term");
    Value term_pool                   = Value::createId("term_pool");
    Value term_value                  = Value::createId("term_value");
    Value term_variable               = Value::createId("term_variable");
    Value term_unary                  = Value::createId("term_unary");
    Value term_binary                 = Value::createId("term_binary");
    Value term_range                  = Value::createId("term_range");
    Value term_external               = Value::createId("term_external");
    Value term_function               = Value::createId("term_function");
    Value unop_neg                    = Value::createId("-");
    Value unop_not                    = Value::createId("~");
    Value unop_abs                    = Value::createId("|");
    Value binop_add                   = Value::createId("+");
    Value binop_or                    = Value::createId("?");
    Value binop_sub                   = Value::createId("-");
    Value binop_mod                   = Value::createId("\\");
    Value binop_mul                   = Value::createId("*");
    Value binop_xor                   = Value::createId("^");
    Value binop_pow                   = Value::createId("**");
    Value binop_div                   = Value::createId("/");
    Value binop_and                   = Value::createId("&");
};

// }}}1

} } // namespace Input Gringo

#endif // _GRINGO_PROGRAMBUILDER_HH
