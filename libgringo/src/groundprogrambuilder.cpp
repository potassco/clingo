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

#include <gringo/groundprogrambuilder.h>
#include <gringo/output.h>
#include <gringo/storage.h>
#include <gringo/func.h>
#include <gringo/optimize.h>
#include <gringo/compute.h>
#include <gringo/rule.h>
#include <gringo/sumaggrlit.h>
#include <gringo/avgaggrlit.h>
#include <gringo/minmaxaggrlit.h>
#include <gringo/parityaggrlit.h>
#include <gringo/junctionaggrlit.h>
#include <gringo/external.h>
#include <gringo/display.h>

GroundProgramBuilder::GroundProgramBuilder(Output *output)
	: output_(output)
{
}

void GroundProgramBuilder::add(Type type, uint32_t n)
{
	switch(type)
	{
		case LIT:
		{
			lits_.push_back(Lit::create(type, vals_.size() - n - 1, n));
			break;
		}
		case TERM:
		{
			if(n > 0)
			{
				ValVec vals;
				std::copy(vals_.end() - n, vals_.end(), std::back_inserter(vals));
				vals_.resize(vals_.size() - n);
				uint32_t name = vals_.back().index;
				vals_.back()  = Val::create(Val::FUNC, storage()->index(Func(storage(), name, vals)));
			}
			break;
		}
		case AGGR_SUM:
		case AGGR_COUNT:
		case AGGR_AVG:
		case AGGR_MIN:
		case AGGR_MAX:
		case AGGR_EVEN:
		case AGGR_EVEN_SET:
		case AGGR_ODD:
		case AGGR_ODD_SET:
		case AGGR_DISJUNCTION:
		{
			assert(type != AGGR_DISJUNCTION || n > 0);
			std::copy(lits_.end() - n, lits_.end(), std::back_inserter(aggrLits_));
			lits_.resize(lits_.size() - n);
			lits_.push_back(Lit::create(type, n ? aggrLits_.size() - n : vals_.size() - 2, n));
			break;
		}
		case STM_RULE:
		case STM_CONSTRAINT:
		{
			Rule::Printer *printer = output_->printer<Rule::Printer>();
			printer->begin();
			if(type == STM_RULE)             { printLit(printer, lits_.size() - n - 1, true); }
			printer->endHead();
			for(uint32_t i = n; i >= 1; i--) { printLit(printer, lits_.size() - i, false); }
			printer->end();
			pop(n + (type == STM_RULE));
			break;
		}
		case STM_SHOW:
		case STM_HIDE:
		{
			Display::Printer *printer = output_->printer<Display::Printer>();
            printer->show(type == STM_SHOW);
			printLit(printer, lits_.size() - 1, true);
			pop(1);
			break;
		}
		case STM_EXTERNAL:
		{
			External::Printer *printer = output_->printer<External::Printer>();
			printLit(printer, lits_.size() - 1, true);
			pop(1);
			break;
		}
		case STM_MINIMIZE:
		case STM_MAXIMIZE:
		case STM_MINIMIZE_SET:
		case STM_MAXIMIZE_SET:
		{
			Optimize::Printer *printer = output_->printer<Optimize::Printer>();
			bool maximize = (type == STM_MAXIMIZE || type == STM_MAXIMIZE_SET);
			bool set = (type == STM_MINIMIZE_SET || type == STM_MAXIMIZE_SET);
			printer->begin(maximize, set);
			for(uint32_t i = n; i >= 1; i--)
			{

				Lit &a     = lits_[lits_.size() - i];
				Val prio   = vals_[a.offset + a.n + 2];
				Val weight = vals_[a.offset + a.n + 1];
				printer->print(predLitRep(a), weight.num, prio.num);
			}
			printer->end();
			pop(n);
			break;
		}
		case STM_COMPUTE:
		{
			Compute::Printer *printer = output_->printer<Compute::Printer>();
			printer->begin();
			for(uint32_t i = n; i >= 1; i--)
			{
				Lit &a = lits_[lits_.size() - i];
				printer->print(predLitRep(a));
			}
			printer->end();
			pop(n);
			break;
		}
		case META_SHOW:
		case META_HIDE:
		case META_EXTERNAL:
		{
			Val num = vals_.back();
			vals_.pop_back();
			Val id  = vals_.back();
			vals_.pop_back();
			assert(id.type == Val::ID);
			assert(num.type == Val::NUM);
			storage()->domain(id.index, num.num);
			if(type == META_EXTERNAL) { output_->external(id.index, num.num); }
			else { output_->show(id.index, num.num, type == META_SHOW); }
			break;
		}
		case META_GLOBALSHOW:
		case META_GLOBALHIDE:
		{
			output_->show(type == META_GLOBALSHOW);
			break;
		}
	}
}

void GroundProgramBuilder::pop(uint32_t n)
{
	if(n > 0)
	{
		Lit &a = *(lits_.end() - n);
		if(a.type == LIT) { vals_.resize(a.offset); }
		else
		{
			if(a.n > 0)
			{
				Lit &b = aggrLits_[a.offset];
				aggrLits_.resize(a.offset);
				vals_.resize(b.offset - (a.type != AGGR_DISJUNCTION));
			}
			else { vals_.resize(a.offset); }
		}
		lits_.resize(lits_.size() - n);
	}
}

