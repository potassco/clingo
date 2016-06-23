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
#ifndef CLASP_SOLVER_TYPES_H_INCLUDED
#define CLASP_SOLVER_TYPES_H_INCLUDED
#ifdef _MSC_VER
#pragma warning (disable : 4200) // nonstandard extension used : zero-sized array
#pragma once
#endif

#include <clasp/literal.h>
#include <clasp/constraint.h>
#include <clasp/util/left_right_sequence.h>
#include <clasp/util/misc_types.h>
#include <clasp/util/type_manip.h>
#include <numeric>
/*!
 * \file 
 * Contains some types used by a Solver
 */
namespace Clasp {
class SharedLiterals;

/*!
 * \addtogroup solver
 */
//@{
///////////////////////////////////////////////////////////////////////////////
// SearchLimits
///////////////////////////////////////////////////////////////////////////////
struct DynamicLimit;
struct BlockLimit;

//! Parameter-Object for managing search limits.
struct SearchLimits {
	typedef DynamicLimit* LimitPtr;
	typedef BlockLimit*   BlockPtr;
	SearchLimits();
	uint64 used;
	struct {
		uint64   conflicts; /*!< Soft limit on number of conflicts for restart. */
		LimitPtr dynamic;   /*!< Use dynamic restarts based on lbd or conflict level. */
		BlockPtr block;     /*!< Optional strategy to increase restart limit. */
		bool     local;     /*!< Apply conflict limit against active branch.  */
	} restart;
	uint64 conflicts; /*!< Soft limit on number of conflicts. */
	uint64 memory;    /*!< Soft memory limit for learnt lemmas (in bytes). */
	uint32 learnts;   /*!< Limit on number of learnt lemmas. */
};

struct DynamicLimit {
	enum Type { lbd_limit, level_limit };
	static DynamicLimit* create(uint32 window);
	void   destroy();
	// resets moving average and adjust
	void   init(float k, Type type, uint32 uLimit = 16000);
	// resets moving average
	void   resetRun();
	// resets moving and global average
	void   reset();
	void   update(uint32 conflictLevel, uint32 lbd);
	uint32 restart(uint32 maxLBD, float k);
	
	uint32 runLen()  const { return num_;  }
	uint32 window()  const { return cap_; }
	bool   reached() const { return runLen() >= window() && (sma(adjust.type) * adjust.rk) > global.avg(adjust.type); }
	struct {
		double avg(Type t) const { return ratio(sum[t], samples); }
		uint64 sum[2]; /**< Sum of lbds/conflict levels since last call to resetGlobal(). */
		uint64 samples;/**< Samples since last call to resetGlobal().                     */
	} global;
	struct {
		double avgRestart() const { return ratio(samples, restarts); }
		uint32 limit;   /**< Number of conflicts before an update is forced. */
		uint32 restarts;/**< Number of restarts since last update.           */
		uint32 samples; /**< Number of samples since last update.            */
		float  rk;      /**< LBD/CFL dynamic limit factor (typically < 1.0). */
		Type   type;    /**< Dynamic limit based on lbd or confllict level.  */
	} adjust;
private: 
	DynamicLimit(uint32 size);
	DynamicLimit(const DynamicLimit&);
	DynamicLimit& operator=(const DynamicLimit&);
	double sma(Type t)  const { return sum_[t] / double(cap_); }
	uint32 smaU(Type t) const { return static_cast<uint32>(sum_[t] / cap_); }
	uint64 sum_[2];
	uint32 cap_;
	uint32 pos_;
	uint32 num_;
	uint32 buffer_[0];
};

struct BlockLimit {
	explicit BlockLimit(uint32 windowSize, double R = 1.4);
	bool push(uint32 nAssign) {
		ema = n >= span
			? exponentialMovingAverage(ema, nAssign, alpha)
			: cumulativeMovingAverage(ema, nAssign, n);
		return ++n >= next;
	}
	double scaled() const { return ema * r; }
	double ema;   // current exponential moving average
	double alpha; // smoothing factor: 2/(span+1)
	uint64 next;  // enable once n >= next
	uint64 inc;   // block restart for next inc conflict
	uint64 n;     // number of data points seen
	uint32 span;  // minimum observation window
	float  r;     // scale factor for ema
};
///////////////////////////////////////////////////////////////////////////////
// Statistics
///////////////////////////////////////////////////////////////////////////////
inline bool matchStatPath(const char*& x, const char* path, std::size_t len) {
	return std::strncmp(x, path, len) == 0 && (!x[len] || x[len++] == '.') && ((x += len) != 0);
}
inline bool matchStatPath(const char*& x, const char* path) {
	return matchStatPath(x, path, std::strlen(path));
}

#define CLASP_STAT_ACCU(m, k, a, accu) accu;
#define CLASP_STAT_DEFINE(m, k, a, accu) m
#define CLASP_STAT_GET(m, k, a, accu) if (std::strcmp(key, k) == 0) { return double( (a) ); }
#define CLASP_STAT_KEY(m, k, a, accu) k "\0"
#define NO_ARG

//! A struct for holding core statistics used by a solver.
/*!
 * Core statistics are always present in a solver and hence
 * can be used by heuristics.
 */
struct CoreStats {
#define CLASP_CORE_STATS(STAT, SELF, OTHER)     \
	STAT(uint64 choices;    , "choices"           , SELF.choices    , SELF.choices    += OTHER.choices   )\
	STAT(uint64 conflicts;  , "conflicts"         , SELF.conflicts  , SELF.conflicts  += OTHER.conflicts )\
	STAT(uint64 analyzed;   , "conflicts_analyzed", SELF.analyzed   , SELF.analyzed   += OTHER.analyzed  )\
	STAT(uint64 restarts;   , "restarts"          , SELF.restarts   , SELF.restarts   += OTHER.restarts  )\
	STAT(uint64 lastRestart;, "restarts_last"     , SELF.lastRestart, SELF.lastRestart = std::max(SELF.lastRestart, OTHER.lastRestart))

