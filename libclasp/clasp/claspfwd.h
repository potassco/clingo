// 
// Copyright (c) 2013-2016, Benjamin Kaufmann
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
#ifndef CLASP_CLASP_FWD_H_INCLUDED
#define CLASP_CLASP_FWD_H_INCLUDED
namespace Potassco {
	class TheoryAtom;
	class TheoryTerm;
	class TheoryData;
	template <class T> struct Span;
	struct Heuristic_t;
	class BufferedStream;
}
//! Root namespace for all types and functions of libclasp.
namespace Clasp { 
class SharedContext;
class MinimizeBuilder;
class SharedMinimizeData;
class Configuration;
class Constraint;
class ConstraintInfo;
class Solver;
struct Model;
struct Problem_t {
	enum Type {Sat = 0, Pb = 1, Asp = 2};
};
typedef Problem_t::Type ProblemType;
class ProgramBuilder;
class ProgramParser;
class SatBuilder;
class PBBuilder;
class ExtDepGraph;
class ConstString;
typedef Potassco::Span<char> StrView;
typedef Potassco::Heuristic_t DomModType;
//! Namespace for types and functions used to define ASP programs.
namespace Asp {
class LogicProgram;
class Preprocessor;
class LpStats;
class PrgAtom;
class PrgBody;
class PrgDisj;
class PrgHead;
class PrgNode;
class PrgDepGraph;
struct PrgEdge;
}}

#endif
