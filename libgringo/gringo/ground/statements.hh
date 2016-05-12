// {{{ GPL License 

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

#ifndef _GRINGO_GROUND_STATEMENTS_HH
#define _GRINGO_GROUND_STATEMENTS_HH

#include <gringo/ground/statement.hh>
#include <gringo/ground/literals.hh>
#include <gringo/output/statements.hh>
#include <gringo/intervals.hh>

namespace Gringo { namespace Ground {

// {{{1 declaration of HeadDefinition

struct HeadDefinition : HeadOccurrence {
    HeadDefinition(UTerm &&repr, Domain *domain);
    HeadDefinition(HeadDefinition &&) = default;
    UGTerm getRepr() const;
    void collectImportant(Term::VarSet &vars);
    void enqueue(Queue &queue);
    virtual void defines(IndexUpdater &update, Instantiator *inst);
    virtual ~HeadDefinition();

    UTerm repr;
    using OffsetMap = std::unordered_map<IndexUpdater*, unsigned>;
    using RInstVec = std::vector<std::reference_wrapper<Instantiator>>;
    using EnqueueVec = std::vector<std::pair<IndexUpdater*, RInstVec>>;
    Domain *domain;
    OffsetMap offsets;
    EnqueueVec enqueueVec;
    bool active = false;
};
using HeadDefVec = std::vector<HeadDefinition>;
using UHeadDef = std::unique_ptr<HeadDefinition>;

// }}}1
// {{{1 declaration of AbstractStatement

struct AbstractStatement : Statement, SolutionCallback {
    AbstractStatement(UTerm &&repr, Domain *domain, ULitVec &&lits, ULitVec &&auxLits);
    virtual ~AbstractStatement();

    virtual void collectImportant(Term::VarSet &vars);
    virtual void printBody(std::ostream &out) const;
    // {{{2 Statement interface
    virtual bool isNormal() const; // false by default
    virtual void analyze(Dep::Node &node, Dep &dep);
    virtual void startLinearize(bool active);
    virtual void linearize(Scripts &scripts, bool positive);
    virtual void enqueue(Queue &q);
    // {{{2 SolutionCallback  interface
    virtual void printHead(std::ostream &out) const;
    virtual void propagate(Queue &queue);
    // {{{2 Printable interface
    virtual void print(std::ostream &out) const;
    // }}}2

    HeadDefinition         def;
    ULitVec                lits;
    ULitVec                auxLits;
    InstVec                insts;
};
// }}}1

// {{{1 declaration of Rule

enum class RuleType : unsigned short { EXTERNAL, NORMAL };

struct Rule : AbstractStatement {
    Rule(PredicateDomain *domain, UTerm &&repr, ULitVec &&body, RuleType type);
    virtual ~Rule();

    // {{{2 Statement Interface
    virtual bool isNormal() const;
    // {{{2 SolutionCallback  interface
    virtual void printHead(std::ostream &out) const;
    virtual void report(Output::OutputBase &out);
    // {{{2 Printable interface
    virtual void print(std::ostream &out) const;
    // }}}2

    RuleType type;
};

// {{{1 declaration of WeakConstraint

struct WeakConstraint : AbstractStatement {
    WeakConstraint(UTermVec &&tuple, ULitVec &&body);
    virtual ~WeakConstraint();

    // {{{2 AbstractStatement interface
    virtual void collectImportant(Term::VarSet &vars);
    // {{{2 Printable interface
    virtual void print(std::ostream &out) const;
    // {{{2 SolutionCallback  interface
    virtual void report(Output::OutputBase &out);
    virtual void printHead(std::ostream &out) const;
    // }}}2

    UTermVec tuple;
};

// {{{1 declaration of ExternalRule

struct ExternalRule : Statement {
    ExternalRule();
    virtual ~ExternalRule();

    // {{{2 Statement Interface
    virtual bool isNormal() const;
    virtual void analyze(Dep::Node &node, Dep &dep);
    virtual void startLinearize(bool active);
    virtual void linearize(Scripts &scripts, bool positive);
    virtual void enqueue(Queue &q);
    // {{{2 Printable interface
    virtual void print(std::ostream &out) const;
    // }}}2

