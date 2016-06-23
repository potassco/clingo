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

#ifndef _GRINGO_GROUND_TEST_GROUNDER_HELPER_HH
#define _GRINGO_GROUND_TEST_GROUNDER_HELPER_HH

#include "gringo/logger.hh"
#include "gringo/ground/dependency.hh"
#include "gringo/input/nongroundparser.hh"
#include "gringo/input/program.hh"
#include "gringo/output/output.hh"
#include "gringo/scripts.hh"

#include "tests/tests.hh"

#include <regex>

namespace Gringo { namespace Ground { namespace Test {

inline void ground(std::string const &str, Output::OutputFormat fmt, std::ostream &ss) {
    Gringo::Test::TestGringoModule module;
    Potassco::TheoryData td;
    Output::OutputBase out(td, {}, ss, fmt);
    Input::Program prg;
    Defines defs;
    Scripts scripts(module);
    Input::NongroundProgramBuilder pb{ scripts, prg, out, defs };
    Input::NonGroundParser ngp{ pb };
    ngp.pushStream("-", gringo_make_unique<std::stringstream>(str), module.logger);
    ngp.parse(module.logger);
    prg.rewrite(defs, module.logger);
    Ground::Program gPrg(prg.toGround(out.data, module.logger));
    gPrg.ground(scripts, out, module.logger);
}

inline std::string groundText(std::string const &str, std::initializer_list<std::string> filter = {""}) {
    std::regex delayedDef("^#delayed\\(([0-9]+)\\) <=> (.*)$");
    std::regex delayedOcc("#delayed\\(([0-9]+)\\)");
    std::map<std::string, std::string> delayedMap;
    std::stringstream ss;

    ground(str, Output::OutputFormat::TEXT, ss);

    std::string line;
    std::vector<std::string> res;
    ss.seekg(0, std::ios::beg);
    while (std::getline(ss, line)) {
        std::smatch m;
        if (std::regex_match(line, m, delayedDef)) {
            delayedMap[m[1]] = m[2];
        }
        else if (!line.compare(0, 9, "#delayed(")) {
            res.emplace_back(std::move(line));
        }
        else {
            for (auto &x : filter) {
                if (!line.compare(0, x.size(), x)) {
                    res.emplace_back(std::move(line));
                    break;
                }
            }
        }
    }
    for (auto &x : res) {
        std::string r;
        auto st = x.cbegin();
        for (auto it = std::sregex_iterator(x.begin(), x.end(), delayedOcc), ie = std::sregex_iterator(); it != ie; ++it) {
            std::smatch match = *it;
            r.append(st, match.prefix().second);
            st = match.suffix().first;
            r.append(delayedMap[match[1]]);
        }
        r.append(st, x.cend());
        x = r;
    }
    std::stringstream oss;
    std::sort(res.begin(), res.end());
    for (auto &x : res) { oss << x << "\n"; }
    return oss.str();
}

inline std::string groundAspif(std::string const &str) {
    std::stringstream ss;
    ground(str, Output::OutputFormat::INTERMEDIATE, ss);
    return ss.str();
}

} } } // namespace Test Ground Gringo

#endif // _GRINGO_GROUND_TEST_GROUNDER_HELPER_HH
