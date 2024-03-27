#include <gringo/ground/instantiator.hh>

namespace Gringo::Ground {

void Instantiator::BackjumpMatcher::match(Assignment &ass) { matcher_->match(ass); }

auto Instantiator::BackjumpMatcher::next(Assignment &ass) -> bool {
    if (matcher_->next(ass)) {
        backjumpable_ = true;
        return true;
    }
    return false;
}

auto Instantiator::BackjumpMatcher::first(Assignment &ass) -> bool {
    matcher_->match(ass);
    return next(ass);
}

auto Instantiator::BackjumpMatcher::depend() const -> DependVec const & { return depend_; }

auto Instantiator::BackjumpMatcher::backjumpable() const -> bool { return backjumpable_; }

void Instantiator::BackjumpMatcher::block() { backjumpable_ = false; }

void Instantiator::add(UMatcher matcher, DependVec depend) {
    matchers_.emplace_back(std::move(matcher), std::move(depend));
}

void Instantiator::finalize(DependVec depend) {
    class SolutionMatcher : public Matcher {
      public:
        void match([[maybe_unused]] Assignment &ass) override {}
        auto next([[maybe_unused]] Assignment &ass) -> bool override { return false; }
    };
    matchers_.emplace_back(std::make_unique<SolutionMatcher>(), std::move(depend));
}

auto Instantiator::enqueue() -> bool {
    bool old = enqueued_;
    enqueued_ = true;
    return old;
}

void Instantiator::instantiate() {
    enqueued_ = false;
    icb_->init();
    auto ie = matchers_.rend();
    auto it = ie - 1;
    auto ib = matchers_.rbegin();
    it->match(ass_);
    do {
        if (it->next(ass_)) {
            for (--it; it->first(ass_); --it) {
            }
        }
        if (it == ib) {
            icb_->report(ass_);
        }
        for (auto idx : it->depend()) {
            matchers_[idx].block();
        }
        for (++it; it != ie && it->backjumpable(); ++it) {
        }
    } while (it != ie);
}

void Queue::add(Instantiator &inst) { queue_.emplace_back(&inst); }

void Queue::process() {
    while (!queue_.empty()) {
        auto n = queue_.size();
        std::stable_sort(queue_.begin(), queue_.end(),
                         [](auto const &a, auto const &b) { return a->priority() > b->priority(); });
        for (auto i = size_t{0}, j = size_t{0}; i < n; ++i) {
            auto &inst = queue_[i];
            inst->instantiate();
            if (i + 1 == n || inst->priority() != queue_[i + 1]->priority()) {
                for (; j <= i; ++j) {
                    // Note: previous gringo versions enqueued the domain for update.
                    // Currently, the update is planned to happen with a generation counter
                    queue_[j]->propagate(*this);
                }
            }
        }
        queue_.erase(queue_.begin(), queue_.begin() + n);
    }
}

} // namespace Gringo::Ground
