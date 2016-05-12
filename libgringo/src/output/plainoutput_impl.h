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

#include <gringo/plainoutput.h>
#include <gringo/rule.h>
#include <gringo/sumaggrlit.h>
#include <gringo/avgaggrlit.h>
#include <gringo/junctionaggrlit.h>
#include <gringo/minmaxaggrlit.h>
#include <gringo/parityaggrlit.h>
#include <gringo/optimize.h>
#include <gringo/compute.h>
#include <gringo/display.h>
#include <gringo/external.h>
#include <gringo/inclit.h>

namespace plainoutput_impl
{
	class DisplayPrinter : public Display::Printer
	{
	public:
		DisplayPrinter(PlainOutput *output) : output_(output) { }
		void print(PredLitRep *l);
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput *output_;
	};

	class ExternalPrinter : public External::Printer
	{
	public:
		ExternalPrinter(PlainOutput *output) : output_(output) { }
		void print(PredLitRep *l);
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput *output_;
	};

	class RulePrinter : public Rule::Printer
	{
	public:
		RulePrinter(PlainOutput *output) : output_(output) { }
		void begin();
		void endHead();
		void print(PredLitRep *l);
		void end();
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput *output_;
		bool         head_;
		bool         printed_;
	};

	class SumAggrLitPrinter : public SumAggrLit::Printer
	{
	public:
		SumAggrLitPrinter(PlainOutput *output) : output_(output) { }
		void begin(bool head, bool sign, bool count);
		void weight(const Val &v);
		void lower(int32_t l);
		void upper(int32_t u);
		void print(PredLitRep *l);
		void end();
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput       *output_;
		int32_t            upper_;
		int32_t            lower_;
		bool               sign_;
		bool               count_;
		bool               hasUpper_;
		bool               hasLower_;
		bool               printedLit_;
		std::ostringstream aggr_;
	};

	class AvgAggrLitPrinter : public AvgAggrLit::Printer
	{
	public:
		AvgAggrLitPrinter(PlainOutput *output) : output_(output) { }
		void begin(bool head, bool sign);
		void weight(const Val &v);
		void lower(int32_t l);
		void upper(int32_t u);
		void print(PredLitRep *l);
		void end();
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput       *output_;
		int32_t            upper_;
		int32_t            lower_;
		bool               sign_;
		bool               count_;
		bool               hasUpper_;
		bool               hasLower_;
		bool               printedLit_;
		std::ostringstream aggr_;
	};

	class MinMaxAggrLitPrinter : public MinMaxAggrLit::Printer
	{
	public:
		MinMaxAggrLitPrinter(PlainOutput *output) : output_(output) { }
		void begin(bool head, bool sign, bool max);
		void weight(const Val &v);
		void lower(const Val &l);
		void upper(const Val &u);
		void print(PredLitRep *l);
		void end();
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput       *output_;
		Val                upper_;
		Val                lower_;
		bool               sign_;
		bool               max_;
		bool               hasUpper_;
		bool               hasLower_;
		bool               printedLit_;
		std::ostringstream aggr_;
	};

	class ParityAggrLitPrinter : public ParityAggrLit::Printer
	{
	public:
		ParityAggrLitPrinter(PlainOutput *output) : output_(output) { }
		void begin(bool head, bool sign, bool even, bool set);
		void print(PredLitRep *l);
		void weight(const Val &v);
		void end();
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput       *output_;
		bool               even_;
		bool               sign_;
		bool               set_;
		bool               printedLit_;
		std::ostringstream aggr_;
	};

	class JunctionAggrLitPrinter : public JunctionAggrLit::Printer
	{
	public:
		JunctionAggrLitPrinter(PlainOutput *output) : output_(output) { }
		void begin(bool head);
		void weight(const Val &v) { (void)v; }
		void print(PredLitRep *l);
		void end() {}
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput       *output_;
		bool               printed_;
		std::ostringstream aggr_;
	};

	class OptimizePrinter : public Optimize::Printer
	{
	public:
		OptimizePrinter(PlainOutput *output) : output_(output) { }
		void begin(bool maximize, bool set);
		void print(PredLitRep *l, int32_t weight, int32_t prio);
		void end();
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput *output_;
		bool         comma_;
		bool         set_;
	};

	class ComputePrinter : public Compute::Printer
	{
	public:
		ComputePrinter(PlainOutput *output) : output_(output) { }
		void begin();
		void print(PredLitRep *l);
		void end();
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		PlainOutput *output_;
	};

	class IncPrinter : public IncLit::Printer
	{
	public:
		using IncLit::Printer::print;
		IncPrinter(PlainOutput *output) : output_(output) {  }
		void print(PredLitRep *l) { (void)l; }
		Output *output() const { return output_; }
	private:
		PlainOutput *output_;
	};

}
