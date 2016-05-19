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

#include <gringo/statementchecker.h>
#include <gringo/literal.h>

using namespace gringo;

StatementChecker::StatementChecker(StatementChecker *parent) : parent_(parent)
{
	
}

void StatementChecker::createSubNode(Literal *l, bool head)
{
	providedSub_.push_back(make_pair(delayed_.back().needed, make_pair(StatementChecker(this), delayed_.back().provided)));
	l->createNode(&providedSub_.back().second.first, head, false);
}

const VarSet &StatementChecker::getVars() const
{
	return vars_;
}

bool StatementChecker::hasVars() const
{
	return vars_.size() > 0;
}

bool StatementChecker::check()
{
	// also create the delayed nodes
	while(delayed_.size() > 0)
	{
		delayed_.back().l->createNode(this, delayed_.back().head, true);
		delayed_.pop_back();
	}

	bool r1, r2;
	do
	{
		r1 = r2 = false;
		bool f = providedIf_.size() > 0;
		while(f)
		{
			f = false;
			std::list<std::pair<VarSet, VarSet> >::iterator di, i = providedIf_.begin();
			while(i != providedIf_.end())
			{
				di = i++;
				VarSet::iterator dk, k = di->first.begin();
				while(k != di->first.end())
				{
					dk = k++;
					if(provided_.find(*dk) != provided_.end())
						di->first.erase(dk);
				}
				if(di->first.size() == 0)
				{
					provided_.insert(di->second.begin(), di->second.end());
					providedIf_.erase(di);
					f = r1 = true;
				}
			}
		}
		f = providedSub_.size() > 0;
		while(f)
		{
			f = false;
			std::list<std::pair<VarSet, std::pair<StatementChecker, VarSet> > >::iterator di, i = providedSub_.begin();
			while(i != providedSub_.end())
			{
				di = i++;
				VarSet::iterator dk, k = di->first.begin();
				while(k != di->first.end())
				{
					dk = k++;
					if(provided_.find(*dk) != provided_.end())
						di->first.erase(dk);
				}
				if(di->first.size() == 0 && di->second.first.check())
				{
					provided_.insert(di->second.second.begin(), di->second.second.end());
					providedSub_.erase(di);
					f = r2 = true;
				}
			}		
		}
		while(f);
	}
	while(r1 || r2);

	return provided_.size() == vars_.size() && providedSub_.size() == 0;
}

void StatementChecker::getFreeVars(VarVector &vars)
{
	if(provided_.size() != vars_.size())
	{
		for(VarSet::iterator i = vars_.begin(); i != vars_.end(); i++)
			if(provided_.find(*i) == provided_.end())
				vars.push_back(*i);
	}
	else
	{
		for(std::list<std::pair<VarSet, std::pair<StatementChecker, VarSet> > >::iterator i = providedSub_.begin(); i != providedSub_.end(); i++)
		{
			i->second.first.getFreeVars(vars);
			break;
		}
	}
}

void StatementChecker::createDelayedNode(Literal *l, bool head, const VarSet &needed, const VarSet &provided)
{
	// this can only happen if the nesting level is > 2
	assert(!parent_);
	delayed_.push_back(Delay());
	delayed_.back().l        = l;
	delayed_.back().head     = head;
	delayed_.back().provided = provided;
	delayed_.back().needed   = needed;
	vars_.insert(provided.begin(), provided.end());
	vars_.insert(needed.begin(), needed.end());
}

void StatementChecker::createNode_(const VarSet &needed, const VarSet &provided)
{
	vars_.insert(provided.begin(), provided.end());
	vars_.insert(needed.begin(), needed.end());
	if(needed.size() > 0 && provided.size() > 0)
		providedIf_.push_back(std::make_pair(needed, provided));
	else if(provided.size() > 0)
		provided_.insert(provided.begin(), provided.end());
}

void StatementChecker::createNode(const VarSet &needed, const VarSet &provided)
{
	if(parent_)
	{
		VarSet p, n;
		for(VarSet::const_iterator it = provided.begin(); it != provided.end(); it++)
			if(parent_->vars_.find(*it) != parent_->vars_.end())
				parent_->providedSub_.back().first.insert(*it);
			else
				p.insert(*it);
		for(VarSet::const_iterator it = needed.begin(); it != needed.end(); it++)
			if(parent_->vars_.find(*it) != parent_->vars_.end())
				parent_->providedSub_.back().first.insert(*it);
			else
				n.insert(*it);
		createNode_(n, p);
	}
	else
		createNode_(needed, provided);
}

