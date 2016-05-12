// 
// Copyright (c) Benjamin Kaufmann
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
#ifndef CLASP_PARSER_H_INCLUDED
#define CLASP_PARSER_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif

#include <istream>
#include <stdexcept>
#include <clasp/literal.h>
#include <clasp/claspfwd.h>
/*!
 * \file 
 * Contains parsers for supported input formats.
 */
namespace Clasp {

//! Instances of this class are thrown if a problem occurs during parsing input.
struct ParseError : public ClaspError {
	ParseError(unsigned line, const char* msg);
	unsigned line;
};

struct Input_t {
	//! Auto-detect input format of program given in prg.
	static InputFormat detectFormat(std::istream& prg);

	//! Reads a logic program in LPARSE-numeric format.
	/*!
	 * \param prg The stream containing the logic program.
	 * \param api The object in which to store the problem.
	 * \pre The api is ready, i.e. startProgram() was called.
	 */
	static bool parseLparse(std::istream& prg, Asp::LogicProgram& api);

	//! Reads a CNF/WCNF in simplified DIMACS-format.
	/*!
	 * \param prg The stream containing the CNF.
	 * \param api The object in which to store the problem.
	 * \pre The api is ready, i.e. startProgram() was called.
	 */
	static bool parseDimacs(std::istream& prg, SatBuilder& api);
	
	//! Reads a Pseudo-Boolean problem in OPB-format.
	/*!
	 * \param prg The stream containing the PB-problem.
	 * \param api The object in which to store the problem.
	 * \pre The api is ready, i.e. startProgram() was called.
	 */
	static bool parseOPB(std::istream& prg, PBBuilder& api);
};
/////////////////////////////////////////////////////////////////////////////////////////
// PARSING BASE
/////////////////////////////////////////////////////////////////////////////////////////
class StreamParser {
public:
	StreamParser();
	virtual ~StreamParser();
	bool parse(StreamSource& prg);
protected:
	StreamSource* input() const { return source_;  }
	bool          check(bool cond, const char* condError) const;
	virtual bool  doParse() = 0;
	bool          skipComments(const char* commentStr);
private:
	StreamSource* source_;
};
/////////////////////////////////////////////////////////////////////////////////////////
// LPARSE PARSING
/////////////////////////////////////////////////////////////////////////////////////////
class LparseParser : public StreamParser {
public:
	explicit LparseParser(Asp::LogicProgram& prg);
	void setProgram(Asp::LogicProgram& prg);
protected:
	virtual bool doParse();
	virtual bool parseRuleExtension(int ruleType) = 0;
	virtual bool endParse();
	Var  parseAtom();
	bool parseBody(uint32 lits, uint32 neg, bool weights);
	Asp::LogicProgram* builder() const { return builder_; }
	Asp::Rule*         active()  const { return active_;  }
	bool               addRule(const Asp::Rule& r) const;
private:
	bool parseRules();
	bool parseRule(int ruleType);
	bool parseSymbolTable();
	bool parseComputeStatement();
	bool parseExtStatement();
	bool parseModels();
	bool knownRuleType(int rt) { return rt > 0 && rt <= 8 && rt != 4 && rt != 7; }
	Asp::LogicProgram* builder_;
	Asp::Rule*         active_;
};
class DefaultLparseParser : public LparseParser {
public:
	DefaultLparseParser(Asp::LogicProgram& prg);
protected:
	// Throws ParseError
	bool parseRuleExtension(int ruleType);
};
/////////////////////////////////////////////////////////////////////////////////////////
// DIMACS PARSING
/////////////////////////////////////////////////////////////////////////////////////////
class DimacsParser : public StreamParser {
public:
	explicit DimacsParser(SatBuilder& prg);
	void     setProgram(SatBuilder& prg);
protected:
	bool doParse();
private:
	void parseHeader();
	void parseClauses();
	SatBuilder* builder_;
	int         numVar_;
	bool        wcnf_;
};
/////////////////////////////////////////////////////////////////////////////////////////
// OPB PARSING
/////////////////////////////////////////////////////////////////////////////////////////
class OPBParser : public StreamParser {
public:
	explicit OPBParser(PBBuilder& prg);
	void setProgram(PBBuilder& prg);
protected:
	bool doParse();
private:
	void parseHeader();
	void parseOptObjective();
	void parseConstraint();
	void parseSum();
	void parseTerm();
	PBBuilder* builder_;
	weight_t   minCost_;
	weight_t   maxCost_;
	struct Constraint {
		WeightLitVec lits;
		weight_t     bound;
		bool         eq;
	}          active_;
	LitVec     term_;
};
/////////////////////////////////////////////////////////////////////////////////////////
// StreamSource
/////////////////////////////////////////////////////////////////////////////////////////
//! Wrapps an std::istream and provides basic functions for extracting numbers and strings.
class StreamSource {
public:
	explicit StreamSource(std::istream& is);
	//! Returns the character at the current reading-position.
	char operator*() {
		if (buffer_[pos_] == 0) { underflow(); }
		return buffer_[pos_];
	}
	//! Advances the current reading-position.
	StreamSource& operator++() { ++pos_; **this; return *this; }
	
