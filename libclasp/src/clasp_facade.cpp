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
		ctx.warn(clasp_format_error("Oversubscription: #Threads=%u exceeds logical CPUs=%u.", numS, solve.recommendedSolvers()));
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
			int lim = en->init(ctx, opt, (int)Range<int64>(-1, INT_MAX).clamp(numM));
			if (lim == 0 || numM < 0) {
				numM = lim;
			}
			algo->setEnumLimit(numM ? static_cast<uint64>(numM) : UINT64_MAX);
			if (mode == enum_static) { ctx.addUnary(ctx.stepLiteral()); }
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
	lpStats_ = 0;
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
	if (program() && lpStats_.get()) {
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
	return static_cast<SatBuilder&>(*builder_.get());
}

PBBuilder& ClaspFacade::startPB(ClaspConfig& config) {
	init(config, true);
	initBuilder(new PBBuilder());
	return static_cast<PBBuilder&>(*builder_.get());
}

Asp::LogicProgram& ClaspFacade::startAsp(ClaspConfig& config, bool enableUpdates) {
	init(config, true);
	Asp::LogicProgram* p = new Asp::LogicProgram();
	lpStats_             = new Asp::LpStats;
	p->accu              = lpStats_.get();
	initBuilder(p);
	p->setOptions(config.asp);
	p->setNonHcfConfiguration(config.testerConfig());
	if (enableUpdates) { enableProgramUpdates(); }
	return *p;
}

bool ClaspFacade::enableProgramUpdates() {
	CLASP_ASSERT_CONTRACT_MSG(program(), "Program was already released!");
	CLASP_ASSERT_CONTRACT(!solving() && !program()->frozen());
	if (!accu_.get()) {
		builder_->updateProgram();
		ctx.requestStepVar();
		enableSolveInterrupts();
		accu_ = new Summary();
		accu_->init(*this);
		accu_->step = UINT32_MAX;
	}
	return lpStats_.get() != 0; // currently only ASP supports program updates
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
		if (complete) { res.flags = uint8(step_.enumerated() ? Result::SAT : Result::UNSAT) | Result::EXT_EXHAUST; }
		else          { res.flags = uint8(step_.enumerated() ? Result::SAT : Result::UNKNOWN); }
		if (signal)   { res.flags|= uint8(Result::EXT_INTERRUPT); }
		step_.result = res;
		accuStep();
		ctx.report(StepReady(step_)); 
	}
	return result();
}

void ClaspFacade::accuStep() {
	if (accu_.get() && accu_->step != step_.step){
		if (step_.stats()) { ctx.accuStats(); }
		accu_->totalTime += step_.totalTime;
		accu_->cpuTime   += step_.cpuTime;
		accu_->solveTime += step_.solveTime;
		accu_->unsatTime += step_.unsatTime;
		accu_->numEnum   += step_.numEnum;
		// no aggregation
		if (step_.numEnum) { accu_->satTime = step_.satTime; }
		accu_->step       = step_.step;
		accu_->result     = step_.result;
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
	if (!p.more() || (solved() && !update().ok())) { return false; }
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
	}
	if (prepared()) { return; }
	SharedMinimizeData* m = 0;
	ProgramBuilder*   prg = program();
	if (prg && prg->endProgram()) {
		assume_.clear();
		prg->getAssumptions(assume_);
		prg->getWeakBounds(en.optBound);
	}
	if (en.optMode != MinimizeMode_t::ignore && (m = ctx.minimize()) != 0) {
		if (!m->setMode(en.optMode, en.optBound)) {
			assume_.push_back(~ctx.stepLiteral());
		}
		if (en.optMode == MinimizeMode_t::enumerate && en.optBound.empty()) {
			ctx.warn("opt-mode=enum: no bound given, optimize statement ignored.");
		}
	}
	CLASP_ASSERT_CONTRACT(!ctx.ok() || !ctx.frozen());
	solve_->prepareEnum(ctx, en.numModels, en.optMode, enumMode);
	if      (!accu_.get())  { builder_ = 0; }
	else if (lpStats_.get()){ static_cast<Asp::LogicProgram*>(builder_.get())->dispose(false); }
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
	return solve_->update(s, m);
}
ExpectedQuantity::ExpectedQuantity(double d) : rep(d >= 0 ? d : -std::min(-d, 3.0)) {}
ExpectedQuantity::Error ExpectedQuantity::error() const {
	if (rep >= 0.0) { return error_none; }
	return static_cast<ExpectedQuantity::Error>(std::min(3, int(-rep)));
}
ExpectedQuantity::ExpectedQuantity(Error e) : rep(-double(int(e))) {}
ExpectedQuantity::operator double() const { return valid() ? rep : std::numeric_limits<double>::quiet_NaN(); }

