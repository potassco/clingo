// Copyright (c) 2010, Arne König
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

#include "gringo/gringo_options.h"
#include "gringo/main_app.h"
#include <gringo/streams.h>

/**
 * Gringo command line application.
 */
class GringoApp : public MainApp
{
public:
	/** returns a singleton instance */
	static GringoApp& instance();

private:
	GringoApp(const GringoApp&);
	const GringoApp& operator=(const GringoApp&);

protected:
	GringoApp() {}
	Output *output() const;
	/** returns a stream of constants provided through the command-line.
	  * \returns input stream containing the constant definitions in ASP
	  */
	Streams::StreamPtr constStream() const;
	// ---------------------------------------------------------------------------------------
	// AppOptions interface
	void initOptions(ProgramOptions::OptionGroup& root, ProgramOptions::OptionGroup& hidden) {
		gringo.initOptions(root, hidden);
	}
	void addDefaults(std::string& defaults) {
		gringo.addDefaults(defaults);
	}
	bool validateOptions(ProgramOptions::OptionValues& v, Messages& m) {
		return gringo.validateOptions(v, m);
	}
	// ---------------------------------------------------------------------------------------
	// Application interface
	ProgramOptions::PosOption getPositionalParser() const;
	void handleSignal(int sig);
	int  doRun();
	std::string getVersion() const;
	// ---------------------------------------------------------------------------------------

public:
	GringoOptions gringo;
};