    HeadDefinition defines;
};

// }}}1

// {{{1 BodyAggregate

// {{{2 declaration of BodyAggregateAccumulate

using Output::BodyAggregateState;
struct BodyAggregateComplete;

struct BodyAggregateAccumulate : AbstractStatement {
    BodyAggregateAccumulate(BodyAggregateComplete &complete, UTermVec &&tuple, ULitVec &&lits, ULitVec &&auxLits);
    virtual ~BodyAggregateAccumulate();
    // {{{3 AbstractStatement interface
    void collectImportant(Term::VarSet &vars);
    // {{{3 Statement interface
    virtual bool isNormal() const { return true; }
    // {{{3 SolutionCallback interface
    void printHead(std::ostream &out) const;
    void report(Output::OutputBase &out);
    // }}}3
    BodyAggregateComplete &complete;
    UTermVec               tuple;
};

// {{{2 declaration of BodyAggregateComplete

struct BodyAggregateComplete : Statement, SolutionCallback, BodyOcc {
    using AccumulateDomainVec = std::vector<std::reference_wrapper<BodyAggregateAccumulate>>;
    using Domain              = AbstractDomain<BodyAggregateState>;
    using DomainElement       = Domain::element_type;
    using TodoVec             = std::vector<std::reference_wrapper<DomainElement>>;

    BodyAggregateComplete(UTerm &&repr, AggregateFunction fun, BoundVec &&bounds);
    // {{{3 Statement interface
    virtual bool isNormal() const;                           // return false -> this one creates choices
    virtual void analyze(Dep::Node &node, Dep &dep);         // use accuDoms to build the dependency...
    virtual void startLinearize(bool active);                // noop because single instantiator
    virtual void linearize(Scripts &scripts, bool positive); // noop because single instantiator
    virtual void enqueue(Queue &q);                          // enqueue the single instantiator
    // {{{3 SolutionCallback  interface
    virtual void printHead(std::ostream &out) const;         // #complete { h1, ..., hn }
    virtual void propagate(Queue &queue);                    // enqueue based on accuDoms
    virtual void report(Output::OutputBase &out);            // loop over headDom here
    // {{{3 Printable interface
    virtual void print(std::ostream &out) const;             // #complete { h1, ..., hn } :- accu1,... , accun.
    // {{{3 BodyOcc interface
    virtual UGTerm getRepr() const;
    virtual bool isPositive() const;
    virtual bool isNegative() const;
    virtual void setType(OccurrenceType x);
    virtual OccurrenceType getType() const;
    virtual DefinedBy &definedBy();
    virtual void checkDefined(LocSet &done, SigSet const &edb, UndefVec &undef) const;
    virtual ~BodyAggregateComplete();
    // }}}3

    AccumulateDomainVec accuDoms;
    Domain              domain;
    HeadDefinition      def;
    UTerm               accuRepr;
    AggregateFunction   fun;
    BoundVec            bounds;
    TodoVec             todo;
    OccurrenceType      occType      = OccurrenceType::STRATIFIED;
    DefinedBy           defBy;
    Instantiator        inst;

    bool                positive     = true; // what with this one ...
};

// {{{2 declaration of BodyAggregateLiteral

struct BodyAggregateLiteral : Literal, BodyOcc {
    using DomainElement = BodyAggregateComplete::DomainElement;

    // {{{3 (De)Constructors
    BodyAggregateLiteral(BodyAggregateComplete &complete, NAF naf);
    virtual ~BodyAggregateLiteral();
    // {{{3 Printable interface
    virtual void print(std::ostream &out) const;
    // {{{3 Literal interface
    virtual UIdx index(Scripts &scripts, BinderType type, Term::VarSet &bound);
    virtual bool isRecursive() const;
    virtual BodyOcc *occurrence();
    virtual void collect(VarTermBoundVec &vars) const;
    virtual Score score(Term::VarSet const &bound);
    virtual std::pair<Output::Literal*,bool> toOutput();
    // {{{3 BodyOcc interface
    virtual DefinedBy &definedBy();
    virtual UGTerm getRepr() const;
    virtual bool isPositive() const;
    virtual bool isNegative() const;
    virtual void setType(OccurrenceType x);
    virtual OccurrenceType getType() const;
    virtual void checkDefined(LocSet &done, SigSet const &edb, UndefVec &) const;
    // }}}3

