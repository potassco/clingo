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

#include "gringo/output/statement.hh"
#include "gringo/output/literals.hh"

namespace Gringo { namespace Output {

void replaceDelayed(DomainData &data, LiteralId &lit, LitVec &delayed) {
    if (call(data, lit, &Literal::isIncomplete)) {
        auto ret = call(data, lit, &Literal::delayedLit);
        assert(ret.first.type() == AtomType::Aux);
        if (ret.second) {
            if (ret.first.sign() != NAF::POS) {
                assert(ret.first.sign() == lit.sign());
                delayed.emplace_back(lit.withSign(NAF::POS));
            }
            else { delayed.emplace_back(lit); }
        }
        lit = ret.first;
    }
}
void replaceDelayed(DomainData &data, LitVec &lits, LitVec &delayed) {
    for (auto &lit : lits) { replaceDelayed(data, lit, delayed); }
}

void translate(DomainData &data, Translator &x, LiteralId &lit) {
    lit = call(data, lit, &Literal::translate, x);
}

void translate(DomainData &data, Translator &x, LitVec &lits) {
    for (auto &lit : lits) {
        translate(data, x, lit);
    }
}

} } // namespace Output Gringo

