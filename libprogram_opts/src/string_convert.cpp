//
//  Copyright (c) Benjamin Kaufmann
//
//  This is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version. 
// 
//  This file is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <http://www.gnu.org/licenses/>.
//
#include <program_opts/string_convert.h>
#include <cstdlib>
#include <climits>
#include <cerrno>
#include <cstdio>
#include <cstdarg>
#if defined(_MSC_VER) 
#pragma warning (disable : 4996)
#define snprintf _snprintf
#if _MSC_VER < 1700
inline unsigned long long strtoull(const char* str, char** endptr, int base) { return  _strtoui64(str, endptr, base); }
inline long long strtoll(const char* str, char** endptr, int base) { return  _strtoi64(str, endptr, base); }
#endif
#endif

#ifdef _WIN32
typedef _locale_t  locale_t;
#define strtod_l   _strtod_l
#define freelocale _free_locale
inline locale_t    default_locale() { return _create_locale(LC_ALL, "C"); }
#else
#include <xlocale.h>
inline locale_t    default_locale() { return newlocale(LC_ALL_MASK, "C", 0); }
#endif
static struct LocaleHolder {
	~LocaleHolder() { freelocale(loc_);  }
	operator locale_t() const { return loc_; }
	locale_t loc_;
} default_locale_g = { default_locale() };

using namespace std;