    BodyAggregateComplete &complete;
    DefinedBy              defs;
    Output::BodyAggregate  gLit;
    OccurrenceType         type   = OccurrenceType::POSITIVELY_STRATIFIED;
};

// }}}2

// {{{1 AssignmentAggregate

// {{{2 declaration of AssignmentAggregateAccumulate

using Output::AssignmentAggregateState;
struct AssignmentAggregateComplete;

struct AssignmentAggregateAccumulate : AbstractStatement {
    AssignmentAggregateAccumulate(AssignmentAggregateComplete &complete, UTermVec &&tuple, ULitVec &&lits, ULitVec &&auxLits);
    virtual ~AssignmentAggregateAccumulate();
    // {{{3 AbstractStatement interface
    void collectImportant(Term::VarSet &vars);
    // {{{3 Statement interface
    virtual bool isNormal() const { return true; }
    // {{{3 SolutionCallback interface
    void printHead(std::ostream &out) const;
    void report(Output::OutputBase &out);
    // }}}3

    AssignmentAggregateComplete &complete;
    UTermVec               tuple;
};

// {{{2 declaration of AssignmentAggregateComplete

struct AssignmentAggregateComplete : Statement, SolutionCallback, BodyOcc {
    using AccumulateDomainVec = std::vector<std::reference_wrapper<AssignmentAggregateAccumulate>>;
    using StateDataMap        = std::unordered_map<Value, AssignmentAggregateState::Data>;
    using Domain              = AbstractDomain<AssignmentAggregateState>;
    using TodoVec             = std::vector<std::reference_wrapper<StateDataMap::value_type>>;
    using DomainElement       = Domain::element_type;

    AssignmentAggregateComplete(UTerm &&repr, UTerm &&dataRepr, AggregateFunction fun);
    // {{{3 Statement interface
    virtual bool isNormal() const;                           // return false -> this one creates choices
    virtual void analyze(Dep::Node &node, Dep &dep);         // use accuDoms to build the dependency...
    virtual void startLinearize(bool active);                // noop because single instantiator
    virtual void linearize(Scripts &scripts, bool positive); // noop because single instantiator
    virtual void enqueue(Queue &q);                          // enqueue the single instantiator
    // {{{3 SolutionCallback  interface
    virtual void printHead(std::ostream &out) const;         // #complete { h1, ..., hn }
    virtual void propagate(Queue &queue);                    // enqueue based on accuDoms
    virtual void report(Output::OutputBase &out);            // loop over headDom here
    // {{{3 Printable interface
    virtual void print(std::ostream &out) const;             // #complete { h1, ..., hn } :- accu1,... , accun.
    // {{{3 BodyOcc interface
    virtual UGTerm getRepr() const;
    virtual bool isPositive() const;
    virtual bool isNegative() const;
    virtual void setType(OccurrenceType x);
    virtual OccurrenceType getType() const;
    virtual DefinedBy &definedBy();
    virtual void checkDefined(LocSet &done, SigSet const &edb, UndefVec &undef) const;
    virtual ~AssignmentAggregateComplete();
    // }}}3

    void insert_(ValVec const &values, StateDataMap::value_type &x);

    AccumulateDomainVec accuDoms;
    StateDataMap        data;
    Domain              domain;
    HeadDefinition      def;
    UTerm               dataRepr;
    AggregateFunction   fun;
    TodoVec             todo;
    OccurrenceType      occType    = OccurrenceType::STRATIFIED;
    DefinedBy           defBy;
    Instantiator        inst;
};

// {{{2 declaration of AssignmentAggregateLiteral

struct AssignmentAggregateLiteral : Literal, BodyOcc {
    using DomainElement = AssignmentAggregateComplete::DomainElement;

