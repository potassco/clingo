// 
// Copyright (c) 2015, Benjamin Kaufmann
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
#ifndef LIBLP_ASPIF_H_INCLUDED
#define LIBLP_ASPIF_H_INCLUDED
#include <potassco/match_basic_types.h>
namespace Potassco {
/*!
 * Parses the given program in asp intermediate format and calls ctx on each parsed element.
 * The error handler h is called on error. If h is 0, ParseError exceptions are used to signal errors.
 */
int readAspif(std::istream& prg, AbstractProgram& out, ErrorHandler h = 0);

class AspifInput : public ProgramReader {
public:
	AspifInput(AbstractProgram& out);
	virtual ~AspifInput();
protected:
	virtual bool doAttach(bool& inc);
	virtual bool doParse();
	virtual void matchTheory(unsigned t);
private:
	uint32_t matchAtoms();
	uint32_t matchLits();
	uint32_t matchWLits(int32_t minW);
	uint32_t matchString();
	uint32_t matchTermList();
	AbstractProgram& out_;
	BasicStack*      data_;
};

//! Writes a program in potassco's asp intermediate format to the given output stream.
class AspifOutput : public AbstractProgram {
public:
	AspifOutput(std::ostream& os);
	virtual void initProgram(bool incremental);
	virtual void beginStep();
	virtual void rule(Head_t ht, const AtomSpan& head, const LitSpan& body);
	virtual void rule(Head_t ht, const AtomSpan& head, Weight_t bound, const WeightLitSpan& lits);
	virtual void minimize(Weight_t prio, const WeightLitSpan& lits);
	virtual void output(const StringSpan& str, const LitSpan& cond);
	virtual void external(Atom_t a, Value_t v);
	virtual void assume(const LitSpan& lits);
	virtual void project(const AtomSpan& atoms);
	virtual void acycEdge(int s, int t, const LitSpan& condition);
	virtual void heuristic(Atom_t a, Heuristic_t t, int bias, unsigned prio, const LitSpan& condition);
	
	virtual void theoryTerm(Id_t termId, int number);
	virtual void theoryTerm(Id_t termId, const StringSpan& name);
	virtual void theoryTerm(Id_t termId, int compound, const IdSpan& args);
	virtual void theoryElement(Id_t elementId, const IdSpan& terms, const LitSpan& cond);
	virtual void theoryAtom(Id_t atomOrZero, Id_t termId, const IdSpan& elements);
	virtual void theoryAtom(Id_t atomOrZero, Id_t termId, const IdSpan& elements, Id_t op, Id_t rhs);
	virtual void endStep();
protected:
	AspifOutput& startDir(Directive_t r);
	AspifOutput& add(int x);
	AspifOutput& add(const WeightLitSpan& lits);
	AspifOutput& add(const LitSpan& lits);
	AspifOutput& add(const AtomSpan& atoms);
	AspifOutput& add(const StringSpan& str);
	AspifOutput& endDir();
private:
	AspifOutput(const AspifOutput&);
	AspifOutput& operator=(const AspifOutput&);
	std::ostream& os_;
};
}
#endif
