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

#ifndef EVALUATOR_H
#define EVALUATOR_H

#include <gringo/gringo.h>

namespace gringo
{
	/**
	 * \brief Basic evaluator backend
	 *
	 * The default implemation passes the given object to output and registers
	 * the head of the rules with the domain of the corresponding literals.
	 */
	class Evaluator
	{
	public:
		/**
		 * \brief Constructor
		 */
		Evaluator();
		/**
		 * \brief Initializes the evaluator
		 * \param g The grounder
		 */
		virtual void initialize(Grounder *g);
		/**
		 * \brief Adds an object to the evaluator
		 * \param r The object
		 */
		virtual void add(NS_OUTPUT::Object *r);
		/**
		 * \brief Starts the evaluation
		 *
		 * The grounder calls this method after all objects of a subprogram have been added.
		 */
		virtual void evaluate();
		/**
		 * \brief Destructor
		 */
		virtual ~Evaluator();
	public:
		/// Pointer to the grounder
		Grounder          *g_;
		/// Pointer to the output class
		NS_OUTPUT::Output *o_;
	};
}

#endif

