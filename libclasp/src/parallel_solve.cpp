// 
// Copyright (c) 2010-2012, Benjamin Kaufmann
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
#if WITH_THREADS
#include <clasp/parallel_solve.h>
#include <clasp/solver.h>
#include <clasp/clause.h>
#include <clasp/enumerator.h>
#include <clasp/util/timer.h>
#include <clasp/minimize_constraint.h>
#include <clasp/util/mutex.h>
#include <tbb/concurrent_queue.h>
namespace Clasp { namespace mt {
/////////////////////////////////////////////////////////////////////////////////////////
// BarrierSemaphore
/////////////////////////////////////////////////////////////////////////////////////////
// A combination of a barrier and a semaphore
class BarrierSemaphore {
public:
	explicit BarrierSemaphore(int counter = 0, int maxParties = 1) : counter_(counter), active_(maxParties) {}
	// Initializes this object
	// PRE: no thread is blocked on the semaphore
	//      (i.e. internal counter is >= 0)
	// NOTE: not thread-safe
	void unsafe_init(int counter = 0, int maxParties = 1) {
		counter_ = counter;
		active_  = maxParties;
	}
	// Returns the current semaphore counter.
	int  counter()   { lock_guard<mutex> lock(semMutex_); return counter_; }
	// Returns the number of parties required to trip this barrier.
	int  parties()   { lock_guard<mutex> lock(semMutex_); return active_;  } 
	// Returns true if all parties are waiting at the barrier
	bool active()    { lock_guard<mutex> lock(semMutex_); return unsafe_active(); }
	
	// barrier interface
	
	// Increases the barrier count, i.e. the number of 
	// parties required to trip this barrier.
	void addParty() {
		lock_guard<mutex> lock(semMutex_);
		++active_;
	}
	// Decreases the barrier count and resets the barrier
	// if reset is true. 
	// PRE: the thread does not itself wait on the barrier
	void removeParty(bool reset) {
		unique_lock<mutex> lock(semMutex_);
		assert(active_ > 0);
		--active_;
		if      (reset)           { unsafe_reset(0); }
		else if (unsafe_active()) { counter_ = -active_; lock.unlock(); semCond_.notify_one(); }
	}
	// Waits until all parties have arrived, i.e. called wait.
	// Exactly one of the parties will receive a return value of true,
	// the others will receive a value of false.
	// Applications shall use this value to designate one thread as the
	// leader that will eventually reset the barrier thereby unblocking the other threads.
	bool wait() {
		unique_lock<mutex> lock(semMutex_);
		if (--counter_ >= 0) { counter_ = -1; }
		return unsafe_wait(lock);
	}
	// Resets the barrier and unblocks any waiting threads.
	void reset(uint32 semCount = 0) {
		lock_guard<mutex> lock(semMutex_);
		unsafe_reset(semCount);
	}
	// semaphore interface
	
