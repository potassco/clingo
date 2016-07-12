// 
// Copyright (c) 2016, Benjamin Kaufmann
// 
// This file is part of Potassco. See http://potassco.sourceforge.net/
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// 
#ifndef LIBLP_ASPIF_TEXT_H_INCLUDED
#define LIBLP_ASPIF_TEXT_H_INCLUDED
#include <potassco/match_basic_types.h>
namespace Potassco {

class AspifTextInput : public ProgramReader {
public:
	AspifTextInput(AbstractProgram* out);
	void setOutput(AbstractProgram& out);
protected:
	virtual bool doAttach(bool& inc);
	virtual bool doParse();
	bool parseStatements();
private:
	void skipws();
	void matchDirective();
	void matchRule(char peek);
	void matchAtoms(const char* seps);
	void matchLits();
	void matchCondition();
	void matchAgg();
	bool match(const char* ts, bool required = true);
	Atom_t matchId();
	Lit_t  matchLit();
	int    matchInt();
	void   matchTerm();
	void   matchAtomArg();
	void   matchStr();
	void   startString();
	void   push(char c);
	void   endString();
	AbstractProgram* out_;
	struct ParseData;
	ParseData* data_;
};

}
#endif