	CoreStats() { reset(); }
	void reset() { std::memset(this, 0, sizeof(CoreStats)); }
	void accu(const CoreStats& o) {
		CLASP_CORE_STATS(CLASP_STAT_ACCU, (*this), o)
	}
	double operator[](const char* key) const {
		CLASP_CORE_STATS(CLASP_STAT_GET, (*this), NO_ARG)
		return -1.0;
	}
	static const char* keys(const char* path) {
		if (!path || !*path) return CLASP_CORE_STATS(CLASP_STAT_KEY,NO_ARG,NO_ARG);
		return 0;
	}
	uint64 backtracks() const { return conflicts-analyzed; }
	uint64 backjumps()  const { return analyzed; }
	double avgRestart() const { return ratio(analyzed, restarts); }
	CLASP_CORE_STATS(CLASP_STAT_DEFINE, NO_ARG, NO_ARG)
};

//! A struct for holding (optional) extra statistics.
struct ExtendedStats {
	typedef ConstraintType type_t;
	typedef uint64 Array[Constraint_t::Type__max];
#define CLASP_EXTENDED_STATS(STAT, SELF, OTHER)     \
	STAT(uint64 domChoices; /**< "domain" choices   */, "domain_choices"        , SELF.domChoices , SELF.domChoices += OTHER.domChoices) \
	STAT(uint64 models;     /**< number of models   */, "models"                , SELF.models     , SELF.models     += OTHER.models)     \
	STAT(uint64 modelLits;  /**< DLs in models      */, "models_level"          , SELF.modelLits  , SELF.modelLits  += OTHER.modelLits)  \
	STAT(uint64 hccTests;   /**< stability tests    */, "hcc_tests"             , SELF.hccTests   , SELF.hccTests   += OTHER.hccTests)   \
	STAT(uint64 hccPartial; /**< partial stab. tests*/, "hcc_partial"           , SELF.hccPartial , SELF.hccPartial += OTHER.hccPartial) \
	STAT(uint64 deleted;    /**< lemmas deleted     */, "lemmas_deleted"        , SELF.deleted    , SELF.deleted    += OTHER.deleted)    \
	STAT(uint64 distributed;/**< lemmas distributed */, "distributed"           , SELF.distributed, SELF.distributed+= OTHER.distributed)\
	STAT(uint64 sumDistLbd; /**< sum of lemma lbds  */, "distributed_sum_lbd"   , SELF.sumDistLbd , SELF.sumDistLbd += OTHER.sumDistLbd) \
	STAT(uint64 integrated; /**< lemmas integrated  */, "integrated"            , SELF.integrated , SELF.integrated += OTHER.integrated) \
	STAT(Array learnts;  /**< lemmas of type t-1    */, "lemmas"                , SELF.lemmas()   , NO_ARG)   \
	STAT(Array lits;     /**< lits of type t-1      */, "lits_learnt"           , SELF.learntLits(), NO_ARG)  \
	STAT(uint32 binary;  /**< binary lemmas         */, "lemmas_binary"         , SELF.binary  , SELF.binary  += OTHER.binary)  \
	STAT(uint32 ternary; /**< ternary lemmas        */, "lemmas_ternary"        , SELF.ternary , SELF.ternary += OTHER.ternary) \
	STAT(double cpuTime; /**< cpu time used         */, "cpu_time"              , SELF.cpuTime , SELF.cpuTime += OTHER.cpuTime) \
	STAT(uint64 intImps; /**< implications on integrating*/, "integrated_imps"  , SELF.intImps , SELF.intImps+= OTHER.intImps)  \
	STAT(uint64 intJumps;/**< backjumps on integrating  */ , "integrated_jumps" , SELF.intJumps, SELF.intJumps+=OTHER.intJumps) \
	STAT(uint64 gpLits;  /**< lits in received gps  */, "guiding_paths_lits"    , SELF.gpLits  , SELF.gpLits  += OTHER.gpLits)  \
	STAT(uint32 gps;     /**< guiding paths received*/, "guiding_paths"         , SELF.gps     , SELF.gps     += OTHER.gps)     \
	STAT(uint32 splits;  /**< split requests handled*/, "splits"                , SELF.splits  , SELF.splits  += OTHER.splits)  \
	STAT(NO_ARG       , "lemmas_conflict", SELF.lemmas(Constraint_t::Conflict), SELF.learnts[0] += OTHER.learnts[0]) \
	STAT(NO_ARG       , "lemmas_loop"    , SELF.lemmas(Constraint_t::Loop)    , SELF.learnts[1] += OTHER.learnts[1]) \
	STAT(NO_ARG       , "lemmas_other"   , SELF.lemmas(Constraint_t::Other)   , SELF.learnts[2] += OTHER.learnts[2]) \
	STAT(NO_ARG       , "lits_conflict"  , SELF.lits[Constraint_t::Conflict-1], SELF.lits[0]    += OTHER.lits[0])    \
	STAT(NO_ARG       , "lits_loop"      , SELF.lits[Constraint_t::Loop-1]    , SELF.lits[1]    += OTHER.lits[1])    \
	STAT(NO_ARG       , "lits_other"     , SELF.lits[Constraint_t::Other-1]   , SELF.lits[2]    += OTHER.lits[2])
	
