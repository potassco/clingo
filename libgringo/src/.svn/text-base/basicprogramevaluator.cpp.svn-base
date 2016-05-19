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

#include <gringo/basicprogramevaluator.h>
#include <gringo/grounder.h>
#include <gringo/domain.h>
#include <gringo/output.h>

using namespace gringo;

BasicProgramEvaluator::AtomNode::AtomNode(Domain *node) : status_(NONE), node_(node)
{
}

BasicProgramEvaluator::BasicProgramEvaluator() : Evaluator()
{
}

void BasicProgramEvaluator::initialize(Grounder *g)
{
	//std::cerr << "starting basic program" << std::endl;
	Evaluator::initialize(g);
	atomHash_.resize(g->getPred()->size());
}

int BasicProgramEvaluator::add(NS_OUTPUT::Atom *r)
{
	assert(!r->neg_);
	int id = r->predUid_;
	AtomHash::iterator res = atomHash_[id].find(r->values_);
	if(res == atomHash_[id].end())
	{
		int uid = atoms_.size();
		atomHash_[id].insert(std::make_pair(r->values_, uid));
		atoms_.push_back(AtomNode(r->node_));
		return uid;
	}
	else
		return res->second;
}

void BasicProgramEvaluator::add(NS_OUTPUT::Fact *r)
{
	assert(dynamic_cast<NS_OUTPUT::Atom*>(r->head_));
	int uid = add(static_cast<NS_OUTPUT::Atom*>(r->head_));
	propagate(uid);
}

void BasicProgramEvaluator::propagate(int uid)
{
	// its used like a stack but who cares :)
	std::vector<int> queue;
	if(atoms_[uid].status_ == NONE)
		queue.push_back(uid);
	while(queue.size() > 0)
	{
		uid = queue.back();
		queue.pop_back();
		atoms_[uid].status_ = FACT;
		for(IntVector::const_iterator it = atoms_[uid].inBody_.begin(); it != atoms_[uid].inBody_.end(); it++)
		{
			
			rules_[*it].second--;
			if(rules_[*it].second == 0 && atoms_[rules_[*it].first].status_ == NONE)
			{
				atoms_[rules_[*it].first].status_ = QUEUED;
				queue.push_back(rules_[*it].first);
			}
		}
		atoms_[uid].inBody_.clear();
	}
}

void BasicProgramEvaluator::add(NS_OUTPUT::Rule *r)
{
	assert(dynamic_cast<NS_OUTPUT::Atom*>(r->head_));
	assert(dynamic_cast<NS_OUTPUT::Conjunction*>(r->body_));
	int uid = add(static_cast<NS_OUTPUT::Atom*>(r->head_));
	if(atoms_[uid].status_ == NONE)
	{
		int ruleId  = rules_.size();
		int numLits = 0;
		NS_OUTPUT::ObjectVector &lits = static_cast<NS_OUTPUT::Conjunction*>(r->body_)->lits_;
		assert(lits.size() > 0);
		for(NS_OUTPUT::ObjectVector::iterator it = lits.begin(); it != lits.end(); it++)
		{
			assert(dynamic_cast<NS_OUTPUT::Atom*>(*it));
			int bodyUid = add(static_cast<NS_OUTPUT::Atom*>(*it));
			if(atoms_[bodyUid].status_ == NONE)
			{
				atoms_[bodyUid].inBody_.push_back(ruleId);
				numLits++;
			}
		}
		rules_.push_back(std::make_pair(uid, numLits));
		if(numLits == 0)
			propagate(uid);
	}
}

void BasicProgramEvaluator::add(NS_OUTPUT::Object *r)
{
	// make shure r is deleted even if an exception is thrown
	std::auto_ptr<NS_OUTPUT::Object> f(r);
	if(dynamic_cast<NS_OUTPUT::Rule*>(r))
	{
		add(static_cast<NS_OUTPUT::Rule*>(r));
	}
	else if(dynamic_cast<NS_OUTPUT::Fact*>(r))
	{
		add(static_cast<NS_OUTPUT::Fact*>(r));
	}
	else
		FAIL(true);
	// for grounding it is essentiell to add the domains
	r->addDomain();
}

void BasicProgramEvaluator::evaluate()
{
	//std::cerr << "evaluating basic program" << std::endl;

	for(int uid = 0; uid < (int)atomHash_.size(); uid++)
	{
		AtomHash &hash = atomHash_[uid];
		for(AtomHash::iterator it = hash.begin(); it != hash.end(); it++)
		{
			if(atoms_[it->second].status_ == FACT)
			{
				NS_OUTPUT::Atom *a = new NS_OUTPUT::Atom(false, atoms_[it->second].node_, uid, it->first);
				NS_OUTPUT::Fact f(a);
#ifdef WITH_ICLINGO
				// keep the facts for later
				f.addDomain(true);
#else
				// we dont need to add the domain as fact
				f.addDomain(false);
#endif
				o_->print(&f);
			}
			else
				// remove all atoms that couldnt be derived from the domain
				atoms_[it->second].node_->removeDomain(it->first);
		}
	}	
	// cleanup the stl constructs :)
	{
		AtomLookUp a;
		std::swap(atomHash_, a);
		Rules r;
		std::swap(rules_, r);
		Atoms f;
		std::swap(atoms_, f);
	}
	//std::cerr << "evaluated basic program" << std::endl;
}

BasicProgramEvaluator::~BasicProgramEvaluator()
{
}

