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

#ifndef FUNCSYMBOL_H
#define FUNCSYMBOL_H

#include <gringo/gringo.h>

namespace gringo
{
	/**
	 * \brief class of function symbols
	 *
	 * consists of a string and a list of values 
	 * Is used in Value
	 */
	class FuncSymbol
	{
	public:
		/**
		 * \brief Constructor - f(a,b,c(1,2,3))    *s="f"
		 * doest not take ownership of s
		 */
		FuncSymbol(int s, const ValueVector& args);
		/**
		 * \brief simple hash function
		 * Merges the hash value of the argument vector with the hash value of the function name
		 */
		size_t getHash() const;
		/**
		 * \brief Comparison operator
		 */
		bool operator==(const FuncSymbol& a) const;
		/**
		 * \brief Prints the function symbol
		 * \param out The output stream
		 */
		void print(const GlobalStorage *g, std::ostream& out) const;
		/**
		 * \brief Get the name of the function symbol
		 * \return the string pointer to the name
		 */
		int getName() const;
		/**
		 * \brief Get the values of the function symbol
		 * \return a vector of values
		 */
		const ValueVector& getValues() const;
		/**
		 * \brief Virtual Destructor
		 */
		virtual ~FuncSymbol();
	protected:
		int                     name_;
		ValueVector		args_;
	};
}

#endif