ExpectedQuantity ClaspFacade::getStatImpl(const char* path, bool keys) const {
#define GET_KEYS(o, path) ( ExpectedQuantity((o).keys(path)) )
#define GET_OBJ(o, path)  ( ExpectedQuantity((o)[path]) )
#define COMMON_KEYS "ctx.\0solvers.\0solver.\0costs.\0time_total\0time_cpu\0time_solve\0time_unsat\0time_sat\0enumerated\0optimal\0step\0result\0"
	static const char* _keys[] = {"accu.\0hccs.\0hcc.\0lp.\0" COMMON_KEYS, 0, "accu.\0lp.\0" COMMON_KEYS, "accu.\0" COMMON_KEYS };
	enum ObjId { id_hccs, id_hcc, id_lp, id_ctx, id_solvers, id_solver, id_costs, id_total, id_cpu, id_solve, id_unsat, id_sat, id_num, id_opt, id_step, id_result };
	if (!path)  path = "";
	std::size_t kLen = 0;
	int         oId  = lpStats_.get() ? ((ctx.sccGraph.get() && ctx.sccGraph->numNonHcfs() != 0) ? id_hccs : id_lp) : id_ctx;
	const char* keyL = _keys[oId];
	bool        accu = step_.stats() != 0 && matchStatPath(path, "accu");
	if (!*path) { 
		if (accu || step_.stats() == 0) { keyL += 6; }
		return keys ? ExpectedQuantity(keyL) : ExpectedQuantity::error_ambiguous_quantity;
	}
	accu = accu && step() != 0;
	for (const char* k = keyL + 6; (kLen = std::strlen(k)) != 0; k += kLen + 1, ++oId) {
		bool match = k[kLen-1] == '.' ? matchStatPath(path, k, kLen-1) : std::strcmp(path, k) == 0;
		if (match) {
			if (!*path && !keys){ return ExpectedQuantity::error_ambiguous_quantity; }
			switch(oId) {
				default: {
					const Summary* stats = accu && accu_.get() ? accu_.get() : &step_;
					if (oId == id_costs) {
						char* x;
						uint32 n = (uint32)std::strtoul(path, &x, 10);
						uint32 N = stats->optimize() && stats->costs() ? (uint32)stats->costs()->size() : 0u;
						if (x == path) {
							if (!*path) { return keys ? ExpectedQuantity("__len\0") : ExpectedQuantity::error_ambiguous_quantity; }
							if (!keys && std::strcmp(path, "__len") == 0) { return ExpectedQuantity(N); }
							return ExpectedQuantity::error_unknown_quantity;
						}
						if (*(path = x) == '.') { ++path; }
						if (*path)  { return ExpectedQuantity::error_unknown_quantity; }
						if (n >= N) { return ExpectedQuantity::error_not_available;    }
						if (!keys)  { return ExpectedQuantity((double)stats->costs()->at(n)); }
					}
					if (keys)            { return ExpectedQuantity(uint32(0)); }
					if (oId <= id_sat)   { return *((&stats->totalTime) + (oId - id_total)); }
					if (oId == id_num)   { return ExpectedQuantity(stats->numEnum); }
					if (oId == id_opt)   { return ExpectedQuantity(stats->optimal()); }
					if (oId == id_step)  { return ExpectedQuantity(stats->step);    }
					if (oId == id_result){ return ExpectedQuantity((double)stats->result); }
					return ExpectedQuantity::error_unknown_quantity; }
				case id_lp: if (!lpStats_.get()) { return ExpectedQuantity::error_not_available; }
					return keys ? GET_KEYS((*lpStats_), path) : GET_OBJ((*lpStats_), path);
				case id_ctx:     return keys ? GET_KEYS(ctx.stats(), path) : GET_OBJ(ctx.stats(), path);
				case id_solvers: return keys ? GET_KEYS(ctx.stats(*ctx.solver(0), accu), path) : getStat(ctx, path, accu, Range32(0, ctx.concurrency()));			
				case id_hccs: if (!ctx.sccGraph.get() || ctx.sccGraph->numNonHcfs() == 0) { return ExpectedQuantity::error_not_available; } else {
					ExpectedQuantity res(0.0);
					for (PrgDepGraph::NonHcfIter it = ctx.sccGraph->nonHcfBegin(), end = ctx.sccGraph->nonHcfEnd(); it != end; ++it) {
						const SharedContext& hCtx= it->second->ctx();
						if (keys) { return GET_KEYS(hCtx.stats(*hCtx.solver(0), accu), path); }
						ExpectedQuantity hccAccu = getStat(hCtx, path, accu, Range32(0, hCtx.concurrency()));
						if (!hccAccu.valid()) { return hccAccu; }
						res.rep += hccAccu.rep;
					}
					return res; }
				case id_solver:
				case id_hcc: 
					Range32 r(0, oId == id_solver ? ctx.concurrency() : ctx.sccGraph.get() ? ctx.sccGraph->numNonHcfs() : 0);
					char* x;
					r.lo = (uint32)std::strtoul(path, &x, 10);
					if (x == path) {
						if (!*path) { return keys ? ExpectedQuantity("__len\0") : ExpectedQuantity::error_ambiguous_quantity; }
						if (!keys && std::strcmp(path, "__len") == 0) { return ExpectedQuantity(r.hi); }
						return ExpectedQuantity::error_unknown_quantity;
					}
					if (r.lo >= r.hi)  { return ExpectedQuantity::error_not_available;    }
					if (*(path = x) == '.') { ++path; }
					const SharedContext* active = &ctx; 
					if (oId == id_hcc) {
						active = &(ctx.sccGraph->nonHcfBegin() + r.lo)->second->ctx();
						r      = Range32(0, active->concurrency());
					}
					return keys ? GET_KEYS(active->stats(*active->solver(r.lo), accu), path) : getStat(*active, path, accu, r);
			}
		}
	}
	return ExpectedQuantity::error_unknown_quantity; 
#undef GET_KEYS
#undef GET_OBJ
#undef COMMON_KEYS
}

