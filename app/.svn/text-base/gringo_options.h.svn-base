// 
// Copyright (c) 2009, Benjamin Kaufmann
// 
// This file is part of gringo. See http://www.cs.uni-potsdam.de/gringo/
// 
// gringo is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with gringo; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
#ifndef GRINGO_GRINGO_OPTIONS_H_INCLUDED
#define GRINGO_GRINGO_OPTIONS_H_INCLUDED

#ifdef _MSC_VER
#pragma warning (disable : 4200) // nonstandard extension used : zero-sized array
#pragma once
#endif

#include <string>
#include <utility>
#include <program_opts/app_options.h>
#include <gringo/grounder.h>
namespace gringo {

/////////////////////////////////////////////////////////////////////////////////////////
// Option groups - Mapping between command-line options and gringo options
/////////////////////////////////////////////////////////////////////////////////////////
bool mapASPils(const std::string&, int&);
bool parsePositional(const std::string&, std::string&);
struct GringoOptions {
	GringoOptions();
	void initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden);
	bool validateOptions(ProgramOptions::OptionValues& values, Messages&);
	void addDefaults(std::string& def);
	std::vector<std::string> consts;
	// ifixed      => Default: -1
	// verbose     => Default: false
	// binderSplit => Default: true
	gringo::Grounder::Options grounderOptions;
	int              aspilsOut;
	bool             syntax;           // Default: false
	bool             convert;          // Default: false
	bool             smodelsOut;
	bool             shift;            // Default: false
	bool             textOut;
	bool             onlyGround;
	bool             stats;
};

}
#endif
