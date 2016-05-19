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

#include <gringo/indexeddomain.h>
#include <gringo/value.h>
#include <gringo/grounder.h>
#include <gringo/dlvgrounder.h>
#include <gringo/literal.h>
#include <gringo/constant.h>

using namespace gringo;

//////////////////////////////// IndexedDomain ///////////////////////////////////////

IndexedDomain::IndexedDomain()
{
}

IndexedDomain::~IndexedDomain()
{
}

//////////////////////////////// IndexedDomainMatchOnly ///////////////////////////////////////

IndexedDomainMatchOnly::IndexedDomainMatchOnly(Literal *l) : IndexedDomain(), l_(l)
{
}

void IndexedDomainMatchOnly::firstMatch(int binder, DLVGrounder *g, MatchStatus &status)
{
	if(l_->match(g->g_))
		status = SuccessfulMatch;
	else
		status = FailureOnFirstMatch;
}

void IndexedDomainMatchOnly::nextMatch(int binder, DLVGrounder *g, MatchStatus &status)
{
	FAIL(true);
}

IndexedDomainMatchOnly::~IndexedDomainMatchOnly()
{
}

//////////////////////////////// IndexedDomainNewDefault ///////////////////////////////////////

IndexedDomainNewDefault::IndexedDomainNewDefault(Grounder *g, ValueVectorSet &domain, VarSet &index, const TermVector &paramNew)
{
	for(int i = 0; i < (int)paramNew.size(); i++)
	{
		VarSet variables;
		paramNew[i]->getVars(variables);
		for (VarSet::const_iterator j = variables.begin(); j != variables.end(); ++j)
		{
			//add the UID of bound variables
			if (index.find(*j) != index.end())
				index_.push_back(*j);
			else
			{
				//and unbound variables
				bind_.push_back(*j);
			}
		}
	}

	//make unique
	sort(index_.begin(), index_.end());
	VarVector::iterator newEnd = std::unique(index_.begin(), index_.end());
	index_.erase(newEnd, index_.end());

	sort(bind_.begin(), bind_.end());
	newEnd = std::unique(bind_.begin(), bind_.end());
	bind_.erase(newEnd, bind_.end());

	VarVector unifyVars;
	unifyVars.insert(unifyVars.end(), bind_.begin(), bind_.end());
	unifyVars.insert(unifyVars.end(), index_.begin(), index_.end());

	for(ValueVectorSet::iterator it = domain.begin(); it != domain.end(); it++)
	{
		const ValueVector &val = (*it);
		ValueVector unifyVals(unifyVars.size());
		bool doContinue = false;

		assert(paramNew.size() == val.size());
		TermVector::const_iterator p = paramNew.begin();
		for (ValueVector::const_iterator i = val.begin(); i != val.end(); ++i, ++p)
		{
			if (!(*p)->unify(g, *i, unifyVars, unifyVals))
			{
				doContinue = true;
				break;
			}
		}

		if (doContinue) continue;

		//die indexedDomain mit dem Index aller einer Instanz aller gebundenen Variablen ist gleich der Instanz aus der Domain
		// (mehrere Instanzen)
		ValueVector &v = domain_[ValueVector(unifyVals.begin() + bind_.size(), unifyVals.end())];
		v.insert(v.end(), unifyVals.begin(), unifyVals.begin() + bind_.size());
	}
}

void IndexedDomainNewDefault::firstMatch(int binder, DLVGrounder *g, MatchStatus &status)
{

	currentIndex_.clear();
	for (VarVector::const_iterator i = index_.begin(); i != index_.end(); ++i)
	{
		currentIndex_.push_back(g->g_->getValue(*i));
	}


	ValueVectorMap::iterator it = domain_.find(currentIndex_);



	if(it != domain_.end())
	{
		current_ = it->second.begin();
		end_     = it->second.end();
		for(unsigned int i = 0; i < bind_.size(); ++i)
		{
			assert(current_ != end_);
			// setze freie Variable X(bind_[i]) auf currentDomain[1 >1< 2], weil X an stelle 2(i) ist
			g->g_->setValue(bind_[i], (*current_), binder);
			++current_;
		}
		status = SuccessfulMatch;
	}
	else
		status = FailureOnFirstMatch;
}

void IndexedDomainNewDefault::nextMatch(int binder, DLVGrounder *g, MatchStatus &status)
{
	if(current_ != end_)
	{
		for(unsigned int i = 0; i < bind_.size(); ++i)
		{
			assert(current_ != end_);
			// setze freie Variable X(it->second) auf currentDomain[1 >1< 2], weil X an stelle 2 ist
			g->g_->setValue(bind_[i], (*current_), binder);
			++current_;
		}
		status = SuccessfulMatch;
	}
	else
		status = FailureOnNextMatch;

}

IndexedDomainNewDefault::~IndexedDomainNewDefault()
{
}

