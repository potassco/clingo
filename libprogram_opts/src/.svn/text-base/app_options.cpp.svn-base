// 
// Copyright (c) 2006-2007, Benjamin Kaufmann
// 
// This file is part of Clasp. See http://www.cs.uni-potsdam.de/clasp/ 
// 
// Clasp is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// 
// Clasp is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with Clasp; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
#include <program_opts/app_options.h>
#include <program_opts/value.h>
#include <sstream>
#include <algorithm>
#include <cctype>

using namespace ProgramOptions;
using namespace std;

/////////////////////////////////////////////////////////////////////////////////////////
// Generic Options - independent of concrete system
/////////////////////////////////////////////////////////////////////////////////////////
GenericOptions::GenericOptions() 
	: verbose(0)
	, help(false)
	, version(false) {}

namespace {
bool mapVLevel(const std::string& s, int& level) {
	if (s.empty()) return !!(level = 2);
	else return ProgramOptions::parseValue(s, level, 1);
}
}

void GenericOptions::initOptions(OptionGroup& root, OptionGroup& hidden) {
	OptionGroup basic("Basic Options");
	basic.addOptions()
		("help,h"   , bool_switch(&help),    "Print help information and exit")
		("version,v", bool_switch(&version), "Print version information and exit")    
		("verbose,V", value<int>(&verbose)->setImplicit()->parser(mapVLevel),   "Verbosity level", "<n>")
	;
	root.addOptions(basic, true);
	hidden.addOptions()
		("file,f", value<StringSeq>(&input)->setComposing(), "Input files\n")
	;
}

void GenericOptions::addDefaults(std::string&) {}

/////////////////////////////////////////////////////////////////////////////////////////
// Parsing & Validation of command line
/////////////////////////////////////////////////////////////////////////////////////////
bool AppOptions::parse(int argc, char** argv, ProgramOptions::PosOption p) {
	OptionValues values;
	try {
		defaults_ = "";
		OptionGroup allOpts, visible("Basic Options"), hidden;
		initOptions(visible, hidden);
		generic.initOptions(visible, hidden);
		addDefaults(defaults_);
		generic.addDefaults(defaults_);
		allOpts.addOptions(visible).addOptions(hidden);
		messages.clear();
		values.store(parseCommandLine(argc, argv, allOpts, false, p));
		if (generic.help || generic.version) { 
			stringstream str;
			str << visible;
			help_ = str.str();
			return true;
		}
		return generic.validateOptions(values, messages) 
			&&   validateOptions(values, messages);
	}
	catch(const std::exception& e) {
		messages.error = e.what();
		return false;
	}
	return true;
}

