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

#ifndef LITERAL_H
#define LITERAL_H

#include <gringo/gringo.h>
#include <gringo/printable.h>

namespace gringo
{
	/**
	 * \brief Base class for all literals that can be handled by the grounder
	 */
	class Literal : public Printable
	{
	public:
		/// Type used during creation of the dependency graph
		enum DependencyAdd 
		{ 
			ADD_BODY_DEP, ///< Used while processing body literals
			ADD_HEAD_DEP, ///< Used while processing head literals
			ADD_NOTHING   ///< Used if no dependency has to be added
		};
	public:
		/// Constructor initializes neg to false
		Literal();
		Literal(const Literal &l);
		/**
		 * \brief Sets the sign of the literal
		 * \param neg The new sign
		 */
		virtual void setNeg(bool neg);
		/**
		 * \brief Retrieve the sign ofthe literal.
		 * \return Returns the sign
		 */
		virtual bool getNeg() const;
		/**
		 * \brief Retrieve the vars of the literal
		 * \param vars Reference to a set used to store the result
		 * \param type Used to filter variables
		 */
		virtual void getVars(VarSet &vars) const = 0;
		/**
		 * \brief Used to check omega restricted parts of literals.
		 *
		 * Some literals like conditionals in symbolic sets require 
		 * omega-restricted parts. This method can be used to ensure this
		 * property. This method is called after preprocess().
		 * \param unsolved Reference to a vector stroring all non-omega-restricted literals
		 * \return Returns true if check passed
		 */
		virtual bool checkO(LiteralVector &unsolved) = 0;
		/**
		 * \brief This method is used for static preprocessing.
		 *
		 * Currently this method is used to remove all terms of the form: a;b.
		 * Additionally new Integrity Rules of the form: :- a, -a. are intoduced if
		 * predicates withe a leading "-" are used.
		 * \param g The grounder
		 * \param e Reference to the object that is preprocessed
		 */
		virtual void preprocess(Grounder *g, Expandable *e, bool head) = 0;
		/**
		 * \brief This method is called to undo a call to finish() and evaluate()
		 */
		virtual void reset() = 0;
		/**
		 * \brief Used to inform the literal that it is finished
		 *
		 * After for example a normal rule is grounded the head literal is finished.
		 * Internally a counter is decremented and if this counter hits zero the literal
		 * is complete or solved.
		 */
		virtual void finish() = 0;
		/**
		 * \brief Used to inform the literal that it is evaluated
		 *
		 * This method method is called after a sub program is grounded.
		 * Example: After a basic program is evaluated this method is called for all
		 * head literals of the rules in the basic program. And then all head literal 
		 * are solved.
		 */
		virtual void evaluate();
		/**
		 * \brief Returns if the literal has a solved domain
		 * \return Returns true if the literal has a solved domain
		 */
		virtual bool solved() = 0;
		/**
		 * \brief Returns if the literal is a fact wrt. the current assignment
		 * \return Returns true if the literal is a fact
		 */
		virtual bool isFact(Grounder *g) = 0;
		/**
		 * \brief Clones the literal
		 * \return Pointer to the new copied literal
		 */
		virtual Literal* clone() const = 0;
		/**
		 * \brief Creates an index on the domain of the literal
		 * \param index Stores a set to the variables that have to be indexed.
		 * \return Returns the new indexed domain
		 */
		virtual IndexedDomain *createIndexedDomain(Grounder *g, VarSet &index) = 0;
		/**
		 * \brief Determines if the literal can be satisfied wrt. the current substitution
		 *
		 * Also local grounding of an aggregate may be startet here.
		 * \param g Reference to the grounder
		 * \return returns true if the literal matches.
		 */
		virtual bool match(Grounder *g) = 0;
		virtual void ground(Grounder *g, GroundStep step);
		/**
		 * \brief Converts the literal into a representation that can be handled by the evaluator/output classes.
		 * \return Returns the representation 
		 */
		virtual NS_OUTPUT::Object *convert() = 0;
		/**
		 * \brief Adds the literal to the dependency graph
		 * \param dg The dependency graph
		 * \param prev The parent node
		 * \param todo The type of dependencies that still have to be added
		 * \return Returns the node that was created if any otherwise zero
		 */
		virtual SDGNode *createNode(SDG *dg, SDGNode *prev, DependencyAdd todo) = 0;
		virtual void createNode(LDGBuilder *dg, bool head) = 0;
		virtual void createNode(StatementChecker *dg, bool head, bool delayed) = 0;
		virtual double heuristicValue() = 0;
		virtual void binderSplit(Expandable *e, const VarSet &relevant);
		virtual void addIncParam(Grounder *g, const Value &v) = 0;

		/// Destructor
 		virtual ~Literal();
	protected:
		/// The sign of the literal
		bool neg_;
	};
}

#endif

