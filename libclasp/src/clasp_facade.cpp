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
#include <clasp/clasp_facade.h>
#include <clasp/lookahead.h>
#include <clasp/dependency_graph.h>
#include <clasp/minimize_constraint.h>
#include <clasp/unfounded_check.h>
#include <clasp/parser.h>
#include <clasp/clingo.h>
#include <clasp/util/timer.h>
#include <cstdio>
#include <cstdlib>
#include <signal.h>
#include <climits>
#include <utility>
#if WITH_THREADS
#include <clasp/util/mutex.h>
#endif
namespace Clasp {
/////////////////////////////////////////////////////////////////////////////////////////
// ClaspConfig
/////////////////////////////////////////////////////////////////////////////////////////
struct ClaspConfig::Impl {
	struct ConfiguratorProxy {
		enum { OnceBit = 62, AcquireBit = 61 };
		ConfiguratorProxy(Configurator* c, Ownership_t::Type own, bool once) : cfg(reinterpret_cast<uintp>(c)), set(0) {
			if (once) { store_set_bit(cfg, OnceBit); }
			if (own == Ownership_t::Acquire) { store_set_bit(cfg, AcquireBit); }
			assert(ptr() == c);
		}
		bool addPost(Solver& s) {
			CLASP_FAIL_IF(s.id() > 63, "invalid solver id!");
			if (test_bit(set, s.id()))  { return true;  }
			if (test_bit(cfg, OnceBit)) { store_set_bit(set, s.id()); }
			return this->ptr()->addPost(s);
		}
		void prepare(SharedContext& ctx) {
			if (ctx.concurrency() < 64) {
				set &= (bit_mask<uint64>(ctx.concurrency()) - 1);
			}
			ptr()->prepare(ctx);
		}
		void destroy() { if (test_bit(cfg, AcquireBit)) { delete ptr(); } }
		Configurator* ptr() const {
			static const uint64 ptrMask = ~(bit_mask<uint64>(OnceBit) | bit_mask<uint64>(AcquireBit));
			return reinterpret_cast<Configurator*>(static_cast<uintp>(cfg & ptrMask));
		}
		uint64 cfg;
		uint64 set;
	};
	typedef PodVector<ConfiguratorProxy>::type PPVec;
	Impl()  { acycSet = 0; }
	~Impl() { reset(); }
	void reset();
	void prepare(SharedContext& ctx);
	bool addPost(Solver& s, const SolverParams& opts);
	void add(Configurator* c, Ownership_t::Type t, bool once) { pp.push_back(ConfiguratorProxy(c, t, once)); }
	PPVec   pp;
	uint64  acycSet;
#if WITH_THREADS
	Clasp::mt::mutex mutex;
#endif
};
void ClaspConfig::Impl::reset() {
	for (; !pp.empty(); pp.pop_back()) { pp.back().destroy(); }
}

void ClaspConfig::Impl::prepare(SharedContext& ctx) {
	if (ctx.concurrency() < 64) {
		acycSet &= (bit_mask<uint64>(ctx.concurrency()) - 1);
	}
	for (PPVec::iterator it = pp.begin(), end = pp.end(); it != end; ++it) {
		it->prepare(ctx);
	}
}
bool ClaspConfig::Impl::addPost(Solver& s, const SolverParams& opts) {
#if WITH_THREADS
#define LOCKED() for (Clasp::mt::unique_lock<Clasp::mt::mutex> lock(mutex); lock.owns_lock(); lock.unlock())
#else
#define LOCKED()
#endif
	CLASP_FAIL_IF(s.sharedContext() == 0, "Solver not attached!");
	if (s.sharedContext()->sccGraph.get()) {
		if (DefaultUnfoundedCheck* pp = static_cast<DefaultUnfoundedCheck*>(s.getPost(PostPropagator::priority_reserved_ufs))) {
			pp->setReasonStrategy(static_cast<DefaultUnfoundedCheck::ReasonStrategy>(opts.loopRep));
		}
		else if (!s.addPost(new DefaultUnfoundedCheck(*s.sharedContext()->sccGraph, static_cast<DefaultUnfoundedCheck::ReasonStrategy>(opts.loopRep)))) {
			return false;
		}
	}
	if (s.sharedContext()->extGraph.get()) {
		bool addAcyc = false;
		// protect access to acycSet
		LOCKED() { addAcyc = !test_bit(acycSet, s.id()) && store_set_bit(acycSet, s.id()); }		
		if (addAcyc && !s.addPost(new AcyclicityCheck(s.sharedContext()->extGraph.get()))) {
			return false;
		}
	}
	for (PPVec::iterator it = pp.begin(), end = pp.end(); it != end; ++it) {
		// protect call to user code
		LOCKED() { if (!it->addPost(s)) { return false; } }
	}
	return true;
#undef LOCKED
}

ClaspConfig::ClaspConfig() : tester_(0), impl_(new Impl()) {}
ClaspConfig::~ClaspConfig() { 
	delete impl_;
	delete tester_;
}

void ClaspConfig::reset() {
	if (tester_) { tester_->reset(); }
	impl_->reset();
	BasicSatConfig::reset();
	solve = SolveOptions();
	asp   = AspOptions();
}

BasicSatConfig* ClaspConfig::addTesterConfig() {
	if (!tester_) { tester_ = new BasicSatConfig(); }
	return tester_;
}

void ClaspConfig::prepare(SharedContext& ctx) {
	BasicSatConfig::prepare(ctx);
	uint32 numS = solve.numSolver();
	if (numS > solve.supportedSolvers()) {
		ctx.warn("Too many solvers.");
		numS = solve.supportedSolvers();
	}
	if (numS > solve.recommendedSolvers()) {
		ctx.warn(ClaspStringBuffer().appendFormat("Oversubscription: #Threads=%u exceeds logical CPUs=%u.", numS, solve.recommendedSolvers()));
	}
	for (uint32 i = 0; i != numS; ++i) {
		if (solver(i).heuId == Heuristic_t::Domain) {
			parse.enableHeuristic();
			break;
		}
	}
	solve.setSolvers(numS);
	if (std::abs(static_cast<int>(solve.numModels)) != 1 || !solve.models()) { 
		ctx.setPreserveModels(true);
	}
	ctx.setConcurrency(solve.numSolver(), SharedContext::resize_resize);
	impl_->prepare(ctx);
}

Configuration* ClaspConfig::config(const char* n) {
	return (n && std::strcmp(n, "tester") == 0) ? testerConfig() : BasicSatConfig::config(n);
}

void ClaspConfig::addConfigurator(Configurator* c, Ownership_t::Type t, bool once) {
	impl_->add(c, t, once);
}

bool ClaspConfig::addPost(Solver& s) const {
	return impl_->addPost(s, solver(s.id())) && BasicSatConfig::addPost(s);
}

ClaspConfig::Configurator::~Configurator() {}
void ClaspConfig::Configurator::prepare(SharedContext&) {}
/////////////////////////////////////////////////////////////////////////////////////////
// ClaspFacade::SolveData/SolveStrategy/AsyncResult
/////////////////////////////////////////////////////////////////////////////////////////
struct ClaspFacade::SolveStrategy {
public:
	enum { SIGCANCEL = 9 };
	enum State { state_start = 0, state_running = 1, state_result = 2, state_model = 3, state_done = 6 };
	SolveStrategy()         { state = signal = 0; nrefs = 1; facade = 0; algo = 0; handler = 0; aTop = 0; }
	virtual ~SolveStrategy(){}
	bool running() const    { return (state & state_running) != 0; }
	bool hasResult()const   { return (state & state_result)  != 0; }
	bool interrupt(int sig) {
		if (!running()) { return false; }
		if (!signal)    { signal = sig; } 
		return cancel(sig);
	}
	void solve(ClaspFacade& f, const LitVec& a, SolveAlgorithm* algo, EventHandler* h) {
		this->facade  = &f;
		this->algo    = algo;
		this->handler = h;
		this->aTop    = (uint32)f.assume_.size();
		this->state   = 0;
		this->signal  = 0;
		f.assume_.insert(f.assume_.end(), a.begin(), a.end());
		if (!isSentinel(f.ctx.stepLiteral())) {
			f.assume_.push_back(f.ctx.stepLiteral());
		}
		preSolve();
		this->state = state_running;
		facade->interrupt(0); // handle pending interrupts
		doSolve();
	}
	void detach(State end, bool more) {
		facade->assume_.resize(aTop);
		facade->stopStep(signal, !more);
		if (handler) { handler->onEvent(StepReady(facade->summary())); }
		state = end;
	}
	bool attach() {
		CLASP_FAIL_IF(!running(), "invalid call to attach!");
		if (!signal && !facade->ctx.master()->hasConflict()) {
			facade->step_.solveTime = facade->step_.unsatTime = RealTime::getTime();
			return true;
		}
		facade->ctx.report(Event::subsystem_solve);
		return false;
	}
	bool startGen() {
		if (!attach()) {
			detach(state_done, facade->ok());
			return false;
		}
		algo->start(facade->ctx, facade->assume_, facade);
		return true;
	}
	void stopGen() {
		if (running()) {
			algo->stop();
			detach(state_done, algo->more());
		}
	}
	virtual void release()  {}
	virtual bool cancel(int){ return algo->interrupt(); }
	ClaspFacade*    facade;
	SolveAlgorithm* algo;
	EventHandler*   handler;
	typedef Clasp::Atomic_t<int>::type SafeIntType;
	SafeIntType     state;
	SafeIntType     signal;
	SafeIntType     nrefs;   // Facade + #(Async)Result objects
	uint32          aTop;
protected:
	void runAlgo(State end);
	virtual void preSolve() {}
	virtual void doSolve() = 0;
};
struct ClaspFacade::SolveData {
	struct CostArray {
		CostArray() : data(0) {}
		~CostArray() { while (!refs.empty()) { delete refs.back(); refs.pop_back(); } }
		struct LevelRef {
			LevelRef(const CostArray* a, uint32 l) : arr(a), at(l) {}
			static double value(const LevelRef* ref) {
				CLASP_FAIL_IF(ref->at >= ref->arr->size(), "expired key");
				return static_cast<double>(ref->arr->data->costs->at(ref->at));
			}
			const CostArray* arr;
			uint32           at;
		};
		typedef PodVector<LevelRef*>::type ElemVec;
		uint32 size() const {
			return data && data->costs ? sizeVec(*data->costs) : 0;
		}
		StatisticObject at(uint32 i) const {
			CLASP_FAIL_IF(i >= size(), "invalid key");
			while (i >= refs.size()) { refs.push_back(new LevelRef(this, sizeVec(refs))); }
			return StatisticObject::value<LevelRef, &LevelRef::value>(refs[i]);
		}
		const Model*    data;
		mutable ElemVec refs;
	};
	typedef SingleOwnerPtr<SolveAlgorithm> AlgoPtr;
	typedef SingleOwnerPtr<Enumerator>     EnumPtr;
	typedef Clasp::Atomic_t<int>::type     SafeIntType;
	SolveData() : en(0), algo(0), active(0), prepared(false), interruptible(false) { qSig = 0; }
	~SolveData() { reset(); }
	void init(SolveAlgorithm* algo, Enumerator* en) {
		this->en   = en;
		this->algo = algo;
		this->algo->setEnumerator(*en);
		if (interruptible) {
			this->algo->enableInterrupts();
		}
	}
	void reset() {
		if (active)    { active->interrupt(SolveStrategy::SIGCANCEL); active->release(); active = 0; }
		if (algo.get()){ algo->resetSolve(); }
		if (en.get())  { en->reset(); }
		prepared = false;
	}
	void prepareEnum(SharedContext& ctx, int64 numM, EnumOptions::OptMode opt, EnumMode mode) {
		CLASP_FAIL_IF(active, "Solve operation still active");
		if (ctx.ok() && !ctx.frozen() && !prepared) {
			if (mode == enum_volatile && ctx.solveMode() == SharedContext::solve_multi) {
				ctx.requestStepVar();
			}
			int lim = en->init(ctx, opt, (int)Range<int64>(-1, INT_MAX).clamp(numM));
			if (lim == 0 || numM < 0) {
				numM = lim;
			}
			algo->setEnumLimit(numM ? static_cast<uint64>(numM) : UINT64_MAX);
			costs.data = lastModel();
			prepared = true;
		}
	}
	bool update(const Solver& s, const Model& m) { return !active->handler || active->handler->onModel(s, m); }
	bool interrupt(int sig) { 
		if (solving()) { return active->interrupt(sig); }
		if (!qSig && sig != SolveStrategy::SIGCANCEL) { qSig = sig; }
		return false;
	}
	bool                      solving()   const  { return active && active->running();   }
	const Model*              lastModel() const  { return en.get() ? &en->lastModel() : 0; }
	const SharedMinimizeData* minimizer() const  { return en.get() ? en->minimizer() : 0; } 
	Enumerator*               enumerator()const  { return en.get(); }
	int                       modelType() const  { return en.get() ? en->modelType() : 0; }
	EnumPtr        en;
	AlgoPtr        algo;
	SolveStrategy* active;
	CostArray      costs;
	SafeIntType    qSig;
	bool           prepared;
	bool           interruptible;
};
void ClaspFacade::SolveStrategy::runAlgo(State done) {
	struct OnExit {
		OnExit(SolveStrategy* s, int st) : self(s), endState(st), more(true) {}
		~OnExit() {
			self->detach(static_cast<State>(endState), more);
		}
		SolveStrategy* self;
		int            endState;
		bool           more;
	} scope(this, done);
	scope.more = attach() ? algo->solve(facade->ctx, facade->assume_, facade) : facade->ctx.ok();
}

#if WITH_THREADS
struct ClaspFacade::AsyncSolve : public SolveStrategy, public EventHandler {
	enum { ASYNC_ERROR = 128 };
	static EventHandler* asyncModelHandler() { return reinterpret_cast<EventHandler*>(0x1); }
	AsyncSolve() { }
	bool hasError() const { 
		return (result.flags & uint8(ASYNC_ERROR)) != 0u;
	}
	virtual void preSolve() {
		if (handler == AsyncSolve::asyncModelHandler()) { handler = this; }
		algo->enableInterrupts();
		result = facade->result();
	}
	virtual void doSolve() {
		// start solve in worker thread
		Clasp::mt::thread(std::mem_fun(&AsyncSolve::threadMain), this).swap(task);
	}
	virtual bool cancel(int sig) {
		if (!SolveStrategy::cancel(sig)) return false;
		if (sig == SIGCANCEL) {
			wait();
			join();
		}
		return true;
	}
	void threadMain() {
		uint8 err = 0;
		try         { runAlgo(state_running); }
		catch (...) { err = uint8(ASYNC_ERROR); }
		{
			mt::unique_lock<Clasp::mt::mutex> lock(this->mqMutex);
			result = facade->result();
			result.flags |= err;
			state  = state_done;
		}
		mqCond.notify_one();
	}
	virtual void release() {
		if      (--nrefs == 1) { interrupt(SIGCANCEL); }
		else if (nrefs   == 0) { join(); delete this; }
	}
	bool next() {
		if (state != state_model) { return false; }
		mt::unique_lock<Clasp::mt::mutex> lock(mqMutex);
		if (state != state_model) { return false; }
		state = state_running;
		mqCond.notify_one();
		return true;
	}
	bool wait(double s = -1.0) {
		if (state == state_start) { return false; }
		if (signal != 0)          { next(); }
		for (mt::unique_lock<Clasp::mt::mutex> lock(mqMutex); !hasResult(); ) {
			if (s < 0.0) { mqCond.wait(lock); }
			else {
				mqCond.wait_for(lock, s);
				if (!hasResult()) { return false; }
			}
		}
		if (state == state_done && join()) { 
			// Just in case other AsyncSolve objects are waiting on the computation.
			// This should not happen but there is some existing code that uses
			// an AsyncSolve object in a separate thread for cancellation.
			mqCond.notify_all();
		}
		return true;
	}
	bool onModel(const Solver&, const Model&) {
		const Result sat = {Result::SAT, 0};
		mt::unique_lock<Clasp::mt::mutex> lock(mqMutex);
		state = state_model;
		result= sat;
		mqCond.notify_one();
		while (state == state_model && signal == 0) { mqCond.wait(lock); }
		return signal == 0;
	}
	bool join() { if (task.joinable()) { task.join(); return true; } return false; }
	Clasp::mt::thread             task;   // async solving thread
	Clasp::mt::mutex              mqMutex;// protects mqCond
	Clasp::mt::condition_variable mqCond; // for iterating over models one by one
	Result                        result; // result of async operation
};
ClaspFacade::AsyncResult::AsyncResult(SolveData& x) : state_(static_cast<AsyncSolve*>(x.active)) { ++state_->nrefs; }
ClaspFacade::AsyncResult::~AsyncResult() { state_->release(); }
ClaspFacade::AsyncResult::AsyncResult(const AsyncResult& x) : state_(x.state_) { ++state_->nrefs; }
int  ClaspFacade::AsyncResult::interrupted()        const { return state_->signal; }
bool ClaspFacade::AsyncResult::error()              const { return ready() && state_->hasError(); }
bool ClaspFacade::AsyncResult::ready()              const { return state_->hasResult(); }
bool ClaspFacade::AsyncResult::ready(Result& r)     const { if (ready()) { r = get(); return true; } return false; }
bool ClaspFacade::AsyncResult::running()            const { return state_->running(); }
void ClaspFacade::AsyncResult::wait()               const { state_->wait(-1.0); }
bool ClaspFacade::AsyncResult::waitFor(double sec)  const { return state_->wait(sec); }
bool ClaspFacade::AsyncResult::next()               const { return state_->next(); }
bool ClaspFacade::AsyncResult::cancel()             const { return state_->interrupt(AsyncSolve::SIGCANCEL); }
ClaspFacade::Result ClaspFacade::AsyncResult::get() const {
	state_->wait();
	if (!state_->hasError()) { return state_->result; }
	throw std::runtime_error("Async operation failed!");
}
bool ClaspFacade::AsyncResult::end() const { 
	// first running() handles case where iterator is outdated (state was reset to start)
	// second running() is used to distinguish models from final result (summary)
	return !running() || !get().sat() || !running();
}
const Model& ClaspFacade::AsyncResult::model() const {
	CLASP_FAIL_IF(state_->state != AsyncSolve::state_model, "Invalid iterator access!");
	return const_cast<const SolveAlgorithm*>(state_->algo)->enumerator()->lastModel();
}
#endif
ClaspFacade::ModelGenerator::ModelGenerator(SolveStrategy& impl) : impl_(&impl) {
	++impl_->nrefs;
}
ClaspFacade::ModelGenerator::ModelGenerator(const ModelGenerator& o) : impl_(o.impl_) {
	++impl_->nrefs;
}
ClaspFacade::ModelGenerator::~ModelGenerator() {
	impl_->release();
}
bool ClaspFacade::ModelGenerator::next() const {
	if (!impl_->running()) {
		return false;
	}
	if (impl_->state != SolveStrategy::state_model && !impl_->startGen()) {
		return false;
	}
	if (!impl_->algo->next()) {
		impl_->stopGen();
		return false;
	}
	impl_->state = SolveStrategy::state_model;
	return true;
}
const Model& ClaspFacade::ModelGenerator::model()  const {
	CLASP_FAIL_IF(!impl_->hasResult(), "Invalid iterator access!");
	return impl_->facade->enumerator()->lastModel();
}
ClaspFacade::Result ClaspFacade::ModelGenerator::result() const {
	CLASP_FAIL_IF(!impl_->hasResult(), "Invalid iterator access!");
	return impl_->facade->result();
}
void ClaspFacade::ModelGenerator::stop() const {
	impl_->stopGen();
}
ClaspFacade::ModelGenerator ClaspFacade::startSolve(const LitVec& a) {
	prepare();
	struct MG : public SolveStrategy {
		virtual void doSolve() { }
		virtual void release() {
			switch (--nrefs) {
				case 1: stopGen();   break;
				case 0: delete this; break;
				default: break;
			}
		}
	};
	solve_->active = new MG();
	solve_->active->solve(*this, a, solve_->algo.get(), 0);
	return ModelGenerator(*solve_->active);
}
/////////////////////////////////////////////////////////////////////////////////////////
// ClaspFacade::Statistics
/////////////////////////////////////////////////////////////////////////////////////////
namespace {
struct KV { const char* key; StatisticObject(*get)(const ClaspFacade::Summary*); };
template <double ClaspFacade::Summary::*time>
StatisticObject _getT(const ClaspFacade::Summary* x) { return StatisticObject::value(&static_cast<const double&>((x->*time))); }
template <uint64 ClaspFacade::Summary::*model>
StatisticObject _getM(const ClaspFacade::Summary* x) { return StatisticObject::value(&static_cast<const uint64&>((x->*model))); }
static const KV sumKeys_s[] = {
	{"total"     , _getT<&ClaspFacade::Summary::totalTime>},
	{"cpu"       , _getT<&ClaspFacade::Summary::cpuTime>},
	{"solve"     , _getT<&ClaspFacade::Summary::solveTime>},
	{"unsat"     , _getT<&ClaspFacade::Summary::unsatTime>},
	{"sat"       , _getT<&ClaspFacade::Summary::satTime>},
	{"enumerated", _getM<&ClaspFacade::Summary::numEnum>},
	{"optimal"   , _getM<&ClaspFacade::Summary::numOptimal>},
};
struct SummaryStats {
	SummaryStats() : stats_(0), range_(0,0) {}
	void bind(const ClaspFacade::Summary& x, Range32 r) { 
		stats_ = &x;
		range_ = r;
	}
	uint32          size()        const { return range_.hi - range_.lo; }
	const char*     key(uint32 i) const { return i < size() ? sumKeys_s[i + range_.lo].key : throw std::out_of_range("SummaryStats::key()"); }
	StatisticObject at(const char* key) const {
		for (const KV* x = sumKeys_s + range_.lo, *end = sumKeys_s + range_.hi; x != end; ++x) {
			if (std::strcmp(x->key, key) == 0) { return x->get(stats_); }
		}
		throw std::out_of_range("SummaryStats::at()");
	}
	StatisticObject toStats() const { return StatisticObject::map(this); }
	const ClaspFacade::Summary* stats_;
	Range32 range_;
};

double _getConcurrency(const SharedContext* ctx) { return ctx->concurrency(); }
double _getResult(const SolveResult* r) { return static_cast<double>(r->operator Clasp::SolveResult::Base()); }
double _getSignal(const SolveResult* r) { return static_cast<double>(r->signal); }
double _getExhausted(const SolveResult* r) { return static_cast<double>(r->exhausted()); }
}
struct ClaspFacade::Statistics {
	Statistics(ClaspFacade& f) : self_(&f), tester_(0), level_(0), clingo_(0) {}
	~Statistics() { delete clingo_; delete solvers_.multi; }
	void start(uint32 level);
	void initLevel(uint32 level);
	void end();
	void addTo(StatsMap& solving, StatsMap* accu) const;
	void accept(StatsVisitor& out, bool final) const;
	bool incremental() const { return self_->incremental(); }
	Potassco::AbstractStatistics* getClingo();
	typedef StatsVec<SolverStats>        SolverVec;
	typedef SingleOwnerPtr<Asp::LpStats> LpStatsPtr;
	typedef PrgDepGraph::NonHcfStats     TesterStats;
	ClaspFacade*  self_;
	LpStatsPtr    lp_;      // level 0 and asp
	SolverStats   solvers_; // level 0
	SolverVec     solver_;  // level > 1
	SolverVec     accu_;    // level > 1 and incremental
	TesterStats*  tester_;  // level > 0 and nonhcfs
	uint32        level_;   // active stats level
	// For clingo stats interface
	class ClingoView : public ClaspStatistics {
	public:
		explicit ClingoView(const ClaspFacade& f);
		void update(const Statistics& s);
	private:
		struct StepStats {
			SummaryStats times;
			SummaryStats models;
			void bind(const ClaspFacade::Summary& x) {
				times.bind(x, Range32(0, 5));
				models.bind(x, Range32(5, 7));
			}
			void addTo(StatsMap& summary) {
				summary.add("times", times.toStats());
				summary.add("models", models.toStats());
			}
		};
		StatsMap keys_;
		StatsMap problem_;
		StatsMap solving_;
		struct Summary : StatsMap { StepStats step; } summary_;
		struct Accu    : StatsMap { StepStats step; StatsMap solving_; };
		typedef SingleOwnerPtr<Accu> AccuPtr;
		AccuPtr accu_;
	}* clingo_; // new clingo stats interface;
};
void ClaspFacade::Statistics::initLevel(uint32 level) {
	if (level_ < level) {
		if (incremental() && !solvers_.multi) { solvers_.multi = new SolverStats(); }
		level_ = level;
	}
	if (self_->isAsp() && !lp_.get()) {
		lp_ = new Asp::LpStats();
	}
	if (self_->ctx.sccGraph.get() && self_->ctx.sccGraph->numNonHcfs() && !tester_) {
		tester_ = self_->ctx.sccGraph->nonHcfStats();
	}
}
void ClaspFacade::Statistics::start(uint32 level) {
	// cleanup previous state
	solvers_.reset();
	solver_.reset();
	if (tester_) { tester_->startStep(self_->config()->testerConfig() ? self_->config()->testerConfig()->context().stats : 0); }
	// init next step
	initLevel(level);
	if (lp_.get() && self_->step_.lpStep()) {
		lp_->accu(*self_->step_.lpStep());
	}
	if (level > 1 && solver_.size() < self_->ctx.concurrency()) {
		uint32 sz = sizeVec(solver_);
		solver_.growTo(self_->ctx.concurrency());
		for (const bool inc = incremental() && (accu_.growTo(sizeVec(solver_)), true); sz != sizeVec(solver_); ++sz) {
			if (!inc) { solver_[sz] = &self_->ctx.solverStats(sz); }
			else      { (solver_[sz] = new SolverStats())->multi = (accu_[sz] = new SolverStats()); }
		}
		if (!incremental()) { solver_.release(); }
	}
}
void ClaspFacade::Statistics::end() {
	self_->ctx.accuStats(solvers_); // compute solvers = sum(solver[1], ... , solver[n])
	solvers_.flush();
	for (uint32 i = incremental() ? 0 : sizeVec(solver_), end = sizeVec(solver_); i != end && self_->ctx.hasSolver(i); ++i) {
		solver_[i]->accu(self_->ctx.solverStats(i), true);
		solver_[i]->flush();
	}
	if (tester_) { tester_->endStep(); }
	if (clingo_) { clingo_->update(*this); }
}
void ClaspFacade::Statistics::addTo(StatsMap& solving, StatsMap* accu) const {
	solvers_.addTo("solvers", solving, accu);
	if (solver_.size())       { solving.add("solver", solver_.toStats()); }
	if (accu && accu_.size()) { accu->add("solver", accu_.toStats()); }
}
void ClaspFacade::Statistics::accept(StatsVisitor& out, bool final) const {
	final = final && solvers_.multi;
	if (out.visitGenerator(StatsVisitor::Enter)) {
		out.visitSolverStats(final ? *solvers_.multi : solvers_);
		if (lp_.get()) { out.visitLogicProgramStats(*lp_); }
		out.visitProblemStats(self_->ctx.stats());
		const SolverVec& solver = final ? accu_ : solver_;
		const uint32 nThreads = final ? (uint32)accu_.size() : self_->ctx.concurrency();
		const uint32 nSolver  = (uint32)solver.size();
		if (nThreads > 1 && nSolver > 1 && out.visitThreads(StatsVisitor::Enter)) {
			for (uint32 i = 0, end = std::min(nSolver, nThreads); i != end; ++i) {
				out.visitThread(i, *solver[i]);
			}
			out.visitThreads(StatsVisitor::Leave);
		}
		out.visitGenerator(StatsVisitor::Leave);
	}
	if (tester_ && out.visitTester(StatsVisitor::Enter)) { 
		tester_->accept(out, final);
		out.visitTester(StatsVisitor::Leave);
	}
}
Potassco::AbstractStatistics* ClaspFacade::Statistics::getClingo() {
	if (!clingo_) {
		clingo_ = new ClingoView(*this->self_);
		clingo_->update(*this);
	}
	return clingo_;
}
ClaspFacade::Statistics::ClingoView::ClingoView(const ClaspFacade& f) {
	summary_.add("call"       , StatisticObject::value(&f.step_.step));
	summary_.add("result"     , StatisticObject::value<SolveResult, _getResult>(&f.step_.result));
	summary_.add("signal"     , StatisticObject::value<SolveResult, _getSignal>(&f.step_.result));
	summary_.add("exhausted"  , StatisticObject::value<SolveResult, _getExhausted>(&f.step_.result));
	summary_.add("costs"      , StatisticObject::array(&f.solve_->costs));
	summary_.add("concurrency", StatisticObject::value<SharedContext, _getConcurrency>(&f.ctx));
	summary_.step.bind(f.step_);
	summary_.step.addTo(summary_);
	if (f.step_.lpStats()) {
		problem_.add("lp", StatisticObject::map(f.step_.lpStats()));
		if (f.incremental()) { problem_.add("lpStep", StatisticObject::map(f.step_.lpStep())); }
	}
	problem_.add("generator", StatisticObject::map(&f.ctx.stats()));
	keys_.add("problem", problem_.toStats());
	keys_.add("solving", solving_.toStats());
	keys_.add("summary", summary_.toStats());
	if (f.incremental()) {
		accu_ = new Accu();
		accu_->step.bind(*f.accu_.get());
	}
	setRoot(keys_.toStats());
}
void ClaspFacade::Statistics::ClingoView::update(const ClaspFacade::Statistics& stats) {
	if (stats.level_ > 0 && accu_.get() && keys_.add("accu", accu_->toStats())) {
		accu_->step.addTo(*accu_);
		accu_->add("solving", accu_->solving_.toStats());
	}
	stats.addTo(solving_, stats.level_ > 0 && accu_.get() ? &accu_->solving_ : 0);
	if (stats.tester_) {
		stats.tester_->addTo(problem_, solving_, stats.level_ > 0 && accu_.get() ? &accu_->solving_ : 0);
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
// ClaspFacade
/////////////////////////////////////////////////////////////////////////////////////////
ClaspFacade::ClaspFacade() : config_(0) { step_.init(*this); }
ClaspFacade::~ClaspFacade() {}
bool ClaspFacade::prepared() const {
	return solve_.get() && solve_->prepared;
}
bool ClaspFacade::solving() const {
	return solve_.get() && solve_->solving();
}
bool ClaspFacade::solved() const {
	return step_.totalTime >= 0;
}
bool ClaspFacade::interrupted() const {
	return result().interrupted();
}
bool ClaspFacade::incremental() const {
	return accu_.get() != 0;
}
ProblemType ClaspFacade::detectProblemType(std::istream& str) {
	return Clasp::detectProblemType(str);
}
const ClaspFacade::Summary&  ClaspFacade::summary(bool accu) const {
	return accu && accu_.get() && accu_->step ? *accu_ : step_;
}

void ClaspFacade::discardProblem() {
	config_  = 0;
	builder_ = 0;
	stats_   = 0;
	solve_   = 0;
	accu_    = 0;
	step_.init(*this);
	if (ctx.frozen() || ctx.numVars()) { ctx.reset(); }
}
void ClaspFacade::init(ClaspConfig& config, bool discard) {
	if (discard) { discardProblem(); }
	ctx.setConfiguration(0, Ownership_t::Retain); // force reload of configuration once done
	config_ = &config;
	if (config_->solve.enumMode == EnumOptions::enum_dom_record) {
		if (config_->solver(0).heuId != Heuristic_t::Domain) {
			ctx.warn("Reasoning mode requires domain heuristic and is ignored!");
			config_->solve.enumMode = EnumOptions::enum_auto;
		}
		else if ((config_->solver(0).heuristic.domPref & HeuParams::pref_show) != 0) {
			ctx.setPreserveShown(true);
		}
	}
	SolveData::EnumPtr e(config.solve.createEnumerator(config.solve));
	if (e.get() == 0) { e = EnumOptions::nullEnumerator(); }
	if (config.solve.numSolver() > 1 && !e->supportsParallel()) {
		ctx.warn("Selected reasoning mode implies #Threads=1.");
		config.solve.setSolvers(1);
	}
	ctx.setConfiguration(&config, Ownership_t::Retain); // prepare and apply config
	if (program() && type_ == Problem_t::Asp) {
		Asp::LogicProgram* p = static_cast<Asp::LogicProgram*>(program());
		p->setOptions(config.asp);
		p->setNonHcfConfiguration(config.testerConfig());
	}
	if (!solve_.get()) { solve_ = new SolveData(); }
	SolveData::AlgoPtr a(config.solve.createSolveObject());
	solve_->init(a.release(), e.release());
	if (discard) { startStep(0); }
}

void ClaspFacade::initBuilder(ProgramBuilder* in) {
	builder_ = in;
	assume_.clear();
	builder_->startProgram(ctx);
}
ProgramBuilder& ClaspFacade::start(ClaspConfig& config, ProblemType t) {
	if      (t == Problem_t::Sat) { return startSat(config); }
	else if (t == Problem_t::Pb)  { return startPB(config);  }
	else if (t == Problem_t::Asp) { return startAsp(config); }
	else                          { throw std::domain_error("Unknown problem type!"); }
}

ProgramBuilder& ClaspFacade::start(ClaspConfig& config, std::istream& str) {
	ProgramParser& p = start(config, detectProblemType(str)).parser();
	CLASP_FAIL_IF(!p.accept(str, config_->parse), "Auto detection failed!");
	if (p.incremental()) { enableProgramUpdates(); }
	return *program();
}

SatBuilder& ClaspFacade::startSat(ClaspConfig& config) {
	init(config, true);
	initBuilder(new SatBuilder(config.solve.maxSat));
	type_ = Problem_t::Sat;
	return static_cast<SatBuilder&>(*builder_.get());
}

PBBuilder& ClaspFacade::startPB(ClaspConfig& config) {
	init(config, true);
	initBuilder(new PBBuilder());
	type_ = Problem_t::Sat;
	return static_cast<PBBuilder&>(*builder_.get());
}

Asp::LogicProgram& ClaspFacade::startAsp(ClaspConfig& config, bool enableUpdates) {
	init(config, true);
	Asp::LogicProgram* p = new Asp::LogicProgram();
	initBuilder(p);
	p->setOptions(config.asp);
	p->setNonHcfConfiguration(config.testerConfig());
	type_ = Problem_t::Asp;
	if (enableUpdates) { enableProgramUpdates(); }
	return *p;
}

bool ClaspFacade::enableProgramUpdates() {
	CLASP_ASSERT_CONTRACT_MSG(program(), "Program was already released!");
	CLASP_ASSERT_CONTRACT(!solving() && !program()->frozen());
	if (!accu_.get()) {
		builder_->updateProgram();
		ctx.setSolveMode(SharedContext::solve_multi);
		enableSolveInterrupts();
		accu_ = new Summary();
		accu_->init(*this);
		accu_->step = UINT32_MAX;
	}
	return isAsp(); // currently only ASP supports program updates
}
void ClaspFacade::enableSolveInterrupts() {
	CLASP_ASSERT_CONTRACT_MSG(!solving()  , "Solving is already active!");
	CLASP_ASSERT_CONTRACT_MSG(solve_.get(), "Active program required!");
	if (!solve_->interruptible) {
		solve_->interruptible = true;
		solve_->algo->enableInterrupts();
	}
}

void ClaspFacade::startStep(uint32 n) {
	step_.init(*this);
	step_.totalTime = -RealTime::getTime();
	step_.cpuTime   = -ProcessTime::getTime();
	step_.step      = n;
	if (!stats_.get()) { stats_ = new Statistics(*this); }
	ctx.report(StepStart(*this));
}

ClaspFacade::Result ClaspFacade::stopStep(int signal, bool complete) {
	if (!solved()) {
		double t = RealTime::getTime();
		step_.totalTime += t;
		step_.cpuTime   += ProcessTime::getTime();
		if (step_.solveTime) {
			step_.solveTime = t - step_.solveTime;
			step_.unsatTime = complete ? t - step_.unsatTime : 0;
		}
		Result res = {uint8(0), uint8(signal)};
		if (complete) { res.flags = uint8(step_.numEnum ? Result::SAT : Result::UNSAT) | Result::EXT_EXHAUST; }
		else          { res.flags = uint8(step_.numEnum ? Result::SAT : Result::UNKNOWN); }
		if (signal)   { res.flags|= uint8(Result::EXT_INTERRUPT); }
		step_.result = res;
		if (res.sat() && step_.model()->opt && !step_.numOptimal) {
			step_.numOptimal = 1;
		}
		updateStats();
		ctx.report(StepReady(step_)); 
	}
	return result();
}

void ClaspFacade::updateStats() {
	if (stats_.get()) {
		stats_->end();
	}
	if (accu_.get() && accu_->step != step_.step) {
		accu_->totalTime  += step_.totalTime;
		accu_->cpuTime    += step_.cpuTime;
		accu_->solveTime  += step_.solveTime;
		accu_->unsatTime  += step_.unsatTime;
		accu_->satTime    += step_.satTime;
		accu_->numEnum    += step_.numEnum;
		accu_->numOptimal += step_.numOptimal;
		// no aggregation
		accu_->step   = step_.step;
		accu_->result = step_.result;
	}
}

bool ClaspFacade::interrupt(int signal) {
	return solve_.get() && (signal || (signal = solve_->qSig.fetch_and_store(0)) != 0) && solve_->interrupt(signal);
}

const ClaspFacade::Summary& ClaspFacade::shutdown() {
	if (solve_.get()) {
		solve_->interrupt(SolveStrategy::SIGCANCEL);
		stopStep(solving() ? solve_->active->signal : solve_->qSig, !ok());
	}
	return summary(true);
}

bool ClaspFacade::read() {
	CLASP_ASSERT_CONTRACT(solve_.get());
	if (!program() || interrupted()) { return false; }
	ProgramParser& p = program()->parser();
	if (!p.isOpen() || (solved() && !update().ok())) { return false; }
	CLASP_FAIL_IF(!p.parse(), "Invalid input stream!");
	if (!p.more()) { p.reset(); }
	return true;
}
void ClaspFacade::prepare(EnumMode enumMode) {
	CLASP_ASSERT_CONTRACT(solve_.get() && !solving());
	EnumOptions& en = config_->solve;
	if (solved()) {
		doUpdate(0, false, SIG_DFL);
		solve_->prepareEnum(ctx, en.numModels, en.optMode, enumMode);
		ctx.endInit();
	}
	if (prepared()) { return; }
	SharedMinimizeData* m = 0;
	ProgramBuilder*   prg = program();
	if (prg && prg->endProgram()) {
		assume_.clear();
		prg->getAssumptions(assume_);
		prg->getWeakBounds(en.optBound);
	}
	stats_->start(uint32(config_->context().stats));
	if (en.optMode != MinimizeMode_t::ignore && (m = ctx.minimize()) != 0) {
		if (!m->setMode(en.optMode, en.optBound)) {
			assume_.push_back(lit_false());
		}
		if (en.optMode == MinimizeMode_t::enumerate && en.optBound.empty()) {
			ctx.warn("opt-mode=enum: no bound given, optimize statement ignored.");
		}
	}
	CLASP_ASSERT_CONTRACT(!ctx.ok() || !ctx.frozen());
	solve_->prepareEnum(ctx, en.numModels, en.optMode, enumMode);
	if      (!accu_.get()) { builder_ = 0; }
	else if (isAsp())      { static_cast<Asp::LogicProgram*>(builder_.get())->dispose(false); }
	if (!builder_.get() && !ctx.heuristic.empty()) {
		bool keepDom = false;
		for (uint32 i = 0; i != config_->solve.numSolver() && !keepDom; ++i) {
			keepDom = config_->solver(i).heuId == Heuristic_t::Domain;
		}
		if (!keepDom) { ctx.heuristic.reset(); }
	}
	if (ctx.ok()) { ctx.endInit(); }
}

ClaspFacade::Result ClaspFacade::solve(EventHandler* handler, const LitVec& a) {
	prepare();
	struct SyncSolve : public SolveStrategy {
		SyncSolve(SolveData& s) : x(&s) { x->active = this; }
		~SyncSolve()                    { x->active = 0;    }
		virtual void doSolve() { runAlgo(state_done); }
		SolveData* x;
	} syncSolve(*solve_);
	syncSolve.solve(*this, a, solve_->algo.get(), handler);
	return result();
}
#if WITH_THREADS
ClaspFacade::AsyncResult ClaspFacade::solveAsync(EventHandler* handler, const LitVec& a) {
	prepare();
	solve_->active = new AsyncSolve();
	solve_->active->solve(*this, a, solve_->algo.get(), handler);
	return AsyncResult(*solve_);
}
ClaspFacade::AsyncResult ClaspFacade::startSolveAsync(const LitVec& a) {
	return solveAsync(AsyncSolve::asyncModelHandler(), a);
}
#endif

ProgramBuilder& ClaspFacade::update(bool updateConfig, void (*sigAct)(int)) {
	CLASP_ASSERT_CONTRACT(config_ && program() && !solving());
	doUpdate(program(), updateConfig, sigAct);
	return *program();
}
ProgramBuilder& ClaspFacade::update(bool updateConfig) {
	return update(updateConfig, SIG_DFL);
}

void ClaspFacade::doUpdate(ProgramBuilder* p, bool updateConfig, void(*sigAct)(int)) {
	if (updateConfig) {
		init(*config_, false);
	}
	if (solved()) {
		startStep(step() + 1);
	}
	if (p && p->frozen()) {
		p->updateProgram();
	}
	if (ctx.frozen()) {
		ctx.unfreeze();
	}
	solve_->reset();
	int sig = sigAct == SIG_DFL ? 0 : solve_->qSig.fetch_and_store(0);
	if (sig && sigAct != SIG_IGN) { sigAct(sig); }
}

bool ClaspFacade::onModel(const Solver& s, const Model& m) {
	step_.unsatTime = RealTime::getTime();
	if (++step_.numEnum == 1) { step_.satTime = step_.unsatTime - step_.solveTime; }
	if (m.opt) { ++step_.numOptimal; }
	return solve_->update(s, m);
}
Enumerator* ClaspFacade::enumerator() const { return solve_.get() ? solve_->enumerator() : 0; }
Potassco::AbstractStatistics* ClaspFacade::getStats() const {
	CLASP_FAIL_IF(!config_ || !solved(), "statistics not (yet) available");
	return stats_->getClingo();
}
/////////////////////////////////////////////////////////////////////////////////////////
// ClaspFacade::Summary
/////////////////////////////////////////////////////////////////////////////////////////
void ClaspFacade::Summary::init(ClaspFacade& f)  { std::memset(this, 0, sizeof(Summary)); facade = &f;}
const Model* ClaspFacade::Summary::model() const { return facade->solve_.get() ? facade->solve_->lastModel() : 0; }
const SumVec* ClaspFacade::Summary::costs()const { return model() ? model()->costs : 0; }
uint64  ClaspFacade::Summary::optimal()    const { return facade->step_.numOptimal; }
bool ClaspFacade::Summary::optimize()      const {
	if (const Enumerator* e = facade->enumerator()){
		return e->optimize() || e->lastModel().opt;
	}
	return false;
}
const Asp::LpStats* ClaspFacade::Summary::lpStep() const {
	return facade->isAsp() ? &static_cast<const Asp::LogicProgram*>(facade->program())->stats : 0;
}
const Asp::LpStats* ClaspFacade::Summary::lpStats() const {
	return facade->stats_.get() ? facade->stats_->lp_.get() : lpStep();
}
const char* ClaspFacade::Summary::consequences() const {
	const Model* m = model();
	return m && m->consequences() ? modelType(*m) : 0;
}

bool ClaspFacade::Summary::hasLower() const {
	const SharedMinimizeData* m = optimize() ? facade->enumerator()->minimizer() : 0;
	return m && m->lower(0) != 0;
}
SumVec ClaspFacade::Summary::lower() const {
	if (hasLower()) {
		const SharedMinimizeData* m = facade->enumerator()->minimizer();
		SumVec ret(m->numRules());
		for (uint32 i = 0; i != m->numRules(); ++i) {
			ret[i] = m->lower(i) + m->adjust(i);
		}
		return ret;
	}
	return SumVec();
}
void ClaspFacade::Summary::accept(StatsVisitor& out) const {
	if (facade->solved()) { facade->stats_->accept(out, this == facade->accu_.get()); }
}

}

