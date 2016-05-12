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

#include "output/lparserule_impl.h"
#include <gringo/predlitrep.h>
#include <gringo/domain.h>
#include <gringo/storage.h>

namespace lparseconverter_impl
{

void RulePrinter::begin()
{
	choice_    = false;
	printHead_ = true;
	head_.clear();
}

void RulePrinter::endHead()
{
	printHead_ = false;
	pos_.clear();
	neg_.clear();
}

void RulePrinter::end()
{
	if(choice_) { 
		if (head_.size() > 0) { output_->printChoiceRule(head_, pos_, neg_); }
	}
	else if(head_.size() > 1) { output_->printDisjunctiveRule(head_, pos_, neg_); }
	else if(head_.empty())    { output_->printBasicRule(1, pos_, neg_); }
	else                      { output_->printBasicRule(head_[0], pos_, neg_); }
}

void RulePrinter::print(PredLitRep *l)
{
	if(printHead_)
	{
		assert(!l->sign());
		assert(head_.size() == 0);
		head_.push_back(output_->symbol(l));
	}
	else
	{
		uint32_t sym = output_->symbol(l);
		if(l->sign()) neg_.push_back(sym);
		else pos_.push_back(sym);
	}
}

void RulePrinter::setHead(LparseConverter::AtomVec &head, bool choice)
{
	assert(!choice_ && head_.empty());
	choice_ = choice;
	std::swap(head_, head);
}

void RulePrinter::setHead(uint32_t sym)
{
	assert(!choice_ && head_.empty());
	head_.push_back(sym);
}

void RulePrinter::addBody(uint32_t sym, bool sign)
{
	if(sign) neg_.push_back(sym);
	else pos_.push_back(sym);
}

void IncPrinter::print()
{
	RulePrinter *printer = static_cast<RulePrinter *>(output_->printer<Rule::Printer>());
	int atom = output_->getIncAtom();
	if(atom > 0) { printer->addBody(atom, false); }
}

}

GRINGO_REGISTER_PRINTER(lparseconverter_impl::RulePrinter, Rule::Printer, LparseConverter)
GRINGO_REGISTER_PRINTER(lparseconverter_impl::IncPrinter, IncLit::Printer, LparseConverter)
