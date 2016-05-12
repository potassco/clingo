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
//  along with this file; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//
// NOTE: ProgramOptions is inspired by Boost.Program_options
//       see: www.boost.org/libs/program_options
//
#ifdef _MSC_VER
#pragma warning (disable : 4786)
#pragma warning (disable : 4503)
#endif
#include <program_opts/program_options.h>
#include <program_opts/errors.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <climits>
#include <sstream>
using namespace std;

namespace ProgramOptions {

	namespace {

		void defaultFormat(std::ostream& os, const ProgramOptions::Option& o)
		{
			stringstream temp;
			temp << "  ";
			temp << "--" << o.longName();
			if (o.getValue()->isImplicit() && !o.argDescription().empty()) {
				temp << "[=" << o.argDescription() << "]";
			}
			if (!o.shortName().empty()) {
				temp << ",-" << o.shortName();
			}
			if (!o.getValue()->isImplicit())
				temp << (o.shortName().empty()?'=':' ') << o.argDescription();

			os << temp.str();
			if (!o.description().empty())
			{
				if (temp.str().size() < size_t(22)) {
					for(size_t pad = 22 - temp.str().size(); pad > 0; --pad) {
						os.put(' ');
					}
				}
				if (temp.str().size() <= size_t(22)) os << " ";

				os << ": " << o.description();
			}
		}

	}
	///////////////////////////////////////////////////////////////////////////////
	// class ValueBase
	///////////////////////////////////////////////////////////////////////////////
	ValueBase::ValueBase() {}
	ValueBase::~ValueBase() {}

	///////////////////////////////////////////////////////////////////////////////
	// class Option
	///////////////////////////////////////////////////////////////////////////////
	Option::Option(const string& longName, const string& shortName, const string& desc, const std::string& argDesc, ValueBase* v)
		: longName_(longName)
		, shortName_(shortName)
		, description_(desc)
		, argDesc_(argDesc)
		, value_(v)
	{
		assert(v);
		assert(!longName.empty());
		if (argDesc_.empty() && !v->isImplicit()) {
			argDesc_ = "<arg>";
		}
	}

	Option::~Option()
	{}

	///////////////////////////////////////////////////////////////////////////////
	// class OptionGroup
	///////////////////////////////////////////////////////////////////////////////
	// NOTE: 
	// parts of the following implementation are based on boost::program_options::options_description
	OptionGroup::OptionGroup(const std::string& description)
		: size_(0)
	{
		options_.push_back(GroupOptions());
		options_[0].first = description;
	}

	OptionGroup::~OptionGroup()
	{}


	OptionGroupInitHelper OptionGroup::addOptions()
	{
		return OptionGroupInitHelper(this);
	}

	OptionGroup& OptionGroup::addOptions(const OptionGroup& other, bool merge)
	{
		if (this != &other)
		{
			if (merge) {
				size_t key = options_.size();
				for (std::size_t i = 0; i != options_.size(); ++i) {
					if (options_[i].first == other.getDescription()) {
						key = i;
						break;
					}
				}
				if (key != options_.size()) {
					return mergeOptions(key, other);
				}
			}
			for (std::size_t i = 0; i != other.options_.size(); ++i)
			{
				Options::size_type ng = options_.size();
				options_.push_back(GroupOptions());
				options_[ng].first = other.options_[i].first;
				for (std::size_t j = 0; j != other.options_[i].second.size(); ++j)
				{
					insertOption(other.options_[i].second[j], ng);
				}    
			}
		}
		return *this;
	}

	OptionGroup& OptionGroup::mergeOptions(std::size_t targetGroupKey, const OptionGroup& other) {
		if (targetGroupKey < options_.size()) {
			for (std::size_t i = 0; i != other.options_.size(); ++i)
			{
				for (std::size_t j = 0; j != other.options_[i].second.size(); ++j)
				{
					insertOption(other.options_[i].second[j], targetGroupKey);
				}    
			}
		}
		return *this;
	}


	// THROW: DuplicateOption if an option with the same short or long name
	// already exists.
	void OptionGroup::addOption(auto_ptr<Option> opt)
	{
		SharedPtr<Option> o(opt.release());
		insertOption(o);
	}

