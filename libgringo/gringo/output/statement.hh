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

#ifndef _GRINGO_OUTPUT_STATEMENT_HH
#define _GRINGO_OUTPUT_STATEMENT_HH

#include <gringo/output/literal.hh>
#include <gringo/backend.hh>
#include <gringo/locatable.hh>
#include <gringo/symbol.hh>
#include <gringo/domain.hh>

namespace Gringo { namespace Output {

// {{{1 declaration of AbstractOutput

class AbstractOutput {
public:
    virtual void output(DomainData &data, Statement &stm) = 0;
    virtual ~AbstractOutput() noexcept = default;
};
using UAbstractOutput = std::unique_ptr<AbstractOutput>;

// {{{1 declaration of Statement

void replaceDelayed(DomainData &data, LiteralId &lit, LitVec &delayed);
void replaceDelayed(DomainData &data, LitVec &lits, LitVec &delayed);
void translate(DomainData &data, Translator &x, LiteralId &lit);
void translate(DomainData &data, Translator &x, LitVec &lits);

class Statement {
public:
    virtual ~Statement() noexcept = default;
    virtual void output(DomainData &data, UBackend &out) const = 0;
    virtual void print(PrintPlain out, char const *prefix = "") const = 0;
    virtual void translate(DomainData &data, Translator &trans) = 0;
    virtual void replaceDelayed(DomainData &data, LitVec &delayed) = 0;
    // convenience function
    void passTo(DomainData &data, AbstractOutput &out) { out.output(data, *this); }
};

// }}}1

} } // namespace Output Gringo

#endif // _GRINGO_OUTPUT_STATEMENT_HH

