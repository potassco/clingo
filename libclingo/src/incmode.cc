// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Benjamin Kaufmann
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

#include <clingo/incmode.hh>

namespace Gringo { namespace {

struct Incmode {
    Incmode(Control &ctl_) : ctl_(ctl_), istop(""), res(Gringo::SolveResult::Unknown, false, false) { }

    int get_max() {
        auto imax  = ctl_.getConst("imax");
        if (imax.type() == Gringo::SymbolType::Num) {
            return imax.num();
        }
        return std::numeric_limits<int>::max();
    }

    int get_min() {
        auto imin  = ctl_.getConst("imin");
        if (imin.type() == Gringo::SymbolType::Num) {
            return imin.num();
        }
        return 0;
    }

    String get_stop() {
        auto istop  = ctl_.getConst("istop");
        if (istop.type() == Gringo::SymbolType::Fun && istop.args().size == 0) {
            return istop.name();
        }
        return "SAT";
    }

    bool check_run() {
        if (step >= imax) { return false; }
        if (step == 0 || step < imin) { return true; }
        auto sat = res.satisfiable();
        if (istop == "SAT" && sat == SolveResult::Satisfiable) { return false; }
        if (istop == "UNSAT" && sat == SolveResult::Unsatisfiable) { return false; }
        if (istop == "UNKNOWN" && sat == SolveResult::Unknown) { return false; }
        return true;
    }

    void run() {
        ctl_.add("check", {"t"}, "#external query(t).");
        imax = get_max();
        imin = get_min();
        istop = get_stop();

        while (check_run()) {
            Control::GroundVec parts;
            parts.reserve(2);
            parts.push_back({"check", {Symbol::createNum(step)}});
            if (step > 0) {
                ctl_.assignExternal(Symbol::createFun("query", {Symbol::createNum(step - 1)}), Potassco::Value_t::Release);
                ctl_.cleanupDomains();
                parts.push_back({"step", {Symbol::createNum(step)}});
            }
            else {
                parts.push_back({"base", {}});
            }
            ctl_.ground(parts, nullptr);
            ctl_.assignExternal(Symbol::createFun("query", {Symbol::createNum(step)}), Potassco::Value_t::True);
            res = ctl_.solveRefactored({}, 0)->get();
            step += 1;
        }
    }

    Control &ctl_;
    int imax = 0;
    int imin = 0;
    int step = 0;
    String istop;
    SolveResult res;
};

} // namespace

void incmode(Gringo::Control &ctl_) {
    Incmode m(ctl_);
    m.run();
}

} // namespace Gringo


