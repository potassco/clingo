//
//  ProgramOptions
//  (C) Copyright Benjamin Kaufmann, 2004 - 2005
//  Permission to copy, use, modify, sell and distribute this software is 
//  granted provided this copyright notice appears in all copies. 
//  This software is provided "as is" without express or implied warranty, 
//  and with no claim as to its suitability for any purpose.
//
//  ProgramOptions is a scaled-down version of boost::program_options
//  see: http://boost-sandbox.sourceforge.net/program_options/html/
// 
#include <program_opts/value.h>
#include <algorithm>
#include <sstream>
#include <iterator>
#include <cctype>
using namespace std;
namespace ProgramOptions { namespace {
	struct ToLower {
		char operator()(char in) const { return (char)std::tolower(static_cast<unsigned char>(in)); }
	};
	struct ToUpper {
		char operator()(char in) const { return (char)std::toupper(static_cast<unsigned char>(in)); }
	};
}

bool parseValue(const std::string& s, bool& b, int)
{
	string copy = toLower(s);
	if (copy.empty() || copy == "true" || copy == "1" || copy == "yes" || copy == "on")
	{
		b = true;
		return true;
	}
	else if (copy == "false" || copy == "0" || copy == "no" || copy == "off")
	{
		b = false;
		return true;
	}
	return false;
}

bool parseValue(const std::string& s, std::string& r, int)
{
	r = s;
	return true;
}

Value<bool>* bool_switch(bool* b)
{
	Value<bool>* nv = value<bool>(b);
	nv->setImplicit();
	return nv;
}

std::string toLower(const std::string& s) {
	std::string ret; ret.reserve(s.size());
	std::transform(s.begin(), s.end(), std::back_inserter(ret), ToLower());
	return ret;
}

std::string toUpper(const std::string& s) {
	std::string ret; ret.reserve(s.size());
	std::transform(s.begin(), s.end(), std::back_inserter(ret), ToUpper());
	return ret;
}


}
