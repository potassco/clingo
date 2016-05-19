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

#include <gringo/lparseoutput.h>
#include <gringo/globalstorage.h>

using namespace gringo;
using namespace NS_OUTPUT;
		
LparseOutput::LparseOutput(std::ostream *out) : Output(out)
{
	stats_.language = Stats::TEXT;
}

void LparseOutput::initialize(GlobalStorage *g, SignatureVector *pred)
{
	Output::initialize(g, pred);
	if(hideAll_)
		*out_ << "#hide." << std::endl;
	for(std::map<Signature, bool>::iterator it = hide_.begin(); it  != hide_.end(); it++)
	{
		if(hideAll_ != it->second)
		{
			*out_ << (hideAll_ ? "#show " : "#hide ") << *g_->getString(it->first.first);
			if(it->first.second > 0)
			{
				*out_ << "(X0";
				for(int i=1; i < it->first.second; i++)
					*out_ << ", X" << i;
				*out_ << ")";
			}
			*out_ << "." << std::endl;
		}
	}
}

void LparseOutput::print(NS_OUTPUT::Object *r)
{
	r->print_plain(this, *out_);
}

void LparseOutput::finalize(bool last)
{
}

LparseOutput::~LparseOutput()
{
}

