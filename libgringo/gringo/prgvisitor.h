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

class PrgVisitor
{
public:
	virtual void visit(VarTerm *var, bool bind) { (void)var; (void)bind; }
	virtual void visit(Term* term, bool bind) { (void)term; (void)bind; }
	virtual void visit(PredLit *pred) { (void)pred; }
	virtual void visit(Lit *lit, bool domain) { (void)lit; (void)domain; }
	virtual void visit(Groundable *grd, bool choice) { (void)grd; (void)choice; }
};

