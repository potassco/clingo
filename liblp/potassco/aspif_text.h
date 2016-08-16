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

//! Class for parsing logic programs in ground text format.
/*!
 * \addtogroup ParseType
 */
class AspifTextInput : public ProgramReader {
public:
	//! Creates a new object and associates it with the given output if any.
	AspifTextInput(AbstractProgram* out);
	//! Sets the program to which parsed elements should be output.
	void setOutput(AbstractProgram& out);
protected:
	//! Checks whether stream starts with a valid token.
	virtual bool doAttach(bool& inc);
	//! Attempts to parses the current step or throws an exception on error.
	/*!
	 * The function calls beginStep()/endStep() on the associated
	 * output object before/after parsing the current step.
	 */
	virtual bool doParse();
	//! Parses statements until next step directive or input is exhausted.
	bool parseStatements();
private:
	void   skipws();
	bool   matchDirective();
	void   matchRule(char peek);
	void   matchAtoms(const char* seps);
	void   matchLits();
	void   matchCondition();
	void   matchAgg();
	bool   match(const char* ts, bool required = true);
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
	BasicStack       data_;
	uint32_t         strStart_;
	uint32_t         strPos_;
};

}
#endif
