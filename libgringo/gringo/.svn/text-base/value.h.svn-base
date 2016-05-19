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

#ifndef VALUE_H
#define VALUE_H

#include <gringo/gringo.h>
#include <gringo/printable.h>

namespace gringo
{
	/**
	 * \brief Class to store values of terms
	 * If strings are stored in a value this class does not take ownership of the string.
	 * The memory of the string has to be freed elsewhere.
	 */
	class Value
	{
	public:
		/// The type o a value
		enum Type { INT, STRING, FUNCSYMBOL, UNDEF };
		/// Hash function object for arrays of values
		struct VectorHash
		{
			/**
			 * \brief Hash function for arrays of values
			 * \param key The array
			 * \return The hash
			 */
			inline size_t operator() (const ValueVector &key) const;
		};	
		/// Comaparison function object for arrays of values
		struct VectorEqual
		{
			/**
			 * \brief Comparison function for arrays of values
			 * \return The hash
			 */
			inline bool operator() (const ValueVector &a, const ValueVector &b) const;
		};	
	public:
		/**
		 * \brief Constructor initializing the value to undef
		 */
		Value();
		/**
		 * \brief Copy constructor
		 */
		Value(const Value &v);
		/**
		 * \brief Creates a value encapsulationg an object of type uid
		 * \param intValue The int
		 */
		Value(Type type, int uid_);
		/**
		 * \brief Calculates a hash for the value
		 * \return The Hash
		 */
		inline size_t hashValue() const;
		/**
		 * Function used to compare values
		 * \param b The other value
		 * \return Return an int less then 0 if the the value is lower than b, 0 if the values are equal or an int > 0 if the value is greater than b
		 */
		int compare(const GlobalStorage *g, const Value &b) const;
		/**
		 * \brief Function used to compare Values in hash_sets or hash_maps.
		 * This function doesnt throw an exception if the types of the values are distinct
		 * \return The result of the comparisson
		 */
		bool equal(const Value &b) const;
		/**
		 * \brief Operator casting any value to int
		 * \return if the current value is a string or undef an exception is thrown otherwise the value of the int is returned
		 */
		void print(const GlobalStorage *g, std::ostream &out) const;
		
		int toInt() const;
		operator int() const;
	public:
		/// The type of the value
		Type type_;
		/// The value its an uid for every type used
		int uid_;
	};
	
	inline const std::pair<const GlobalStorage *, const Value> print(const GlobalStorage *g, const Value &v)
	{
		return std::make_pair(g, v);
	}

	inline std::ostream &operator<<(std::ostream &out, const std::pair<const GlobalStorage *, const Value> &p)
	{
		p.second.print(p.first, out);
		return out;
	}
	
	/// Type to efficiently access values
	typedef HashSet<ValueVector, Value::VectorHash, Value::VectorEqual>::type ValueVectorSet;

	size_t Value::hashValue() const
	{
		switch(type_)
		{
			case INT:
				return (size_t)uid_;
			case STRING:
				return (size_t)uid_;
			case FUNCSYMBOL:
				return ~(size_t)uid_;
			default:
				// this shouldnt happen
				FAIL(true);
		}
	}

	bool Value::VectorEqual::operator() (const ValueVector &a, const ValueVector &b) const
	{
		if(a.size() != b.size())
			return false;
		for(ValueVector::const_iterator i = a.begin(), j = b.begin(); i != a.end(); i++, j++)
		{
			if(!i->equal(*j))
				return false;
		}
		return true;
	}	
	
	size_t Value::VectorHash::operator() (const ValueVector &key) const
	{
		size_t hash = 0;
		size_t x = 0;
		for(ValueVector::const_iterator it = key.begin(); it != key.end(); it++, x++)
		{
			hash = (hash << 4) + it->hashValue();
			if((x = hash & 0xF0000000L) != 0)
			{
				hash ^= (x >> 24);
			}
			hash &= ~x;
		}
		return hash;
	}

}

#endif

