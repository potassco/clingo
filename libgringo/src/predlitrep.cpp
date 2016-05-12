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

#include <gringo/predlitrep.h>
#include <gringo/domain.h>

PredLitRep::PredLitRep(bool sign, Domain *dom)
	: sign_(sign)
	, match_(true)
	, top_(0)
	, dom_(dom)
{
}

void PredLitRep::addDomain(Grounder *g, bool fact)
{
	assert(top_ + dom_->arity() <= vals_.size());
	dom_->insert(g, vals_.begin() + top_, fact);
}

ValRng PredLitRep::vals() const
{
	return ValRng(vals_.begin() + top_, vals_.begin() + top_ + dom_->arity());
}

ValRng PredLitRep::vals(uint32_t top) const
{
	return ValRng(vals_.begin() + top, vals_.begin() + top + dom_->arity());
}

