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
#ifndef PROGRAM_OPTIONS_VALUE_BASE_H_INCLUDED
#define PROGRAM_OPTIONS_VALUE_BASE_H_INCLUDED
#ifdef _MSC_VER
#pragma warning (disable : 4786)
#pragma warning (disable : 4503)
#endif
#include <string>
namespace ProgramOptions {

//! base class for values of options
class ValueBase
{
protected:
	ValueBase();
public:
	virtual ~ValueBase() = 0;

	//! returns true if the object already stores a value
	virtual bool hasValue() const = 0;

	//! returns true if the value of the option is implicit
	/*!
	* this property is only meaningful for command line options.
	* An implicit option is a flag, i.e an option that does not
	* expect an explicit value. 
	* Example: --help or --version
	*/
	virtual bool isImplicit() const = 0;

	//! returns true if the value of this option can be composed from multiple source
	virtual bool isComposing() const = 0;

	//! returns true if the value currently holds its default value
	virtual bool isDefaulted() const = 0;

	//! sets the value's default value as value.
	/*!
	* \return true if the default value was set
	*/
	virtual bool applyDefault() = 0;

	//! Parses the given string and stores the result in this value.
	/*!
	* \return 
	* - true if the given string contains a valid value
	* - false otherwise
	*/
	virtual bool parse(const std::string&) = 0;
};

}
#endif
