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

#include <gringo/countaggregate.h>
#include <gringo/conditionalliteral.h>
#include <gringo/term.h>
#include <gringo/value.h>
#include <gringo/output.h>

using namespace gringo;

namespace
{
	struct Hash
	{
		Value::VectorHash hash;
		size_t operator()(const std::pair<int, ValueVector> &k) const
		{
			return (size_t)k.first + hash(k.second);
		}
	};
	struct Equal
	{
		Value::VectorEqual equal;
		size_t operator()(const std::pair<int, ValueVector> &a, const std::pair<int, ValueVector> &b) const
		{
			return a.first == b.first && equal(a.second, b.second);
		}
	};
	typedef HashSet<std::pair<int, ValueVector>, Hash, Equal>::type UidValueSet;
}

CountAggregate::CountAggregate(ConditionalLiteralVector *literals) : AggregateLiteral(literals)
{
}

void CountAggregate::doMatch(Grounder *g)
{
	UidValueSet set;
	fact_ = true;
	fixedValue_    = 0;
	minLowerBound_ = 0;
	maxUpperBound_ = 0;
	for(ConditionalLiteralVector::iterator it = literals_->begin(); it != literals_->end(); it++)
	{
		ConditionalLiteral *p = *it;
		p->ground(g, GROUND);
		for(p->start(); p->hasNext(); p->next())
		{
			// caution there is no -0
			if(!p->match() || !set.insert(std::make_pair(p->getNeg() ? -1 - p->getUid() : p->getUid(), p->getValues())).second)
			{
				p->remove();
				continue;
			}
			if(p->isFact())
				fixedValue_++;
			else
			{
				fact_ = false;
				maxUpperBound_++;
			}
		}
	}
	maxUpperBound_+= fixedValue_;
	minLowerBound_+= fixedValue_;
}

void CountAggregate::print(const GlobalStorage *g, std::ostream &out) const
{
	if(lower_)
		out << pp(g, lower_) << " ";
	out << "sum [";
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

CountAggregate::CountAggregate(const CountAggregate &a) : AggregateLiteral(a)
{
}

NS_OUTPUT::Object *CountAggregate::convert()
{
	NS_OUTPUT::ObjectVector lits;
	IntVector weights;
	for(ConditionalLiteralVector::iterator it = getLiterals()->begin(); it != getLiterals()->end(); it++)
	{
		ConditionalLiteral *p = *it;
		for(p->start(); p->hasNext(); p->next())
			lits.push_back(p->convert());
	}
	NS_OUTPUT::Aggregate *a;
	bool hasUpper = upperBound_ < maxUpperBound_;
	bool hasLower = lowerBound_ > minLowerBound_;

	if(hasLower && hasUpper)
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::COUNT, lowerBound_, lits, weights, upperBound_);
	else if(hasLower)
	{
		if(lowerBound_ == 1 && lits.size() == 1 && !lits.front()->neg_ && !getNeg())
			return lits.front();
		else
			a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::COUNT, lowerBound_, lits, weights);
	}
	else if(hasUpper)
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::COUNT, lits, weights, upperBound_);
	else
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::COUNT, lits, weights);
	return a;
}

Literal *CountAggregate::clone() const
{
	return new CountAggregate(*this);
}

CountAggregate::~CountAggregate()
{
}

