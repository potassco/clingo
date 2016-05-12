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
#include <gringo/printer.h>
#include <gringo/lit.h>

struct IncConfig
{
	int  incBegin;
	int  incEnd;
	bool incBase;
	bool incVolatile;

	IncConfig()
		: incBegin(1)
		, incEnd(1)
		, incBase(false)
		, incVolatile(true)
	{ }
};

class IncLit : public Lit
{
public:
	class Printer : public ::Printer
	{
	public:
		using ::Printer::print;
		virtual void print() { }
		virtual ~Printer() { }
	};

public:
	IncLit(const Loc &loc, IncConfig &config_, bool cumulative, uint32_t varId);
	void normalize(Grounder *g, Expander *expander);
	bool fact() const { return true; }
	bool isFalse(Grounder *g);
	bool forcePrint() { return !cumulative_; }
	void accept(::Printer *v);
	void index(Grounder *g, Groundable *gr, VarSet &bound);
	void visit(PrgVisitor *visitor);
	bool match(Grounder *grounder);
	void print(Storage *sto, std::ostream &out) const;
	double score(Grounder *, VarSet &) const;
	Lit *clone() const;
	const IncConfig &config() const { return config_; }
	const VarTerm *var() const { return var_.get(); }
	bool cumulative() const { return cumulative_; }
	~IncLit();
private:
	IncConfig         &config_;
	bool               cumulative_;
	clone_ptr<VarTerm> var_;
};
