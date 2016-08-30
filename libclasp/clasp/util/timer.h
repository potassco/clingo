// 
// Copyright (c) 2006-2012, Benjamin Kaufmann
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
/*!
 * \file
 * \brief Defines various types for getting absolute times.
 */
namespace Clasp {

//! A type for getting the current process time.
struct ProcessTime {
	static double getTime();
};
//! A type for getting the current thread time.
struct ThreadTime {
	static double getTime();
};
//! A tpe for getting the current wall-clock time.
struct RealTime {
	static double getTime();
};
//! A class for measuring elapsed time.
/*!
 * \tparam TimeType must provide a single static function
 * TimeType::getTime() returning an absolute time.
 */
template <class TimeType>
class Timer {
public:
	Timer() : start_(0), split_(0), total_(0) {}
	
	void   start()   { start_ = TimeType::getTime(); }
	void   stop()    { split(TimeType::getTime()); }
	void   reset()   { *this  = Timer(); }
	//! Same as stop(), start();
	void   lap()     { double t; split(t = TimeType::getTime()); start_ = t; }
	//! Returns the elapsed time (in seconds) for last start-stop cycle.
	double elapsed() const { return split_; }
	//! Returns the total elapsed time for all start-stop cycles.
	double total()   const { return total_; }
private:
	void split(double t) { total_ += (split_ = t-start_); }
	double start_;
	double split_;
	double total_;
};

}
#endif
