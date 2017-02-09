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

#ifndef _GRINGO_GROUND_STATEMENTS_HH
#define _GRINGO_GROUND_STATEMENTS_HH

#include <gringo/ground/statement.hh>
#include <gringo/ground/literals.hh>
#include <gringo/output/statements.hh>
#include <gringo/intervals.hh>
#include <gringo/output/theory.hh>

namespace Gringo { namespace Ground {

// {{{1 declaration of HeadDefinition

class HeadDefinition : private HeadOccurrence {
public:
    using OffsetMap = std::unordered_map<IndexUpdater*, unsigned>;
    using RInstVec = std::vector<std::reference_wrapper<Instantiator>>;
    using EnqueueVec = std::vector<std::pair<IndexUpdater*, RInstVec>>;

    HeadDefinition(UTerm &&repr, Domain *domain);
    HeadDefinition(HeadDefinition &&) = default;
    virtual ~HeadDefinition() noexcept;
    void collectImportant(Term::VarSet &vars);
    void enqueue(Queue &queue);
    operator bool () const { return static_cast<bool>(repr_); }
    Domain &dom() { assert(domain_); return *domain_; }
    UTerm const &domRepr() const { return repr_; }
    void init() {
        if (domain_) { domain_->init(); }
    }
    void setActive(bool active) { active_ = active; }
    void analyze(Statement::Dep::Node &node, Statement::Dep &dep) {
        if (repr_) {
            dep.provides(node, *this, repr_->gterm());
        }
    }
private:
    // {{{2 HeadOccurrence interface
    virtual void defines(IndexUpdater &update, Instantiator *inst) override;
    /// }}}2

private:
    UTerm repr_;
    Domain *domain_;
    OffsetMap offsets_;
    EnqueueVec enqueueVec_;
    bool active_ = false;
};

// {{{1 declaration of AbstractStatement

class AbstractStatement : public Statement, protected SolutionCallback {
public:
    AbstractStatement(UTerm &&repr, Domain *domain, ULitVec &&lits);
    virtual ~AbstractStatement() noexcept;
    // This function returns true if all non-auxiliary literals are non-recursive.
    // The accumulation rule can use this function to mark complete rules as output recursive.
    // Note that only the complete rule should set the recursive flag to false.
    // Until that point all atoms should be marked as recursive to correctly capture negative recursion.
    // This has to be done this way because with negative recursion,
    // the complete rule might appear in a later component;
    // the complete rule is guaranteed to appear after or together with an accumulation statemnt
    // because of its positive dependency to them.
    // TODO: This function should only consider positive recursion.
    //       To do this, the literal interface has to be extended.
    virtual bool isOutputRecursive() const;
    virtual void collectImportant(Term::VarSet &vars);
    virtual void printBody(std::ostream &out) const;
    // {{{2 Statement interface
    bool isNormal() const override; // false by default
    void analyze(Dep::Node &node, Dep &dep) override;
    void startLinearize(bool active) override;
    void linearize(Context &context, bool positive, Logger &log) override;
    void enqueue(Queue &q) override;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // }}}2
protected:
    // {{{2 SolutionCallback interface
    void printHead(std::ostream &out) const override;
    void propagate(Queue &queue) override;
    // }}}2

protected:
    HeadDefinition def_;
    ULitVec lits_;
    InstVec insts_;
};

// }}}1

// Statements

// {{{1 declaration of Rule

enum class RuleType : unsigned short { External, Disjunctive, Choice };

class Rule : public Statement, protected SolutionCallback {
public:
    using HeadVec = std::vector<std::pair<UTerm, Domain*>>;
    using HeadDefVec = std::vector<HeadDefinition>;
    Rule(HeadVec &&heads, ULitVec &&lits, RuleType type_);
    virtual ~Rule() noexcept;
    // {{{2 Statement interface
    bool isNormal() const override;
    void analyze(Dep::Node &node, Dep &dep) override;
    void startLinearize(bool active) override;
    void linearize(Context &context, bool positive, Logger &log) override;
    void enqueue(Queue &q) override;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // }}}2
protected:
    // {{{2 SolutionCallback interface
    void report(Output::OutputBase &out, Logger &log) override;
    void printHead(std::ostream &out) const override;
    void propagate(Queue &queue) override;
    // }}}2

protected:
    HeadDefVec defs_;
    ULitVec lits_;
    InstVec insts_;
    RuleType type_;
};

// {{{1 declaration of ExternalRule

class ExternalRule : public Statement {
public:
    ExternalRule();
    virtual ~ExternalRule() noexcept;

    // {{{2 Statement Interface
    virtual bool isNormal() const;
    virtual void analyze(Dep::Node &node, Dep &dep);
    virtual void startLinearize(bool active);
    virtual void linearize(Context &context, bool positive, Logger &log);
    virtual void enqueue(Queue &q);
    // {{{2 Printable interface
    virtual void print(std::ostream &out) const;
    // }}}2

private:
    HeadDefinition def_;
};

// }}}1

// {{{1 declaration of ShowStatement

class ShowStatement : public AbstractStatement {
public:
    ShowStatement(UTerm &&term, bool csp, ULitVec &&body);
    virtual ~ShowStatement() noexcept;

    // {{{2 AbstractStatement interface
    void collectImportant(Term::VarSet &vars) override;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // }}}2
private:
    // {{{2 SolutionCallback interface
    void report(Output::OutputBase &out, Logger &log) override;
    void printHead(std::ostream &out) const override;
    // }}}2

private:
    UTerm term_;
    bool csp_;
};

// {{{1 declaration of EdgeStatement

class EdgeStatement : public AbstractStatement {
public:
    EdgeStatement(UTerm &&u, UTerm &&v, ULitVec &&body);
    virtual ~EdgeStatement() noexcept;

