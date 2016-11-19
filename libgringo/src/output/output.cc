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

#include "gringo/output/output.hh"
#include "gringo/logger.hh"
#include "gringo/output/aggregates.hh"
#include "gringo/output/backends.hh"
#include "reify/program.hh"
#include <cstring>

namespace Gringo { namespace Output {

// {{{1 internal functions and classes

namespace {

// {{{2 definition of DelayedStatement

class DelayedStatement : public Statement {
public:
    DelayedStatement(LiteralId lit)
    : lit_(lit)
    { }
    void output(DomainData &, UBackend &) const override { }
    void print(PrintPlain out, char const *prefix) const override {
        auto lit = call(out.domain, lit_, &Literal::delayedLit).first;
        out << prefix;
        call(out.domain, lit, &Literal::printPlain, out);
        out << " <=> ";
        call(out.domain, lit_, &Literal::printPlain, out);
        out << "\n";
    }
    void translate(DomainData &data, Translator &trans) override {
        trans.output(data, *this);
        call(data, lit_, &Literal::translate, trans);
    }
    void replaceDelayed(DomainData &, LitVec &) override { }
    virtual ~DelayedStatement() noexcept = default;
private:
    LiteralId lit_;
};

// {{{2 definition of TranslateStatement

template <class T>
class TranslateStatement : public Statement {
public:
    TranslateStatement(T const &lambda)
    : lambda_(lambda)
    { }
    void output(DomainData &, UBackend &) const override { }
    void print(PrintPlain, char const *) const override { }
    void translate(DomainData &data, Translator &trans) override {
        trans.output(data, *this);
        lambda_(data, trans);
    }
    void replaceDelayed(DomainData &, LitVec &) override { }
    virtual ~TranslateStatement() { }
private:
    T const &lambda_;
};

template <class T>
void translateLambda(DomainData &data, AbstractOutput &out, T const &lambda) {
    TranslateStatement<T>(lambda).passTo(data, out);
}

// {{{2 definition of EndStepStatement

class EndStepStatement : public Statement {
public:
    EndStepStatement(OutputPredicates const &outPreds, bool solve, Logger &log)
    : outPreds_(outPreds), log_(log), solve_(solve) { }
    void output(DomainData &, UBackend &out) const override {
        if (solve_) {
            out->endStep();
        }
    }
    void print(PrintPlain out, char const *prefix) const override {
        for (auto &x : outPreds_) {
            if (std::get<1>(x).match("", 0)) { out << prefix << "#show.\n"; }
            else                             { out << prefix << "#show " << (std::get<2>(x) ? "$" : "") << std::get<1>(x) << ".\n"; }
        }
    }
    void translate(DomainData &data, Translator &trans) override {
        trans.translate(data, outPreds_, log_);
        trans.output(data, *this);
    }
    void replaceDelayed(DomainData &, LitVec &) override { }
    virtual ~EndStepStatement() { }
private:
    OutputPredicates const &outPreds_;
    Logger &log_;
    bool solve_;
};

// }}}2

} // namespace

// {{{1 definition of TranslatorOutput

TranslatorOutput::TranslatorOutput(UAbstractOutput &&out)
: trans_(std::move(out)) { }

void TranslatorOutput::output(DomainData &data, Statement &stm) {
    stm.translate(data, trans_);
}

// {{{1 definition of TextOutput

TextOutput::TextOutput(std::string prefix, std::ostream &stream, UAbstractOutput &&out)
: prefix_(prefix)
, stream_(stream)
, out_(std::move(out)) { }

void TextOutput::TextOutput::output(DomainData &data, Statement &stm) {
    stm.print({data, stream_}, prefix_.c_str());
    if (out_) {
        out_->output(data, stm);
    }
}

// {{{1 definition of BackendOutput

BackendOutput::BackendOutput(UBackend &&out)
: out_(std::move(out)) { }

void BackendOutput::output(DomainData &data, Statement &stm) {
    stm.output(data, out_);
}

// {{{1 definition of OutputBase

OutputBase::OutputBase(Potassco::TheoryData &data, OutputPredicates &&outPreds, std::ostream &out, OutputFormat format, OutputOptions opts)
: outPreds(std::move(outPreds))
, data(data)
, out_(fromFormat(out, format, opts))
{ }

OutputBase::OutputBase(Potassco::TheoryData &data, OutputPredicates &&outPreds, UBackend &&out, OutputOptions opts)
: outPreds(std::move(outPreds))
, data(data)
, out_(fromBackend(std::move(out), opts))
{ }

OutputBase::OutputBase(Potassco::TheoryData &data, OutputPredicates &&outPreds, UAbstractOutput &&out)
: outPreds(std::move(outPreds))
, data(data)
, out_(std::move(out))
{ }

namespace {

template <class T>
class BackendAdapter : public Backend {
public:
    template <class... U>
    BackendAdapter(U&&... args) : prg_(std::forward<U>(args)...) {  }
    void initProgram(bool incremental)  override { prg_.initProgram(incremental); }
    void beginStep()  override { prg_.beginStep(); }

