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

#ifndef BASICPROGRAMEVALUATOR_H
#define BASICPROGRAMEVALUATOR_H

#include <gringo/gringo.h>
#include <gringo/evaluator.h>
#include <gringo/output.h>

namespace gringo
{
	/**
	 * \brief Evaluator backend which calculates the single answer set for a basic program
	 */
	class BasicProgramEvaluator : public Evaluator
	{
	private:
		/// Type used to assign an id to a (variable free) predicate
		typedef HashMap<ValueVector, int, Value::VectorHash, Value::VectorEqual>::type AtomHash;
		/// Type used to store the ids to all possible predicates
		typedef std::vector<AtomHash> AtomLookUp;
		/// Used to store the atoms
		enum Status { NONE = 0, FACT = 1, QUEUED = 2};
		struct AtomNode
		{
			AtomNode(Domain *node);

			Status status_;
			Domain *node_;
			IntVector inBody_;
		};
		typedef std::vector<std::pair<int, int> > Rules;
		typedef std::vector<AtomNode> Atoms;
	public:
		/// Constructor
		BasicProgramEvaluator();
		void initialize(Grounder *g);
		void add(NS_OUTPUT::Object *r);
		void evaluate();
		void propagate(int uid);
		/// Destructor
		~BasicProgramEvaluator();
	private:
		/**
		 * \brief Helper function used to add atoms
		 * This function returns the id of the atom. If a new atoms is added 
		 * a new id is assigned.
		 *
		 * \param r The atom
		 * \return Returns the id of the atom
		 */
		int add(NS_OUTPUT::Atom *r);
		/**
		 * \brief Helper function to add facts.
		 * \param r The fact
		 */
		void add(NS_OUTPUT::Fact *r);
		/**
		 * \brief Helper function to add basic rules.
		 * \param r The rule
		 */
		void add(NS_OUTPUT::Rule *r);
		/**
		 * \brief Helper function to add bodies.
		 * \param r The body
		 */
		void add(NS_OUTPUT::Conjunction *r);
	private:
		/// Stores the ids of all atoms (predicates)
		AtomLookUp atomHash_;
		/// Stores all atoms
		Atoms      atoms_;
		/// Stores the basic program
		Rules      rules_;
	};
}

#endif

