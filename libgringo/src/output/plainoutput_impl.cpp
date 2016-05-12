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

#include "output/plainoutput_impl.h"
#include <gringo/plainoutput.h>
#include <gringo/domain.h>
#include <gringo/predlit.h>

namespace plainoutput_impl
{
	void DisplayPrinter::print(PredLitRep *l)
	{
		out() << (show() ? "#show " : "#hide ");
		output_->print(l, out());
		out() << ".\n";
	}

	void ExternalPrinter::print(PredLitRep *l)
	{
		out() << "#external ";
		output_->print(l, out());
		out() << ".\n";
	}

	void RulePrinter::begin()
	{
		output_->beginRule();
	}

	void RulePrinter::endHead()
	{
		output_->endHead();
	}

	void RulePrinter::print(PredLitRep *l)
	{
		output_->print();
		output_->print(l, out());
	}

	void RulePrinter::end()
	{
		output_->endRule();
	}

	void SumAggrLitPrinter::begin(bool head, bool sign, bool count)
	{
		(void)head;
		output_->print();
		aggr_.str("");
		sign_       = sign;
		count_      = count;
		hasUpper_   = false;
		hasLower_   = false;
		printedLit_ = false;
	}

	void SumAggrLitPrinter::weight(const Val &v)
	{
		if(!count_) aggr_ << "=" << v.number();
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
		if(printedLit_) aggr_ << ",";
		else printedLit_ = true;
		output_->print(l, aggr_);
	}

	void SumAggrLitPrinter::end()
	{
		if(sign_) out() << "not ";
		if(hasLower_) out() << lower_;
		if(count_) out() << "#count{" << aggr_.str() << "}";
		else out() << "#sum[" << aggr_.str() << "]";
		if(hasUpper_) out() << upper_;
	}

	void AvgAggrLitPrinter::begin(bool head, bool sign)
	{
		(void)head;
		output_->print();
		aggr_.str("");
		sign_       = sign;
		hasUpper_   = false;
		hasLower_   = false;
		printedLit_ = false;
	}

	void AvgAggrLitPrinter::weight(const Val &v)
	{
		aggr_ << "=" << v.number();
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
		if(printedLit_) aggr_ << ",";
		else printedLit_ = true;
		output_->print(l, aggr_);
	}

	void AvgAggrLitPrinter::end()
	{
		if(sign_) out() << "not ";
		if(hasLower_) out() << lower_;
		out() << "#avg[" << aggr_.str() << "]";
		if(hasUpper_) out() << upper_;
	}

	void MinMaxAggrLitPrinter::begin(bool head, bool sign, bool max)
	{
		(void)head;
		output_->print();
		aggr_.str("");
		sign_       = sign;
		max_        = max;
		hasUpper_   = false;
		hasLower_   = false;
		printedLit_ = false;
	}

	void MinMaxAggrLitPrinter::weight(const Val &v)
	{
		aggr_ << "=";
		v.print(output_->storage(), aggr_);
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
		if(printedLit_) aggr_ << ",";
		else printedLit_ = true;
		output_->print(l, aggr_);
	}

	void MinMaxAggrLitPrinter::end()
	{
		if(sign_) out() << "not ";
		if(hasLower_) lower_.print(output_->storage(), out());
		if(max_) out() << "#max[" << aggr_.str() << "]";
		else out() << "#min[" << aggr_.str() << "]";
		if(hasUpper_) upper_.print(output_->storage(), out());
	}

	void ParityAggrLitPrinter::begin(bool head, bool sign, bool even, bool set)
	{
		(void)head;
		output_->print();
		aggr_.str("");
		sign_       = sign;
		even_       = even;
		set_        = set;
		printedLit_ = false;
	}

	void ParityAggrLitPrinter::print(PredLitRep *l)
	{
		if(printedLit_) aggr_ << ",";
		else printedLit_ = true;
		output_->print(l, aggr_);
	}

	void ParityAggrLitPrinter::weight(const Val &v)
	{
		if(set_) return;
		if(v.number() % 2 == 0) aggr_ << "=0";
		else aggr_ << "=1";
	}

	void ParityAggrLitPrinter::end()
	{
		if(sign_) out() << "not ";
		if(even_) out() << "#even";
		else out() << "#odd";
		if(set_) out() << "{" << aggr_.str() << "}";
		else out() << "[" << aggr_.str() << "]";
	}

	void JunctionAggrLitPrinter::begin(bool)
	{
	}

	void JunctionAggrLitPrinter::print(PredLitRep *l)
	{
		output_->print();
		output_->print(l, out());
	}

	void OptimizePrinter::begin(bool maximize, bool set)
	{
		set_ = set;
		comma_ = false;
		out() << (maximize ? "#maximize" : "#minimize");
		out() << (set_ ? "{" : "[");
	}

	void OptimizePrinter::print(PredLitRep *l, int32_t weight, int32_t prio)
	{
		if(comma_) out() << ",";
		else comma_ = true;
		output_->print(l, out());
		if(!set_) out() << "=" << weight;
		out() << "@" << prio;
	}

	void OptimizePrinter::end()
	{
		out() << (set_ ? "}.\n" : "].\n");
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

GRINGO_REGISTER_PRINTER(plainoutput_impl::DisplayPrinter, Display::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::ExternalPrinter, External::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::RulePrinter, Rule::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::SumAggrLitPrinter, SumAggrLit::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::AvgAggrLitPrinter, AvgAggrLit::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::MinMaxAggrLitPrinter, MinMaxAggrLit::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::ParityAggrLitPrinter, ParityAggrLit::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::JunctionAggrLitPrinter, JunctionAggrLit::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::OptimizePrinter, Optimize::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::IncPrinter, IncLit::Printer, PlainOutput)
GRINGO_REGISTER_PRINTER(plainoutput_impl::ComputePrinter, Compute::Printer, PlainOutput)