    void rule(Head_t ht, const AtomSpan& head, const LitSpan& body)  override { prg_.rule(ht, head, body); }
    void rule(Head_t ht, const AtomSpan& head, Weight_t bound, const WeightLitSpan& body)  override { prg_.rule(ht, head, bound, body); }
    void minimize(Weight_t prio, const WeightLitSpan& lits)  override { prg_.minimize(prio, lits); }

    void project(const AtomSpan& atoms) override { prg_.project(atoms); }
    void output(Symbol sym, Potassco::Atom_t atom) override {
        std::ostringstream out;
        out << sym;
        if (atom != 0) {
            Potassco::Lit_t lit = atom;
            prg_.output(Potassco::toSpan(out.str().c_str()), Potassco::LitSpan{&lit, 1});
        }
        else {
            prg_.output(Potassco::toSpan(out.str().c_str()), Potassco::LitSpan{nullptr, 0});
        }
    }
    void output(Symbol sym, Potassco::LitSpan const& condition) override {
        std::ostringstream out;
        out << sym;
        prg_.output(Potassco::toSpan(out.str().c_str()), condition);
    }
    void output(Symbol sym, int value, Potassco::LitSpan const& condition) override {
        std::ostringstream out;
        out << sym << "=" << value;
        prg_.output(Potassco::toSpan(out.str().c_str()), condition);
    }
    void external(Atom_t a, Value_t v)  override { prg_.external(a, v); }
    void assume(const LitSpan& lits)  override { prg_.assume(lits); }
    void heuristic(Atom_t a, Heuristic_t t, int bias, unsigned prio, const LitSpan& condition)  override { prg_.heuristic(a, t, bias, prio, condition); }
    void acycEdge(int s, int t, const LitSpan& condition)  override { prg_.acycEdge(s, t, condition); }