	ExtendedStats() { reset(); }
	void reset() { std::memset(this, 0, sizeof(ExtendedStats)); }
	void accu(const ExtendedStats& o) {
		CLASP_EXTENDED_STATS(CLASP_STAT_ACCU, (*this), o)
	}
	double operator[](const char* key) const {
		CLASP_EXTENDED_STATS(CLASP_STAT_GET, (*this), NO_ARG)
		return -1.0;
	}
	static const char* keys(const char* path) {
		if (!path || !*path) { return CLASP_EXTENDED_STATS(CLASP_STAT_KEY,NO_ARG,NO_ARG); }
		return 0;
	}
	void addLearnt(uint32 size, type_t t) {
		if (t == Constraint_t::Static) return;
		learnts[t-1]+= 1;
		lits[t-1]   += size;
		binary      += (size == 2);
		ternary     += (size == 3);
	}
	uint64 lemmas()        const { return std::accumulate(learnts, learnts+Constraint_t::Type__max, uint64(0)); }
	uint64 learntLits()    const { return std::accumulate(lits, lits+Constraint_t::Type__max, uint64(0)); }
	uint64 lemmas(type_t t)const { return learnts[t-1]; }
	double avgLen(type_t t)const { return ratio(lits[t-1], lemmas(t)); }
	double avgModel()      const { return ratio(modelLits, models);    }
	double distRatio()     const { return ratio(distributed, learnts[0] + learnts[1]);  }
	double avgDistLbd()    const { return ratio(sumDistLbd, distributed); }
	double avgIntJump()    const { return ratio(intJumps, intImps); }
	double avgGp()         const { return ratio(gpLits, gps); }
	double intRatio()      const { return ratio(integrated, distributed); }
	CLASP_EXTENDED_STATS(CLASP_STAT_DEFINE,NO_ARG,NO_ARG)
};

//! A struct for holding (optional) jump statistics.
struct JumpStats {
#define CLASP_JUMP_STATS(STAT, SELF, OTHER)     \
	STAT(uint64 jumps;    , "jumps"    , SELF.jumps    , SELF.jumps    += OTHER.jumps)   \
	STAT(uint64 bounded;  , "jumps_bounded"  , SELF.bounded  , SELF.bounded  += OTHER.bounded)  \
	STAT(uint64 jumpSum;  , "levels", SELF.jumpSum  , SELF.jumpSum  += OTHER.jumpSum)  \
	STAT(uint64 boundSum; , "levels_bounded" , SELF.boundSum , SELF.boundSum += OTHER.boundSum) \
	STAT(uint32 maxJump;  , "max"  , SELF.maxJump  , MAX_MEM(SELF.maxJump, OTHER.maxJump))   \
	STAT(uint32 maxJumpEx;, "max_executed", SELF.maxJumpEx, MAX_MEM(SELF.maxJumpEx,OTHER.maxJumpEx)) \
	STAT(uint32 maxBound; , "max_bounded", SELF.maxBound , MAX_MEM(SELF.maxBound, OTHER.maxBound)) 

