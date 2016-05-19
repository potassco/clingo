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

#ifndef CLASP_TIMER_H_INCLUDED
#define CLASP_TIMER_H_INCLUDED

#ifdef _MSC_VER
#pragma once
#endif

class Timer {
public:
	Timer() : start_(0), last_(0), elapsed_(0) {}
	void   start()   { start_    = clockStamp(); }
	void   stop()    { last_ = clockStamp() - start_; elapsed_ += last_; }
	void   reset()   { *this  = Timer(); }
	double current() const 
	{ 
		return last_ / ticksPerSec();
	}
	double elapsed() const { 
		return elapsed_ / ticksPerSec();
	}
	static double clockStamp();
	static double ticksPerSec();
	operator double() const
	{
		return elapsed();
	}
private:
	double start_;
	double last_;
	double elapsed_;
};

#endif
