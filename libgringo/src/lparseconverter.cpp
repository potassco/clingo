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

#include <gringo/lparseoutput.h>
#include <gringo/predlitrep.h>
#include <gringo/domain.h>
#include <gringo/storage.h>
#include <gringo/rule.h>
#include <gringo/sumaggrlit.h>
#include <gringo/junctionaggrlit.h>

namespace lparseconverter_impl
{

GRINGO_EXPORT_PRINTER(RulePrinter)
GRINGO_EXPORT_PRINTER(OptimizePrinter)
GRINGO_EXPORT_PRINTER(ComputePrinter)
GRINGO_EXPORT_PRINTER(SumPrinter)
GRINGO_EXPORT_PRINTER(AvgPrinter)
GRINGO_EXPORT_PRINTER(MinMaxPrinter)
GRINGO_EXPORT_PRINTER(ParityPrinter)
GRINGO_EXPORT_PRINTER(JunctionPrinter)
GRINGO_EXPORT_PRINTER(DisplayPrinter)
GRINGO_EXPORT_PRINTER(ExternalPrinter)
GRINGO_EXPORT_PRINTER(IncPrinter)

}

LparseConverter::ValCmp::ValCmp(const ValVec *v, uint32_t s) :
	vals(v),
	size(s)
{
}

size_t LparseConverter::ValCmp::operator()(uint32_t i) const
{
	return boost::hash_range(vals->begin() + i, vals->begin() + i + size);
}

bool LparseConverter::ValCmp::operator()(uint32_t a, uint32_t b) const
{
	return std::equal(vals->begin() + a, vals->begin() + a + size, vals->begin() + b);
}

LparseConverter::LparseConverter(std::ostream *out, bool shiftDisj)
	: Output(out)
	, shiftDisjunctions_(shiftDisj)
{
	initPrinters<LparseConverter>();
}

void LparseConverter::initialize()
{
	false_ = symbol();
}

void LparseConverter::addDomain(Domain *d)
{
	symTab_.push_back(SymbolMap(0, ValCmp(&vals_, d->arity()), ValCmp(&vals_, d->arity())));
}

uint32_t LparseConverter::symbol(PredLitRep *l)
{
	uint32_t domId = l->dom()->domId();
	uint32_t size  = vals_.size();
	std::copy(l->vals().begin(), l->vals().end(), std::back_insert_iterator<ValVec>(vals_));
	std::pair<SymbolMap::iterator, bool> res = symTab_[domId].insert(SymbolMap::value_type(size, 0));
	if(res.second)
	{
		uint32_t sym = symbol();
		res.first->second = sym;
		if(newSymbols_.size() <= domId) { newSymbols_.resize(domId + 1); }
		newSymbols_[domId].push_back(AtomRef(sym, size));
	}
	else vals_.resize(size);
	return res.first->second;
}

void LparseConverter::showAtom(PredLitRep *l)
{
	atomsShown_[DisplayMap::key_type(l->dom()->nameId(), l->dom()->arity())].insert(symbol(l));
}

void LparseConverter::hideAtom(PredLitRep *l)
{
	atomsHidden_[DisplayMap::key_type(l->dom()->nameId(), l->dom()->arity())].insert(symbol(l));
}

void LparseConverter::prioLit(PredLitRep *l, int32_t weight, int32_t prio, bool maximize)
{
	Minimize &m = prioMap_[prio];
	if(weight != 0)
	{
		if((l->sign() != maximize) == (weight > 0))
		{
			m.neg.push_back(symbol(l));
			m.wNeg.push_back(std::abs(weight));
		}
		else
		{
			m.pos.push_back(symbol(l));
			m.wPos.push_back(std::abs(weight));
		}
	}
}

void LparseConverter::printSymbolTable()
{
	for(DomainMap::const_iterator i = s_->domains().begin(); i != s_->domains().end(); ++i)
	{
		uint32_t nameId = i->second->nameId();
		uint32_t arity  = i->second->arity();
		uint32_t domId  = i->second->domId();
		DisplayMap::iterator shown = atomsShown_.find(DisplayMap::key_type(nameId, arity));
		bool globShow = show(nameId, arity);
		if(globShow || shown != atomsShown_.end())
		{
			DisplayMap::iterator hidden = atomsHidden_.find(DisplayMap::key_type(nameId, arity));
			const std::string &name = s_->string(nameId);
			if(newSymbols_.size() <= domId) newSymbols_.resize(domId + 1);
			foreach(AtomRef &j, newSymbols_[domId])
			{
				if((globShow && (hidden == atomsHidden_.end() || hidden->second.find(j.first) == hidden->second.end())) ||
				   (!globShow && shown != atomsShown_.end() && shown->second.find(j.first) != shown->second.end()))
				{
					printSymbolTableEntry(j, arity, name);
				}
			}
		}
	}
}

void LparseConverter::printExternalTable()
{
	if(external_.size() > 0 || atomsExternal_.size() > 0)
	{
		for(DomainMap::const_iterator i = s_->domains().begin(); i != s_->domains().end(); ++i)
		{
			uint32_t nameId = i->second->nameId();
			uint32_t arity  = i->second->arity();
			uint32_t domId  = i->second->domId();
			ExternalMap::iterator ext = atomsExternal_.find(Signature(nameId, arity));
			bool globExt = external_.find(Signature(nameId, arity)) != external_.end();
			if(globExt || ext != atomsExternal_.end())
			{
				if(newSymbols_.size() <= domId) newSymbols_.resize(domId + 1);
				const std::string &name = s_->string(nameId);
				foreach(AtomRef &j, newSymbols_[domId])
				{
					if(globExt || ext->second.find(j.first) != ext->second.end())
						printExternalTableEntry(j, arity, name);
				}
			}
		}
	}
}

void LparseConverter::externalAtom(PredLitRep *l)
{
	atomsExternal_[ExternalMap::key_type(l->dom()->nameId(), l->dom()->arity())].insert(symbol(l));
}

void LparseConverter::addCompute(PredLitRep *l)
{
	if(l->sign()) { compNeg_.push_back(symbol(l)); }
	else          { compPos_.push_back(symbol(l)); }
}

void LparseConverter::finalize()
{
	foreach(PrioMap::value_type &min, prioMap_)
		printMinimizeRule(min.second.pos, min.second.neg, min.second.wPos, min.second.wNeg);
    printComputeRule(1, compPos_, compNeg_);
	doFinalize();
	newSymbols_.clear();
}

LparseConverter::~LparseConverter()
{
}

