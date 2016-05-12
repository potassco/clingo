// 
// Copyright (c) 2013, Benjamin Kaufmann
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
#ifndef CLASP_CLI_FWD_H_INCLUDED
#define CLASP_CLI_FWD_H_INCLUDED
#include <iosfwd>
namespace ProgramOptions {
class  OptionContext;
class  OptionGroup;
class  ParsedOptions;
}
namespace Clasp { namespace Cli {
template <class T> bool store(const char* value, T& out);
bool isDisabled(const char* optValue);
}}
#endif
