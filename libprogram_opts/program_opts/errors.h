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
#ifndef PROGRAM_OPTIONS_ERRORS_H_INCLUDED
#define PROGRAM_OPTIONS_ERRORS_H_INCLUDED
#include <stdexcept>
namespace ProgramOptions
{

	//! thrown when a value has an unexpected type
	class BadValue : public std::logic_error
	{
	public:
		BadValue(const std::string& msg)
			: std::logic_error(msg)
		{}
		~BadValue() throw () {}
		std::string getOptionName() const
		{
			return optionName_;
		}
	private:
		std::string optionName_;
	};

	//! thrown when an option has an illegal name
	class BadOptionName : public std::logic_error
	{
	public:
		BadOptionName(const std::string& name)
			: std::logic_error(std::string("bad option name: ").append(name))
		{}
		~BadOptionName() throw () {}
	};

	//! thrown when more than one option in a group has the same name
	class DuplicateOption : public std::logic_error
	{
	public:
		DuplicateOption(const std::string& name, const std::string& grp)
			: std::logic_error(grp + " - " + "Duplicate Option: '" + name + "'")
			, name_(name)
			, grpDesc_(grp)
		{}
		const std::string& getOptionName() const {return name_;}
		const std::string& getGroupName() const {return grpDesc_;}
		~DuplicateOption() throw() {}
	private:
		const std::string name_;
		const std::string grpDesc_;
	};

	//! thrown when an unknown option is requested
	class UnknownOption : public std::logic_error
	{
	public:
		UnknownOption(const std::string& name)
			: std::logic_error(std::string("unknown option: ").append(name))
		{}
		~UnknownOption() throw() {}
	};

	//! thrown when there's ambiguity amoung several possible options.
	class AmbiguousOption : public std::logic_error
	{
	public:
		AmbiguousOption(const std::string& optname)
			: std::logic_error(std::string("ambiguous option: ").append(optname))
		{}
		~AmbiguousOption() throw () {}
	};

	//! thrown when there are several occurrences of an option in one source
	class MultipleOccurences : public std::logic_error
	{
	public:
		MultipleOccurences(const std::string& opt)
			: std::logic_error(std::string("multiple occurences: ").append(opt))
		{}
		~MultipleOccurences() throw () {}
	};
}
#endif
