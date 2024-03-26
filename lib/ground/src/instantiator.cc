#include <gringo/ground/instantiator.hh>

namespace Gringo::Ground {

class SolutionMatcher : public Matcher {
  public:
    void match([[maybe_unused]] Assignment &ass) override {}
    auto next([[maybe_unused]] Assignment &ass) -> bool override { return false; }
};

void Instantiator::add(UMatcher matcher, DependVec depend) {
    binders_.emplace_back(std::move(matcher), std::move(depend));
}

void Instantiator::finalize(DependVec depend) {
    binders_.emplace_back(std::make_unique<SolutionMatcher>(), std::move(depend));
}

auto Instantiator::enqueue() -> bool {
    bool old = enqueued_;
    enqueued_ = true;
    return old;
}

void Instantiator::instantiate() {
    enqueued_ = false;
    auto ie = binders_.rend();
    auto it = ie - 1;
    auto ib = binders_.rbegin();
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
            binders_[idx].block();
        }
        for (++it; it != ie && it->backjumpable(); ++it) {
        }
    } while (it != ie);
}

void Queue::add(Instantiator &inst) { queue_.emplace_back(&inst); }

void Queue::process() {
    for (auto gen = size_t{0}; !queue_.empty(); ++gen) {
        auto n = queue_.size();
        std::stable_sort(queue_.begin(), queue_.end(),
                         [](auto const &a, auto const &b) { return a->priority() > b->priority(); });
        for (auto i = size_t{0}, j = size_t{0}; i < n; ++i) {
            auto &inst = queue_[i];
            inst->instantiate();
            if (i + 1 == n || inst->priority() != queue_[i + 1]->priority()) {
                for (; j <= i; ++j) {
                    queue_[j]->propagate(gen, *this);
                }
            }
        }
        queue_.erase(queue_.begin(), queue_.begin() + n);
    }
}

} // namespace Gringo::Ground
