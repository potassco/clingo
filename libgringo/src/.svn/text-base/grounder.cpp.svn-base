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

#include <gringo/grounder.h>
#include <gringo/statement.h>
#include <gringo/variable.h>
#include <gringo/predicateliteral.h>
#include <gringo/normalrule.h>
#include <gringo/statementdependencygraph.h>
#include <gringo/program.h>
#include <gringo/value.h>
#include <gringo/evaluator.h>
#include <gringo/domain.h>
#include <gringo/gringoexception.h>

using namespace gringo;

Grounder::Grounder(const Options &opts) : opts_(opts), incremental_(false), incStep_(0), internalVars_(0), output_(0), eval_(0)
{
}

void Grounder::setOutput(NS_OUTPUT::Output *output)
{
	output_ = output;
}

void Grounder::addStatement(Statement *rule)
{
	if(incParts_.size() > 0)
		incParts_.back().second++;
	rules_.push_back(rule);
}

void Grounder::addDomains(int id, std::vector<IntVector*>::iterator pos, std::vector<IntVector*>::iterator end, IntVector &list)
{
	if(pos == end)
	{
		domains_.push_back(DomainPredicate(id, new IntVector(list)));
	}
	else
	{
		for(IntVector::iterator it = (*pos)->begin(); it != (*pos)->end(); it++)
		{
			list.push_back(*it);
			addDomains(id, pos + 1, end, list);
			list.pop_back();
		}
	}
}

void Grounder::addDomains(int id, std::vector<IntVector*>* list)
{
	IntVector empty;
	addDomains(id, list->begin(), list->end(), empty);
	for(std::vector<IntVector*>::iterator i = list->begin(); i != list->end(); ++i)
		delete (*i);
	delete list;
}

void Grounder::setIncShift(const std::string &id, int arity)
{
	shifts_[createPred(createString(id), arity)] = std::make_pair(true, createPred(createString("#next_" + id), arity));
}

void Grounder::setIncShift(const std::string &a, const std::string &b, int arity)
{
	shifts_[createPred(createString(a), arity)] = std::make_pair(false, createPred(createString(b), arity));
	invShifts_[createPred(createString(b), arity)] = createPred(createString(a), arity);
}

std::pair<int,int> Grounder::getIncShift(int uid, bool head) const
{
	IncShifts::const_iterator res = shifts_.find(uid);
	if(head && res != shifts_.end() && res->second.first)
		return std::make_pair(res->second.second, uid);
	else
	{
		InvShifts::const_iterator res = invShifts_.find(uid);
		if(res != invShifts_.end())
			return std::make_pair(uid, res->second);
	}
	return std::make_pair(uid, uid);
}

bool Grounder::isIncShift(int uid) const
{
	return shifts_.find(uid) != shifts_.end();
}

bool Grounder::checkIncShift(int uid) const
{
	if(getDomain(uid)->getType() == Domain::NORMAL) return false;
	IncShifts::const_iterator res = shifts_.find(uid);
	if(res == shifts_.end()) return false;
	else return getDomain(res->second.second)->getType() == Domain::NORMAL;
}

void Grounder::buildDepGraph()
{
	SDG dg;
	for(StatementVector::iterator it = rules_.begin(); it != rules_.end(); it++)
		(*it)->buildDepGraph(&dg);
	reset();
	dg.calcSCCs(this);
}

void Grounder::check()
{
	for(ProgramVector::iterator it = sccs_.begin(); it != sccs_.end(); it++)
		if(!(*it)->check(this))
			throw GrinGoException("Error: the program is not groundable.");
	substitution_.resize(varMap_.size() + 2);
	binder_.resize(varMap_.size() + 2, -1);
}

void Grounder::addDomains()
{
	for(StatementVector::iterator it = rules_.begin(); it != rules_.end(); it++)
	{
		Statement *rule = *it;
		VarSet vars;
		rule->getVars(vars);
		for(DomainPredicateVector::iterator it = domains_.begin(); it != domains_.end(); it++)
		{
			DomainPredicate &dp = *it;
			for(IntVector::iterator it = dp.second->begin(); it != dp.second->end(); it++)
			{
				if(vars.find(getVar(*it)) != vars.end())
				{
					// construct predicate literal
					TermVector *tv = new TermVector();
					for(IntVector::iterator vars = dp.second->begin(); vars != dp.second->end(); vars++)
						tv->push_back(new Variable(this, *vars));
					PredicateLiteral *pred = new PredicateLiteral(this, dp.first, tv);
					rule->addDomain(pred);
					break;
				}
			}
		}
	}
}