    AssignmentAggregateLiteral(AssignmentAggregateComplete &complete);
    virtual ~AssignmentAggregateLiteral();
    // {{{3 Printable interface
    virtual void print(std::ostream &out) const;
    // {{{3 Literal interface
    virtual UIdx index(Scripts &scripts, BinderType type, Term::VarSet &bound);
    virtual bool isRecursive() const;
    virtual BodyOcc *occurrence();
    virtual void collect(VarTermBoundVec &vars) const;
    virtual Score score(Term::VarSet const &bound);
    virtual std::pair<Output::Literal*,bool> toOutput();
    // {{{3 BodyOcc interface
    virtual DefinedBy &definedBy();
    virtual UGTerm getRepr() const;
    virtual bool isPositive() const;
    virtual bool isNegative() const;
    virtual void setType(OccurrenceType x);
    virtual OccurrenceType getType() const;
    virtual void checkDefined(LocSet &done, SigSet const &edb, UndefVec &) const;
    // }}}3

    AssignmentAggregateComplete &complete;
    DefinedBy                   defs;
    Output::AssignmentAggregate gLit;
    OccurrenceType              type   = OccurrenceType::POSITIVELY_STRATIFIED;
};

// }}}2

// {{{1 Conjunction

// The following classes correspond to the rules indicated below.
// empty       :- body.        % to handle the empty case
// accu        :- empty, cond. % accumulation of conditon (blocks if condition is a fact and no head available yet)
// accu        :- empty, head. % accumulation of head (blocks if no condition available yet, unblocks if there is a head and the condition is fact)
// aggr        :- accu.        % complete rule (let's through unblocked conjunctions)
// head        :- aggr, body.  % literal

using Output::ConjunctionState;
struct ConjunctionComplete;

// {{{2 declaration of ConjunctionAccumulateEmpty

struct ConjunctionAccumulateEmpty : AbstractStatement {
    ConjunctionAccumulateEmpty(ConjunctionComplete &complete, ULitVec &&auxLits);
    virtual ~ConjunctionAccumulateEmpty();
    // {{{3 Statement interface
    virtual bool isNormal() const;
    // {{{3 SolutionCallback interface
    void report(Output::OutputBase &out);
    // }}}3

    ConjunctionComplete &complete;
};

// {{{2 declaration of ConjunctionAccumulateCond

struct ConjunctionAccumulateCond : AbstractStatement {
    ConjunctionAccumulateCond(ConjunctionComplete &complete, ULitVec &&lits);
    virtual ~ConjunctionAccumulateCond();
    // {{{3 Statement interface
    virtual bool isNormal() const;
    virtual void linearize(Scripts &scripts, bool positive);
    // {{{3 SolutionCallback  interface
    virtual void report(Output::OutputBase &out);
    // }}}3

    ConjunctionComplete &complete;
};

// {{{2 declaration of ConjunctionAccumulateHead

struct ConjunctionAccumulateHead : AbstractStatement {
    ConjunctionAccumulateHead(ConjunctionComplete &complete, ULitVec &&lits);
    virtual ~ConjunctionAccumulateHead();
    // {{{3 Statement interface
    virtual bool isNormal() const;
    // {{{3 SolutionCallback  interface
    virtual void report(Output::OutputBase &out);
    // }}}3

    ConjunctionComplete &complete;
};

// {{{2 declaration of ConjunctionComplete

struct ConjunctionComplete : Statement, SolutionCallback, BodyOcc {
    using Domain = AbstractDomain<ConjunctionState>;
    using DomainElement = Domain::element_type;
    using TodoVec = std::vector<std::reference_wrapper<DomainElement>>;

    ConjunctionComplete(UTerm &&repr, UTermVec &&local);
    UTerm condRepr() const;
    UTerm headRepr() const;
    UTerm accuRepr() const;
    UTerm emptyRepr() const;
    // {{{3 Statement interface
    virtual bool isNormal() const;
    virtual void analyze(Dep::Node &node, Dep &dep);
    virtual void startLinearize(bool active);
    virtual void linearize(Scripts &scripts, bool positive);
    virtual void enqueue(Queue &q);
    // {{{3 SolutionCallback  interfac
    virtual void printHead(std::ostream &out) const;
    virtual void propagate(Queue &queue);
    virtual void report(Output::OutputBase &out);
    virtual unsigned priority() const { return 1; }
    // {{{3 Printable interface
    virtual void print(std::ostream &out) const;
    // {{{3 BodyOcc interface
    virtual UGTerm getRepr() const;
    virtual bool isPositive() const;
    virtual bool isNegative() const;
    virtual void setType(OccurrenceType x);
    virtual OccurrenceType getType() const;
    virtual DefinedBy &definedBy();
    virtual void checkDefined(LocSet &done, SigSet const &edb, UndefVec &undef) const;
    // }}}3
    virtual ~ConjunctionComplete();

