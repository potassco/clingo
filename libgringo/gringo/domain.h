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

#ifndef DOMAIN_H
#define DOMAIN_H

#include <gringo/gringo.h>
#include <gringo/value.h>

namespace gringo
{
	class Domain
	{
	public:
		enum Type {UNDEFINED = 3, FACT = 0, BASIC = 1, NORMAL = 2};
	public:
		Domain();
		bool complete() const;
		bool solved() const;
		void setSolved(bool solved);
		void reset();
		void finish();
		void evaluate();
		bool hasFacts() const;
		void addFact(const ValueVector &values);
		bool isFact(const ValueVector &values) const;
		bool inDomain(const ValueVector &values) const;
		void addDomain(const ValueVector &values);
		void removeDomain(const ValueVector &values);
		void setType(Type type_);
		Type getType() const { return type_; }
		int getDefines();
		ValueVectorSet &getDomain() const;
		void moveDomain(Domain *nextDomain);
		~Domain();
	private:
		enum Type type_;
		int defines_;
		ValueVectorSet facts_;
		ValueVectorSet domain_;
		bool solved_;
	};
}

#endif

