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
#ifndef PROGRAM_OPTIONS_PROGRAM_OPTIONS_H_INCLUDED
#define PROGRAM_OPTIONS_PROGRAM_OPTIONS_H_INCLUDED
#include <string>
#ifdef _MSC_VER
#pragma warning (disable : 4786)
#pragma warning (disable : 4503)
namespace std {
	using ::size_t;
}
#endif
#include <iosfwd>
#include <map>
#include <vector>
#include <set>
#include <stdexcept>
#include <memory>
#include "value_base.h"
#include "detail/smartpointer.h"
namespace ProgramOptions {
	class ValueBase;
	//! represents one program option.
	/*!
	* An Option consists of a description (long name, short name, description) 
	* and a (typed) value.
	*/
	class Option
	{
	public:
		/*!
		* \pre longName != ""
		* \pre value != 0
		*/
		Option( const std::string& longName, const std::string& shortName,
			const std::string& description, const std::string& argDesc, ValueBase* value);

		~Option();

		const std::string& longName() const   {return longName_;}
		const std::string& shortName() const  {return shortName_;}
		const std::string& description() const  {return description_;}
		const std::string& argDescription() const { return argDesc_; }

		SharedPtr<ValueBase> getValue() const {return value_;}
	private:
		std::string longName_;
		std::string shortName_;
		std::string description_;
		std::string argDesc_;
		SharedPtr<ValueBase> value_;
	};

	class OptionGroupInitHelper;

	//! container of options
	class OptionGroup
	{
	public:
		/*!
		* \param description A string that describes this group. Primarily useful for output
		*/
		OptionGroup(const std::string& description = "");
		~OptionGroup();

		//! returns an object that can be used to add options
		/*!
		* \par usage \n
		* \code
		* OptionGroup g;
		* g.addOptions()
		*   ("opt1", value<int>(), "an int option")    // <- no semicolon
		*   ("opt2", value<double>())                  // <- no semicolon
		*   ("opt3", value<char>())                    // <- no semicolon
		* ;                                            // note the semicolon!
		* \endcode
		* \see OptionGroup::addOption
		*/
		OptionGroupInitHelper addOptions();

		std::string getDescription() const
		{
			return options_[0].first;
		}


		//! adds option to this group
		/*!
		* \throw DuplicateOption if an option with the same short or long name already exists
		* in this group
		*/
		void addOption(std::auto_ptr<Option> option);

		//! adds the options of other to this group
		/*!
		* \throw DuplicateOption if an option with the same short or long name already exists
		* in this group
		* \param mergeGroup if true and there is already some group G' such that G'.getDescription() == other.getDescription()
		*                   options of other are added to G'
		*/
		OptionGroup& addOptions(const OptionGroup& other, bool mergeGroup = false);

		//! returns the number of options in this group
		std::size_t size() const;
		//! returns the number of groups in this group
		std::size_t groups() const;

		//! returns size() == 0
		bool empty() const;

		//! returns > 0 if this group contains an option with the given name.
		/*!
		* \note name is treated as the long name of an option.
		* Prepend a '-' character if you want to search using the short name.
		*/
		std::size_t count(const char* name) const;

		//! returns the number of options whose names start with the given prefix
		std::size_t countPrefix(const char* name) const;


		//! returns the option with the given name
		/*!
		* \throw UnknownOption if count(name) == 0
		* \see OptionGroup::count
		*/
		const Option& find(const char* name) const;

		//! returns the option whose name starts with name
		/*!
		* \throw UnknownOption if countPrefix (name) == 0 and AmbiguousOption if countPrefix() > 1
		*/
		const Option& findPrefix(const char* name) const;

		//! returns the long names of the options in this group
		std::set<std::string> primaryKeys() const;

		typedef void (*FormatFunction)(std::ostream&, const Option& option);
		friend std::ostream& operator<<(std::ostream& os,  const OptionGroup& grp);


		void writeToStream(std::ostream&, FormatFunction f) const;
	private:
		typedef std::pair<std::string, std::vector<SharedPtr<Option> > > GroupOptions;
		typedef std::vector<GroupOptions> Options;
		typedef std::pair<size_t, size_t> OptionKey;

		typedef std::map<std::string, OptionKey> Name2OptionIndex;
		typedef std::pair<Name2OptionIndex::const_iterator, Name2OptionIndex::const_iterator> PrefixRange;
		OptionGroup& mergeOptions(std::size_t targetGroupKey, const OptionGroup& other);
		Options options_;
		Name2OptionIndex  name2Index_;
		Options::size_type size_;

