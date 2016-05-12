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

#ifndef _GRINGO_GROUND_LITERALS_HH
#define _GRINGO_GROUND_LITERALS_HH

#include <gringo/terms.hh>
#include <gringo/ground/literal.hh>
#include <gringo/output/literals.hh>

namespace Gringo { 

struct Scripts;

namespace Ground {

inline double estimate(unsigned size, Term const &term, Term::VarSet const &bound) {
    Term::VarSet vars;
    term.collect(vars);
    bool found = false;
    for (auto &x : vars) {
        if (bound.find(x) != bound.end()) {
            found = true;
            break;
        }
    }
    return term.estimate(size, bound) + !found * 10000000;
}

// {{{ declaration of RangeLiteral

using RangeLiteralShared = std::pair<UTerm, UTerm>;
struct RangeLiteral : Literal {
    RangeLiteral(UTerm &&assign, UTerm &&left, UTerm &&right);
    virtual void print(std::ostream &out) const;
    virtual bool isRecursive() const;
    virtual BodyOcc *occurrence();
    virtual void collect(VarTermBoundVec &vars) const;
    virtual UIdx index(Scripts &scripts, BinderType type, Term::VarSet &bound);
    virtual std::pair<Output::Literal*,bool> toOutput();
    virtual Score score(Term::VarSet const &bound);
    virtual ~RangeLiteral();

    UTerm assign;
    RangeLiteralShared range;
};

// }}}
// {{{ declaration of ScriptLiteral

using ScriptLiteralShared = std::pair<FWString, UTermVec>;
struct ScriptLiteral : Literal {
    ScriptLiteral(UTerm &&assign, FWString name, UTermVec &&args);
    virtual void print(std::ostream &out) const;
    virtual bool isRecursive() const;
    virtual BodyOcc *occurrence();
    virtual void collect(VarTermBoundVec &vars) const;
    virtual UIdx index(Scripts &scripts, BinderType type, Term::VarSet &bound);
    virtual std::pair<Output::Literal*,bool> toOutput();
    virtual Score score(Term::VarSet const &bound);
    virtual ~ScriptLiteral();

    UTerm assign;
    ScriptLiteralShared shared;
};

// }}}
// {{{ declaration of RelationLiteral

using RelationShared = std::tuple<Relation, UTerm, UTerm>;
struct RelationLiteral : Literal {
    RelationLiteral(Relation rel, UTerm &&left, UTerm &&right);
    virtual void print(std::ostream &out) const;
    virtual bool isRecursive() const;
    virtual BodyOcc *occurrence();
    virtual void collect(VarTermBoundVec &vars) const;
    virtual UIdx index(Scripts &scripts, BinderType type, Term::VarSet &bound);
    virtual std::pair<Output::Literal*,bool> toOutput();
    virtual Score score(Term::VarSet const &bound);
    virtual ~RelationLiteral();

    RelationShared shared;
};

// }}}
// {{{ declaration of PredicateLiteral

struct PredicateLiteral : Literal, BodyOcc {
    PredicateLiteral(PredicateDomain &dom, NAF naf, UTerm &&repr);
    virtual void print(std::ostream &out) const;
    virtual UGTerm getRepr() const;
    virtual bool isPositive() const;
    virtual bool isNegative() const;
    virtual void setType(OccurrenceType x);
    virtual OccurrenceType getType() const;
    virtual bool isRecursive() const;
    virtual BodyOcc *occurrence();
    virtual void collect(VarTermBoundVec &vars) const;
    virtual DefinedBy &definedBy();
    virtual UIdx index(Scripts &scripts, BinderType type, Term::VarSet &bound);
    virtual std::pair<Output::Literal*,bool> toOutput();
    virtual Score score(Term::VarSet const &bound);
    virtual void checkDefined(LocSet &done, SigSet const &edb, UndefVec &undef) const;
    virtual ~PredicateLiteral();

    OccurrenceType type = OccurrenceType::POSITIVELY_STRATIFIED;
    UTerm repr;
    DefinedBy defs;
    PredicateDomain &domain;
    Output::PredicateLiteral gLit;   
};

struct ProjectionLiteral : PredicateLiteral {
    ProjectionLiteral(PredicateDomain &dom, UTerm &&repr, bool initialized);
    virtual UIdx index(Scripts &scripts, BinderType type, Term::VarSet &bound);
    virtual ~ProjectionLiteral();
    bool initialized_;
};

// }}}
// {{{ declaration of CSPLiteral

struct ExternalBodyOcc : BodyOcc {
    ExternalBodyOcc();
    virtual UGTerm getRepr() const;
    virtual bool isPositive() const;
    virtual bool isNegative() const;
    virtual void setType(OccurrenceType x);
    virtual OccurrenceType getType() const;
    virtual DefinedBy &definedBy();
    virtual void checkDefined(LocSet &done, SigSet const &edb, UndefVec &undef) const;
    virtual ~ExternalBodyOcc();
    DefinedBy defs;
};

using CSPLiteralShared = std::tuple<Relation, CSPAddTerm, CSPAddTerm>;
struct CSPLiteral : Literal {
    CSPLiteral(Relation rel, CSPAddTerm &&left, CSPAddTerm &&right);
    virtual void print(std::ostream &out) const;
    virtual bool isRecursive() const;
    virtual BodyOcc *occurrence();
    virtual void collectImportant(Term::VarSet &vars);
    virtual void collect(VarTermBoundVec &vars) const;
    virtual UIdx index(Scripts &scripts, BinderType type, Term::VarSet &bound);
    virtual std::pair<Output::Literal*,bool> toOutput();
    virtual Score score(Term::VarSet const &bound);
    virtual ~CSPLiteral();

    Output::CSPLiteral repr;
    CSPLiteralShared   terms;
    ExternalBodyOcc    ext;
};

// }}}

} } // namespace Ground Gringo

#endif // _GRINGO_GROUND_LITERALS_HH