void GroundProgramBuilder::printLit(Printer *printer, uint32_t offset, bool head)
{
	Lit &a = lits_[offset];
	if(a.type == LIT) { printer->print(predLitRep(a)); }
	else
	{
		switch(a.type)
		{
			case AGGR_SUM:
			case AGGR_COUNT:
			{
				SumAggrLit::Printer *aggrPrinter = output_->printer<SumAggrLit::Printer>();
				aggrPrinter->begin(head, a.sign, a.type == AGGR_COUNT);
				Val lower;
				Val upper;
				if(a.n == 0)
				{
					lower = vals_[a.offset];
					upper = vals_[a.offset + 1];
				}
				else
				{
					Lit &first = aggrLits_[a.offset];
					Lit &last  = aggrLits_[a.offset + a.n - 1];
					lower = vals_[first.offset - 1];
					upper = vals_[last.offset + last.n + 1 + (a.type == AGGR_SUM)];
				}
				if(lower.type != Val::UNDEF) { assert(lower.type == Val::NUM); aggrPrinter->lower(lower.num); }
				if(upper.type != Val::UNDEF) { assert(upper.type == Val::NUM); aggrPrinter->upper(upper.num); }
				printAggrLits(aggrPrinter, a, a.type == AGGR_SUM);
				aggrPrinter->end();
				break;
			}
			case AGGR_AVG:
			{
				AvgAggrLit::Printer *aggrPrinter = output_->printer<AvgAggrLit::Printer>();
				aggrPrinter->begin(head, a.sign);
				Val lower;
				Val upper;
				if(a.n == 0)
				{
					lower = vals_[a.offset];
					upper = vals_[a.offset + 1];
				}
				else
				{
					Lit &first = aggrLits_[a.offset];
					Lit &last  = aggrLits_[a.offset + a.n - 1];
					lower = vals_[first.offset - 1];
					upper = vals_[last.offset + last.n + 2];
				}
				if(lower.type != Val::UNDEF) { assert(lower.type == Val::NUM); aggrPrinter->lower(lower.num); }
				if(upper.type != Val::UNDEF) { assert(upper.type == Val::NUM); aggrPrinter->upper(upper.num); }
				printAggrLits(aggrPrinter, a, true);
				aggrPrinter->end();
				break;
			}
			case AGGR_MIN:
			case AGGR_MAX:
			{
				MinMaxAggrLit::Printer *aggrPrinter = output_->printer<MinMaxAggrLit::Printer>();
				aggrPrinter->begin(head, a.sign, a.type == AGGR_MAX);
				Val lower;
				Val upper;
				if(a.n == 0)
				{
					lower = vals_[a.offset];
					upper = vals_[a.offset + 1];
				}
				else
				{
					Lit &first = aggrLits_[a.offset];
					Lit &last  = aggrLits_[a.offset + a.n - 1];
					lower = vals_[first.offset - 1];
					upper = vals_[last.offset + last.n + 2];
				}
				if(lower.type != Val::UNDEF) { aggrPrinter->lower(lower); }
				if(upper.type != Val::UNDEF) { aggrPrinter->upper(upper); }
				printAggrLits(aggrPrinter, a, true);
				aggrPrinter->end();
				break;
			}
			case AGGR_EVEN:
			case AGGR_EVEN_SET:
			case AGGR_ODD:
			case AGGR_ODD_SET:
			{
				ParityAggrLit::Printer *aggrPrinter = output_->printer<ParityAggrLit::Printer>();
				bool even = (a.type == AGGR_EVEN || a.type == AGGR_EVEN_SET);
				bool set = (a.type == AGGR_EVEN_SET || a.type == AGGR_ODD_SET);
				aggrPrinter->begin(head, a.sign, even, set);
				printAggrLits(aggrPrinter, a, !set);
				aggrPrinter->end();
				break;
			}
			case AGGR_DISJUNCTION:
			{
				JunctionAggrLit::Printer *aggrPrinter = output_->printer<JunctionAggrLit::Printer>();
				aggrPrinter->begin(head);
				printAggrLits(aggrPrinter, a, false);
				aggrPrinter->end();
				break;
			}
			default:
			{
				assert(a.type == LIT);
				printer->print(predLitRep(a));
				break;
			}
		}
	}
}

void GroundProgramBuilder::printAggrLits(AggrLit::Printer *printer, Lit &a, bool weight)
{
	if(a.n > 0)
	{
		foreach(Lit &b, boost::iterator_range<LitVec::iterator>(aggrLits_.begin() + a.offset, aggrLits_.begin() + a.offset + a.n))
		{
			printer->print(predLitRep(b));
			if(weight) { printer->weight(vals_[b.offset + b.n + 1]); }
			else       { printer->weight(Val::create(Val::NUM, 1)); }
		}
	}
}

PredLitRep *GroundProgramBuilder::predLitRep(Lit &a)
{
	Domain *dom = storage()->domain(vals_[a.offset].index, a.n);
	lit_.dom_   = dom;
	lit_.sign_  = a.sign;
	lit_.vals_.resize(a.n);
	std::copy(vals_.begin() + a.offset + 1, vals_.begin() + a.offset + 1 + a.n, lit_.vals_.begin());
	return &lit_;
}

void GroundProgramBuilder::addVal(const Val &val)
{
	vals_.push_back(val);
}

void GroundProgramBuilder::addSign()
{
	lits_.back().sign = true;
}

Storage *GroundProgramBuilder::storage()
{
	return output_->storage();
}
