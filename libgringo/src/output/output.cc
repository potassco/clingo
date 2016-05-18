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
    void output(DomainData &, Backend &) const override { }
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
    void output(DomainData &, Backend &) const override { }
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
    EndStepStatement(OutputPredicates const &outPreds, bool solve)
    : outPreds_(outPreds), solve_(solve) { }
    void output(DomainData &, Backend &out) const override {
        if (solve_) {
            out.endStep();
        }
    }
    void print(PrintPlain out, char const *prefix) const override {
        for (auto &x : outPreds_) {
            if (std::get<1>(x).match("", 0)) { out << prefix << "#show " << (std::get<2>(x) ? "$" : "") << std::get<1>(x) << ".\n"; }
            else                             { out << prefix << "#show.\n"; }
        }
    }
    void translate(DomainData &data, Translator &trans) override {
        trans.translate(data, outPreds_);
        trans.output(data, *this);
    }
    void replaceDelayed(DomainData &, LitVec &) override { }
    virtual ~EndStepStatement() { }
private:
    OutputPredicates const &outPreds_;
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
    stm.output(data, *out_);
}

// {{{1 definition of OutputBase

OutputBase::OutputBase(Potassco::TheoryData &data, OutputPredicates &&outPreds, std::ostream &out, OutputFormat format, OutputDebug debug)
: outPreds(std::move(outPreds))
, data(data)
, out_(fromFormat(out, format, debug))
{ }

OutputBase::OutputBase(Potassco::TheoryData &data, OutputPredicates &&outPreds, UBackend &&out, OutputDebug debug)
: outPreds(std::move(outPreds))
, data(data)
, out_(fromBackend(std::move(out), debug))
{ }

OutputBase::OutputBase(Potassco::TheoryData &data, OutputPredicates &&outPreds, UAbstractOutput &&out)
: outPreds(std::move(outPreds))
, data(data)
, out_(std::move(out))
{ }

UAbstractOutput OutputBase::fromFormat(std::ostream &stream, OutputFormat format, OutputDebug debug) {
    if (format == OutputFormat::TEXT) {
        UAbstractOutput out;
        out = gringo_make_unique<TextOutput>("", stream);
        if (debug == OutputDebug::TEXT) {
            out = gringo_make_unique<TextOutput>("% ", std::cerr, std::move(out));
        }
        return out;
    }
    else {
        UBackend backend;
        switch (format) {
            case OutputFormat::REIFY: {
                throw std::logic_error("implement reified format");
            }
            case OutputFormat::INTERMEDIATE: {
                backend = gringo_make_unique<IntermediateFormatBackend>(data.theory().data(), stream);
                break;
            }
            case OutputFormat::SMODELS: {
                backend = gringo_make_unique<SmodelsFormatBackend>(stream);
                break;
            }
            case OutputFormat::TEXT: {
                throw std::logic_error("cannot happen");
            }
        }
        return fromBackend(std::move(backend), debug);
    }
}

UAbstractOutput OutputBase::fromBackend(UBackend &&backend, OutputDebug debug) {
    UAbstractOutput out;
    out = gringo_make_unique<BackendOutput>(std::move(backend));
    if (debug == OutputDebug::TRANSLATE || debug == OutputDebug::ALL) {
        out = gringo_make_unique<TextOutput>("%% ", std::cerr, std::move(out));
    }
    out = gringo_make_unique<TranslatorOutput>(std::move(out));
    if (debug == OutputDebug::TEXT || debug == OutputDebug::ALL) {
        out = gringo_make_unique<TextOutput>("% ", std::cerr, std::move(out));
    }
    return out;
}

void OutputBase::init(bool incremental) {
    backendLambda(data, *out_, [incremental](DomainData &, Backend &out) { out.init(incremental); });
}

void OutputBase::output(Statement &x) {
    x.replaceDelayed(data, delayed_);
    out_->output(data, x);
}

void OutputBase::flush() {
    for (auto &lit : delayed_) { DelayedStatement(lit).passTo(data, *out_); }
    delayed_.clear();
}

void OutputBase::beginStep() {
    backendLambda(data, *out_, [](DomainData &, Backend &out) { out.beginStep(); });
}

void OutputBase::endStep(bool solve) {
    if (!outPreds.empty()) {
        std::move(outPredsForce.begin(), outPredsForce.end(), std::back_inserter(outPreds));
        outPredsForce.clear();
    }
    EndStepStatement(outPreds, solve).passTo(data, *out_);
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

void OutputBase::reset() {
    data.reset();
    translateLambda(data, *out_, [](DomainData &, Translator &x) { x.reset(); });
}

void OutputBase::checkOutPreds() {
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
                GRINGO_REPORT(W_ATOM_UNDEFINED)
                    << std::get<0>(x) << ": info: no atoms over signature occur in program:\n"
                    << "  " << std::get<1>(x) << "\n";
            }
        }
    }
}

SymVec OutputBase::atoms(int atomset, IsTrueLookup isTrue) const {
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
    backendLambda(data, *out_, [&backend](DomainData &, Backend &out) { backend = &out; });
    return backend;
}

// }}}1

} } // namespace Output Gringo
