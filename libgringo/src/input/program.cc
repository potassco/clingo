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

#include "gringo/input/program.hh"
#include "gringo/input/literals.hh"
#include "gringo/input/aggregates.hh"
#include "gringo/ground/literal.hh"
#include "gringo/ground/statements.hh"
#include "gringo/term.hh"
#include "gringo/logger.hh"
#include "gringo/graph.hh"
#include "gringo/safetycheck.hh"

namespace Gringo { namespace Input {

// {{{ definition of Block

Block::Block(Location const &loc, String name, IdVec &&params)
    : loc(loc)
    , name(name)
    , params(std::move(params))
    , edb(std::make_shared<Ground::SEdb::element_type>(nullptr, SymVec())) {
    UTermVec args;
    for (auto &param : this->params) { args.emplace_back(make_locatable<ValTerm>(param.first, Symbol::createId(param.second))); }
    if (args.empty()) { std::get<0>(*edb) = make_locatable<ValTerm>(loc, Symbol::createId(name)); }
    else              { std::get<0>(*edb) = make_locatable<FunctionTerm>(loc, name, std::move(args)); }
}

Block::Block(Block&&) = default;
Block &Block::operator=(Block&&) = default;
Block::~Block() = default;

Term const &Block::sig() const {
    return *std::get<0>(*edb);
}
Block::operator Term const &() const { return sig(); }

// }}}
// {{{ definition of Program

Program::Program() {
    begin(Location("<internal>", 1, 1, "<internal>", 1, 1), "base", IdVec({}));
}

Program::Program(Program &&) = default;

void Program::begin(Location const &loc, String name, IdVec &&params) {
    current_ = &*blocks_.push(loc, (std::string("#inc_") + name.c_str()).c_str(), std::move(params)).first;
}

void Program::add(UStm &&stm) {
    current_->addedEdb.emplace_back(stm->isEDB());
    if (current_->addedEdb.back().type() == SymbolType::Special) {
        current_->addedStms.emplace_back(std::move(stm));
        current_->addedEdb.pop_back();
    }
}

void Program::addInput(Sig sig) {
    sigs_.push(sig);
}

void Program::add(TheoryDef &&def, Logger &log) {
    auto it = theoryDefs_.find(def.name());
    if (it == theoryDefs_.end()) {
        theoryDefs_.push(std::move(def));
    }
    else {
        GRINGO_REPORT(log, Warnings::RuntimeError)
            << def.loc() << ": error: redefinition of theory:" << "\n"
            << "  " << def.name() << "\n"
            << it->loc() << ": note: theory first defined here\n";
    }

}

void Program::rewrite(Defines &defs, Logger &log) {
    for (auto &block : blocks_) {
        // {{{3 replacing definitions
        Defines incDefs;

        UTermVec args;
        AuxGen gen;
        for (auto &param : block.params) {
            args.emplace_back(gen.uniqueVar(param.first, 0, "#Inc"));
            incDefs.add(param.first, param.second, get_clone(args.back()), false, log);
        }
        sigs_.push(Sig(block.name, static_cast<uint32_t>(args.size()), false));
        UTerm blockTerm(args.empty()
            ? (UTerm)make_locatable<ValTerm>(block.loc, Symbol::createId(block.name))
            : make_locatable<FunctionTerm>(block.loc, block.name, get_clone(args)));
        VarTermBoundVec blockBound;
        blockTerm->collect(blockBound, true);
        incDefs.init(log);

        for (auto &fact : block.addedEdb) { sigs_.push(fact.sig()); }
        auto replace = [&](Defines &defs, Symbol fact) -> Symbol {
            if (defs.empty() || fact.type() == SymbolType::Special) { return fact; }
            UTerm rt;
            Symbol rv;
            defs.apply(fact, rv, rt, false);
            if (rt) {
                Location loc{rt->loc()};
                block.addedStms.emplace_back(make_locatable<Statement>(loc, gringo_make_unique<SimpleHeadLiteral>(make_locatable<PredicateLiteral>(loc, NAF::POS, std::move(rt))), UBodyAggrVec{}));
                return Symbol();
            }
            else if (rv.type() != SymbolType::Special) { return rv; }
            else { return fact; }
        };
        if (!defs.empty() || !incDefs.empty()) {
            for (auto &fact : block.addedEdb) {
                Symbol rv = replace(incDefs, replace(defs, fact));
                if (rv.type() != SymbolType::Special) { std::get<1>(*block.edb).emplace_back(rv); }
            }
            block.addedEdb.clear();
        }
        else if (std::get<1>(*block.edb).empty()) { std::swap(std::get<1>(*block.edb), block.addedEdb); }
        else { std::copy(block.addedEdb.begin(), block.addedEdb.end(), std::back_inserter(std::get<1>(*block.edb))); }
        // {{{3 rewriting
        // steps:
        // 1. unpool
        // 2. initialize theory
        // 3. simplify
        // 4. shift
        // 5. unpool comparisions
        // 6. rewrite aggregates
        auto rewrite2 = [&](UStm &x) -> void {
            std::get<1>(*block.edb).emplace_back(x->isEDB());
            if (std::get<1>(*block.edb).back().type() == SymbolType::Special) {
                x->add(make_locatable<PredicateLiteral>(block.loc, NAF::POS, get_clone(blockTerm), true));
                x->rewrite();
                block.stms.emplace_back(std::move(x));
                std::get<1>(*block.edb).pop_back();
            }
            else { sigs_.push(std::get<1>(*block.edb).back().sig()); }
        };
        auto rewrite1 = [&](UStm &x) -> void {
            x->initTheory(theoryDefs_, log);
            if (x->simplify(project_, log)) {
                x->shift();
                if (x->hasUnpoolComparison()) {
                    for (auto &y : x->unpoolComparison()) {
                        rewrite2(y);
                    }
                }
                else {
                    rewrite2(x);
                }
            }
        };
        for (auto &x : block.addedStms) {
            x->replace(defs);
            x->replace(incDefs);
            x->assignLevels(blockBound);
            if (x->hasPool(true)) { for (auto &y : x->unpool(true)) { rewrite1(y); } }
            else                  { rewrite1(x); }
        }
        block.addedStms.clear();
        // }}}3
    }
    // {{{3 projection
    for (auto &x : project_) {
        if (!x.done) {
            Location loc(x.project->loc());
            UBodyAggrVec body;
            body.emplace_back(gringo_make_unique<SimpleBodyLiteral>(make_locatable<ProjectionLiteral>(loc, get_clone(x.project))));
            stms_.emplace_back(make_locatable<Statement>(
                loc,
                gringo_make_unique<SimpleHeadLiteral>(make_locatable<PredicateLiteral>(loc, NAF::POS, get_clone(x.projected))),
                std::move(body)));
            x.done = true;
        }
    }
    // }}}3
}

void Program::check(Logger &log) {
    for (auto &block : blocks_) {
        for (auto &stm : block.stms) { stm->check(log); }
    }
    std::unordered_map<Sig, Location> seenSigs;
    for (auto &def : theoryDefs_) {
        for (auto &atomDef : def.atomDefs()) {
            auto seenSig = seenSigs.emplace(atomDef.sig(), atomDef.loc());
            if (!seenSig.second) {
                GRINGO_REPORT(log, Warnings::RuntimeError)
                    << atomDef.loc() << ": error: multiple definitions for theory atom:" << "\n"
                    << "  " << atomDef.sig() << "\n"
                    << seenSig.first->second << ": note: first defined here\n";
            }
        }
    }
}

void Program::print(std::ostream &out) const {
    for (auto &def : theoryDefs_) {
        out << def << "\n";
    }
    for (auto &block : blocks_) {
        for (auto &x : block.addedEdb)          { out << x << "." << "\n"; }
        for (auto &x : std::get<1>(*block.edb)) { out << x << "." << "\n"; }
        for (auto &x : block.addedStms)         { out << *x << "\n"; }
        for (auto &x : block.stms)              { out << *x << "\n"; }
    }
    for (auto &x : stms_) { out << *x << "\n"; }
}

// Defines atoms that have been seen in earlier steps
class DummyStatement : public Ground::Statement, private Ground::HeadOccurrence {
public:
    DummyStatement(UGTermVec terms, bool normal) : terms_{std::move(terms)}, normal_{normal}  {}
    bool isNormal() const override { return normal_; }
    void analyze(Dep::Node &node, Dep &dep) override {
        for (auto &term : terms_) {
            dep.provides(node, *this, get_clone(term));
        }
    }
    void startLinearize(bool) override { }
    void linearize(Context &, bool, Logger &) override { }
    void enqueue(Ground::Queue &) override { }
    void print(std::ostream &out) const override {
        print_comma(out, terms_, ";", [](std::ostream &out, UGTerm const &term) { out << *term; });
        out << ".";
    }
private:
    void defines(IndexUpdater &, Ground::Instantiator *) override { }
private:
    UGTermVec terms_;
    bool normal_;
};

Ground::Program Program::toGround(std::set<Sig> const &sigs, DomainData &domains, Logger &log) {
    Ground::UStmVec stms;
    if (!pheads.empty()) { stms.emplace_back(gringo_make_unique<DummyStatement>(std::move(pheads), true)); }
    if (!nheads.empty()) { stms.emplace_back(gringo_make_unique<DummyStatement>(std::move(nheads), false)); }
    stms.emplace_back(make_locatable<Ground::ExternalRule>(Location("#external", 1, 1, "#external", 1, 1)));
    ToGroundArg arg(auxNames_, domains);
    Ground::SEdbVec edb;
    for (auto &block : blocks_) {
        if (sigs.find(Sig{block.name.c_str() + 5, numeric_cast<uint32_t>(block.params.size()), false}) != sigs.end()) {
            edb.emplace_back(block.edb);
            for (auto &x : block.stms) {
                x->toGround(arg, stms);
            }
        }
    }
    for (auto &x : stms_) {
        x->toGround(arg, stms);
    }
    Ground::Statement::Dep dep;
    for (auto &x : stms) {
        bool normal(x->isNormal());
        auto &node(dep.add(std::move(x), normal));
        node.stm->analyze(node, dep);
    }
    auto ret = dep.analyze();
    Ground::Program prg(std::move(edb), std::move(std::get<0>(ret)));
    pheads = std::move(std::get<1>(ret));
    nheads = std::move(std::get<2>(ret));
    for (auto &sig : sigs_) {
        domains.add(sig);
    }
    Ground::UndefVec undef;
    for (auto &x : dep.depend.occs) {
        for (auto &y : x.second.first->depend) { (*std::get<0>(y)).checkDefined(locs_, sigs_, undef); }
    }
    std::sort(undef.begin(), undef.end(), [](Ground::UndefVec::value_type const &a, Ground::UndefVec::value_type const &b) { return a.first < b.first; });
    for (auto &x : undef) {
        GRINGO_REPORT(log, Warnings::AtomUndefined)
            << x.first << ": info: atom does not occur in any rule head:\n"
            << "  " << *x.second << "\n";
    }
    return prg;
}

Program::~Program() { }

std::ostream &operator<<(std::ostream &out, Program const &p) {
    p.print(out);
    return out;
}

// }}}

// }}}

} } // namespace Input Gringo
