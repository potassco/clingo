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

#ifndef _GRINGO_INPUT_LITERALS_HH
#define _GRINGO_INPUT_LITERALS_HH

#include <gringo/terms.hh>
#include <gringo/input/literal.hh>

namespace Gringo { namespace Input {

// {{{1 declaration of PredicateLiteral

struct PredicateLiteral : Literal {
    PredicateLiteral(NAF naf, UTerm &&repr, bool auxiliary = false);
    unsigned projectScore() const override;
    void collect(VarTermBoundVec &vars, bool bound) const override;
    void toTuple(UTermVec &tuple, int &id) const override;
    PredicateLiteral *clone() const override;
    void print(std::ostream &out) const override;
    bool operator==(Literal const &other) const override;
    size_t hash() const override;
    bool simplify(Logger &log, Projections &project, SimplifyState &state, bool positional = true, bool singleton = false) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, RelationVec &assign, AuxGen &auxGen) override;
    ULitVec unpool(bool beforeRewrite, bool head) const override;
    Symbol isEDB() const override;
    bool hasPool(bool beforeRewrite, bool head) const override;
    void replace(Defines &dx) override;
    Ground::ULit toGround(DomainData &x, bool auxiliary) const override;
    ULit shift(bool negate) override;
    UTerm headRepr() const override;
    bool auxiliary() const override { return auxiliary_; }
    void auxiliary(bool auxiliary) override { auxiliary_ = auxiliary; }
    virtual ~PredicateLiteral();

    NAF naf;
    bool auxiliary_;
    UTerm repr;
};

struct ProjectionLiteral : PredicateLiteral {
    ProjectionLiteral(UTerm &&repr);
    ProjectionLiteral *clone() const override;
    ULitVec unpool(bool beforeRewrite, bool head) const override;
    Ground::ULit toGround(DomainData &x, bool auxiliary) const override;
    ULit shift(bool negate) override;
    virtual ~ProjectionLiteral();
    mutable bool initialized_;
};

// }}}
// {{{ declaration of RelationLiteral

class RelationLiteral : public Literal {
public:
    using Terms = std::vector<std::pair<Relation, UTerm>>;
    RelationLiteral(NAF naf, UTerm &&left, Terms &&right);
    RelationLiteral(Relation rel, UTerm &&left, UTerm &&right);
    RelationLiteral(NAF naf, Relation rel, UTerm &&left, UTerm &&right);

    RelationLiteral() = delete;
    RelationLiteral(RelationLiteral const &other) = delete;
    RelationLiteral(RelationLiteral &&other) noexcept = default;
    RelationLiteral &operator=(RelationLiteral const &other) = delete;
    RelationLiteral &operator=(RelationLiteral &&other) noexcept = default;
    ~RelationLiteral() noexcept override;
    static ULit make(Term::LevelMap::value_type &x);
    static ULit make(Literal::RelationVec::value_type &x);
    // {{{ Term interface
    bool needSetShift() const override;
    unsigned projectScore() const override;
    void collect(VarTermBoundVec &vars, bool bound) const override;
    void toTuple(UTermVec &tuple, int &id) const override;
    RelationLiteral *clone() const override;
    void print(std::ostream &out) const override;
    bool operator==(Literal const &other) const override;
    size_t hash() const override;
    bool simplify(Logger &log, Projections &project, SimplifyState &state, bool positional = true, bool singleton = false) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, RelationVec &assign, AuxGen &auxGen) override;
    ULitVec unpool(bool beforeRewrite, bool head) const override;
    ULitVecVec unpoolComparison() const override;
    bool hasUnpoolComparison() const override;
    bool hasPool(bool beforeRewrite, bool head) const override;
    void replace(Defines &dx) override;
    Ground::ULit toGround(DomainData &x, bool auxiliary) const override;
    UTerm headRepr() const override;
    ULit shift(bool negate) override;
    bool auxiliary() const override { return true; }
    void auxiliary(bool aux) override { }
    // }}}

private:
    UTerm left_;
    Terms right_;
    NAF naf_;
};

// }}}
// {{{ declaration of RangeLiteral

struct RangeLiteral : Literal {
    RangeLiteral(UTerm &&assign, UTerm &&lower, UTerm &&upper);
    void collect(VarTermBoundVec &vars, bool bound) const override;
    void toTuple(UTermVec &tuple, int &id) const override;
    RangeLiteral *clone() const override;
    void print(std::ostream &out) const override;
    bool operator==(Literal const &other) const override;
    size_t hash() const override;
    bool simplify(Logger &log, Projections &project, SimplifyState &state, bool positional = true, bool singleton = false) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, RelationVec &assign, AuxGen &auxGen) override;
    ULitVec unpool(bool beforeRewrite, bool head) const override;
    bool hasPool(bool beforeRewrite, bool head) const override;
    void replace(Defines &dx) override;
    Ground::ULit toGround(DomainData &x, bool auxiliary) const override;
    ULit shift(bool negate) override;
    UTerm headRepr() const override;
    bool auxiliary() const override { return true; }
    void auxiliary(bool) override { }
    virtual ~RangeLiteral();
    static ULit make(SimplifyState::DotsMap::value_type &dots);

    UTerm assign;
    UTerm lower;
    UTerm upper;
};

