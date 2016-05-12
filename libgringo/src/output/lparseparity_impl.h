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
#include <gringo/parityaggrlit.h>
#include <gringo/lparseconverter.h>
#include "output/lparserule_impl.h"

namespace lparseconverter_impl
{
class ParityPrinter : public ParityAggrLit::Printer
{
public:
	ParityPrinter(LparseConverter *output) : output_(output) { }
	void begin(bool head, bool sign, bool even, bool set);
	void print(PredLitRep *l);
	void weight(const Val &v);
	void end();
	Output *output() const { return output_; }
	std::ostream &out() const { return output_->out(); }
private:
	LparseConverter      *output_;
	std::vector<int32_t>  lits_;
	std::vector<bool>     signs_;
	std::vector<bool>     weights_;
	bool                  head_;
	bool                  even_;
	bool                  sign_;
};

}

