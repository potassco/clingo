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

#include "gringo/ground/program.hh"
#include "gringo/output/output.hh"

#define DEBUG_INSTANTIATION 0

namespace Gringo { namespace Ground {

// {{{ definition of Parameters

Parameters::Parameters() = default;
Parameters::~Parameters() { }
void Parameters::add(String name, SymVec &&args) {
    params[Sig((std::string("#inc_") + name.c_str()).c_str(), static_cast<uint32_t>(args.size()), false)].emplace(std::move(args));
}
bool Parameters::find(Sig sig) const {
    auto it = params.find(sig);
    return it != params.end() && !it->second.empty();
}
ParamSet::const_iterator Parameters::begin() const { return params.begin(); }
ParamSet::const_iterator Parameters::end() const   { return params.end(); }
bool Parameters::empty() const                     { return params.empty(); }
void Parameters::clear() { params.clear(); }

// }}}
// {{{ definition of Program

Program::Program(SEdbVec &&edb, Statement::Dep::ComponentVec &&stms, ClassicalNegationVec &&negate)
    : edb(std::move(edb))
    , stms(std::move(stms))
    , negate(std::move(negate)) { }

std::ostream &operator<<(std::ostream &out, Program const &p) {
    bool comma = false;
    for (auto &component : p.stms) {
        if (comma) { out << "\n"; }
        else       { comma = true; }
        out << "%" << (component.second ? " positive" : "") <<  " component";
        for (auto &stm : component.first) { out << "\n" << *stm; }
    }
    return out;
}

void Program::linearize(Context &context, Logger &log) {
    for (auto &x : stms) {
        for (auto &y : x.first) { y->startLinearize(true); }
        for (auto &y : x.first) { y->linearize(context, x.second, log); }
        for (auto &y : x.first) { y->startLinearize(false); }
    }
    linearized = true;
}

void Program::ground(Context &context, Output::OutputBase &out, Logger &log) {
    Parameters params;
    params.add("base", SymVec({}));
    ground(params, context, out, true, log);
}

void Program::ground(Parameters const &params, Context &context, Output::OutputBase &out, bool finalize, Logger &log) {
    for (auto &dom : out.predDoms()) {
        auto name = dom->sig().name();
        if (name.startsWith("#p_")) {
            // The idea here is to assign a fresh uid to each projection atom.
            // Furthermore, the fresh atom is derived by the old atom.
            // This prevents redefinition errors from projections.
            for (auto &atom : *dom) {
                if (!atom.fact() && atom.hasUid() && atom.defined()) {
                    Output::Rule &rule = out.tempRule(false);;
                    Atom_t oldUid = atom.uid();
                    Atom_t newUid = out.data.newAtom();
                    rule.addHead(Output::LiteralId{NAF::POS, Output::AtomType::Aux, newUid, dom->domainOffset()});
                    rule.addBody(Output::LiteralId{NAF::POS, Output::AtomType::Aux, oldUid, dom->domainOffset()});
                    out.output(rule);
                    atom.resetUid(newUid);
                }
            }
        }
        else if (name.startsWith("#inc_")) {
            // clear incremental domains
            dom->clear();
        }
        dom->incNext();
    }
    out.checkOutPreds(log);
    for (auto &x : edb) {
        if (params.find(std::get<0>(*x)->getSig())) {
            for (auto &z : std::get<1>(*x)) {
                auto it(out.predDoms().find(z.sig()));
                assert(it != out.predDoms().end());
                auto ret((*it)->define(z, true));
                if (!std::get<2>(ret)) {
                    Potassco::Id_t offset = static_cast<Id_t>(std::get<0>(ret) - (*it)->begin());
                    Potassco::Id_t domain = static_cast<Id_t>(it - out.predDoms().begin());
                    out.output(out.tempRule(false).addHead({NAF::POS, Output::AtomType::Predicate, offset, domain}));
                }
            }
        }
    }
    for (auto &p : params) {
        auto base = out.predDoms().find(p.first);
        if (base != out.predDoms().end()) {
            for (auto &args : p.second) {
                if (args.size() == 0) { (*base)->define(Symbol::createId(p.first.name()), true); }
                else { (*base)->define(Symbol::createFun(p.first.name(), Potassco::toSpan(args)), true); }
            }
        }
    }
    for (auto &x : out.predDoms()) { x->nextGeneration(); }
    Queue q;
    for (auto &x : stms) {
        if (!linearized) {
            for (auto &y : x.first) { y->startLinearize(true); }
            for (auto &y : x.first) { y->linearize(context, x.second, log); }
            for (auto &y : x.first) { y->startLinearize(false); }
        }
#if DEBUG_INSTANTIATION > 0
        std::cerr << "============= component ===========" << std::endl;
#endif
        for (auto &y : x.first) {
#if DEBUG_INSTANTIATION > 0
            std::cerr << "  enqueue: " << *y << std::endl;
#endif
            y->enqueue(q);
        }
        q.process(out, log);
    }
    for (auto &x : negate) {
        for (auto neg(x.second.begin() + x.second.incOffset()), ie(x.second.end()); neg != ie; ++neg) {
            Symbol v = static_cast<Symbol>(*neg).flipSign();
            auto pos(x.first.find(v));
            if (pos != x.first.end() && pos->defined()) {
                out.output(out
                    .tempRule(false)
                    .addBody({NAF::POS, Output::AtomType::Predicate, static_cast<Potassco::Id_t>(pos - x.first.begin()), x.first.domainOffset()})
                    .addBody({NAF::POS, Output::AtomType::Predicate, static_cast<Potassco::Id_t>(neg - x.second.begin()), x.second.domainOffset()}));
            }
        }
    }
    out.flush();
    if (finalize) { out.endStep(true, log); }
    linearized = true;
}

// }}}

} } // namespace Ground Gringo
