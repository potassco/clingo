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
#ifdef _MSC_VER
#pragma warning (disable : 4996) // std::copy unsafe
#endif
#include <potassco/match_basic_types.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <istream>
#include <algorithm>
namespace Potassco {
static std::string fmterror(const char* msg, unsigned line) {
	char buf[80];
	sprintf(buf, "parse error in line %u: ", line);
	return std::string(buf).append(msg);
}
AbstractProgram::~AbstractProgram() {}
void AbstractProgram::initProgram(bool) {}
void AbstractProgram::beginStep() {}
void AbstractProgram::project(const AtomSpan&) { throw std::logic_error("projection directive not supported"); }
void AbstractProgram::output(const StringSpan&, const LitSpan&) { throw std::logic_error("output directive not supported"); }
void AbstractProgram::external(Atom_t, Value_t) { throw std::logic_error("external directive not supported"); }
void AbstractProgram::assume(const LitSpan&) { throw std::logic_error("assumption directive not supported"); }
void AbstractProgram::heuristic(Atom_t, Heuristic_t, int, unsigned, const LitSpan&) { throw std::logic_error("heuristic directive not supported"); }
void AbstractProgram::acycEdge(int, int, const LitSpan&) { throw std::logic_error("edge directive not supported"); }
void AbstractProgram::theoryTerm(Id_t, int) { throw std::logic_error("theory data not supported"); }
void AbstractProgram::theoryTerm(Id_t, const StringSpan&)  { throw std::logic_error("theory data not supported"); }
void AbstractProgram::theoryTerm(Id_t, int, const IdSpan&) { throw std::logic_error("theory data not supported"); }
void AbstractProgram::theoryElement(Id_t, const IdSpan&, const LitSpan&) { throw std::logic_error("theory data not supported"); }
void AbstractProgram::theoryAtom(Id_t, Id_t, const IdSpan&) { throw std::logic_error("theory data not supported"); }
void AbstractProgram::theoryAtom(Id_t, Id_t, const IdSpan&, Id_t, Id_t) { throw std::logic_error("theory data not supported"); }
void AbstractProgram::endStep() {}
const StringSpan Heuristic_t::pred ={"_heuristic(", 11};
ParseError::ParseError(unsigned a_line, const char* a_msg) 
	: std::logic_error(fmterror(a_msg, a_line))
	, line(a_line) {
}
/////////////////////////////////////////////////////////////////////////////////////////
// BufferedStream
/////////////////////////////////////////////////////////////////////////////////////////
BufferedStream::BufferedStream(std::istream& str) : str_(str), rpos_(0), line_(1) {
	buf_ = new char[ALLOC_SIZE];
	underflow();
}
BufferedStream::~BufferedStream() {
	delete [] buf_;
}
char BufferedStream::rget() {
	char c = peek();
	if (!buf_[++rpos_]) { underflow(); }
	return c;
}
char BufferedStream::get() {
	if (char c = peek()) {
		rget();
		if (c == '\r') {
			c = '\n';
			if (peek() == '\n') rget();
		}
		if (c == '\n') { ++line_; }
		return c;
	}
	return 0;
}
void BufferedStream::skipWs() {
	for (char c; (c = peek()) >= 9 && c < 33;) { get(); }
}

void BufferedStream::underflow(bool upPos) {
	if (!str_) return;
	if (upPos && rpos_) {
		// keep last char for unget
		buf_[0] = buf_[rpos_ - 1];
		rpos_ = 1;
	}
	std::size_t n = ALLOC_SIZE - (1 + rpos_);
	str_.read(buf_ + rpos_, n);
	std::size_t r = static_cast<std::size_t>(str_.gcount());
	buf_[r + rpos_] = 0;
}
bool BufferedStream::unget(char c) {
	if (!rpos_) return false;
	if ( (buf_[--rpos_] = c) == '\n') { --line_; }
	return true;
}
bool BufferedStream::match(const char* w) {
	std::size_t wLen = std::strlen(w);
	std::size_t bLen = BUF_SIZE - rpos_;
	if (bLen < wLen) {
		if (wLen > BUF_SIZE) { throw std::logic_error("Token too long - Increase BUF_SIZE!"); }
		std::memcpy(buf_, buf_ + rpos_, bLen);
		rpos_ = bLen;
		underflow(false);
		rpos_ = 0;
	}
	if (std::strncmp(w, buf_ + rpos_, wLen) == 0) {
		if (!buf_[rpos_ += wLen]) { underflow(); }
		return true;
	}
	return false;
}
bool BufferedStream::match(int64_t& res, bool noSkipWs) {
	if (!noSkipWs) { skipWs(); }
	char s = peek();
	if (s == '+' || s == '-') { rget(); }
	if (!isDigit(peek())) { return false; }
	for (res = toDigit(rget()); isDigit(peek()); ) {
		res *= 10;
		res += toDigit(rget());
	}
	if (s == '-') { res = -res; }
	return true;
}
int BufferedStream::copy(char* out, int max) {
	if (max < 0) return max;
	std::size_t os = 0;
	for (std::size_t n = static_cast<std::size_t>(max); n && peek();) {
		std::size_t b = (ALLOC_SIZE - rpos_) - 1;
		std::size_t m = std::min(n, b);
		out = std::copy(buf_ + rpos_, buf_ + rpos_ + m, out);
		n -= m;
		os += m;
		rpos_ += m;
		if (!peek()) { underflow(); }
	}
	return static_cast<int>(os);
}
unsigned BufferedStream::line() const { return line_; }
/////////////////////////////////////////////////////////////////////////////////////////
// ProgramReader
/////////////////////////////////////////////////////////////////////////////////////////
ProgramReader::ProgramReader() : str_(0), varMax_(static_cast<unsigned>(INT_MAX)), inc_(false) {}
ProgramReader::~ProgramReader() { delete str_; }
bool ProgramReader::accept(std::istream& str) {
	reset();
	str_ = new StreamType(str);
	inc_ = false;
	return doAttach(inc_);
}
bool ProgramReader::incremental() const {
	return inc_;
}
bool ProgramReader::parse(ReadMode m) {
	if (str_ == 0)  { throw ParseError(1, "no input stream"); }
	do {
		if (!doParse()) { return false; }
		stream()->skipWs();
		require(!more() || incremental(), "invalid extra input");
	} while (m == ReadMode::Complete && more());
	return true;
}
bool ProgramReader::more() {
	return str_ && (str_->skipWs(), !str_->end());
}
void ProgramReader::reset() {
	delete str_; str_ = 0;
	doReset();
}
void ProgramReader::doReset() {}
unsigned ProgramReader::line()   const { return str_ ? str_->line() : 1; }
BufferedStream* ProgramReader::stream() const { return str_; }
bool ProgramReader::require(bool cnd, const char* msg) const { return cnd || (throw ParseError(line(), msg), false); }
char ProgramReader::peek(bool skipws) const { if (skipws) str_->skipWs(); return str_->peek(); }
void ProgramReader::skipLine() {
	while (str_->peek() && str_->get() != '\n') {}
}
int readProgram(std::istream& str, ProgramReader& reader, ErrorHandler err) {
	try {
		if (!reader.accept(str) || !reader.parse(ProgramReader::Complete)) { 
			throw ParseError(reader.line(), "invalid input format"); 
		}
	}
	catch (const ParseError& e) {
		if (!err) { throw; }
		return err(e.line, e.what());
	}
	catch (const std::exception& e) {
		if (!err) { throw; }
		return err(reader.line(), e.what());
	}
	return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////
// String matching
/////////////////////////////////////////////////////////////////////////////////////////
bool match(const char*& input, const char* word) {
	std::size_t len = std::strlen(word);
	if (std::strncmp(input, word, len) == 0) {
		input += len;
		return true;
	}
	return false;
}
bool matchAtomArg(const char*& input, StringSpan& arg) {
	const char* scan = input;
	for (int p = 0; *scan; ++scan) {
		if      (*scan == '(') { ++p; }
		else if (*scan == ')') { if (--p < 0) { break; } }
		else if (*scan == ',') { if (p == 0) { break; } }
		else if (*scan == '"') {
			bool quoted = false;
			for (++scan; *scan && (*scan != '\"' || quoted); ++scan) {
				quoted = !quoted && *scan == '\\';
			}
			if (!*scan) { return false; }
		}
	}
	arg = toSpan(input, static_cast<std::size_t>(scan - input));
	input = scan;
	return arg.size != 0;
}
bool match(const char*& input, Heuristic_t& heuType) {
	for (unsigned x = 0; x <= static_cast<unsigned>(Heuristic_t::eMax); ++x) {
		if (match(input, toString(static_cast<Heuristic_t>(x)))) {
			heuType = static_cast<Heuristic_t>(x);
			return true;
		}
	}
	return false;
}

bool match(const char*& input, int& out) {
	char* eptr;
	long t = std::strtol(input, &eptr, 10);
	if (eptr == input || t < INT_MIN || t > INT_MAX) {
		return false;
	}
	out = static_cast<int>(t);
	input = eptr;
	return true;
}

int matchDomHeuPred(const char*& in, StringSpan& atom, Heuristic_t& type, int& bias, unsigned& prio) {
	int p;
	if (!match(in, begin(Heuristic_t::pred))) { return 0; }
	if (!matchAtomArg(in, atom) || !match(in, ",")) { return -1; }
	if (!match(in, type)        || !match(in, ",")) { return -2; }
	if (!match(in, bias)) { return -3; }
	prio = static_cast<unsigned>(bias < 0 ? -bias : bias);
	if (!match(in, ",")) { return match(in, ")") ? 1 : -3; }
	if (!match(in, p) || p < 0) { return -4; }
	prio = static_cast<unsigned>(p);
	return match(in, ")") ? 1 : -4;
}

int matchEdgePred(const char*& in, StringSpan& n0, StringSpan& n1) {
	int sPos, tPos, ePos = -1;
	if (sscanf(in, "_acyc_%*d_%n%*d_%n%*d%n", &sPos, &tPos, &ePos) == 0 && ePos > 0) {
		n0 = toSpan(in + sPos, (tPos - sPos) - 1);
		n1 = toSpan(in + tPos, ePos - tPos);
		in += ePos;
		return size(n0) > 0 && size(n1) > 0 ? 1 : -1;
	}
	else if (match(in, "_edge(")) {
		if (!matchAtomArg(in, n0) || !match(in, ",")) { return -1; }
		if (!matchAtomArg(in, n1) || !match(in, ")")) { return -2; }
		return 1;
	}
	return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////
// Data stack
/////////////////////////////////////////////////////////////////////////////////////////
RawStack::RawStack() : mem_(0), top_(0), cap_(0) {}
RawStack::~RawStack() { std::free(mem_); }
RawStack::RawStack(const RawStack& other) {
	mem_ = (unsigned char*)std::malloc(other.top());
	std::memcpy(mem_, other.mem_, other.top());
	top_ = cap_ = other.top();
}
RawStack& RawStack::operator=(const RawStack& other) {
	RawStack(other).swap(*this);
	return *this;
}
void RawStack::swap(RawStack& other) {
	std::swap(mem_, other.mem_);
	std::swap(top_, other.top_);
	std::swap(cap_, other.cap_);
}
uint32_t RawStack::top() const {
	return top_;
}
uint32_t RawStack::capacity() const {
	return cap_;
}
void RawStack::clear() {
	top_ = 0;
}
void RawStack::reserve(uint32_t nc) {
	if (nc > capacity()) {
		unsigned char* t = (unsigned char*)std::realloc(mem_, nc);
		if (!t) throw std::bad_alloc();
		mem_ = t;
		cap_ = nc;
	}
}

void RawStack::setTop(uint32_t idx) {
	assert(idx <= cap_);
	top_ = idx;
}
uint32_t RawStack::pop_(uint32_t sz) {
	assert(sz <= top_);
	return top_ -= sz;
}
uint32_t RawStack::push_(uint32_t nSize) {
	uint32_t ret = top_;
	if ((top_ += nSize) >= ret) {
		if (top_ > cap_) {
			uint32_t nc = (capacity() * 3) >> 1;
			if (top_ > nc) { nc = top_ > 64u ? top_ : 64u; }
			reserve(nc);
		}
		return ret;
	}
	else {
		throw std::bad_alloc();
	}
}
void* RawStack::get(uint32_t idx) const {
	assert(idx <= cap_);
	return mem_ + idx;
}

void BasicStack::push(uint32_t obj) {
	uint32_t* x; push(&x, 1);
	*x = obj;
}
void BasicStack::push(WeightLit_t obj) {
	WeightLit_t* x; push(&x, 1);
	*x = obj;
}
template <class T>
void BasicStack::push(T** x, uint32_t len) {
	*x = (T*)get(push_(sizeof(T) * len));
}
void BasicStack::push(char** x, uint32_t len) {
	if (uint32_t mod = (len & (sizeof(uint32_t)-1))) {
		len += sizeof(uint32_t) - mod;
	}
	*x = (char*)get(push_(len));
}
template <class T>
void BasicStack::pop(const T** x, uint32_t len) {
	this->setTop(this->top() - (sizeof(T) * len));
	*x = (T*)this->get(this->top());
}
void BasicStack::pop(const char** x, uint32_t len) {
	if (uint32_t mod = (len & (sizeof(uint32_t)-1))) {
		len += sizeof(uint32_t) - mod;
	}
	setTop(top() - len);
	*x = (char*)this->get(top());
}
#define INSTANTIATE_STACK(T) \
template void BasicStack::push(T**, uint32_t);\
template void BasicStack::pop(const T**, uint32_t)

INSTANTIATE_STACK(uint32_t);
INSTANTIATE_STACK(int32_t);
INSTANTIATE_STACK(WeightLit_t);
#undef INSTANTIATE_STACK
}