	void OptionGroup::insertOption(const SharedPtr<Option>& opt, Options::size_type g)
	{
		const string& s = opt->shortName();
		const string& l = opt->longName();

		OptionKey k(g, options_[g].second.size());
		if (!s.empty())
		{
			Name2OptionIndex::iterator shortPos = name2Index_.find("-" + s);
			if (shortPos != name2Index_.end())
				throw DuplicateOption(l, getDescription());

			name2Index_.insert(Name2OptionIndex::value_type("-" + s, k));
		}
		if (!l.empty())
		{
			Name2OptionIndex::iterator longPos = name2Index_.find(l);
			if (longPos != name2Index_.end())
				throw DuplicateOption(l, getDescription());
			name2Index_.insert(Name2OptionIndex::value_type(l, k));
		}
		options_[g].second.push_back(opt);
		++size_;
	}

	std::size_t OptionGroup::size() const
	{
		return size_;
	}

	std::size_t OptionGroup::groups() const
	{
		return options_.size();
	}

	bool OptionGroup::empty() const
	{
		return size_ == 0;
	}
	// name is long name by default.
	// Prepend a '-' character if you want to search using the short name.
	std::size_t OptionGroup::count(const char* name) const
	{
		return name2Index_.count(name);
	}

	std::size_t OptionGroup::countPrefix(const char* name) const
	{
		PrefixRange r = getPrefixRange(name);
		return distance(r.first, r.second);
	}

	const Option& OptionGroup::find(const char* name) const
	{
		Name2OptionIndex::const_iterator it = name2Index_.find(name);
		if (it != name2Index_.end())
		{
			const OptionKey& k = it->second;
			return *options_[k.first].second[k.second];
		}


		throw UnknownOption(getDescription().length() ? getDescription() + "::" + name : std::string(name));
	}

	const Option& OptionGroup::findPrefix(const char* name) const
	{
		PrefixRange r = getPrefixRange(name);
		if (distance(r.first, r.second) == 0)
			throw UnknownOption(name);
		if (distance(r.first, r.second) > 1) {
			std::string str = "'";
			str += name;
			str += "' could be:";
			for (; r.first != r.second; ++r.first) { 
				str += "\n  " + r.first->first;
			}
			throw AmbiguousOption(str);
		}


		const OptionKey& k = r.first->second;
		return *options_[k.first].second[k.second];
	}

	set<string> OptionGroup::primaryKeys() const
	{
		set<string> result;
		for (std::size_t i = 0; i != options_.size(); ++i)
			for (std::size_t j = 0; j != options_[i].second.size(); ++j)
				result.insert(options_[i].second[j]->longName());
		return result;
	}
	void OptionGroup::writeToStream(std::ostream& os, FormatFunction f) const
	{
		// print all sub-groups
		for (std::size_t i = 1; i < options_.size(); ++i)
		{
			if (!options_[i].first.empty())
				os << "\n" << options_[i].first << ":\n" << endl;
			for (std::size_t j = 0; j != options_[i].second.size(); ++j)
			{
				f(os, *options_[i].second[j]);
				os << '\n';
			}
		}
		// print main group
		if (!options_[0].first.empty()) {
			os << "\n" << options_[0].first << ":\n" << endl;
		}
		for (std::size_t j = 0; j != options_[0].second.size(); ++j)
		{
			f(os, *options_[0].second[j]);
			os << '\n';
		}
	}

	OptionGroup::PrefixRange OptionGroup::getPrefixRange(const char* prefix) const
	{
		Name2OptionIndex::const_iterator b = name2Index_.lower_bound(prefix);
		Name2OptionIndex::const_iterator e = name2Index_.upper_bound(string(prefix) + char(CHAR_MAX));
		return make_pair(b, e);
	}
	///////////////////////////////////////////////////////////////////////////////
	// class OptionGroupInitHelper
	///////////////////////////////////////////////////////////////////////////////
	OptionGroupInitHelper::OptionGroupInitHelper(OptionGroup* owner)
		: owner_(owner)
	{
		assert(owner);
	}