namespace bk_lib { 

static int detectBase(const char* x) {
	if (x[0] == '0') {
		if (x[1] == 'x' || x[1] == 'X') return 16;
		if (x[1] >= '0' && x[1] <= '7') return 8;
	}
	return 10;
}

static bool empty(const char* x, const char** errPos) {
	if (x && *x) return false;
	if (errPos) { *errPos = x; }
	return true;
}

static int parsed(int tok, const char* end, const char** errPos) {
	if (errPos) *errPos = end;
	return tok;
}

static bool parseSigned(const char*& x, long long& out, long long sMin, long long sMax) {
	if (!x || !*x) {
		return false;
	}
	if ((std::strncmp(x, "imax", 4) == 0 && (out = sMax) != 0) || (std::strncmp(x, "imin", 4) == 0 && (out = sMin) != 0)) {
		x += 4;
		return true;
	}
	const char* safe = x;
	char* err;
	out = strtoll(x, &err, detectBase(x));
	if (err == safe) { return false; }
	if ((out == LLONG_MAX || out == LLONG_MIN) && errno == ERANGE) { x = safe; return false; }
	if (out < sMin || out > sMax) { x = safe; return false; }
	x = err;
	return true;
}

static bool parseUnsigned(const char*& x, unsigned long long& out, unsigned long long uMax) {
	if (!x || !*x || (*x == '-' && x[1] != '1')) {
		return false;
	}
	std::size_t len = 4;
	if (std::strncmp(x, "imax", len) == 0 || std::strncmp(x, "umax", len) == 0 || std::strncmp(x, "-1", len=2) == 0) {
		out = *x != 'i' ? uMax : uMax >> 1;
		x  += len;
		return true;
	}
	const char* safe = x;
	char* err;
	out = strtoull(x, &err, detectBase(x));
	if (out == ULLONG_MAX && errno == ERANGE) { err = (char*)x; }
	if (out > uMax) { x = safe; return false; }
	x = err;
	return true;
}

static std::string& append_number(std::string& out, const char* fmt, ...) {
	char buf[33];
	va_list args;
	va_start(args, fmt);
	int w = vsnprintf(buf, 32, fmt, args);	
	va_end(args);
	if (w < 0 || w > 32) { w = 32; }
	return out.append(buf, static_cast<size_t>(w));
}

int xconvert(const char* x, bool& out, const char** errPos, int) {
	if      (empty(x, errPos))           { return 0; }
	else if (*x == '1')                  { out = true;  x += 1; }
	else if (*x == '0')                  { out = false; x += 1; }
	else if (strncmp(x, "no", 2)  == 0)  { out = false; x += 2; }
	else if (strncmp(x, "on", 2)  == 0)  { out = true;  x += 2; }
	else if (strncmp(x, "yes", 3) == 0)  { out = true;  x += 3; }
	else if (strncmp(x, "off", 3) == 0)  { out = false; x += 3; }
	else if (strncmp(x, "true", 4) == 0) { out = true;  x += 4; }
	else if (strncmp(x, "false", 5) == 0){ out = false; x += 5; }
	return parsed(1, x, errPos);
}
int xconvert(const char* x, char& out, const char** errPos, int) {
	if (empty(x, errPos))     { return 0; }
	if ((out = *x++) == '\\') {
		switch(*x) {
			case 't': out = '\t'; ++x; break;
			case 'n': out = '\n'; ++x; break;
			case 'v': out = '\v'; ++x; break;
			default: break;
		}
	}
	return parsed(1, x, errPos);
}
int xconvert(const char* x, int& out, const char** errPos, int) {
	long long temp;
	if (parseSigned(x, temp, INT_MIN, INT_MAX)) {
		out = static_cast<int>(temp);
		return parsed(1, x, errPos);
	}
	return parsed(0, x, errPos);
}

int xconvert(const char* x, unsigned& out, const char** errPos, int) {
	unsigned long long temp = 0;
	if (parseUnsigned(x, temp, UINT_MAX)) {
		out = static_cast<unsigned>(temp);
		return parsed(1, x, errPos);
	}
	return parsed(0, x, errPos);
}

int xconvert(const char* x, long& out, const char** errPos, int) {
	long long temp;
	if (parseSigned(x, temp, LONG_MIN, LONG_MAX)) {
		out = static_cast<long>(temp);
		return parsed(1, x, errPos);
	}
	return parsed(0, x, errPos);
}

int xconvert(const char* x, unsigned long& out, const char** errPos, int) {
	unsigned long long temp = 0;
	if (parseUnsigned(x, temp, ULONG_MAX)) {
		out = static_cast<unsigned long>(temp);
		return parsed(1, x, errPos);
	}
	return parsed(0, x, errPos);
}

#if defined(LLONG_MAX)
int xconvert(const char* x, long long& out, const char** errPos, int) {
	int tok = parseSigned(x, out, LLONG_MIN, LLONG_MAX);
	return parsed(tok, x, errPos);
}
int xconvert(const char* x, unsigned long long& out, const char** errPos, int) {
	int tok = parseUnsigned(x, out, ULLONG_MAX);
	return parsed(tok, x, errPos);
}
#endif

int xconvert(const char* x, double& out, const char** errPos, int) {
	if (empty(x, errPos)) { return 0; }
	char* err;
	out = strtod_l(x, &err, default_locale_g);
	return parsed(err != x, err, errPos);
}

int xconvert(const char* x, const char*& out, const char** errPos, int) {
	out = x;
	if (errPos) { *errPos = x + strlen(x); }
	return 1;
}
int xconvert(const char* x, string& out, const char** errPos, int sep) {
	const char* end;
	if (sep == 0 || (end = strchr(x, char(sep))) == 0) {
		out = x;
	}
	else {
		out.assign(x, end - x);
	}
	if (errPos) { *errPos = x + out.length(); }
	return 1;
}

string& xconvert(string& out, bool b) { return out.append(b ? "true": "false"); }
string& xconvert(string& out, char c) { return out.append(1, c); }
string& xconvert(string& out, int n)  { return xconvert(out,static_cast<long>(n)); }
string& xconvert(string& out, unsigned int n) { return n != static_cast<unsigned int>(-1) ? xconvert(out, static_cast<unsigned long>(n)) : out.append("umax"); }
string& xconvert(string& out, long n) { return append_number(out, "%ld", n); }
string& xconvert(string& out, unsigned long n) {
	return n != static_cast<unsigned long>(-1) ? append_number(out, "%lu", n) : out.append("umax");
}

#if defined(LLONG_MAX)
string& xconvert(string& out, long long n) {
	return append_number(out, "%lld", n);
}

string& xconvert(string& out, unsigned long long n) {
	return n != static_cast<unsigned long long>(-1) 
		? append_number(out, "%llu", n)
		: out.append("umax");
}
#endif

string& xconvert(string& out, double d) {
	return append_number(out, "%g", d);
}

bad_string_cast::~bad_string_cast() throw() {}
const char* bad_string_cast::what() const throw() { return "bad_string_cast"; }

}
