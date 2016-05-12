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

#include <iomanip>
#include <gringo/stats.h>
#include <gringo/rule.h>
#include <gringo/optimize.h>
#include <gringo/rellit.h>
#include <gringo/sumaggrlit.h>
#include <gringo/minmaxaggrlit.h>
#include <gringo/term.h>
#include <gringo/varterm.h>

Stats::Stats()
	: numStm_(0), numStmRules_(0), numStmFacts_(0), numStmConstraints_(0), numStmOptimize_(0)
	, numAggr_(0), numAggrCount_(0), numAggrSum_(0), numAggrAvg_(0), numAggrEven_(0), numAggrMinmax_(0)
	, numLit_(0), numLitRel_(0)
	, numTerm_(0), numTermVar_(0), numTermRestr_(0), numTermConst_(0)
	, numScc(0), numSccNonTrivial(0)
	, numPred(0), numPredVisible(0), avgPredParams(0.0), numPredPos_(0), numPredNeg_(0), numPredAggr_(0)
	, integrityConstraint_(false), inAggregate_(false)
{
}

void Stats::visit(Statement *stm)
{
	numStm_ ++;
	integrityConstraint_ = true;
	stm->visit(this);

	if(dynamic_cast<Rule*>(stm))
	{
		if(integrityConstraint_) numStmConstraints_ ++;
		else numStmRules_ ++;
	}
	else if(dynamic_cast<Optimize*>(stm))
	{
		numStmOptimize_ ++;
	}
}

void Stats::visit(VarTerm *var, bool bind)
{
	(void)var;
	(void)bind;
}

void Stats::visit(Term* term, bool bind)
{
	(void)bind;
	numTerm_ ++;
	if(dynamic_cast<VarTerm*>(term)) numTermVar_ ++;
	else if(term->constant()) numTermConst_ ++;
	else numTermRestr_ ++;
}

void Stats::visit(PredLit *pred)
{
	if(pred->head()) integrityConstraint_ = false;
	else
	{
		if(inAggregate_) numPredAggr_ ++;
		if(pred->sign()) numPredNeg_ ++;
		else numPredPos_ ++;
	}
}

void Stats::visit(Lit *lit, bool domain)
{
	(void)domain;
	if(dynamic_cast<WeightLit*>(lit)) return;
	if(!lit->head()) numLit_ ++;
	if(dynamic_cast<AggrLit*>(lit))
	{
		numAggr_ ++;
		if(dynamic_cast<SumAggrLit*>(lit))
		{
			if(dynamic_cast<SumAggrLit*>(lit)->set()) numAggrCount_ ++;
			else numAggrSum_ ++;
		}
		else if(dynamic_cast<MinMaxAggrLit*>(lit)) numAggrMinmax_ ++;

		inAggregate_ = true;
		lit->visit(this);
		inAggregate_ = false;
	}
	else
	{
		if(dynamic_cast<RelLit*>(lit)) numLitRel_ ++;
		lit->visit(this);
	}
}

void Stats::visit(Groundable *grd, bool choice)
{
	(void)choice;
	grd->visit(this);
}

void Stats::print(std::ostream &out)
{
	out << std::setiosflags(std::ios::right) << std::resetiosflags(std::ios::left);
	out << "=== Grounder Input Statistics ===" << std::endl;

	// dependency graph
	out << "components              : " << std::setw(5) << numScc << std::endl;
	out << " non-trivial            : " << std::setw(5) << numSccNonTrivial << std::endl << std::endl;

	// predicates
	out << "predicates              : " << std::setw(5) << numPred << std::endl;
	out << " visible                : " << std::setw(5) << numPredVisible << std::endl;
	out << " average parameters     : " << std::setw(5) << std::setprecision(2)  << avgPredParams << std::endl << std::endl;

	// statements
	out << "statements              : " << std::setw(5) << numStm_ << std::endl;
	out << " rules                  : " << std::setw(5) << numStmRules_ << std::endl;
	out << " facts                  : " << std::setw(5) << numStmFacts_ << std::endl;
	out << " constraints            : " << std::setw(5) << numStmConstraints_ << std::endl;
	out << " optimize               : " << std::setw(5) << numStmOptimize_ << std::endl << std::endl;

	// body literals
	out << "body literals           : " << std::setw(5) << numLit_ << std::endl;
	out << " literals in aggregates : " << std::setw(5) << numPredAggr_ << std::endl;
	out << " relations              : " << std::setw(5) << numLitRel_ << std::endl;
	out << " positive predicates    : " << std::setw(5) << numPredPos_ << std::endl;
	out << " negative predicates    : " << std::setw(5) << numPredNeg_ << std::endl << std::endl;

	// aggregates
	out << "aggregates              : " << std::setw(5) << numAggr_ << std::endl;
	out << " count                  : " << std::setw(5) << numAggrCount_ << std::endl;
	out << " sum                    : " << std::setw(5) << numAggrSum_ << std::endl;
	out << " avg                    : " << std::setw(5) << numAggrAvg_ << std::endl;
	out << " even/odd               : " << std::setw(5) << numAggrEven_ << std::endl;
	out << " min/max                : " << std::setw(5) << numAggrMinmax_ << std::endl << std::endl;

	// terms
	out << "terms                   : " << std::setw(5) << numTerm_ << std::endl;
	out << " variable terms         : " << std::setw(5) << numTermVar_ << std::endl;
	out << " restricted terms       : " << std::setw(5) << numTermRestr_ << std::endl;
	out << " constant terms         : " << std::setw(5) << numTermConst_ << std::endl << std::endl;

}
