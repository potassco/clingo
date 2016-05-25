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

#ifndef _GRINGO_INPUT_AGGREGATE_HH
#define _GRINGO_INPUT_AGGREGATE_HH

#include <gringo/input/literal.hh>
#include <gringo/safetycheck.hh>
#include <gringo/terms.hh>
#include <list>

namespace Gringo {

namespace Ground {
enum class RuleType : unsigned short;
using ULitVec = std::vector<ULit>;
class Statement;
using UStm = std::unique_ptr<Statement>;
using UStmVec = std::vector<UStm>;
}

namespace Input {

// {{{ declaration of auxiliary types

using CondLit = std::pair<ULit, ULitVec>;
using CondLitVec = std::vector<CondLit>;
using BodyAggrElem = std::pair<UTermVec, ULitVec>;
using BodyAggrElemVec = std::vector<BodyAggrElem>;

using HeadAggrElem = std::tuple<UTermVec, ULit, ULitVec>;
using HeadAggrElemVec = std::vector<HeadAggrElem>;

struct BodyAggregate;
using UBodyAggr = std::unique_ptr<BodyAggregate>;
using UBodyAggrVec = std::vector<UBodyAggr>;

struct HeadAggregate;
using UHeadAggr = std::unique_ptr<HeadAggregate>;
using UHeadAggrVec = std::vector<UHeadAggr>;

// }}}

// {{{ declaration of AssignLevel

struct AssignLevel {
    typedef std::unordered_map<String, unsigned> BoundSet;

    void add(VarTermBoundVec &vars);
    AssignLevel &subLevel();
    void assignLevels();
    void assignLevels(unsigned level, BoundSet const &bound);
    virtual ~AssignLevel();

    std::list<AssignLevel> childs;
    std::unordered_map<String, std::vector<VarTerm*>> occurr;
};

// }}}
// {{{ declaration of CheckLevel

struct CheckLevel {
    struct Ent {
        bool operator<(Ent const &) const;
    };
    using SC     = SafetyChecker<VarTerm*, Ent>;
    using VarMap = std::unordered_map<String, SC::VarNode *>;

    CheckLevel(Location const &loc, Printable const &p);
    CheckLevel(CheckLevel &&);
    SC::VarNode &var(VarTerm &var);
    void check();
    ~CheckLevel();

    Location         loc;
    Printable const &p;
    SC               dep;
    SC::EntNode     *current = 0;
    VarMap           vars;
};
using ChkLvlVec = std::vector<CheckLevel>;
void addVars(ChkLvlVec &levels, VarTermBoundVec &vars);

// }}}
// {{{ declaration of ToGroundArg

using CreateLit     = std::function<void (Ground::ULitVec &, bool, bool)>;
using CreateStm     = std::function<Ground::UStm (Ground::ULitVec &&)>;
using CreateStmVec  = std::vector<CreateStm>;
using CreateBody    = std::pair<CreateLit, CreateStmVec>;
using CreateBodyVec = std::vector<CreateBody>;
using CreateHead    = CreateStm;

struct ToGroundArg {
    ToGroundArg(unsigned &auxNames, DomainData &domains);
    String newId(bool increment = true);
    UTermVec getGlobal(VarTermBoundVec const &vars);
    UTermVec getLocal(VarTermBoundVec const &vars);
    UTerm newId(UTermVec &&global, Location const &loc, bool increment = true);
    template <class T>
    UTerm newId(T const &x) {
        VarTermBoundVec vars;
        x.collect(vars);
        return newId(getGlobal(vars), x.loc());
    }
    ~ToGroundArg();

