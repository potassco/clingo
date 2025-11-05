#include <clingo/ground/instantiator.hh>

#include <clingo/util/checked_math.hh>
#include <clingo/util/print.hh>

#include <chrono>

namespace CppClingo::Ground {

namespace {

class ProfileTimer {
  public:
    explicit ProfileTimer(uint64_t *target) noexcept
        : target_{target}, start_{target_ != nullptr ? now() : TimePoint{}} {}

    ProfileTimer(const ProfileTimer &) = delete;
    auto operator=(const ProfileTimer &) -> ProfileTimer & = delete;

    ~ProfileTimer() {
        if (target_ != nullptr) {
            *target_ += diff(start_, now());
        }
    }

  private:
    using TimePoint = std::chrono::high_resolution_clock::time_point;

    static auto now() -> TimePoint { return std::chrono::high_resolution_clock::now(); }
    static auto diff(TimePoint start, TimePoint finish) -> uint64_t {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start).count();
    }

    uint64_t *target_;
    TimePoint start_;
};

} // namespace

void Instantiator::BackjumpMatcher::init(InstantiationContext const &ctx, size_t gen) {
    matcher_->init(ctx, gen);
}

void Instantiator::BackjumpMatcher::match(EvalContext const &ctx) {
    matcher_->match(ctx);
}

auto Instantiator::BackjumpMatcher::next(EvalContext const &ctx) -> bool {
    if (matcher_->next(ctx)) {
        backjumpable_ = true;
        return true;
    }
    return false;
}

auto Instantiator::BackjumpMatcher::first(EvalContext const &ctx) -> bool {
    matcher_->match(ctx);
    return next(ctx);
}

void Instantiator::BackjumpMatcher::print(std::ostream &out, size_t index) const {
    matcher_->print(out);
    out << " [" << matcher_->type() << "; " << index;
    if (!depend_.empty()) {
        out << ": " << Util::p_range(depend_);
    }
    out << "]";
}

auto Instantiator::BackjumpMatcher::depend() const -> DependVec const & {
    return depend_;
}

auto Instantiator::BackjumpMatcher::backjumpable() const -> bool {
    return backjumpable_;
}

void Instantiator::BackjumpMatcher::block() {
    backjumpable_ = false;
}

void Instantiator::add(UMatcher matcher, DependVec depend) {
    matchers_.emplace_back(std::move(matcher), std::move(depend));
}

void Instantiator::init(InstantiationContext const &ctx, size_t gen) {
    icb_->init(gen);
    for (auto &matcher : matchers_) {
        matcher.init(ctx, gen);
    }
}

void Instantiator::finalize(DependVec depend) {
    class SolutionMatcher : public Matcher {
      public:
        void do_init([[maybe_unused]] InstantiationContext const &ctx, [[maybe_unused]] size_t gen) override {};
        void do_match([[maybe_unused]] EvalContext const &ctx) override {}
        auto do_next([[maybe_unused]] EvalContext const &ctx) -> bool override { return false; }
        void do_print(std::ostream &out) const override { out << "#solution"; }
    };
    matchers_.emplace_back(std::make_unique<SolutionMatcher>(), std::move(depend));
}

auto Instantiator::enqueue() -> bool {
    bool old = enqueued_;
    enqueued_ = true;
    return old;
}

void Instantiator::print(std::ostream &out) const {
    icb_->print_head(out);
    out << " <- ";
    out << Util::p_range(matchers_, "; ", [index = size_t{0}](std::ostream &out, auto const &matcher) mutable {
        matcher.print(out, index);
        ++index;
    });
}

auto Instantiator::instantiate(Logger &log, SymbolStore &store, OutputStm &out, Util::StopFlag *stop) -> GroundResult {
    auto timer = ProfileTimer{stats_ != nullptr ? &stats_->time_instantiate : nullptr};
    enqueued_ = false;
    auto ie = matchers_.rend();
    auto it = ie - 1;
    auto ib = matchers_.rbegin();
    auto ctx = EvalContext{log, store, out, ass_};
    it->match(ctx);
    CLINGO_REPORT(log, trace) << "  instantiate: " << *this;
    do {
        if (stop != nullptr && stop->stop_requested()) {
            return GroundResult::interrupted;
        }
        CLINGO_REPORT(log, trace) << "    start at " << std::distance(it, ie) - 1;
        if (it->next(ctx)) {
            if (stats_ != nullptr) {
                ++stats_->matches;
            }
            for (--it; it->first(ctx); --it) {
                if (stats_ != nullptr) {
                    ++stats_->matches;
                }
            }
            CLINGO_REPORT(log, trace) << "    advanced to " << std::distance(it, ie) - 1;
        }
        if (it == ib) {
            CLINGO_REPORT(log, trace) << "    solution";
            if (stats_ != nullptr) {
                ++stats_->instances;
            }
            if (!icb_->report(ctx)) {
                return GroundResult::unsatisfiable;
            }
        }
        CLINGO_REPORT(log, trace) << "    block: " << Util::p_range(it->depend());
        for (auto idx : it->depend()) {
            matchers_[idx].block();
        }
        for (++it; it != ie && it->backjumpable(); ++it) {
        }
        CLINGO_REPORT(log, trace) << "    backjumped to " << std::distance(it, ie) - 1;
    } while (it != ie);
    return GroundResult::ok;
}

void Instantiator::propagate(SymbolStore &store, OutputStm &out, Queue &queue) {
    auto timer = ProfileTimer{stats_ != nullptr ? &stats_->time_propagate : nullptr};
    icb_->propagate(store, out, queue);
}

void Queue::insert(Instantiator inst, std::optional<size_t> index) {
    if (index) {
        if (update_.size() <= *index) {
            update_.resize(*index + 1);
        }
        update_[*index].emplace_back(insts_.size());
    }
    auto prio = inst.priority();
    if (prio < std::numeric_limits<size_t>::max()) {
        max_prio_ = std::max(max_prio_, prio + 1);
    }
    insts_.emplace_back(std::move(inst));
}

void Queue::enter_(size_t i) {
    auto &inst = insts_[i];
    if (!inst.enqueue()) {
        auto prio = inst.priority();
        if (prio == std::numeric_limits<size_t>::max()) {
            prio = max_prio_;
        }
        if (queues_.size() <= prio) {
            queues_.resize(prio + 1);
        }
        queues_[prio].emplace_back(&inst);
        ++size_;
    }
}

void Queue::propagate(size_t index) {
    if (index < update_.size()) {
        for (auto i : update_[index]) {
            enter_(i);
        }
    }
}

auto Queue::process(Logger &log, SymbolStore &store, OutputStm &out) -> GroundResult {
    // ground
    CLINGO_REPORT(log, debug) << "      instantiators: ";
    for (auto i = size_t{0}; i < insts_.size(); ++i) {
        enter_(i);
        CLINGO_REPORT(log, debug) << "        " << insts_[i];
    }
    auto current = std::vector<Instantiator *>{};
    for (auto gen = size_t{0}; size_ > 0; ++gen) {
        CLINGO_REPORT(log, trace) << "process generation: " << gen;
        for (auto &queue : queues_) {
            current.clear();
            current.swap(queue);
            size_ -= current.size();
            for (auto *inst : current) {
                inst->init(InstantiationContext{log, store, out}, gen);
            }
            for (auto *inst : current) {
                if (auto res = inst->instantiate(log, store, out, stop_); res != GroundResult::ok) {
                    return res;
                }
            }
            for (auto *inst : current) {
                inst->propagate(store, out, *this);
            }
        }
    }
    return GroundResult::ok;
}

} // namespace CppClingo::Ground