    // {{{2 AbstractStatement interface
    void collectImportant(Term::VarSet &vars) override;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // }}}2
private:
    // {{{2 SolutionCallback interface
    void report(Output::OutputBase &out, Logger &log) override;
    void printHead(std::ostream &out) const override;
    // }}}2

private:
    UTerm u_;
    UTerm v_;
};

// {{{1 declaration of ProjectStatement

class ProjectStatement : public AbstractStatement {
public:
    ProjectStatement(UTerm &&atom, ULitVec &&body);
    virtual ~ProjectStatement() noexcept;

    // {{{2 AbstractStatement interface
    void collectImportant(Term::VarSet &vars) override;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // }}}2
private:
    // {{{2 SolutionCallback interface
    void report(Output::OutputBase &out, Logger &log) override;
    void printHead(std::ostream &out) const override;
    // }}}2

private:
    UTerm atom_;
};

// {{{1 declaration of HeuristicStatement

class HeuristicStatement : public AbstractStatement {
public:
    HeuristicStatement(UTerm &&atom, UTerm &&value, UTerm &&bias, UTerm &&mod, ULitVec &&body);
    virtual ~HeuristicStatement() noexcept;

    // {{{2 AbstractStatement interface
    void collectImportant(Term::VarSet &vars) override;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // }}}2
private:
    // {{{2 SolutionCallback interface
    void report(Output::OutputBase &out, Logger &log) override;
    void printHead(std::ostream &out) const override;
    // }}}2

private:
    UTerm atom_;
    UTerm value_;
    UTerm priority_;
    UTerm mod_;
};

// {{{1 declaration of WeakConstraint

class WeakConstraint : public AbstractStatement {
public:
    WeakConstraint(UTermVec &&tuple, ULitVec &&body);
    virtual ~WeakConstraint() noexcept;

    // {{{2 AbstractStatement interface
    void collectImportant(Term::VarSet &vars) override;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // }}}2
private:
    // {{{2 SolutionCallback interface
    void report(Output::OutputBase &out, Logger &log) override;
    void printHead(std::ostream &out) const override;
    // }}}2

private:
    UTermVec tuple_;
};

// }}}1

// Body Aggregates

// {{{1 declaration of BodyAggregateAccumulate

using Output::BodyAggregateAtom;
using Output::BodyAggregateDomain;
class BodyAggregateComplete;

class BodyAggregateAccumulate : public AbstractStatement {
public:
    BodyAggregateAccumulate(BodyAggregateComplete &complete, UTermVec &&tuple, ULitVec &&lits);
    virtual ~BodyAggregateAccumulate() noexcept;
    void printHead(std::ostream &out) const override;

    // {{{2 AbstractStatement interface
    void collectImportant(Term::VarSet &vars) override;
    // {{{3 Statement interface
    bool isNormal() const override { return true; }
    void linearize(Context &context, bool positive, Logger &log) override;
    // }}}2

private:
    // {{{2 SolutionCallback interface (w/o printHead)
    void report(Output::OutputBase &out, Logger &log) override;
    // }}}2

private:
    BodyAggregateComplete &complete_;
    UTermVec tuple_;
};

// {{{1 declaration of BodyAggregateComplete

class BodyAggregateComplete : public Statement, private SolutionCallback, private BodyOcc {
public:
    using AccumulateDomainVec = std::vector<std::reference_wrapper<BodyAggregateAccumulate>>;
    using TodoVec             = std::vector<Id_t>;

    BodyAggregateComplete(DomainData &data, UTerm &&repr, AggregateFunction fun, BoundVec &&bounds);
    virtual ~BodyAggregateComplete() noexcept;
    BodyAggregateDomain &dom() { return static_cast<BodyAggregateDomain&>(def_.dom()); }
    void enqueue(BodyAggregateDomain::Iterator atom);
    UTerm const &accuRepr() const { return accuRepr_; }
    UTerm const &domRepr() const { return def_.domRepr(); }
    AggregateFunction fun() const { return fun_; }
    void addAccuDom(BodyAggregateAccumulate &accu) { accuDoms_.emplace_back(accu); }
    BoundVec const &bounds() const { return bounds_; }
    bool monotone() const { return monotone_; }
    void setOutputRecursive() { outputRecursive_ = true; }

