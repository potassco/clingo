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

#include "output/reifiedoutput_impl.h"
#include <gringo/plainoutput.h>
#include <gringo/domain.h>
#include <gringo/predlit.h>
#include <gringo/func.h>
#include <gringo/storage.h>

namespace reifiedoutput_impl
{
	void DisplayPrinter::print(PredLitRep *l)
	{
		out() << (show() ? "show" : "hide") << "(";
		output_->val(output_->symbol(l)).print(output_->storage(), out());
		out() << ").\n";
	}

	void ExternalPrinter::print(PredLitRep *l)
	{
		out() << "external(";
		output_->val(output_->symbol(l)).print(output_->storage(), out());
		out() << ").\n";
	}

	void RulePrinter::begin()
	{
		output_->startSet();
		output_->pushDep();
	}

	void RulePrinter::endHead()
	{
		output_->startSet();
		output_->pushDep(2);
	}

	void RulePrinter::print(PredLitRep *l)
	{
		size_t symbol = output_->symbol(l);
		if(!l->sign()) { output_->addDep(symbol); }
		output_->addToSet(symbol);
	}

	void RulePrinter::end()
	{
		uint32_t body = output_->addSet();
		output_->popSet();
		const ReifiedOutput::Set &head = output_->getSet();
		assert(head.size() <= 1);
		out() << "rule(";
		if(head.empty()) { out() << "pos(false)"; }
		else             { output_->val(head.front()).print(output()->storage(), out()); }
		out() << ",pos(conjunction(" << body << "))).\n";
		Val val;
		val = Val::create(Val::NUM, int(body));
		val = Val::create(Val::FUNC, output_->storage()->index(Func(output_->storage(), output_->storage()->index("conjunction"), ValVec(1, val))));
		val = Val::create(Val::FUNC, output_->storage()->index(Func(output_->storage(), output_->storage()->index("pos"), ValVec(1, val))));
		size_t conjunction = output_->symbol(val);
		output_->popSet();
		output_->addDep(conjunction, 2);
		output_->popDep(true, 2); // lits -> body -> head
		output_->popDep(false);
	}

	void SumAggrLitPrinter::begin(bool head, bool sign, bool count)
	{
		(void)count;
		head_       = head;
		sign_       = sign;
		hasUpper_   = false;
		hasLower_   = false;
		hasNeg_     = false;
		min_        = 0;
		max_        = 0;
		output_->startList();
		if(!head_ && !sign_) { output_->pushDep(2); }
	}

	void SumAggrLitPrinter::weight(const Val &v)
	{
		int num = v.number();
		min_ += std::min(0, num);
		max_ += std::max(0, num);
		output_->addToList(symbol_, v);
		if(num < 0) { hasNeg_ = true; }
		if(pos_ && (num != 0 || head_)) { output_->addDep(symbol_); }
	}

	void SumAggrLitPrinter::lower(int32_t l)
	{
		hasLower_ = true;
		lower_    = l;
	}

	void SumAggrLitPrinter::upper(int32_t u)
	{
		hasUpper_ = true;
		upper_    = u;
	}

	void SumAggrLitPrinter::print(PredLitRep *l)
	{
		symbol_ = output_->symbol(l);
		pos_    = !l->sign() && !sign_;
	}

	void SumAggrLitPrinter::end()
	{
		uint32_t list = output_->addList();
		output_->popList();
		ValVec vals;
		vals.push_back(Val::create(Val::NUM, hasLower_ ? lower_ : min_));
		vals.push_back(Val::create(Val::NUM, (int)list));
		vals.push_back(Val::create(Val::NUM, hasUpper_ ? upper_ : max_));
		Val val;
		val = Val::create(Val::FUNC, output_->storage()->index(Func(output_->storage(), output_->storage()->index("sum"), vals)));
		val = Val::create(Val::FUNC, output_->storage()->index(Func(output_->storage(), output_->storage()->index(sign_ ? "neg" : "pos"), ValVec(1, val))));
		size_t sum = output_->symbol(val);
		output_->addToSet(sum);
		if(!sign_ && !head_)
		{
			if(hasNeg_ || hasLower_)
			{
				output_->addDep(sum, 2);
				output_->popDep(true);
				output_->popDep(false);
				output_->addDep(sum);
			}
			else { output_->popDep(false, 2); }
		}
	}

