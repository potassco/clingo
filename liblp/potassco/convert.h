// 
// Copyright (c) 2015, Benjamin Kaufmann
// 
// This file is part of Potassco.
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
#ifndef LIBLP_CONVERT_H_INCLUDED
#define LIBLP_CONVERT_H_INCLUDED

#include <potassco/smodels.h>
namespace Potassco {

//! Converts a given program so that it can be expressed in smodels format.
/*!
 * \ingroup WriteType
 */
class SmodelsConvert : public AbstractProgram {
public:
	//! Creates a new object that passes converted programs to out.
	/*!
	 * The parameter enableClaspExt determines how heuristic, edge, and external 
	 * directives are handled.
	 * If true, heuristic and edge directives are converted to _heuristic and
	 * _edge predicates, while external directives passed to out. 
	 * Otherwise, heuristic and edge directives are not converted but 
	 * directly passed to out, while external directives are mapped to
	 * choice rules or integrity constraints.
	 */
	SmodelsConvert(AbstractProgram& out, bool enableClaspExt);
	~SmodelsConvert();
	//! Calls initProgram() on the associated output program.
	virtual void initProgram(bool incremental);
	//! Calls beginStep() on the associated output program.
	virtual void beginStep();
	//! Converts the given rule into one or more smodels rules.
	virtual void rule(Head_t t, const AtomSpan& head, const LitSpan& body);
	//! Converts the given rule into one or more smodels rules.
	virtual void rule(Head_t t, const AtomSpan& head, Weight_t bound, const WeightLitSpan& body);
	//! Converts literals associated with a priority to a set of corresponding smodels minimize rules.
	virtual void minimize(Weight_t prio, const WeightLitSpan& lits);
	//! Adds an atom named str that is equivalent to the condition to the symbol table.
	virtual void output(const StringSpan& str, const LitSpan& cond);
	//! Marks the atom that is equivalent to a as external.
	virtual void external(Atom_t a, Value_t v);
	//! Adds an _heuristic predicate over the given atom to the symbol table that is equivalent to condition.
	virtual void heuristic(Atom_t a, Heuristic_t t, int bias, unsigned prio, const LitSpan& condition);
	//! Adds an _edge(s,t) predicate to the symbol table that is equivalent to condition.
	virtual void acycEdge(int s, int t, const LitSpan& condition);

	//! Finalizes conversion and calls endStep() on the associated output program.
	virtual void endStep();

	//! Returns the output literal associated to in.
	Lit_t       get(Lit_t in) const;
	//! Returns the name associated with the given (output) smodels atom or 0 if no name exists.
	const char* getName(Atom_t a) const;
	//! Returns the max used smodels atom (valid atoms are [1..n]).
	unsigned    maxAtom() const;
protected:
	//! Creates a (named) atom that is equivalent to the given condition.
	Atom_t makeAtom(const LitSpan& lits, bool named);
	//! Processes all outstanding conversions.
	void flush();
	//! Converts external atoms.
	void flushExternal();
	//! Converts minimize statements.
	void flushMinimize();
	//! Converts heuristic directives to _heuristic predicates.
	void flushHeuristic();
	//! Converts (atom,name) pairs to output directives.
	void flushSymbols();
private:
	SmodelsConvert(const SmodelsConvert&);
	SmodelsConvert& operator=(const SmodelsConvert&);
	struct SmData;
	AbstractProgram& out_;
	SmData*          data_;
	bool             ext_;
};

}
#endif