    // {{{2 Statement interface
    bool isNormal() const override;                           // return false -> this one creates choices
    void analyze(Dep::Node &node, Dep &dep) override;         // use accuDoms to build the dependency...
    void startLinearize(bool active) override;                // noop because single instantiator
    void linearize(Context &context, bool positive, Logger &log) override; // noop because single instantiator
    void enqueue(Queue &q) override;                          // enqueue the single instantiator
    // {{{2 Printable interface
    void print(std::ostream &out) const override;             // #complete { h1, ..., hn } :- accu1,... , accun.
    // }}}2

private:
    // {{{2 SolutionCallback interface
    void printHead(std::ostream &out) const override;         // #complete { h1, ..., hn }
    void propagate(Queue &queue) override;                    // enqueue based on accuDoms
    void report(Output::OutputBase &out, Logger &log) override;            // loop over headDom here
    // {{{2 BodyOcc interface
    UGTerm getRepr() const override;
    bool isPositive() const override;
    bool isNegative() const override;
    void setType(OccurrenceType x) override;
    OccurrenceType getType() const override;
    DefinedBy &definedBy() override;
    void checkDefined(LocSet &done, SigSet const &edb, UndefVec &undef) const override;
    // }}}2

private:
    AccumulateDomainVec accuDoms_;
    HeadDefinition def_;
    UTerm accuRepr_;
    AggregateFunction fun_;
    BoundVec bounds_;
    TodoVec todo_;
    OccurrenceType occType_ = OccurrenceType::STRATIFIED;
    DefinedBy defBy_;
    Instantiator inst_;
    bool monotone_ = true;
    bool outputRecursive_ = false;
};

// {{{1 declaration of BodyAggregateLiteral

class BodyAggregateLiteral : public Literal, private BodyOcc {
public:
    BodyAggregateLiteral(BodyAggregateComplete &complete, NAF naf, bool auxiliary);
    virtual ~BodyAggregateLiteral() noexcept;

    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // {{{2 Literal interface
    UIdx index(Context &context, BinderType type, Term::VarSet &bound) override;
    bool isRecursive() const override;
    BodyOcc *occurrence() override;
    void collect(VarTermBoundVec &vars) const override;
    Score score(Term::VarSet const &bound, Logger &log) override;
    std::pair<Output::LiteralId, bool> toOutput(Logger &log) override;
    bool auxiliary() const override { return auxiliary_; }
    // }}}2

private:
    // {{{2 BodyOcc interface
    DefinedBy &definedBy() override;
    UGTerm getRepr() const override;
    bool isPositive() const override;
    bool isNegative() const override;
    void setType(OccurrenceType x) override;
    OccurrenceType getType() const override;
    void checkDefined(LocSet &done, SigSet const &edb, UndefVec &) const override;
    // }}}2

private:
    BodyAggregateComplete &complete_;
    DefinedBy defs_;
    Potassco::Id_t offset_ = 0;
    NAF naf_;
    bool auxiliary_;
    OccurrenceType type_ = OccurrenceType::POSITIVELY_STRATIFIED;
};

// }}}1

// {{{1 declaration of AssignmentAggregateAccumulate

using Output::AssignmentAggregateAtom;
using Output::AssignmentAggregateData;
using Output::AssignmentAggregateDomain;
class AssignmentAggregateComplete;

class AssignmentAggregateAccumulate : public AbstractStatement {
public:
    AssignmentAggregateAccumulate(AssignmentAggregateComplete &complete, UTermVec &&tuple, ULitVec &&lits);
    virtual ~AssignmentAggregateAccumulate() noexcept;
    void printHead(std::ostream &out) const override;

    // {{{2 AbstractStatement interface
    void collectImportant(Term::VarSet &vars) override;
    // {{{2 Statement interface
    virtual bool isNormal() const override { return true; }
    void linearize(Context &context, bool positive, Logger &log) override;
    // }}}2

private:
    // {{{2 SolutionCallback interface (w/o printHead)
    void report(Output::OutputBase &out, Logger &log) override;
    // }}}2

private:
    AssignmentAggregateComplete &complete_;
    UTermVec tuple_;
};

// {{{1 declaration of AssignmentAggregateComplete

class AssignmentAggregateComplete : public Statement, private SolutionCallback, private BodyOcc {
public:
    using AccumulateDomainVec = std::vector<std::reference_wrapper<AssignmentAggregateAccumulate>>;
    using TodoVec             = std::vector<Id_t>;

