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

#include "output/lparseminmax_impl.h"
#include "output/lparsesum_impl.h"
#include <gringo/predlitrep.h>
#include <gringo/domain.h>
#include <gringo/storage.h>

namespace lparseconverter_impl
{

void MinMaxPrinter::begin(bool head, bool sign, bool max)
{
	head_     = head;
	sign_     = sign;
	max_      = max;
	hasLower_ = false;
	hasUpper_ = false;
	lits_.clear();
	signs_.clear();
	weights_.clear();
}

void MinMaxPrinter::lower(const Val &l)
{
	hasLower_ = true;
	lower_    = l;
}

void MinMaxPrinter::upper(const Val &u)
{
	hasUpper_ = true;
	upper_    = u;
}

void MinMaxPrinter::print(PredLitRep *l)
{
	signs_.push_back(l->sign());
	lits_.push_back(output_->symbol(l));
}

void MinMaxPrinter::weight(const Val &v)
{
	weights_.push_back(v);
}

void MinMaxPrinter::end()
{
	if (head_) {
		int32_t n = output_->symbol();
		(static_cast<RulePrinter *>(output_->printer<Rule::Printer>()))->setHead(n);
		// choice
		{
			RulePrinter ruleprinter(output_);
			ruleprinter.begin();
			addConstraint(unknown, true, false, ruleprinter);
			ruleprinter.addBody(n, false);
			ruleprinter.end();
		}
		// integrity constraints
		if(hasUpper_)
		{
			RulePrinter ruleprinter(output_);
			ruleprinter.begin();
			addConstraint(true, false, true, ruleprinter);
			ruleprinter.addBody(n, false);
			ruleprinter.end();
		}
		if(hasLower_)
		{
			RulePrinter ruleprinter(output_);
			ruleprinter.begin();
			addConstraint(false, false, true, ruleprinter);
			ruleprinter.addBody(n, false);
			ruleprinter.end();
		}
	}
	else if(hasLower_ && hasUpper_ && sign_)
	{
		RulePrinter ruleprinter(output_);
		ruleprinter.begin();
		uint32_t m = output_->symbol();
		ruleprinter.setHead(m);
		(static_cast<RulePrinter *>(output_->printer<Rule::Printer>()))->addBody(m, sign_);
		SumPrinter printer(output_, &ruleprinter);
		addConstraint(false, false, false, ruleprinter);
		addConstraint(true, false, false, ruleprinter);
		ruleprinter.end();
	}
	else
	{
		RulePrinter *ruleprinter = static_cast<RulePrinter *>(output_->printer<Rule::Printer>());
		if(hasLower_) addConstraint(false, false, sign_, *ruleprinter);
		if(hasUpper_) addConstraint(true, false, sign_, *ruleprinter);
	}
}

void MinMaxPrinter::addConstraint(tribool bound, bool head, bool sign, RulePrinter &ruleprinter)
{
	SumPrinter printer(output_, &ruleprinter);
	printer.begin(head, sign, false);
	if(!bound == max_) printer.lower(1);
	else if(bound == max_) printer.upper(0);
	Val val = Val::create(Val::NUM, 1);
	for(size_t i = 0; i < lits_.size(); i++)
	{
		if(head || (!bound && (weights_[i].compare(lower_, output_->storage()) >= 0) == max_)
		   || (bound==true && (weights_[i].compare(upper_, output_->storage()) >  0) == max_))
		{
			printer.print(lits_[i], signs_[i]);
			printer.weight(val);
		}
	}
	printer.end();
}

}

GRINGO_REGISTER_PRINTER(lparseconverter_impl::MinMaxPrinter, MinMaxAggrLit::Printer, LparseConverter)

