// Copyright (c) 2010, Arne König <arkoenig@uni-potsdam.de>
//
// This file is part of gringo.
//
// gringo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with gringo.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <gringo/gringo.h>
#include <gringo/prgvisitor.h>

class Stats : public PrgVisitor
{
private:
	//! Number of statements in the program
	size_t numStm_;
	//! Number of rules (sans facts and integrity constraints)
	size_t numStmRules_;
	//! Number of facts
	size_t numStmFacts_;
	//! Number of integrity constraints
	size_t numStmConstraints_;
	//! Number of optimize (minimize/maximize) statements
	size_t numStmOptimize_;

	//! Number of aggregates
	size_t numAggr_;
	//! Number of #count aggregates
	size_t numAggrCount_;
	//! Number of #sum aggregates
	size_t numAggrSum_;
	//! Number of #avg aggregates
	size_t numAggrAvg_;
	//! Number of #even and #odd aggregates
	size_t numAggrEven_;
	//! Number of #min and #max aggregates
	size_t numAggrMinmax_;

	//! Number of literals in the program
	size_t numLit_;
	//! Number of relational literals
	size_t numLitRel_;

	//! Number of terms
	size_t numTerm_;
	//! Number of variables
	size_t numTermVar_;
	//! Number of restricted terms (including a variables)
	size_t numTermRestr_;
	//! Number of const terms
	size_t numTermConst_;

public: // added by grounder
	//! Number of SCCs
	size_t numScc;
	//! Number of non-trivial SCCs (|SCC| > 1)
	size_t numSccNonTrivial;
	//! Number of predicates
	size_t numPred;
	//! Number of visible predicates
	size_t numPredVisible;
	//! Average number of parameters per predicate
	float avgPredParams;

private:
	//! Number of positive predicate literals
	size_t numPredPos_;
	//! Number of negative predicate literals
	size_t numPredNeg_;
	//! Number of predicate literals in aggregates
	size_t numPredAggr_;

public:
	Stats();
	//! Adds a fact to the counter
	void addFact() { numStmFacts_ ++; numStm_ ++; }
	//! Prints statistics to the supplied stream
	void print(std::ostream &out);

	// visitor pattern
	void visit(VarTerm *var, bool bind);
	void visit(Term* term, bool bind);
	void visit(PredLit *lit);
	void visit(Lit *lit, bool domain);
	void visit(Groundable *grd, bool choice);
	void visit(Statement *stm);

private: // visitor state
	//! whether current rule is an integrity constraint
	bool integrityConstraint_;
	//! whether visitor is in an aggregate
	bool inAggregate_;

};