void Grounder::addZeroDomain(int uid)
{
	Domain *d = getDomain(uid);
	d->setSolved(true);
	Signature &sig = (*getPred())[uid];
	std::cerr << "Warning: " << *getString(sig.first) << "/" << sig.second << " is never defined." << std::endl;
}

void Grounder::reset()
{
	for(StatementVector::iterator it = rules_.begin(); it != rules_.end(); it++)
		(*it)->reset();
}

void Grounder::preprocess()
{
	StatementVector::iterator r = rules_.begin();
	for(IncParts::iterator it = incParts_.begin(); it != incParts_.end(); it++)
		for(int j = 0; j < it->second; j++, r++)
			(*r)->setIncPart(this, it->first.first, it->first.second);

	// the size of rules_ may increase during preprocessing make shure the newly inserted rules are preprocessed too
	for(size_t i = 0; i < rules_.size(); i++)
		rules_[i]->preprocess(this);
}

int Grounder::getIncStep() const
{
	return incStep_;
}

bool Grounder::isIncGrounding() const
{
	return incremental_;
}

void Grounder::prepare(bool incremental)
{
	incremental_ = incremental;

	if(incremental_ && incParts_.size() == 0)
	{
		incParts_.push_back(make_pair(std::make_pair(BASE, Value()), rules_.size()));
		std::cerr << "Warning: There are no #base, #cumulative or #volatile sections." << std::endl;
	}

	preprocess();
	addDomains();
	buildDepGraph();
	check();

	substitution_.resize(varMap_.size() + 2);
	binder_.resize(varMap_.size() + 2, -1);
}

void Grounder::ground()
{
	if(incremental_)
	{
		if(incStep_ == 0)
			output_->initialize(this, getPred());
		else
			output_->reinitialize();
		for(; incStep_ + 1 <= options().iquery; incStep_++)
		{
			reset();
			ground_();
		}
		reset();
		ground_();
		incStep_++;
		output_->finalize(false);
	}
	else
	{
		output_->initialize(this, getPred());
		if(incParts_.size() > 0 && opts_.ifixed < 0 && !opts_.ibase)
			throw GrinGoException("Error: A fixed number of incremental steps is needed to ground the program.");
		if(incParts_.size() > 0 && !opts_.ibase)
		{
			do
			{
				reset();
				ground_();
				incStep_++;
			}
			while(incStep_ <= opts_.ifixed);
		}
		else
		{
			reset();
			ground_();
		}
		output_->finalize(true);
	}
}

void Grounder::setIncPart(IncPart part, const Value &v)
{
	if(incParts_.size() == 0 && rules_.size() > 0)
	{
		incParts_.push_back(make_pair(std::make_pair(BASE, Value()), rules_.size()));
		std::cerr << "Warning: There are statements not within a #base, #cumulative or #volatile section." << std::endl;
		std::cerr << "         These Statements are put into the #base section." << std::endl;
	}
	incParts_.push_back(make_pair(std::make_pair(part, v), 0));
}


void Grounder::addProgram(Program *scc)
{
	sccs_.push_back(scc);
}

void Grounder::ground_()
{
	
	for(ProgramVector::iterator it = sccs_.begin(); it != sccs_.end(); it++)
	{
		Program *scc = *it;
		//std::cerr << pp(this, scc) << std::endl;
		if(opts_.debug)
		{
			switch(scc->getType())
			{
				case Program::FACT:
					std::cerr << "grounding fact program..." << std::endl;
					break;
				case Program::BASIC:
					std::cerr << "grounding basic program..." << std::endl;
					break;
				case Program::NORMAL:
					std::cerr << "grounding normal program..." << std::endl;
					break;
			}
		}
		eval_ = scc->getEvaluator();
		eval_->initialize(this);
		StatementVector *rules = scc->getStatements();
		for(StatementVector::iterator it = rules->begin(); it !=rules->end(); it++)
		{
			Statement *rule = *it;
			if(incStep_ == 0 || !incremental_)
				rule->ground(this, PREPARE);
			else
				rule->ground(this, REINIT);

			if(opts_.debug)
				std::cerr << "  " << pp(this, rule) << std::endl;

			rule->ground(this, GROUND);
			rule->ground(this, RELEASE);
			rule->finish();
		}
		// this solves the domains
		for(StatementVector::iterator it = rules->begin(); it !=rules->end(); it++)
		{
			Statement *rule = *it;
			rule->evaluate();
		}
		eval_->evaluate();
		eval_ = 0;
	}

	for(IncShifts::iterator it = shifts_.begin(); it != shifts_.end(); it++)
	{
		Domain *next = getDomain(it->second.second);
		Domain *before = getDomain(it->first);
		before->moveDomain(next);
	}

}

