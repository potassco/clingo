// Copyright (c) 2010, Roland Kaminski <kaminski@cs.uni-potsdam.de>
// Copyright (c) 2009, Benjamin Kaufmann
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

#include <program_opts/app_options.h>
#include <gringo/output.h>
#include <iosfwd>
#include <memory>

// exit codes
#define S_SATISFIABLE   10   // problem is satisfiable
#define S_UNSATISFIABLE 20   // problem was proved to be unsatisfiable
#define S_UNKNOWN       0    // satisfiablity of problem not knwon; search was interrupted or did not start
#define S_ERROR EXIT_FAILURE // internal error, except out of memory
#define S_MEMORY        127  // out of memory

// Base class for gringo based applications
class MainApp : public AppOptions
{
public:
	virtual void printHelp()    const;
	virtual void printVersion() const;
	int          run(int argc, char** argv);
	void         printWarnings();
protected:
	MainApp();
	virtual std::string getVersion()    const = 0;
	virtual std::string getUsage()      const;
	virtual std::string getExecutable() const;
private:
	MainApp(const MainApp&);
	MainApp& operator=(const MainApp&);
	static void sigHandler(int sig);
	void    installSigHandlers();
	virtual void handleSignal(int sig)        = 0;
	virtual int  doRun()                      = 0; 
	virtual ProgramOptions::PosOption getPositionalParser() const = 0;
private:
	const  char    *executable_;
	static MainApp *app_;
};

