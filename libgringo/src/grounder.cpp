// Copyright (c) 2009, Roland Kaminski <kaminski@cs.uni-potsdam.de>
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

#include <gringo/grounder.h>
#include <gringo/statement.h>
#include <gringo/domain.h>
#include <gringo/printer.h>
#include <gringo/stmdep.h>
#include <gringo/constterm.h>
#include <gringo/output.h>
#include <gringo/predindex.h>
#include <gringo/exceptions.h>
#include <gringo/luaterm.h>
#include <gringo/inclit.h>

#ifdef WITH_LUA
#	include <lua_impl.h>
#else

class Grounder::LuaImpl
{
public:
	LuaImpl(Grounder *) { }
	void call(Loc const &, char const *, const ValVec &, ValVec &) { throw std::runtime_error("lua: gringo was build without lua support"); }
	void exec(const Loc &, const std::string &) { throw std::runtime_error("lua: gringo was build without lua support"); }
	lua_State *state() { return 0; }
	void pushVal(const Val &) { }
};

#endif

TermDepthExpansion::TermDepthExpansion(IncConfig &config)
	: config(config)
{
}

bool TermDepthExpansion::limit(Grounder *g, const ValRng &rng, int32_t &offset) const
{
	bool found = false;
	offset = 0;
	foreach(const Val &val, rng)
	{
		if ( val.type == Val::FUNC )
		{
			int32_t depth = g->func(val.index).getDepth();
			if(depth >= config.incEnd)
			{
				if(offset < depth) { offset = depth; }
				found = true;
			}
		}
	}
	return found;
}

void TermDepthExpansion::expand(Grounder *g) const
{
	foreach (const DomainMap::const_reference &ref, g->domains())
	{
		for(int i = config.incBegin; i < config.incEnd; i++)
		{
			const_cast<Domain*>(ref.second)->addOffset(i);
		}
	}
}

TermDepthExpansion::~TermDepthExpansion()
{
}


Grounder::Grounder(Output *output, bool debug, TermExpansionPtr exp)
	: Storage(output)
	, internal_(0)
	, debug_(debug)
	, initialized_(false)
	, luaImpl_(new LuaImpl(this))
	, termExpansion_(exp)
{
}

void Grounder::luaExec(const Loc &loc, const std::string &s)
{
	luaImpl_->exec(loc, s);
}

void Grounder::luaCall(Loc const &loc, char const *name, const ValVec &args, ValVec &vals)
{
	luaImpl_->call(loc, name, args, vals);
}

lua_State *Grounder::luaState()
{
	return luaImpl_->state();
}

void Grounder::luaPushVal(const Val &val)
{
	return luaImpl_->pushVal(val);
}

void Grounder::addInternal(Statement *s)
{
	s->normalize(this);
	if(s->edbFact())
	{
		s->ground(this);
		delete s;
		stats_.addFact();
	}
	else { statements_.push_back(s); }
}

StatementRng Grounder::add(Statement *s)
{
	size_t offset = statements_.size();
	internal_     = 0;
	addInternal(s);
	return StatementRng(statements_.begin() + offset, statements_.end());
}

void Grounder::analyze(const std::string &depGraph, bool stats)
{
	// build dependency graph
	StmDep::Builder prgDep;
	foreach(Statement &s, statements_) prgDep.visit(&s);
	prgDep.analyze(this);
	if(!depGraph.empty())
	{
		std::ofstream out(depGraph.c_str());
		prgDep.toDot(this, out);
	}

	// generate input statistics
	if(stats)
	{
		foreach(DomainMap::reference dom, const_cast<DomainMap&>(domains()))
			dom.second->complete(false);
		foreach(Statement &s, statements_) stats_.visit(&s);
		stats_.numScc = components_.size();
		stats_.numPred = domains().size();
		size_t paramCount = 0;
		foreach(DomainMap::reference dom, const_cast<DomainMap&>(domains()))
		{
			paramCount += dom.second->arity();
			stats_.numPredVisible += output()->show(dom.second->nameId(),dom.second->arity());
		}
		stats_.avgPredParams = (stats_.numPred == 0) ? 0 : paramCount*1.0 / stats_.numPred;

		foreach(Component &component, components_)
		{
			if(component.statements.size() > 1) stats_.numSccNonTrivial ++;
		}
		stats_.print(std::cerr);
	}
}

void Grounder::ground()
{
	foreach(Statement &statement, statements_) { statement.enqueued(true); }
	foreach(DomainMap::reference dom, const_cast<DomainMap&>(domains()))
		dom.second->complete(false);
	foreach(Component &component, components_)
	{
		foreach(Statement *statement, component.statements)
		{
			if(debug_)
			{
				std::cerr << "% ";
				statement->print(this, std::cerr);
				std::cerr << std::endl;
			}
			if(!initialized_) { statement->init(this, VarSet()); }
			statement->enqueued(false);
			enqueue(statement);
			ground_();
		}
		foreach(Domain *dom, component.domains) { dom->complete(true); }
	}
	initialized_ = true;
	//debug_ = false;
}

void Grounder::ground_()
{
	while(!queue_.empty())
	{
		Groundable *g = queue_.front();
		queue_.pop_front();
		g->enqueued(false);
		g->ground(this);
	}
}

void Grounder::enqueue(Groundable *g)
{
	if(!g->enqueued())
	{
		g->enqueued(true);
		queue_.push_back(g);
	}
}

void Grounder::beginComponent()
{
	components_.push_back(Component());
}

void Grounder::addToComponent(Statement *stm)
{
	stm->check(this);
	components_.back().statements.push_back(stm);
}

void Grounder::addToComponent(Domain *dom)
{
	components_.back().domains.push_back(dom);
}

void Grounder::endComponent(bool positive)
{
	(void)positive;
	foreach(Domain *dom, components_.back().domains) dom->complete(true);
}

uint32_t Grounder::createVar()
{
	std::ostringstream oss;
	oss << "#I" << internal_++;
	std::string str(oss.str());
	return index(str);
}

void Grounder::externalStm(uint32_t nameId, uint32_t arity)
{
	domain(nameId, arity)->external(true);
	output()->external(nameId, arity);
}

Grounder::~Grounder()
{
}
