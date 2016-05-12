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

#include <gringo/output/statements.hh>
#include <gringo/output/lparseoutputter.hh>
#include <gringo/control.hh>

namespace Gringo { namespace Output {

struct PlainLparseOutputter : LparseOutputter {
    PlainLparseOutputter(std::ostream &out);
    virtual void incremental();
    virtual void printBasicRule(unsigned head, LitVec const &body);
    virtual void printChoiceRule(AtomVec const &head, LitVec const &body);
    virtual void printCardinalityRule(unsigned head, unsigned lower, LitVec const &body);
    virtual void printWeightRule(unsigned head, unsigned lower, LitWeightVec const &body);
    virtual void printMinimize(LitWeightVec const &body);
    virtual void printDisjunctiveRule(AtomVec const &head, LitVec const &body);
    virtual unsigned falseUid();
    virtual unsigned newUid();
    virtual void finishRules();
    virtual void printSymbol(unsigned atomUid, Value v);
    virtual void printExternal(unsigned atomUid, TruthValue type);
    virtual void finishSymbols();
    virtual bool &disposeMinimize() { return disposeMinimize_; }
    virtual ~PlainLparseOutputter();

    std::ostream &out;
    unsigned      uids             = 2;
    bool          disposeMinimize_ = true;
};

struct StmHandler {
    virtual void operator()(Statement &x) = 0;
    // TODO: this should go into a statement!!
    virtual void operator()(PredicateDomain::element_type &head, TruthValue type) = 0;
    virtual void incremental() { }
    virtual void finish(OutputPredicates &outPreds) = 0;
    virtual void atoms(int atomset, std::function<bool(unsigned)> const &isTrue, ValVec &atoms, OutputPredicates const &outPreds) = 0;
    virtual void simplify(AssignmentLookup assignment) = 0;
    virtual ~StmHandler() { }
};
using UStmHandler = std::unique_ptr<StmHandler>;

enum class LparseDebug : unsigned { NONE = 0, PLAIN = 1, LPARSE = 2, ALL = 3 };

struct OutputBase {
    OutputBase(OutputPredicates &&outPreds, std::ostream &out, bool lparse = false);
    OutputBase(OutputPredicates &&outPreds, LparseOutputter &out, LparseDebug debug = LparseDebug::NONE);
    void output(Value const &val);
    
    std::pair<unsigned, unsigned> simplify(AssignmentLookup assignment);
    void incremental();
    void createExternal(PredicateDomain::element_type &head);
    void assignExternal(PredicateDomain::element_type &head, TruthValue type);
    void output(UStm &&x);
    void output(Statement &x);
    void flush();
    void finish();
    void checkOutPreds();
    ValVec atoms(int atomset, std::function<bool(unsigned)> const &isTrue) const;
    PredicateDomain::element_type *find2(Gringo::Value val);
    AtomState const *find(Gringo::Value val) const;

    ValVec            tempVals;
    LitVec            tempLits;
    RuleRef           tempRule;   // Note: performance
    PredDomMap        domains;
    UStmVec           stms;
    UStmHandler       handler;
    OutputPredicates  outPreds;
    OutputPredicates  outPredsForce;
    bool              keepFacts = false;
};

} } // namespace Output Gringo

#endif // _GRINGO_OUTPUT_OUTPUT_HH

