
#include <gringo/aggregateliteral.h>

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

#include <gringo/avgaggregate.h>
#include <gringo/conditionalliteral.h>
#include <gringo/term.h>
#include <gringo/value.h>
#include <gringo/output.h>

using namespace gringo;

AvgAggregate::AvgAggregate(ConditionalLiteralVector *literals) : AggregateLiteral(literals)
{
}

void AvgAggregate::doMatch(Grounder *g)
{
	// avg aggregates may only be used lower and upper bounds
	assert(equal_ == 0);
	// fixedValue_ is not set and may not be used !!!

	avgLower_ = lower_ ? lower_->getValue(g) : 0;
	avgUpper_ = upper_ ? upper_->getValue(g) : 0;

	fact_ = true;
	minLowerBound_ = 0;
	maxUpperBound_ = 0;
	for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
	{
		ConditionalLiteral *p = *it;
		p->ground(g, GROUND);
		for(p->start(); p->hasNext(); p->next())
		{
			if(!p->match())
			{
				p->remove();
				continue;
			}
			int weightUpper = p->getWeight() - avgUpper_;
			int weightLower = p->getWeight() - avgLower_;
			if(p->isFact())
			{
				maxUpperBound_+= weightUpper;
				minLowerBound_+= weightLower;
			}
			else
			{
				fact_ = false;
				if(weightUpper > 0)
					maxUpperBound_+= weightUpper;
				if(weightLower < 0)
					minLowerBound_+= weightLower;
			}
		}
	}
	if(!lower_)
		minLowerBound_ = INT_MIN;
	if(!upper_)
		maxUpperBound_ = INT_MAX;
}

bool AvgAggregate::checkBounds(Grounder *g)
{
	lowerBound_ = lower_ ? 0 : minLowerBound_;
	upperBound_ = upper_ ? 0 : maxUpperBound_;
	// stupid bounds
	if(lowerBound_ > upperBound_)
		return getNeg();
	// the aggregate lies completely in the intervall
	// ---|         |--- <- Bounds
	// ------|   |------ <- Aggregate
	if(minLowerBound_ >= lowerBound_ && maxUpperBound_ <= upperBound_)
		return !getNeg();
	// the intervals dont intersect
	// ----------|   |--- <- Bounds
	// ---|   |---------- <- Aggregate
	if(maxUpperBound_ < lowerBound_ || minLowerBound_ > upperBound_)
		return getNeg();
	// the intervalls intersect
	return true;
}

void AvgAggregate::print(const GlobalStorage *g, std::ostream &out) const
{
	if(lower_)
		out << pp(g, lower_) << " ";
	out << "avg [";
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

AvgAggregate::AvgAggregate(const AvgAggregate &a) : AggregateLiteral(a)
{
}

NS_OUTPUT::Object *AvgAggregate::convert()
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
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::AVG, avgLower_, lits, weights, avgUpper_);
	else if(hasLower)
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::AVG, avgLower_, lits, weights);
	else if(hasUpper)
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::AVG, lits, weights, avgUpper_);
	else
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::AVG, lits, weights);

	return a;
}

Literal *AvgAggregate::clone() const
{
	return new AvgAggregate(*this);
}

AvgAggregate::~AvgAggregate()
{
}