	JumpStats() { reset(); }
	void reset(){ std::memset(this, 0, sizeof(*this)); }
	void accu(const JumpStats& o) {
#define MAX_MEM(X,Y) X = std::max((X), (Y))
		CLASP_JUMP_STATS(CLASP_STAT_ACCU, (*this), o)
#undef MAX_MEM
	}
	double operator[](const char* key) const {
		CLASP_JUMP_STATS(CLASP_STAT_GET, (*this), NO_ARG)
		return -1.0;
	}
	static const char* keys(const char* path) {
		if (!path || !*path) { return CLASP_JUMP_STATS(CLASP_STAT_KEY,NO_ARG,NO_ARG); }
		return 0;
	}
	void update(uint32 dl, uint32 uipLevel, uint32 bLevel) {
		++jumps;
		jumpSum += dl - uipLevel; 
		maxJump = std::max(maxJump, dl - uipLevel);
		if (uipLevel < bLevel) {
			++bounded;
			boundSum += bLevel - uipLevel;
			maxJumpEx = std::max(maxJumpEx, dl - bLevel);
			maxBound  = std::max(maxBound, bLevel - uipLevel);
		}
		else { maxJumpEx = maxJump; }
	}
	uint64 jumped()     const { return jumpSum - boundSum; }
	double jumpedRatio()const { return ratio(jumped(), jumpSum); }
	double avgBound()   const { return ratio(boundSum, bounded); }
	double avgJump()    const { return ratio(jumpSum, jumps); }
	double avgJumpEx()  const { return ratio(jumped(), jumps); }
	CLASP_JUMP_STATS(CLASP_STAT_DEFINE,NO_ARG,NO_ARG)
};
//! A struct for aggregating statistics maintained in a solver object.
struct SolverStats : public CoreStats {
	SolverStats();
	SolverStats(const SolverStats& o);
	~SolverStats();
	bool enableStats(const SolverStats& other);
	int  level() const;
	bool enableExtended();
	bool enableJump();
	void enableLimit(uint32 size);
	void reset();
	void accu(const SolverStats& o);
	void swapStats(SolverStats& o);
	double operator[](const char* key) const;
	const char* subKeys(const char* p) const;
	const char* keys(const char* path) const;
	inline void addLearnt(uint32 size, ConstraintType t);
	inline void addConflict(uint32 dl, uint32 uipLevel, uint32 bLevel, uint32 lbd);
	inline void addDeleted(uint32 num);
	inline void addDistributed(uint32 lbd, ConstraintType t);
	inline void addTest(bool partial);
	inline void addModel(uint32 decisionLevel);
	inline void addCpuTime(double t);
	inline void addSplit(uint32 num = 1);
	inline void addDomChoice(uint32 num = 1);
	inline void addIntegratedAsserting(uint32 receivedDL, uint32 jumpDL);
	inline void addIntegrated(uint32 num = 1);
	inline void removeIntegrated(uint32 num = 1);
	inline void addPath(const LitVec::size_type& sz);
	DynamicLimit*  limit; /**< Optional dynamic limit.       */
	ExtendedStats* extra; /**< Optional extended statistics. */
	JumpStats*     jumps; /**< Optional jump statistics.     */
private: SolverStats& operator=(const SolverStats&);
};
inline void SolverStats::addLearnt(uint32 size, ConstraintType t)  { if (extra) { extra->addLearnt(size, t); } }
inline void SolverStats::addDeleted(uint32 num)                    { if (extra) { extra->deleted += num; }  }
inline void SolverStats::addDistributed(uint32 lbd, ConstraintType){ if (extra) { ++extra->distributed; extra->sumDistLbd += lbd; } }
inline void SolverStats::addIntegrated(uint32 n)                   { if (extra) { extra->integrated += n;} }
inline void SolverStats::removeIntegrated(uint32 n)                { if (extra) { extra->integrated -= n;} }
inline void SolverStats::addCpuTime(double t)                      { if (extra) { extra->cpuTime += t; }    }
inline void SolverStats::addSplit(uint32 num)                      { if (extra) { extra->splits += num; }  }
inline void SolverStats::addPath(const LitVec::size_type& sz)      { if (extra) { ++extra->gps; extra->gpLits += sz; } }
inline void SolverStats::addTest(bool partial)                     { if (extra) { ++extra->hccTests; extra->hccPartial += (uint32)partial; } }
inline void SolverStats::addModel(uint32 DL)                       { if (extra) { ++extra->models; extra->modelLits += DL; } }
inline void SolverStats::addDomChoice(uint32 n)                    { if (extra) { extra->domChoices += n; } }
inline void SolverStats::addIntegratedAsserting(uint32 rDL, uint32 jDL) {
	if (extra) { ++extra->intImps; extra->intJumps += (rDL - jDL); }
}
inline void SolverStats::addConflict(uint32 dl, uint32 uipLevel, uint32 bLevel, uint32 lbd) {
	++analyzed;
	if (limit) { limit->update(dl, lbd); }
	if (jumps) { jumps->update(dl, uipLevel, bLevel); }
}
inline const char* SolverStats::keys(const char* path) const {
	if (!path || !*path) {
		if (jumps && extra) { return CLASP_CORE_STATS(CLASP_STAT_KEY, NO_ARG, NO_ARG) ".jumps\0.extra\0"; }
		if (extra)          { return CLASP_CORE_STATS(CLASP_STAT_KEY, NO_ARG, NO_ARG) ".extra\0"; }
		if (jumps)          { return CLASP_CORE_STATS(CLASP_STAT_KEY, NO_ARG, NO_ARG) ".jumps\0"; }
		return CoreStats::keys(path);
	}
	return subKeys(path);
}
#undef CLASP_STAT_ACCU
#undef CLASP_STAT_DEFINE
#undef CLASP_STAT_GET
#undef CLASP_STAT_KEY
#undef CLASP_CORE_STATS
#undef CLASP_EXTENDED_STATS
#undef CLASP_JUMP_STATS
#undef NO_ARG

///////////////////////////////////////////////////////////////////////////////
// Clauses
///////////////////////////////////////////////////////////////////////////////
//! Primitive representation of a clause.
struct ClauseRep {
	typedef ConstraintInfo Info;
	static ClauseRep create(Literal* cl, uint32 sz, const Info& i = Info())  { return ClauseRep(cl, sz, false, i);}
	static ClauseRep prepared(Literal* cl, uint32 sz, const Info& i = Info()){ return ClauseRep(cl, sz, true, i); }
	ClauseRep(Literal* cl = 0, uint32 sz = 0, bool p = false, const Info& i = Info()) : info(i), size(sz), prep(uint32(p)), lits(cl) {}
	Info     info;    /*!< Additional clause info.    */
	uint32   size:31; /*!< Size of array of literals. */
	uint32   prep: 1; /*!< Whether lits is already prepared. */
	Literal* lits;    /*!< Pointer to array of literals (not owned!). */
	bool isImp() const { return size > 1 && size < 4; }
};

//! (Abstract) base class for clause types.
/*!
 * ClauseHead is used to enforce a common memory-layout for all clauses.
 * It contains the two watched literals and a cache literal to improve
 * propagation performance. A virtual call to Constraint::propagate()
 * is only needed if the other watch is not true and the cache literal
 * is false.
 */
class ClauseHead : public Constraint {
public:
	enum { HEAD_LITS = 3, MAX_SHORT_LEN = 5 };
	explicit ClauseHead(const InfoType& init);
	// base interface
	//! Propagates the head and calls updateWatch() if necessary.
	PropResult propagate(Solver& s, Literal, uint32& data);
	//! Type of clause.
	Type       type()     const { return info_.type(); }
	//! Returns the activity of this clause.
	ScoreType  activity() const { return info_.score(); }
	//! True if this clause currently is the antecedent of an assignment.
	bool       locked(const Solver& s) const;
	//! Halves the activity of this clause.
	void       decreaseActivity() { info_.score().reduce(); }
	void       resetActivity()    { info_.score().reset(); }
	//! Downcast from LearntConstraint.
	ClauseHead* clause() { return this; }
	
