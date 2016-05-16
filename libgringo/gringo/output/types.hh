// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Roland Kaminski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// }}}

#ifndef _GRINGO_OUTPUT_TYPES_HH
#define _GRINGO_OUTPUT_TYPES_HH

#include <gringo/domain.hh>
#include <gringo/types.hh>

namespace Gringo { namespace Output {

class Translator;
class TheoryData;
class PredicateDomain;
class LiteralId;
class DisjointElement;
class Statement;
struct AuxAtom;
struct PrintPlain;
class DomainData;
class OutputBase;

using LitVec = std::vector<LiteralId>;
using ClauseId = std::pair<Id_t, Id_t>;
using FormulaId = std::pair<Id_t, Id_t>;
using Formula = std::vector<ClauseId>;
using CSPBound = std::pair<int, int>;
using AssignmentLookup = std::function<std::pair<bool, Potassco::Value_t>(unsigned)>; // (isExternal, truthValue)
using IsTrueLookup = std::function<bool(unsigned)>;
using OutputPredicates = std::vector<std::tuple<Location, Sig, bool>>;
using CoefVarVec = std::vector<std::pair<int, Symbol>>;

struct UPredDomHash;
struct UPredDomEqualTo;
using PredDomMap = UniqueVec<std::unique_ptr<PredicateDomain>, UPredDomHash, UPredDomEqualTo>;

enum class OutputDebug { NONE, TEXT, TRANSLATE, ALL };
enum class OutputFormat { TEXT, INTERMEDIATE, SMODELS, REIFY };

} } // namespace Output Gringo

#endif // _GRINGO_OUTPUT_TYPES_HH
