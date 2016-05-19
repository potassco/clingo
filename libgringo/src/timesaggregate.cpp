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

#include <gringo/timesaggregate.h>
#include <gringo/conditionalliteral.h>
#include <gringo/term.h>
#include <gringo/value.h>
#include <gringo/output.h>


using namespace gringo;

namespace
{
	// very simple class to prevent integers from overflowing
	// in theory templates could be used here
	// NOTE: INT_MIN+1 is already treated as overflow
	class SafeInt
	{
	public:
		SafeInt(int v) : value(v) { }
		operator int() const { return value; }
		SafeInt operator*(const int b) const
		{
			if(value == 0 || b == 0)
				return 0;
			if(value < 0 && b < 0)
				return (value < INT_MAX / b) ? INT_MAX : (value * b);
			if(value < 0)
				return (value < (-INT_MAX) / b) ? -INT_MAX : (value * b);
			else if(b < 0)
				return (b < (-INT_MAX) / value) ? -INT_MAX : (value * b);
			else
				return (value > INT_MAX / b) ? INT_MAX : (value * b);
		}
		bool overflow() const
		{
			if(value >= 0)
				return value >= INT_MAX;
			else
				return value <= -INT_MAX;
		}
		int value;
	};
	SafeInt operator *(const SafeInt &a, const SafeInt &b)
	{
		return a * b.value;
	}
	SafeInt operator *(const int a, const SafeInt &b)
	{
		return b * a;
	}
}

TimesAggregate::TimesAggregate(ConditionalLiteralVector *literals) : AggregateLiteral(literals)
{
}

void TimesAggregate::doMatch(Grounder *g)
{
	// NOTE: fixedvalue may not be used
	fact_        = true;
	bool hasZero = false;
	bool hasNeg  = false;
	SafeInt upper(1);
	SafeInt fixed(1);

	for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
	{
		ConditionalLiteral *p = *it;
		p->ground(g, GROUND);
		for(p->start(); p->hasNext(); p->next())
		{
			int weight = p->getWeight();
			if(!p->match())
			{
				p->remove();
				continue;
			}
			if(p->isFact())
				fixed = fixed * weight;
			else
			{
				fact_ = false;
				if(weight < 0)
					hasNeg = true;;
				if(weight == 0)
					hasZero = true;
				else
					upper = upper * abs(weight);
			}
		}
	}
	minLowerBound_ = hasNeg ? upper * fixed * (-1) : (hasZero ? SafeInt(0) : fixed);
	maxUpperBound_ = upper * fixed;
	if(fixed < 0)
		std::swap(minLowerBound_, maxUpperBound_);
}

void TimesAggregate::print(const GlobalStorage *g, std::ostream &out) const
{
	if(lower_)
		out << pp(g, lower_) << " ";
	out << "times [";
	bool comma = false;
	for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
	{
		if(comma)
			out << ", ";
		else
			comma = true;
		out << pp(g, *it);
	}
	out << "]";
	if(upper_)
		out << " " << pp(g, upper_);
}

TimesAggregate::TimesAggregate(const TimesAggregate &a) : AggregateLiteral(a)
{
}

NS_OUTPUT::Object *TimesAggregate::convert()
{
	NS_OUTPUT::ObjectVector lits;
	IntVector weights;
	for(ConditionalLiteralVector::iterator it = getLiterals()->begin(); it != getLiterals()->end(); it++)
	{
		ConditionalLiteral *p = *it;
		for(p->start(); p->hasNext(); p->next())
		{
			lits.push_back(p->convert());
			weights.push_back(p->getWeight());
		}
	}
	NS_OUTPUT::Aggregate *a;
	bool hasUpper = upper_ && (upperBound_ < maxUpperBound_);
	bool hasLower = lower_ && (lowerBound_ > minLowerBound_);

	if(hasLower && hasUpper)
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::TIMES, lowerBound_, lits, weights, upperBound_);
	else if(hasLower)
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::TIMES, lowerBound_, lits, weights);
	else if(hasUpper)
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::TIMES, lits, weights, upperBound_);
	else
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::TIMES, lits, weights);

	return a;
}

Literal *TimesAggregate::clone() const
{
	return new TimesAggregate(*this);
}

TimesAggregate::~TimesAggregate()
{
}