    AssignmentAggregateComplete(DomainData &data, UTerm &&repr, UTerm &&dataRepr, AggregateFunction fun);
    AssignmentAggregateDomain &dom() { return static_cast<AssignmentAggregateDomain&>(def_.dom()); }
    virtual ~AssignmentAggregateComplete() noexcept;
    UTerm const &dataRepr() const { return dataRepr_; }
    UTerm const &domRepr() const { return def_.domRepr(); }
    AggregateFunction fun() const { return fun_; }
    void enqueue(Id_t dataId);
    void addAccuDom(AssignmentAggregateAccumulate &accu) { accuDoms_.emplace_back(accu); }
    void setOutputRecursive() { outputRecursive_ = true; }
    // {{{2 Statement interface
    bool isNormal() const override;                           // return false -> this one creates choices
    void analyze(Dep::Node &node, Dep &dep) override;         // use accuDoms to build the dependency...
    void startLinearize(bool active) override;                // noop because single instantiator
    void linearize(Context &context, bool positive, Logger &log) override; // noop because single instantiator
    void enqueue(Queue &q) override;                          // enqueue the single instantiator
    // {{{2 Printable interface
    void print(std::ostream &out) const override;             // #complete { h1, ..., hn } :- accu1,... , accun.
    // }}}2

private:
    // {{{2 SolutionCallback interface
    void printHead(std::ostream &out) const override;         // #complete { h1, ..., hn }
    void propagate(Queue &queue) override;                    // enqueue based on accuDoms
    void report(Output::OutputBase &out, Logger &log) override;            // loop over headDom here
    // {{{2 BodyOcc interface
    UGTerm getRepr() const override;
    bool isPositive() const override;
    bool isNegative() const override;
    void setType(OccurrenceType x) override;
    OccurrenceType getType() const override;
    DefinedBy &definedBy() override;
    void checkDefined(LocSet &done, SigSet const &edb, UndefVec &undef) const override;
    // }}}2

private:
    AccumulateDomainVec accuDoms_;
    HeadDefinition def_;
    UTerm dataRepr_;
    AggregateFunction fun_;
    TodoVec todo_;
    DefinedBy defBy_;
    Instantiator inst_;
    OccurrenceType occType_ = OccurrenceType::STRATIFIED;
    bool outputRecursive_ = false;
};

// {{{1 declaration of AssignmentAggregateLiteral

class AssignmentAggregateLiteral : public Literal, private BodyOcc {
public:
    AssignmentAggregateLiteral(AssignmentAggregateComplete &complete, bool auxiliary);
    virtual ~AssignmentAggregateLiteral() noexcept;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // {{{2 Literal interface
    UIdx index(Context &context, BinderType type, Term::VarSet &bound) override;
    bool isRecursive() const override;
    BodyOcc *occurrence() override;
    void collect(VarTermBoundVec &vars) const override;
    Score score(Term::VarSet const &bound, Logger &log) override;
    std::pair<Output::LiteralId,bool> toOutput(Logger &log) override;
    bool auxiliary() const override { return auxiliary_; }
    // }}}2

private:
    // {{{2 BodyOcc interface
    DefinedBy &definedBy() override;
    UGTerm getRepr() const override;
    bool isPositive() const override;
    bool isNegative() const override;
    void setType(OccurrenceType x) override;
    OccurrenceType getType() const override;
    void checkDefined(LocSet &done, SigSet const &edb, UndefVec &) const override;
    // }}}2

private:
    AssignmentAggregateComplete &complete_;
    DefinedBy defs_;
    Id_t offset_ = InvalidId;
    OccurrenceType type_ = OccurrenceType::POSITIVELY_STRATIFIED;
    bool auxiliary_;
};

// }}}1

// {{{1 declaration of ConjunctionAccumulateEmpty

// The following classes correspond to the rules indicated below.
// empty :- body.              % to handle the empty case
// accu  :- empty, cond.       % accumulation of conditon (blocks if condition is a fact and no head available yet)
// accu  :- empty, cond, head. % accumulation of head (blocks if no condition available yet, unblocks if there is a head and the condition is fact)
// aggr  :- accu.              % complete rule (let's through unblocked conjunctions)
// head  :- aggr, body.        % literal

using Output::ConjunctionAtom;
using Output::ConjunctionDomain;
class ConjunctionComplete;

class ConjunctionAccumulateEmpty : public AbstractStatement {
public:
    ConjunctionAccumulateEmpty(ConjunctionComplete &complete, ULitVec &&lits);
    virtual ~ConjunctionAccumulateEmpty() noexcept;
    // {{{2 Statement interface
    bool isNormal() const override;
    // }}}2

private:
    // {{{2 SolutionCallback interface
    void report(Output::OutputBase &out, Logger &log) override;
    // }}}2

private:
    ConjunctionComplete &complete_;
};

// {{{1 declaration of ConjunctionAccumulateCond

class ConjunctionAccumulateCond : public AbstractStatement {
public:
    ConjunctionAccumulateCond(ConjunctionComplete &complete, ULitVec &&lits);
    virtual ~ConjunctionAccumulateCond() noexcept;
    // {{{2 Statement interface
    bool isNormal() const override;
    void linearize(Context &context, bool positive, Logger &log) override;
    // }}}2
private:
    // {{{2 SolutionCallback interface
    void report(Output::OutputBase &out, Logger &log) override;
    // }}}2

private:
    ConjunctionComplete &complete_;
};

// {{{1 declaration of ConjunctionAccumulateHead

class ConjunctionAccumulateHead : public AbstractStatement {
public:
    ConjunctionAccumulateHead(ConjunctionComplete &complete, ULitVec &&lits);
    virtual ~ConjunctionAccumulateHead() noexcept;
    // {{{2 Statement interface
    bool isNormal() const override;
    void linearize(Context &context, bool positive, Logger &log) override;
    // }}}2

private:
    // {{{2 SolutionCallback interface
    void report(Output::OutputBase &out, Logger &log) override;
    // }}}2

private:
    ConjunctionComplete &complete_;
};

// {{{1 declaration of ConjunctionComplete

class ConjunctionComplete : public Statement, private SolutionCallback, private BodyOcc {
public:
    using TodoVec = std::vector<Id_t>;
    ConjunctionComplete(DomainData &data, UTerm &&repr, UTermVec &&local);
    virtual ~ConjunctionComplete() noexcept;
    UTerm condRepr() const;
    UTerm headRepr() const;
    UTerm accuRepr() const;
    UTerm emptyRepr() const;
    UTerm const &domRepr() const { return def_.domRepr(); }
    ConjunctionDomain &dom() { return static_cast<ConjunctionDomain&>(def_.dom()); }
    PredicateDomain &emptyDom() { return domEmpty_; }
    PredicateDomain &condDom() { return domCond_; }
    void reportEmpty(Logger &log);
    void reportCond(DomainData &data, Symbol cond, Output::LitVec &lits, Logger &log);
    void reportHead(DomainData &data, Symbol cond, Output::LitVec &lits, Logger &log);
    void setCondRecursive() { condRecursive_ = true; }
    void setHeadRecursive() { headRecursive_ = true; }
    // {{{2 Statement interface
    bool isNormal() const override;
    void analyze(Dep::Node &node, Dep &dep) override;
    void startLinearize(bool active) override;
    void linearize(Context &context, bool positive, Logger &log) override;
    void enqueue(Queue &q) override;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // }}}2

private:
    // {{{2 SolutionCallback interface
    void printHead(std::ostream &out) const override;
    void propagate(Queue &queue) override;
    void report(Output::OutputBase &out, Logger &log) override;
    unsigned priority() const override { return 1; }
    // {{{2 BodyOcc interface
    UGTerm getRepr() const override;
    bool isPositive() const override;
    bool isNegative() const override;
    void setType(OccurrenceType x) override;
    OccurrenceType getType() const override;
    DefinedBy &definedBy() override;
    void checkDefined(LocSet &done, SigSet const &edb, UndefVec &undef) const override;
    // }}}2
    template <class F>
    void reportOther(F f, Logger &log);

private:
    HeadDefinition def_; // condtional literal
    PredicateDomain domEmpty_;
    PredicateDomain domCond_;
    OccurrenceType occType_ = OccurrenceType::STRATIFIED;
    DefinedBy defBy_;
    Instantiator inst_;  // dummy instantiator
    TodoVec todo_;       // contains literals that might have to be exported
    UTermVec local_;     // local variables in condition
    bool condRecursive_ = false;
    bool headRecursive_ = false;
};

// {{{1 declaration of ConjunctionLiteral

class ConjunctionLiteral : public Literal, private BodyOcc {
public:
    ConjunctionLiteral(ConjunctionComplete &complete, bool auxiliary);
    virtual ~ConjunctionLiteral() noexcept;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // {{{2 Literal interface
    UIdx index(Context &context, BinderType type, Term::VarSet &bound) override;
    bool isRecursive() const override;
    BodyOcc *occurrence() override;
    void collect(VarTermBoundVec &vars) const override;
    Score score(Term::VarSet const &bound, Logger &log) override;
    std::pair<Output::LiteralId,bool> toOutput(Logger &log) override;
    bool auxiliary() const override { return auxiliary_; }
    // }}}2

private:
    // {{{2 BodyOcc interface
    DefinedBy &definedBy() override;
    UGTerm getRepr() const override;
    bool isPositive() const override;
    bool isNegative() const override;
    void setType(OccurrenceType x) override;
    OccurrenceType getType() const override;
    void checkDefined(LocSet &done, SigSet const &edb, UndefVec &) const override;
    // }}}2

private:
    ConjunctionComplete &complete_;
    DefinedBy defs_;
    Id_t offset_;
    OccurrenceType type_ = OccurrenceType::POSITIVELY_STRATIFIED;
    bool auxiliary_;
};

// }}}1

// {{{1 declaration of DisjointAccumulate

using Output::DisjointAtom;
using Output::DisjointDomain;
class DisjointComplete;

class DisjointAccumulate : public AbstractStatement {
public:
    DisjointAccumulate(DisjointComplete &complete, ULitVec &&lits);
    DisjointAccumulate(DisjointComplete &complete, UTermVec &&tuple, CSPAddTerm &&value, ULitVec &&lits);
    void printHead(std::ostream &out) const override;
    virtual ~DisjointAccumulate() noexcept;
    // {{{2 AbstractStatement interface
    void collectImportant(Term::VarSet &vars) override;
    // {{{2 Statement interface
    bool isNormal() const override { return true; }
    void linearize(Context &context, bool positive, Logger &log) override;
private:
    // {{{2 SolutionCallback interface
    void report(Output::OutputBase &out, Logger &log) override;
    // }}}2

private:
    DisjointComplete &complete_;
    UTermVec tuple_;
    CSPAddTerm value_;
    bool neutral_ = true;
};

// {{{1 declaration of DisjointComplete

class DisjointComplete : public Statement, private SolutionCallback, private BodyOcc {
public:
    using AccuVec = std::vector<std::reference_wrapper<DisjointAccumulate>>;
    using TodoVec = std::vector<Id_t>;