ExpectedQuantity ClaspFacade::getStat(const char* path)const {
	return config_ && step_.totalTime >= 0.0 ? getStatImpl(path, false) : ExpectedQuantity::error_not_available;
}
const char*      ClaspFacade::getKeys(const char* path)const {
	ExpectedQuantity x = config_ && step_.totalTime >= 0.0 ? getStatImpl(path, true) : ExpectedQuantity::error_not_available;
	if (x.valid()) { return (const char*)static_cast<uintp>(x.rep); }
	return x.error() == ExpectedQuantity::error_unknown_quantity ? 0 : "\0";
}
ExpectedQuantity ClaspFacade::getStat(const SharedContext& ctx, const char* key, bool accu, const Range<uint32>& r) const {
	if (!key || !*key) { return ExpectedQuantity::error_ambiguous_quantity; }
	ExpectedQuantity res(0.0);
	for (uint32 i = r.lo; i != r.hi && ctx.hasSolver(i); ++i) {
		ExpectedQuantity x = ctx.stats(*ctx.solver(i), accu)[key];
		if (!x.valid()) { return x; }
		res.rep += x.rep;
	}
	return res;	
}
Enumerator* ClaspFacade::enumerator() const { return solve_.get() ? solve_->enumerator() : 0; }
/////////////////////////////////////////////////////////////////////////////////////////
// ClaspFacade::Summary
/////////////////////////////////////////////////////////////////////////////////////////
void ClaspFacade::Summary::init(ClaspFacade& f)  { std::memset(this, 0, sizeof(Summary)); facade = &f;}
int ClaspFacade::Summary::stats()          const { return ctx().master()->stats.level(); }
const Model* ClaspFacade::Summary::model() const { return facade->solve_.get() ? facade->solve_->lastModel() : 0; }
bool ClaspFacade::Summary::optimize()      const {
	if (const Enumerator* e = facade->enumerator()){
		return e->optimize() || e->lastModel().opt;
	}
	return false;
}
const char* ClaspFacade::Summary::consequences() const {
	const Model* m = model();
	return m && m->consequences() ? modelType(*m) : 0;
}
uint64 ClaspFacade::Summary::optimal() const { 
	const Model* m = model();
	if (m && m->opt) {
		return !m->consequences() ? std::max(m->num, uint64(1)) : uint64(complete());
	}
	return 0;
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

}
