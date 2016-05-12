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

#include <clasp/constraint.h>
#include <string>
namespace Clasp {
/////////////////////////////////////////////////////////////////////////////////////////
// (Learnt)Constraint
/////////////////////////////////////////////////////////////////////////////////////////
Constraint::Constraint()                  {}
Constraint::~Constraint()                 {}
bool Constraint::simplify(Solver&, bool)  { return false; }
void Constraint::destroy()                { delete this; }
void Constraint::undoLevel(Solver&)       {}
uint32 Constraint::estimateComplexity(const Solver&) const { return 1;  }
LearntConstraint::~LearntConstraint()     {}
LearntConstraint::LearntConstraint()      {}
uint32 LearntConstraint::activity() const { return 0; }
void LearntConstraint::decreaseActivity() {}
/////////////////////////////////////////////////////////////////////////////////////////
// PostPropagator
/////////////////////////////////////////////////////////////////////////////////////////
PostPropagator::PostPropagator() : next(0)           {}
PostPropagator::~PostPropagator()                    {}
uint32 PostPropagator::priority() const              { return uint32(priority_single); }
void PostPropagator::reset()                         {}
bool PostPropagator::isModel(Solver&)                { return true; }
bool PostPropagator::nextSymModel(Solver&, bool)     { return false; }
ConstraintType PostPropagator::reason(const Literal&, LitVec&) { return PostPropagator::type(); }
ConstraintType PostPropagator::type() const                    { return Constraint_t::static_constraint; }
Constraint::PropResult PostPropagator::propagate(const Literal&, uint32&, Solver&) { 
	return PropResult(true, false); 
}
/////////////////////////////////////////////////////////////////////////////////////////
// Antecedent
/////////////////////////////////////////////////////////////////////////////////////////
PlatformError::PlatformError(const char* msg) : ClaspError(std::string("Platform Error: ")+msg) {}
bool Antecedent::checkPlatformAssumptions() {
	int32* i = new int32(22);
	uint64 p = (uint64)(uintp)i;
	bool convOk = ((int32*)(uintp)p) == i;
	bool alignmentOk = (p & 3) == 0;
	delete i;
	if ( !alignmentOk ) {
		throw PlatformError("Unsupported Pointer-Alignment!");
	}
	if ( !convOk ) {
		throw PlatformError("Can't convert between Pointer and Integer!");
	}
	p = ~uintp(1);
	store_set_bit(p, 0);
	if (!test_bit(p, 0)) {
		throw PlatformError("Can't set LSB in pointer!");
	}
	store_clear_bit(p, 0);
	if (p != (~uintp(1))) {
		throw PlatformError("Can't restore LSB in pointer!");
	}
	Literal max = posLit(varMax-1);
	Antecedent a(max);
	if (a.type() != Antecedent::binary_constraint || a.firstLiteral() != max) {
		throw PlatformError("Cast between 64- and 32-bit integer does not work as expected!");
	}
	Antecedent b(max, ~max);
	if (b.type() != Antecedent::ternary_constraint || b.firstLiteral() != max || b.secondLiteral() != ~max) {
		throw PlatformError("Cast between 64- and 32-bit integer does not work as expected!");
	}
	return true;
}

}
