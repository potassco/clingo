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
#include <gringo/domain.h>
#include <gringo/storage.h>
#include <gringo/predlitrep.h>

namespace plainoutput_impl
{

GRINGO_EXPORT_PRINTER(DisplayPrinter)
GRINGO_EXPORT_PRINTER(ExternalPrinter)
GRINGO_EXPORT_PRINTER(RulePrinter)
GRINGO_EXPORT_PRINTER(SumAggrLitPrinter)
GRINGO_EXPORT_PRINTER(AvgAggrLitPrinter)
GRINGO_EXPORT_PRINTER(MinMaxAggrLitPrinter)
GRINGO_EXPORT_PRINTER(ParityAggrLitPrinter)
GRINGO_EXPORT_PRINTER(JunctionAggrLitPrinter)
GRINGO_EXPORT_PRINTER(OptimizePrinter)
GRINGO_EXPORT_PRINTER(ComputePrinter)
GRINGO_EXPORT_PRINTER(IncPrinter)

}

PlainOutput::PlainOutput(std::ostream *out)
	: Output(out)
{
	initPrinters<PlainOutput>();
}

void PlainOutput::beginRule()
{
	head_        = true;
	printedHead_ = false;
}

void PlainOutput::endHead()
{
	head_        = false;
	printedBody_ = false;
}

void PlainOutput::print()
{
	if(head_)
	{
		if(printedHead_) { *out_ << "|"; }
		printedHead_ = true;
	}
	else
	{
		if(printedBody_) { *out_ << ","; }
		else             { *out_ << ":-"; }
		printedBody_ = true;
	}
}

void PlainOutput::endRule()
{
	if(!printedHead_ && !printedBody_) { *out_ << ":-"; }
	*out_ << ".\n";
}

void PlainOutput::print(PredLitRep *l, std::ostream &out)
{
	if(l->sign())
		out << "not ";
	out << storage()->string(l->dom()->nameId());
	if(l->vals().size() > 0)
	{
		out << "(";
		l->vals().front().print(storage(), out);
		for(ValVec::const_iterator i = l->vals().begin() + 1; i != l->vals().end(); ++i)
		{
			out << ",";
			i->print(storage(), out);
		}
		out << ")";
	}
}

void PlainOutput::finalize()
{
	foreach(const Signature &sig, external_)
	{
		DomainMap::const_iterator i = s_->domains().find(sig);
		if(i != s_->domains().end())
		{
			const std::string &name  = s_->string(i->second->nameId());
			uint32_t           arity = i->second->arity();
			out() << "#external " << name << "/" << arity << ".\n";
		}
	}
	if(!computeLits_.empty())
	{
		bool comma = false;
		out() << "#compute{";
		for(size_t i = 0; i < computeLits_.size(); i++)
		{
			if(comma) out() << ",";
			else comma = true;
			if(computeSigns_[i]) out() << "not ";
			out() << storage()->string(computeLits_[i]);
		}
		out() << "}.\n";
	}
}

void PlainOutput::doShow(bool s)
{
	*out_ << (s ? "#show" : "#hide") << ".\n";
}

void PlainOutput::doShow(uint32_t nameId, uint32_t arity, bool s)
{
	*out_ << (s ? "#show " : "#hide ") << storage()->string(nameId) << "/" << arity << ".\n";
}

void PlainOutput::addCompute(PredLitRep *l)
{
	computeLits_.push_back(l->dom()->nameId());
	computeSigns_.push_back(l->sign());
}

PlainOutput::~PlainOutput()
{
}

