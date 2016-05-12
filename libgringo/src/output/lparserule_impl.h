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
#include <gringo/rule.h>
#include <gringo/inclit.h>
#include <gringo/lparseconverter.h>

namespace lparseconverter_impl
{

class RulePrinter : public Rule::Printer
{
public:
	RulePrinter(LparseConverter *output) : output_(output) { }
	void begin();
	void endHead();
	void print(PredLitRep *l);
	void setHead(LparseConverter::AtomVec &head, bool choice);
	void setHead(uint32_t sym);
	void addBody(uint32_t sym, bool sign);
	void end();
	Output *output() const { return output_; }
	std::ostream &out() const { return output_->out(); }
private:
	LparseConverter         *output_;
	LparseConverter::AtomVec pos_;
	LparseConverter::AtomVec neg_;
	LparseConverter::AtomVec head_;
	bool                     printHead_;
	bool                     choice_;
	bool                     headCond_;
};

class IncPrinter : public IncLit::Printer
{
public:
	IncPrinter(LparseConverter *output) : output_(output) {  }
	void print(PredLitRep *l);
	void print();
	Output *output() const { return output_; }
private:
	LparseConverter *output_;
};

inline void IncPrinter::print(PredLitRep *) { }

}
