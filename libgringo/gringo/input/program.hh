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

#ifndef _GRINGO_INPUT_PROGRAM_HH
#define _GRINGO_INPUT_PROGRAM_HH

#include <gringo/terms.hh>
#include <gringo/input/literal.hh>
#include <gringo/input/statement.hh>
#include <gringo/ground/program.hh>

namespace Gringo { namespace Input {

// {{{ declaration of Program

using IdVec = Ground::IdVec;

struct Block {
    Block(Location const &loc, String name, IdVec &&params);
    Block(Block&&);
    Block &operator=(Block &&);
    ~Block();

    Term const &sig() const;
    operator Term const &() const;

    Location        loc;
    String          name;
    IdVec           params;
    SymVec          addedEdb;
    Ground::SEdb    edb;
    UStmVec         addedStms;
    UStmVec         stms;
};
using BlockMap = UniqueVec<Block, HashKey<Term>, EqualToKey<Term>>;

class Program {
public:
    Program();
    Program(Program &&x);
    void begin(Location const &loc, String name, IdVec &&params);
    void add(UStm &&stm);
    void add(TheoryDef &&def, Logger &log);
    void rewrite(Defines &defs, Logger &log);
    void check(Logger &log);
    void print(std::ostream &out) const;
    Ground::Program toGround(DomainData &domains, Logger &log);
    ~Program();

private:
    void rewriteDots();
    void rewriteArithmetics();
    void unpool();

    unsigned              auxNames_ = 0;
    Ground::LocSet        locs_;
    Ground::SigSet        sigs_;
    BlockMap              blocks_;
    Block                *current_;
    Projections           project_;
    UStmVec               stms_;
    TheoryDefs            theoryDefs_;
};

std::ostream &operator<<(std::ostream &out, Program const &p);

// }}}

} } // namespace Input Gringo

#endif //_GRINGO_INPUT_PROGRAM_HH
