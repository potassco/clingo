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

namespace Gringo { namespace Ground {

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
    virtual std::pair<Output::LiteralId,bool> toOutput();
    virtual Score score(Term::VarSet const &bound);
    virtual bool auxiliary() const { return true; }
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
    virtual std::pair<Output::LiteralId,bool> toOutput();
    virtual Score score(Term::VarSet const &bound);
    virtual bool auxiliary() const { return true; }
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
    virtual std::pair<Output::LiteralId,bool> toOutput();
    virtual Score score(Term::VarSet const &bound);
    virtual bool auxiliary() const { return true; }
    virtual ~RelationLiteral();

    RelationShared shared;
};

// }}}
// {{{ declaration of PredicateLiteral

struct PredicateLiteral : Literal, BodyOcc {
    PredicateLiteral(bool auxiliary, PredicateDomain &domain, NAF naf, UTerm &&repr);
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
    virtual std::pair<Output::LiteralId,bool> toOutput();
    virtual Score score(Term::VarSet const &bound);
    virtual void checkDefined(LocSet &done, SigSet const &edb, UndefVec &undef) const;
    virtual bool auxiliary() const { return auxiliary_; }
    virtual ~PredicateLiteral();

    OccurrenceType type = OccurrenceType::POSITIVELY_STRATIFIED;
    bool auxiliary_;
    UTerm repr;
    DefinedBy defs;
    PredicateDomain &domain;
    Potassco::Id_t offset = 0;
    NAF naf;

};

struct ProjectionLiteral : PredicateLiteral {
    ProjectionLiteral(bool auxiliary, PredicateDomain &dom, UTerm &&repr, bool initialized);
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
class CSPLiteral : public Literal {
public:
    CSPLiteral(DomainData &data, bool auxiliary, Relation rel, CSPAddTerm &&left, CSPAddTerm &&right);
    void print(std::ostream &out) const override;
    bool isRecursive() const override;
    BodyOcc *occurrence() override;
    void collectImportant(Term::VarSet &vars) override;
    void collect(VarTermBoundVec &vars) const override;
    UIdx index(Scripts &scripts, BinderType type, Term::VarSet &bound) override;
    std::pair<Output::LiteralId,bool> toOutput() override;
    Score score(Term::VarSet const &bound) override;
    bool auxiliary() const override { return auxiliary_; }
    virtual ~CSPLiteral() noexcept;

private:
    DomainData &data_;
    CSPLiteralShared terms_;
    ExternalBodyOcc ext_;
    bool auxiliary_;
};

// }}}

} } // namespace Ground Gringo

#endif // _GRINGO_GROUND_LITERALS_HH


