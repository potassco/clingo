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

#include <gringo/minaggregate.h>
#include <gringo/conditionalliteral.h>
#include <gringo/term.h>
#include <gringo/value.h>
#include <gringo/output.h>
#include <gringo/dlvgrounder.h>
#include <gringo/grounder.h>
#include <gringo/indexeddomain.h>
#include <gringo/variable.h>

using namespace gringo;

MinAggregate::MinAggregate(ConditionalLiteralVector *literals) : AggregateLiteral(literals)
{
}

void MinAggregate::doMatch(Grounder *g)
{
	fact_          = true;
	minLowerBound_ = INT_MAX;
	fixedValue_    = INT_MAX;
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
			int weight = p->getWeight();
			if(p->isFact())
				fixedValue_ = std::min(fixedValue_, weight);
			else
				fact_ = false;
			minLowerBound_ = std::min(minLowerBound_, weight);
		}
	}
	maxUpperBound_ = fixedValue_;
}

namespace
{
	class IndexedDomainMinAggregate : public IndexedDomain
	{
	protected:
		typedef std::set<int> IntSet;
	public:
		IndexedDomainMinAggregate(MinAggregate *l, int var);
		virtual void firstMatch(int binder, DLVGrounder *g, MatchStatus &status);
		virtual void nextMatch(int binder, DLVGrounder *g, MatchStatus &status);
		virtual ~IndexedDomainMinAggregate();
	protected:
		MinAggregate *l_;
		int var_;
		IntSet set_;
		IntSet::iterator current_;
	};

	IndexedDomainMinAggregate::IndexedDomainMinAggregate(MinAggregate *l, int var) : l_(l), var_(var)
	{
		
	}

	void IndexedDomainMinAggregate::firstMatch(int binder, DLVGrounder *g, MatchStatus &status)
	{
		l_->doMatch(g->g_);
		set_.clear();
		ConditionalLiteralVector *l = l_->getLiterals();
		for(ConditionalLiteralVector::iterator it = l->begin(); it != l->end(); it++)
		{
			ConditionalLiteral *p = *it;
			for(p->start(); p->hasNext(); p->next())
			{
				// insert all possible bindings for the bound
				int weight = p->getWeight();
				if(weight < l_->fixedValue_)
					set_.insert(weight);
			}
		}
		set_.insert(l_->fixedValue_);

		current_ = set_.begin();
		g->g_->setValue(var_, Value(Value::INT, *current_), binder);
		l_->setEqual(*current_); 
		status = SuccessfulMatch;
	}

	void IndexedDomainMinAggregate::nextMatch(int binder, DLVGrounder *g, MatchStatus &status)
	{
		
		current_++;
		if(current_ != set_.end())
		{
			g->g_->setValue(var_, Value(Value::INT, *current_), binder);
			l_->setEqual(*current_); 
			status = SuccessfulMatch;
		}
		else
			status = FailureOnNextMatch;
	}

	IndexedDomainMinAggregate::~IndexedDomainMinAggregate()
	{
	}
}

IndexedDomain *MinAggregate::createIndexedDomain(Grounder *g, VarSet &index)
{
	if(equal_)
	{
		if(index.find(equal_->getUID()) != index.end())
		{
			return new IndexedDomainMatchOnly(this);
		}
		else
		{
			return new IndexedDomainMinAggregate(this, equal_->getUID());
		}
	}
	else
		return new IndexedDomainMatchOnly(this);
}

void MinAggregate::print(const GlobalStorage *g, std::ostream &out) const
{
	if(lower_)
		out << pp(g, lower_) << " ";
	out << "min [";
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

MinAggregate::MinAggregate(const MinAggregate &a) : AggregateLiteral(a)
{
}

NS_OUTPUT::Object *MinAggregate::convert()
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
	bool hasUpper = upperBound_ < maxUpperBound_;
	bool hasLower = lowerBound_ > minLowerBound_;

	if(hasLower && hasUpper)
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::MIN, lowerBound_, lits, weights, upperBound_);
	else if(hasLower)
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::MIN, lowerBound_, lits, weights);
	else if(hasUpper)
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::MIN, lits, weights, upperBound_);
	else
		a = new NS_OUTPUT::Aggregate(getNeg(), NS_OUTPUT::Aggregate::MIN, lits, weights);
	return a;
}

Literal *MinAggregate::clone() const
{
	return new MinAggregate(*this);
}

MinAggregate::~MinAggregate()
{
}

