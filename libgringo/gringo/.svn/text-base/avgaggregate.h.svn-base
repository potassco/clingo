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

#ifndef AVGAGGREGATE_H
#define AVGAGGREGATE_H

#include <gringo/gringo.h>
#include <gringo/aggregateliteral.h>

namespace gringo
{
	class AvgAggregate : public AggregateLiteral
	{
	public:
		AvgAggregate(ConditionalLiteralVector *literals);
		AvgAggregate(const AvgAggregate &a);
		virtual bool checkBounds(Grounder *g);
		virtual Literal *clone() const;
		virtual void doMatch(Grounder *g);
		virtual void print(const GlobalStorage *g, std::ostream &out) const;
		virtual NS_OUTPUT::Object *convert();
		virtual ~AvgAggregate();
	private:
		int avgLower_;
		int avgUpper_;

	};
}

#endif

