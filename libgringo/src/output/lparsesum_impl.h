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
#include <gringo/sumaggrlit.h>
#include <gringo/lparseconverter.h>

namespace lparseconverter_impl
{

class RulePrinter;
class SumPrinter : public SumAggrLit::Printer
{
public:
	SumPrinter(LparseConverter *output, RulePrinter *printer = 0) : output_(output), printer_(printer) { }
	void begin(bool head, bool sign, bool count);
	void lower(int32_t l);
	void upper(int32_t u);
	void print(PredLitRep *l);
	void print(int32_t symbol, bool sign);
	void weight(const Val &v);
	void end();
	tribool simplify();
	Output *output() const { return output_; }
	std::ostream &out() const { return output_->out(); }
private:
	LparseConverter           *output_;
	LparseConverter::AtomVec   pos_;
	LparseConverter::AtomVec   neg_;
	LparseConverter::AtomVec   choice_;
	LparseConverter::WeightVec wPos_;
	LparseConverter::WeightVec wNeg_;
	bool                       head_;
	bool                       sign_;
	bool                       card_;
	bool                       hasLower_;
	bool                       hasUpper_;
	int32_t                    lower_;
	int32_t                    upper_;
	RulePrinter               *printer_;
};

}