    Domain dom;         // condtional literal
    PredicateDomain domEmpty;
    PredicateDomain domCond;
    HeadDefinition def; // condtional literal
    OccurrenceType occType = OccurrenceType::STRATIFIED;
    DefinedBy defBy;
    Instantiator inst;  // dummy instantiator
    TodoVec todo;       // contains literals that might have to be exported
    UTermVec local;     // local variables in condition
    bool condRecursive = false;
};

// {{{2 declaration of ConjunctionLiteral

struct ConjunctionLiteral : Literal, BodyOcc {
    using DomainElement = ConjunctionComplete::DomainElement;

    ConjunctionLiteral(ConjunctionComplete &complete);
    virtual ~ConjunctionLiteral();
    // {{{3 Printable interface
    virtual void print(std::ostream &out) const;
    // {{{3 Literal interface
    virtual UIdx index(Scripts &scripts, BinderType type, Term::VarSet &bound);
    virtual bool isRecursive() const;
    virtual BodyOcc *occurrence();
    virtual void collect(VarTermBoundVec &vars) const;
    virtual Score score(Term::VarSet const &bound);
    virtual std::pair<Output::Literal*,bool> toOutput();
    // {{{3 BodyOcc interface
    virtual DefinedBy &definedBy();
    virtual UGTerm getRepr() const;
    virtual bool isPositive() const;
    virtual bool isNegative() const;
    virtual void setType(OccurrenceType x);
    virtual OccurrenceType getType() const;
    virtual void checkDefined(LocSet &done, SigSet const &edb, UndefVec &) const;
    // }}}3

    ConjunctionComplete &complete;
    DefinedBy            defs;
    Output::Conjunction  gLit;
    OccurrenceType       type   = OccurrenceType::POSITIVELY_STRATIFIED;
};
// }}}2

// {{{1 Disjoint

// {{{2 declaration of DisjointAccumulate

using Output::DisjointState;
struct DisjointComplete;

struct DisjointAccumulate : AbstractStatement {
    DisjointAccumulate(DisjointComplete &complete, ULitVec &&lits, ULitVec &&auxLits);
    DisjointAccumulate(DisjointComplete &complete, UTermVec &&tuple, CSPAddTerm &&value, ULitVec &&lits, ULitVec &&auxLits);
    virtual ~DisjointAccumulate();
    // {{{3 AbstractStatement interface
    void collectImportant(Term::VarSet &vars);
    // {{{3 Statement interface
    virtual bool isNormal() const { return true; }
    // {{{3 SolutionCallback interface
    void printHead(std::ostream &out) const;
    void report(Output::OutputBase &out);
    // }}}3

    DisjointComplete &complete;
    UTermVec          tuple;
    CSPAddTerm        value;
    bool              neutral   = true;
};

// {{{2 declaration of DisjointComplete

struct DisjointComplete : Statement, SolutionCallback, BodyOcc {
    using AccumulateDomainVec = std::vector<std::reference_wrapper<DisjointAccumulate>>;
    using Domain              = AbstractDomain<DisjointState>;
    using DomainElement       = Domain::element_type;
    using TodoVec             = std::vector<std::reference_wrapper<DomainElement>>;

    DisjointComplete(UTerm &&repr);
    // {{{3 Statement interface
    virtual bool isNormal() const;
    virtual void analyze(Dep::Node &node, Dep &dep);
    virtual void startLinearize(bool active);
    virtual void linearize(Scripts &scripts, bool positive);
    virtual void enqueue(Queue &q);
    // {{{3 SolutionCallback  interface
    virtual void printHead(std::ostream &out) const;         // #complete { h1, ..., hn }
    virtual void propagate(Queue &queue);                    // enqueue based on accuDoms
    virtual void report(Output::OutputBase &out);            // loop over headDom here
    // {{{3 Printable interface
    virtual void print(std::ostream &out) const;             // #complete { h1, ..., hn } :- accu1,... , accun.
    // {{{3 BodyOcc interface
    virtual UGTerm getRepr() const;
    virtual bool isPositive() const;
    virtual bool isNegative() const;
    virtual void setType(OccurrenceType x);
    virtual OccurrenceType getType() const;
    virtual DefinedBy &definedBy();
    virtual void checkDefined(LocSet &done, SigSet const &edb, UndefVec &undef) const;
    virtual ~DisjointComplete();
    // }}}3

