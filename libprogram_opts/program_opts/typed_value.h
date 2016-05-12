//
//  Copyright (c) Benjamin Kaufmann 2004
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
//
// NOTE: ProgramOptions is inspired by Boost.Program_options
//       see: www.boost.org/libs/program_options
//
#ifndef PROGRAM_OPTIONS_TYPED_VALUE_H_INCLUDED
#define PROGRAM_OPTIONS_TYPED_VALUE_H_INCLUDED
#ifdef _MSC_VER
#pragma warning (disable : 4786)
#pragma warning (disable : 4503)
#pragma warning (disable : 4200)
#endif
#include "value.h"
#include "string_convert.h"
#include "detail/notifier.h"
#include "errors.h"
#include <memory>
namespace ProgramOptions { namespace detail {
template <class T>
struct Parser { typedef bool (*type)(const std::string&, T&); };
} // end namespace detail
using bk_lib::string_cast;
///////////////////////////////////////////////////////////////////////////////
// Enumeration Parser: string->int mapping
///////////////////////////////////////////////////////////////////////////////
struct ValueMappingBase : private std::vector<std::pair<const char*, int> > {
	typedef std::vector<std::pair<const char*, int> > base_type;
	using base_type::size;
	using base_type::empty;
	void       add(const char* strVal, int eVal){ push_back(value_type(strVal, eVal)); }
	const int* get(const char* strVal) const    {
		for (const_iterator it = begin(), end = this->end(); it != end; ++it) {
			if (strcasecmp(strVal, it->first) == 0) { return &it->second; }
		}
		return 0;
	}
};
template <class T>
struct ValueMapping : ValueMappingBase {
	typedef ValueMapping     this_type;
	typedef ValueMappingBase base_type;
	using base_type::get;
	this_type& operator()(const char* strVal, T eVal){ base_type::add(strVal, static_cast<int>(eVal)); return *this; }
	static ValueMapping& instance()                  { static ValueMapping m; return m; }
	operator typename detail::Parser<T>::type()      { return &parse<T>; }
	operator typename detail::Parser<int>::type()    { return &parse<int>; }
	template <class U>
	static bool parse(const std::string& value, U& out){ 
		const ValueMappingBase& m = ValueMapping::instance();
		if (const int* x = m.get(value.c_str())) { out = static_cast<U>(*x); return true; } 
		return false; 
	}
private:
	ValueMapping() {}
};
template <class T> ValueMapping<T>& values() { return ValueMapping<T>::instance(); }
///////////////////////////////////////////////////////////////////////////////
// StoredValue - a typed value that writes to an existing variable
///////////////////////////////////////////////////////////////////////////////
template <class T>
class StoredValue : public Value {
public:
	typedef typename detail::Parser<T>::type parser_type;
	StoredValue(T& var, parser_type p) 
		: Value(0)
		, address_(&var)
		, parser_(p) {
		this->setProperty(Value::property_location);
	}
	bool doParse(const std::string&, const std::string& value) {
		return this->parser_(value, *address_);
	}
protected:
	T*            address_; // storage location of this value
	parser_type   parser_;  // str -> T
};
////////////////////////////////////////////////////////////////////////////////////
// NotifiedValue - a typed value that is created on demand and passed to a callback
////////////////////////////////////////////////////////////////////////////////////
template <class T>
struct DefaultCreator {
	static T* create() { return new T(); }
};
template <class T>
class NotifiedValue : public Value {
public:
	typedef typename detail::Parser<T>::type parser_type;
	typedef detail::Notifier<const T*>       notifier_type;
	NotifiedValue(T* (*cf)(), const notifier_type& n, parser_type p) 
		: Value(0)
		, parser_(p)
		, notify_(n) {
		value_.create = cf;
	}
	NotifiedValue<T>* storeTo(T& obj) {
		value_.address = &obj;
		this->setProperty(Value::property_location);
		return this;
	}
	bool doParse(const std::string& name, const std::string& value) {
		bool ret;
		T* pv = 0;
		std::auto_ptr<T> holder;
		if (this->hasProperty(Value::property_location)) {
			pv  = value_.address;
		}
		else {
			holder.reset(value_.create());
			pv = holder.get();
		}
		ret = this->parser_(value, *pv);
		if (ret && notify_.notify(name, pv)) {
			this->storeTo(*pv);
			holder.release();
		}
		return ret;
	}
protected:
	union {
		T* address;
		T* (*create)();
	}             value_;
	parser_type   parser_;
	notifier_type notify_;
};
///////////////////////////////////////////////////////////////////////////////
// CustomValue - a value that must be parsed/interpreted by a custom context
///////////////////////////////////////////////////////////////////////////////
class CustomValue : public Value {
public:
	typedef detail::Notifier<const std::string&> notifier_type;
	CustomValue(const notifier_type& n) 
		: Value(0)
		, notify_(n) { }
	bool doParse(const std::string& name, const std::string& value) {
		return notify_.notify(name, value);
	}
protected:
	notifier_type notify_;
};
///////////////////////////////////////////////////////////////////////////////
// value factories
///////////////////////////////////////////////////////////////////////////////
#define LIT_TO_STRING_X(lit) #lit
//! stringifies a literal like 1 or 23.0
#define LIT_TO_STRING(lit) LIT_TO_STRING_X(lit)
struct FlagAction {
	typedef detail::Parser<bool>::type parser_t;
	static inline bool store_true(const std::string& v, bool& b) { 
		if (v.empty()) { return (b = true); }
		return string_cast<bool>(v, b);
	}
	static inline bool store_false(const std::string& v, bool& b) { 
		bool temp;
		return store_true(v, temp) && ((b = !temp), true);
	}
	enum Action { act_store_true, act_store_false } act;
	FlagAction(Action a) : act(a) {}
	parser_t parser() const { return act == act_store_true ? store_true : store_false; }
};
static const FlagAction store_true(  (FlagAction::act_store_true) );
static const FlagAction store_false( (FlagAction::act_store_false));

/*!
 * Creates a value that is bound to an existing variable.
 * Assignments to the created value are directly stored in the
 * given variable.
 *
 * \param v The variable to which the new value object is bound
 * \param p The parser to use for parsing the value. If no parser is given, 
 *           type T must provide an operator>>(std::istream&, T&).
 */
template <class T>
inline StoredValue<T>* storeTo(T& v, typename detail::Parser<T>::type p = &string_cast<T>) {
	return new StoredValue<T>(v, p);
}
inline StoredValue<bool>* flag(bool& b, FlagAction x = store_true) { 
	return static_cast<StoredValue<bool>*>(storeTo(b, x.parser())->flag());
}

/*!
 * Creates a notified value, i.e. a value for which
 * a notification function is called once it was parsed.
 * The return value of that function determines whether the
 * value is kept (true) or deleted (false). In the former
 * case ownership of the value is transferred to the notified context.
 *
 * \param p0 A pointer to an object that should be passed 
 *           to the notification function once invoked.
 * \param nf The function to be invoked once a value is created.
 *           On invocation, the first parameter will be p0. The second
 *           parameter will be the value's option name and the third
 *           the location of the newly created value.
 *
 * \param parser The parser to use for parsing the value
 *  
 * \see OptionGroup::addOptions()
 */
template <class T, class ParamT>
inline NotifiedValue<T>* notify(ParamT* p0, typename detail::Notify<const T*, ParamT>::type nf, typename detail::Parser<T>::type parser = &string_cast<T>) {
	return new NotifiedValue<T>(&DefaultCreator<T>::create, detail::Notifier<const T*>(p0, nf), parser);
}
template <class T, class ParamT>
inline NotifiedValue<T>* storeNotify(T& obj, ParamT* p0, typename detail::Notify<const T*, ParamT>::type nf, typename detail::Parser<T>::type parser = &string_cast<T>) {
	return notify<T>(p0, nf, parser)->storeTo(obj);
}
template <class ParamT>
inline NotifiedValue<bool>* flag(ParamT* p0, typename detail::Notify<const bool*, ParamT>::type nf, FlagAction a = store_true) {
	return static_cast<NotifiedValue<bool>*>(notify<bool>(p0, nf, a.parser())->flag());
}

/*!
 * Creates a custom value, i.e. a value that is fully controlled
 * (parsed and created) by a notified context. 
 * 
 * During parsing of options, the notification function of a custom
 * value is called with its option name and the parsed value.
 * The return value of that function determines whether the
 * value is considered valid (true) or invalid (false). 
 *
 * \param p0 A pointer to an object that should be passed 
 *           to the notification function once invoked.
 * \param nf The function to be invoked once a value is parsed.
 *           On invocation, the first parameter will be p0. The second
 *           parameter will be the value's option name and the third
 *           a pointer to the parsed value string.
 *  
 * \see OptionGroup::addOptions()
 */
template <class ParamT>
inline CustomValue* notify(ParamT* p0, typename detail::Notify<const std::string&, ParamT>::type nf) {
	return new CustomValue(detail::Notifier<const std::string&>(p0, nf));
}
}
#endif
