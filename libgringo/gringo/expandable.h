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

#ifndef EXPANDABLE_H
#define EXPANDABLE_H

#include <gringo/gringo.h>

namespace gringo
{
	/// Interface which allows to expand objects by literals
	class Expandable
	{
	public:
		enum ExpansionType { MATERM, RANGETERM, COMPLEXTERM };
	public:
		/**
		 * \brief Appends a literal to the object
		 * \param l The literal
		 */
		virtual void appendLiteral(Literal *l, ExpansionType type) = 0;
		virtual ~Expandable(){};
	};
}

#endif 
