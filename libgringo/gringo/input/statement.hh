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

#ifndef _GRINGO_INPUT_STATEMENT_HH
#define _GRINGO_INPUT_STATEMENT_HH

#include <gringo/printable.hh>
#include <gringo/locatable.hh>

namespace Gringo { 
    
class Defines;
struct Term;

using UTerm = std::unique_ptr<Term>;

namespace Ground {

// {{{ forward declarations

struct Statement;
using UStm = std::unique_ptr<Statement>;
using UStmVec = std::vector<UStm>;

// }}}

} // namespace Ground

namespace Input {

// {{{ forward declarations

struct Literal;
struct BodyAggregate;
struct ToGroundArg;
struct Statement;
struct Projections;
struct HeadAggregate;

using UTermVec     = std::vector<UTerm>;
using UHeadAggr    = std::unique_ptr<HeadAggregate>;
using ULit         = std::unique_ptr<Literal>;
using UBodyAggr    = std::unique_ptr<BodyAggregate>;
using UBodyAggrVec = std::vector<UBodyAggr>;

// }}}
// {{{ declaration of Statement

using UStm = std::unique_ptr<Statement>;
using UStmVec = std::vector<UStm>;

enum class StatementType { RULE, EXTERNAL, WEAKCONSTRAINT };

struct Statement : Printable, Locatable {
    Statement(UTerm &&weight, UTerm &&prioriy, UTermVec &&tuple, UBodyAggrVec &&body);
    Statement(UTermVec &&tuple, UBodyAggrVec &&body);
    Statement(UHeadAggr &&head, UBodyAggrVec &&body, StatementType type);
    virtual UStmVec unpool(bool beforeRewrite);
    virtual bool rewrite1(Projections &project);
    virtual void rewrite2();
    virtual Value isEDB() const;
    virtual void print(std::ostream &out) const;
    virtual bool hasPool(bool beforeRewrite) const;
    virtual bool check() const;
    virtual void replace(Defines &dx);
    virtual void toGround(ToGroundArg &x, Ground::UStmVec &stms) const;
    virtual void add(ULit &&lit);
    virtual ~Statement();

    UHeadAggr     head;
    UBodyAggrVec  body;
    StatementType type;
};

// }}}

} } // namespace Input Gringo

#endif // _GRINGO_INPUT_STATEMENT_HH
