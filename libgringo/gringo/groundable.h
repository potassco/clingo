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

class Groundable
{
public:
	Groundable();
	virtual bool grounded(Grounder *g) = 0;
	virtual void ground(Grounder *g) = 0;
	virtual void visit(PrgVisitor *visitor) = 0;
	virtual void init(Grounder *g, const VarSet &bound) = 0;
	VarVec &vars() { return vars_; }
	bool enqueued() const { return enqueued_; }
	void enqueued(bool e) { enqueued_ = e; }
	void instantiator(Instantiator *inst);
	Instantiator *instantiator() const;
	uint32_t level() const { return level_; }
	void level(uint32_t level) { level_ = level; }
	void litDep(LitDep::GrdNode *litDep);
	LitDep::GrdNode *litDep() { return litDep_.get(); }
protected:
	~Groundable();
protected:
	bool                       enqueued_;
	uint32_t                   level_;
	clone_ptr<Instantiator>    inst_;
	clone_ptr<LitDep::GrdNode> litDep_;
	VarVec                     vars_;
};