    DisjointComplete(DomainData &data, UTerm &&repr);
    virtual ~DisjointComplete() noexcept;

    DisjointDomain &dom() { return static_cast<DisjointDomain&>(def_.dom()); }
    void enqueue(DisjointDomain::Iterator atom);
    UTerm const &accuRepr() const { return accuRepr_; }
    UTerm const &domRepr() const { return def_.domRepr(); }
    void addAccuDom(DisjointAccumulate &accu) { accuDoms_.emplace_back(accu); }
    void setOutputRecursive() { outputRecursive_ = true; }

    // {{{2 Statement interface
    bool isNormal() const override;
    void analyze(Dep::Node &node, Dep &dep) override;
    void startLinearize(bool active) override;
    void linearize(Context &context, bool positive, Logger &log) override;
    void enqueue(Queue &q) override;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // }}}2
private:
    // {{{2 SolutionCallback interface
    void printHead(std::ostream &out) const override;
    void propagate(Queue &queue) override;
    void report(Output::OutputBase &out, Logger &log) override;
    // {{{2 BodyOcc interface
    UGTerm getRepr() const override;
    bool isPositive() const override;
    bool isNegative() const override;
    void setType(OccurrenceType x) override;
    OccurrenceType getType() const override;
    DefinedBy &definedBy() override;
    void checkDefined(LocSet &done, SigSet const &edb, UndefVec &undef) const override;
    // }}}2

private:
    AccuVec accuDoms_;
    HeadDefinition def_;
    UTerm accuRepr_;
    TodoVec todo_;
    DefinedBy defBy_;
    Instantiator inst_;
    OccurrenceType occType_ = OccurrenceType::STRATIFIED;
    bool outputRecursive_ = false;
};

// {{{1 declaration of DisjointLiteral

class DisjointLiteral : public Literal, private BodyOcc {
public:
    DisjointLiteral(DisjointComplete &complete, NAF naf, bool auxiliary);
    virtual ~DisjointLiteral() noexcept;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // {{{2 Literal interface
    UIdx index(Context &context, BinderType type, Term::VarSet &bound) override;
    bool isRecursive() const override;
    BodyOcc *occurrence() override;
    void collect(VarTermBoundVec &vars) const override;
    Score score(Term::VarSet const &bound, Logger &log) override;
    std::pair<Output::LiteralId,bool> toOutput(Logger &log) override;
    bool auxiliary() const override { return auxiliary_; }
    // }}}2
private:
    // {{{2 BodyOcc interface
    DefinedBy &definedBy() override;
    UGTerm getRepr() const override;
    bool isPositive() const override;
    bool isNegative() const override;
    void setType(OccurrenceType x) override;
    OccurrenceType getType() const override;
    void checkDefined(LocSet &done, SigSet const &edb, UndefVec &) const override;
    // }}}2

private:
    DisjointComplete &complete_;
    DefinedBy defs_;
    Id_t offset_ = InvalidId;
    OccurrenceType type_ = OccurrenceType::POSITIVELY_STRATIFIED;
    NAF naf_;
    bool auxiliary_;
};

// }}}1

// {{{1 declaration of TheoryAccumulate

using Output::TheoryAtom;
using Output::TheoryDomain;
class TheoryComplete;

class TheoryAccumulate : public AbstractStatement {
public:
    TheoryAccumulate(TheoryComplete &complete, ULitVec &&lits);
    TheoryAccumulate(TheoryComplete &complete, Output::UTheoryTermVec &&tuple, ULitVec &&lits);
    virtual ~TheoryAccumulate() noexcept;
    void printHead(std::ostream &out) const override;
    // {{{2 AbstractStatement interface
    void collectImportant(Term::VarSet &vars) override;
    // {{{2 Statement interface
    bool isNormal() const override { return true; }
    void linearize(Context &context, bool positive, Logger &log) override;
    // }}}2
private:
    // {{{2 SolutionCallback interface
    void report(Output::OutputBase &out, Logger &log) override;
    // }}}2

private:
    TheoryComplete &complete_;
    Output::UTheoryTermVec tuple_;
    bool neutral_ = true;
};

// {{{1 declaration of TheoryComplete

class TheoryComplete : public Statement, private SolutionCallback, private BodyOcc {
public:
    using AccuVec = std::vector<std::reference_wrapper<TheoryAccumulate>>;
    using TodoVec = std::vector<Id_t>;

