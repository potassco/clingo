// Copyright (c) 2009, Roland Kaminski <kaminski@cs.uni-potsdam.de>
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

#ifndef _GRINGO_H
#define _GRINGO_H

#define GRINGO_VERSION "3.0.5"

#include <gringo/val.h>
#include <gringo/clone_ptr.h>

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/ptr_container/ptr_unordered_map.hpp>
#include <boost/ptr_container/ptr_map.hpp>
#include <boost/unordered/unordered_set.hpp>
#include <boost/unordered/unordered_map.hpp>
#include <boost/functional/hash.hpp>
#include <boost/foreach.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/hashed_index.hpp>

#include <iostream>
#include <cstdio>
#include <stdexcept>
#include <limits>
#include <vector>
#include <memory>
#include <string>
#include <set>
#include <stack>
#include <map>
#include <queue>
#include <sstream>
#include <cassert>
#include <list>
#include <fstream>

#define foreach BOOST_FOREACH
inline bool unknown(
		boost::logic::tribool x,
		boost::logic::detail::indeterminate_t dummy = boost::logic::detail::indeterminate_t())
{
	(void)dummy;
	return x.value == boost::logic::tribool::indeterminate_value;
}

using boost::logic::tribool;

class ArgTerm;
class CondLit;
class ConstTerm;
class Domain;
class Expander;
class Func;
class Groundable;
class Grounder;
class Index;
class Instantiator;
class Lexer;
class Lit;
class LuaLit;
class LuaTerm;
class LparseConverter;
class MathLit;
class Output;
class Parser;
class PredIndex;
class PredLit;
class PredLitRep;
class PredLitSet;
class PrgVisitor;
class Printer;
class RelLit;
class RelLit;
class Rule;
class Statement;
class Storage;
class Term;
class VarCollector;
class VarTerm;
class WeightLit;
class IncLit;
struct IncConfig;
class Streams;

struct TermExpansion;
struct Loc;

namespace LitDep
{
	class GrdNode;
}

typedef std::vector<std::string> StringVec;
typedef boost::ptr_vector<Statement> StatementPtrVec;
typedef boost::ptr_vector<Index> IndexPtrVec;
typedef boost::ptr_vector<Lit> LitPtrVec;
typedef boost::ptr_vector<Term> TermPtrVec;
typedef std::vector<VarTerm*> VarTermVec;
typedef std::vector<std::string> StringVec;
typedef std::vector<Val> ValVec;
typedef std::vector<uint32_t> VarVec;
typedef std::map<uint32_t,uint32_t> VarMap;
typedef std::set<uint32_t> VarSet;
typedef std::pair<uint32_t, uint32_t> Signature;
typedef boost::ptr_unordered_map<Signature, Domain> DomainMap;
typedef boost::ptr_vector<CondLit> CondLitVec;
typedef boost::iterator_range<ValVec::const_iterator> ValRng;
typedef boost::iterator_range<StatementPtrVec::iterator> StatementRng;
typedef std::pair<Loc, uint32_t> VarSig;
typedef std::vector<VarSig> VarSigVec;
typedef std::auto_ptr<TermExpansion> TermExpansionPtr;

namespace boost
{
	template <>
	Term* new_clone(const Term& a);
	template <>
	VarTerm* new_clone(const VarTerm& a);
	template <>
	Lit* new_clone(const Lit& a);
	template <>
	WeightLit* new_clone(const WeightLit& a);
	template <>
	Instantiator* new_clone(const Instantiator& a);
	template <>
	LitDep::GrdNode* new_clone(const LitDep::GrdNode& a);
	template <>
	CondLit* new_clone(const CondLit& a);
}

#endif
