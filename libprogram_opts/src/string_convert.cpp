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
#if defined(_MSC_VER) 
#pragma warning (disable : 4996)
#define snprintf _snprintf
#if _MSC_VER < 1700
inline unsigned long long strtoull(const char* str, char** endptr, int base) { return  _strtoui64(str, endptr, base); }
#endif
#endif

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

int xconvert(const char* x, bool& out, const char** errPos, int) {
	if      (empty(x, errPos))                { return 0; }
	else if (*x == '1')                       { out = true;  x += 1; }
	else if (*x == '0')                       { out = false; x += 1; }
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

int xconvert(const char* x, long& out, const char** errPos, int) {
	if (empty(x, errPos)) { return 0; }
	char* err;
	out     = strtol(x, &err, detectBase(x));
	if ((out == LONG_MAX || out == LONG_MIN) && errno == ERANGE) { err = (char*)x; }
	return parsed(err != x, err, errPos);
}

int xconvert(const char* x, unsigned long& out, const char** errPos, int) {
	if (empty(x, errPos)) { return 0; }
	char* err;
	if (strncmp(x, "umax", 4) == 0) {
		out = static_cast<unsigned long>(-1);
		err = (char*)x+4;
	}
	else if (strncmp(x, "-1", 2) == 0) {
		out = static_cast<unsigned long>(-1);
		err = (char*)x+2;
	}
	else if (*x != '-') {
		out = strtoul(x, &err, detectBase(x));
		if (out == ULONG_MAX && errno == ERANGE) { err = (char*)x; }
	}
	else { err = (char*)x; }
	return parsed(err != x, err, errPos);
}

int xconvert(const char* x, double& out, const char** errPos, int) {
	if (empty(x, errPos)) { return 0; }
	char* err;
	out = strtod(x, &err);
	return parsed(err != x, err, errPos);
}


int xconvert(const char* x, unsigned& out, const char** errPos, int) {
	unsigned long temp = 0;
	int tok = xconvert(x, temp, errPos, 0);
	if (tok == 0 || (temp > UINT_MAX && temp != static_cast<unsigned long>(-1))) {
		return parsed(0, x, errPos);
	}
	out = (unsigned)temp;
	return tok;	
}
int xconvert(const char* x, int& out, const char** errPos, int) {
	long temp = 0;
	int tok = xconvert(x, temp, errPos, 0);
	if (tok == 0 || temp < (long)INT_MIN || temp > (long)INT_MAX) {
		return parsed(0, x, errPos);
	}
	out = (int)temp;
	return tok;
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
string& xconvert(string& out, long n) {
	char buf[33];
	int w = snprintf(buf, 32, "%ld", n);
	if (w < 0 || w > 32) { w = 32; }
	return out.append(buf, static_cast<size_t>(w));
}
string& xconvert(string& out, unsigned long n) {
	if (n == static_cast<unsigned long>(-1)) {
		return out.append("umax");
	}
	char buf[33];
	int w = snprintf(buf, 32, "%lu", n);
	if (w < 0 || w > 32) { w = 32; }
	return out.append(buf, static_cast<size_t>(w));
}
string& xconvert(string& out, double d) {
	char buf[33];
	int w = snprintf(buf, 32, "%g", d);
	if (w < 0 || w > 32) { w = 32; }
	return out.append(buf, static_cast<size_t>(w));
}

#if defined(LLONG_MAX)
int xconvert(const char* x, long long& out, const char** errPos, int) {
	if (empty(x, errPos)) { return 0; }
	unsigned long long temp, max = static_cast<unsigned long long>(LLONG_MAX);
	bool neg = *x == '-';
	if (neg) { ++x; ++max; }
	int res = xconvert(x, temp, errPos, 0);
	if (res && temp <= max) {
		if (temp <= LLONG_MAX) { out = static_cast<long long>(temp); if (neg) out *= -1; }
		else                   { out = LLONG_MIN; }
		return res;
	}
	return parsed(0, x, errPos);
}
int xconvert(const char* x, unsigned long long& out, const char** errPos, int) {
	unsigned long temp;
	int t = xconvert(x, temp, errPos, 0);
	if (t) { out = temp; return t; }
	if (empty(x, errPos) || *x == '-') { return 0; }
	char* err;
	out = strtoull(x, &err, detectBase(x));
	return parsed(err != x, err, errPos);
}
string& xconvert(string& out, long long x) {
	long temp = static_cast<long>(x);
	if (temp == x) { return xconvert(out, temp); }
	if (x < 0) {
		out.append(1, '-');
		unsigned long long u;
		if (x != LLONG_MIN) { u = static_cast<unsigned long long>( -x ); }
		else                { u = static_cast<unsigned long long>( -(x+1) ) + 1; }
		return xconvert(out, u);
	}
	return xconvert(out, static_cast<unsigned long long>(x));
}
string& xconvert(string& out, unsigned long long x) {
	unsigned long temp = static_cast<unsigned long>(x);
	if (temp == x) { return xconvert(out, temp); }
	char buf[64];
	int w = snprintf(buf, 63, "%llu", x);
	if (w < 0 || w > 63) { w = 63; }
	return out.append(buf, static_cast<size_t>(w));
}
#endif

bad_string_cast::~bad_string_cast() throw() {}
const char* bad_string_cast::what() const throw() { return "bad_string_cast"; }

}