	// clause interface
	typedef std::pair<bool, bool> BoolPair;
	//! Adds watches for first two literals in head to solver.
	void attach(Solver& s);
	void resetScore(ScoreType sc);
	//! Returns true if head is satisfied w.r.t current assignment in s.
	bool satisfied(const Solver& s) const;
	//! Conditional clause?
	bool tagged() const { return info_.tagged(); }
	//! Contains aux vars?
	bool aux()    const { return info_.aux(); }
	bool learnt() const { return info_.learnt(); }
	uint32 lbd()  const { return info_.lbd(); }
	//! Removes watches from s.
	virtual void     detach(Solver& s);
	//! Returns the size of this clause.
	virtual uint32   size()              const = 0;
	//! Returns the literals of this clause in out.
	virtual void     toLits(LitVec& out) const = 0;
	//! Returns true if this clause is a valid "reverse antecedent" for p.
	virtual bool     isReverseReason(const Solver& s, Literal p, uint32 maxL, uint32 maxN) = 0;
	//! Removes p from clause if possible.
	/*!
	 * \return
	 *   The first component of the returned pair specifies whether or not
	 *   p was removed from the clause.
	 *   The second component of the returned pair specifies whether
	 *   the clause should be kept (false) or removed (true). 
	 */
	virtual BoolPair strengthen(Solver& s, Literal p, bool allowToShort = true) = 0;
protected:
	struct Local {
		void   init(uint32 sz);
		bool   isSmall()     const  { return (mem[0] & 1u) == 0u; }
		bool   contracted()  const  { return (mem[0] & 3u) == 3u; }
		bool   strengthened()const  { return (mem[0] & 5u) == 5u; }
		uint32 size()        const  { return mem[0] >> 3; }
		void   setSize(uint32 size) { mem[0] = (size << 3) | (mem[0] & 7u); }
		void   markContracted()     { mem[0] |= 2u; }
		void   markStrengthened()   { mem[0] |= 4u; }
		void   clearContracted()    { mem[0] &= ~2u; }
		void   clearIdx()           { mem[1] = 0; }
		uint32 mem[2];
	};
	bool toImplication(Solver& s);
	void clearTagged()    { info_.setTagged(false); }
	void setLbd(uint32 x) { info_.setLbd(x); }
	//! Shall replace the watched literal at position pos with a non-false literal.
	/*!
	 * \pre pos in [0,1] 
	 * \pre s.isFalse(head_[pos]) && s.isFalse(head_[2])
	 * \pre head_[pos^1] is the other watched literal
	 */
	virtual bool updateWatch(Solver& s, uint32 pos) = 0;
	union {
		Local           local_;
		SharedLiterals* shared_;
	};
	InfoType info_;
	Literal  head_[HEAD_LITS]; // two watched literals and one cache literal
};
//! Allocator for small (at most 32-byte) clauses.
class SmallClauseAlloc {
public:
	SmallClauseAlloc();
	~SmallClauseAlloc();
	void* allocate() {
		if(freeList_ == 0) {
			allocBlock();
		}
		Chunk* r   = freeList_;
		freeList_  = r->next;
		return r;
	}
	void   free(void* mem) {
		Chunk* b = reinterpret_cast<Chunk*>(mem);
		b->next  = freeList_;
		freeList_= b;
	}
private:
	SmallClauseAlloc(const SmallClauseAlloc&);
	SmallClauseAlloc& operator=(const SmallClauseAlloc&);
	struct Chunk {
		Chunk*        next; // enforce ptr alignment
		unsigned char mem[32 - sizeof(Chunk*)];
	};
	struct Block {
		enum { num_chunks = 1023 };
		Block* next;
		unsigned char pad[32-sizeof(Block*)];
		Chunk  chunk[num_chunks];
	};
	void allocBlock();
	Block*  blocks_;
	Chunk*  freeList_;
};
///////////////////////////////////////////////////////////////////////////////
// Watches
///////////////////////////////////////////////////////////////////////////////
//! Represents a clause watch in a Solver.
struct ClauseWatch {
	//! Clause watch: clause head
	explicit ClauseWatch(ClauseHead* a_head) : head(a_head) { }
	ClauseHead* head;
	struct EqHead {
		explicit EqHead(ClauseHead* h) : head(h) {}
		bool operator()(const ClauseWatch& w) const { return head == w.head; }
		ClauseHead* head;
	};
};

//! Represents a generic watch in a Solver.
struct GenericWatch {
	//! A constraint and some associated data.
	explicit GenericWatch(Constraint* a_con, uint32 a_data = 0) : con(a_con), data(a_data) {}
	//! Calls propagate on the stored constraint and passes the stored data to that constraint.
	Constraint::PropResult propagate(Solver& s, Literal p) { return con->propagate(s, p, data); }
	
