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

#include <gringo/statementdependencygraph.h>
#include <gringo/predicateliteral.h>
#include <gringo/domain.h>
#include <gringo/grounder.h>
#include <gringo/program.h>

using namespace gringo;
		
// ==================== SDG::SCC ============================
SDG::SCC::SCC() : type_(FACT), edges_(0)
{
}

// ======================= SDG ==============================

SDG::SDG() : last_(0)
{
}

SDGNode *SDG::createStatementNode(Statement *r, bool preserveOrder)
{
	SDGNode *n = new SDGNode(r);
	ruleNodes_.push_back(n);
	if(preserveOrder)
	{
		if(last_)
			n->addDependency(last_);
		last_ = n;
	}
	return n;
}

SDGNode *SDG::createPredicateNode(PredicateLiteral *pred)
{
	int uid = pred->getUid();
	for(int i = predicateNodes_.size(); i <= uid; i++)
		predicateNodes_.push_back(new SDGNode(i));
	return predicateNodes_[uid];
}

namespace
{
	struct Call
	{
		Call(SDGNode *v, SDGNodeVector::iterator w) : v(v), root(true), w(w)
		{
		}

		SDGNode *v;
		bool root;
		SDGNodeVector::iterator w;
	};
}

void SDG::tarjan(SDGNode *v, int &index, int &back, std::vector<SDGNode*> &stack)
{
	std::vector<Call> callstack;
	callstack.push_back(Call(v, v->getDependency()->begin()));
	v->index_ = index++;
	while(!callstack.empty())
	{
Start:
		Call &c = callstack.back();
		v = c.v;
		for(; c.w != c.v->getDependency()->end(); c.w++)
		{
			SDGNode *w = *c.w;
			if(w->index_ == 0)
			{
				callstack.push_back(Call(w, w->getDependency()->begin()));
				w->index_ = index++;
				goto Start;
			}
			if(w->index_ < v->index_)
			{
				v->index_ = (*c.w)->index_;
				c.root = false;
			}
		}
		if(c.root)
		{
			int nodes = 1;
			v->scc_ = new SCC();
			if(v->getStatement())
				v->scc_->rules_.push_back(v->getStatement());
			index--;
			while(!stack.empty() && v->index_ <= stack.back()->index_)
			{
				stack.back()->index_ = back;
				stack.back()->scc_ = v->scc_;
				if(stack.back()->getStatement())
					v->scc_->rules_.push_back(stack.back()->getStatement());
				stack.pop_back();
				index--;
				nodes++;
			}
			v->index_ = back;
			back--;
			// initialize with fact or basic program
			v->scc_->type_ = nodes == 1 ? SCC::FACT : SCC::BASIC;
			//scc->type_ = nodes == 1 ? SCC::FACT : SCC::NORMAL;
			sccs_.push_back(v->scc_);
			// calc type and dependency of program
			if(calcSCCDep(v))
				sccRoots_.insert(v->scc_);
		}
		else
		{
			stack.push_back(v);
		}
		callstack.pop_back();
	}
}

bool SDG::calcSCCDep(SDGNode *v)
{
	bool root = true;
	std::vector<SDGNode*> bfs;
	bfs.push_back(v);

	// do a depth first search limited to the scc to build a tree of sccs
	while(!bfs.empty())
	{
		v = bfs.back();
		bfs.pop_back();
		v->done_ = true;
		SDGNodeVector *dep = v->getDependency();
		// build tree of sccs
		for(SDGNodeVector::iterator it = dep->begin(); it != dep->end(); it++)
		{
			SDGNode *w = *it;
			assert(w->scc_);
			if(w->scc_ == v->scc_ && !w->done_)
				bfs.push_back(w);
			if(w->scc_ != v->scc_)
			{
				if(w->scc_->type_ == SCC::NORMAL)
					v->scc_->type_ = SCC::NORMAL;
				if(w->scc_->sccs_.insert(v->scc_).second)
					v->scc_->edges_++;
				root = false;
			}
		}
		// try to find neg dep in scc
		if(v->scc_->type_ != SCC::NORMAL)
		{
			SDGNodeVector *negDep = v->getNegDependency();
			for(SDGNodeVector::iterator it = negDep->begin(); it != negDep->end(); it++)
			{
				SDGNode *w = *it;
				if(w->scc_ == v->scc_)
				{
					v->scc_->type_ = SCC::NORMAL;
					break;
				}
			}
		}
	}
	return root;
}

