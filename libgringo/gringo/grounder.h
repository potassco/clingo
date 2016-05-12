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

#pragma once

#include <gringo/gringo.h>
#include <gringo/storage.h>
#include <gringo/context.h>
#include <gringo/locateable.h>
#include <gringo/stats.h>

typedef struct lua_State lua_State;

struct TermExpansion
{
	virtual bool limit(Grounder *g, const ValRng &rng, int32_t &) const;
	virtual void expand(Grounder *g) const;
	virtual ~TermExpansion();
};

typedef std::auto_ptr<TermExpansion> TermExpansionPtr;

inline bool TermExpansion::limit(Grounder *, const ValRng &, int32_t &) const { return false; }
inline void TermExpansion::expand(Grounder *) const { }
inline TermExpansion::~TermExpansion() { }

struct TermDepthExpansion : public TermExpansion
{
	TermDepthExpansion(IncConfig &config);
	virtual bool limit(Grounder *g, const ValRng &rng, int32_t &offset) const;
	virtual void expand(Grounder *g) const;
	virtual ~TermDepthExpansion();

	IncConfig &config;
};

class Grounder : public Storage, public Context
{
private:
	class LuaImpl;
	typedef std::deque<Groundable*> GroundableVec;
	struct Component
	{
		typedef std::vector<Statement*> StatementVec;
		typedef std::vector<Domain*> DomainVec;
		Component() { }
		StatementVec statements;
		DomainVec    domains;
	};
	typedef std::vector<Component> ComponentVec;


public:
	Grounder(Output *out, bool debug, TermExpansionPtr exp);
	void analyze(const std::string &depGraph = "", bool stats = false);
	void luaExec(const Loc &loc, const std::string &s);
    void luaCall(Loc const &loc, char const *name, const ValVec &args, ValVec &vals);
	lua_State *luaState();
	void luaPushVal(const Val &val);
	void ground();
	StatementRng add(Statement *s);
	void addInternal(Statement *stm);
	void enqueue(Groundable *g);
	void beginComponent();
	void addToComponent(Statement *stm);
	void addToComponent(Domain *dom);
	void endComponent(bool positive);
	void externalStm(uint32_t nameId, uint32_t arity);
	uint32_t createVar();
	
	const TermExpansion &termExpansion() const;
	~Grounder();

private:
	void ground_();

private:

	StatementPtrVec        statements_;
	GroundableVec          queue_;
	ComponentVec           components_;
	uint32_t               internal_;
	bool                   debug_;
	bool                   initialized_;
	std::auto_ptr<LuaImpl> luaImpl_;
	Stats                  stats_;
	TermExpansionPtr       termExpansion_;

};

inline const TermExpansion &Grounder::termExpansion() const { return *termExpansion_; }