	OptionGroupInitHelper& OptionGroupInitHelper::operator()(const string& name,
		ValueBase* val,
		const char* description, const char* argDesc)
	{
		string::size_type n = name.find(',');
		string shortName, longName, desc, arg;
		if (description)
			desc = description;
		if (argDesc) {
			arg = argDesc;
		}
		if (n != string::npos)
		{
			if (!n || n != name.size() - 2)
			{
				delete val;
				throw BadOptionName(name);
			}
			longName = name.substr(0, n);
			shortName = name.substr(n+1,1);
		}
		else
		{
			longName = name;
		}
		owner_->addOption(auto_ptr<Option>(new Option(longName, shortName, desc, arg, val)));
		return *this;
	}


	std::ostream& operator<<(std::ostream& os, const OptionGroup& grp)
	{
		grp.writeToStream(os, defaultFormat);
		return os;
	}
	///////////////////////////////////////////////////////////////////////////////
	// class OptionValues
	///////////////////////////////////////////////////////////////////////////////
	OptionValues::OptionValues()
	{}

	void OptionValues::store(const ParsedOptions& options)
	{
		if (options.grp_)
		{
			const OptionGroup& grp = *options.grp_;
			std::set<string> finalValues;
			for (ValueMap::const_iterator it = values_.begin(); it != values_.end(); ++it)
			{
				if (!it->second->isDefaulted() && !it->second->isComposing())
					finalValues.insert(it->first);
			}
			for (ParsedOptions::Options::const_iterator i = options.options_.begin();
				i != options.options_.end(); ++i)
			{
				const std::string& name = i->first;
				if (!name.empty() && grp.count(name.c_str()) && !finalValues.count(name))
				{
					const Option& o = grp.find(name.c_str());
					if (o.getValue()->hasValue() && !o.getValue()->isComposing()) {
						std::string d("Option '");
						d += name;
						d += "'";
						throw MultipleOccurences(d);
					}
					if (o.getValue()->parse(i->second)) {
						values_.insert(ValueMap::value_type(name, o.getValue()));
					}
					else {
						std::string d("'");
						d += i->second;
						d += "': invalid value for Option '";
						d += name;
						d += "'";
						throw BadValue(d);
					}
				}
			}
			// apply defaults
			std::set<std::string> keys = grp.primaryKeys();
			for (std::set<std::string>::const_iterator k = keys.begin(); k != keys.end(); ++k)
			{
				if (values_.count(k->c_str()) == 0)
				{
					const Option& o = grp.find(k->c_str());
					if (o.getValue()->applyDefault())
					{
						values_.insert(ValueMap::value_type(k->c_str(), o.getValue()));
					}
				}
			}
		}
	}

	std::size_t OptionValues::size() const
	{
		return values_.size();
	}
	bool OptionValues::empty() const
	{
		return values_.empty();
	}
	std::size_t OptionValues::count(const char* name) const
	{
		return values_.count(name);
	}
	ValueBase& OptionValues::operator[](const char* name)
	{
		ValueMap::iterator it = values_.find(name);
		if (it == values_.end())
			throw UnknownOption(name);
		return *it->second;
	}

	void OptionValues::clear()
	{
		values_.clear();
	}

	///////////////////////////////////////////////////////////////////////////////
	// class OptionParser
	///////////////////////////////////////////////////////////////////////////////
	OptionParser::OptionParser(OptionGroup& o, bool allowUnreg)
		: po_(o)
		, allowUnreg_(allowUnreg)
	{}

	OptionParser::~OptionParser()
	{}

	ParsedOptions OptionParser::parse()
	{
		doParse();
		return po_;
	}

	OptionParser::OptionType OptionParser::getOptionType(const char* o) const
	{
		if (*o == '-' && *(o + 1) == '-')
			return  *(o + 2) != '\0' ? long_opt : end_opt;

		return *o == '-' && *(o + 1) != '\0' ? short_opt : no_opt;
	}

