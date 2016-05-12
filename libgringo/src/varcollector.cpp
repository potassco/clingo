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

#include <gringo/varcollector.h>
#include <gringo/varterm.h>
#include <gringo/lit.h>
#include <gringo/groundable.h>
#include <gringo/grounder.h>

VarCollector::VarCollector(Grounder *grounder)
	: vars_(0)
	, level_(0)
	, grounder_(grounder)
{
}

void VarCollector::visit(VarTerm *var, bool bind)
{
	(void)bind;
	if(var->anonymous())
	{
		var->index(vars_, level_, true);
		grd_->vars().push_back(vars_);
		vars_++;
		grounder_->reserve(vars_);
	}
	else
	{
		std::pair<VarMap::iterator, bool> res = varMap_.insert(VarMap::value_type(var->nameId(), var));
		if(res.second)
		{
			varStack_.push_back(var->nameId());
			var->index(vars_, level_, true);
			grd_->vars().push_back(vars_);
			vars_++;
			grounder_->reserve(vars_);

		}
		else var->index(res.first->second->index(), res.first->second->level(), res.first->second->level() == level_);
	}
}

void VarCollector::visit(Term *term, bool bind)
{
	term->visit(this, bind);
}

void VarCollector::visit(Lit *lit, bool domain)
{
	(void)domain;
	lit->visit(this);
}

void VarCollector::visit(Groundable *grd, bool choice)
{
	(void)choice;
	grdQueue_.push_back(grd);
}

uint32_t VarCollector::collect()
{
	level_ = 0;
	while(grdQueue_.size() > 0)
	{
		grd_ = grdQueue_.back();
		grdQueue_.pop_back();
		if(grd_)
		{
			grdQueue_.push_back(0);
			varStack_.push_back(std::numeric_limits<uint32_t>::max());
			grd_->level(level_);
			grd_->visit(this);
			level_++;
		}
		else 
		{
			level_--;
			while(varStack_.back() != std::numeric_limits<uint32_t>::max())
			{
				varMap_.erase(varStack_.back());
				varStack_.pop_back();
			}
			varStack_.pop_back();
		}
	}
	return vars_;
}

