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

#ifndef GRINGO_H
#define GRINGO_H

#include <cassert>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <queue>
#include <map>
#include <list>
#include <stack>
#include <cmath>
#include <cfloat>
#include <typeinfo>
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <climits>
#include <memory>
#include <stdexcept>
#include <limits>

#define FAIL_CONCAT2(x,y) x "( " #y "): Assertion failed!"
#define FAIL_CONCAT(x, y) FAIL_CONCAT2(x, y)
#define FAIL(x) (void)(!(x) || (throw std::logic_error(FAIL_CONCAT(__FILE__,__LINE__)),true))

#if !defined(HAS_TR1_UNORDERED) || HAS_TR1_UNORDERED == 0
#include <ext/hash_set>
#include <ext/hash_map>
template<class Key, class T, class F = __gnu_cxx::hash<Key>, class Eq = std::equal_to<Key>, class A= std::allocator<T> >
struct HashMap {
    typedef __gnu_cxx::hash_map<Key, T, F, Eq, A> type;
};
template<class V, class H= __gnu_cxx::hash<V>, class Eq = std::equal_to<V>, class A=std::allocator<V> >
struct HashSet {
    typedef  __gnu_cxx::hash_set<V, H, Eq, A> type;
};
inline size_t hashString(const std::string& str) {
	return __gnu_cxx::__stl_hash_string(str.c_str());
}
#else
#include <unordered_map>
#include <unordered_set>
template<class Key, class Ty, class Hash = NS_TR1::hash<Key>, class Pred = std::equal_to<Key>, class Alloc = std::allocator<Key> >
struct HashMap {
    typedef NS_TR1::unordered_map<Key, Ty, Hash, Pred, Alloc> type;
};
template<class Value, class Hash = NS_TR1::hash<Value>, class Pred = std::equal_to<Value>, class Alloc = std::allocator<Value> >
struct HashSet {
    typedef NS_TR1::unordered_set<Value, Hash, Pred, Alloc> type;
};
inline size_t hashString(const std::string& str) {
	return NS_TR1::hash<std::string>()(str);
}
#endif


namespace gringo
{
	class GlobalStorage;

	class PDG;
	class PDGBuilder;

	class LDGBuilder;
	typedef std::vector<LDGBuilder*> LDGBuilderVector;

	class LDG;
	typedef std::vector<LDG*> LDGVector;

	typedef std::pair<int, int> Signature;
	typedef std::vector<Signature> SignatureVector;
	class Evaluator;

	class Expandable;
	class Groundable;

	class IndexedDomain;
	typedef std::vector<IndexedDomain*> IndexedDomainVector;

	class Value;
	typedef std::vector<Value> ValueVector;

	enum MatchStatus { SuccessfulMatch, FailureOnNextMatch, FailureOnFirstMatch };
	enum IncPart { NONE, BASE, LAMBDA, DELTA };
	enum GroundStep { PREPARE, REINIT, GROUND, RELEASE };

	class DLVGrounder;

	class StatementChecker;
	class Printable;
	class DependencyRelation;

	enum VarsType { VARS_PROVIDED, VARS_NEEDED, VARS_GLOBAL, VARS_ALL };
	typedef std::set<int> VarSet;
	typedef std::set<int> IntSet;
	typedef std::vector<int> VarVector;
	typedef std::vector<int> IntVector;

	class Variable;
	class Constant;
	typedef std::vector<std::string*> StringVector;
	typedef std::vector<Constant*> ConstantVector;

	class AggregateLiteral;

	class SDG;

	class GrinGoParser;
	class LparseParser;

	class GrinGoLexer;
	class LparseLexer;
	class PlainLparseLexer;

	class Grounder;

	class Literal;
	typedef std::vector<Literal*> LiteralVector;
	typedef std::list<Literal*> LiteralList;

	class SDGNode;
	typedef std::vector<SDGNode*> SDGNodeVector;

	class Statement;
	typedef std::vector<Statement*> StatementVector;

	class Term;
	typedef std::vector<Term*> TermVector;

	class ConditionalLiteral;
	typedef std::vector<ConditionalLiteral*> ConditionalLiteralVector;

	class PredicateLiteral;
	typedef std::vector<PredicateLiteral*> PredicateLiteralVector;

	class FuncSymbol;
	typedef std::vector<FuncSymbol* > FuncSymbolVector;

	class ConditionalLiteralTarget;

	inline std::ostream& NL(std::ostream& os)
	{
		return os.put(os.widen('\n'));
	}

	class Program;
	typedef std::vector<Program*> ProgramVector;

	class Domain;
	typedef std::vector<Domain*> DomainVector;

	namespace NS_OUTPUT
	{
		class Output;
		struct Object;
		struct Atom;
	}
}

#endif

