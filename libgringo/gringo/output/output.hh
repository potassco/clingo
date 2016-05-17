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

#ifndef _GRINGO_OUTPUT_OUTPUT_HH
#define _GRINGO_OUTPUT_OUTPUT_HH

#include <gringo/control.hh>
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

// TODO: rename (again)
class OutputBase {
public:
    OutputBase(Potassco::TheoryData &data, OutputPredicates &&outPreds, std::ostream &out, OutputFormat format = OutputFormat::INTERMEDIATE, OutputDebug debug = OutputDebug::NONE);
    OutputBase(Potassco::TheoryData &data, OutputPredicates &&outPreds, UBackend &&out, OutputDebug debug = OutputDebug::NONE);
    OutputBase(Potassco::TheoryData &data, OutputPredicates &&outPreds, UAbstractOutput &&out);
    template <typename T>
    OutputBase(T create, Potassco::TheoryData &data, OutputPredicates &&outPreds, OutputDebug debug = OutputDebug::NONE)
    : outPreds(std::move(outPreds))
    , data(data)
    , out_(fromBackend(create(this->data.theory()), debug)) { }

    std::pair<Id_t, Id_t> simplify(AssignmentLookup assignment);
    void incremental();
    void output(Statement &x);
    void flush();
    void init(bool incremental);
    void beginStep();
    void endStep(bool solve);
    void checkOutPreds();
    SymVec atoms(int atomset, IsTrueLookup lookup) const;
    std::pair<PredicateDomain::Iterator, PredicateDomain*> find(Symbol val);
    std::pair<PredicateDomain::ConstIterator, PredicateDomain const *> find(Symbol val) const;
    PredDomMap &predDoms() { return data.predDoms(); }
    PredDomMap const &predDoms() const { return data.predDoms(); }
    Rule &tempRule(bool choice) { return tempRule_.reset(choice); }
    SymVec &tempVals() { tempVals_.clear(); return tempVals_; }
    LitVec &tempLits() { tempLits_.clear(); return tempLits_; }
    Backend *backend();
    void reset();
private:
    UAbstractOutput fromFormat(std::ostream &out, OutputFormat format, OutputDebug debug);
    UAbstractOutput fromBackend(UBackend &&out, OutputDebug debug);

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