void SDG::calcSCCs(Grounder *g)
{
	int index = 1, back = ruleNodes_.size() + predicateNodes_.size() - 1;
	std::vector<SDGNode*> stack;
	/*
	std::cout << "rule nodes: " << ruleNodes_.size() << std::endl;
	std::cout << "pred nodes: " << predicateNodes_.size() << std::endl;
	for(SDGNodeVector::iterator i = ruleNodes_.begin(); i != ruleNodes_.end(); i++)
	{
		std::cout << *i << " =>";
		for(SDGNodeVector::iterator j = (*i)->getDependency()->begin(); j != (*i)->getDependency()->end(); j++)
			std::cout << " " << *j;
		std::cout << std::endl;
	}
	for(SDGNodeVector::iterator i = predicateNodes_.begin(); i != predicateNodes_.end(); i++)
	{
		std::cout << *i << " =>";
		for(SDGNodeVector::iterator j = (*i)->getDependency()->begin(); j != (*i)->getDependency()->end(); j++)
			std::cout << " " << *j;
		std::cout << std::endl;
	}
	*/
	for(SDGNodeVector::iterator it = ruleNodes_.begin(); it != ruleNodes_.end(); it++)
	{
		SDGNode *v = *it;
		if(v->index_ == 0)
			tarjan(v, index, back, stack);
	}
	for(SDGNodeVector::iterator it = predicateNodes_.begin(); it != predicateNodes_.end(); it++)
	{
		SDGNode *v = *it;
		if(v->index_ == 0)
			tarjan(v, index, back, stack);
		// set the type of the domain
		g->getDomain(v->getDomain())->setType(static_cast<Domain::Type>(v->scc_->type_));
	}

	std::stack<SCC*> s;

	for(SDGNodeVector::iterator it = predicateNodes_.begin(); it != predicateNodes_.end(); it++)
	{
		if(g->checkIncShift((*it)->getDomain()))
			s.push((*it)->getSCC());
	}

	while(!s.empty())
	{
		SCC *scc = s.top();
		s.pop();
		if(scc->type_ != SCC::NORMAL)
		{
			scc->type_ = SCC::NORMAL;
			for(SCCSet::iterator it = scc->sccs_.begin(); it != scc->sccs_.end(); it++)
				s.push(*it);
		}
	}

	for(SDGNodeVector::iterator it = predicateNodes_.begin(); it != predicateNodes_.end(); it++)
	{
		SDGNode *v = *it;
		if(v->scc_->type_ == SCC::NORMAL)
			g->getDomain(v->getDomain())->setType(static_cast<Domain::Type>(v->scc_->type_));
		else if(v->dependency_.size() == 0)
		{
			if(!g->isIncShift(v->getDomain())) g->addZeroDomain(v->getDomain());
			else g->getDomain(v->getDomain())->setSolved(true);
		}
	}

	// do a topological sort
	std::queue<SCC*> bf;
	for(SCCSet::iterator it = sccRoots_.begin(); it != sccRoots_.end(); it++)
	{
		SCC *scc = *it;
		bf.push(scc);
	}
	while(!bf.empty())
	{
		SCC *top = bf.front();
		assert(top->edges_ == 0);
		bf.pop();
		// if there is something to ground add it to the grounder
		if(top->rules_.size() > 0)
		{
			Program *p = new Program(static_cast<Program::Type>(top->type_), top->rules_);
			//std::cout << pp(g, p) << std::endl;
			g->addProgram(p);
		}
		for(SCCSet::iterator it = top->sccs_.begin(); it != top->sccs_.end(); it++)
		{
			SCC *scc = *it;
			scc->edges_--;
			if(scc->edges_ == 0)
				bf.push(scc);
		}
	}
}

SDG::~SDG()
{
	for(SCCVector::iterator it = sccs_.begin(); it != sccs_.end(); it++)
		delete *it;
	for(SDGNodeVector::iterator it = ruleNodes_.begin(); it != ruleNodes_.end(); it++)
		delete *it;
	for(SDGNodeVector::iterator it = predicateNodes_.begin(); it != predicateNodes_.end(); it++)
		delete *it;
}

// =================================== SDGNode ===========================================

SDGNode::SDGNode(int domain) : index_(0), type_(PREDICATENODE), done_(0), scc_(0), dom_(domain)
{
}

SDGNode::SDGNode(Statement *rule) : index_(0), type_(STATEMENTNODE), done_(0), scc_(0), rule_(rule)
{
}

SDGNodeVector *SDGNode::getNegDependency() const
{
	return const_cast<SDGNodeVector *>(&negDependency_);
}

SDGNodeVector *SDGNode::getDependency() const
{
	return const_cast<SDGNodeVector *>(&dependency_);
}

int SDGNode::getDomain() const
{
	if(type_ == PREDICATENODE)
		return dom_; 
	else
		return -1;
}

Statement* SDGNode::getStatement() const
{ 
	if(type_ == STATEMENTNODE)
		return rule_; 
	else
		return 0;
}

SDGNode::Type SDGNode::getType() const
{
	return static_cast<SDGNode::Type>(type_);
}

void SDGNode::addDependency(SDGNode *n, bool neg)
{
	assert(n);
	if(neg)
		negDependency_.push_back(n);
	dependency_.push_back(n);
}

SDGNode::~SDGNode() 
{
}

