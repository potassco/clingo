// Copyright (c) 2010, Arne König <arkoenig@uni-potsdam.de>
// Copyright (c) 2010, Roland Kaminski <kaminski@cs.uni-potsdam.de>
//
// This file is part of gringo.
//
// gringo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// gringo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with gringo.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <string>
#include <program_opts/app_options.h>
#include <gringo/gringo.h>

/**
 * Gringo options.
 */
class GringoOptions
{
public:
	enum IExpand { IEXPAND_ALL, IEXPAND_DEPTH };

public:
	GringoOptions();

	// AppOptions interface
	void initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden);
	bool validateOptions(ProgramOptions::OptionValues& values, Messages&);
	void addDefaults(std::string& def);
	TermExpansionPtr termExpansion(IncConfig &config) const;

	/** The constant assignments in the format "constant=term" */
	std::vector<std::string> consts;
	/** Whether to print smodels output */
	bool smodelsOut;
	/** Whether to print in lparse format */
	bool textOut;
	bool metaOut;
	/** True iff some output was requested*/
	bool groundOnly;
	int ifixed;
	bool ibase;
	bool groundInput;
	/** whether disjunctions will get shifted */
	bool disjShift;
	/** filename for optional dependency graph dump */
	std::string depGraph;
	bool compat;
	/** whether statistics will be printed to stderr */
	bool stats;
	IExpand iexpand;
};

