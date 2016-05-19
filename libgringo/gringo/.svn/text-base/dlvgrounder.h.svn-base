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

#ifndef DLVGROUNDER_H
#define DLVGROUNDER_H

#include <gringo/gringo.h>

namespace gringo
{
	class DLVGrounder
	{
	public:
		DLVGrounder(Grounder *g, Groundable *r, LiteralVector *lits, LDG *dg, const VarVector &relevant);
		void ground();
		void debug();
		void reinit();
		void release();
		~DLVGrounder();
	private:
		int closestBinder(int l, VarVector &vars, std::map<int,int> &firstBinder);
                void sortLiterals(LDG *dg);
		void calcDependency();
	public:
		Grounder *g_;
		Groundable *r_;
		LiteralVector lit_;
		IndexedDomainVector dom_;
		std::vector<VarVector> var_;
		std::vector<VarVector> dep_;
		VarVector closestBinderVar_;
		VarVector closestBinderDep_;
		VarVector closestBinderRel_;
		VarVector closestBinderSol_;
		VarVector global_;
		VarVector relevant_;
		VarSet    index_;
		std::vector<VarVector> provided_;
	};
}

#endif

