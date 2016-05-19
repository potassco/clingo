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
#ifndef PROGRAM_OPTIONS_VALUE_H_INCLUDED
#define PROGRAM_OPTIONS_VALUE_H_INCLUDED
#ifdef _MSC_VER
#pragma warning (disable : 4786)
#pragma warning (disable : 4503)
#endif
#include "program_options.h"
#include "value_base.h"
#include "errors.h"
#include <typeinfo>
#include <string>
#include <memory>
#include <sstream>
#include <vector>
#include <map>
#include <stdexcept>
#include <cctype>
namespace ProgramOptions {
//! typed value of an option
template <class T>
class Value : public ValueBase {
public:
	typedef bool (*custom_parser)(const std::string&, T&);
	typedef T value_type;
	/*!
	* \param storeTo where to store the value once it is known.
	*/
	Value(T* storeTo = 0)
		: value_(storeTo)
		, parser_(0)
		, implicit_(false)
		, composing_(false)
		, hasValue_(false)
	{}

	bool hasValue()     const   {return hasValue_; }
	bool isImplicit()   const   {return implicit_;}
	bool isComposing()  const   {return composing_;}
	const T& value()    const {
		if (!value_) throw BadValue("no value");
		return *value_;
	}
	T& value() {
		if (!value_) throw BadValue("no value");
		return *value_;
	}
	
	//! marks this value as implicit
	Value<T>* setImplicit() { implicit_ = true; return this; }
	//! marks this value as composing
	Value<T>* setComposing() { composing_ = true; return this; }

	Value<T>* parser(custom_parser pf) { parser_ = pf; return this; }  
	
	bool parse(const std::string& s);

	//! sets a default value for this value
	virtual Value<T>* defaultValue(const T& t) = 0;
protected:
	value_type*   value_;
	custom_parser parser_;
	bool          implicit_; // can be initialized from an empty string?
	bool          composing_;// multiple values allowed?
	bool          hasValue_; // true, once a value was parsed
private:
	Value<T>(const Value<T>&);
	Value<T>& operator=(const Value<T>&);
};

///////////////////////////////////////////////////////////////////////////////
// value parsing functions
///////////////////////////////////////////////////////////////////////////////
template <class T>
bool parseValue(const std::string& s, T& t, double)
{
	std::stringstream str;
	str << s;
	if ( (str>>t) ) {
		std::char_traits<char>::int_type c;
		for (c = str.get(); std::isspace(c); c = str.get()) {;}
		return c == std::char_traits<char>::eof();
	}
	return false;
}

bool parseValue(const std::string& s, bool& b, int);
bool parseValue(const std::string& s, std::string& r, int);

template <class T>
bool parseValue(const std::string& s, std::vector<T>& result, int)
{
	std::stringstream str(s);
	for (std::string item; std::getline(str, item, ',');) {
		T temp;
		if (!parseValue(item, temp, 1)) {
			return false;
		}
		result.push_back(temp);
	}
	return str.eof();
}

template <class T, class U>
bool parseValue(const std::string& s, std::pair<T, U>& result, int) {
	std::string copy(s);
	for (std::string::size_type i = 0; i < copy.length(); ++i) {
		if (copy[i] == ',') copy[i] = ';';
	}
	std::stringstream str(copy);
	if ( !(str >> result.first) ) {return false; }
	str >> std::skipws;
	char c;
	if (str.peek() == ';' && !(str >> c >> result.second)) {
		return false;
	}
	return str.eof();
}

template <class T>
bool Value<T>::parse(const std::string& s) {
	bool ret = parser_ 
		? parser_(s, *value_)
		: parseValue(s, *value_, 1);
	return hasValue_ = ret;
}

namespace detail {
template <class T>
class OwnedValue : public Value<T> {
public:
	typedef Value<T> base_type;
	OwnedValue() : Value<T>(new T()), defaultValue_(0), defaulted_(false) {}
	~OwnedValue() { 
		delete base_type::value_; 
		delete defaultValue_;
	}
	bool isDefaulted()  const   {return defaulted_;}
	bool applyDefault() {
		if (defaultValue_) {
			*base_type::value_ = *defaultValue_;
			return defaulted_ = true;
		}
		return false;
	}
	Value<T>* defaultValue(const T& t) {
		T* nd = new T(t);
		delete defaultValue_;
		defaultValue_ = nd;
		return this;
	}
	bool parse(const std::string& s) {
		if (base_type::parse(s)) {
			defaulted_ = false;
			return true;
		}
		return false;
	}
private:
	T*   defaultValue_;
	bool defaulted_;
};

template <class T>
class SharedValue : public Value<T> {
public:
	typedef Value<T> base_type;
	explicit SharedValue(T& storeTo) : Value<T>(&storeTo) {}
	bool isDefaulted()  const   { return false; }
	bool applyDefault()         { return false; }
	Value<T>* defaultValue(const T& t) { *base_type::value_ = t; return this; }
};
}

///////////////////////////////////////////////////////////////////////////////
// value creation functions
///////////////////////////////////////////////////////////////////////////////
//! creates a new Value-object for values of type T
template <class T>
Value<T>* storeTo(T& v) {
	return new detail::SharedValue<T>(v);
}

template <class T>
Value<T>* value(T* v = 0) {
	return v == 0
		? (Value<T>*)new detail::OwnedValue<T>()
		: storeTo(*v);
}

//! creates a Value-object for options that are bool-switches like --help or --version
/*!
* \note same as value<bool>()->setImplicit()
*/
Value<bool>* bool_switch(bool* b = 0);


///////////////////////////////////////////////////////////////////////////////
// option to value functions
///////////////////////////////////////////////////////////////////////////////

//! down_cast for Value-Objects.
/*!
* \throw BadValue if opt is not of type Value<T>
*/
template <class T>
const T& value_cast(const ValueBase& opt, T* = 0)
{
	if (const Value<T>* p = dynamic_cast<const Value<T>*>(&opt))
		return p->value();
	std::string err = "value is not an ";
	err += typeid(T).name();
	throw BadValue(err.c_str());
}

template <class T>
T& value_cast(ValueBase& opt, T* = 0)
{
	if (Value<T>* p = dynamic_cast<Value<T>*>(&opt))
		return p->value();
	std::string err = "value is not an ";
	err += typeid(T).name();
	throw BadValue(err.c_str());
}

template <class T, class U>
const T& option_as(const U& container, const char* name, T* = 0)
{
	try {
		return ProgramOptions::value_cast<T>(container[name]);
	}
	catch(const BadValue& v)
	{
		std::string msg = "Option ";
		msg += name;
		msg += ": ";
		msg += v.what();
		throw BadValue(msg);
	}
}

///////////////////////////////////////////////////////////////////////////////
// helper functions
///////////////////////////////////////////////////////////////////////////////
std::string toLower(const std::string& in);
std::string toUpper(const std::string& in);

}
#endif
