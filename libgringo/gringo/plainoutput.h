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

class PlainOutput : public Output
{
public:
	PlainOutput(std::ostream *out);
	void beginRule();
	void endHead();
	void print();
	void endRule();
	void print(PredLitRep *l, std::ostream &out);
	void finalize();
	void doShow(bool s);
	void doShow(uint32_t nameId, uint32_t arity, bool s);
	void addCompute(PredLitRep *l);
	~PlainOutput();
private:
	bool head_;
	bool printedHead_;
	bool printedBody_;
	std::vector<int32_t> computeLits_;
	std::vector<bool>    computeSigns_;
};

