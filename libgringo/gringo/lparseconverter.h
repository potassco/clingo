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

#pragma once

#include <gringo/gringo.h>
#include <gringo/output.h>

class LparseConverter : public Output
{
public:
	typedef std::vector<uint32_t> AtomVec;
	typedef std::vector<int32_t>  WeightVec;
	struct ValCmp
	{
		ValCmp(const ValVec *v, uint32_t s);
		size_t operator()(uint32_t i) const;
		bool operator()(uint32_t a, uint32_t b) const;
		const ValVec *vals;
		uint32_t size;
	};
	typedef boost::unordered_map<uint32_t, uint32_t, ValCmp, ValCmp> SymbolMap;
protected:
	typedef std::vector<SymbolMap> SymbolTable;
	struct Minimize
	{
		AtomVec pos;
		AtomVec neg;
		WeightVec wPos;
		WeightVec wNeg;
	};
	typedef std::map<int32_t, Minimize> PrioMap;
	typedef boost::unordered_map<std::pair<uint32_t,uint32_t>, boost::unordered_set<uint32_t> > DisplayMap;
	typedef boost::unordered_map<std::pair<uint32_t,uint32_t>, boost::unordered_set<uint32_t> > ExternalMap;
	typedef std::pair<uint32_t, uint32_t> AtomRef;
	typedef std::vector<std::vector<AtomRef> > NewSymbols;
public:
	LparseConverter(std::ostream *out, bool shiftDisj);
	void addDomain(Domain *d);
	void prioLit(PredLitRep *l, int32_t weight, int32_t prio, bool maximize);
	void showAtom(PredLitRep *l);
	void hideAtom(PredLitRep *l);
	uint32_t symbol(PredLitRep *l);
	virtual void initialize();
	void finalize();
	void externalAtom(PredLitRep *l);
	void printSymbolTable();
	void printExternalTable();
	bool shiftDisjunctions() const { return shiftDisjunctions_; }
	void addCompute(PredLitRep *l);
	virtual ~LparseConverter();
public:
	virtual void printBasicRule(int head, const AtomVec &pos, const AtomVec &neg) = 0;
	virtual void printConstraintRule(int head, int bound, const AtomVec &pos, const AtomVec &neg) = 0;
	virtual void printChoiceRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg) = 0;
	virtual void printWeightRule(int head, int bound, const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg) = 0;
	virtual void printMinimizeRule(const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg) = 0;
	virtual void printDisjunctiveRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg) = 0;
	virtual void printComputeRule(int models, const AtomVec &pos, const AtomVec &neg) = 0;
	virtual void printSymbolTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name) = 0;
	virtual void printExternalTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name) = 0;
	virtual uint32_t symbol() = 0;
	virtual void doFinalize() = 0;
	virtual int getIncAtom() { return -1; }
protected:
	DisplayMap   atomsHidden_;
	DisplayMap   atomsShown_;
	ExternalMap  atomsExternal_;
	SymbolTable  symTab_;
	PrioMap      prioMap_;
	ValVec       vals_;
	uint32_t     false_;
	NewSymbols   newSymbols_;
	bool         shiftDisjunctions_;
	std::vector<uint32_t> compPos_;
	std::vector<uint32_t> compNeg_;
};
