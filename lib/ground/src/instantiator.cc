#include <gringo/ground/instantiator.hh>

#include <gringo/util/checked_math.hh>

// #include <iostream>

namespace Gringo::Ground {

void Instantiator::BackjumpMatcher::init(SymbolStore &store, size_t gen) { matcher_->init(store, gen); }

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

void Instantiator::init(SymbolStore &store, size_t gen) {
    icb_->init(gen);
    for (auto &matcher : matchers_) {
        matcher.init(store, gen);
    }
}

void Instantiator::finalize(DependVec depend) {
    class SolutionMatcher : public Matcher {
      public:
        void init([[maybe_unused]] SymbolStore &store, [[maybe_unused]] size_t gen) override {};
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
    auto ie = matchers_.rend();
    auto it = ie - 1;
    auto ib = matchers_.rbegin();
    it->match(store, ass_);
    do {
        // std::cerr << "***start at " << std::distance(it, ie) - 1 << "\n";
        if (it->next(store, ass_)) {
            for (--it; it->first(store, ass_); --it) {
            }
            // std::cerr << "***advanced to " << std::distance(it, ie) - 1 << "\n";
        }
        if (it == ib) {
            // std::cerr << "***solution\n";
            icb_->report(store, ass_);
        }
        // std::cerr << "***block";
        for (auto idx : it->depend()) {
            // std::cerr << " " << idx;
            matchers_[idx].block();
        }
        // std::cerr << "\n";
        for (++it; it != ie && it->backjumpable(); ++it) {
        }
        // std::cerr << "***backjumped to " << std::distance(it, ie) - 1 << "\n";
    } while (it != ie);
}

void Instantiator::propagate(Queue &queue) { icb_->propagate(queue); }

void Queue::insert(Instantiator inst, std::optional<size_t> index) {
    if (index) {
        if (update_.size() <= *index) {
            update_.resize(*index + 1);
        }
        update_[*index].emplace_back(insts_.size());
    }
    insts_.emplace_back(std::move(inst));
}

void Queue::enter_(size_t i) {
    auto &inst = insts_[i];
    if (!inst.enqueue()) {
        queues_.at(inst.priority()).emplace_back(&inst);
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

void Queue::process(SymbolStore &store) {
    // ground
    for (auto i = size_t{0}; i < insts_.size(); ++i) {
        enter_(i);
    }
    auto current = std::vector<Instantiator *>{};
    for (auto gen = size_t{0}; size_ > 0; ++gen) {
        for (auto &queue : queues_) {
            current.clear();
            current.swap(queue);
            size_ -= current.size();
            for (auto *inst : current) {
                inst->init(store, gen);
            }
            for (auto *inst : current) {
                inst->instantiate(store);
            }
            for (auto *inst : current) {
                inst->propagate(*this);
            }
        }
    }
}

} // namespace Gringo::Ground
