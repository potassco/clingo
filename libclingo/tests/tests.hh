// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright Roland Kaminski

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

#include "clingo.hh"
#include "catch.hpp"

namespace Clingo { namespace Test {

using S = std::string;
using ModelVec = std::vector<SymbolVector>;
using MessageVec = std::vector<std::pair<WarningCode, std::string>>;

struct MCB {
    MCB(ModelVec &models) : models(models) {
        models.clear();
    }
    bool operator()(Model m) {
        models.emplace_back();
        for (auto sym : m.symbols(ShowType::Shown)) {
            models.back().emplace_back(sym);
        }
        std::sort(models.back().begin(), models.back().end());
        return true;
    }
    ~MCB() {
        std::sort(models.begin(), models.end());
    }
    ModelVec &models;
};

inline SolveResult test_solve(SolveHandle &&sh, ModelVec &models) {
    MCB cb(models);
    SolveResult ret;
    for (auto &m : sh) { cb(m); }
    return sh.get();
}

class LCB {
public:
    LCB(MessageVec &messages) : messages_(messages) { }
    void operator()(WarningCode code, char const *msg) { messages_.emplace_back(code, msg); }
private:
    MessageVec &messages_;
};

} } // namespace Test Clingo
