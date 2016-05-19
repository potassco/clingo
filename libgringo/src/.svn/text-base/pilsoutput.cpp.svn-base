// Copyright (c) 2008, Roland Kaminski
//
// This file is part of GrinGo.
//
// GrinGo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// GrinGo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with GrinGo.  If not, see <http://www.gnu.org/licenses/>.

#include <gringo/pilsoutput.h>

using namespace gringo;
using namespace NS_OUTPUT;
		
PilsOutput::PilsOutput(std::ostream *out, unsigned int normalForm) : Output(out), normalForm_(normalForm)
{
	stats_.language = Stats::ASPILS;
}

const char* HEADER = "1";
const char* VERSION = "1";

void PilsOutput::initialize(GlobalStorage *g, SignatureVector *pred)
{
	Output::initialize(g, pred);

	*out_ << HEADER << " " << "3" << " " << VERSION << " " << normalForm_ << " " << "0 0" << NL;
}

void PilsOutput::print(NS_OUTPUT::Object *r)
{
	r->print(this, *out_);
}

void PilsOutput::finalize(bool last)
{
	if(last)
	{
		//write the optimize statement for all added sums
		if (optimizedIDs_.size())
		{
			*out_ << "1d" << " " << optimizedIDs_.size() + 2 << " " << newUid() << " " << "1e" << " " << optimizedIDs_.size();
			for (IntVector::const_iterator i = optimizedIDs_.begin(); i != optimizedIDs_.end(); ++i)
			{
				*out_ << " " << *i;
			}
			*out_ << " 0" << NL;
		}
		// end of output
		*out_ << "0 0 0" << NL;
	}
}

void PilsOutput::addOptimizedID(unsigned int id)
{
	optimizedIDs_.push_back(id);
}

unsigned int PilsOutput::getNormalForm() const
{
	return normalForm_;
}

PilsOutput::~PilsOutput()
{
}

