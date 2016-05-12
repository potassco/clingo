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

#include "output/lparsesum_impl.h"
#include "output/lparserule_impl.h"
#include <gringo/predlitrep.h>
#include <gringo/domain.h>
#include <gringo/storage.h>

namespace lparseconverter_impl
{

void SumPrinter::begin(bool head, bool sign, bool count)
{
	(void)count;
	head_     = head;
	sign_     = sign;
	hasLower_ = false;
	hasUpper_ = false;
	lower_ = 0;
	upper_ = 0;
	pos_.clear();
	neg_.clear();
	wPos_.clear();
	wNeg_.clear();
	choice_.clear();
}

void SumPrinter::lower(int32_t l)
{
	hasLower_ = true;
	lower_    = l;
}

void SumPrinter::upper(int32_t u)
{
	hasUpper_ = true;
	upper_    = u;
}

void SumPrinter::print(PredLitRep *l)
{
	uint32_t sym = output_->symbol(l);
	assert(sym > 0);
	if(l->sign()) neg_.push_back(sym);
	else pos_.push_back(sym);
}

void SumPrinter::print(int32_t symbol, bool sign)
{
	if(sign) neg_.push_back(symbol);
	else pos_.push_back(symbol);
}

void SumPrinter::weight(const Val &v)
{
	if(pos_.size() > wPos_.size()) wPos_.push_back(v.number());
	else wNeg_.push_back(v.number());
}

tribool SumPrinter::simplify()
{
	int64_t sum = 0;
	size_t j = 0;
	card_ = true;
	for(size_t i = 0; i < pos_.size(); i++)
	{
		if(head_) choice_.push_back(pos_[i]);
		if(wPos_[i] < 0)
		{
			lower_-= wPos_[i];
			upper_-= wPos_[i];
			neg_.push_back(pos_[i]);
			wNeg_.push_back(-wPos_[i]);
		}
		else if(wPos_[i] > 0)
		{
			card_ = card_ && wPos_[i] == 1;
			if(j < i)
			{
				pos_[j]  = pos_[i];
				wPos_[j] = wPos_[i];
			}
			sum+= wPos_[j++];
		}
	}
	pos_.resize(j);
	wPos_.resize(j);
	j = 0;
	for(size_t i = 0; i < neg_.size(); i++)
	{
		if(wNeg_[i] < 0)
		{
			card_ = card_ && wNeg_[i] == -1;
			lower_-= wNeg_[i];
			upper_-= wNeg_[i];
			sum   -= wNeg_[i];
			pos_.push_back(neg_[i]);
			wPos_.push_back(-wNeg_[i]);
		}
		else if(wNeg_[i] > 0)
		{
			card_ = card_ && wNeg_[i] == 1;
			if(j < i)
			{
				neg_[j]  = neg_[i];
				wNeg_[j] = wNeg_[i];
			}
			sum+= wNeg_[j++];
		}
	}
	neg_.resize(j);
	wNeg_.resize(j);

	// turn into count aggregate (important for optimizations later on)
	if(wNeg_.size() + wPos_.size() == 1 && sum > 1)
	{
		if(hasLower_)             { lower_ = (lower_ + sum - 1) / sum; }
		if(hasUpper_)             { upper_ = upper_ / sum; }
		if(wPos_.size() > 0)      { wPos_[0] = 1; }
		else if(wNeg_.size() > 0) { wNeg_[0] = 1; }
		sum = 1;
	}

	// TODO: multiple occurrences of the same literal could be combined
	//       in the head even between true and false literals!!!
	if(hasLower_ && lower_ <= 0)   { hasLower_ = false; }
	if(!hasLower_)                 { lower_    = 0; }
	if(hasUpper_ && upper_ >= sum) { hasUpper_ = false; }
	if(!hasUpper_)                 { upper_    = sum; }
	if(hasUpper_ && hasLower_ && lower_ > upper_) { return sign_; }
	if((upper_ < 0) || (sum < lower_))            { return sign_; }
	else if(!hasLower_ && !hasUpper_)             { return !sign_; }
	else                                          { return unknown; }
}

void SumPrinter::end()
{
	tribool truthValue = simplify();

	if(!printer_) { printer_ = static_cast<RulePrinter *>(output_->printer<Rule::Printer>()); }

	if(head_)
	{
		assert(!sign_);
		if(!truthValue) printer_->setHead(1);
		else if(truthValue) printer_->setHead(choice_, true);
		else
		{
			if(pos_.size() == 1 && neg_.size() == 0 && hasLower_) printer_->setHead(pos_[0]);
			else
			{
				assert(hasLower_ || hasUpper_);
				uint32_t body = output_->symbol();
				printer_->setHead(body);
				output_->printChoiceRule(choice_, LparseConverter::AtomVec(1, body), LparseConverter::AtomVec());
				if(hasLower_)
				{
					uint32_t l = output_->symbol();
					if(card_) output_->printConstraintRule(l, lower_, pos_, neg_);
					else output_->printWeightRule(l, lower_, pos_, neg_, wPos_, wNeg_);
					output_->printBasicRule(1, LparseConverter::AtomVec(1, body), LparseConverter::AtomVec(1, l));
				}
				if(hasUpper_)
				{
					uint32_t u = output_->symbol();
					if(card_) output_->printConstraintRule(u, upper_ + 1, pos_, neg_);
					else output_->printWeightRule(u, upper_ + 1, pos_, neg_, wPos_, wNeg_);
					LparseConverter::AtomVec pos(2); pos[0] = u; pos[1] = body;
					output_->printBasicRule(1, pos, LparseConverter::AtomVec());
				}
			}
		}
	}
	else
	{
		if(!truthValue) printer_->addBody(1, false);
		else if(unknown(truthValue))
		{
			if(pos_.size() == 1 && neg_.size() == 0 && hasLower_)                { printer_->addBody(pos_[0], sign_); }
			else if(pos_.size() == 1 && neg_.size() == 0 && !sign_ && hasUpper_) { printer_->addBody(pos_[0], true); }
			else if(pos_.size() == 0 && neg_.size() == 1 && !sign_ && hasLower_) { printer_->addBody(neg_[0], true); }
			else if(pos_.size() == 0 && neg_.size() == 1 &&  sign_ && hasUpper_) { printer_->addBody(neg_[0], true); }
			else
			{
				if(!hasLower_ || !hasUpper_ || !sign_)
				{
					if(hasLower_)
					{
						uint32_t l = output_->symbol();
						if(card_) output_->printConstraintRule(l, lower_, pos_, neg_);
						else output_->printWeightRule(l, lower_, pos_, neg_, wPos_, wNeg_);
						printer_->addBody(l, sign_);
					}
					if(hasUpper_)
					{
						uint32_t u = output_->symbol();
						if(card_) output_->printConstraintRule(u, upper_ + 1, pos_, neg_);
						else output_->printWeightRule(u, upper_ + 1, pos_, neg_, wPos_, wNeg_);
						if(!sign_) printer_->addBody(u, true);
						else
						{
							uint32_t n = output_->symbol();
							printer_->addBody(n, true);
							output_->printBasicRule(n, LparseConverter::AtomVec(), LparseConverter::AtomVec(1, u));
						}
					}
				}
				else
				{
					uint32_t l = output_->symbol();
					uint32_t u = output_->symbol();
					uint32_t n = output_->symbol();
					printer_->addBody(n, true);
					if(card_) output_->printConstraintRule(l, lower_, pos_, neg_);
					else output_->printWeightRule(l, lower_, pos_, neg_, wPos_, wNeg_);
					if(card_) output_->printConstraintRule(u, upper_ + 1, pos_, neg_);
					else output_->printWeightRule(u, upper_ + 1, pos_, neg_, wPos_, wNeg_);
					output_->printBasicRule(n, LparseConverter::AtomVec(1, l), LparseConverter::AtomVec(1, u));
				}
			}
		}
	}
}

}

GRINGO_REGISTER_PRINTER(lparseconverter_impl::SumPrinter, SumAggrLit::Printer, LparseConverter)
