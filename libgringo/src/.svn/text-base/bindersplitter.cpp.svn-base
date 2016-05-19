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

#include <gringo/bindersplitter.h>
#include <gringo/literaldependencygraph.h>
#include <gringo/grounder.h>
#include <gringo/indexeddomain.h>
#include <gringo/dlvgrounder.h>
#include <gringo/value.h>
#include <gringo/output.h>
#include <gringo/term.h>
#include <gringo/domain.h>

using namespace gringo;
		
BinderSplitter::BinderSplitter(Domain *domain, TermVector *param, const VarVector &relevant) : domain_(domain), param_(param), relevant_(relevant)
{
}

bool BinderSplitter::checkO(LiteralVector &unsolved)
{
	FAIL(true);
}

SDGNode *BinderSplitter::createNode(SDG *dg, SDGNode *prev, DependencyAdd todo)
{
	FAIL(true);
}

void BinderSplitter::createNode(LDGBuilder *dg, bool head)
{
	VarSet needed, provided(relevant_.begin(), relevant_.end());
	dg->createNode(this, head, needed, provided);
}

void BinderSplitter::createNode(StatementChecker *dg, bool head, bool delayed)
{
	FAIL(true);
}

void BinderSplitter::print(const GlobalStorage *g, std::ostream &out) const
{
	out << "true";
}

void BinderSplitter::reset()
{
	FAIL(true);
}

void BinderSplitter::finish()
{
	FAIL(true);
}

void BinderSplitter::evaluate()
{
	FAIL(true);
}

bool BinderSplitter::isFact(Grounder *g)
{
	return true;
}

bool BinderSplitter::solved()
{
	return true;
}

bool BinderSplitter::match(Grounder *g)
{
	// theres always a constructed indexed domain
	FAIL(true);
}

NS_OUTPUT::Object *BinderSplitter::convert()
{
	FAIL(true);
}

namespace gringo
{
	namespace
	{
		/*
		std::string print(const ValueVector &v)
		{
			std::stringstream ss;
			bool comma = false;
			for(ValueVector::const_iterator i = v.begin(); i != v.end(); i++)
			{
				if(!comma)
					comma = true;
				else
					ss << ", ";
				ss << *i;
			}
			return ss.str();
		}
		*/

		class IndexedDomainBS : public IndexedDomain
		{
		private:
			typedef HashMap<ValueVector, ValueVector, Value::VectorHash, Value::VectorEqual>::type ValueVectorMap;
		public:
			IndexedDomainBS(Grounder *g, ValueVectorSet &domain, VarSet &index, TermVector &param, VarVector &relevant)
			{
				VarVector unrelevant;
				for(int i = 0; i < (int)param.size(); i++)
				{
					VarSet variables;
					param[i]->getVars(variables);
					for (VarSet::const_iterator j = variables.begin(); j != variables.end(); ++j)
					{
						if(std::binary_search(relevant.begin(), relevant.end(), *j))
						{
							if (index.find(*j) != index.end())
								//add the UID of bound variables
								index_.push_back(*j);
							else
								//and unbound variables
								bind_.push_back(*j);
						}
						else
						{
							unrelevant.push_back(*j);
						}
					}
				}

				//make unique
				sort(index_.begin(), index_.end());
				index_.erase(std::unique(index_.begin(), index_.end()), index_.end());

				sort(bind_.begin(), bind_.end());
				bind_.erase(std::unique(bind_.begin(), bind_.end()), bind_.end());

				sort(unrelevant.begin(), unrelevant.end());
				unrelevant.erase(std::unique(unrelevant.begin(), unrelevant.end()), unrelevant.end());

				VarVector unifyVars;
				unifyVars.insert(unifyVars.end(), bind_.begin(), bind_.end());
				unifyVars.insert(unifyVars.end(), index_.begin(), index_.end());
				unifyVars.insert(unifyVars.end(), unrelevant.begin(), unrelevant.end());

				ValueVectorSet domainsBS;
				for(ValueVectorSet::iterator it = domain.begin(); it != domain.end(); it++)
				{
					const ValueVector &val = (*it);
					ValueVector unifyVals(unifyVars.size());
					bool doContinue = false;

					assert(param.size() == val.size());
					TermVector::const_iterator p = param.begin();
					for (ValueVector::const_iterator i = val.begin(); i != val.end(); ++i, ++p)
					{
						if (!(*p)->unify(g, *i, unifyVars, unifyVals))
						{
							doContinue = true;
							break;
						}
					}

					if(doContinue || !domainsBS.insert(ValueVector(unifyVals.begin(), unifyVals.end() - unrelevant.size())).second)
						continue;
					
					//die indexedDomain mit dem Index aller einer Instanz aller gebundenen Variablen ist gleich der Instanz aus der Domain
					// (mehrere Instanzen)
					ValueVector &v = domain_[ValueVector(unifyVals.begin() + bind_.size(), unifyVals.end() - unrelevant.size())];
					v.insert(v.end(), unifyVals.begin(), unifyVals.begin() + bind_.size());
				}
			}

