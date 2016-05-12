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

#include "tests/gringo_module.hh"

#include <clasp/clasp_facade.h>
#include <clasp/solver.h>


namespace Gringo { namespace Output { namespace Test {

// {{{ definition of solve

using Model  = std::vector<std::string>;
using Filter = std::initializer_list<std::string>;
using Models = std::vector<Model>;

class ModelPrinter : public Clasp::EventHandler {
public:
    ModelPrinter(Models &models, Filter &filter) : models(models), filter(filter) {}
    bool onModel(const Clasp::Solver& s, const Clasp::Model& m) {
        models.emplace_back();
        const Clasp::SymbolTable& symTab = s.symbolTable();
        for (Clasp::SymbolTable::const_iterator it = symTab.begin(); it != symTab.end(); ++it) {
            if (m.isTrue(it->second.lit) && !it->second.name.empty()) {
                std::string atom(it->second.name.c_str());
                for (auto &x : filter) {
                    if (!atom.compare(0, x.size(), x)) {
                        models.back().emplace_back(std::move(atom));
                        break;
                    }
                }
            }
        }
        std::sort(models.back().begin(), models.back().end());
        return true;
    }
    Models &models;
    Filter &filter;
};

inline Models solve(std::function<bool(OutputBase &, Scripts &, Input::Program&, Input::NonGroundParser &)> ground, std::string &&str, Filter filter = {""}, std::initializer_list<Clasp::wsum_t> minimize = {}) {
    // grounder: setup
    std::stringstream ss;
    PlainLparseOutputter plo(ss);
    OutputBase out({}, plo);
    Input::Program prg;
    Defines defs;
    Scripts scripts(Gringo::Test::getTestModule());
    Input::NongroundProgramBuilder pb(scripts, prg, out, defs);
    Input::NonGroundParser parser(pb);
    parser.pushStream("-", gringo_make_unique<std::stringstream>(std::move(str)));
    Models models;
    // grounder: parse
    parser.parse();
    // grounder: preprocess
    defs.init();
    prg.rewrite(defs);
    prg.check();
    if (ground(out, scripts, prg, parser)) {
        Clasp::ClaspFacade libclasp;
        Clasp::ClaspConfig config;
        config.solve.numModels = 0;
        config.solve.optMode = Clasp::EnumOptions::OptMode::enumerate;
        config.solve.optBound.assign(minimize.begin(), minimize.end());
        Clasp::Asp::LogicProgram &prg = libclasp.startAsp(config);
        prg.parseProgram(ss);
        libclasp.prepare();
        ModelPrinter printer(models, filter);
        libclasp.solve(&printer);
    }
    std::sort(models.begin(), models.end());
    return models;
}

inline Models solve(std::string &&str, std::initializer_list<std::string> filter = {""}, std::initializer_list<Clasp::wsum_t> minimize = {}) {
    auto ground = [](OutputBase &out, Scripts &scripts, Input::Program &prg, Input::NonGroundParser &) -> bool {
        // grounder: ground
        if (!message_printer()->hasError()) {
            Ground::Program gPrg(prg.toGround(out.domains));
            gPrg.ground(scripts, out);
            return true;
        }
        return false;
    };
    return solve(ground, std::move(str), filter, minimize);
}

// }}}

} } } // namespace Test Output Gringo

#endif // _GRINGO_OUTPUT_TEST_SOLVER_HELPER_HH
