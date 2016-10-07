//
// Copyright (c) 2006-2016, Benjamin Kaufmann
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
namespace Clasp {
/////////////////////////////////////////////////////////////////////////////////////////
// Constraint
/////////////////////////////////////////////////////////////////////////////////////////
Constraint::Constraint()                  {}
Constraint::~Constraint()                 {}
void Constraint::destroy(Solver*, bool)   { delete this; }
ConstraintType Constraint::type() const   { return Constraint_t::Static; }
bool Constraint::simplify(Solver&, bool)  { return false; }
void Constraint::undoLevel(Solver&)       {}
uint32 Constraint::estimateComplexity(const Solver&) const { return 1;  }
bool Constraint::valid(Solver&)           { return true; }
ClauseHead* Constraint::clause()          { return 0; }
void Constraint::decreaseActivity()       {}
void Constraint::resetActivity()          {}
ConstraintScore Constraint::activity() const { return makeScore(); }
bool Constraint::locked(const Solver&) const { return true; }
uint32 Constraint::isOpen(const Solver&, const TypeSet&, LitVec&) { return 0; }
/////////////////////////////////////////////////////////////////////////////////////////
// PostPropagator
/////////////////////////////////////////////////////////////////////////////////////////
PostPropagator::PostPropagator() : next(0)           {}
PostPropagator::~PostPropagator()                    {}
bool PostPropagator::init(Solver&)                   { return true; }
void PostPropagator::reset()                         {}
bool PostPropagator::isModel(Solver& s)              { return valid(s); }
void PostPropagator::reason(Solver&, Literal, LitVec&) {}
Constraint::PropResult PostPropagator::propagate(Solver&, Literal, uint32&) {
	return PropResult(true, false);
}
void PostPropagator::cancelPropagation() {
	for (PostPropagator* n = this->next; n; n = n->next) { n->reset(); }
}
MessageHandler::MessageHandler() {}
/////////////////////////////////////////////////////////////////////////////////////////
// PropagatorList
/////////////////////////////////////////////////////////////////////////////////////////
PropagatorList::PropagatorList() : head_(0) {}
PropagatorList::~PropagatorList() { clear(); }
void PropagatorList::clear() {
	for (PostPropagator* r = head_; r;) {
		PostPropagator* t = r;
		r = r->next;
		t->destroy();
	}
	head_ = 0;
}
void PropagatorList::add(PostPropagator* p) {
	CLASP_ASSERT_CONTRACT_MSG(p && p->next == 0, "Invalid post propagator");
	uint32 prio = p->priority();
	for (PostPropagator** r = head(), *x;; r = &x->next) {
		if ((x = *r) == 0 || prio < (uint32)x->priority()) {
			p->next = x;
			*r      = p;
			break;
		}
	}
}
void PropagatorList::remove(PostPropagator* p) {
	CLASP_ASSERT_CONTRACT_MSG(p, "Invalid post propagator");
	for (PostPropagator** r = head(), *x; *r; r = &x->next) {
		if ((x = *r) == p) {
			*r      = x->next;
			p->next = 0;
			break;
		}
	}
}
PostPropagator* PropagatorList::find(uint32 prio) const {
	for (PostPropagator* x = head_; x; x = x->next) {
		uint32 xp = x->priority();
		if (xp >= prio) { return xp == prio ? x : 0; }
	}
	return 0;
}
}