	// Decrement the semaphore's counter.
	// If the counter is zero or less prior to the call
	// the calling thread is suspended.
	// Returns false to signal that all but the calling thread
	// are currently blocked.
	bool down() {
		unique_lock<mutex> lock(semMutex_);
		if (--counter_ >= 0) { return true; }
		return !unsafe_wait(lock);
	}
	// Increments the semaphore's counter and resumes 
	// one thread which has called down() if the counter 
	// was less than zero prior to the call.
	void up() {
		bool notify;
		{
			lock_guard<mutex> lock(semMutex_);
			notify    = (++counter_ < 1);
		}
		if (notify) semCond_.notify_one();
	}
private:
	BarrierSemaphore(const BarrierSemaphore&);
	BarrierSemaphore& operator=(const BarrierSemaphore&);
	typedef condition_variable cv;
	bool    unsafe_active() const { return -counter_ >= active_; }
	void    unsafe_reset(uint32 semCount) {
		int prev = counter_;
		counter_ = semCount;
		if (prev < 0) { semCond_.notify_all(); }
	}
	// Returns true for the leader, else false
	bool    unsafe_wait(unique_lock<mutex>& lock) {
		assert(counter_ < 0);
		// don't put the last thread to sleep!
		if (!unsafe_active()) {
			semCond_.wait(lock);
		}
		return unsafe_active();
	}	
	cv    semCond_;  // waiting threads
	mutex semMutex_; // mutex for updating counter
	int   counter_;  // semaphore's counter
	int   active_;   // number of active threads
};
/////////////////////////////////////////////////////////////////////////////////////////
// ParallelSolve::Impl
/////////////////////////////////////////////////////////////////////////////////////////
struct ParallelSolve::SharedData {
	typedef tbb::concurrent_queue<const LitVec*> queue;
	enum MsgFlag {
		terminate_flag        = 1u, sync_flag  = 2u,  split_flag    = 4u, 
		restart_flag          = 8u, complete_flag = 16u,
		interrupt_flag        = 32u, // set on terminate from outside
		allow_split_flag      = 64u, // set if splitting mode is active
		forbid_restart_flag   = 128u,// set if restarts are no longer allowed
		cancel_restart_flag   = 256u,// set if current restart request was cancelled by some thread
		restart_abandoned_flag= 512u,// set to signal that threads must not give up their gp
	};
	enum Message {
		msg_terminate      = (terminate_flag),
		msg_interrupt      = (terminate_flag | interrupt_flag),
		msg_sync_restart   = (sync_flag | restart_flag),
		msg_split          = split_flag
	};
	SharedData() : path(0) { reset(0); control = 0; }
	void reset(SharedContext* a_ctx) {
		clearQueue();
		syncT.reset();
		workSem.unsafe_init(0, a_ctx ? a_ctx->concurrency() : 0);
		globalR.reset();
		maxConflict = globalR.current();
		error       = 0;
		initMask    = 0;
		ctx         = a_ctx;
		path        = 0;
		nextId      = 1;
		workReq     = 0;
		restartReq  = 0;
	}
	void clearQueue() {
		for (const LitVec* a = 0; workQ.try_pop(a); ) {
			if (a != path) { delete a; }
		}
	}
	const LitVec* requestWork(uint32 id) {
		// try to get initial path
		uint64 m(uint64(1) << id);
		uint64 init = initMask;
		if ((m & init) == m) { initMask -= m; return path; }
		// try to get path from split queue
		const LitVec* res = 0;
		if (workQ.try_pop(res)) { return res; }
		return 0;
	}
	// MESSAGES
	bool        hasMessage()  const { return (control & uint32(7)) != 0; }
	bool        synchronize() const { return (control & uint32(sync_flag))      != 0; }
	bool        terminate()   const { return (control & uint32(terminate_flag)) != 0; }
	bool        split()       const { return (control & uint32(split_flag))     != 0; }
	bool        postMessage(Message m, bool notify);
	void        aboutToSplit()      { if (--workReq == 0) updateSplitFlag();  }
	void        updateSplitFlag();
	// CONTROL FLAGS
	bool        hasControl(uint32 f) const { return (control & f) != 0;        }
	bool        interrupt()          const { return hasControl(interrupt_flag);}
	bool        complete()           const { return hasControl(complete_flag); }
	bool        restart()            const { return hasControl(restart_flag);  }
	bool        allowSplit()         const { return hasControl(allow_split_flag); }
	bool        allowRestart()       const { return !hasControl(forbid_restart_flag); }
	bool        setControl(uint32 flags)   { return (fetch_and_or(control, flags) & flags) != flags; }
	bool        clearControl(uint32 flags) { return (fetch_and_and(control, ~flags) & flags) == flags; }
	ScheduleStrategy globalR;     // global restart strategy
	uint64           maxConflict; // current restart limit
	uint64           error;       // bitmask of erroneous solvers
	SharedContext*   ctx;         // shared context object
	const LitVec*    path;        // initial guiding path - typically empty
	atomic<uint64>   initMask;    // bitmask for initialization
	Timer<RealTime>  syncT;       // thread sync time
	mutex            modelM;      // model-mutex 
	BarrierSemaphore workSem;     // work-semaphore
	queue            workQ;       // work-queue
	uint32           nextId;      // next solver id to use
	atomic<int>      workReq;     // > 0: someone needs work
	atomic<uint32>   restartReq;  // == numThreads(): restart
	atomic<uint32>   control;     // set of active message flags
	atomic<uint32>   modCount;    // coounter for synchronizing models
};

// post message to all threads
bool ParallelSolve::SharedData::postMessage(Message m, bool notifyWaiting) {
	if (m == msg_split) {
		if (++workReq == 1) { updateSplitFlag(); }
		return true;
	}
	else if (setControl(m)) {
		// control message - notify all if requested
		if (notifyWaiting) workSem.reset();
		if ((uint32(m) & uint32(sync_flag|terminate_flag)) != 0) {
			syncT.reset();
			syncT.start();
		}
		return true;
	}
	return false;
}

void ParallelSolve::SharedData::updateSplitFlag() {
	for (bool splitF;;) {
		splitF = (workReq > 0);	
		if (split() == splitF) return;
		if (splitF) fetch_and_or(control,   uint32(split_flag)); 
		else        fetch_and_and(control, ~uint32(split_flag));
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// ParallelSolve
/////////////////////////////////////////////////////////////////////////////////////////
ParallelSolve::ParallelSolve(Enumerator* e, const ParallelSolveOptions& opts)
	: SolveAlgorithm(e, opts.limit)
	, shared_(new SharedData)
	, thread_(0)
	, distribution_(opts.distribute)
	, maxRestarts_(0)
	, intGrace_(1024)
	, intTopo_(opts.integrate.topo)
	, intFlags_(ClauseCreator::clause_not_root_sat | ClauseCreator::clause_no_add)
	, modeSplit_(opts.algorithm.mode == ParallelSolveOptions::Algorithm::mode_split) {
	setRestarts(opts.restarts.maxR, opts.restarts.sched);
	setIntegrate(opts.integrate.grace, opts.integrate.filter);
}

ParallelSolve::~ParallelSolve() {
	if (shared_->nextId > 1) {
		// algorithm was not started but there may be active threads -
		// force orderly shutdown
		ParallelSolve::doInterrupt();
		shared_->workSem.removeParty(true);
		joinThreads();
	}
	destroyThread(masterId);
	delete shared_;
}

bool ParallelSolve::beginSolve(SharedContext& ctx) {
	assert(ctx.concurrency() && "Illegal number of threads");
	if (shared_->terminate()) { return false; }
	shared_->reset(&ctx);
	if (!enumerator().supportsParallel() && numThreads() > 1) {
		ctx.report(warning(Event::subsystem_solve, "Selected reasoning mode implies #Threads=1."));
		shared_->workSem.unsafe_init(1);
		modeSplit_ = false;
		ctx.setConcurrency(1, SharedContext::mode_reserve);
	}
	shared_->setControl(modeSplit_ ? SharedData::allow_split_flag : SharedData::forbid_restart_flag);
	shared_->modCount = uint32(enumerator().optimize());
	if (distribution_.types != 0 && ctx.distributor.get() == 0 && numThreads() > 1) {
		if (distribution_.mode == ParallelSolveOptions::Distribution::mode_local) {
			ctx.distributor.reset(new mt::LocalDistribution(distribution_, ctx.concurrency(), intTopo_));
		}
		else {
			ctx.distributor.reset(new mt::GlobalDistribution(distribution_, ctx.concurrency(), intTopo_));
		}
	}
	shared_->setControl(SharedData::sync_flag); // force initial sync with all threads
	shared_->syncT.start();
	reportProgress(MessageEvent(*ctx.master(), "SYNC", MessageEvent::sent));
	for (uint32 i = 1; i != ctx.concurrency(); ++i) {
		uint32 id = shared_->nextId++;
		allocThread(id, *ctx.solver(id), ctx.configuration()->search(id));
		// start in new thread
		Clasp::thread x(std::mem_fun(&ParallelSolve::solveParallel), this, id);
		thread_[id]->setThread(x);
	}
	return true;
}

void ParallelSolve::setIntegrate(uint32 grace, uint8 filter) {
	typedef ParallelSolveOptions::Integration Dist;
	intGrace_     = grace;
	intFlags_     = ClauseCreator::clause_no_add;
	if (filter == Dist::filter_heuristic) { store_set_bit(intFlags_, 31); }
	if (filter != Dist::filter_no)        { intFlags_ |= ClauseCreator::clause_not_root_sat; }
	if (filter == Dist::filter_sat)       { intFlags_ |= ClauseCreator::clause_not_sat; }
}

void ParallelSolve::setRestarts(uint32 maxR, const ScheduleStrategy& rs) {
	maxRestarts_         = maxR;
	shared_->globalR     = maxR ? rs : ScheduleStrategy::none();
	shared_->maxConflict = shared_->globalR.current();
}

uint32 ParallelSolve::numThreads() const { return shared_->workSem.parties(); }

void ParallelSolve::allocThread(uint32 id, Solver& s, const SolveParams& p) {
	if (!thread_) {
		uint32 n = numThreads();
		thread_  = new ParallelHandler*[n];
		std::fill(thread_, thread_+n, static_cast<ParallelHandler*>(0));
	}
	size_t sz   = ((sizeof(ParallelHandler)+63) / 64) * 64;
	thread_[id] = new (alignedAlloc(sz, 64)) ParallelHandler(*this, s, p);
}

void ParallelSolve::destroyThread(uint32 id) {
	if (thread_ && thread_[id]) {
		assert(!thread_[id]->joinable() && "Shutdown not completed!");
		thread_[id]->~ParallelHandler();
		alignedFree(thread_[id]);
		thread_[id] = 0;
		if (id == masterId) {
			delete [] thread_;
			thread_ = 0;
		}
	}
}

inline void ParallelSolve::reportProgress(const Event& ev) const {
	return shared_->ctx->report(ev);
}

// joins with and destroys all active threads
void ParallelSolve::joinThreads() {
	int    ec     = thread_[masterId]->error();
	uint32 winner = thread_[masterId]->winner() ? uint32(masterId) : UINT32_MAX;
	shared_->error= ec ? 1 : 0;
	for (uint32 i = 1, end = shared_->nextId; i != end; ++i) {
		if (thread_[i]->join() != 0) {
			shared_->error |= uint64(1) << i;
			ec = std::max(ec, thread_[i]->error());
		}
		if (thread_[i]->winner() && i < winner) {
			winner = i;
		}
		destroyThread(i);
	}
	// detach master only after all client threads are done
	thread_[masterId]->detach(*shared_->ctx, shared_->interrupt());
	thread_[masterId]->setError(!shared_->interrupt() ? thread_[masterId]->error() : ec);
	shared_->ctx->setWinner(winner);
	shared_->nextId = 1;
	shared_->syncT.stop();
	reportProgress(MessageEvent(*shared_->ctx->master(), "TERMINATE", MessageEvent::completed, shared_->syncT.total()));
}

// Entry point for master solver
bool ParallelSolve::doSolve(SharedContext& ctx, const LitVec& path) {
	if (beginSolve(ctx)) {
		Solver& s = *ctx.master();
		assert(s.id() == masterId);
		allocThread(masterId, s, ctx.configuration()->search(masterId));
		shared_->path = &path;
		solveParallel(masterId);
		reportProgress(message(Event::subsystem_solve, "Joining with other threads", &s));
		joinThreads();
		int err = thread_[masterId]->error();
		destroyThread(masterId);
		shared_->ctx->distributor.reset(0);
		switch(err) {
			case error_none   : break;
			case error_oom    : throw std::bad_alloc();
			case error_runtime: throw std::runtime_error("RUNTIME ERROR!");
			default:            throw std::runtime_error("UNKNOWN ERROR!");
		}
	}
	return !shared_->complete();
}

// main solve loop executed by all threads
void ParallelSolve::solveParallel(uint32 id) {
	Solver& s           = thread_[id]->solver();
	const SolveParams& p= thread_[id]->params();
	SolveLimits lim     = limits();
	SolverStats agg;
	PathPtr a(0);
	try {
		// establish solver<->handler connection and attach to shared context
		// should this fail because of an initial conflict, we'll terminate
		// in requestWork.
		thread_[id]->attach(*shared_->ctx);
		BasicSolve solve(s, p, &lim);
		agg.enableStats(s.stats);
		for (GpType t; requestWork(s, a); solve.reset()) {
			agg.accu(s.stats);
			s.stats.reset();
			thread_[id]->setGpType(t = ((a.is_owner() || modeSplit_) ? gp_split : gp_fixed));
			if (enumerator().start(s, *a, a.is_owner()) && thread_[id]->solveGP(solve, t, shared_->maxConflict) == value_free) {
				terminate(s, false);
			}
			s.clearStopConflict();
			enumerator().end(s);
		}
	}
	catch (const std::bad_alloc&)  { exception(id,a,error_oom, "ERROR: std::bad_alloc" ); }
	catch (const std::exception& e){ exception(id,a,error_runtime, e.what()); }
	catch (...)                    { exception(id,a,error_other, "ERROR: unknown");  }
	assert(shared_->terminate() || thread_[id]->error() != error_none);
	// this thread is leaving
	shared_->workSem.removeParty(shared_->terminate());
	// update stats
	s.stats.accu(agg);
	if (id != masterId) {
		// remove solver<->handler connection and detach from shared context
		// note: since detach can change the problem, we must not yet detach the master
		// because some client might still be in the middle of an attach operation
		thread_[id]->detach(*shared_->ctx, shared_->interrupt());
		s.stats.addCpuTime(ThreadTime::getTime());
	}
}

void ParallelSolve::exception(uint32 id, PathPtr& path, ErrorCode e, const char* what) {
	try {
		reportProgress(message(Event::subsystem_solve, what, &thread_[id]->solver()));
		thread_[id]->setError(e);
		if (id == masterId || shared_->workSem.active()) { 
			ParallelSolve::doInterrupt();
			return;
		}
		else if (path.get() && shared_->allowSplit()) {
			shared_->workQ.push(path.release());
			shared_->workSem.up();
		}
		reportProgress(warning(Event::subsystem_solve, "Thread failed and was removed.", &thread_[id]->solver()));
	}
	catch(...) { ParallelSolve::doInterrupt(); }
}

// forced termination from outside
bool ParallelSolve::doInterrupt() {
	// do not notify blocked threads to avoid possible
	// deadlock in semaphore!
	shared_->postMessage(SharedData::msg_interrupt, false);
	return true;
}

// tries to get new work for the given solver
bool ParallelSolve::requestWork(Solver& s, PathPtr& out) { 
	const LitVec* a = 0;
	for (int popped = 0; !shared_->terminate();) {
		// only clear path and stop conflict - we don't propagate() here
		// because we would then have to handle any eventual conflicts
		if (++popped == 1 && !s.popRootLevel(s.rootLevel())) {
			// s has a real top-level conflict - problem is unsat
			terminate(s, true);
		}
		else if (shared_->synchronize()) {
			// a synchronize request is active - we are fine with
			// this but did not yet had a chance to react on it
			waitOnSync(s);
		}
		else if (a || (a = shared_->requestWork(s.id())) != 0) {
			assert(s.decisionLevel() == 0);
			// got new work from work-queue
			out = a;
			// do not take over ownership of initial gp!
			if (a == shared_->path) { out.release(); }
			// propagate any new facts before starting new work
			if (s.simplify())       { return true; }
			// s now has a conflict - either an artifical stop conflict
			// or a real conflict - we'll handle it in the next iteration
			// via the call to popRootLevel()
			popped = 0;
		}
		else if (shared_->allowSplit()) {
			// gp mode is active - request a split	
			// and wait until someone has work for us
			reportProgress(MessageEvent(s, "SPLIT", MessageEvent::sent));
			shared_->postMessage(SharedData::msg_split, false);
			if (!shared_->workSem.down() && !shared_->synchronize()) {
				// we are the last man standing, there is no
				// work left - quitting time?
				terminate(s, true);
			}
		}
		else {
			// portfolio mode is active - no splitting, no work left
			// quitting time? 
			terminate(s, true);
		}
	}
	return false;
}

// terminated from inside of algorithm
// check if there is more to do
void ParallelSolve::terminate(Solver& s, bool complete) {
	if (!shared_->terminate()) {
		if (enumerator().tentative() && complete) {
			if (shared_->setControl(SharedData::sync_flag|SharedData::complete_flag)) {
				thread_[s.id()]->setWinner();
				reportProgress(MessageEvent(s, "SYNC", MessageEvent::sent));
			}
		}
		else {
			reportProgress(MessageEvent(s, "TERMINATE", MessageEvent::sent));
			shared_->postMessage(SharedData::msg_terminate, true);
			thread_[s.id()]->setWinner();
			if (complete) { shared_->setControl(SharedData::complete_flag); }
		}
	}
}

// handles an active synchronization request
// returns true to signal that s should restart otherwise false
bool ParallelSolve::waitOnSync(Solver& s) {
	if (!thread_[s.id()]->handleRestartMessage()) {
		shared_->setControl(SharedData::cancel_restart_flag);
	}
	bool hasPath  = thread_[s.id()]->hasPath();
	bool tentative= enumerator().tentative();
	if (shared_->workSem.wait()) {
		// last man standing - complete synchronization request
		shared_->workReq     = 0;
		shared_->restartReq  = 0;
		bool restart         = shared_->hasControl(SharedData::restart_flag);
		bool init            = true;
		if (restart) {
			init = shared_->allowRestart() && !shared_->hasControl(SharedData::cancel_restart_flag);
			if (init) { shared_->globalR.next(); }
			shared_->maxConflict = shared_->allowRestart() && shared_->globalR.idx < maxRestarts_
				? shared_->globalR.current()
				: UINT64_MAX;
		}
		else if (shared_->maxConflict != UINT64_MAX && !shared_->allowRestart()) {
			shared_->maxConflict = UINT64_MAX;
		}
		if (init) { initQueue();  }
		else      { shared_->setControl(SharedData::restart_abandoned_flag); }
		if (tentative && shared_->complete()) {
			if (enumerator().commitComplete()) { shared_->setControl(SharedData::terminate_flag); }
			else                               { shared_->modCount = uint32(0); shared_->clearControl(SharedData::complete_flag); }
		}
		shared_->clearControl(SharedData::msg_split | SharedData::msg_sync_restart | SharedData::restart_abandoned_flag | SharedData::cancel_restart_flag);
		shared_->syncT.lap();
		reportProgress(MessageEvent(s, "SYNC", MessageEvent::completed, shared_->syncT.elapsed()));
		// wake up all blocked threads
		shared_->workSem.reset();
	}
	return shared_->terminate() || (hasPath && !shared_->hasControl(SharedData::restart_abandoned_flag));
}

// If guiding path scheme is active only one
// thread can start with gp (typically empty) and this
// thread must then split up search-space dynamically.
// Otherwise, all threads can start with initial gp
// TODO:
//  heuristic for initial splits?
void ParallelSolve::initQueue() {
	shared_->clearQueue();
	uint64 init = UINT64_MAX;
	if (shared_->allowSplit()) {
		if (modeSplit_ && !enumerator().supportsSplitting(*shared_->ctx)) {
			shared_->ctx->report(warning(Event::subsystem_solve, "Selected strategies imply Mode=compete."));
			shared_->clearControl(SharedData::allow_split_flag);
			shared_->setControl(SharedData::forbid_restart_flag);
			modeSplit_ = false;
		}
		else {
			init = 0;
			shared_->workQ.push(shared_->path);
		}
	}
	shared_->initMask = init;
	assert(shared_->allowSplit() || shared_->hasControl(SharedData::forbid_restart_flag));
}

// adds work to the work-queue
void ParallelSolve::pushWork(LitVec* v) { 
	assert(v);
	shared_->workQ.push(v);
	shared_->workSem.up();
}

// called whenever some solver proved unsat
bool ParallelSolve::commitUnsat(Solver& s) {
	if (!enumerator().optimize() || shared_->terminate() || shared_->synchronize()) {
		return false; 
	}
	if (!thread_[s.id()]->disjointPath()) {
		lock_guard<mutex> lock(shared_->modelM);
		if (!enumerator().commitUnsat(s)) {
			terminate(s, true);
			return false;
		}
		++shared_->modCount;
		return true;
	}
	return enumerator().commitUnsat(s);
}

// called whenever some solver has found a model
bool ParallelSolve::commitModel(Solver& s) { 
	// grab lock - models must be processed sequentially
	// in order to simplify printing and to avoid duplicates
	// in all non-trivial enumeration modes
	bool stop = false;
	{lock_guard<mutex> lock(shared_->modelM);
	// first check if the model is still valid once all
	// information is integrated into the solver
	if (thread_[s.id()]->isModel(s) && (stop=shared_->terminate()) == false && enumerator().commitModel(s)) {
		if (enumerator().lastModel().num == 1 && !enumerator().supportsRestarts()) {
			// switch to backtracking based splitting algorithm
			// the solver's gp will act as the root for splitting and is
			// from now on disjoint from all other gps
			shared_->setControl(SharedData::forbid_restart_flag | SharedData::allow_split_flag);
			thread_[s.id()]->setGpType(gp_split); 
			enumerator().setDisjoint(s, true);
		}
		++shared_->modCount;
		if ((stop = !reportModel(s)) == true) {
			// must be called while holding the lock - otherwise
			// we have a race condition with solvers that
			// are currently blocking on the mutex and we could enumerate 
			// more models than requested by the user
			terminate(s, !moreModels(s));
		}
	}}
	return !stop;
}

uint64 ParallelSolve::hasErrors() const {
	return shared_->error;
}
bool ParallelSolve::interrupted() const {
	return shared_->interrupt();
}
void ParallelSolve::resetSolve() {
	shared_->control = 0;
}
void ParallelSolve::enableInterrupts() {}
// updates s with new messages and uses s to create a new guiding path
// if necessary and possible
bool ParallelSolve::handleMessages(Solver& s) {
	// check if there are new messages for s
	if (!shared_->hasMessage()) {
		// nothing to do
		return true; 
	}
	ParallelHandler* h = thread_[s.id()];
	if (shared_->terminate()) {
		reportProgress(MessageEvent(s, "TERMINATE", MessageEvent::received));
		h->handleTerminateMessage();
		s.setStopConflict();
		return false;
	}
	if (shared_->synchronize()) {
		reportProgress(MessageEvent(s, "SYNC", MessageEvent::received));
		if (waitOnSync(s)) {
			s.setStopConflict();
			return false;
		}
		return true;
	}
	if (h->disjointPath() && s.splittable() && shared_->workReq > 0) {
		// First declare split request as handled
		// and only then do the actual split.
		// This way, we minimize the chance for 
		// "over"-splitting, i.e. one split request handled
		// by more than one thread.
		shared_->aboutToSplit();
		reportProgress(MessageEvent(s, "SPLIT", MessageEvent::received));
		h->handleSplitMessage();
		enumerator().setDisjoint(s, true);
	}
	return true;
}

bool ParallelSolve::integrateModels(Solver& s, uint32& upCount) {
	uint32 gCount = shared_->modCount;
	return gCount == upCount || (enumerator().update(s) && (upCount = gCount) == gCount);
}

void ParallelSolve::requestRestart() {
	if (shared_->allowRestart() && ++shared_->restartReq == numThreads()) {
		shared_->postMessage(SharedData::msg_sync_restart, true);
	}
}

SolveAlgorithm* ParallelSolveOptions::createSolveObject() const {
	return numSolver() > 1 ? new ParallelSolve(0, *this) : BasicSolveOptions::createSolveObject();
}
////////////////////////////////////////////////////////////////////////////////////
// ParallelHandler
/////////////////////////////////////////////////////////////////////////////////////////
ParallelHandler::ParallelHandler(ParallelSolve& ctrl, Solver& s, const SolveParams& p) 
	: MessageHandler()
	, ctrl_(&ctrl)
	, solver_(&s)
	, params_(&p)
	, received_(0)
	, recEnd_(0)
	, intEnd_(0)
	, error_(0)
	, win_(0)
	, up_(0) {
	this->next = this;
}

ParallelHandler::~ParallelHandler() { clearDB(0); delete [] received_; }

// adds this as post propagator to its solver and attaches the solver to ctx.
bool ParallelHandler::attach(SharedContext& ctx) {
	assert(solver_ && params_);
	gp_.reset();
	error_  = 0;
	win_    = 0;
	up_     = 0;
	act_    = 0;
	next    = 0;
	if (!received_ && ctx.distributor.get()) {
		received_ = new SharedLiterals*[RECEIVE_BUFFER_SIZE];
	}
	ctx.report(message(Event::subsystem_solve, "attach", solver_));
	solver_->addPost(this);
	return ctx.attach(solver_->id());
}

// removes this from the list of post propagators of its solver and detaches the solver from ctx.
void ParallelHandler::detach(SharedContext& ctx, bool) {
	handleTerminateMessage();
	if (solver_->sharedContext() == &ctx) {
		clearDB(!error() ? solver_ : 0);
		ctx.detach(*solver_, error() != 0);
	}
	ctx.report(message(Event::subsystem_solve, "detach", solver_));
}

void ParallelHandler::clearDB(Solver* s) {
	for (ClauseDB::iterator it = integrated_.begin(), end = integrated_.end(); it != end; ++it) {
		ClauseHead* c = static_cast<ClauseHead*>(*it);
		if (s) { s->addLearnt(c, c->size(), Constraint_t::learnt_other); }
		else   { c->destroy(); }
	}
	integrated_.clear();
	intEnd_= 0;
	for (uint32 i = 0; i != recEnd_; ++i) { received_[i]->release(); }
	recEnd_= 0;
}

ValueRep ParallelHandler::solveGP(BasicSolve& solve, GpType t, uint64 restart) {
	ValueRep res = value_free;
	bool     term= false;
	Solver&  s   = solve.solver();
	gp_.reset(restart, t);
	assert(act_ == 0);
	do {
		ctrl_->integrateModels(s, gp_.modCount);
		up_ = act_ = 1; // activate enumerator and bounds
		res = solve.solve();
		up_ = act_ = 0; // de-activate enumerator and bounds
		if      (res == value_true)  { term = !ctrl_->commitModel(s); }
		else if (res == value_false) { term = !ctrl_->commitUnsat(s); solve.reset(term); gp_.reset(restart, gp_.type); }
	} while (!term && res != value_free);
	return res;
}

// detach from solver, i.e. ignore any further messages 
void ParallelHandler::handleTerminateMessage() {
	if (this->next != this) {
		// mark removed propagator by creating "self-loop"
		solver_->removePost(this);
		this->next = this;
	}
}

// split-off new guiding path and add it to solve object
void ParallelHandler::handleSplitMessage() {
	assert(solver_ && "ParallelHandler::handleSplitMessage(): not attached!");
	assert(solver_->splittable());
	Solver& s = *solver_;
	SingleOwnerPtr<LitVec> newPath(new LitVec());
	s.split(*newPath);
	ctrl_->pushWork(newPath.release());
}

bool ParallelHandler::handleRestartMessage() {
	// TODO
	// we may want to implement some heuristic, like
	// computing a local var order. 
	return true;
}

bool ParallelHandler::simplify(Solver& s, bool sh) {
	ClauseDB::size_type i, j, end = integrated_.size();
	for (i = j = 0; i != end; ++i) {
		Constraint* c = integrated_[i];
		if (c->simplify(s, sh)) { 
			c->destroy(&s, false); 
			intEnd_ -= (i < intEnd_);
		}
		else                    { 
			integrated_[j++] = c;  
		}
	}
	shrinkVecTo(integrated_, j);
	if (intEnd_ > integrated_.size()) intEnd_ = integrated_.size();
	return false;
}

bool ParallelHandler::propagateFixpoint(Solver& s, PostPropagator* ctx) {
	// Check for messages and integrate any new information from
	// models/lemma exchange but only if path is setup.
	// Skip updates if called from other post propagator so that we do not
	// disturb any active propagation.
	if (int up = (ctx == 0 && up_ != 0)) {
		up_ ^= (uint32)s.updateMode();
		up  += (act_ == 0 || (up_ && (s.stats.choices & 63) != 0));
		if (s.stats.conflicts >= gp_.restart)  { ctrl_->requestRestart(); gp_.restart *= 2; }
		for (uint32 cDL = s.decisionLevel();;) {
			bool ok = ctrl_->handleMessages(s) && (up > 1 ? integrate(s) : ctrl_->integrateModels(s, gp_.modCount));
			if (!ok)                         { return false; }
			if (cDL != s.decisionLevel())    { // cancel active propagation on cDL
				for (PostPropagator* n = next; n ; n = n->next){ n->reset(); }
				cDL = s.decisionLevel();
			}	
			if      (!s.queueSize())         { if (++up == 3) return true; }
			else if (!s.propagateUntil(this)){ return false; }
		}
	}
	return ctrl_->handleMessages(s);
}

// checks whether s still has a model once all 
// information from previously found models were integrated 
bool ParallelHandler::isModel(Solver& s) {
	assert(s.numFreeVars() == 0);
	// either no unprocessed updates or still a model after
	// updates were integrated
	return ctrl_->integrateModels(s, gp_.modCount)
		&& s.numFreeVars() == 0
		&& s.queueSize()   == 0;
}
bool ParallelHandler::integrate(Solver& s) {
	uint32 rec = recEnd_ + s.receive(received_ + recEnd_, RECEIVE_BUFFER_SIZE - recEnd_);
	if (!rec) { return true; }
	ClauseCreator::Result ret;
	uint32 dl       = s.decisionLevel(), added = 0, i = 0;
	uint32 intFlags = ctrl_->integrateFlags();
	recEnd_         = 0;
	if (params_->reduce.strategy.glue != 0) {
		intFlags |= ClauseCreator::clause_int_lbd;
	}
	do {
		ret    = ClauseCreator::integrate(s, received_[i++], intFlags, Constraint_t::learnt_other);
		added += ret.status != ClauseCreator::status_subsumed; 
		if (ret.local) { add(ret.local); }
		if (ret.unit()){ s.stats.addIntegratedAsserting(dl, s.decisionLevel()); dl = s.decisionLevel(); }
		if (!ret.ok()) { while (i != rec) { received_[recEnd_++] = received_[i++]; } }
	} while (i != rec);
	s.stats.addIntegrated(added);
	return !s.hasConflict();
}

void ParallelHandler::add(ClauseHead* h) {
	if (intEnd_ < integrated_.size()) {
		ClauseHead* o = (ClauseHead*)integrated_[intEnd_];
		integrated_[intEnd_] = h;
		assert(o);
		if (!ctrl_->integrateUseHeuristic() || o->locked(*solver_) || o->activity().activity() > 0) {
			solver_->addLearnt(o, o->size(), Constraint_t::learnt_other);
		}
		else {
			o->destroy(solver_, true);
			solver_->stats.removeIntegrated();
		}
	}
	else {
		integrated_.push_back(h);
	}
	if (++intEnd_ >= ctrl_->integrateGrace()) {
		intEnd_ = 0;
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// Distribution
/////////////////////////////////////////////////////////////////////////////////////////
uint64 ParallelSolveOptions::initPeerMask(uint32 id, Integration::Topology topo, uint32 maxT)  {
	if (topo == Integration::topo_all) { return Distributor::initSet(maxT) ^ Distributor::mask(id); }
	if (topo == Integration::topo_ring){
		uint32 prev = id > 0 ? id - 1 : maxT - 1;
		uint32 next = (id + 1) % maxT;
		return Distributor::mask(prev) | Distributor::mask(next);
	}
	bool ext = topo == Integration::topo_cubex;
	uint32 n = maxT;
	uint32 k = 1;
	for (uint32 i = n / 2; i > 0; i /= 2, k *= 2) { }
	uint64 res = 0, x = 1;
	for (uint32 m = 1; m <= k; m *= 2) {
		uint32 i = m ^ id;
		if      (i < n)         { res |= (x << i);     }
		else if (ext && k != m) { res |= (x << (i^k)); }
	}
	if (ext) {
		uint32 s = k ^ id;
		for(uint32 m = 1; m < k && s >= n; m *= 2) {
			uint32 i = m ^ s;
			if (i < n) { res |= (x << i); }
		}
	}
	assert( (res & (x<<id)) == 0 );
	return res;
}
/////////////////////////////////////////////////////////////////////////////////////////
// GlobalDistribution
/////////////////////////////////////////////////////////////////////////////////////////
GlobalDistribution::GlobalDistribution(const Policy& p, uint32 maxT, uint32 topo) : Distributor(p), queue_(0) {
	typedef ParallelSolveOptions::Integration::Topology Topology;
	assert(maxT <= ParallelSolveOptions::supportedSolvers());
	Topology t = static_cast<Topology>(topo);
	queue_     = new Queue(maxT);
	threadId_  = (ThreadInfo*)alignedAlloc((maxT * sizeof(ThreadInfo)), 64);
	for (uint32 i = 0; i != maxT; ++i) {
		new (&threadId_[i]) ThreadInfo;
		threadId_[i].id       = queue_->addThread();
		threadId_[i].peerMask = ParallelSolveOptions::initPeerMask(i, t, maxT);
	}
}
GlobalDistribution::~GlobalDistribution() {
	static_assert(sizeof(ThreadInfo) == 64, "Invalid size");
	release();
}
void GlobalDistribution::release() {
	if (queue_) {
		for (uint32 i = 0; i != queue_->maxThreads(); ++i) {
			Queue::ThreadId& id = getThreadId(i);
			for (ClausePair n; queue_->tryConsume(id, n); ) { 
				if (n.sender != i) { n.lits->release(); }
			}
			threadId_[i].~ThreadInfo();
		}
		delete queue_;
		queue_ = 0;
		alignedFree(threadId_);
	}
}
void GlobalDistribution::publish(const Solver& s, SharedLiterals* n) {
	assert(n->refCount() >= (queue_->maxThreads()-1));
	queue_->publish(ClausePair(s.id(), n), getThreadId(s.id()));
}
uint32 GlobalDistribution::receive(const Solver& in, SharedLiterals** out, uint32 maxn) {
	uint32 r = 0;
	Queue::ThreadId& id = getThreadId(in.id());
	uint64 peers = getPeerMask(in.id());
	for (ClausePair n; r != maxn && queue_->tryConsume(id, n); ) {
		if (n.sender != in.id()) {
			if (inSet(peers, n.sender))  { out[r++] = n.lits; }
			else if (n.lits->size() == 1){ out[r++] = n.lits; }
			else                         { n.lits->release(); }
		}
	}
	return r;
}

/////////////////////////////////////////////////////////////////////////////////////////
// LocalDistribution
/////////////////////////////////////////////////////////////////////////////////////////
LocalDistribution::LocalDistribution(const Policy& p, uint32 maxT, uint32 topo) : Distributor(p), thread_(0), numThread_(0) {
	typedef ParallelSolveOptions::Integration::Topology Topology;
	assert(maxT <= ParallelSolveOptions::supportedSolvers());
	Topology t = static_cast<Topology>(topo);
	thread_    = new ThreadData*[numThread_ = maxT];
	size_t sz  = ((sizeof(ThreadData) + 63) / 64) * 64;
	for (uint32 i = 0; i != maxT; ++i) {
		ThreadData* ti = new (alignedAlloc(sz, 64)) ThreadData;
		ti->received.init(&ti->sentinal);
		ti->peers = ParallelSolveOptions::initPeerMask(i, t, maxT);
		ti->free  = 0;
		thread_[i]= ti;
	}
}
LocalDistribution::~LocalDistribution() {
	while (numThread_) {
		ThreadData* ti      = thread_[--numThread_];
		thread_[numThread_] = 0;
		for (QNode* n; (n = ti->received.pop()) != 0; ) {
			static_cast<SharedLiterals*>(n->data)->release();
		}
		ti->~ThreadData();
		alignedFree(ti);
	}
	for (MPSCPtrQueue::RawNode* n; (n = blocks_.tryPop()) != 0; ) {
		alignedFree(n);
	}
	delete [] thread_;
}

void LocalDistribution::freeNode(uint32 tId, QNode* n) const {
	if (n != &thread_[tId]->sentinal) {
		n->next = thread_[tId]->free;
		thread_[tId]->free = n;
	}
}

LocalDistribution::QNode* LocalDistribution::allocNode(uint32 tId, SharedLiterals* clause) {
	for (ThreadData* td = thread_[tId];;) {
		if (QNode* n = td->free) {
			td->free = static_cast<QNode*>(static_cast<MPSCPtrQueue::RawNode*>(n->next));
			n->data  = clause;
			return n;
		}
		// alloc a new block of node;
		const uint32 nNodes = 128;
		QNode* raw = (QNode*)alignedAlloc(sizeof(QNode) * nNodes, 64);
		// add nodes [1, nNodes) to free list
		for (uint32 i = 1; i != nNodes-1; ++i) {
			raw[i].next = &raw[i+1];
		}
		raw[nNodes-1].next = 0;
		td->free = &raw[1];
		// use first node to link to block list
		blocks_.push(raw);
	}
}

void LocalDistribution::publish(const Solver& s, SharedLiterals* n) {
	assert(n->refCount() >= (numThread_-1));
	uint32 sender = s.id();
	uint32 size   = n->size();
	uint32 decRef = 0;
	for (uint32 i = 0; i != numThread_; ++i) {
		if (i == sender) { continue; }
		if (size > 1 && !inSet(thread_[i]->peers, sender)) { ++decRef; }
		else {
			QNode* node = allocNode(sender, n);
			thread_[i]->received.push(node);
		}
	}
	if (decRef) { n->release(decRef); }
}
uint32 LocalDistribution::receive(const Solver& in, SharedLiterals** out, uint32 maxn) {
	MPSCPtrQueue& q = thread_[in.id()]->received;
	QNode*        n = 0;
	uint32        r = 0;
	while (r != maxn && (n = q.pop()) != 0) {
		out[r++] = static_cast<SharedLiterals*>(n->data);
		freeNode(in.id(), n);
	}
	return r;
}

} } // namespace Clasp::mt

#endif
