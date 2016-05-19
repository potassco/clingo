// 
// Copyright (c) 2009, Benjamin Kaufmann
// 
// This file is part of Clasp. See http://www.cs.uni-potsdam.de/clasp/ 
// 
// Clasp is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// Clasp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Clasp; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
#ifndef CLASP_OUTPUT_H_INCLUDED
#define CLASP_OUTPUT_H_INCLUDED

#include <clasp/program_builder.h> // for PreproStats

namespace Clasp {
class Enumerator;

/*!
 * Base class for printing input format dependent information,
 * like models and program statistics.
 */
class OutputFormat {
public:
	OutputFormat();
	virtual ~OutputFormat();
	//! Pre-defined format keys
	enum FormtKey { 
		comment,    /**< Prefix to be used for outputting status information/comments */
		model,      /**< Prefix to be used for model lines */
		opt,        /**< Prefix to be used for optimization lines */ 
		solution,   /**< Prefix for the solution line */
		atom_sep,   /**< Suffix printed after each atom when printing consequences */
		end_format=5 
	};
	//! Possible results
	enum Result   { 
		sat,        /**< The problem is satisfiable (has at least one model) */
		optimum,    /**< The optimization problem is satisfiable and the optimal model was found */
		unsat,      /**< The problem was found to be unsat */
		unknwon,    /**< The satisfiability of the problem is still unknown */
		end_result=4 
	};
	const char* format[end_format]; /**< Default format strings */
	const char* result[end_result]; /**< Default result strings */

	//! Called once before solve
	virtual void initSolve(const Solver& s, ProgramBuilder* api, Enumerator* e);
	
	/*!
	 * Called for each model.
	 * \param s  The solver storing the current model
	 * \param en The active enumerator
	 */
	virtual void printModel(const Solver& s, const Enumerator& en) = 0;
	/*!
	 * Called after search has stopped.
	 * \param st The statistics of the last solve operation
	 * \param en The active enumerator
	 */
	virtual void printStats(const SolverStatistics& st, const Enumerator& en) = 0;
	
	/*!
	 * Prints the current consequences (marked atoms) in the format 
	 * format[model] (<atom>format[atom_sep] )*
	 */
	virtual void printConsequences(const Solver& s, const Enumerator& en);
	/*!
	 * Prints the current value(s) of the optimization function in the format
	 * format[opt] printOptimizeValues()
	 */
	virtual void printOptimize(const MinimizeConstraint& min);
	/*!
	 * Prints the solution line in the format format[solution] result[state]
	 * where state is:
	 *  - sat  : if s.stats.search.models > 0,
	 *  - opt  : if s.stats.search.models > 0 and complete and en.minimize() != 0,
	 *  - unsat: if s.stats.search.models = 0 and complete, otherwise
	 *  - unknwon.
	 */ 
	virtual void printSolution(const Solver& s, const Enumerator& en, bool complete);
	
	void         printOptimizeValues(const MinimizeConstraint& m);
protected:
	void printJumpStats(const SolverStatistics& st);
	inline double percent(uint64 r, uint64 b) { 
		if (b == 0) return 0;
		return (static_cast<double>(r)/b)*100.0;
	}
	inline double average(uint64 x, uint64 y) {
		if (!x || !y) return 0.0;
		return static_cast<double>(x) / static_cast<double>(y);
	}
private:
	OutputFormat(const OutputFormat&);
	OutputFormat& operator=(const OutputFormat&);
	const char* formt_[4];
};

//! Default ASP-format printer.
/*!
 * Prints either in clasp's default format or, if asp09 is true,
 * in ASP'09 competition format.
 * \see http://www.cs.kuleuven.be/~dtai/ASP-competition 
 */
class AspOutput : public OutputFormat {
public: 
	explicit AspOutput(bool asp09);
	void initSolve(const Solver& s, ProgramBuilder* api, Enumerator*);
	/*!
	 * Prints all named atoms of the symbol table that 
	 * are true in the current model.
	 * If asp09 is true, atoms are separated by a period otherwise by a space.
	 */
	void printModel(const Solver& s, const Enumerator&);
	/*!
	 * If asp09 is false, prints logic program, problem, and search statistics.
	 */
	void printStats(const SolverStatistics& st, const Enumerator& en);

	/*!
	 * If asp09 is false, prints "Optimization: " followed by the value(s) of
	 * the optimization function(s).
	 */
	void printOptimize(const MinimizeConstraint& min) {
		if (!asp09_) { OutputFormat::printOptimize(min); }
	}

	const PreproStats& programStats() const { return stats_; }
private:
	// Hook for derived classes
	virtual void printExtendedModel(const Solver&) {}
	PreproStats stats_;
	bool        asp09_;
};

//! Default printer for DIMACS problems
/*!
 * Prints in SAT-competition format.
 * \see http://www.satcompetition.org/2009/format-solvers2009.html
 */
class SatOutput : public OutputFormat {
public:
	SatOutput();
	void printModel(const Solver& s, const Enumerator&);
	/*!
	 * Prints problem and search statistics.
	 */
	void printStats(const SolverStatistics& st, const Enumerator& en);
};

//! Default printer for OPB-problems
/*!
 * Prints in PB-competition format.
 * \see http://www.cril.univ-artois.fr/PB09/solver_req.html
 */
class PbOutput : public SatOutput {
public: 
	explicit PbOutput(bool printSuboptimalModels);
	~PbOutput();
	void printModel(const Solver& s, const Enumerator& en);
	void printSolution(const Solver& s, const Enumerator& en, bool complete);
private:
	uint8* lastModel_;
	bool   printSuboptModels_;
};

}
#endif
