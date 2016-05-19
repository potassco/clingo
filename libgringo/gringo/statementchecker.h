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

#ifndef STATEMENTCHECKER_H
#define STATEMENTCHECKER_H

#include <gringo/gringo.h>

namespace gringo
{
	class StatementChecker
	{
	private:
		struct Delay
		{
			Literal *l;
			bool head;
			VarSet provided;
			VarSet needed;
		};
		typedef std::vector<Delay> DelayVector;
	public:
		StatementChecker(StatementChecker *parent = 0);
		bool check();
		void create();
		void getFreeVars(VarVector &vars);
		const VarSet &getVars() const;
		bool hasVars() const;

		void createDelayedNode(Literal *l, bool head, const VarSet &needed, const VarSet &provided);
		void createSubNode(Literal *l, bool head);
		void createNode(const VarSet &needed, const VarSet &provided);
	private:
		void createNode_(const VarSet &needed, const VarSet &provided);
	private:
		StatementChecker *parent_;
		VarSet vars_;
		VarSet provided_;
		DelayVector delayed_;
		std::list<std::pair<VarSet, VarSet> > providedIf_;
		std::list<std::pair<VarSet, std::pair<StatementChecker, VarSet> > > providedSub_;
	};
}

#endif

