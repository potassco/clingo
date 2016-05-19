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

#ifndef STATEMENT_H
#define STATEMENT_H

#include <gringo/gringo.h>
#include <gringo/printable.h>
#include <gringo/groundable.h>

namespace gringo
{
	/**
	 * \brief Base class for every construct in a logic program that has to be grounded
	 */
	class Statement : public Printable, public Groundable
	{
	public:
		/// Constructor
		Statement();
		/**
		 * \brief Adds the needed dependencies to the SDG
		 * \param dg The SDG
		 */
		virtual void buildDepGraph(SDG *dg) = 0;
		/**
		 * \brief Returns all variables occuring in the Statement
		 * \param vars Reference to the result
		 */
		virtual void getVars(VarSet &vars) const = 0;
		/**
		 * \brief Checks the omega restricted part of the Statement
		 *
		 * Some literals like conditionals may need some domain restricted
		 * parts. This can be checked here. CheckO must be called before 
		 * check().
		 * \param unsolved List of literals that are not omega restricted
		 * \return Returns false in case of failure
		 */
		virtual bool checkO(LiteralVector &unsolved) = 0;
		/**
		 * \brief Checks if the program is lambda restricted
		 * \param free List of variables that are which are not bound by complete predicates
		 * \return Returns false in case of failure
		 */
		virtual bool check(Grounder *g, VarVector &free) = 0;
		/**
		 * \brief Performs static preprocessing.
		 *
		 * Currently ; and classical negation are removed here.
		 * \param g Reference to the grounder
		 */
		virtual void preprocess(Grounder *g) = 0;
		/**
		 * \brief Resets calls to finish and evaluate.
		 */
		virtual void reset() = 0;
		/**
		 * \brief Called if the grounding of the statement finished.
		 *
		 * This method is also used during check. Here it is called even if no 
		 * grounding was performed yet. Instead it is used to find out which 
		 * predicates would be complete after grounding the statement.
		 */
		virtual void finish() = 0;
		/**
		 * \brief Called if the evaluation of a subset of statements finished.
		 *
		 * Works simlar like finish.
		 */
		virtual void evaluate() = 0;
		/**
		 * \brief Starts the grounding of the statement
		 * \param g Reference to the grounder
		 * \todo Make use of the return value (if inconsistency found)
		 * \return Currently the returnvalue isn't used
		 */
		virtual bool ground(Grounder *g, GroundStep step) = 0;
		/**
		 * \brief Adds lparse domain statements to the statement
		 * \param pl The domain predicate
		 */
		virtual void addDomain(PredicateLiteral *pl) = 0;
		virtual void setIncPart(Grounder *g, IncPart part, const Value &v) = 0;
		/// Destructor
		virtual ~Statement();
	};
}

#endif

