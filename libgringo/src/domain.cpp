// Copyright (c) 2008, Roland Kaminski
//
// This file is part of GrinGo.
//
// GrinGo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GrinGo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GrinGo.  If not, see <http://www.gnu.org/licenses/>.

#include <gringo/domain.h>
#include <gringo/value.h>

using namespace gringo;

Domain::Domain() : type_(UNDEFINED), defines_(0), solved_(false)
{
}

void Domain::setType(Type type)
{
	type_ = type;
}

int Domain::getDefines()
{
	return defines_;
}

bool Domain::complete() const
{
	return defines_ == 0;
}

bool Domain::solved() const
{
	return solved_;
}

void Domain::setSolved(bool solved)
{
	solved_ = solved;
/*
// incremental grounding may fail cause of that
// since this is only a very small optimization
// i simply turn it off in iclingo
#ifndef WITH_ICLASP
	// when a domain is solved all of its entries are facts
	if(solved_)
	{
		ValueVectorSet facts;
		std::swap(facts, facts_);
	}
#endif
*/
}

void Domain::evaluate()
{
	setSolved(complete() && (type_ == FACT  || type_ == BASIC));
}

void Domain::reset()
{
	defines_++;
	solved_ = false;
}

void Domain::finish()
{
	defines_--;
}

bool Domain::isFact(const ValueVector &values) const
{
	return facts_.find(values) != facts_.end();
}

void Domain::addFact(const ValueVector &values)
{
	// fact programs dont need to store their facts seperatly cause 
	// directly after grounding all values in their domain are facts
	// and there cant be any cycles through fact programs
	// examples:
	// a(1).
	// a(X) :- b(X)
	// b(X) :- a(X)
	// here a(1) is a fact but the rest is a basic program and a/1 is
	// not yet complete
	//if(type_ != FACT)
		facts_.insert(values);
}

bool Domain::inDomain(const ValueVector &values) const
{
	return domain_.find(values) != domain_.end();
}

void Domain::removeDomain(const ValueVector &values)
{
	domain_.erase(values);
}

void Domain::addDomain(const ValueVector &values)
{
	domain_.insert(values);
}

bool Domain::hasFacts() const
{
	return facts_.size() > 0;
}

ValueVectorSet &Domain::getDomain() const
{
	return const_cast<ValueVectorSet &>(domain_);
}

void Domain::moveDomain(Domain *nextDomain)
{
	domain_.swap(nextDomain->domain_);
	facts_.swap(nextDomain->facts_);
	nextDomain->domain_.clear();
	nextDomain->facts_.clear();
}

Domain::~Domain() 
{
}