	void AvgAggrLitPrinter::begin(bool head, bool sign)
	{
		head_       = head;
		min_        = 0;
		max_        = 0;
		sign_       = sign;
		hasUpper_   = false;
		hasLower_   = false;
		output_->startList();
		if(!head_ && !sign_) { output_->pushDep(2); }
	}

	void AvgAggrLitPrinter::weight(const Val &v)
	{
		int num = v.number();
		min_ = std::min(min_, num);
		max_ = std::max(max_, num);
		output_->addToList(symbol_, v);
	}

	void AvgAggrLitPrinter::lower(int32_t l)
	{
		hasLower_ = true;
		lower_    = l;
	}

	void AvgAggrLitPrinter::upper(int32_t u)
	{
		hasUpper_ = true;
		upper_    = u;
	}

	void AvgAggrLitPrinter::print(PredLitRep *l)
	{
		symbol_ = output_->symbol(l);
		if(!l->sign() && !sign_) { output_->addDep(symbol_); }
	}

	void AvgAggrLitPrinter::end()
	{
		uint32_t list = output_->addList();
		output_->popList();
		ValVec vals;
		vals.push_back(Val::create(Val::NUM, hasLower_ ? lower_ : min_));
		vals.push_back(Val::create(Val::NUM, (int)list));
		vals.push_back(Val::create(Val::NUM, hasUpper_ ? upper_ : max_));
		Val val;
		val = Val::create(Val::FUNC, output_->storage()->index(Func(output_->storage(), output_->storage()->index("avg"), vals)));
		val = Val::create(Val::FUNC, output_->storage()->index(Func(output_->storage(), output_->storage()->index(sign_ ? "neg" : "pos"), ValVec(1, val))));
		size_t avg = output_->symbol(val);
		output_->addToSet(avg);
		if(!head_ && !sign_)
		{
			output_->addDep(avg, 2);
			output_->popDep(true);
			output_->popDep(false);
			output_->addDep(avg);
		}
	}

	void MinMaxAggrLitPrinter::begin(bool head, bool sign, bool max)
	{
		head_       = head;
		sign_       = sign;
		max_        = max;
		hasUpper_   = false;
		hasLower_   = false;
		output_->startList();
		if(!head_ && !sign_) { output_->pushDep(2); }
	}

	void MinMaxAggrLitPrinter::weight(const Val &v)
	{
		output_->addToList(symbol_, v);
	}

	void MinMaxAggrLitPrinter::lower(const Val &l)
	{
		hasLower_ = true;
		lower_    = l;
	}

	void MinMaxAggrLitPrinter::upper(const Val &u)
	{
		hasUpper_ = true;
		upper_    = u;
	}

	void MinMaxAggrLitPrinter::print(PredLitRep *l)
	{
		symbol_ = output_->symbol(l);
		if(!l->sign() && !sign_) { output_->addDep(symbol_); }
	}

	void MinMaxAggrLitPrinter::end()
	{
		uint32_t list = output_->addList();
		output_->popList();
		ValVec vals;
		vals.push_back(hasLower_ ? lower_ : max_ ? Val::inf() : Val::sup());
		vals.push_back(Val::create(Val::NUM, (int)list));
		vals.push_back(hasUpper_ ? upper_ : max_ ? Val::sup() : Val::inf());
		Val val;
		val = Val::create(Val::FUNC, output_->storage()->index(Func(output_->storage(), output_->storage()->index(max_ ? "max" : "min"), vals)));
		val = Val::create(Val::FUNC, output_->storage()->index(Func(output_->storage(), output_->storage()->index(sign_ ? "neg" : "pos"), ValVec(1, val))));
		size_t minmax = output_->symbol(val);
		output_->addToSet(minmax);
		if(!head_ && !sign_)
		{
			if((hasLower_ && max_) || (hasUpper_ && !max_))
			{
				output_->addDep(minmax, 2);
				output_->popDep(true);
				output_->popDep(false);
				output_->addDep(minmax);
			}
			else { output_->popDep(false, 2); }
		}
	}

	void ParityAggrLitPrinter::begin(bool head, bool sign, bool even, bool set)
	{
		head_ = head;
		(void)set;
		sign_ = sign;
		even_ = even;
		output_->startList();
		if(!head_ && !sign_) { output_->pushDep(2); }
	}

	void ParityAggrLitPrinter::print(PredLitRep *l)
	{
		symbol_ = output_->symbol(l);
		if(!l->sign() && !sign_) { output_->addDep(symbol_); }
	}