    TheoryComplete(DomainData &data, UTerm &&repr, TheoryAtomType type, UTerm &&name);
    TheoryComplete(DomainData &data, UTerm &&repr, TheoryAtomType type, UTerm &&name, String op, Output::UTheoryTerm &&guard);
    virtual ~TheoryComplete() noexcept;
    TheoryDomain &dom() { return static_cast<TheoryDomain&>(def_.dom()); }
    void enqueue(TheoryDomain::Iterator atom);
    UTerm const &accuRepr() const { return accuRepr_; }
    UTerm const &domRepr() const { return def_.domRepr(); }
    void addAccuDom(TheoryAccumulate &accu) { accuDoms_.emplace_back(accu); }
    void setOutputRecursive() { outputRecursive_ = true; }
    UTerm const &name() const { return name_; }
    Output::UTheoryTerm const &guard() const { return guard_; }
    bool hasGuard() const { return static_cast<bool>(guard_); }
    String op() const { return op_; }
    TheoryAtomType type() const { return type_; }
    // {{{2 Statement interface
    bool isNormal() const override;
    void analyze(Dep::Node &node, Dep &dep) override;
    void startLinearize(bool active) override;
    void linearize(Context &context, bool positive, Logger &log) override;
    void enqueue(Queue &q) override;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // }}}2
private:
    // {{{2 SolutionCallback interface
    void printHead(std::ostream &out) const override;
    void propagate(Queue &queue) override;
    void report(Output::OutputBase &out, Logger &log) override;
    // {{{2 BodyOcc interface
    UGTerm getRepr() const override;
    bool isPositive() const override;
    bool isNegative() const override;
    void setType(OccurrenceType x) override;
    OccurrenceType getType() const override;
    DefinedBy &definedBy() override;
    void checkDefined(LocSet &done, SigSet const &edb, UndefVec &undef) const override;
    // }}}2

private:
    AccuVec accuDoms_;
    HeadDefinition def_;
    UTerm accuRepr_;
    TodoVec todo_;
    String op_;
    Output::UTheoryTerm guard_;
    UTerm name_;
    DefinedBy defBy_;
    Instantiator inst_;
    TheoryAtomType type_;
    OccurrenceType occType_ = OccurrenceType::STRATIFIED;
    bool outputRecursive_ = false;
};

// {{{1 declaration of TheoryLiteral

class TheoryLiteral : public Literal, private BodyOcc {
public:
    TheoryLiteral(TheoryComplete &complete, NAF naf, bool auxiliary);
    virtual ~TheoryLiteral() noexcept;
    TheoryAtomType type() const { return complete_.type(); }
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // {{{2 Literal interface
    UIdx index(Context &context, BinderType type, Term::VarSet &bound) override;
    bool isRecursive() const override;
    BodyOcc *occurrence() override;
    void collect(VarTermBoundVec &vars) const override;
    Score score(Term::VarSet const &bound, Logger &log) override;
    std::pair<Output::LiteralId,bool> toOutput(Logger &log) override;
    bool auxiliary() const override { return auxiliary_; }
    // }}}2
private:
    // {{{2 BodyOcc interface
    DefinedBy &definedBy() override;
    UGTerm getRepr() const override;
    bool isPositive() const override;
    bool isNegative() const override;
    void setType(OccurrenceType x) override;
    OccurrenceType getType() const override;
    void checkDefined(LocSet &done, SigSet const &edb, UndefVec &) const override;
    // }}}2

private:
    TheoryComplete &complete_;
    DefinedBy defs_;
    NAF naf_;
    Id_t offset_;
    OccurrenceType type_ = OccurrenceType::POSITIVELY_STRATIFIED;
    bool auxiliary_;
};

// {{{1 declaration of TheoryRule

class TheoryRule : public AbstractStatement {
public:
    TheoryRule(TheoryLiteral &lit, ULitVec &&lits);
    virtual ~TheoryRule() noexcept;
    // {{{2 AbstractStatement interface
    void collectImportant(Term::VarSet &vars) override;
    // {{{2 SolutionCallback interface
    void printHead(std::ostream &out) const override;
    void report(Output::OutputBase &out, Logger &log) override;
    // }}}2

private:
    TheoryLiteral &lit_;
};

// }}}1

// Head Aggregates

// {{{1 declaration of HeadAggregateComplete

using Output::HeadAggregateAtom;
using Output::HeadAggregateDomain;
class HeadAggregateAccumulate;

class HeadAggregateComplete : public Statement, private SolutionCallback, private BodyOcc {
public:
    using TodoVec = std::vector<HeadAggregateDomain::SizeType>;
    using AccumulateDomainVec = std::vector<std::reference_wrapper<HeadAggregateAccumulate>>;

    HeadAggregateComplete(DomainData &data, UTerm &&repr, AggregateFunction fun, BoundVec &&bounds);
    virtual ~HeadAggregateComplete() noexcept;

