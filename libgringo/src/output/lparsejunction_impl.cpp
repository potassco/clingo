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

#include "output/lparsejunction_impl.h"
#include "output/lparserule_impl.h"
#include <gringo/predlitrep.h>
#include <gringo/domain.h>
#include <gringo/storage.h>

namespace lparseconverter_impl
{

void JunctionPrinter::begin(bool head)
{
	head_ = head;
	pos_.clear();
	neg_.clear();
}

void JunctionPrinter::print(PredLitRep *l)
{
	uint32_t sym = output_->symbol(l);
	assert(sym > 0);
	if(l->sign()) neg_.push_back(sym);
	else pos_.push_back(sym);
}

void JunctionPrinter::end()
{
	RulePrinter *printer = static_cast<RulePrinter *>(output_->printer<Rule::Printer>());
	if(head_)
	{
		if(pos_.size() == 1 && neg_.size() == 0) printer->setHead(pos_[0]);
		else if(!output_->shiftDisjunctions()) printer->setHead(pos_, false);
		else
		{
			// add replacement as head
			uint32_t d = output_->symbol();
			printer->setHead(d);

			// write a shifted rule for every atom in the disjunction
			for(size_t i = 0; i < pos_.size(); i++)
			{
				LparseConverter::AtomVec neg;
				for(size_t k = 0; k < pos_.size(); k++) if(k != i) neg.push_back(pos_[k]);
				output_->printBasicRule(pos_[i], LparseConverter::AtomVec(1, d), neg);
			}

		}
	}
	else
	{
		for(size_t i = 0; i < pos_.size(); i++)
			printer->addBody(pos_[i], false);
		for(size_t i = 0; i < neg_.size(); i++)
			printer->addBody(neg_[i], true);
	}
}

}

GRINGO_REGISTER_PRINTER(lparseconverter_impl::JunctionPrinter, JunctionAggrLit::Printer, LparseConverter)