    void theoryTerm(Id_t termId, int number)  override { prg_.theoryTerm(termId, number); }
    void theoryTerm(Id_t termId, const StringSpan& name)  override { prg_.theoryTerm(termId, name); }
    void theoryTerm(Id_t termId, int cId, const IdSpan& args)  override { prg_.theoryTerm(termId, cId, args); }
    void theoryElement(Id_t elementId, const IdSpan& terms, const LitSpan& cond)  override { prg_.theoryElement(elementId, terms, cond); }
    void theoryAtom(Id_t atomOrZero, Id_t termId, const IdSpan& elements)  override { prg_.theoryAtom(atomOrZero, termId, elements); }
    void theoryAtom(Id_t atomOrZero, Id_t termId, const IdSpan& elements, Id_t op, Id_t rhs)  override {prg_.theoryAtom(atomOrZero, termId, elements, op, rhs); }
    void endStep() override { prg_.endStep(); }
private:
    T prg_;
};

} // namespace

UAbstractOutput OutputBase::fromFormat(std::ostream &stream, OutputFormat format, OutputOptions opts) {
    if (format == OutputFormat::TEXT) {
        UAbstractOutput out;
        out = gringo_make_unique<TextOutput>("", stream);
        if (opts.debug == OutputDebug::TEXT) {
            out = gringo_make_unique<TextOutput>("% ", std::cerr, std::move(out));
        }
        return out;
    }
    else {
        UBackend backend;
        switch (format) {
            case OutputFormat::REIFY: {
                backend = gringo_make_unique<BackendAdapter<Reify::Reifier>>(stream, opts.reifySCCs, opts.reifySteps);
                break;
            }
            case OutputFormat::INTERMEDIATE: {
                backend = gringo_make_unique<BackendAdapter<IntermediateFormatBackend>>(stream);
                break;
            }
            case OutputFormat::SMODELS: {
                backend = gringo_make_unique<BackendAdapter<SmodelsFormatBackend>>(stream);
                break;
            }
            case OutputFormat::TEXT: {
                throw std::logic_error("cannot happen");
            }
        }
        return fromBackend(std::move(backend), opts);
    }
}

UAbstractOutput OutputBase::fromBackend(UBackend &&backend, OutputOptions opts) {
    UAbstractOutput out;
    out = gringo_make_unique<BackendOutput>(std::move(backend));
    if (opts.debug == OutputDebug::TRANSLATE || opts.debug == OutputDebug::ALL) {
        out = gringo_make_unique<TextOutput>("%% ", std::cerr, std::move(out));
    }
    out = gringo_make_unique<TranslatorOutput>(std::move(out));
    if (opts.debug == OutputDebug::TEXT || opts.debug == OutputDebug::ALL) {
        out = gringo_make_unique<TextOutput>("% ", std::cerr, std::move(out));
    }
    return out;
}

void OutputBase::init(bool incremental) {
    backendLambda(data, *out_, [incremental](DomainData &, UBackend &out) { out->initProgram(incremental); });
}

void OutputBase::output(Statement &x) {
    x.replaceDelayed(data, delayed_);
    out_->output(data, x);
}

void OutputBase::flush() {
    for (auto &lit : delayed_) { DelayedStatement(lit).passTo(data, *out_); }
    delayed_.clear();
    backendLambda(data, *out_, [](DomainData &data, UBackend &out) {
        auto getCond = [&data](Id_t elem) {
            TheoryData &td = data.theory();
            BackendLitVec bc;
            for (auto &lit : td.getCondition(elem)) {
                bc.emplace_back(call(data, lit, &Literal::uid));
            }
            return bc;
        };
        Gringo::output(data.theory().data(), *out, getCond);
    });
}

void OutputBase::beginStep() {
    backendLambda(data, *out_, [](DomainData &, UBackend &out) { out->beginStep(); });
}

void OutputBase::endStep(bool solve, Logger &log) {
    if (!outPreds.empty()) {
        std::move(outPredsForce.begin(), outPredsForce.end(), std::back_inserter(outPreds));
        outPredsForce.clear();
    }
    EndStepStatement(outPreds, solve, log).passTo(data, *out_);
    // TODO: get rid of such things #d domains should be stored somewhere else
    std::set<Sig> rm;
    for (auto &x : predDoms()) {
        if (x->sig().name().startsWith("#d")) {
            rm.emplace(x->sig());
        }
    }
    if (!rm.empty()) {
        predDoms().erase([&rm](UPredDom const &dom) {
            return rm.find(dom->sig()) != rm.end();
        });
    }
}

void OutputBase::reset(bool resetData) {
    data.reset(resetData);
    translateLambda(data, *out_, [](DomainData &, Translator &x) { x.reset(); });
}

void OutputBase::checkOutPreds(Logger &log) {
    auto le = [](OutputPredicates::value_type const &x, OutputPredicates::value_type const &y) -> bool {
        if (std::get<1>(x) != std::get<1>(y)) { return std::get<1>(x) < std::get<1>(y); }
        return std::get<2>(x) < std::get<2>(y);
    };
    auto eq = [](OutputPredicates::value_type const &x, OutputPredicates::value_type const &y) {
        return std::get<1>(x) == std::get<1>(y) && std::get<2>(x) == std::get<2>(y);
    };
    std::sort(outPreds.begin(), outPreds.end(), le);
    outPreds.erase(std::unique(outPreds.begin(), outPreds.end(), eq), outPreds.end());
    for (auto &x : outPreds) {
        if (!std::get<1>(x).match("", 0) && !std::get<2>(x)) {
            auto it(predDoms().find(std::get<1>(x)));
            if (it == predDoms().end()) {
                GRINGO_REPORT(log, Warnings::AtomUndefined)
                    << std::get<0>(x) << ": info: no atoms over signature occur in program:\n"
                    << "  " << std::get<1>(x) << "\n";
            }
        }
    }
}

SymVec OutputBase::atoms(unsigned atomset, IsTrueLookup isTrue) const {
    SymVec atoms;
    translateLambda(const_cast<DomainData&>(data), *out_, [&](DomainData &data, Translator &trans) {
        trans.atoms(data, atomset, isTrue, atoms, outPreds);
    });
    return atoms;
}

std::pair<PredicateDomain::ConstIterator, PredicateDomain const *> OutputBase::find(Symbol val) const {
    return const_cast<OutputBase*>(this)->find(val);
}

std::pair<PredicateDomain::Iterator, PredicateDomain*> OutputBase::find(Symbol val) {
    if (val.type() == SymbolType::Fun) {
        auto it = predDoms().find(val.sig());
        if (it != predDoms().end()) {
            auto jt = (*it)->find(val);
            if (jt != (*it)->end() && jt->defined()) {
                return {jt, it->get()};
            }
        }
    }
    return {PredicateDomain::Iterator(), nullptr};
}

std::pair<Id_t, Id_t> OutputBase::simplify(AssignmentLookup assignment) {
    Id_t facts = 0;
    Id_t deleted = 0;
    if (true) {
    if (data.canSimplify()) {
        std::vector<Mapping> mappings;
        for (auto &dom : data.predDoms()) {
            mappings.emplace_back();
            auto ret = dom->cleanup(assignment, mappings.back());
            facts+= ret.first;
            deleted+= ret.second;
        }
        translateLambda(data, *out_, [&](DomainData &data, Translator &trans) { trans.simplify(data, mappings, assignment); });
    }
    }
    return {facts, deleted};
}

Backend *OutputBase::backend() {
    Backend *backend = nullptr;
    backendLambda(data, *out_, [&backend](DomainData &, UBackend &out) { backend = out.get(); });
    return backend;
}

namespace {

class BackendTee : public Backend {
public:
    using Head_t = Potassco::Head_t;
    using Value_t = Potassco::Value_t;
    using Heuristic_t = Potassco::Heuristic_t;
    using IdSpan = Potassco::IdSpan;
    using AtomSpan = Potassco::AtomSpan;
    using LitSpan = Potassco::LitSpan;
    using WeightLitSpan = Potassco::WeightLitSpan;