	Constraint* con;    /**< The constraint watching a certain literal. */
	uint32      data;   /**< Additional data associated with this watch - passed to constraint on update. */

	struct EqConstraint {
		explicit EqConstraint(Constraint* c) : con(c) {}
		bool operator()(const GenericWatch& w) const { return con == w.con; }
		Constraint* con;
	};	
};

//! Watch list type.
typedef bk_lib::left_right_sequence<ClauseWatch, GenericWatch, 0> WatchList;
inline void releaseVec(WatchList& w) { w.clear(true); }

///////////////////////////////////////////////////////////////////////////////
// Assignment
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//! Type for storing reasons for variable assignments together with additional data.
/*!
 * \note On 32-bit systems additional data is stored in the high-word of antecedents.
 */
struct ReasonStore32 : PodVector<Antecedent>::type {
	uint32 data(uint32 v) const { return decode((*this)[v]);}
	void   setData(uint32 v, uint32 data) { encode((*this)[v], data); }
	static void encode(Antecedent& a, uint32 data) {
		a.asUint() = (uint64(data)<<32) | static_cast<uint32>(a.asUint());
	}
	static uint32 decode(const Antecedent& a) {
		return static_cast<uint32>(a.asUint()>>32);
	}
	struct value_type {
		value_type(const Antecedent& a, uint32 d) : ante_(a) {
			if (d != UINT32_MAX) { encode(ante_, d); assert(data() == d && ante_.type() == Antecedent::Generic); }
		}
		const Antecedent& ante() const { return ante_;      }
		      uint32      data() const { return ante_.type() == Antecedent::Generic ? decode(ante_) : UINT32_MAX; }
		Antecedent ante_;
	};
};

//! Type for storing reasons for variable assignments together with additional data.
/*
 * \note On 64-bit systems additional data is stored in a separate container.
 */
struct ReasonStore64 : PodVector<Antecedent>::type {
	uint32  dataSize() const               { return (uint32)data_.size(); }
	void    dataResize(uint32 nv)          { if (nv > dataSize()) data_.resize(nv, UINT32_MAX); }
	uint32  data(uint32 v) const           { return v < dataSize() ? data_[v] : UINT32_MAX; }
	void    setData(uint32 v, uint32 data) { dataResize(v+1); data_[v] = data; }
	VarVec  data_;
	struct  value_type : std::pair<Antecedent, uint32> {
		value_type(const Antecedent& a, uint32 d) : std::pair<Antecedent, uint32>(a, d) {}
		const Antecedent& ante() const { return first;  }
		      uint32      data() const { return second; }
	};
};

//! A set of configurable values for a variable.
/*!
 * Beside its currently assigned value, a variable
 * can also have a user, saved, preferred, and default value.
 * These values are used in sign selection to determine the signed literal 
 * of a variable to be assign first.
 * During sign selection, the values form a hierarchy:
 * user > saved > preferred > current sign score of heuristic > default value
 */
struct ValueSet {
	ValueSet() : rep(0) {}
	enum Value { user_value = 0x03u, saved_value = 0x0Cu, pref_value = 0x30u, def_value = 0xC0u };
	bool     sign()       const { return (right_most_bit(rep) & 0xAAu) != 0; }
	bool     empty()      const { return rep == 0; }
	bool     has(Value v) const { return (rep & v) != 0; }
	bool     has(uint32 f)const { return (rep & f) != 0; }
	ValueRep get(Value v) const { return static_cast<ValueRep>((rep & v) / right_most_bit(static_cast<uint8>(v))); }
	void     set(Value which, ValueRep to) { rep &= ~which; rep |= (to * right_most_bit(static_cast<uint8>(which))); }
	void     save(ValueRep x)   { rep &= ~saved_value; rep |= (x << 2); }
	uint8 rep;
};

//! Stores assignment related information.
/*!
 * For each variable v, the class stores 
 *  - v's current value (value_free if unassigned)
 *  - the decision level on which v was assign (only valid if value(v) != value_free)
 *  - the reason why v is in the assignment (only valid if value(v) != value_free)
 *  - (optionally) some additional data associated with the reason
 *  .
 * Furthermore, the class stores the sequences of assignments as a set of
 * true literals in its trail-member.
 */
class Assignment  {
public:
	typedef PodVector<uint32>::type     AssignVec;
	typedef PodVector<ValueSet>::type   PrefVec;
	typedef bk_lib::detail::if_then_else<
		sizeof(Constraint*)==sizeof(uint64)
		, ReasonStore64
		, ReasonStore32>::type            ReasonVec;
	typedef ReasonVec::value_type       ReasonWithData;
	Assignment() : front(0), elims_(0), units_(0) { }
	LitVec            trail;   // assignment sequence
	LitVec::size_type front;   // and "propagation queue"
	bool              qEmpty() const { return front == trail.size(); }
	uint32            qSize()  const { return (uint32)trail.size() - front; }
	Literal           qPop()         { return trail[front++]; }
	void              qReset()       { front  = trail.size(); }

