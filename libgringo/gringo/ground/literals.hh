// {{{ MIT License

// Copyright 2017 Roland Kaminski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

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
    void print(std::ostream &out) const override;
    bool isRecursive() const override;
    BodyOcc *occurrence() override;
    void collect(VarTermBoundVec &vars) const override;
    UIdx index(Context &context, BinderType type, Term::VarSet &bound) override;
    std::pair<Output::LiteralId,bool> toOutput(Logger &log) override;
    Score score(Term::VarSet const &bound, Logger &log) override;
    bool auxiliary() const override { return true; }
    virtual ~RangeLiteral();

    UTerm assign;
    RangeLiteralShared range;
};

// }}}
// {{{ declaration of ScriptLiteral

using ScriptLiteralShared = std::pair<String, UTermVec>;
struct ScriptLiteral : Literal {
    ScriptLiteral(UTerm &&assign, String name, UTermVec &&args);
    void print(std::ostream &out) const override;
    bool isRecursive() const override;
    BodyOcc *occurrence() override;
    void collect(VarTermBoundVec &vars) const override;
    UIdx index(Context &context, BinderType type, Term::VarSet &bound) override;
    std::pair<Output::LiteralId,bool> toOutput(Logger &log) override;
    Score score(Term::VarSet const &bound, Logger &log) override;
    bool auxiliary() const override { return true; }
    virtual ~ScriptLiteral();

    UTerm assign;
    ScriptLiteralShared shared;
};

// }}}
// {{{ declaration of RelationLiteral

using RelationShared = std::tuple<Relation, UTerm, UTerm>;
struct RelationLiteral : Literal {
    RelationLiteral(Relation rel, UTerm &&left, UTerm &&right);
    void print(std::ostream &out) const override;
    bool isRecursive() const override;
    BodyOcc *occurrence() override;
    void collect(VarTermBoundVec &vars) const override;
    UIdx index(Context &context, BinderType type, Term::VarSet &bound) override;
    std::pair<Output::LiteralId,bool> toOutput(Logger &log) override;
    Score score(Term::VarSet const &bound, Logger &log) override;
    bool auxiliary() const override { return true; }
    virtual ~RelationLiteral();

    RelationShared shared;
};

// }}}
// {{{ declaration of PredicateLiteral

struct PredicateLiteral : Literal, BodyOcc {
    PredicateLiteral(bool auxiliary, PredicateDomain &domain, NAF naf, UTerm &&repr);
    void print(std::ostream &out) const override;
    UGTerm getRepr() const override;
    bool isPositive() const override;
    bool isNegative() const override;
    void setType(OccurrenceType x) override;
    OccurrenceType getType() const override;
    bool isRecursive() const override;
    BodyOcc *occurrence() override;
    void collect(VarTermBoundVec &vars) const override;
    DefinedBy &definedBy() override;
    UIdx index(Context &context, BinderType type, Term::VarSet &bound) override;
    std::pair<Output::LiteralId,bool> toOutput(Logger &log) override;
    Score score(Term::VarSet const &bound, Logger &log) override;
    void checkDefined(LocSet &done, SigSet const &edb, UndefVec &undef) const override;
    bool auxiliary() const override { return auxiliary_; }
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
    UIdx index(Context &context, BinderType type, Term::VarSet &bound) override;
    virtual ~ProjectionLiteral();
    bool initialized_;
};

// }}}

} } // namespace Ground Gringo

#endif // _GRINGO_GROUND_LITERALS_HH


