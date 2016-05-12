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

class Index
{
public:
	virtual std::pair<bool,bool> firstMatch(Grounder *grounder, int binder) = 0;
	virtual std::pair<bool,bool> nextMatch(Grounder *grounder, int binder) = 0;
	virtual void reset() = 0;
	virtual void finish() = 0;
	virtual bool hasNew() const = 0;
	virtual ~Index() { }
};

class MatchIndex : public Index
{
public:
	MatchIndex(Lit *lit);
	std::pair<bool,bool> firstMatch(Grounder *grounder, int binder);
	std::pair<bool,bool> nextMatch(Grounder *grounder, int binder);
	void reset() { finished_ = false; }
	void finish() { finished_ = true; }
	bool hasNew() const { return !finished_; }
private:
	bool finished_;
	Lit *lit_;
};

