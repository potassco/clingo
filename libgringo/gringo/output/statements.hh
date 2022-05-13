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

#ifndef _GRINGO_OUTPUT_STATEMENTS_HH
#define _GRINGO_OUTPUT_STATEMENTS_HH

#include <gringo/output/statement.hh>
#include <gringo/output/literals.hh>

namespace Gringo { namespace Output {

// {{{1 declaration of Rule

class Rule : public Statement {
public:
    Rule(bool choice = false);
    void output(DomainData &data, UBackend &out) const override;
    void print(PrintPlain out, char const *prefix) const override;
    void translate(DomainData &data, Translator &trans) override;
    void replaceDelayed(DomainData &data, LitVec &delayed) override;
    Rule &reset(bool choice);
    Rule &addHead(LiteralId lit);
    Rule &addBody(LiteralId lit);
    template <class T>
    Rule &addBody(T const &t) {
        for (auto &x : t) { body_.emplace_back(x); }
        return *this;
    }
    template <class T>
    Rule &addHead(T const &t) {
        for (auto &x : t) { head_.emplace_back(x); }
        return *this;
    }
    Rule &negatePrevious(DomainData &data);
    size_t numHeads() const { return head_.size(); }
    LitVec const &heads() const { return head_; }
    LitVec const &body() const { return body_; }
    virtual ~Rule();

private:
    bool   choice_;
    LitVec head_;
    LitVec body_;
};

// {{{1 declaration of External

class External : public Statement {
public:
    External(LiteralId head, Potassco::Value_t type);
    void replaceDelayed(DomainData &data, LitVec &delayed) override;
    void print(PrintPlain out, char const *prefix) const override;
    void translate(DomainData &data, Translator &trans) override;
    void output(DomainData &data, UBackend &out) const override;
    virtual ~External();

private:
    LiteralId head_;
    Potassco::Value_t type_;
};

// }}}1
// {{{1 declaration of ShowStatement

class ShowStatement : public Statement {
public:
    ShowStatement(Symbol term, LitVec const &body);
    void replaceDelayed(DomainData &data, LitVec &delayed) override;
    void output(DomainData &data, UBackend &out) const override;
    void print(PrintPlain out, char const *prefix) const override;
    void translate(DomainData &data, Translator &trans) override;
    virtual ~ShowStatement() noexcept = default;

private:
    Symbol term_;
    LitVec body_;
};

// {{{1 declaration of ProjectStatement

class ProjectStatement : public Statement {
public:
    ProjectStatement(LiteralId atom);
    void replaceDelayed(DomainData &data, LitVec &delayed) override;
    void output(DomainData &data, UBackend &out) const override;
    void print(PrintPlain out, char const *prefix) const override;
    void translate(DomainData &data, Translator &trans) override;
    virtual ~ProjectStatement() noexcept = default;

private:
    LiteralId atom_;
};

// {{{1 declaration of HeuristicStatement

class HeuristicStatement : public Statement {
public:
    HeuristicStatement(LiteralId atom, int value, int priority, Potassco::Heuristic_t mod, LitVec const &body);
    void replaceDelayed(DomainData &data, LitVec &delayed) override;
    void output(DomainData &data, UBackend &out) const override;
    void print(PrintPlain out, char const *prefix) const override;
    void translate(DomainData &data, Translator &trans) override;
    virtual ~HeuristicStatement() noexcept = default;

private:
    LiteralId atom_;
    int value_;
    int priority_;
    Potassco::Heuristic_t mod_;
    LitVec body_;
};

// {{{1 declaration of EdgeStatement

class EdgeStatement : public Statement {
public:
    EdgeStatement(Symbol u, Symbol v, LitVec const &body);
    void replaceDelayed(DomainData &data, LitVec &delayed) override;
    void output(DomainData &data, UBackend &out) const override;
    void print(PrintPlain out, char const *prefix) const override;
    void translate(DomainData &data, Translator &trans) override;
    virtual ~EdgeStatement() noexcept = default;

private:
    Symbol u_;
    Symbol v_;
    Id_t uidU_;
    Id_t uidV_;
    LitVec body_;
};

// {{{1 declaration of WeakConstraint

class WeakConstraint : public Statement {
public:
    WeakConstraint(SymVec tuple, LitVec const &lits)
    : tuple_(tuple)
    , lits_(lits) { }
    void translate(DomainData &data, Translator &x) override;
    void output(DomainData &data, UBackend &out) const override;
    void print(PrintPlain out, char const *prefix) const override;
    void replaceDelayed(DomainData &data, LitVec &delayed) override;
    virtual ~WeakConstraint() noexcept = default;

private:
    SymVec tuple_;
    LitVec lits_;
};

// {{{1 declaration of TheoryDirective

class TheoryDirective : public Statement {
public:
    TheoryDirective(LiteralId theoryLit);
    void translate(DomainData &data, Translator &x) override;
    void output(DomainData &data, UBackend &out) const override;
    void print(PrintPlain out, char const *prefix) const override;
    void replaceDelayed(DomainData &data, LitVec &delayed) override;
    virtual ~TheoryDirective() noexcept = default;

private:
    LiteralId theoryLit_;
};

// }}}1

// Translation

// {{{1 declaration of Translator

enum class ShowType : unsigned {
    Shown      = 2,
    Atoms      = 4,
    Terms      = 8,
    Theory     = 16,
    All        = 31,
    Complement = 32
};

class Translator {
private:
    struct OutputEntry {
        OutputEntry(Symbol term, LiteralId cond)
        : term(term)
        , cond(cond) { }
        Symbol term;
        LiteralId cond;
        operator const Symbol&() const { return term; }
    };
    struct TodoOutputEntry {
        TodoOutputEntry(Symbol term, Formula &&cond)
        : term(term)
        , cond(std::move(cond)) { }
        operator const Symbol&() const { return term; }
        Symbol term;
        Formula cond;
    };
    struct OutputTable {
        using Table = UniqueVec<OutputEntry, std::hash<Symbol>, std::equal_to<Symbol>>;
        using Todo = UniqueVec<TodoOutputEntry, std::hash<Symbol>, std::equal_to<Symbol>>;
        Table table;
        Todo todo;
    };
public:
    using TupleLit        = std::pair<TupleId, LiteralId>;
    using MinimizeList    = std::vector<TupleLit>;
    using ProjectionVec   = std::vector<std::pair<Potassco::Id_t, Potassco::Id_t>>;
    using TupleLitMap     = UniqueVec<TupleLit, HashFirst<TupleId>, EqualToFirst<TupleId>>;