	//! Reads a base-10 integer.
	/*!
	 * \pre system uses ASCII
	 */
	bool parseInt(int& val);
	bool parseInt64(int64& val);
	bool parseInt(int& val, int min, int max);
	int  parseInt(int min, int max, const char* err) {
		int x;
		return parseInt(x, min, max) ? x : (error(err ? err : "Integer expected!"), 0);
	}

	//! Consumes next character if equal to c.
	bool match(char c) { return (**this == c) && (++*this, true); }
	//! Consumes next character(s) if equal to EOL.
	/*!
	 * Consumes the next character if it is either '\n' or '\r'
	 * and increments the internal line counter.
	 * 
	 * \note If next char is '\r', the function will also consume
	 * a following '\n' (i.e. matchEol also matches CR/LF).
	 */
	bool matchEol();
	//! Skips horizontal white-space.
	bool skipSpace() { while (match(' ') || match('\t')) { ; } return true; }
	//! Skips horizontal and vertical white-space.
	bool skipWhite() { do { skipSpace(); } while (matchEol()); return true; }
	//! Returns the number of matched EOLs + 1.
	unsigned line() const { return line_; }

	void error(const char* err) const { throw ParseError(line(), err); }
private:
	StreamSource(const std::istream&);
	StreamSource& operator=(const StreamSource&);
	void underflow();
	char buffer_[2048];
	std::istream& in_;
	unsigned pos_;
	unsigned line_;
};
//! Skips the current line.
inline void skipLine(StreamSource& in) { while (*in && !in.matchEol()) { ++in; } }

//! Consumes next character if equal to c.
/*!
 * \param in StreamSource from which characters should be read
 * \param c character to match
 * \param sw skip leading spaces
 * \return
 *  - true if character c was consumed
 *  - false otherwise
 *  .
 */
inline bool match(StreamSource& in, char c, bool sw) { return (!sw || in.skipSpace()) && in.match(c); }

//! Consumes string str.
/*!
 * \param in StreamSource from which characters should be read
 * \param str string to match
 * \param sw skip leading spaces
 * \pre   str != 0
 * \return
 *  - true if string str was consumed
 *  - false otherwise
 *  .
 */
inline bool match(StreamSource& in, const char* str, bool sw) {
	if (sw) in.skipSpace();
	while (*str && in.match(*str)) { ++str; }
	return *str == 0;
}

inline bool matchEol(StreamSource& in, bool sw) {
	if (sw) in.skipSpace();
	return in.matchEol();
}

//! Extracts characters from in and stores them into buf until a newline character or eof is found.
/*!
 * \note    The newline character is extracted and discarded, i.e. it is not stored and the next input operation will begin after it.
 * \return  True if a newline was found, false on eof
 * \post    buf.back() == '\0'
 */
bool readLine( StreamSource& in, PodVector<char>::type& buf );

}
#endif
