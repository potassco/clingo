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
#ifndef LIBLP_SMODELS_H_INCLUDED
#define LIBLP_SMODELS_H_INCLUDED
#include <potassco/match_basic_types.h>
namespace Potassco {

//! Returns the id of a basic smodels rule that corresponds to the given rule or 0 if it can't be expressed as one rule.
int isSmodelsRule(const HeadView& head, const BodyView& body);

class AtomTable {
public:
	virtual ~AtomTable();
	virtual void   add(Atom_t id, const StringSpan& name, bool output) = 0;
	virtual Atom_t find(const StringSpan& name) = 0;
};

class SmodelsInput : public ProgramReader {
public:
	//! Options for configuring reading of smodels format.
	struct Options {
		Options() : claspExt(false), cEdge(false), cHeuristic(false), filter(false) {}
		//! Enable clasp extensions for handling incremental programs.
		Options& enableClaspExt() { claspExt = true; return *this; }
		//! Convert _edge/_acyc_ atoms to edge directives.
		Options& convertEdges() { cEdge = true; return *this; }
		//! Convert _heuristic atoms to heuristic directives.
		Options& convertHeuristic() { cHeuristic = true; return *this; }
		//! Remove converted atoms from output.
		Options& dropConverted() { filter = true; return *this; }
		bool claspExt;
		bool cEdge;
		bool cHeuristic;
		bool filter;
	};
	SmodelsInput(AbstractProgram& out, const Options& opts, AtomTable* symTab = 0);
	virtual ~SmodelsInput();
protected:
	virtual bool doAttach(bool& inc);
	virtual bool doParse();
	virtual void doReset();

	virtual bool readRules();
	virtual bool readSymbols();
	virtual bool readCompute(const char* sec, bool val);
	virtual bool readExtra();
private:
	struct NodeTab;
	struct SymTab;
	AbstractProgram& out_;
	AtomTable*       atoms_;
	NodeTab*         nodes_;
	Options          opts_;
	bool             delSyms_;
};

// Parses the given program in smodels format and calls out on each parsed element.
// The error handler h is called on error. If h is 0, ParseError exceptions are used to signal errors.
int readSmodels(std::istream& prg, AbstractProgram& out, ErrorHandler h = 0, const SmodelsInput::Options& opts = SmodelsInput::Options());

//! Writes a program in smodels numeric format to the given output stream.
/*!
 * \note The class only supports program constructs that can be directly
 * expressed in smodels numeric format.
 */
class SmodelsOutput : public AbstractProgram {
public:
	SmodelsOutput(std::ostream& os, bool enableClaspExt);
	virtual void initProgram(bool);
	virtual void beginStep();
	//! Writes the given rule provided that isSmodelsRule(head, body) returns a non-zero value.
	virtual void rule(const HeadView& head, const BodyView& body);
	//! Writes the given minimize rule while ignoring its priority.
	virtual void minimize(Weight_t prio, const WeightLitSpan& lits);
	//! Writes the entry (a, str) to the symbol table provided that condition equals a.
	/*!
	 * \note Symbols shall only be added once after all rules were added.
	 */
	virtual void output(const StringSpan& str, const LitSpan& cond);
	//! Writes lits as a compute statement.
	/*!
	 * \note The function shall be called at most once per step and only after all rules and symbols were added.
	 */
	virtual void assume(const LitSpan& lits);
	//! Requires enableClaspExt or throws exception.
	virtual void external(Atom_t a, Value_t v);
	//! Not supported - throws exception.
	virtual void project(const AtomSpan& atoms);
	//! Not supported - throws exception.
	virtual void acycEdge(int s, int t, const LitSpan& condition);
	//! Not supported - throws exception.
	virtual void heuristic(Atom_t a, Heuristic_t t, int bias, unsigned prio, const LitSpan& condition);
	virtual void endStep();
protected:
	SmodelsOutput& startRule(int rt);
	SmodelsOutput& add(const HeadView& head);
	SmodelsOutput& add(int rt, const WeightLitSpan& lits, unsigned bnd);
	SmodelsOutput& add(unsigned i);
	SmodelsOutput& endRule();
	void require(bool cnd, const char* msg) const;

	bool incremental() const { return inc_; }
	bool extended()    const { return ext_; }
private:
	SmodelsOutput(const SmodelsOutput&);
	SmodelsOutput& operator=(const SmodelsOutput&);
	std::ostream& os_;
	int          sec_;
	bool         ext_;
	bool         inc_;
};

}
#endif