    AccumulateDomainVec accuDoms;
    Domain              domain;
    HeadDefinition      def;
    UTerm               accuRepr;
    TodoVec             todo;
    OccurrenceType      occType = OccurrenceType::STRATIFIED;
    DefinedBy           defBy;
    Instantiator        inst;
};

// {{{2 declaration of DisjointLiteral

struct DisjointLiteral : Literal, BodyOcc {
    using DomainElement = DisjointComplete::DomainElement;

    DisjointLiteral(DisjointComplete &complete, NAF naf);
    virtual ~DisjointLiteral();
    // {{{3 Printable interface
    virtual void print(std::ostream &out) const;
    // {{{3 Literal interface
    virtual UIdx index(Scripts &scripts, BinderType type, Term::VarSet &bound);
    virtual bool isRecursive() const;
    virtual BodyOcc *occurrence();
    virtual void collect(VarTermBoundVec &vars) const;
    virtual Score score(Term::VarSet const &bound);
    virtual std::pair<Output::Literal*,bool> toOutput();
    // {{{3 BodyOcc interface
    virtual DefinedBy &definedBy();
    virtual UGTerm getRepr() const;
    virtual bool isPositive() const;
    virtual bool isNegative() const;
    virtual void setType(OccurrenceType x);
    virtual OccurrenceType getType() const;
    virtual void checkDefined(LocSet &done, SigSet const &edb, UndefVec &) const;
    // }}}3

    DisjointComplete        &complete;
    DefinedBy               defs;
    Output::DisjointLiteral gLit;
    OccurrenceType          type   = OccurrenceType::POSITIVELY_STRATIFIED;
};

// }}}2

// }}}1

// {{{1 HeadAggregate

// {{{2 declaration of HeadAggregateRule

using Output::HeadAggregateState;

struct HeadAggregateRule : AbstractStatement {
    using Domain        = AbstractDomain<HeadAggregateState>;
    using DomainElement = Domain::element_type;
    using TodoVec       = std::vector<std::reference_wrapper<HeadAggregateState>>;
    // {{{3 (De)Constructors
    HeadAggregateRule(UTerm &&repr, AggregateFunction fun, BoundVec &&bounds, ULitVec &&lits);
    virtual ~HeadAggregateRule();
    // {{{3 Printable interface
    void print(std::ostream &out) const;
    // {{{3 SolutionCallback interface
    void report(Output::OutputBase &out);
    // }}}3
    
    Domain              domain;
    AggregateFunction   fun;
    BoundVec            bounds;
    TodoVec             todo;
};

// {{{2 declaration of HeadAggregateLiteral

struct HeadAggregateLiteral : Literal, BodyOcc {
    using DomainElement = HeadAggregateRule::DomainElement;
    // {{{3 (De)Constructors
    HeadAggregateLiteral(HeadAggregateRule &headRule);
    virtual ~HeadAggregateLiteral();
    // {{{3 Printable interface
    virtual void print(std::ostream &out) const;
    // {{{3 Literal interface
    virtual UIdx index(Scripts &scripts, BinderType type, Term::VarSet &bound);
    virtual bool isRecursive() const;
    virtual BodyOcc *occurrence();
    virtual void collect(VarTermBoundVec &vars) const;
    virtual Score score(Term::VarSet const &bound);
    virtual std::pair<Output::Literal*,bool> toOutput();
    // {{{3 BodyOcc interface
    virtual DefinedBy &definedBy();
    virtual UGTerm getRepr() const;
    virtual bool isPositive() const;
    virtual bool isNegative() const;
    virtual void setType(OccurrenceType x);
    virtual OccurrenceType getType() const;
    virtual void checkDefined(LocSet &done, SigSet const &edb, UndefVec &) const;
    // }}}3

