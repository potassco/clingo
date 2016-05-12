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

#include "gringo/gringo_app.h"
#include <gringo/inclit.h>
#include <gringo/parser.h>
#include <gringo/converter.h>
#include <gringo/grounder.h>
#include <gringo/plainoutput.h>
#include <gringo/lparseoutput.h>
#include <gringo/reifiedoutput.h>

namespace
{
	bool parsePositional(const std::string&, std::string& out)
	{
		out = "file";
		return true;
	}
}

Output *GringoApp::output() const
{
	if (gringo.metaOut)
		return new ReifiedOutput(&std::cout);
	else if (gringo.textOut)
		return new PlainOutput(&std::cout);
	else
		return new LparseOutput(&std::cout, gringo.disjShift);
}

GringoApp& GringoApp::instance()
{
	static GringoApp app;
	return app;
}

Streams::StreamPtr GringoApp::constStream() const
{
	std::auto_ptr<std::stringstream> constants(new std::stringstream());
	for(std::vector<std::string>::const_iterator i = gringo.consts.begin(); i != gringo.consts.end(); ++i)
		*constants << "#const " << *i << ".\n";
	return Streams::StreamPtr(constants.release());
}

int GringoApp::doRun()
{
	std::auto_ptr<Output> o(output());
	Streams  inputStreams(generic.input, constStream());
	if(gringo.groundInput)
	{
		Storage   s(o.get());
		Converter c(o.get(), inputStreams);

		(void)s;
		o->initialize();
		c.parse();
		o->finalize();
	}
	else
	{
		IncConfig config;
		Grounder  g(o.get(), generic.verbose > 2, gringo.termExpansion(config));
		Parser    p(&g, config, inputStreams, gringo.compat);

		config.incBegin = 1;
		config.incEnd   = config.incBegin + gringo.ifixed;
		config.incBase  = gringo.ibase;

		o->initialize();
		p.parse();
		g.analyze(gringo.depGraph, gringo.stats);
		g.ground();
		o->finalize();
	}

	return EXIT_SUCCESS;
}

ProgramOptions::PosOption GringoApp::getPositionalParser() const
{
	return &parsePositional;
}

void GringoApp::handleSignal(int sig)
{
	(void)sig;
	printf("\n*** INTERRUPTED! ***\n");
	_exit(S_UNKNOWN);
}

std::string GringoApp::getVersion() const
{
	return GRINGO_VERSION;
}
