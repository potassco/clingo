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

void Program::add(TheoryDef &&def, Logger &log) {
    auto it = theoryDefs_.find(def.name());
    if (it == theoryDefs_.end()) {
        theoryDefs_.push(std::move(def));
    }
    else {
        GRINGO_REPORT(log, clingo_error_runtime)
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
        incDefs.init(log);

        for (auto &fact : block.addedEdb) { sigs_.push(fact.sig()); }
        auto replace = [&](Defines &defs, Symbol fact) -> Symbol {
            if (defs.empty() || fact.type() == SymbolType::Special) { return fact; }
            UTerm rt;
            Symbol rv;
            defs.apply(fact, rv, rt, false);
            if (rt) {
                Location loc{rt->loc()};
                block.addedStms.emplace_back(make_locatable<Statement>(loc, gringo_make_unique<SimpleHeadLiteral>(make_locatable<PredicateLiteral>(loc, NAF::POS, std::move(rt))), UBodyAggrVec{}, StatementType::RULE));
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
        auto rewrite2 = [&](UStm &x) -> void {
            std::get<1>(*block.edb).emplace_back(x->isEDB());
            if (std::get<1>(*block.edb).back().type() == SymbolType::Special) {
                x->add(make_locatable<PredicateLiteral>(block.loc, NAF::POS, get_clone(blockTerm), true));
                x->rewrite2();
                block.stms.emplace_back(std::move(x));
                std::get<1>(*block.edb).pop_back();
            }
            else { sigs_.push(std::get<1>(*block.edb).back().sig()); }
        };
        auto rewrite1 = [&](UStm &x) -> void {
            x->initTheory(theoryDefs_, log);
            if (x->rewrite1(project_, log)) {
                if (x->hasPool(false)) { for (auto &y : x->unpool(false)) { rewrite2(y); } }
                else                   { rewrite2(x); }
            }
        };
        for (auto &x : block.addedStms) {
            x->replace(defs);
            x->replace(incDefs);
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
                std::move(body), StatementType::RULE));
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
                GRINGO_REPORT(log, clingo_error_runtime)
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

Ground::Program Program::toGround(DomainData &domains, Logger &log) {
    HashSet<uint64_t> neg;
    Ground::Program::ClassicalNegationVec negate;
    auto gn = [&neg, &negate, &domains](Sig x) {
        if (neg.insert(std::hash<uint64_t>(), std::equal_to<uint64_t>(), x.rep()).second) {
            negate.emplace_back(domains.add(x.flipSign()), domains.add(x));
        }
    };
    Ground::UStmVec stms;
    stms.emplace_back(make_locatable<Ground::ExternalRule>(Location("#external", 1, 1, "#external", 1, 1)));
    ToGroundArg arg(auxNames_, domains);
    Ground::SEdbVec edb;
    for (auto &block : blocks_) {
        for (auto &x : block.edb->second) {
            auto sig = x.sig();
            if (sig.sign()) { gn(sig); }
        }
        edb.emplace_back(block.edb);
        for (auto &x : block.stms) {
            x->getNeg(gn);
            x->toGround(arg, stms);
        }
    }
    for (auto &x : stms_) {
        x->getNeg(gn);
        x->toGround(arg, stms);
    }
    Ground::Statement::Dep dep;
    for (auto &x : stms) {
        bool normal(x->isNormal());
        auto &node(dep.add(std::move(x), normal));
        node.stm->analyze(node, dep);
    }
    Ground::Program prg(std::move(edb), dep.analyze(), std::move(negate));
    for (auto &sig : sigs_) {
        domains.add(sig);
    }
    Ground::UndefVec undef;
    for (auto &x : dep.depend.occs) {
        for (auto &y : x.second.first->depend) { (*std::get<0>(y)).checkDefined(locs_, sigs_, undef); }
    }
    std::sort(undef.begin(), undef.end(), [](Ground::UndefVec::value_type const &a, Ground::UndefVec::value_type const &b) { return a.first < b.first; });
    for (auto &x : undef) {
        GRINGO_REPORT(log, clingo_warning_atom_undefined)
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

} } // namespace Input Gringo
