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

#ifndef INDEXEDDOMAIN_H
#define INDEXEDDOMAIN_H

#include <gringo/gringo.h>
#include <gringo/value.h>

namespace gringo
{
	class IndexedDomain
	{
	public:
		IndexedDomain();
		virtual void firstMatch(int binder, DLVGrounder *g, MatchStatus &status) = 0;
		virtual void nextMatch(int binder, DLVGrounder *g, MatchStatus &status) = 0;
		virtual ~IndexedDomain();
	};

	class IndexedDomainMatchOnly : public IndexedDomain
	{
	public:
		IndexedDomainMatchOnly(Literal *l);
		virtual void firstMatch(int binder, DLVGrounder *g, MatchStatus &status);
		virtual void nextMatch(int binder, DLVGrounder *g, MatchStatus &status);
		virtual ~IndexedDomainMatchOnly();
	protected:
		Literal *l_;
	};

	class IndexedDomainNewDefault : public IndexedDomain
	{
		typedef HashMap<ValueVector, ValueVector, Value::VectorHash, Value::VectorEqual>::type ValueVectorMap;
	public:
		IndexedDomainNewDefault(Grounder *g, ValueVectorSet &domain, VarSet &index, const TermVector &param);
		virtual void firstMatch(int binder, DLVGrounder *g, MatchStatus &status);
		virtual void nextMatch(int binder, DLVGrounder *g, MatchStatus &status);
		virtual ~IndexedDomainNewDefault();
	protected:
		ValueVector currentIndex_;
                ValueVectorMap domain_;
		VarVector bind_;
		VarVector index_;
		ValueVector::iterator current_, end_;
	};
}

#endif

