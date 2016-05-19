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

#ifndef NORMALRULE_H
#define NORMALRULE_H

#include <gringo/gringo.h>
#include <gringo/statement.h>
#include <gringo/expandable.h>

namespace gringo
{
	/// This class is used to represent normal rules including integrity constraints and facts
	class NormalRule : public Statement, public Expandable
	{
	public:
		/**
		 * \brief Constructor
		 * \param head The head of the rule
		 * \param body The body of the rule
		 */
		NormalRule(Literal *head, LiteralVector *body);
		/// Destructor
		virtual ~NormalRule();
		// implemented from base class or interface
		virtual void getVars(VarSet &vars) const;
		virtual void buildDepGraph(SDG *dg);
		virtual void print(const GlobalStorage *g, std::ostream &out) const;
		virtual bool checkO(LiteralVector &unsolved);
		virtual bool check(Grounder *g, VarVector &free);
		virtual void reset();
		virtual void preprocess(Grounder *g);
		virtual void appendLiteral(Literal *l, ExpansionType type);
		virtual void finish();
		virtual void evaluate();
		virtual void grounded(Grounder *g);
		virtual bool ground(Grounder *g, GroundStep step);
		virtual void addDomain(PredicateLiteral *pl);
		virtual void setIncPart(Grounder *g, IncPart part, const Value &v);
	private:
		/**
		 * \brief Calculate the relevant vars in the body
		 * \param relevant The resulting set of relevant vars
		 */
		void getRelevantVars(const VarSet &global, VarSet &relevant);
		void getRelevantVars(const VarSet &global, VarVector &relevant);
	public:
		/// The haed
		Literal *head_;
		/// The body
		LiteralVector *body_;
		DLVGrounder *grounder_;
		struct
		{
			unsigned int ground_   : 1;
			unsigned int base_     : 1;
			unsigned int last_     : 1;
			unsigned int lambda_   : 1;
			unsigned int delta_    : 1;
			unsigned int isGround_ : 1;
		};
	};
}

#endif