	const Option* OptionParser::getOption(const char* name, OptionType t)
	{
		if (!name || !*name) return 0;
		assert(t == short_opt || t == long_opt);
		if (t == short_opt)
		{
			string n = "-";
			n += name;
			if (po_.grp_->count(n.c_str()))
				return &po_.grp_->find(n.c_str());
		}
		else if (t == long_opt)
		{
			if (po_.grp_->count(name))
				return &po_.grp_->find(name);
			if (po_.grp_->countPrefix(name))
			{
				return &po_.grp_->findPrefix(name);
			}
		}
		if (!allowUnreg_)
			throw UnknownOption(name);
		return 0;
	}

	void OptionParser::addOptionValue(const string& name, const string& value)
	{
		po_.options_.push_back(std::make_pair(name, value));
	}


	namespace {   
		///////////////////////////////////////////////////////////////////////////////
		// class CommandLineParser
		///////////////////////////////////////////////////////////////////////////////    
		class CommandLineParser : public OptionParser
		{
		public:
			CommandLineParser(OptionGroup& o, bool allowUnreg, int& argc, char** argv, PosOption po)
				: OptionParser(o, allowUnreg)
				, currentArg_(0)
				, nextArgNr_(1)
				, argc_(&argc)
				, argv_(argv)
				, posOpt_(po)
			{}
		private:
			void doParse()
			{
				bool breakEarly  = false;
				const Option* po = 0;
				std::string poName, poNameLast;
				while (next() && !breakEarly)
				{
					switch(getOptionType())
					{
					case short_opt: handleShortOpt(currentArg_ + 1); break;
					case long_opt:  handleLongOpt(currentArg_ + 2);  break;
					case end_opt: breakEarly = true; break;
					case no_opt:  
						if (!posOpt_ || !posOpt_(currentArg_, poName))
						{
							po = getOption("Positional Option", long_opt);
						}
						else if (poName != poNameLast)
						{
							po = getOption(poName.c_str(), long_opt);
							poNameLast = poName;
						}
						if (po) 
						{
							addOptionValue(po->longName(), currentArg_);
							removeArgs(1);
						}
						break;
					default:
						assert(0);
					}
				}
			}
		private:
			OptionType getOptionType() const
			{
				return OptionParser::getOptionType(currentArg_);
			}

			void removeArgs(int n)
			{
				int copyFrom = nextArgNr_;
				// <= damit auch das terminierende 0-Array kopiert wird
				for (int i = nextArgNr_ - n ; copyFrom <= *argc_;)
					argv_[i++] = argv_[copyFrom++];
				*argc_ -= n;
				nextArgNr_ -= n;
			}


			bool next()
			{
				if (nextArgNr_ < *argc_)
				{
					currentArg_ = argv_[nextArgNr_++];
					return true;
				}
				currentArg_ = 0;
				return false;
			}

			void handleShortOpt(const char* optName)
			{
				if ( *(optName + 1) == 0)
				{ // e.g -c
					if (const Option* o = getOption(optName, short_opt))
					{
						string value = "";
						if (!o->getValue()->isImplicit())
						{
							if (next())
							{
								value = currentArg_;
								addOptionValue(o->longName(), value);
								removeArgs(2);
							}
							else
							{
								throw runtime_error(string("required parameter missing after -") + optName);
							}
						}
						else
						{
							addOptionValue(o->longName(), value);
							removeArgs(1);
						}
					}
					// else: ignore option
				}
				else  
				{ // e.g -cab
					const char* save = optName;
					char optn[2];
					optn[0] = optName[0];
					optn[1] = '\0';
					if (const Option* o = getOption(optn, short_opt))
					{
						string value = "";
						if (o->getValue()->isImplicit())
						{ // combined option: eg: -io -> -i -o 
							addOptionValue(o->longName(), value);
							++optName;
							while (*optName)
							{
								optn[0] = *optName;
								if ( (o = getOption(optn, short_opt)) != 0 && o->getValue()->isImplicit())
								{
									addOptionValue(o->longName(), value);
								}
								else
									throw runtime_error(string("illegal option '") + optn + "' in -" + save);
								++optName;
							}
							removeArgs(1);
						}
						else
						{ // option and value
							string value(optName + 1);
							addOptionValue(o->longName(), value);
							removeArgs(1);
						}
					}
					// else: ignore option and value
				}
			}
			void handleLongOpt(const char* optName)
			{
				string name(optName);
				string value;
				string::size_type p = name.find('='); 
				if (p != string::npos)
				{
					value.assign(name, p + 1, string::npos);
					name.erase(p, string::npos);
				}
				if (const Option* o = getOption(name.c_str(), long_opt))
				{
					if (!o->getValue()->isImplicit() && value.empty())
					{
						if (next())
						{
							value = currentArg_;
							addOptionValue(o->longName(), value);
							removeArgs(2);
						}
						else
							throw runtime_error(string("required parameter missing after --") + optName);
					}
					else
					{
						addOptionValue(o->longName(), value);
						removeArgs(1);
					}
				}
				// else: ignore option
			}
			const char* currentArg_;
			int nextArgNr_;
			int* argc_;
			char** argv_;
			PosOption posOpt_;
		};

