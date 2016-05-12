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

class Printer
{
private:
	typedef std::map<Signature, bool> ShowMap;
public:
	Printer() { }
	virtual void print(PredLitRep *l) = 0;
	virtual Output *output() const = 0;
	template<class P>
	static uint32_t printer();
	template<class P, class O>
	static Printer *create(O *output) { return new P(output); }
	virtual ~Printer() { }
public:
	static size_t printers(bool reg);
};

inline size_t Printer::printers(bool reg)
{
	static int printers = 0;
	return reg ? printers++ : printers;
}

template<class P>
inline uint32_t Printer::printer()
{
	static size_t printer = Printer::printers(true);
	return printer;
}

