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
#ifndef LIBLP_MATCH_BASIC_TYPES_H
#define LIBLP_MATCH_BASIC_TYPES_H
#include <potassco/basic_types.h>
#include <stdexcept>
#include <climits>
#include <iosfwd>
#include <stdint.h>
namespace Potassco {

class ParseError : public std::logic_error {
public:
	ParseError(unsigned a_line, const char* msg);
	unsigned line;
};

class BufferedStream {
public:
	enum { BUF_SIZE = 4096 };
	explicit BufferedStream(std::istream& str);
	~BufferedStream();
	char peek() const { return buf_[rpos_]; }
	bool end()  const { return peek() == 0; }
	char get();
	bool unget(char c);
	bool match(int64_t& i, bool noSkipWs = false);
	bool match(const char* tok);
	void skipWs();
	int  copy(char* bufferOut, int max);
	static inline int  toDigit(char c) { return static_cast<int>(c - '0'); }
	static inline bool isDigit(char c) { return c >= '0' && c <= '9'; } 
	unsigned line() const;
private:
	char rget();
	enum { ALLOC_SIZE = BUF_SIZE + 1 };
	BufferedStream& operator=(const BufferedStream&);
	void underflow(bool up = true);
	typedef char* BufferType;
	std::istream& str_;
	BufferType    buf_;
	std::size_t   rpos_;
	unsigned      line_;
};

inline bool match(BufferedStream& str, const char* word, bool skipWs) {
	if (skipWs) str.skipWs();
	return str.match(word);
}
inline int matchInt(BufferedStream& str, int min = INT_MIN, int max = INT_MAX, const char* err = "integer expected") {
	int64_t x;
	if (str.match(x) && x >= min && x <= max) { return static_cast<int>(x); }
	throw ParseError(str.line(), err);
}
inline unsigned matchPos(BufferedStream& str, unsigned max, const char* err) {
	int64_t x;
	if (str.match(x) && x >= 0 && static_cast<uint64_t>(x) <= max) { return static_cast<unsigned>(x); }
	throw ParseError(str.line(), err);
}
inline unsigned matchPos(BufferedStream& str, const char* err = "non-negative integer expected") {
	return matchPos(str, static_cast<unsigned>(-1), err);
}
inline Atom_t matchAtom(BufferedStream& str, unsigned aMax = atomMax, const char* err = "atom expected") {
	int64_t x; int64_t max = static_cast<int64_t>(aMax);
	if (str.match(x) && x >= atomMin && x <= max) { return static_cast<Atom_t>(x); }
	throw ParseError(str.line(), err);
}
inline Lit_t matchLit(BufferedStream& str, unsigned aMax = atomMax, const char* err = "literal expected") {
	int64_t x; int64_t max = static_cast<int64_t>(aMax);
	if (str.match(x) && x != 0 && x >= -max && x <= max) { return static_cast<Lit_t>(x); }
	throw ParseError(str.line(), err);
}
inline WeightLit_t matchWLit(BufferedStream& str, unsigned aMax = atomMax, Weight_t minW = 0, const char* err = "weight literal expected") {
	WeightLit_t wl;
	wl.lit    = matchLit(str, aMax, err);
	wl.weight = matchInt(str, minW, INT_MAX, "invalid weight literal weight");
	return wl;
}
class BasicStack : public RawStack {
public:
	void push(int32_t x) { push(static_cast<uint32_t>(x)); }
	void push(uint32_t x);
	void push(WeightLit_t x);
	template <class T> T       pop() { const T* x;  pop(&x, 1); return *x; }
	template <class T> T*      makeSpan(uint32_t len) { T* x; push(&x, len); return x; }
	template <class T> Span<T> popSpan(uint32_t len) { const T* x; pop(&x, len); return toSpan(x, len); }
protected:
	template <class T>
	void pop(const T**, uint32_t len);
	void pop(const char**, uint32_t len);
	template <class T>
	void push(T**, uint32_t len);
	void push(char**, uint32_t len);
};
class ProgramReader {
public:
	ProgramReader();
	virtual ~ProgramReader();
	bool accept(std::istream& str);
	bool incremental() const;
	bool parse();
	bool more();
	void reset();
	unsigned line() const;
	void setMaxVar(unsigned v) { varMax_ = v; }
protected:
	typedef BufferedStream StreamType;
	typedef WeightLit_t WLit_t;
	virtual bool doAttach(bool& inc) = 0;
	virtual bool doParse() = 0;
	virtual void doReset();
	StreamType*  stream() const;
	char     peek(bool skipws) const;
	bool     require(bool cnd, const char* msg) const;
	bool     match(const char* word, bool skipWs = true) { return Potassco::match(*stream(), word, skipWs); }
	int      matchInt(int min = INT_MIN, int max = INT_MAX, const char* err = "integer expected") { return Potassco::matchInt(*stream(), min, max, err); }
	unsigned matchPos(unsigned max = static_cast<unsigned>(-1), const char* err = "unsigned integer expected") { return Potassco::matchPos(*stream(), max, err); }
	unsigned matchPos(const char* err) { return matchPos(static_cast<unsigned>(-1), err); }
	Atom_t   matchAtom(const char* err = "atom expected") { return Potassco::matchAtom(*stream(), varMax_, err); }
	Lit_t    matchLit(const char* err = "literal expected") { return Potassco::matchLit(*stream(), varMax_, err); }
	WLit_t   matchWLit(int min, const char* err = "weight literal expected") { return Potassco::matchWLit(*stream(), varMax_, min, err); }
	void     skipLine();
private:
	ProgramReader(const ProgramReader&);
	ProgramReader& operator=(const ProgramReader&);
	StreamType* str_;
	unsigned    varMax_;
	bool        inc_;
};

bool match(const char*& input, const char* word);
bool matchAtomArg(const char*& input, StringSpan& arg);
bool match(const char*& input, Heuristic_t& heuType);
bool match(const char*& input, int& out);
//! Tries to match potassco's special _heuristic/3 or _heuristic/4 predicate.
/*!
 * \return > 1 on match, 0 if in does not start with _heuristic, < 0 if in has wrong arity or parameters.
 */
int  matchDomHeuPred(const char*& in
	, StringSpan&  atom
	, Heuristic_t& type
	, int&         bias
	, unsigned&    prio);
//! Tries to match an _edge/2 or _acyc_/0 predicate.
int matchEdgePred(const char*& in, StringSpan& n0, StringSpan& n1);

int readProgram(std::istream& str, ProgramReader& r, ErrorHandler err);
}

#endif
