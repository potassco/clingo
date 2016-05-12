// Copyright (c) 2010, Roland Kaminski <kaminski@cs.uni-potsdam.de>
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
#include <gringo/lparseconverter.h>
#include <clasp/program_builder.h>

class ClaspOutput : public LparseConverter
{
	typedef std::vector<bool> BoolVec;
public:
	ClaspOutput(bool shiftDisj);
	virtual void initialize();
	void setProgramBuilder(Clasp::ProgramBuilder* api) { b_ = api; }
	const SymbolMap &symbolMap(uint32_t domId) const;
	ValRng vals(Domain *dom, uint32_t offset) const;
	~ClaspOutput();
protected:
	void printBasicRule(int head, const AtomVec &pos, const AtomVec &neg);
	void printConstraintRule(int head, int bound, const AtomVec &pos, const AtomVec &neg);
	void printChoiceRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg);
	void printWeightRule(int head, int bound, const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg);
	void printMinimizeRule(const AtomVec &pos, const AtomVec &neg, const WeightVec &wPos, const WeightVec &wNeg);
	void printDisjunctiveRule(const AtomVec &head, const AtomVec &pos, const AtomVec &neg);
	void printComputeRule(int models, const AtomVec &pos, const AtomVec &neg);
	void printSymbolTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name);
	void printExternalTableEntry(const AtomRef &atom, uint32_t arity, const std::string &name);
	uint32_t symbol();
	void doFinalize();
protected:
	Clasp::ProgramBuilder *b_;
	BoolVec  atomUnnamed_;
	uint32_t lastUnnamed_;
};

class iClaspOutput : public ClaspOutput
{
public:
	iClaspOutput(bool shiftDisj);
	void initialize();
	int getIncAtom();
private:
	int incUid_;
};