int Grounder::createUniqueVar()
{
	int uid;
	do
	{
		std::stringstream ss;
		ss << "I_" << internalVars_++;
		uid = createString(ss.str());
	}
	while(varMap_.find(uid) != varMap_.end());
	return uid;
}

const std::string *Grounder::getVarString(int uid)
{
	// inefficient but we need it only for error messages
	for(VariableMap::iterator it = varMap_.begin(); it != varMap_.end(); it++)
		if(it->second == uid)
			return getString(it->first);
	// we should get a string for every variable
	FAIL(true);
}

int Grounder::getVar(int var)
{
	VariableMap::iterator it = varMap_.find(var);
	if(it != varMap_.end())
		return it->second;
	else
		return 0;
}

int Grounder::registerVar(int var)
{
	int &uid = varMap_[var];
	if(uid == 0)
		uid = varMap_.size();
	return uid;
}

Grounder::~Grounder()
{
	for(DomainPredicateVector::iterator it = domains_.begin(); it != domains_.end(); it++)
		delete (*it).second;
	for(StatementVector::iterator it = rules_.begin(); it != rules_.end(); it++)
		delete *it;
	for(ConstTerms::iterator it = constTerms_.begin(); it != constTerms_.end(); it++)
		delete it->second.second;
	for(ProgramVector::iterator it = sccs_.begin(); it != sccs_.end(); it++)
		delete *it;
}

Value Grounder::getValue(int var)
{
	return substitution_[var];
}

void Grounder::setTempValue(int var, const Value &val)
{
	substitution_[var] = val;
}

void Grounder::setValue(int var, const Value &val, int binder)
{
	substitution_[var] = val;
	binder_[var] = binder;
}

int Grounder::getBinder(int var) const
{
	return binder_[var];
}

void Grounder::setConstValue(int id, Term *t)
{
	std::pair<ConstTerms::iterator, bool> res = constTerms_.insert(std::make_pair(id, std::make_pair(false, t)));
	if(!res.second)
	{
		std::cerr << "Warning: multiple definitions of #const " << *getString(id) << std::endl;
		delete t;
	}
}

Value Grounder::getConstValue(int id)
{
	ConstTerms::iterator it = constTerms_.find(id);
	if(it != constTerms_.end())
	{
		if(it->second.first)
			throw GrinGoException("Error: cyclic constant definition.");
		it->second.first = true;
		Value v = it->second.second->getConstValue(this);
		it->second.first = false;
		return v;
	}
	else
		return Value(Value::STRING, id);
}

Evaluator *Grounder::getEvaluator()
{
	return eval_;
}

NS_OUTPUT::Output *Grounder::getOutput()
{
	return output_;
}

const Grounder::Options &Grounder::options() const
{
	return opts_;
}

void Grounder::addTrueNegation(int id, int arity)
{
#ifdef WITH_ICLASP
	// TODO: this is ugly
	if(incremental_)
	{
		static bool warn = true;
		if(!warn)
			return;
		warn = false;
		std::cerr << "Warning: Classical negation is not handled correctly in combination with the incremental output." << std::endl;
		std::cerr << "         You have to add rules like: :- a, -a. on your own! (at least for now)" << std::endl;
		return;
	}
#endif
	if(trueNegPred_.insert(Signature(id, arity)).second)
	{
		TermVector *tp = new TermVector();
		TermVector *tn = new TermVector();
		for(int i = 0; i < arity; i++)
		{
			// in theory existing vars could be reused
			int var = createUniqueVar();
			tp->push_back(new Variable(this, var));
			tn->push_back(new Variable(this, var));
		}
		int pos = createString(getString(id)->substr(1));
		PredicateLiteral *p = new PredicateLiteral(this, pos, tp);
		PredicateLiteral *n = new PredicateLiteral(this, id, tn);
		LiteralVector *body = new LiteralVector();
		body->push_back(p);
		body->push_back(n);
		NormalRule *r = new NormalRule(0, body);
		addStatement(r);
	}
}

