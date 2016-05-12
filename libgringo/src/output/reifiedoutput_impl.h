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

#include <gringo/reifiedoutput.h>
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

namespace reifiedoutput_impl
{
	class DisplayPrinter : public Display::Printer
	{
	public:
		DisplayPrinter(ReifiedOutput *output) : output_(output) { }
		void print(PredLitRep *l);
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		ReifiedOutput *output_;
	};

	class ExternalPrinter : public External::Printer
	{
	public:
		ExternalPrinter(ReifiedOutput *output) : output_(output) { }
		void print(PredLitRep *l);
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		ReifiedOutput *output_;
	};

	class RulePrinter : public Rule::Printer
	{
	public:
		RulePrinter(ReifiedOutput *output) : output_(output) { }
		void begin();
		void endHead();
		void print(PredLitRep *l);
		void end();
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		ReifiedOutput *output_;
		bool           head_;
		bool           printed_;
	};

	class SumAggrLitPrinter : public SumAggrLit::Printer
	{
	public:
		SumAggrLitPrinter(ReifiedOutput *output) : output_(output) { }
		void begin(bool head, bool sign, bool count);
		void weight(const Val &v);
		void lower(int32_t l);
		void upper(int32_t u);
		void print(PredLitRep *l);
		void end();
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		ReifiedOutput     *output_;
		int32_t            upper_;
		int32_t            lower_;
		int32_t            min_;
		int32_t            max_;
		size_t             symbol_;
		bool               pos_;
		bool               head_;
		bool               sign_;
		bool               hasUpper_;
		bool               hasLower_;
		bool               hasNeg_;
	};

	class AvgAggrLitPrinter : public AvgAggrLit::Printer
	{
	public:
		AvgAggrLitPrinter(ReifiedOutput *output) : output_(output) { }
		void begin(bool head, bool sign);
		void weight(const Val &v);
		void lower(int32_t l);
		void upper(int32_t u);
		void print(PredLitRep *l);
		void end();
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		ReifiedOutput       *output_;
		int32_t            upper_;
		int32_t            lower_;
		int32_t            min_;
		int32_t            max_;
		size_t             symbol_;
		bool               head_;
		bool               sign_;
		bool               hasUpper_;
		bool               hasLower_;
	};

	class MinMaxAggrLitPrinter : public MinMaxAggrLit::Printer
	{
	public:
		MinMaxAggrLitPrinter(ReifiedOutput *output) : output_(output) { }
		void begin(bool head, bool sign, bool max);
		void weight(const Val &v);
		void lower(const Val &l);
		void upper(const Val &u);
		void print(PredLitRep *l);
		void end();
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		ReifiedOutput     *output_;
		ReifiedOutput::Set pos_;
		Val                upper_;
		Val                lower_;
		size_t             symbol_;
		bool               sign_;
		bool               head_;
		bool               max_;
		bool               hasUpper_;
		bool               hasLower_;
		bool               printedLit_;
		std::ostringstream aggr_;
	};

	class ParityAggrLitPrinter : public ParityAggrLit::Printer
	{
	public:
		ParityAggrLitPrinter(ReifiedOutput *output) : output_(output) { }
		void begin(bool head, bool sign, bool even, bool set);
		void print(PredLitRep *l);
		void weight(const Val &v);
		void end();
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		ReifiedOutput *output_;
		size_t      symbol_;
		bool        head_;
		bool        even_;
		bool        sign_;
		bool        set_;
	};

	class JunctionAggrLitPrinter : public JunctionAggrLit::Printer
	{
	public:
		JunctionAggrLitPrinter(ReifiedOutput *output) : output_(output) { }
		void begin(bool head);
		void weight(const Val &v) { (void)v; }
		void print(PredLitRep *l);
		void end();
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		ReifiedOutput       *output_;
		bool              head_;

	};

	class OptimizePrinter : public Optimize::Printer
	{
	public:
		OptimizePrinter(ReifiedOutput *output) : output_(output) { }
		void begin(bool maximize, bool set);
		void print(PredLitRep *l, int32_t weight, int32_t prio);
		void end();
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		ReifiedOutput *output_;
		bool        maximize_;
	};

	class ComputePrinter : public Compute::Printer
	{
	public:
		ComputePrinter(ReifiedOutput *output) : output_(output) { }
		void begin();
		void print(PredLitRep *l);
		void end();
		Output *output() const { return output_; }
		std::ostream &out() const { return output_->out(); }
	private:
		ReifiedOutput *output_;
	};

	class IncPrinter : public IncLit::Printer
	{
	public:
		using IncLit::Printer::print;
		IncPrinter(ReifiedOutput *output) : output_(output) {  }
		void print(PredLitRep *l) { (void)l; }
		Output *output() const { return output_; }
	private:
		ReifiedOutput *output_;
	};

}
