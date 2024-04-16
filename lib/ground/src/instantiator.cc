#include <gringo/ground/instantiator.hh>

#include <gringo/util/checked_math.hh>

namespace Gringo::Ground {

void Instantiator::BackjumpMatcher::match(SymbolStore &store, Assignment &ass) { matcher_->match(store, ass); }

auto Instantiator::BackjumpMatcher::next(SymbolStore &store, Assignment &ass) -> bool {
    if (matcher_->next(store, ass)) {
        backjumpable_ = true;
        return true;
    }
    return false;
}

auto Instantiator::BackjumpMatcher::first(SymbolStore &store, Assignment &ass) -> bool {
    matcher_->match(store, ass);
    return next(store, ass);
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
        void match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) override {}
        auto next([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) -> bool override {
            return false;
        }
    };
    matchers_.emplace_back(std::make_unique<SolutionMatcher>(), std::move(depend));
}

auto Instantiator::enqueue() -> bool {
    bool old = enqueued_;
    enqueued_ = true;
    return old;
}

void Instantiator::instantiate(SymbolStore &store) {
    enqueued_ = false;
    icb_->init();
    auto ie = matchers_.rend();
    auto it = ie - 1;
    auto ib = matchers_.rbegin();
    it->match(store, ass_);
    do {
        if (it->next(store, ass_)) {
            for (--it; it->first(store, ass_); --it) {
            }
        }
        if (it == ib) {
            icb_->report(store, ass_);
        }
        for (auto idx : it->depend()) {
            matchers_[idx].block();
        }
        for (++it; it != ie && it->backjumpable(); ++it) {
        }
    } while (it != ie);
}

void Queue::add(Instantiator &inst) { queue_.emplace_back(&inst); }

void Queue::process(SymbolStore &store) {
    while (!queue_.empty()) {
        auto n = std::ssize(queue_);
        std::stable_sort(queue_.begin(), queue_.end(),
                         [](auto const &a, auto const &b) { return a->priority() > b->priority(); });
        for (auto i = ssize_t{0}, j = ssize_t{0}; i < n; ++i) {
            auto &inst = queue_[i];
            inst->instantiate(store);
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
