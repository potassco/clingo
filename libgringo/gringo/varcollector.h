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
#include <gringo/prgvisitor.h>

class VarCollector : public PrgVisitor
{
	typedef std::map<uint32_t, VarTerm*> VarMap;
	typedef std::vector<uint32_t> VarStack;
	typedef std::vector<Groundable*> GrdQueue;
public:
	VarCollector(Grounder *grounder);
	void visit(VarTerm *var, bool bind);
	void visit(Term *term, bool bind);
	void visit(Lit *lit, bool domain);
	void visit(Groundable *g, bool choice);
	uint32_t collect();
private:
	VarMap      varMap_;
	VarStack    varStack_;
	GrdQueue    grdQueue_;
	Groundable *grd_;
	uint32_t    vars_;
	uint32_t    level_;
	Grounder   *grounder_;
};