		PrefixRange getPrefixRange(const char* name) const;
		void insertOption(const SharedPtr<Option>& o, Options::size_type g = 0);
	};

	class OptionGroupInitHelper
	{
	public:
		explicit OptionGroupInitHelper(OptionGroup* owner);
		OptionGroupInitHelper& operator()(  const std::string& name,
			ValueBase* val,
			const char* description = 0,
			const char* argDesc = 0);
	private:
		OptionGroup* owner_;
	};

	class ParsedOptions;

	//! stores the values of parsed options.
	class OptionValues
	{
	public:
		OptionValues();

		/*!
		* Stores the values of all options that are defined in opts
		* If *this already has a non-defaulted value of an option, that value
		* is not changed, even if opts specify some value.
		* 
		* For options having a default-value but which are not defined in opts
		* the default-value is stored.
		*/
		void store(const ParsedOptions& opts);

		//! returns the number of stored values
		std::size_t size() const;

		//! returns true if size() == 0
		bool empty() const;

		void clear();

		/*!
		* returns > 0 if *this contains a value for the option with the long-name name.
		*/
		std::size_t count(const char* name) const;

		/*!
		* returns the value for the option with the long-name name.
		* \pre count(name) > 0
		* \throw UnknownOption if precondition is not met.
		*/
		const ValueBase& operator[](const char* name) const {
			return const_cast<OptionValues*>(this)->operator[](name);
		}
		ValueBase& operator[](const char* name);
	private:
		typedef std::map<std::string, SharedPtr<ValueBase> > ValueMap;
		ValueMap values_;
	};

	/*!
	* container of key-value-pairs representing options found in
	* command-line/config-file.
	*/
	class ParsedOptions
	{
	public:
		/*!
		* \param grp The OptionGroup for which this object stores raw-values.
		*/
		explicit ParsedOptions(const OptionGroup& grp)
			: grp_(&grp)
		{}

		const OptionGroup* grp_;
		typedef std::pair<std::string, std::string> KeyValue;
		typedef std::vector<KeyValue> Options;
		Options options_;
	};

	//! base class for options parsers
	class OptionParser
	{
	public:
		enum OptionType {short_opt, long_opt, end_opt, no_opt};
		OptionParser(OptionGroup& o, bool allowUnreg);
		virtual ~OptionParser();
		ParsedOptions parse();
	protected:
		OptionType getOptionType(const char* o) const;
		const Option* getOption(const char* name, OptionType t);
		void addOptionValue(const std::string& name, const std::string& value);
	private:
		virtual void doParse() = 0;
		ParsedOptions po_;
		bool allowUnreg_; 
	};

	///////////////////////////////////////////////////////////////////////////////
	// parse functions
	///////////////////////////////////////////////////////////////////////////////
	typedef bool (*PosOption)(const std::string&, std::string&);
	
	/*!
	* parses the command line starting at index 1 and removes
	* all found options from argv.
	* \param argc nr of arguments in argv
	* \param argv the command line arguments
	* \param grp options to search in the command line.
	* \param allowUnregistered Allow arguments that match no option in grp
	* \param positionalOption An optional function that is called for
	         command line tokens that have no option name. The function must either 
					 return true and store tthe name of the option in grp that 
					 should receive the token as value in out 
					 or return false to signal an error.
	* \return A ParsedOptions-Object containing names and values for all options found.
	* 
	* \throw std::runtime_error if command line syntax is incorrect
	* \throw UnknownOption if allowUnregistered is false and an argument is found
	* that does not match any option.
	* \note Use the returned object to populate an OptionValues-Object.
	*/
	ParsedOptions parseCommandLine(int& argc, char** argv, OptionGroup& grp,
		bool allowUnregistered = true,
		PosOption positionalOption = 0);

	/*!
	* parses a config file having the format key = value.
	* \param is the stream representing the config file
	* \param o options to search in the config file
	* \param allowUnregistered Allow arguments that match no option in grp
	* 
	* \return A ParsedOptions-Object containing names and values for all options found.
	*
	* \throw std::runtime_error if config file has incorrect syntax.
	* \throw UnknownOption if allowUnregistered is false and an argument is found
	* \note keys are option's long names.
	* \note lines starting with # are treated as comments and are ignored.
	* \note Use the returned object to populate an OptionValues-Object.
	*/
	ParsedOptions parseCfgFile(std::istream& is, OptionGroup& o, bool allowUnregistered);


}

#endif
