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
#ifndef PROGRAM_OPTIONS_ERRORS_H_INCLUDED
#define PROGRAM_OPTIONS_ERRORS_H_INCLUDED
#include <stdexcept>
#include <string>
namespace ProgramOptions {

//! Base class for all exceptions.
class Error : public std::logic_error {
public:
	explicit Error(const std::string& what) : std::logic_error(what) {}
};

//! Used for signaling errors on command-line and in declaring options.
class SyntaxError : public Error {
public:
	enum Type {
		missing_value,
		extra_value,
		invalid_format
	};
	SyntaxError(Type t, const std::string& key);
	~SyntaxError() throw () {}
	Type               type() const { return type_; }
	const std::string& key()  const { return key_;  }
private:
	std::string key_;
	Type        type_;
};

//! Used for signaling errors in OptionContext.
class ContextError : public Error {
public:
	enum Type {
		duplicate_option,
		unknown_option,
		ambiguous_option,
		unknown_group,
	};
	ContextError(const std::string& ctx, Type t, const std::string& key, const std::string& desc = "");
	~ContextError() throw () {}
	Type               type() const { return type_; }
	const std::string& key()  const { return key_;  }
	const std::string& ctx()  const { return ctx_;  }
private:
	std::string ctx_;
	std::string key_;
	Type        type_;
};

class DuplicateOption : public ContextError {
public:
	DuplicateOption(const std::string& ctx, const std::string& key) : ContextError(ctx, ContextError::duplicate_option, key) {}
	~DuplicateOption() throw () {}
};
class UnknownOption : public ContextError {
public:
	UnknownOption(const std::string& ctx, const std::string& key) : ContextError(ctx, ContextError::unknown_option, key) {}
	~UnknownOption() throw () {}
};
class AmbiguousOption : public ContextError {
public:
	AmbiguousOption(const std::string& ctx, const std::string& key, const std::string& alt) : ContextError(ctx, ContextError::ambiguous_option, key, alt) {}
	~AmbiguousOption() throw () {}
};

//! Used for signaling validation errors when trying to assign option values.
class ValueError : public Error {
public:
	enum Type {
		multiple_occurences,
		invalid_default,
		invalid_value
	};
	ValueError(const std::string& ctx, Type t, const std::string& opt, const std::string& value);
	~ValueError() throw () {}
	Type               type() const { return type_; }
	const std::string& key()  const { return key_;  }
	const std::string& ctx()  const { return ctx_;  }
	const std::string& value()const { return value_;}
private:
	std::string ctx_;
	std::string key_;
	std::string value_;
	Type        type_;
};


}
#endif
