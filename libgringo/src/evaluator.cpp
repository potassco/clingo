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

#include <gringo/evaluator.h>
#include <gringo/grounder.h>
#include <gringo/output.h>

using namespace gringo;
		
Evaluator::Evaluator() : g_(0), o_(0)
{
}

void Evaluator::initialize(Grounder *g)
{
	g_    = g;
	o_    = g->getOutput();
}

void Evaluator::add(NS_OUTPUT::Object *r)
{
	std::auto_ptr<NS_OUTPUT::Object> f(r);
	r->addDomain();
	o_->print(r);
}

void Evaluator::evaluate()
{

}

Evaluator::~Evaluator()
{
}