    Translator(UAbstractOutput &&out);
    void addMinimize(TupleId tuple, LiteralId cond);
    void atoms(DomainData &data, unsigned atomset, IsTrueLookup isTrue, SymVec &atoms, OutputPredicates const &outPreds);
    void translate(DomainData &data, OutputPredicates const &outPreds, Logger &log);
    void output(DomainData &data, Statement &x);
    void simplify(DomainData &data, Mappings &mappings, AssignmentLookup assignment);
    void showTerm(DomainData &data, Symbol term, LitVec &&cond);
    LiteralId removeNotNot(DomainData &data, LiteralId lit);
    unsigned nodeUid(Symbol v);
    // These are used to cache literals of translated formulas.
    // The clauses and formuals are tied to clauses and formulas in DomainData.
    // Hence, they have to be deleted after each step.
    LiteralId clause(ClauseId id, bool conjunctive, bool equivalence);
    void clause(LiteralId lit, ClauseId id, bool conjunctive, bool equivalence);
    void reset() { clauses_.clear(); }

    ~Translator();
private:
    LitVec updateCond(DomainData &data, OutputTable::Table &table, OutputTable::Todo::ValueType &todo);
    void showAtom(DomainData &data, PredDomMap::Iterator it);
    void showValue(DomainData &data, Symbol value, LitVec const &cond);
    void showValue(DomainData &data, Bound const &bound, LitVec const &cond);
    void translateMinimize(DomainData &data);
    void outputSymbols(DomainData &data, OutputPredicates const &outPreds, Logger &log);
    bool showSig(OutputPredicates const &outPreds, Sig sig);

