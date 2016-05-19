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

#include <gringo/gringolexer.h>

using namespace gringo;

GrinGoLexer::GrinGoLexer(std::istream *in) : in(in), bufmin(8192), bufsize(0), buffer(0), start(0), offset(0), cursor(0), limit(0), marker(0), eof(0), line(1)
{
}
	
std::string GrinGoLexer::getToken()
{
	return std::string(start, cursor);
}

void GrinGoLexer::step()
{
	offset = start;
	line++;
}

void GrinGoLexer::reset(std::istream *i)
{
	in     = i;
	start  = buffer;
	cursor = buffer;
	limit  = buffer;
	marker = buffer;
	eof    = 0;
	offset = buffer;
	line   = 1;
}

int GrinGoLexer::getLine()
{
	return line;
}

int GrinGoLexer::getColumn()
{
	return start - offset;
}

GrinGoLexer::~GrinGoLexer()
{
	if(buffer)
	{
		free(buffer);
		buffer = 0;
	}
}

// its ugly to write this one on your own but offers great flexibility
void GrinGoLexer::fill(int n)
{
	if(!eof) 
	{
		// make shure that the last token is completely stored in the buffer
		int shift = start - buffer;
		if(shift > 0)
		{
			memmove(buffer, start, limit - start);
			start  = buffer;
			offset-= shift;
			marker-= shift;
			limit -= shift;
			cursor-= shift;
		}
		// make shure we can read at least max(n,bufmin) bytes
		int inc   = std::max(n, bufmin);
		if(bufsize < inc + (limit - buffer))
		{
			bufsize = inc + (limit - buffer);
			char *buf = (char*)realloc(buffer, bufsize * sizeof(char));
			start  = buf + (start - buffer);
			cursor = buf + (cursor - buffer);
			limit  = buf + (limit - buffer);
			marker = buf + (marker - buffer);
			offset = buf + (offset - buffer);
			buffer = buf;

		}
		// read new bytes from stream
		in->read(limit, inc);
		limit+= in->gcount();
		// handle eof specially: set last char to \n
		if(in->gcount() < inc)
		{
			eof = limit;
			*eof++ = '\n';
		}
	}
}

