// Copyright (c) 2009, Roland Kaminski <kaminski@cs.uni-potsdam.de>
//
// This file is part of gringo.
//
// gringo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with gringo.  If not, see <http://www.gnu.org/licenses/>.

#include <gringo/argterm.h>

ArgTerm::ArgTerm(const Loc &loc, Term *term, TermPtrVec &terms)
	: Term(loc)
	, term_(term)
	, terms_(terms.release())
{
}

void ArgTerm::print(Storage *sto, std::ostream &out) const
{
	term_->print(sto, out);
	out << ";;";
	bool comma = false;
	foreach(const Term &term, terms_)
	{
		if(comma) out << ",";
		else comma = true;
		term.print(sto, out);
	}
}

Term::Split ArgTerm::split() 
{ 
	return Split(term_.release(), &terms_);
}

Term *ArgTerm::clone() const
{
	return new ArgTerm(*this);
}

double ArgTerm::estimate(VarSet const &, double) const {
    return 1;
}

ArgTerm::~ArgTerm()
{
}

