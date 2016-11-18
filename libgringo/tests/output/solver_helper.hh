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

#ifndef _GRINGO_OUTPUT_TEST_SOLVER_HELPER_HH
#define _GRINGO_OUTPUT_TEST_SOLVER_HELPER_HH

#include "gringo/logger.hh"
#include "gringo/ground/dependency.hh"
#include "gringo/input/nongroundparser.hh"
#include "gringo/input/program.hh"
#include "gringo/output/output.hh"
#include "gringo/scripts.hh"

#include "tests/tests.hh"

#include <clasp/clasp_facade.h>
#include <clasp/solver.h>
#include <regex>


namespace Gringo { namespace Output { namespace Test {

// {{{ definition of solve

using Model  = std::vector<std::string>;
using Filter = std::initializer_list<std::string>;
using Models = std::vector<Model>;
using ModelsAndMessages = std::pair<std::vector<Model>, std::vector<std::string>>;

class ModelPrinter : public Clasp::EventHandler {
public:
    ModelPrinter(Models &models, Filter &filter) : models(models), filter(filter) {}
    bool onModel(const Clasp::Solver& s, const Clasp::Model& m) {
        models.emplace_back();
        const Clasp::OutputTable& out = s.outputTable();
        for (Clasp::OutputTable::pred_iterator it = out.pred_begin(); it != out.pred_end(); ++it) {
            if (m.isTrue(it->cond)) {
                onAtom(it->name.c_str());
            }
        }
        for (Clasp::OutputTable::fact_iterator it = out.fact_begin(); it != out.fact_end(); ++it) {
            onAtom(it->c_str());
        }
        std::sort(models.back().begin(), models.back().end());
        return true;
    }
    void onAtom(std::string&& atom) {
        for (auto &x : filter) {
            if (atom.compare(0, x.size(), x) == 0) {
                models.back().emplace_back(std::move(atom));
                return;
            }
        }

    }
    Models &models;
    Filter &filter;
};

struct ClingoState {
    ClingoState()
    : out(td, {}, ss, OutputFormat::INTERMEDIATE)
    , pb(scripts, prg, out, defs)
    , parser(pb, incmode) {
    }
    Gringo::Test::TestGringoModule module;
    std::stringstream ss;
    Potassco::TheoryData td;
    OutputBase out;
    Input::Program prg;
    Defines defs;
    Scripts scripts;
    Input::NongroundProgramBuilder pb;
    Input::NonGroundParser parser;
    bool incmode;
};

inline bool ground(ClingoState &state) {
    // grounder: ground
    if (!state.module.logger.hasError()) {
        Ground::Program gPrg(state.prg.toGround(state.out.data, state.module));
        state.out.init(false);
        state.out.beginStep();
        gPrg.ground(state.scripts, state.out, state.module);
        return true;
    }
    return false;
}

inline Models solve(ClingoState &state, std::string const &str, Filter filter = {""}, std::initializer_list<Clasp::wsum_t> minimize = {}) {
    state.parser.pushStream("-", gringo_make_unique<std::stringstream>(str), state.module);
    Models models;
    // grounder: parse
    state.parser.parse(state.module);
    // grounder: preprocess
    state.defs.init(state.module);
    state.prg.rewrite(state.defs, state.module);
    state.prg.check(state.module);
    if (ground(state)) {
        Clasp::ClaspFacade libclasp;
        Clasp::ClaspConfig config;
        config.solve.numModels = 0;
        config.solve.optMode = Clasp::EnumOptions::OptMode::enumerate;
        config.solve.optBound.assign(minimize.begin(), minimize.end());
        Clasp::Asp::LogicProgram &prg = libclasp.startAsp(config);
        prg.parseProgram(state.ss);
        libclasp.prepare();
        ModelPrinter printer(models, filter);
        libclasp.solve(&printer);
    }
    std::sort(models.begin(), models.end());
    return models;
}

inline ModelsAndMessages solve(std::string const &str, std::initializer_list<std::string> filter = {""}, std::initializer_list<Clasp::wsum_t> minimize = {}) {
    ClingoState state;
    return {solve(state, str, filter, minimize), state.module.messages()};
}

// }}}

} } } // namespace Test Output Gringo

#endif // _GRINGO_OUTPUT_TEST_SOLVER_HELPER_HH
