// 
// Copyright (c) 2006-2011, Benjamin Kaufmann
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
#include <clasp/solver_types.h>
#include <clasp/solver.h>
#include <clasp/clause.h>
#include <new>
namespace Clasp {
/////////////////////////////////////////////////////////////////////////////////////////
// SolverStats
/////////////////////////////////////////////////////////////////////////////////////////
bool SolverStats::enableStats(const SolverStats& o) {
	if (o.extra && !enableExtended()) { return false; }
	if (o.jumps && !enableJump())     { return false; }
	return true;
}
bool SolverStats::enableExtended() { 
	if (!extra) { extra = new (std::nothrow) ExtendedStats(); }
	return extra != 0;
}
bool SolverStats::enableJump() {
	if (!jumps) { jumps = new (std::nothrow) JumpStats(); }
	return jumps != 0;
}
void SolverStats::enableQueue(uint32 size) { 
	if (queue && queue->maxSize()!=size) { queue->destroy(); queue = 0; }
	if (!queue)                          { queue = SumQueue::create(size); }
}
void SolverStats::reset() {
	CoreStats::reset();
	if (queue) queue->resetGlobal();
	if (extra) extra->reset();
	if (jumps) jumps->reset();
}
void SolverStats::accu(const SolverStats& o) {
	CoreStats::accu(o);
	if (extra && o.extra) extra->accu(*o.extra);
	if (jumps && o.jumps) jumps->accu(*o.jumps);
}
void SolverStats::swapStats(SolverStats& o) {
	std::swap(static_cast<CoreStats&>(*this), static_cast<CoreStats&>(o));
	std::swap(extra, o.extra);
	std::swap(jumps, o.jumps);
}
double SolverStats::operator[](const char* path) const {
	bool ext = matchStatPath(path, "extra");
	if (ext || matchStatPath(path, "jumps")) {
		if (!*path)      { return -2.0; }
		if (ext && extra){ return (*extra)[path]; }
		if (!ext&& jumps){ return (*jumps)[path]; }
		return -3.0; 
	}
	return CoreStats::operator[](path);
}
const char* SolverStats::subKeys(const char* path) const {
	bool ext = matchStatPath(path, "extra");
	if (ext || matchStatPath(path, "jumps")) {
		return ext ? ExtendedStats::keys(path) : JumpStats::keys(path); 
	}
	return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////
// ClauseHead
/////////////////////////////////////////////////////////////////////////////////////////
ClauseHead::Info::Info(const Clasp::ClauseInfo& init) {
	data.act  = init.activity();
	data.key  = !init.tagged() ? 0 : TAGGED_CLAUSE;
	data.lbd  = std::min(init.lbd(), uint32(ClauseHead::MAX_LBD));
	data.type = init.type();
}

ClauseHead::ClauseHead(const ClauseInfo& init) : info_(init){
	static_assert(sizeof(ClauseHead)<=32, "Unsupported Alignment");
	head_[2]  = negLit(0);
}

void ClauseHead::attach(Solver& s) {
	assert(head_[0] != head_[1] && head_[1] != head_[2]);
	s.addWatch(~head_[0], ClauseWatch(this));
	s.addWatch(~head_[1], ClauseWatch(this));
}

void ClauseHead::detach(Solver& s) {
	s.removeWatch(~head_[0], this);
	s.removeWatch(~head_[1], this);
}

bool ClauseHead::locked(const Solver& s) const {
	return (s.isTrue(head_[0]) && s.reason(head_[0]) == this)
	  ||   (s.isTrue(head_[1]) && s.reason(head_[1]) == this);
}

bool ClauseHead::satisfied(const Solver& s) {
	return s.isTrue(head_[0]) || s.isTrue(head_[1]) || s.isTrue(head_[2]);
}

bool ClauseHead::toImplication(Solver& s) {
	ConstraintType t  = ClauseHead::type();
	uint32 sz         = isSentinel(head_[1]) ? 1 : 2 + (!s.isFalse(head_[2]) || s.level(head_[2].var()) > 0);
	ClauseRep rep     = ClauseRep::create(head_, sz, ClauseInfo(t).setLbd(2).setTagged(tagged()));
	bool   implicit   = s.allowImplicit(rep);
	bool   locked     = ClauseHead::locked(s) && s.decisionLevel() > 0;
	rep.prep          = 1;
	if ((locked || !implicit) && sz > 1) { return false; }
	s.add(rep, false);
	detach(s);
	return true;
}
/////////////////////////////////////////////////////////////////////////////////////////
// SmallClauseAlloc
/////////////////////////////////////////////////////////////////////////////////////////
SmallClauseAlloc::SmallClauseAlloc() : blocks_(0), freeList_(0) { }
SmallClauseAlloc::~SmallClauseAlloc() { 
	Block* r = blocks_;
	while (r) {
		Block* t = r;
		r = r->next;
		::operator delete(t);
	}
}

void SmallClauseAlloc::allocBlock() {
	Block* r = (Block*)::operator new(sizeof(Block));
	for (uint32 i = 0; i < Block::num_chunks-1; ++i) {
		r->chunk[i].next = &r->chunk[i+1];
	}
	r->chunk[Block::num_chunks-1].next = freeList_;
	freeList_ = r->chunk;
	r->next   = blocks_;
	blocks_   = r;
}

}