    HeadAggregateRule   &headRule;
    DefinedBy            defs;
    DomainElement       *gResult = nullptr;
    OccurrenceType       type = OccurrenceType::POSITIVELY_STRATIFIED;
};

// {{{2 declaration of HeadAggregateAccumulate

struct HeadAggregateAccumulate : AbstractStatement {
    HeadAggregateAccumulate(HeadAggregateRule &headRule, unsigned elemIndex, UTermVec &&tuple, PredicateDomain *predDom, UTerm &&predRepr, ULitVec &&lits);
    virtual ~HeadAggregateAccumulate();
    // {{{3 AbstractStatement interface
    void collectImportant(Term::VarSet &vars);
    // {{{3 SolutionCallback interface
    void printHead(std::ostream &out) const;
    void report(Output::OutputBase &out);
    // }}}3

    UHeadDef           predDef;
    HeadAggregateRule &headRule;
    unsigned           elemIndex;
    UTermVec           tuple;
};

// {{{2 declaration of HeadAggregateComplete

struct HeadAggregateComplete : Statement, SolutionCallback, BodyOcc {
    using AccumulateDomainVec = std::vector<std::reference_wrapper<HeadAggregateAccumulate>>;

    HeadAggregateComplete(HeadAggregateRule &headRule);
    // {{{3 Statement interface
    virtual bool isNormal() const;                           // return false -> this one creates choices
    virtual void analyze(Dep::Node &node, Dep &dep);         // use accuDoms to build the dependency...
    virtual void startLinearize(bool active);                // noop because single instantiator
    virtual void linearize(Scripts &scripts, bool positive); // noop because single instantiator
    virtual void enqueue(Queue &q);                          // enqueue the single instantiator
    // {{{3 SolutionCallback  interface
    virtual void printHead(std::ostream &out) const;         // #complete { h1, ..., hn }
    virtual void propagate(Queue &queue);                    // enqueue based on accuDoms
    virtual void report(Output::OutputBase &out);            // loop over headDom here
    // {{{3 Printable interface
    virtual void print(std::ostream &out) const;             // #complete { h1, ..., hn } :- accu1,... , accun.
    // {{{3 BodyOcc interface
    virtual UGTerm getRepr() const;
    virtual bool isPositive() const;
    virtual bool isNegative() const;
    virtual void setType(OccurrenceType x);
    virtual OccurrenceType getType() const;
    virtual DefinedBy &definedBy();
    virtual void checkDefined(LocSet &done, SigSet const &edb, UndefVec &undef) const;
    virtual ~HeadAggregateComplete();
    // }}}3

    HeadAggregateRule   &headRule;
    UTerm                completeRepr;
    AccumulateDomainVec  accuDoms;
    Instantiator         inst;
    OccurrenceType       occType = OccurrenceType::STRATIFIED;
    DefinedBy            defBy;
};

// }}}2

// {{{1 Disjunction

using Output::DisjunctionState;

// {{{2 declaration of DisjunctionComplete

struct DisjunctionComplete : Statement, SolutionCallback, BodyOcc {
    using Domain = std::unordered_map<Value, DisjunctionState>;
    using TodoVec = std::vector<std::reference_wrapper<DisjunctionState>>;
    using TodoHeadVec = std::vector<std::tuple<int, Gringo::Output::DisjunctionElement &, Value, Output::ULitVec>>;
    using HeadVec = std::vector<HeadDefinition>;
    using LocalVec = std::vector<UTermVec>;

