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
#include <gringo/lparseconverter.h>

class LparseOutput : public LparseConverter
{
public:
	LparseOutput(std::ostream *out, bool shiftDisj);
	void doFinalize();
public:
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
private:
    uint32_t models_;
	uint32_t symbols_;
	bool hasExternal_;
	std::vector<int32_t> computePos_;
	std::vector<int32_t> computeNeg_;
};