			void firstMatch(int binder, DLVGrounder *g, MatchStatus &status)
			{

				currentIndex_.clear();
				for (VarVector::const_iterator i = index_.begin(); i != index_.end(); ++i)
				{
					currentIndex_.push_back(g->g_->getValue(*i));
				}
				ValueVectorMap::iterator it = domain_.find(currentIndex_);
				if(it != domain_.end())
				{
					current_ = it->second.begin();
					end_     = it->second.end();
					for(unsigned int i = 0; i < bind_.size(); ++i)
					{
						assert(current_ != end_);
						// setze freie Variable X(bind_[i]) auf currentDomain[1 >1< 2], weil X an stelle 2(i) ist
						g->g_->setValue(bind_[i], (*current_), binder);
						//std::cerr << "setting(" << bind_[i] << ") " << *g->g_->getVarString(bind_[i]) << " = " << *current_ << std::endl;
						++current_;
					}
					status = SuccessfulMatch;
				}
				else
					status = FailureOnFirstMatch;
			}

			void nextMatch(int binder, DLVGrounder *g, MatchStatus &status)
			{
				if(current_ != end_)
				{
					for(unsigned int i = 0; i < bind_.size(); ++i)
					{
						assert(current_ != end_);
						// setze freie Variable X(it->second) auf currentDomain[1 >1< 2], weil X an stelle 2 ist
						g->g_->setValue(bind_[i], (*current_), binder);
						//std::cerr << "setting " << *g->g_->getVarString(bind_[i]) << " = " << *current_ << std::endl;
						++current_;
					}
					status = SuccessfulMatch;
				}
				else
					status = FailureOnNextMatch;
			}

			~IndexedDomainBS()
			{
			}
		private:
			ValueVector currentIndex_;
			ValueVectorMap domain_;
			VarVector bind_;
			VarVector index_;
			ValueVector::iterator current_, end_;
		};
	}
}

IndexedDomain *BinderSplitter::createIndexedDomain(Grounder *g, VarSet &index)
{
	return new IndexedDomainBS(g, domain_->getDomain(), index, *param_, relevant_);
}

Literal* BinderSplitter::clone() const
{
	FAIL(true);
}

BinderSplitter::~BinderSplitter()
{
}

void  BinderSplitter::addIncParam(Grounder *g, const Value &v)
{
	FAIL(true);
}

void BinderSplitter::preprocess(Grounder *g, Expandable *e, bool head)
{
	FAIL(true);
}

double BinderSplitter::heuristicValue()
{
	// this selects splitted domains as soon as possible 
	// dont know if this is good but it is simple :)
	return 0;
}

void BinderSplitter::getVars(VarSet &vars) const
{
	vars.insert(relevant_.begin(), relevant_.end());
}

