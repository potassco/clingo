// 
// Copyright (c) 2006-2007, Benjamin Kaufmann
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
#ifndef CLASP_READER_H_INCLUDED
#define CLASP_READER_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif

#include <istream>
#include <stdexcept>
#include <clasp/literal.h>
/*!
 * \file 
 * Defines basic functions and classes for program input.
 */
namespace Clasp {
class ProgramBuilder;
class Solver;
class MinimizeConstraint;
struct ObjectiveFunction {
	WeightLitVec lits;
	int32        adjust;  
};

class Input {
public:
	enum Format {SMODELS, DIMACS, OPB};
	Input() {}
	virtual ~Input() {}
	virtual Format format() const = 0;
	virtual bool   read(Solver& s, ProgramBuilder* api, int numModels) = 0;
	virtual MinimizeConstraint* getMinimize(Solver& s, ProgramBuilder* api, bool heu) = 0;
	virtual void   getAssumptions(LitVec& a) = 0;
private:
	Input(const Input&);
	Input& operator=(const Input&);
};

class StreamInput : public Input {
public:
	explicit StreamInput(std::istream& in, Format f);
	Format format() const { return format_; }
	bool   read(Solver& s, ProgramBuilder* api, int numModels);
	MinimizeConstraint* getMinimize(Solver& s, ProgramBuilder* api, bool heu);
	void   getAssumptions(LitVec&) {}
private:
	ObjectiveFunction func_;
	std::istream&     prg_;
	Format            format_;
};


//! Auto-detect input format of problem
Input::Format detectFormat(std::istream& prg);

//! Reads a logic program in SMODELS-input format
/*!
 * \ingroup problem
 * \param prg The stream containing the logic program
 * \param api The ProgramBuilder object to use for program creation.
 * \pre The api is ready, i.e. startProgram() was called
 */
bool parseLparse(std::istream& prg, ProgramBuilder& api);

//! Reads a CNF in simplified DIMACS-format
/*!
 * \ingroup problem
 * \param prg The stream containing the CNF
 * \param s The Solver to use for solving the problem 
 * \param assertPure If true, pure literals are asserted
 */
bool parseDimacs(std::istream& prg, Solver& s, bool assertPure);

//! Reads a Pseudo-Boolean problem in linear OPB-format
/*!
 * \ingroup problem
 * \param prg The stream containing the PB-problem
 * \param s The Solver to use for solving the problem 
 * \param objective An out parameter that contains an optional objective function. 
 */
bool parseOPB(std::istream& prg, Solver& s, ObjectiveFunction& objective);


//! Instances of this class are thrown if a problem occurs during reading the input
struct ReadError : public ClaspError {
	ReadError(unsigned line, const char* msg);
	static std::string format(unsigned line, const char* msg);
	unsigned line_;
};

//! Wrapps an std::istream and provides basic functions for extracting numbers and strings.
class StreamSource {
public:
	explicit StreamSource(std::istream& is);
	//! returns the character at the current reading-position
	char operator*();
	//! advances the current reading-position
	StreamSource& operator++();
	//! returns the line number of the current reading-position
	unsigned line() const { return line_; }
	
	//! reads a base-10 integer
	/*!
	 * \pre system uses ASCII
	 */
	bool parseInt(int& val);
	//! skips horizontal white-space
	void skipWhite();
	//! works like std::getline
	bool readLine( char* buf, unsigned maxSize );
private:
	StreamSource(const std::istream&);
	StreamSource& operator=(const StreamSource&);
	void underflow();
	char buffer_[2048];
	std::istream& in_;
	unsigned pos_;
	unsigned line_;
};

//! skips horizontal and vertical white-space
inline bool skipAllWhite(StreamSource& in) {
	do { in.skipWhite(); } while (*in == '\n' && *++in != 0);
	return true;
}

//! skips the current line
inline void skipLine(StreamSource& in) {
	while (*in && *in != '\n') ++in;
	if (*in) ++in;
}

//! consumes next character if equal to c
/*!
 * \param in StreamSource from which characters should be read
 * \param c character to match
 * \param sw skip leading white-space
 * \return
 *  - true if character c was consumed
 *  - false otherwise
 *  .
 */
inline bool match(StreamSource& in, char c, bool sw) {
	if (sw) in.skipWhite();
	if (*in == c) {
		++in;
		return true;
	}
	return false;
}

//! consumes string str 
/*!
 * \param in StreamSource from which characters should be read
 * \param str string to match
 * \param sw skip leading white-space
 * \return
 *  - true if string str was consumed
 *  - false otherwise
 *  .
 */
inline bool match(StreamSource& in, const char* str, bool sw) {
	if (sw) in.skipWhite();
	for (; *str && *in && *str == *in; ++str, ++in) {;}
	return *str == 0;
}

//! extracts characters from in and stores them into buf until a newline character or eof is found
/*!
 * \note    The newline character is extracted and discarded, i.e. it is not stored and the next input operation will begin after it.
 * \return  True if a newline was found, false on eof
 * \post    buf.back() == '\0'
 */
bool readLine( StreamSource& in, PodVector<char>::type& buf );

}
#endif
