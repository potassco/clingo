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
#include <clasp/solver_types.h>
#include <clasp/solver.h>
#include <clasp/clause.h>
#include <new>
namespace Clasp {
///////////////////////////////////////////////////////////////////////////////
// SearchLimits
///////////////////////////////////////////////////////////////////////////////
SearchLimits::SearchLimits() {
	std::memset(this, 0, sizeof(SearchLimits));
	restart.conflicts = UINT64_MAX;
	conflicts = UINT64_MAX;
	memory    = UINT64_MAX;
	learnts   = UINT32_MAX;
}
/////////////////////////////////////////////////////////////////////////////////////////
// DynamicLimit
/////////////////////////////////////////////////////////////////////////////////////////
DynamicLimit* DynamicLimit::create(uint32 size) {
	CLASP_FAIL_IF(size == 0, "size must be > 0");
	void* m = ::operator new(sizeof(DynamicLimit) + (size*sizeof(uint32)));
	return new (m)DynamicLimit(size);
}
DynamicLimit::DynamicLimit(uint32 sz) : cap_(sz), pos_(0), num_(0) {
	std::memset(&global, 0, sizeof(global));
	init(0.7f, lbd_limit);
}
void DynamicLimit::init(float k, Type t, uint32 uLimit) {
	resetRun();
	std::memset(&adjust, 0, sizeof(adjust));
	adjust.limit = uLimit;
	adjust.rk    = k;
	adjust.type  = t;
}
void DynamicLimit::destroy() {
	this->~DynamicLimit(); 
	::operator delete(this);
}
void DynamicLimit::resetRun() {
	sum_[0] = sum_[1] = pos_ = num_ = 0;
}
void DynamicLimit::reset() {
	std::memset(&global, 0, sizeof(global));
	resetRun();
}
void DynamicLimit::update(uint32 dl, uint32 lbd) {
	// update global avg
	++adjust.samples;
	++global.samples;
	global.sum[lbd_limit]   += lbd;
	global.sum[level_limit] += dl;
	// update moving avg
	sum_[lbd_limit]   += lbd;
	sum_[level_limit] += dl;
	if (++num_ > cap_) {
		sum_[lbd_limit]   -= (buffer_[pos_] & 127u);
		sum_[level_limit] -= (buffer_[pos_] >> 7u);
	}
	buffer_[pos_++] = ((dl << 7) + lbd);
	if (pos_ == cap_) { pos_ = 0; }
}
uint32 DynamicLimit::restart(uint32 maxLBD, float k) {
	++adjust.restarts;
	if (adjust.samples >= adjust.limit) {
		Type nt = global.avg(lbd_limit) > maxLBD ? level_limit : lbd_limit;
		if (nt == adjust.type) {
			double rLen = adjust.avgRestart();
			bool   sx   = num_ >= adjust.limit;
			float  rk   = adjust.rk;
			uint32 uLim = adjust.limit;
			if      (rLen >= 16000.0){ rk += 0.1f;  uLim = 16000; }
			else if (sx)             { rk += 0.05f; uLim = std::max(uint32(16000), uLim-10000); }
			else if (rLen >= 4000.0) { rk += 0.05f; }
			else if (rLen >= 1000.0) { uLim += 10000u; }
			else if (rk > k)         { rk -= 0.05f; }
			init(rk, nt, uLim);
		}
		else { init(k, nt); }
	}
	else { resetRun(); }
	return adjust.limit;
}
BlockLimit::BlockLimit(uint32 windowSize, double R) 
	: ema(0.0), alpha(2.0/(windowSize+1))
	, next(windowSize), inc(50), n(0)
	, span(windowSize)
	, r(static_cast<float>(R)) {
}
/////////////////////////////////////////////////////////////////////////////////////////
// SolverStats
/////////////////////////////////////////////////////////////////////////////////////////
SolverStats::SolverStats() : limit(0), extra(0), jumps(0) {}
SolverStats::SolverStats(const SolverStats& o) : CoreStats(o), limit(0), extra(0), jumps(0) { 
	enableStats(o); 
}
SolverStats::~SolverStats() { 
	delete jumps; 
	delete extra;
	if (limit) limit->destroy();
}
int SolverStats::level() const { 
	return (extra != 0) + (jumps != 0); 
}
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
void SolverStats::enableLimit(uint32 size) { 
	if (limit && limit->window() != size) { limit->destroy(); limit = 0; }
	if (!limit)                           { limit = DynamicLimit::create(size); }
}
void SolverStats::reset() {
	CoreStats::reset();
	if (limit) limit->reset();
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
ClauseHead::ClauseHead(const InfoType& init) : info_(init){
	static_assert(sizeof(ClauseHead)<=32, "Unsupported Alignment");
	head_[2] = lit_false();
}
void ClauseHead::resetScore(ScoreType sc) {
	info_.setScore(sc);
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

bool ClauseHead::satisfied(const Solver& s) const {
	return s.isTrue(head_[0]) || s.isTrue(head_[1]) || s.isTrue(head_[2]);
}

bool ClauseHead::toImplication(Solver& s) {
	uint32 sz     = isSentinel(head_[1]) ? 1 : 2 + (!s.isFalse(head_[2]) || s.level(head_[2].var()) > 0);
	ClauseRep rep = ClauseRep::create(head_, sz, InfoType(ClauseHead::type()).setLbd(2).setTagged(tagged()));
	bool implicit = s.allowImplicit(rep);
	bool locked   = ClauseHead::locked(s) && s.decisionLevel() > 0;
	rep.prep      = 1;
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
