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

#ifndef _GRINGO_OUTPUT_OUTPUT_HH
#define _GRINGO_OUTPUT_OUTPUT_HH

#include <gringo/output/types.hh>
#include <gringo/output/statements.hh>
#include <gringo/output/theory.hh>

namespace Gringo { namespace Output {

class TranslatorOutput : public AbstractOutput {
public:
    TranslatorOutput(UAbstractOutput &&out);
    void output(DomainData &data, Statement &stm) override;
private:
    Translator trans_;
};

class TextOutput : public AbstractOutput {
public:
    TextOutput(std::string prefix, std::ostream &stream, UAbstractOutput &&out = nullptr);
    void output(DomainData &data, Statement &stm) override;
private:
    std::string     prefix_;
    std::ostream   &stream_;
    UAbstractOutput out_;
};

class BackendOutput : public AbstractOutput {
public:
    BackendOutput(UBackend &&out);
    void output(DomainData &data, Statement &stm) override;
private:
    UBackend out_;
};

struct OutputOptions {
    OutputDebug debug      = OutputDebug::NONE;
    bool        reifySCCs  = false;
    bool        reifySteps = false;
};

using Assumptions = std::vector<std::pair<Gringo::Symbol, bool>>;
class OutputBase {
public:
    OutputBase(Potassco::TheoryData &data, OutputPredicates &&outPreds, std::ostream &out, OutputFormat format = OutputFormat::INTERMEDIATE, OutputOptions opts = OutputOptions());
    OutputBase(Potassco::TheoryData &data, OutputPredicates &&outPreds, UBackend &&out, OutputOptions opts = OutputOptions());
    OutputBase(Potassco::TheoryData &data, OutputPredicates &&outPreds, UAbstractOutput &&out);

    std::pair<Id_t, Id_t> simplify(AssignmentLookup assignment);
    void incremental();
    void output(Statement &x);
    void flush();
    void init(bool incremental);
    void beginStep();
    void endStep(bool solve, Logger &log);
    void checkOutPreds(Logger &log);
    SymVec atoms(unsigned atomset, IsTrueLookup lookup) const;
    std::pair<PredicateDomain::Iterator, PredicateDomain*> find(Symbol val);
    std::pair<PredicateDomain::ConstIterator, PredicateDomain const *> find(Symbol val) const;
    PredDomMap &predDoms() { return data.predDoms(); }
    PredDomMap const &predDoms() const { return data.predDoms(); }
    Rule &tempRule(bool choice) { return tempRule_.reset(choice); }
    SymVec &tempVals() { tempVals_.clear(); return tempVals_; }
    LitVec &tempLits() { tempLits_.clear(); return tempLits_; }
    Backend *backend();
    void registerObserver(UBackend prg, bool replace);
    void reset(bool resetData);
    void assume(Assumptions &&ass);
private:
    UAbstractOutput fromFormat(std::ostream &out, OutputFormat format, OutputOptions opts);
    UAbstractOutput fromBackend(UBackend &&out, OutputOptions opts);

public:
    SymVec tempVals_;
    LitVec tempLits_;
    Rule tempRule_;
    LitVec delayed_;
    OutputPredicates outPreds;
    DomainData data;
    OutputPredicates outPredsForce;
    UAbstractOutput out_;
    bool keepFacts = false;
};

} } // namespace Output Gringo

#endif // _GRINGO_OUTPUT_OUTPUT_HH

