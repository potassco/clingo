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
#include <gringo/junctionaggrlit.h>
#include <gringo/lparseconverter.h>

namespace lparseconverter_impl
{

class JunctionPrinter : public JunctionAggrLit::Printer
{
public:
	JunctionPrinter(LparseConverter *output) : output_(output) { }
	void begin(bool head);
	void print(PredLitRep *l);
	void weight(const Val &v) { (void)v; }
	void end();
	Output *output() const { return output_; }
	std::ostream &out() const { return output_->out(); }
private:
	LparseConverter           *output_;
	LparseConverter::AtomVec   pos_;
	LparseConverter::AtomVec   neg_;
	bool                       head_;
};

}

