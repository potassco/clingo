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

#ifndef _GRINGO_GROUND_STATEMENT_HH
#define _GRINGO_GROUND_STATEMENT_HH

#include <gringo/ground/literal.hh>
#include <gringo/ground/dependency.hh>

namespace Gringo { namespace Ground {

struct Queue;

// {{{ declaration of Statement

struct Statement;
using UStm       = std::unique_ptr<Statement>;
using UStmVec    = std::vector<UStm>;
using UStmVecVec = std::vector<UStmVec>;

struct Statement : Printable {
    using Dep = Dependency<UStm, HeadOccurrence>;
    virtual bool isNormal() const = 0;
    virtual void analyze(Dep::Node &node, Dep &dep) = 0;
    virtual void startLinearize(bool active) = 0;
    virtual void linearize(Scripts &scripts, bool positive) = 0;
    virtual void enqueue(Queue &q) = 0;
    virtual ~Statement() { }
};

// }}}

} } // namespace Ground Gringo

#endif // _GRINGO_GROUND_STATEMENT_HH
