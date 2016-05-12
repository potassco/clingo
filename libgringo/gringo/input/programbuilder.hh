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

#include <vector>
#include <memory>

namespace Gringo { 

struct Scripts;
struct CSPMulTerm;
struct CSPAddTerm;

namespace Input { 

class Program;
struct Statement;
struct BodyAggregate;
struct HeadAggregate;
struct Literal;
struct CSPLiteral;
struct CSPElem;

} // namespace Input

namespace Output {

struct OutputBase;

} } // namespace Output Gringo

namespace Gringo { namespace Input {

// {{{1 declaration of unique ids of program elements

enum IdVecUid         : unsigned { };
enum CSPAddTermUid    : unsigned { };
enum CSPMulTermUid    : unsigned { };
enum CSPLitUid        : unsigned { };
enum TermUid          : unsigned { };
enum TermVecUid       : unsigned { };
enum TermVecVecUid    : unsigned { };
enum LitUid           : unsigned { };
enum LitVecUid        : unsigned { };
enum CondLitVecUid    : unsigned { };
enum BdAggrElemVecUid : unsigned { };
enum HdAggrElemVecUid : unsigned { };
enum HdLitUid         : unsigned { };
enum BdLitVecUid      : unsigned { };
enum BoundVecUid      : unsigned { };
enum CSPElemVecUid    : unsigned { };

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
    virtual HdLitUid headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, HdAggrElemVecUid headaggrelemvec) = 0;
    virtual HdLitUid headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid headaggrelemvec) = 0;
    virtual HdLitUid disjunction(Location const &loc, CondLitVecUid condlitvec) = 0;
    // {{{2 bodies
    virtual BdLitVecUid body() = 0;
    virtual BdLitVecUid bodylit(BdLitVecUid body, LitUid bodylit) = 0;
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
    virtual TermUid term(Location const &loc, Value val);                                // constant
    virtual TermUid term(Location const &loc, FWString name);                            // variable
    virtual TermUid term(Location const &loc, UnOp op, TermUid a);                       // unary operation
    virtual TermUid term(Location const &loc, UnOp op, TermVecUid a);                    // unary operation
    virtual TermUid term(Location const &loc, BinOp op, TermUid a, TermUid b);           // binary operation
    virtual TermUid term(Location const &loc, TermUid a, TermUid b);                     // assignment
    virtual TermUid term(Location const &loc, FWString name, TermVecVecUid b, bool lua); // function or lua function
    virtual TermUid term(Location const &loc, TermVecUid args, bool forceTuple);         // a tuple term (or simply a term)
    virtual TermUid pool(Location const &loc, TermVecUid args);                          // a pool term

    // {{{2 term vectors
    virtual TermVecUid termvec();
    virtual TermVecUid termvec(TermVecUid uid, TermUid term);
    // {{{2 id vectors
    virtual IdVecUid idvec();
    virtual IdVecUid idvec(IdVecUid uid, Location const &loc, FWString id);
    // {{{2 csp
    virtual CSPMulTermUid cspmulterm(Location const &loc, TermUid coe, TermUid var);
    virtual CSPMulTermUid cspmulterm(Location const &loc, TermUid coe);
    virtual CSPAddTermUid cspaddterm(Location const &loc, CSPAddTermUid a, CSPMulTermUid b, bool add);
    virtual CSPAddTermUid cspaddterm(Location const &loc, CSPMulTermUid a);
    virtual LitUid csplit(CSPLitUid a);
    virtual CSPLitUid csplit(Location const &loc, CSPLitUid a, Relation rel, CSPAddTermUid b);
    virtual CSPLitUid csplit(Location const &loc, CSPAddTermUid a, Relation rel, CSPAddTermUid b);
    // {{{2 term vector vectors
    virtual TermVecVecUid termvecvec();
    virtual TermVecVecUid termvecvec(TermVecVecUid uid, TermVecUid termvecUid);
    // {{{2 literals
    virtual LitUid boollit(Location const &loc, bool type);
    virtual LitUid predlit(Location const &loc, NAF naf, bool neg, FWString name, TermVecVecUid argvecvecUid);
    virtual LitUid rellit(Location const &loc, Relation rel, TermUid termUidLeft, TermUid termUidRight);
    // {{{2 literal vectors
    virtual LitVecUid litvec();
    virtual LitVecUid litvec(LitVecUid uid, LitUid literalUid);
    // {{{2 conditional literal vectors
    virtual CondLitVecUid condlitvec();
    virtual CondLitVecUid condlitvec(CondLitVecUid uid, LitUid lit, LitVecUid litvec);
    // {{{2 body aggregate elements
    virtual BdAggrElemVecUid bodyaggrelemvec();
    virtual BdAggrElemVecUid bodyaggrelemvec(BdAggrElemVecUid uid, TermVecUid termvec, LitVecUid litvec);
    // {{{2 head aggregate elements
    virtual HdAggrElemVecUid headaggrelemvec();
    virtual HdAggrElemVecUid headaggrelemvec(HdAggrElemVecUid uid, TermVecUid termvec, LitUid lit, LitVecUid litvec);
    // {{{2 bounds
    virtual BoundVecUid boundvec();
    virtual BoundVecUid boundvec(BoundVecUid uid, Relation rel, TermUid term);
    // {{{2 heads
    virtual HdLitUid headlit(LitUid lit);
    virtual HdLitUid headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, HdAggrElemVecUid headaggrelemvec);
    virtual HdLitUid headaggr(Location const &loc, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid headaggrelemvec);
    virtual HdLitUid disjunction(Location const &loc, CondLitVecUid condlitvec);
    // {{{2 bodies
    virtual BdLitVecUid body();
    virtual BdLitVecUid bodylit(BdLitVecUid body, LitUid bodylit);
    virtual BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, BdAggrElemVecUid bodyaggrelemvec);
    virtual BdLitVecUid bodyaggr(BdLitVecUid body, Location const &loc, NAF naf, AggregateFunction fun, BoundVecUid bounds, CondLitVecUid bodyaggrelemvec);
    virtual BdLitVecUid conjunction(BdLitVecUid body, Location const &loc, LitUid head, LitVecUid litvec);
    virtual BdLitVecUid disjoint(BdLitVecUid body, Location const &loc, NAF naf, CSPElemVecUid elem);
    // {{{2 csp constraint elements
    virtual CSPElemVecUid cspelemvec();
    virtual CSPElemVecUid cspelemvec(CSPElemVecUid uid, Location const &loc, TermVecUid termvec, CSPAddTermUid addterm, LitVecUid litvec);
    // {{{2 statements
    virtual void rule(Location const &loc, HdLitUid head);
    virtual void rule(Location const &loc, HdLitUid head, BdLitVecUid body);
    virtual void define(Location const &loc, FWString name, TermUid value, bool defaultDef);
    virtual void optimize(Location const &loc, TermUid weight, TermUid priority, TermVecUid cond, BdLitVecUid body);
    virtual void showsig(Location const &loc, FWSignature sig, bool csp);
    virtual void show(Location const &loc, TermUid t, BdLitVecUid body, bool csp);
    virtual void python(Location const &loc, FWString code);
    virtual void lua(Location const &loc, FWString code);
    virtual void block(Location const &loc, FWString name, IdVecUid args);
    virtual void external(Location const &loc, LitUid head, BdLitVecUid body);
    // }}}2
    virtual ~NongroundProgramBuilder();

private:
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
    Scripts            &scripts_;
    Program            &prg_;
    Output::OutputBase &out;
    Defines            &defs_;
    bool                rewriteMinimize_;
};

// }}}1

} } // namespace Input Gringo

#endif // _GRINGO_PROGRAMBUILDER_HH
