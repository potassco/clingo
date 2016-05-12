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

#ifndef _GRINGO_INPUT_LITERAL_HH
#define _GRINGO_INPUT_LITERAL_HH

#include <gringo/term.hh>
#include <gringo/domain.hh>

namespace Gringo { 

namespace Ground { 
    struct Literal;
    using ULit = std::unique_ptr<Literal>;
}

namespace Input {

// {{{ declaration of Literal

struct Literal;
using ULit    = std::unique_ptr<Literal>;
using ULitVec = std::vector<ULit>;

struct Projection {
    Projection(UTerm &&projected, UTerm &&project);
    ~Projection();
    operator Term const &() const;

    UTerm const projected;
    UTerm const project;
    bool        done = false;
};

struct Projections {
    using ProjectionMap = unique_list<Projection, identity<Term>>;

    Projections();
    Projections(Projections &&x);
    ProjectionMap::iterator begin();
    ProjectionMap::iterator end();
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
    virtual bool simplify(Projections &project, SimplifyState &state, bool positional = true, bool singleton = false) = 0;
    //! Collects variables.
    //! \pre Must be called after simplify to properly account for bound variables.
    virtual void collect(VarTermBoundVec &vars, bool bound) const = 0;
    //! Removes non-invertible arithmetics.
    //! \note This method will not be called for head literals.
    virtual void rewriteArithmetics(Term::ArithmeticsMap &arith, AssignVec &assign, AuxGen &auxGen) = 0;
    virtual void toTuple(UTermVec &tuple, int &id) = 0;
    virtual Value isEDB() const;
    virtual bool hasPool(bool beforeRewrite) const = 0;
    virtual void replace(Defines &dx) = 0;
    virtual Ground::ULit toGround(PredDomMap &x) const = 0;
    virtual ULit shift(bool negate) = 0;
    virtual UTerm headRepr() const = 0;
    virtual ~Literal() { }
};

// }}}

} } // namespace Input Gringo

GRINGO_HASH(Gringo::Input::Literal)

#endif // _GRINGO_INPUT_LITERAL_HH
