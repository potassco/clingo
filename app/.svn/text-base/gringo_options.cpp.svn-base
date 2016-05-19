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
#include "gringo_options.h"
#include <program_opts/value.h>
namespace gringo {
	
GringoOptions::GringoOptions() 
	: aspilsOut(-1)
	, syntax(false)
	, convert(false)
	, smodelsOut(false)
	, shift(false)
	, textOut(false)
	, onlyGround(false)
	, stats(false) 
{ }

void GringoOptions::initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden) {
	using namespace ProgramOptions;
	OptionGroup gringo("GrinGo Options");

	gringo.addOptions()
		("const,c"         , storeTo(consts)->setComposing(), "Replace constant <c> by value <v>\n", "<c>=<v>")

		("text,t"          , bool_switch(&textOut), "Print plain text format")
		("lparse,l"        , bool_switch(&smodelsOut), "Print Lparse format")
		("aspils,a"        , storeTo(aspilsOut)->parser(mapASPils),
			"Print ASPils format in normal form <num>\n"
			"      Default: 7\n"
			"      Valid:   [1...7]\n"
			"        1: Print in normal form Simple\n"
			"        2: Print in normal form SimpleDLP\n"
			"        3: Print in normal form SModels\n"
			"        4: Print in normal form CModels\n"
			"        5: Print in normal form CModelsExtended\n"
			"        6: Print in normal form DLV\n"
			"        7: Print in normal form Conglomeration\n" , "<num>")

		("ground,g", bool_switch(&convert), "Enable lightweight mode for ground input\n")
		("shift"           , bool_switch(&shift), "Shift disjunctions into the body\n")

		("bindersplit" , storeTo(grounderOptions.binderSplit),
		        "Configure binder splitting\n"
			"      Default: yes\n"
			"      Valid:   yes, no\n"
			"        yes: Enable binder splitting\n"
			"        no : Disable binder splitting\n")

		("ifixed", storeTo(grounderOptions.ifixed)  , "Fix number of incremental steps to <num>", "<num>")
		("ibase",  bool_switch(&grounderOptions.ibase)  , "Process base program only\n")
	;
	
	OptionGroup basic("Basic Options");
	basic.addOptions()
		("syntax",    bool_switch(&syntax),  "Print syntax information and exit\n")
		("debug",    bool_switch(&grounderOptions.debug),  "Print internal representations of rules during grounding\n")
#if !defined(WITH_CLASP)
		("stats", bool_switch(&stats), "Print extended statistics")
#endif
	;
	root.addOptions(gringo);
	root.addOptions(basic,true);
}

bool GringoOptions::validateOptions(ProgramOptions::OptionValues& values, Messages& m) {
	int out = smodelsOut + (aspilsOut > 0) + textOut;
	if (out > 1) {
		m.error = "multiple outputs defined";
		return false;
	}
	if (out == 0) {
		smodelsOut = true;
		onlyGround = false;
	}
	else {
		onlyGround = true;
	}
	return true;
}

void GringoOptions::addDefaults(std::string& def) {
	 def += "  --lparse --bindersplit=yes\n";
}

bool mapASPils(const std::string& s, int &out) {
	return ProgramOptions::parseValue(s, out, 1) && out >= 1 && out <= 7;
}

bool parsePositional(const std::string&, std::string& out) {
	out = "file";
	return true;
}

}

