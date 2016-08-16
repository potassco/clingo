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
 * \addtogroup ParseType
 */
///@{
/*!
 * Parses the given program in asp intermediate format and calls ctx on each parsed element.
 * The error handler h is called on error. If h is 0, ParseError exceptions are used to signal errors.
 */
int readAspif(std::istream& prg, AbstractProgram& out, ErrorHandler h = 0);

//! Class for parsing logic programs in asp intermediate format.
class AspifInput : public ProgramReader {
public:
	//! Creates a new parser object that calls out on each parsed element.
	AspifInput(AbstractProgram& out);
	virtual ~AspifInput();
protected:
	//! Checks whether stream starts with aspif header.
	virtual bool doAttach(bool& inc);
	//! Parses the current step and throws exception on error.
	/*!
	 * The function calls beginStep()/endStep() on the associated
	 * output object before/after parsing the current step.
	 */
	virtual bool doParse();
	//! Attempts to parse a theory directive of type t.
	/*!
	 * \see Potassco::Theory_t
	 */
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
///@}

//! Writes a program in potassco's asp intermediate format to the given output stream.
/*!
 * \ingroup WriteType
 */
class AspifOutput : public AbstractProgram {
public:
	//! Creates a new object and associates it with the given output stream.
	AspifOutput(std::ostream& os);
	//! Writes an aspif header to the stream.
	virtual void initProgram(bool incremental);
	//! Prepares the object for a new program step.
	virtual void beginStep();
	//! Writes an aspif rule directive.
	virtual void rule(Head_t ht, const AtomSpan& head, const LitSpan& body);
	//! Writes an aspif rule directive.
	virtual void rule(Head_t ht, const AtomSpan& head, Weight_t bound, const WeightLitSpan& lits);
	//! Writes an aspif minimize directive.
	virtual void minimize(Weight_t prio, const WeightLitSpan& lits);
	//! Writes an aspif output directive.
	virtual void output(const StringSpan& str, const LitSpan& cond);
	//! Writes an aspif external directive.
	virtual void external(Atom_t a, Value_t v);
	//! Writes an aspif assumption directive.
	virtual void assume(const LitSpan& lits);
	//! Writes an aspif projection directive.
	virtual void project(const AtomSpan& atoms);
	//! Writes an aspif edge directive.
	virtual void acycEdge(int s, int t, const LitSpan& condition);
	//! Writes an aspif heuristic directive.
	virtual void heuristic(Atom_t a, Heuristic_t t, int bias, unsigned prio, const LitSpan& condition);
	
	//! Writes an aspif theory number term.
	virtual void theoryTerm(Id_t termId, int number);
	//! Writes an aspif theory symbolic term.
	virtual void theoryTerm(Id_t termId, const StringSpan& name);
	//! Writes an aspif theory compound term.
	virtual void theoryTerm(Id_t termId, int compound, const IdSpan& args);
	//! Writes an aspif theory element directive.
	virtual void theoryElement(Id_t elementId, const IdSpan& terms, const LitSpan& cond);
	//! Writes an aspif theory atom directive.
	virtual void theoryAtom(Id_t atomOrZero, Id_t termId, const IdSpan& elements);
	//! Writes an aspif theory atom directive with guard.
	virtual void theoryAtom(Id_t atomOrZero, Id_t termId, const IdSpan& elements, Id_t op, Id_t rhs);
	//! Writes the aspif step terminator.
	virtual void endStep();
protected:
	//! Starts writing an aspif directive.
	AspifOutput& startDir(Directive_t r);
	//! Writes x.
	AspifOutput& add(int x);
	//! Writes size(lits) followed by the elements in lits.
	AspifOutput& add(const WeightLitSpan& lits);
	//! Writes size(lits) followed by the literals in lits.
	AspifOutput& add(const LitSpan& lits);
	//! Writes size(atoms) followed by the atoms in atoms.
	AspifOutput& add(const AtomSpan& atoms);
	//! Writes size(str) followed by the characters in str.
	AspifOutput& add(const StringSpan& str);
	//! Terminates the active directive by writing a newline.
	AspifOutput& endDir();
private:
	AspifOutput(const AspifOutput&);
	AspifOutput& operator=(const AspifOutput&);
	std::ostream& os_;
};
}
#endif
