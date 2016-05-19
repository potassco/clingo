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

#include <gringo/program.h>
#include <gringo/statement.h>
#include <gringo/grounder.h>
#include <gringo/literal.h>
#include <gringo/evaluator.h>
#include <gringo/basicprogramevaluator.h>

using namespace gringo;

Program::Program(Type type, StatementVector &rules) : type_(type), eval_(0)
{
	std::swap(rules, rules_);
}

Evaluator *Program::getEvaluator()
{
	if(!eval_)
	{
		switch(type_)
		{
			case Program::FACT:
				eval_ = new Evaluator();
				break;
			case Program::BASIC:
				eval_ = new BasicProgramEvaluator();
				break;
			case Program::NORMAL:
				eval_ = new Evaluator();
				break;
		}
	}
	return eval_;
}

void Program::print(const GlobalStorage *g, std::ostream &out) const
{
	if(rules_.size() > 0)
	{
		switch(type_)
		{
			case Program::FACT:
				out << "% fact program:" << std::endl;
				break;
			case Program::BASIC:
				out << "% basic program:" << std::endl;
				break;
			case Program::NORMAL:
				out << "% normal program:" << std::endl;
				break;
		}
		for(StatementVector::const_iterator it = rules_.begin(); it != rules_.end(); it++)
		{
			out << pp(g, *it) << std::endl;;
		}
	}
}

bool Program::check(Grounder *g)
{
	VarVector free;
	LiteralVector unsolved;
	// conditionals have to be omega restricted
	for(StatementVector::iterator i = rules_.begin(); i != rules_.end(); i++)
	{
		if(!(*i)->checkO(unsolved))
		{
			std::cerr << "the following rule cannot be grounded, ";
			std::cerr << "non domain predicates : { ";
			bool comma = false;
			for(LiteralVector::iterator j = unsolved.begin(); j != unsolved.end(); j++)
			{
				if(comma)
					std::cerr << ", ";
				else
					comma = true;
				std::cerr << pp(g, *j);
			}
			std::cerr << " }" << std::endl;
			std::cerr << "	" << pp(g, *i) << std::endl;
			return false;
		}
	}
	// check if scc is lambda restricted
	for(StatementVector::iterator ok = rules_.begin(); ok != rules_.end();)
	{
		for(; ok != rules_.end(); ok++)
		{
			if(!(*ok)->check(g, free))
				break;
			(*ok)->finish();
		}
		if(ok != rules_.end())
		{
			StatementVector::iterator it = ok;
			for(it++; it != rules_.end(); it++)
			{
				if((*it)->check(g, free))
				{
					std::swap(*ok, *it);
					(*ok++)->finish();
					break;
				}
			}
			if(it == rules_.end())
			{
				std::cerr << "the following rule cannot be grounded, ";
				std::cerr << "weakly restricted variables: { ";
				bool comma = false;
				for(VarVector::iterator it = free.begin(); it != free.end(); it++)
				{
					if(comma)
						std::cerr << ", ";
					else
						comma = true;
					std::cerr << *g->getVarString(*it);
				}
				std::cerr << " }" << std::endl;
				std::cerr << "	" << pp(g, rules_.back()) << std::endl;
				return false;
			}
		}
	}
	for(StatementVector::iterator it = rules_.begin(); it != rules_.end(); it++)
		(*it)->evaluate();
	return true;
}

StatementVector *Program::getStatements()
{
	return &rules_;
}

Program::Type Program::getType()
{
	return type_;
}

Program::~Program()
{
	if(eval_)
		delete eval_;
}

