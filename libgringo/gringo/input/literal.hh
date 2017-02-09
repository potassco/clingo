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

#ifndef _GRINGO_INPUT_LITERAL_HH
#define _GRINGO_INPUT_LITERAL_HH

#include <gringo/term.hh>
#include <gringo/domain.hh>
#include <gringo/input/types.hh>

namespace Gringo { namespace Input {

// {{{ declaration of Literal

struct Projection {
    Projection(UTerm &&projected, UTerm &&project);
    Projection(Projection &&);
    Projection &operator=(Projection &&);
    ~Projection();
    operator Term const &() const;

    UTerm projected;
    UTerm project;
    bool  done = false;
};

struct Projections {
    using ProjectionMap = UniqueVec<Projection, std::hash<Term>, std::equal_to<Term>>;

    Projections();
    Projections(Projections &&x);
    ProjectionMap::Iterator begin();
    ProjectionMap::Iterator end();
    ~Projections();
    UTerm add(Term &term);

    ProjectionMap proj;
};

struct Literal : Printable, Hashable, Locatable, Comparable<Literal>, Clonable<Literal> {
    using AssignVec = std::vector<std::pair<UTerm, UTerm>>;

    virtual unsigned projectScore() const { return 2; }
    //! Removes all occurrences of PoolTerm instances.
    //! Returns all unpooled incarnations of the literal.
    //! \note The literal becomes unusable after the method returns.
    //! \post The returned pool does not contain PoolTerm instances.
    virtual ULitVec unpool(bool beforeRewrite) const = 0;
    //! Simplifies the literal.
    //! Flag positional=true indicates that anonymous variables in this literal can be projected.
    //! Flag singleton=true disables projections for positive predicate literals (e.g., in non-monotone aggregates)
    virtual bool simplify(Logger &log, Projections &project, SimplifyState &state, bool positional = true, bool singleton = false) = 0;
    //! Collects variables.
    //! \pre Must be called after simplify to properly account for bound variables.
    virtual void collect(VarTermBoundVec &vars, bool bound) const = 0;
    //! Removes non-invertible arithmetics.
    //! \note This method will not be called for head literals.
    virtual void rewriteArithmetics(Term::ArithmeticsMap &arith, AssignVec &assign, AuxGen &auxGen) = 0;
    virtual void toTuple(UTermVec &tuple, int &id) = 0;
    virtual Symbol isEDB() const;
    virtual bool hasPool(bool beforeRewrite) const = 0;
    virtual void replace(Defines &dx) = 0;
    virtual Ground::ULit toGround(DomainData &x, bool auxiliary) const = 0;
    virtual ULit shift(bool negate) = 0;
    virtual UTerm headRepr() const = 0;
    virtual bool auxiliary() const = 0;
    virtual void auxiliary(bool auxiliary) = 0;
    virtual void getNeg(std::function<void (Sig)> f) const = 0;
    virtual ~Literal() { }
};

// }}}

} } // namespace Input Gringo

GRINGO_HASH(Gringo::Input::Literal)

#endif // _GRINGO_INPUT_LITERAL_HH
