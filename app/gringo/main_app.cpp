// Copyright (c) 2010, Roland Kaminski <kaminski@cs.uni-potsdam.de>
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

#include "gringo/main_app.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <signal.h>

using namespace std;

MainApp::MainApp() {}

void MainApp::printWarnings()
{
	while(!messages.warning.empty()) 
	{
		cerr << "Warning: " << messages.warning.back() << endl;
		messages.warning.pop_back();
	}
}

int MainApp::run(int argc, char** argv)
{
	executable_ = argv[0];
	try
	{
		if(!parse(argc, argv, getPositionalParser()))
			throw std::runtime_error( messages.error.c_str() );
		if(generic.help)    { printHelp(); return EXIT_SUCCESS; }
		if(generic.version) { printVersion(); return EXIT_SUCCESS; }
		printWarnings();
		installSigHandlers();
		return doRun();
	}
	catch(const std::bad_alloc& e)
	{
		cerr << "\nERROR: " << e.what() << endl;
		return S_MEMORY;
	}
	catch(const std::exception& e)
	{
		cerr << "\nERROR: " << e.what() << endl;
		return S_ERROR;
	}
}

void MainApp::sigHandler(int sig)
{
	// ignore further signals
	signal(SIGINT, SIG_IGN);  // Ctrl + C
	signal(SIGTERM, SIG_IGN); // kill(but not kill -9)
	app_->handleSignal(sig);
}

MainApp *MainApp::app_ = 0;

void MainApp::installSigHandlers()
{
	assert(!app_);
	app_ = this;
	if(signal(SIGINT, &MainApp::sigHandler) == SIG_IGN)  signal(SIGINT, SIG_IGN);
	if(signal(SIGTERM, &MainApp::sigHandler) == SIG_IGN) signal(SIGTERM, SIG_IGN);
}

void MainApp::printHelp() const
{
	cout << getExecutable() << " version " << getVersion()
	     << "\n\n"
	     << "Usage: " << getExecutable() << " " << getUsage()
	     << "\n"
	     << getHelp()
	     << "\n"
	     << "Usage: " << getExecutable() << " " << getUsage()
	     << "\n\n"
	     << "Default commandline: \n"
	     << "  " << getExecutable() << " " << getDefaults() << endl;
}

void MainApp::printVersion() const
{
	cout << getExecutable() << " " << getVersion() << "\n\n";
	cout << "Copyright (C) Arne König" << "\n";
	cout << "Copyright (C) Benjamin Kaufmann" << "\n";
	cout << "Copyright (C) Roland Kaminski" << "\n";
	cout << "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n";
	cout << "Gringo is free software: you are free to change and redistribute it.\n";
	cout << "There is NO WARRANTY, to the extent permitted by law." << endl;
}

std::string MainApp::getUsage() const
{
	return std::string("[options] [files]");
}

std::string MainApp::getExecutable() const
{
	return std::string(executable_);
}