// }}}
// {{{ declaration of ScriptLiteral

struct ScriptLiteral : Literal {
    ScriptLiteral(UTerm &&assign, String name, UTermVec &&args);
    void collect(VarTermBoundVec &vars, bool bound) const override;
    void toTuple(UTermVec &tuple, int &id) const override;
    ScriptLiteral *clone() const override;
    void print(std::ostream &out) const override;
    bool operator==(Literal const &other) const override;
    size_t hash() const override;
    bool simplify(Logger &log, Projections &project, SimplifyState &state, bool positional = true, bool singleton = false) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, RelationVec &assign, AuxGen &auxGen) override;
    ULitVec unpool(bool beforeRewrite, bool head) const override;
    bool hasPool(bool beforeRewrite, bool head) const override;
    void replace(Defines &dx) override;
    Ground::ULit toGround(DomainData &x, bool auxiliary) const override;
    ULit shift(bool negate) override;
    UTerm headRepr() const override;
    bool auxiliary() const override { return true; }
    void auxiliary(bool) override { }
    virtual ~ScriptLiteral();
    static ULit make(SimplifyState::ScriptMap::value_type &script);

    UTerm assign;
    String name;
    UTermVec args;
};

// }}}
// {{{ declaration of VoidLiteral

struct VoidLiteral : Literal {
    VoidLiteral();
    unsigned projectScore() const override { return 0; }
    void collect(VarTermBoundVec &vars, bool bound) const override;
    void toTuple(UTermVec &tuple, int &id) const override;
    VoidLiteral *clone() const override;
    void print(std::ostream &out) const override;
    bool operator==(Literal const &other) const override;
    size_t hash() const override;
    bool simplify(Logger &log, Projections &project, SimplifyState &state, bool positional = true, bool singleton = false) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, RelationVec &assign, AuxGen &auxGen) override;
    ULitVec unpool(bool beforeRewrite, bool head) const override;
    bool hasPool(bool beforeRewrite, bool head) const override;
    void replace(Defines &dx) override;
    Ground::ULit toGround(DomainData &x, bool auxiliary) const override;
    ULit shift(bool negate) override;
    UTerm headRepr() const override;
    bool auxiliary() const override { return true; }
    void auxiliary(bool) override { }
    virtual ~VoidLiteral();
};

// }}}
// {{{ declaration of BooleanSetLiteral

// This literals simply evaluates to a fixed Boolean but also has a set
// representation usable in set-based aggregates.

class BooleanSetLiteral : public Literal {
public:
    BooleanSetLiteral(UTerm repr, bool value);
    BooleanSetLiteral() = delete;
    BooleanSetLiteral(BooleanSetLiteral const &other) = delete;
    BooleanSetLiteral(BooleanSetLiteral &&other) noexcept;
    BooleanSetLiteral &operator=(BooleanSetLiteral const &other) = delete;
    BooleanSetLiteral &operator=(BooleanSetLiteral &&other) noexcept;
    bool triviallyTrue() const override { return value_; }
    unsigned projectScore() const override;
    void collect(VarTermBoundVec &vars, bool bound) const override;
    void toTuple(UTermVec &tuple, int &id) const override;
    BooleanSetLiteral *clone() const override;
    void print(std::ostream &out) const override;
    bool operator==(Literal const &other) const override;
    size_t hash() const override;
    bool simplify(Logger &log, Projections &project, SimplifyState &state, bool positional = true, bool singleton = false) override;
    void rewriteArithmetics(Term::ArithmeticsMap &arith, RelationVec &assign, AuxGen &auxGen) override;
    ULitVec unpool(bool beforeRewrite, bool head) const override;
    bool hasPool(bool beforeRewrite, bool head) const override;
    void replace(Defines &dx) override;
    Ground::ULit toGround(DomainData &x, bool auxiliary) const override;
    ULit shift(bool negate) override;
    UTerm headRepr() const override;
    bool auxiliary() const override;
    void auxiliary(bool aux) override;
    ~BooleanSetLiteral() noexcept override;
private:
    UTerm repr_;
    bool value_;
};

// }}}

} } // namespace Input Gringo

#endif // _GRINGO_INPUT_LITERALS_HH