	//! Number of variables in the three-valued assignment.
	uint32            numVars()    const { return (uint32)assign_.size(); }
	//! Number of assigned variables.
	uint32            assigned()   const { return (uint32)trail.size();   }
	//! Number of free variables.
	uint32            free()       const { return numVars() - (assigned()+elims_);   }
	//! Returns the largest possible decision level.
	uint32            maxLevel()   const { return (1u<<28)-2; }
	//! Returns v's value in the three-valued assignment.
	ValueRep          value(Var v) const { return ValueRep(assign_[v] & 3u); }
	//! Returns the decision level on which v was assigned if value(v) != value_free.
	uint32            level(Var v) const { return assign_[v] >> 4u; }
	//! Returns true if v was not eliminated from the assignment.
	bool              valid(Var v) const { return (assign_[v] & elim_mask) != elim_mask; }
	//! Returns the set of preferred values of v.
	const ValueSet    pref(Var v)  const { return v < pref_.size() ? pref_[v] : ValueSet(); }
	//! Returns the reason for v being assigned if value(v) != value_free.
	const Antecedent& reason(Var v)const { return reason_[v]; }
	//! Returns the reason data associated with v.
	uint32            data(Var v)  const { return reason_.data(v); }

	//! Resize to nv variables.
	void resize(uint32 nv) {
		assign_.resize(nv);
		reason_.resize(nv);
	}
	//! Adds a var to assignment - initially the new var is unassigned.
	Var addVar() {
		assign_.push_back(0);
		reason_.push_back(0);
		return numVars()-1;
	}
	//! Allocates space for storing preferred values for all variables.
	void requestPrefs() {
		if (pref_.size() != assign_.size()) { pref_.resize(assign_.size()); }
	}
	//! Eliminates v from the assignment.
	void eliminate(Var v) {
		assert(value(v) == value_free && "Can not eliminate assigned var!\n");
		if (valid(v)) { assign_[v] = elim_mask|value_true; ++elims_; }
	}
	//! Assigns p.var() on level lev to the value that makes p true and stores x as reason for the assignment.
	/*!
	 * \return true if the assignment is consistent. False, otherwise.
	 * \post If true is returned, p is in trail. Otherwise, ~p is.
	 */
	bool assign(Literal p, uint32 lev, const Antecedent& x) {
		const Var      v   = p.var();
		const ValueRep val = value(v);
		if (val == value_free) {
			assert(valid(v));
			assign_[v] = (lev<<4) + trueValue(p);
			reason_[v] = x;
			trail.push_back(p);
			return true;
		}
		return val == trueValue(p);
	}
	bool assign(Literal p, uint32 lev, Constraint* c, uint32 data) {
		const Var      v   = p.var();
		const ValueRep val = value(v);
		if (val == value_free) {
			assert(valid(v));
			assign_[v] = (lev<<4) + trueValue(p);
			reason_[v] = c;
			reason_.setData(v, data);
			trail.push_back(p);
			return true;
		}
		return val == trueValue(p);
	}
	//! Undos all assignments in the range trail[first, last).
	/*!
	 * \param first First assignment to be undone.
	 * \param save  If true, previous assignment of a var is saved before it is undone.
	 */
	void undoTrail(LitVec::size_type first, bool save) {
		if (!save) { popUntil<&Assignment::clear>(trail[first]); }
		else       { requestPrefs(); popUntil<&Assignment::saveAndClear>(trail[first]); }
		qReset();
	}
	//! Undos the last assignment.
	void undoLast() { clear(trail.back().var()); trail.pop_back(); }
	//! Returns the last assignment as a true literal.
	Literal last() const { return trail.back(); }
	Literal&last()       { return trail.back(); }
	/*!
	 * \name Implementation functions
	 * Low-level implementation functions. Use with care and only if you
	 * know what you are doing!
	 */
	//@{
	uint32 units()              const { return units_; }
	bool   seen(Var v, uint8 m) const { return (assign_[v] & (m<<2)) != 0; }
	void   setSeen(Var v, uint8 m)    { assign_[v] |= (m<<2); }
	void   clearSeen(Var v)           { assign_[v] &= ~uint32(12); }
	void   clearValue(Var v)          { assign_[v] &= ~uint32(3); }
	void   setValue(Var v, ValueRep val) {
		assert(value(v) == val || value(v) == value_free);
		assign_[v] |= val;
	}
	void   setReason(Var v, const Antecedent& a) { reason_[v] = a;  }
	void   setData(Var v, uint32 data) { reason_.setData(v, data); }
	void   setPref(Var v, ValueSet::Value which, ValueRep to) { pref_[v].set(which, to); }
	void   copyAssignment(Assignment& o) const { o.assign_ = assign_; }
	bool   markUnits()                 { while (units_ != front) { setSeen(trail[units_++].var(), 3u); }  return true; }
	void   setUnits(uint32 ts)         { units_ = ts; }
	void   resetPrefs()                { pref_.assign(pref_.size(), ValueSet()); }
	//@}
private:
	static const uint32 elim_mask = uint32(0xFFFFFFF0u);
	Assignment(const Assignment&);
	Assignment& operator=(const Assignment&);
	void    clear(Var v)              { assign_[v]= 0; }
	void    saveAndClear(Var v)       { pref_[v].save(value(v)); clear(v); }
	template <void (Assignment::*op)(Var v)>
	void popUntil(Literal stop) {
		Literal p;
		do {
			p = trail.back(); trail.pop_back();
			(this->*op)(p.var());
		} while (p != stop);
	}
	AssignVec assign_; // for each var: three-valued assignment
	ReasonVec reason_; // for each var: reason for being assigned (+ optional data)
	PrefVec   pref_;   // for each var: set of preferred values
	uint32    elims_;  // number of variables that were eliminated from the assignment
	uint32    units_;  // number of marked top-level assignments
};

//! Stores information about a literal that is implied on an earlier level than the current decision level.
struct ImpliedLiteral {
	typedef Assignment::ReasonWithData AnteInfo;
	ImpliedLiteral(Literal a_lit, uint32 a_level, const Antecedent& a_ante, uint32 a_data = UINT32_MAX) 
		: lit(a_lit)
		, level(a_level)
		, ante(a_ante, a_data) {
	}
	Literal  lit;   /**< The implied literal */
	uint32   level; /**< The earliest decision level on which lit is implied */
	AnteInfo ante;  /**< The reason why lit is implied on decision-level level */
};
//! A type for storing ImpliedLiteral objects.
struct ImpliedList {
	typedef PodVector<ImpliedLiteral>::type VecType;
	typedef VecType::const_iterator iterator;
	ImpliedList() : level(0), front(0) {}
	//! Searches for an entry <p> in list. Returns 0 if none is found.
	ImpliedLiteral* find(Literal p) {
		for (uint32 i = 0, end = lits.size(); i != end; ++i) {
			if (lits[i].lit == p)  { return &lits[i]; }
		}
		return 0;
	}
	//! Adds a new object to the list.
	void add(uint32 dl, const ImpliedLiteral& n) {
		if (dl > level) { level = dl; }
		lits.push_back(n);
	}
	//! Returns true if list contains entries that must be reassigned on current dl.
	bool active(uint32 dl) const { return dl < level && front != lits.size(); }
	//! Reassigns all literals that are still implied.
	bool assign(Solver& s);
	iterator begin() const { return lits.begin(); }
	iterator end()   const { return lits.end();   }
	VecType lits;  // current set of (out-of-order) implied literals
	uint32  level; // highest dl on which lits must be reassigned
	uint32  front; // current starting position in lits
};
//@}
}
#endif