    unsigned   &auxNames;
    DomainData &domains;
};

// }}}

// {{{ declaration of BodyAggregate

struct BodyAggregate : Printable, Hashable, Locatable, Clonable<BodyAggregate>, Comparable<BodyAggregate> {
    //! Removes RawTheoryTerms from TheoryLiterals
    virtual void initTheory(TheoryDefs &def) { (void)def; };
    virtual unsigned projectScore() const { return 2; }
    //! Unpool the aggregate and aggregate elements.
    virtual void unpool(UBodyAggrVec &x, bool beforeRewrite) = 0;
    //! Simplify the aggregate and aggregate elements.
    //! \pre Must be called after unpool.
    virtual bool simplify(Projections &project, SimplifyState &state, bool singleton, MessagePrinter &log) = 0;
    //! Assign levels to variables using the VarCollector.
    //! \pre Must be called after simplify.
    virtual void assignLevels(AssignLevel &lvl) = 0;
    virtual void check(ChkLvlVec &lvl, MessagePrinter &log) const = 0;
    //! Rewrite arithmetics.
    //! \pre Requires variables assigned to levels.
    virtual void rewriteArithmetics(Term::ArithmeticsMap &arith, Literal::AssignVec &assign, AuxGen &auxGen) = 0;
    //! Rewrite aggregates.
    //! Separates assignment aggregates from ordinary bounds, and
    //! transforms literal aggregates into tuple aggregates.
    //! Returns false iff the aggregates must be removed.
    //! Argument aggr contains the rewritten aggregates.
    virtual bool rewriteAggregates(UBodyAggrVec &aggr) = 0;
    //! Returns true if the aggregate is an assignment aggregate.
    //! \note Does not consider relation literals.
    virtual void removeAssignment() = 0;
    virtual bool isAssignment() const = 0;
    //! Collects all variables occuring in the aggregate.
    //! Occurrences bound by the aggregate are marked as such
    //! (occurrences bound in nested scopes are not marked).
    virtual void collect(VarTermBoundVec &vars) const = 0;
    virtual bool hasPool(bool beforeRewrite) const = 0;
    virtual void replace(Defines &dx) = 0;
    virtual CreateBody toGround(ToGroundArg &x, Ground::UStmVec &stms) const = 0;
    virtual ~BodyAggregate() { }
};

// }}}

// {{{ declaration of HeadAggregate

struct HeadAggregate : Printable, Hashable, Locatable, Clonable<HeadAggregate>, Comparable<HeadAggregate> {
    //! Removes RawTheoryTerms from TheoryLiterals
    virtual void initTheory(TheoryDefs &def, bool hasBody) { (void)def; (void)hasBody; }
    virtual bool isPredicate() const { return false; }
    //! Unpool the aggregate and aggregate elements.
    virtual void unpool(UHeadAggrVec &x, bool beforeRewrite) = 0;
    //! Simplify the aggregate and aggregate elements.
    //! \pre Must be called after unpool.
    virtual bool simplify(Projections &project, SimplifyState &state) = 0;
    //! Assign levels to variables using the VarCollector.
    //! \pre Must be called after simplify.
    virtual void assignLevels(AssignLevel &lvl) = 0;
    virtual void check(ChkLvlVec &lvl, MessagePrinter &log) const = 0;
    //! Rewrite arithmetics.
    //! \pre Requires variables assigned to levels.
    virtual void rewriteArithmetics(Term::ArithmeticsMap &arith, AuxGen &auxGen) = 0;
    virtual UHeadAggr rewriteAggregates(UBodyAggrVec &aggr) = 0;
    //! Collects all variables occuring in the aggregate.
    //! Occurrences bound by the aggregate are marked as such
    //! (occurrences bound in nested scopes are not marked).
    virtual void collect(VarTermBoundVec &vars) const = 0;
    virtual bool hasPool(bool beforeRewrite) const = 0;
    virtual void replace(Defines &dx) = 0;
    virtual CreateHead toGround(ToGroundArg &x, Ground::UStmVec &stms, Ground::RuleType type) const = 0;
    virtual Symbol isEDB() const;
    virtual ~HeadAggregate() { }
};

// }}}

} } // namespace Input Gringo

GRINGO_HASH(Gringo::Input::BodyAggregate)
GRINGO_HASH(Gringo::Input::HeadAggregate)

#endif // _GRINGO_INPUT_AGGREGATE_HH
