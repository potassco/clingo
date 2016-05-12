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

#include "output/lparseparity_impl.h"
#include "output/lparsesum_impl.h"
#include <gringo/predlitrep.h>
#include <gringo/domain.h>
#include <gringo/storage.h>

namespace lparseconverter_impl
{

void ParityPrinter::begin(bool head, bool sign, bool even, bool set)
{
	head_     = head;
	sign_     = sign;
	even_     = even;
	(void)set;
	lits_.clear();
	signs_.clear();
	weights_.clear();
}

void ParityPrinter::print(PredLitRep *l)
{
	lits_.push_back(output_->symbol(l));
	signs_.push_back(l->sign());
}

void ParityPrinter::weight(const Val &v) {
	weights_.push_back(v.number() % 2 != 0);
};

void ParityPrinter::end()
{
	// print condition
	uint32_t cur, last = output_->symbol();
	if(even_) output_->printBasicRule(last, LparseConverter::AtomVec(), LparseConverter::AtomVec());
	for(size_t i = 0; i < lits_.size(); i++)
	{
		if(!weights_[i]) continue;
		cur = output_->symbol();
		if(signs_[i])
		{
			LparseConverter::AtomVec vec(2); vec[0] = last; vec[1] = lits_[i];
			output_->printBasicRule(cur, vec, LparseConverter::AtomVec());
			output_->printBasicRule(cur, LparseConverter::AtomVec(), vec);
		}
		else
		{
			output_->printBasicRule(cur, LparseConverter::AtomVec(1, last), LparseConverter::AtomVec(1, lits_[i]));
			output_->printBasicRule(cur, LparseConverter::AtomVec(1, lits_[i]), LparseConverter::AtomVec(1, last));
		}
		last = cur;
	}

	if(head_)
	{
		if(lits_.empty())
		{
			// print integrity constraint
			(static_cast<RulePrinter *>(output_->printer<Rule::Printer>()))->setHead(1);
			return;
		}
		// add replacement as head
		int32_t n = output_->symbol();
		(static_cast<RulePrinter *>(output_->printer<Rule::Printer>()))->setHead(n);

		// print choice with replacement as body
		RulePrinter ruleprinter(output_);
		ruleprinter.begin();
		SumPrinter printer(output_, &ruleprinter);
		printer.begin(true, false, false);
		Val val = Val::create(Val::NUM, 1);
		for(size_t i = 0; i < lits_.size(); i++)
		{
			printer.print(lits_[i], signs_[i]);
			printer.weight(val);
		}
		printer.end();
		ruleprinter.addBody(n, false);
		ruleprinter.end();

		// print integrity constraint: if replacement is true, condition has to be true
		output_->printBasicRule(1, LparseConverter::AtomVec(1, n), LparseConverter::AtomVec(1, last));
	}
	// add condition to body
	else (static_cast<RulePrinter *>(output_->printer<Rule::Printer>()))->addBody(last, sign_);
}

}

GRINGO_REGISTER_PRINTER(lparseconverter_impl::ParityPrinter, ParityAggrLit::Printer, LparseConverter)