    DisjunctionComplete(UTerm &&repr);
    UTerm condRepr(unsigned elemIndex) const;
    UTerm headRepr(unsigned elemIndex, int headIndex) const;
    UTerm accuRepr() const;
    UTerm emptyRepr() const;
    void appendLocal(UTermVec &&local);
    void appendHead(PredicateDomain &headDom, UTerm &&headRepr);
    // {{{3 Statement interface
    virtual bool isNormal() const;                           // return false -> this one creates choices
    virtual void analyze(Dep::Node &node, Dep &dep);         // use accuDoms to build the dependency...
    virtual void startLinearize(bool active);                // noop because single instantiator
    virtual void linearize(Scripts &scripts, bool positive); // noop because single instantiator
    virtual void enqueue(Queue &q);                          // enqueue the single instantiator
    // {{{3 SolutionCallback  interface
    virtual void printHead(std::ostream &out) const;         // #complete { h1, ..., hn }
    virtual void propagate(Queue &queue);                    // enqueue based on accuDoms
    virtual void report(Output::OutputBase &out);            // loop over headDom here
    virtual unsigned priority() const { return 1; }
    // {{{3 Printable interface
    virtual void print(std::ostream &out) const;             // #complete { h1, ..., hn } :- accu1,... , accun.
    // {{{3 BodyOcc interface
    virtual UGTerm getRepr() const;
    virtual bool isPositive() const;
    virtual bool isNegative() const;
    virtual void setType(OccurrenceType x);
    virtual OccurrenceType getType() const;
    virtual DefinedBy &definedBy();
    virtual void checkDefined(LocSet &done, SigSet const &edb, UndefVec &undef) const;
    // }}}3
    virtual ~DisjunctionComplete();

    Domain dom;
    UTerm repr;
    PredicateDomain domEmpty;
    PredicateDomain domCond;
    OccurrenceType occType = OccurrenceType::STRATIFIED;
    DefinedBy defBy;
    Instantiator inst;  // dummy instantiator
    TodoVec todo;       // contains literals that might have to be exported
    TodoHeadVec todoHead;
    LocalVec locals;
    HeadVec heads;
};

// {{{2 declaration of DisjunctionRule

struct DisjunctionRule : AbstractStatement {
    DisjunctionRule(DisjunctionComplete &complete, ULitVec &&lits);
    virtual ~DisjunctionRule();
    // {{{3 Statement interface
    virtual bool isNormal() const;
    // {{{3 SolutionCallback interface
    void report(Output::OutputBase &out);
    // }}}3

    DisjunctionComplete &complete;
};

// {{{2 declaration of DisjunctionAccumulateCond

struct DisjunctionAccumulateCond : AbstractStatement {
    DisjunctionAccumulateCond(DisjunctionComplete &complete, unsigned elemIndex, ULitVec &&lits);
    virtual ~DisjunctionAccumulateCond(); 
    // {{{3 SolutionCallback interface
    void report(Output::OutputBase &out);
    // }}}3

    DisjunctionComplete &complete;
};

// {{{2 declaration of DisjunctionAccumulateHead

struct DisjunctionAccumulateHead : AbstractStatement {
    DisjunctionAccumulateHead(DisjunctionComplete &complete, unsigned elemIndex, int headIndex, ULitVec &&lits);
    virtual ~DisjunctionAccumulateHead(); 
    // {{{3 SolutionCallback interface
    void report(Output::OutputBase &out);
    // }}}3

    DisjunctionComplete &complete;
    int headIndex;
};

// }}}2

/*
// {{{2 declaration of DisjunctionLiteral

struct DisjunctionLiteral : Literal, BodyOcc {
    using DomainElement = DisjunctionComplete::DomainElement;

    // {{{3 (De)Constructors
    DisjunctionLiteral(DisjunctionComplete &complete);
    virtual ~DisjunctionLiteral();
    // {{{3 Printable interface
    virtual void print(std::ostream &out) const;
    // {{{3 Literal interface
    virtual UIdx index(Scripts &scripts, BinderType type, Term::VarSet &bound);
    virtual bool isRecursive() const;
    virtual BodyOcc *occurrence();
    virtual void collect(VarTermBoundVec &vars) const;
    virtual Score score(Term::VarSet const &bound);
    virtual Output::Literal *toOutput();
    // {{{3 BodyOcc interface
    virtual DefinedBy &definedBy();
    virtual UGTerm getRepr() const;
    virtual bool isPositive() const;
    virtual bool isNegative() const;
    virtual void setType(OccurrenceType x);
    virtual OccurrenceType getType() const;
    virtual void checkDefined(LocSet &done, SigSet const &edb, UndefVec &) const;
    // }}}3

    ConjunctionComplete &complete;
    DisjunctionRule     &headRule;
    DefinedBy            defs;
    DomainElement       *gResult = nullptr;
    OccurrenceType       type = OccurrenceType::POSITIVELY_STRATIFIED;
};

// }}}2
*/

// }}}1

} } // namespace Ground Gringo

#endif // _GRINGO_GROUND_STATEMENTS_HH