    OutputTable termOutput_;
    MinimizeList minimize_;   // stores minimize constraint for current step
    TupleLitMap tuples_;      // to incrementally extend minimize constraint
    HashSet<uint64_t> seenSigs_;
    UAbstractOutput out_;
    UniqueVec<Symbol> nodeUids_;
    struct ClauseKey {
        uint64_t offset : 32;
        uint64_t size : 30;
        uint64_t conjunctive : 1;
        uint64_t equivalence : 1;
        uint64_t literal;

        uint64_t lower() const {
            return static_cast<uint64_t>(offset) << 32 | size << 2 | conjunctive << 1 | equivalence;
        }
        size_t hash() const {
            return std::hash<uint64_t>()(lower());
        }
        bool operator==(ClauseKey const &other) const {
            return lower() == other.lower();
        }
    };
    struct ClauseKeyLiterals {
        static constexpr ClauseKey open = { 0xffffffff, 0x3fffffff, 1, 1, std::numeric_limits<uint64_t>::max() };
        static constexpr ClauseKey deleted = { 0xffffffff, 0x3fffffff, 1, 1, std::numeric_limits<uint64_t>::max() - 1 };
    };
    HashSet<ClauseKey, ClauseKeyLiterals> clauses_;
};

// {{{1 declaration of Minimize

class Minimize : public Statement {
public:
    using LitWeightVec = std::vector<std::pair<LiteralId, int>>;
    Minimize(int priority);
    Minimize &add(LiteralId lit, Potassco::Weight_t weight);
    void translate(DomainData &data, Translator &x) override;
    void output(DomainData &data, UBackend &out) const override;
    void print(PrintPlain out, char const *prefix) const override;
    void replaceDelayed(DomainData &data, LitVec &delayed) override;
    virtual ~Minimize();

private:
    int priority_;
    LitWeightVec lits_;
};


// {{{1 declaration of Symtab

class Symtab : public Statement {
public:
    Symtab(Symbol symbol, LitVec &&body);
    void output(DomainData &data, UBackend &out) const override;
    void print(PrintPlain out, char const *prefix) const override;
    void translate(DomainData &data, Translator &trans) override;
    void replaceDelayed(DomainData &data, LitVec &delayed) override;
    virtual ~Symtab();

private:
    Symbol symbol_;
    LitVec body_;
};

// {{{1 declaration of WeightRule

class WeightRule : public Statement {
public:
    WeightRule(LiteralId head, Potassco::Weight_t lower, LitUintVec &&body);
    void output(DomainData &data, UBackend &out) const override;
    void print(PrintPlain out, char const *prefix) const override;
    void translate(DomainData &data, Translator &trans) override;
    void replaceDelayed(DomainData &data, LitVec &delayed) override;
    virtual ~WeightRule();

private:
    LiteralId          head_;
    LitUintVec         body_;
    Potassco::Weight_t lower_;
};

// }}}1

// Helpers

// {{{1 definition of BackendStatement

template <class T>
class BackendStatement : public Statement {
public:
    BackendStatement(T const &lambda)
    : lambda_(lambda)
    { }
    void output(DomainData &data, UBackend &out) const override { lambda_(data, out); }
    void print(PrintPlain, char const *) const override { }
    void translate(DomainData &data, Translator &trans) override {
        trans.output(data, *this);
    }
    void replaceDelayed(DomainData &, LitVec &) override { }
    virtual ~BackendStatement() { }
private:
    T const &lambda_;
};

template <class T>
void backendLambda(DomainData &data, AbstractOutput &out, T const &lambda) {
    BackendStatement<T>(lambda).passTo(data, out);
}

template <class T>
void backendLambda(DomainData &data, Translator &trans, T const &lambda) {
    BackendStatement<T> bs(lambda);
    trans.output(data, bs);
}

// }}}1

} } // namespace Output Gringo

#endif // _GRINGO_OUTPUT_STATEMENTS_HH
