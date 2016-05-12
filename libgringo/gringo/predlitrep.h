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

#pragma once

#include <gringo/gringo.h>

class PredLitRep
{
public:
	PredLitRep(bool sign, Domain *dom);
	void addDomain(Grounder *g, bool fact);
	void sign(bool s) { sign_ = s; }
	//! whether the literal has a sign
	bool sign() const { return sign_; }
	ValRng vals() const;
	ValRng vals(uint32_t top) const;
	Domain *dom() const { return dom_; }
protected:
	bool     sign_;
	bool     match_;
	uint32_t top_;
	Domain  *dom_;
	ValVec   vals_;
};

