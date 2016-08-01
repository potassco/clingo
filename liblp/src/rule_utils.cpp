// 
// Copyright (c) 2016, Benjamin Kaufmann
// 
// This file is part of Potassco. See http://potassco.sourceforge.net/
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// 
#include <potassco/rule_utils.h>
#include <cstring>
#include <stdexcept>
namespace Potassco {
Rule_t Rule_t::normal(Head_t ht, const AtomSpan& head, const LitSpan& body) {
	Rule_t r = {ht, head, Body_t::Normal, {body}};
	return r;
}
Rule_t Rule_t::sum(Head_t ht, const AtomSpan& head, const Sum_t& sum) {
	Rule_t r = {ht, head, Body_t::Sum, {}};
	r.agg = sum;
	return r;
}
Rule_t Rule_t::sum(Head_t ht, const AtomSpan& head, Weight_t bound, const WeightLitSpan& lits) {
	Sum_t s = {lits, bound};
	return sum(ht, head, s);
}
/////////////////////////////////////////////////////////////////////////////////////////
// RuleBuilder
/////////////////////////////////////////////////////////////////////////////////////////
struct RuleBuilder::RuleInfo {
	enum STATE { active = 1u, done = 2u };
	enum TYPE { rule = 0u, minimize = 1u };
	uint32_t type   : 30;
	uint32_t state  :  2;
	uint32_t head   : 28;
	uint32_t hType  :  2;
	uint32_t hState :  2;
	uint32_t hSize;
	uint32_t body   : 28;
	uint32_t bType  :  2;
	uint32_t bState :  2;
	uint32_t bSize;
	Weight_t bound;
};
RuleBuilder::RuleBuilder() {
	clear();
}
void RuleBuilder::require(bool cnd, const char* msg) const {
	if (!cnd) { throw std::logic_error(msg); }
}
RuleBuilder& RuleBuilder::clear() {
	data_.clear();
	RuleInfo* r = data_.push<RuleInfo>();
	std::memset(r, 0, sizeof(RuleInfo));
	r->state = RuleInfo::active;
	r->type = Directive_t::Rule;
	return *this;
}
RuleBuilder& RuleBuilder::clearBody() {
	RuleInfo* r = info();
	if (r->body >= r->head) {
		data_.setTop(r->body);
		r->bState = 0;
	}
	r->bSize = 0;
	r->bType = Body_t::Normal;
	return *this;
}

RuleBuilder::RuleInfo* RuleBuilder::info() const {
	return static_cast<RuleInfo*>(data_.get(0));
}
RuleBuilder::RuleInfo* RuleBuilder::init() {
	RuleInfo* r = info();
	if (r->state == RuleInfo::done) {
		clear();
		r = info();
	}
	return r;
}
RuleBuilder& RuleBuilder::start(Head_t ht) {
	RuleInfo* r = init();
	if (r->hState != RuleInfo::active) {
		require(r->hState == 0u && r->type == Directive_t::Rule, "invalid call to start()");
		endBody();
		r->head = data_.top();
		r->hType = ht;
		r->hState = RuleInfo::active;
		r->hSize = 0;
	}
	return *this;
}
RuleBuilder& RuleBuilder::addHead(Atom_t a) {
	RuleInfo* r = init();
	if (r->hState != RuleInfo::active) { start(); }
	++r->hSize;
	*data_.push<Atom_t>() = a;
	return *this;
}
RuleBuilder& RuleBuilder::startBody() { startBody(Body_t::Normal, -1); return *this; }
RuleBuilder& RuleBuilder::startSum(Weight_t bound) { startBody(Body_t::Sum, bound); return *this; }
RuleBuilder& RuleBuilder::startMinimize(Weight_t prio) {
	RuleInfo* r = init();
	require(!r->head && !r->body, "invalid call to startMinimize()");
	r->type = Directive_t::Minimize;
	return startSum(prio);
}
RuleBuilder::RuleInfo* RuleBuilder::startBody(Body_t bt, Weight_t bnd) {
	RuleInfo* r = init();
	if (r->bState != RuleInfo::active) {
		require(r->bState == 0u, "invalid call to startBody()");
		endHead();
		r->body = data_.top();
		r->bType = bt;
		r->bState = RuleInfo::active;
		r->bound = bnd;
		r->bSize = 0;
	}
	return r;
}
RuleBuilder& RuleBuilder::addGoal(Lit_t lit) {
	return addGoal(lit, 1);
}
RuleBuilder& RuleBuilder::addGoal(Lit_t lit, Weight_t w) {
	WeightLit_t wl = {lit, w};
	addGoal(wl);
	return *this;
}
RuleBuilder& RuleBuilder::addGoal(WeightLit_t lit) {
	RuleInfo* r = startBody(Body_t::Normal, -1);
	if (lit.weight == 0) { return *this; }
	++r->bSize;
	if (r->bType == Body_t::Normal) {
		*data_.push<Lit_t>() = lit.lit;
	}
	else {
		*data_.push<WeightLit_t>() = lit;
	}
	return *this;
}
RuleBuilder& RuleBuilder::setBound(Weight_t bound) {
	info()->bound = bound;
	return *this;
}
void RuleBuilder::endHead() {
	RuleInfo* r = init();
	if (r->hState == RuleInfo::active) { r->hState = RuleInfo::done; }
}
void RuleBuilder::endBody() {
	RuleInfo* r = init();
	if (r->bState == RuleInfo::active) { r->bState = RuleInfo::done; }
}
RuleBuilder& RuleBuilder::end(AbstractProgram* out) {
	RuleInfo* r = info();
	if (r->state != RuleInfo::done) {
		if (!r->head && r->type == Directive_t::Rule) { start(); }
		endHead();
		if (!r->body) { startBody(); }
		endBody();
		r->state = RuleInfo::done;
	}
	if (!out) { return *this; }
	if (r->type == Directive_t::Minimize) {
		out->minimize(r->bound, toSpan(static_cast<const WeightLit_t*>(data_.get(r->body)), r->bSize));
	}
	else if (r->bType == Body_t::Normal) {
		out->rule(static_cast<Head_t>(r->hType), toSpan(static_cast<const Atom_t*>(data_.get(r->head)), r->hSize),
			toSpan(static_cast<const Lit_t*>(data_.get(r->body)), r->bSize));
	}
	else {
		out->rule(static_cast<Head_t>(r->hType), toSpan(static_cast<const Atom_t*>(data_.get(r->head)), r->hSize),
			r->bound, toSpan(static_cast<const WeightLit_t*>(data_.get(r->body)), r->bSize));
	}
	return *this;
}
uint32_t     RuleBuilder::bodySize() const { return info()->bSize; }
uint32_t     RuleBuilder::headSize() const { return info()->hSize; }
Body_t       RuleBuilder::bodyType() const { return static_cast<Body_t>(info()->bType); }
Atom_t*      RuleBuilder::head()     const { return static_cast<Atom_t*>(data_.get(info()->head)); }
Lit_t*       RuleBuilder::body()     const { return static_cast<Lit_t*>(data_.get(info()->body)); }
WeightLit_t* RuleBuilder::sum()      const { return static_cast<WeightLit_t*>(data_.get(info()->body)); }
Weight_t     RuleBuilder::bound()    const { return info()->bound; }
RuleBuilder& RuleBuilder::weaken(Body_t to, bool w) {
	RuleInfo* r = info();
	if (r->bType != Body_t::Normal && to != r->bType) {
		if (to == Body_t::Normal) {
			Lit_t* x = body();
			for (WeightLit_t* w = sum(), *end = w + r->bSize; w != end; ++w) { *x++ = w->lit; }
			data_.setTop(r->body + (r->bSize * sizeof(Lit_t)));
			r->bType = Body_t::Normal;
		}
		else if (to == Body_t::Count) {
			Weight_t min = 1;
			if (w && r->bSize) {
				min = sum()->weight;
				for (WeightLit_t* it = sum(), *end = it + r->bSize; it != end; ++it) {
					if (min > it->weight) { min = it->weight; }
					it->weight = 1;
				}
			}
			r->bound = (r->bound+(min-1))/min;
			r->bType = Body_t::Count;
		}
	}
	return *this;
}
Rule_t RuleBuilder::rule() const {
	RuleInfo* r = info();
	Rule_t ret;
	ret.ht   = static_cast<Head_t>(r->hType);
	ret.head = toSpan(static_cast<const Atom_t*>(data_.get(r->head)), r->hSize);
	ret.bt   = static_cast<Body_t>(r->bType);
	if (r->bType == Body_t::Normal) {
		ret.cond = toSpan(static_cast<const Lit_t*>(data_.get(r->body)), r->bSize);
	}
	else {
		ret.agg.bound = r->bound;
		ret.agg.lits = toSpan(static_cast<const WeightLit_t*>(data_.get(r->body)), r->bSize);
	}
	return ret;
}
}
