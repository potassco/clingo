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

#ifndef PRINTABLE_H
#define PRINTABLE_H

#include <gringo/gringo.h>

namespace gringo
{
	typedef std::pair<GlobalStorage *, std::ostream> OutputStream;
	/**
	 * Interface for Printable objects like terms, literals or statements.
	 * This interface is used to print out an internal representation of the problem instance.
	 */
	class Printable
	{
	public:
		/**
		 * \brief Constructor - does nothing
		 */
		Printable();
		/**
		 * \brief Virtual destructor
		 */
		virtual ~Printable();
		/**
		 * \brief Prints the object on the given stream
		 * \param stream The output stream
		 */
		virtual void print(const GlobalStorage *g, std::ostream &out) const = 0;
	};
	
	inline const std::pair<const GlobalStorage *, const Printable *> pp(const GlobalStorage *g, const Printable *p)
	{
		return std::make_pair(g, p);
	}
	
	inline std::ostream &operator<<(std::ostream &out, const std::pair<const GlobalStorage *, const Printable *> &p)
	{
		p.second->print(p.first, out);
		return out;
	}
}

#endif

