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

#include <gringo/gringo.h>
#include <gringo/display.h>
#include <gringo/optimize.h>
#include <gringo/compute.h>
#include <gringo/external.h>
#include <gringo/lparseconverter.h>

namespace lparseconverter_impl
{

class DisplayPrinter : public Display::Printer
{
public:
	DisplayPrinter(LparseConverter *output) : output_(output) { }
	void print(PredLitRep *l);
	Output *output() const { return output_; }
	std::ostream &out() const { return output_->out(); }
private:
	LparseConverter *output_;
};

void DisplayPrinter::print(PredLitRep *l)
{
	if(show()) output_->showAtom(l);
	else output_->hideAtom(l);
}

class ExternalPrinter : public External::Printer
{
public:
	ExternalPrinter(LparseConverter *output) : output_(output) { }
	void print(PredLitRep *l);
	Output *output() const { return output_; }
	std::ostream &out() const { return output_->out(); }
private:
	LparseConverter *output_;
};

void ExternalPrinter::print(PredLitRep *l)
{
	output_->externalAtom(l);
}

class OptimizePrinter : public Optimize::Printer
{
public:
	OptimizePrinter(LparseConverter *output) : output_(output) { }
	void begin(bool maximize, bool set) { maximize_ = maximize; (void)set; }
	void print(PredLitRep *l, int32_t weight, int32_t prio);
	void end() { }
	Output *output() const { return output_; }
	std::ostream &out() const { return output_->out(); }
private:
	LparseConverter *output_;
	bool maximize_;
};

void OptimizePrinter::print(PredLitRep *l, int32_t weight, int32_t prio)
{
	output_->prioLit(l, weight, prio, maximize_);
}

class ComputePrinter : public Compute::Printer
{
public:
	ComputePrinter(LparseConverter *output) : output_(output) { }
	void begin() { }
	void print(PredLitRep *l);
	void end() { }
	Output *output() const { return output_; }
	std::ostream &out() const { return output_->out(); }
private:
	LparseConverter *output_;
};

void ComputePrinter::print(PredLitRep *l)
{
	output_->addCompute(l);
}

}

GRINGO_REGISTER_PRINTER(lparseconverter_impl::DisplayPrinter, Display::Printer, LparseConverter)
GRINGO_REGISTER_PRINTER(lparseconverter_impl::OptimizePrinter, Optimize::Printer, LparseConverter)
GRINGO_REGISTER_PRINTER(lparseconverter_impl::ComputePrinter, Compute::Printer, LparseConverter)
GRINGO_REGISTER_PRINTER(lparseconverter_impl::ExternalPrinter, External::Printer, LparseConverter)