    BackendTee(UBackend a, UBackend b) : a_(std::move(a)), b_(std::move(b)) { }
    ~BackendTee() override = default;

    void initProgram(bool incremental) override {
        a_->initProgram(incremental);
        b_->initProgram(incremental);
    }
    void beginStep() override {
        a_->beginStep();
        b_->beginStep();
    }
    void endStep() override {
        a_->endStep();
        b_->endStep();
    }

    void rule(Head_t ht, const AtomSpan& head, const LitSpan& body) override {
        a_->rule(ht, head, body);
        b_->rule(ht, head, body);
    }
    void rule(Head_t ht, const AtomSpan& head, Weight_t bound, const WeightLitSpan& body) override {
        a_->rule(ht, head, bound, body);
        b_->rule(ht, head, bound, body);
    }
    void minimize(Weight_t prio, const WeightLitSpan& lits) override {
        a_->minimize(prio, lits);
        b_->minimize(prio, lits);
    }

    void project(const AtomSpan& atoms) override {
        a_->project(atoms);
        b_->project(atoms);
    }
    void output(Symbol sym, Potassco::Atom_t atom) override {
        a_->output(sym, atom);
        b_->output(sym, atom);
    }
    void output(Symbol sym, Potassco::LitSpan const& condition) override {
        a_->output(sym, condition);
        b_->output(sym, condition);
    }
    void output(Symbol sym, int value, Potassco::LitSpan const& condition) override {
        a_->output(sym, value, condition);
        b_->output(sym, value, condition);
    }
    void external(Atom_t a, Value_t v) override {
        a_->external(a, v);
        b_->external(a, v);
    }
    void assume(const LitSpan& lits) override {
        a_->assume(lits);
        b_->assume(lits);
    }
    void heuristic(Atom_t a, Heuristic_t t, int bias, unsigned prio, const LitSpan& condition) override {
        a_->heuristic(a, t, bias, prio, condition);
        b_->heuristic(a, t, bias, prio, condition);
    }
    void acycEdge(int s, int t, const LitSpan& condition) override {
        a_->acycEdge(s, t, condition);
        b_->acycEdge(s, t, condition);
    }

    void theoryTerm(Id_t termId, int number) override {
        a_->theoryTerm(termId, number);
        b_->theoryTerm(termId, number);
    }
    void theoryTerm(Id_t termId, const StringSpan& name) override {
        a_->theoryTerm(termId, name);
        b_->theoryTerm(termId, name);
    }
    void theoryTerm(Id_t termId, int cId, const IdSpan& args) override {
        a_->theoryTerm(termId, cId, args);
        b_->theoryTerm(termId, cId, args);
    }
    void theoryElement(Id_t elementId, const IdSpan& terms, const LitSpan& cond) override {
        a_->theoryElement(elementId, terms, cond);
        b_->theoryElement(elementId, terms, cond);
    }
    void theoryAtom(Id_t atomOrZero, Id_t termId, const IdSpan& elements) override {
        a_->theoryAtom(atomOrZero, termId, elements);
        b_->theoryAtom(atomOrZero, termId, elements);
    }
    void theoryAtom(Id_t atomOrZero, Id_t termId, const IdSpan& elements, Id_t op, Id_t rhs) override {
        a_->theoryAtom(atomOrZero, termId, elements, op, rhs);
        b_->theoryAtom(atomOrZero, termId, elements, op, rhs);
    }

private:
    UBackend a_;
    UBackend b_;
};

} // namespace

void OutputBase::registerObserver(UBackend prg, bool replace) {
    backendLambda(data, *out_, [&prg, replace](DomainData &, UBackend &out) {
        if (prg) {
            if (replace) { out = std::move(prg); }
            else         { out = gringo_make_unique<BackendTee>(std::move(prg), std::move(out)); }
        }
    });
}

// }}}1

} } // namespace Output Gringo
