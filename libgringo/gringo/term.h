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

#ifndef TERM_H
#define TERM_H

#include <gringo/gringo.h>
#include <gringo/printable.h>

namespace gringo
{
	/**
	 * \brief Base class of all Terms
	 *
	 * Terms are variables, strings or functions. They are used by most literals. 
	 * During grounding every term will be evaluated to a value.
	 */
	class Term : public Printable
	{
	public:
		/**
		 * \brief Constructor - does nothing
		 */
		Term();
		/**
		 * \brief Retrieve all Variables in term
		 * \param vars stores the result
		 */
		virtual void getVars(VarSet &vars) const = 0;
		/**
		 * \brief Return true if the Term is complex
		 *
		 * Everything except variables, ids, strings has to be complex
		 * \return Returns true if the term is complex
		 */
		virtual bool isComplex() = 0;
		/**
		 * \brief Returns the Value of the term under the current substitution
		 * \return The value
		 */
		virtual Value getValue(Grounder *g) = 0;
		/**
		 * \brief Returns the Value of the term
		 * There must be no variables, rangeterms or multipleargsterms in the term. 
		 * Ids are replaced by their corresponding const value, stored in the grounder.
		 * \return The value
		 */
		virtual Value getConstValue(Grounder *g) = 0;
		/**
		 * \brief Creates a copy of the term
		 * \return The copy
		 */
		virtual Term* clone() const = 0;
		/**
		 * \brief Static preprocess of the term
		 *
		 * Currently rangeterms are removed and replaced by rangeliterals.
		 * \param p Reference to the place where the current term is stored
		 * \param g The grounder
		 * \param e Expandable object
		 */
		virtual void preprocess(Literal *l, Term *&p, Grounder *g, Expandable *e) = 0;
		/**
		 * \brief Unifies the term with the value
		 *
		 * \param t The Value the term is unified with
		 * \param boundVariables The bound variables
		 * \param freeVariables The non bound variables
		 * \param boundSubstitutions OutputVector for all substitutions of the bound variables
		 * boundSubstitution[i] is the substitution for variable boundVariables[i]
		 * \param freeSubstitutions Output map for all substitutions of non bound variables
		 * freeSubstitution[i] is the substitution for variable freeVariables[i]
		 *
		 *
		 * \return true if the term unifies with t, otherwise false
		 */
		virtual bool unify(const GlobalStorage *g, const Value& t, const VarVector& vars, ValueVector& subst) const = 0;
		virtual void addIncParam(Grounder *g, Term *&p, const Value &v) = 0;
		/**
		 * \brief Virtual Destructor
		 */
		virtual ~Term();
	};
}

#endif