    HeadAggregateDomain &dom();
    UTerm const &domRepr() const { return repr_; }
    AggregateFunction fun() const { return fun_; }
    BoundVec const &bounds() const { return bounds_; }
    void enqueue(HeadAggregateDomain::Iterator atom);
    void addAccuDom(HeadAggregateAccumulate &accu) { accuDoms_.emplace_back(accu); }
    // {{{3 Statement interface
    bool isNormal() const override;                           // return false -> this one creates choices
    void analyze(Dep::Node &node, Dep &dep) override;         // use accuDoms to build the dependency...
    void startLinearize(bool active) override;                // noop because single instantiator
    void linearize(Context &context, bool positive, Logger &log) override; // noop because single instantiator
    void enqueue(Queue &q) override;                          // enqueue the single instantiator
    // {{{2 Printable interface
    void print(std::ostream &out) const override;             // #complete { h1, ..., hn } :- accu1,... , accun.
    // }}}2
private:
    // {{{2 SolutionCallback interface
    void printHead(std::ostream &out) const override;         // #complete { h1, ..., hn }
    void propagate(Queue &queue) override;                    // enqueue based on accuDoms
    void report(Output::OutputBase &out, Logger &log) override;            // loop over headDom here
    // {{{2 BodyOcc interface
    UGTerm getRepr() const override;
    bool isPositive() const override;
    bool isNegative() const override;
    void setType(OccurrenceType x) override;
    OccurrenceType getType() const override;
    DefinedBy &definedBy() override;
    void checkDefined(LocSet &done, SigSet const &edb, UndefVec &undef) const override;
    // }}}2

private:
    UTerm repr_;
    HeadAggregateDomain &domain_;
    AccumulateDomainVec accuDoms_;
    Instantiator inst_;
    OccurrenceType occType_ = OccurrenceType::STRATIFIED;
    DefinedBy defBy_;
    AggregateFunction fun_;
    BoundVec bounds_;
    TodoVec todo_;
};

// {{{1 declaration of HeadAggregateRule

class HeadAggregateRule : public AbstractStatement {
public:
    HeadAggregateRule(HeadAggregateComplete &complete, ULitVec &&lits);
    virtual ~HeadAggregateRule() noexcept;

    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // }}}2

private:
    // {{{2 SolutionCallback interface
    void report(Output::OutputBase &out, Logger &log) override;
    // }}}2

private:
    HeadAggregateComplete &complete_;
};

// {{{1 declaration of HeadAggregateLiteral

class HeadAggregateLiteral : public Literal, private BodyOcc {
public:
    HeadAggregateLiteral(HeadAggregateComplete &complete);
    virtual ~HeadAggregateLiteral() noexcept;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // {{{2 Literal interface
    UIdx index(Context &context, BinderType type, Term::VarSet &bound) override;
    bool isRecursive() const override;
    BodyOcc *occurrence() override;
    void collect(VarTermBoundVec &vars) const override;
    Score score(Term::VarSet const &bound, Logger &log) override;
    std::pair<Output::LiteralId,bool> toOutput(Logger &log) override;
    bool auxiliary() const override { return true; } // by construction
    // }}}2
private:
    // {{{2 BodyOcc interface
    DefinedBy &definedBy() override;
    UGTerm getRepr() const override;
    bool isPositive() const override;
    bool isNegative() const override;
    void setType(OccurrenceType x) override;
    OccurrenceType getType() const override;
    void checkDefined(LocSet &done, SigSet const &edb, UndefVec &) const override;
    // }}}2

private:
    HeadAggregateComplete &complete_;
    DefinedBy defs_;
    Potassco::Id_t offset_ = 0;
    OccurrenceType type_ = OccurrenceType::POSITIVELY_STRATIFIED;
};

// {{{1 declaration of HeadAggregateAccumulate

class HeadAggregateAccumulate : public AbstractStatement {
public:
    HeadAggregateAccumulate(HeadAggregateComplete &complete, UTermVec &&tuple, PredicateDomain *predDom, UTerm &&predRepr, ULitVec &&lits);
    virtual ~HeadAggregateAccumulate() noexcept;
    HeadDefinition &predDef() { return predDef_; }
    HeadDefinition const &predDef() const { return predDef_; }
    UTermVec const &tuple() const { return tuple_; }
    void printHead(std::ostream &out) const override;
    // {{{2 AbstractStatement interface
    void collectImportant(Term::VarSet &vars) override;
    // }}}2

private:
    // {{{2 SolutionCallback interface (w/o printHead)
    void report(Output::OutputBase &out, Logger &log) override;
    // }}}2

private:
    HeadAggregateComplete &complete_;
    HeadDefinition predDef_;
    UTermVec tuple_;
};

// }}}1

// {{{1 declaration of DisjunctionComplete

// Example:
//   p(1..3,X) : q(X,Y,G); r(1..10,Z) : s(Z,G) :- t(G,H).
// Domains:
//   DisjunctionLiteral: #d(G)
//   Condition 1: #c1(X,G)
//   Condition 2: #c2(Z,G)
// Pseudo Rules:
//   #d(G) :- t(G,H).
//   #c1(X,G) :- q(X,Y,G), #d(G).
//   #c2(Z,G) :- r(Z,G), #d(G).
//   p(R,X) :- R = 1..10, #c1(X,G).
//   p(R,Z) :- R = 1..10, #c2(Z,G).
// Notes:
//   - Given that the "head condition" never contains predicate literals (it can
//     just contain ranges), the grounding of the ranges is done within the
//     accumulation rule. This way no auxiliary domains are introduced.
// TODO:
//   - If there are no conditions, there is no need for accumulation rules altogether.

using Output::DisjunctionAtom;
using Output::DisjunctionDomain;

class DisjunctionAccumulate;
class DisjunctionComplete : public Statement, private SolutionCallback, private BodyOcc {
public:
    using TodoVec = std::vector<Id_t>;
    using AccumulateVec = std::vector<std::reference_wrapper<DisjunctionAccumulate>>;
    DisjunctionComplete(DomainData &data, UTerm &&repr);
    virtual ~DisjunctionComplete() noexcept;
    DisjunctionDomain &dom() { return domain_; }
    UTerm const &domRepr() const { return repr_; }
    void addAccu(DisjunctionAccumulate &accu) { accu_.emplace_back(accu); }
    void enqueue(DisjunctionDomain::Iterator atom);
    // {{{2 Statement interface
    bool isNormal() const override;                           // return false -> this one creates choices
    void analyze(Dep::Node &node, Dep &dep) override;         // use accuDoms to build the dependency...
    void startLinearize(bool active) override;                // noop because single instantiator
    void linearize(Context &context, bool positive, Logger &log) override; // noop because single instantiator
    void enqueue(Queue &q) override;                          // enqueue the single instantiator
    // {{{2 Printable interface
    void print(std::ostream &out) const override;             // #complete { h1, ..., hn } :- accu1,... , accun.
    // }}}2

private:
    // {{{2 SolutionCallback interface
    void printHead(std::ostream &out) const override;         // #complete { h1, ..., hn }
    void propagate(Queue &queue) override;                    // enqueue based on accuDoms
    void report(Output::OutputBase &out, Logger &log) override;            // loop over headDom here
    unsigned priority() const override { return 1; }
    // {{{2 BodyOcc interface
    UGTerm getRepr() const override;
    bool isPositive() const override;
    bool isNegative() const override;
    void setType(OccurrenceType x) override;
    OccurrenceType getType() const override;
    DefinedBy &definedBy() override;
    void checkDefined(LocSet &done, SigSet const &edb, UndefVec &undef) const override;
    // }}}2

private:
    UTerm repr_;
    DisjunctionDomain &domain_;
    DefinedBy defBy_;
    Instantiator inst_;
    TodoVec todo_;
    AccumulateVec accu_;
    OccurrenceType occType_ = OccurrenceType::STRATIFIED;
};

// {{{1 declaration of DisjunctionLiteral

class DisjunctionLiteral : public Literal, private BodyOcc {
public:
    DisjunctionLiteral(DisjunctionComplete &complete);
    virtual ~DisjunctionLiteral() noexcept;
    // {{{2 Printable interface
    void print(std::ostream &out) const override;
    // {{{2 Literal interface
    UIdx index(Context &context, BinderType type, Term::VarSet &bound) override;
    bool isRecursive() const override;
    BodyOcc *occurrence() override;
    void collect(VarTermBoundVec &vars) const override;
    Score score(Term::VarSet const &bound, Logger &log) override;
    std::pair<Output::LiteralId,bool> toOutput(Logger &log) override;
    bool auxiliary() const override { return true; } // by construction
    // }}}2
private:
    // {{{2 BodyOcc interface
    DefinedBy &definedBy() override;
    UGTerm getRepr() const override;
    bool isPositive() const override;
    bool isNegative() const override;
    void setType(OccurrenceType x) override;
    OccurrenceType getType() const override;
    void checkDefined(LocSet &done, SigSet const &edb, UndefVec &) const override;
    // }}}2

private:
    DisjunctionComplete &complete_;
    DefinedBy defs_;
    Potassco::Id_t offset_ = 0;
    OccurrenceType type_ = OccurrenceType::POSITIVELY_STRATIFIED;
};

// {{{1 declaration of DisjunctionRule

class DisjunctionRule : public AbstractStatement {
public:
    DisjunctionRule(DisjunctionComplete &complete, ULitVec &&lits);
    virtual ~DisjunctionRule() noexcept;
    // {{{2 Statement interface
    bool isNormal() const override;
    // {{{2 SolutionCallback interface
    void report(Output::OutputBase &out, Logger &log) override;
    // }}}2

private:
    DisjunctionComplete &complete_;
};

// {{{1 declaration of DisjunctionAccumulate

class DisjunctionAccumulate;
class DisjunctionAccumulateHead : public SolutionCallback {
public:
    DisjunctionAccumulateHead(DisjunctionAccumulate &accu) : accu_(accu) { }
    void report(Output::OutputBase &out, Logger &log) override;
    void propagate(Queue &) override { }
    void printHead(std::ostream &out) const override;
    virtual ~DisjunctionAccumulateHead() noexcept = default;
private:
    DisjunctionAccumulate &accu_;
};

class DisjunctionAccumulate : public AbstractStatement {
    friend class DisjunctionAccumulateHead;
public:
    DisjunctionAccumulate(DisjunctionComplete &complete, PredicateDomain *predDom, UTerm &&predRepr, ULitVec &&headCond, UTerm &&elemRepr, ULitVec &&lits);
    virtual ~DisjunctionAccumulate() noexcept;
    HeadDefinition &predDef() { return predDef_; }
    void printPred(std::ostream &out) const;
    void analyze(Dep::Node &node, Dep &dep) override;
    void collectImportant(Term::VarSet &vars) override;
private:
    void reportHead(Output::OutputBase &out, Logger &log);
    void linearize(Context &context, bool positive, Logger &log) override;
    // {{{2 SolutionCallback interface
    void report(Output::OutputBase &out, Logger &log) override;
    // }}}2

private:
    DisjunctionComplete &complete_;
    UTerm elemRepr_;
    HeadDefinition predDef_;
    ULitVec headCond_;
    DisjunctionAccumulateHead accuHead_;
    Instantiator instHead_;
};

// }}}1

} } // namespace Ground Gringo

#endif // _GRINGO_GROUND_STATEMENTS_HH