	void ParityAggrLitPrinter::weight(const Val &v)
	{
		output_->addToList(symbol_, Val::create(Val::NUM, v.number() % 2));
	}

	void ParityAggrLitPrinter::end()
	{
		uint32_t list = output_->addList();
		output_->popList();
		Val val;
		val = Val::create(Val::NUM, (int)list);
		val = Val::create(Val::FUNC, output_->storage()->index(Func(output_->storage(), output_->storage()->index(even_ ? "even" : "odd"), ValVec(1, val))));
		val = Val::create(Val::FUNC, output_->storage()->index(Func(output_->storage(), output_->storage()->index(sign_ ? "neg" : "pos"), ValVec(1, val))));
		size_t parity = output_->symbol(val);
		output_->addToSet(parity);
		if(!head_ && !sign_)
		{
			output_->addDep(parity, 2);
			output_->popDep(true);
			output_->popDep(false);
			output_->addDep(parity);
		}
	}

	void JunctionAggrLitPrinter::begin(bool head)
	{
		head_ = head;
		if(head_) { output_->startSet(); }
	}

	void JunctionAggrLitPrinter::print(PredLitRep *l)
	{
		uint32_t symbol = output_->symbol(l);
		output_->addToSet(symbol);
		if(!l->sign()) { output_->addDep(symbol); }
	}

	void JunctionAggrLitPrinter::end()
	{
		if(head_)
		{
			if(output_->getSet().size() == 1)
			{
				size_t sym = output_->getSet().back();
				output_->popSet();
				output_->addToSet(sym);
			}
			else if(output_->getSet().size() > 0)
			{
				uint32_t set = output_->addSet();
				output_->popSet();
				Val val;
				val = Val::create(Val::NUM, (int)set);
				val = Val::create(Val::FUNC, output_->storage()->index(Func(output_->storage(), output_->storage()->index("disjunction"), ValVec(1, val))));
				val = Val::create(Val::FUNC, output_->storage()->index(Func(output_->storage(), output_->storage()->index("pos"), ValVec(1, val))));
				output_->addToSet(output_->symbol(val));
			}
			else { output_->popSet(); }
		}
	}

	void OptimizePrinter::begin(bool maximize, bool set)
	{
		maximize_ = maximize;
		(void)set;
	}

	void OptimizePrinter::print(PredLitRep *l, int32_t weight, int32_t prio)
	{
		output_->minimize(output_->symbol(l), maximize_ ? -weight : weight, prio);
	}

	void OptimizePrinter::end()
	{
	}

	void ComputePrinter::begin()
	{
	}

	void ComputePrinter::print(PredLitRep *l)
	{
		output_->addCompute(l);
	}

	void ComputePrinter::end()
	{
	}

}

GRINGO_REGISTER_PRINTER(reifiedoutput_impl::DisplayPrinter, Display::Printer, ReifiedOutput)
GRINGO_REGISTER_PRINTER(reifiedoutput_impl::ExternalPrinter, External::Printer, ReifiedOutput)
GRINGO_REGISTER_PRINTER(reifiedoutput_impl::RulePrinter, Rule::Printer, ReifiedOutput)
GRINGO_REGISTER_PRINTER(reifiedoutput_impl::SumAggrLitPrinter, SumAggrLit::Printer, ReifiedOutput)
GRINGO_REGISTER_PRINTER(reifiedoutput_impl::AvgAggrLitPrinter, AvgAggrLit::Printer, ReifiedOutput)
GRINGO_REGISTER_PRINTER(reifiedoutput_impl::MinMaxAggrLitPrinter, MinMaxAggrLit::Printer, ReifiedOutput)
GRINGO_REGISTER_PRINTER(reifiedoutput_impl::ParityAggrLitPrinter, ParityAggrLit::Printer, ReifiedOutput)
GRINGO_REGISTER_PRINTER(reifiedoutput_impl::JunctionAggrLitPrinter, JunctionAggrLit::Printer, ReifiedOutput)
GRINGO_REGISTER_PRINTER(reifiedoutput_impl::OptimizePrinter, Optimize::Printer, ReifiedOutput)
GRINGO_REGISTER_PRINTER(reifiedoutput_impl::ComputePrinter, Compute::Printer, ReifiedOutput)
GRINGO_REGISTER_PRINTER(reifiedoutput_impl::IncPrinter, IncLit::Printer, ReifiedOutput)