		///////////////////////////////////////////////////////////////////////////////
		// class CfgFileParser
		///////////////////////////////////////////////////////////////////////////////    
		class CfgFileParser : public OptionParser
		{
		public:
			CfgFileParser(OptionGroup& o, bool allowUnreg, std::istream& in)
				: OptionParser(o, allowUnreg)
				, in_(in)
			{}
		private:
			inline void trimLeft(std::string& str, const std::string& charList = " \t")
			{
				std::string::size_type pos = str.find_first_not_of(charList);
				if (pos != 0)
					str.erase(0, pos);
			}
			inline void trimRight(std::string& str, const std::string& charList = " \t")
			{
				std::string::size_type pos = str.find_last_not_of(charList);
				if (pos != std::string::npos)
					str.erase(pos + 1, std::string::npos);
			}
			bool splitHalf( const std::string& str, const std::string& seperator,
				std::string& leftSide,
				std::string& rightSide)
			{
				std::string::size_type sepPos = str.find(seperator);
				leftSide.assign(str, 0, sepPos);
				if (sepPos != std::string::npos)
				{
					rightSide.assign(str, sepPos + seperator.length(), std::string::npos);
					return true;
				}
				return false;
			}
			void doParse()
			{
				int lineNr = 0;
				std::string sectionName;      // current section name
				std::string sectionValue;     // current section value
				bool inSection = false;       // true if multi line section value

				// reads the config file.
				// A config file may only contain empty lines, single line comments or
				// sections structured in a name = value fashion.
				// value can span multiple lines, but parts in different lines than name
				// must not contain a '='-Character.
				for (std::string line; std::getline(in_, line);)
				{
					++lineNr;
					trimLeft(line);
					trimRight(line);

					if (line.empty() || line.find("#") == 0)
					{
						// An empty line or single line comment stops a multi line section value.
						if (inSection)
						{
							if (getOption(sectionName.c_str(), long_opt))
								addOptionValue(sectionName, sectionValue);
							inSection = false;
						}
						continue;
					}
					std::string::size_type pos;
					if ( (pos = line.find("=")) != std::string::npos)
					{
						// A new section terminates a multi line section value.
						// First process the current section value...
						if (inSection)
						{
							if (getOption(sectionName.c_str(), long_opt))
								addOptionValue(sectionName, sectionValue);
							inSection = false;
						}
						// ...then save the new section's value.
						splitHalf(line, "=", sectionName, sectionValue);
						trimRight(sectionName);
						trimLeft(sectionValue, " \t\n");
						inSection = true;
					}
					else if (inSection)
					{
						sectionValue += " ";
						sectionValue += line;
					}
					else
					{
						throw std::runtime_error("illegal option file format");
					}
				}
				if (inSection)
				{ // file does not end with an empty line
					if (getOption(sectionName.c_str(), long_opt))
						addOptionValue(sectionName, sectionValue);
				}
			}
			std::istream& in_;
		};

	}   // end unnamed namespace

	ParsedOptions parseCommandLine(int& argc, char** argv, OptionGroup& o, bool allowUnreg,
		PosOption po)
	{
		CommandLineParser pa(o, allowUnreg, argc, argv, po);
		return pa.parse();
	}


	ParsedOptions parseCfgFile(std::istream& in, OptionGroup& o, bool allowUnreg)
	{
		CfgFileParser p(o, allowUnreg, in);
		return p.parse();
	}

}
