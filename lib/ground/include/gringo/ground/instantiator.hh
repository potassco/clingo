#pragma once

#include <gringo/core/symbol.hh>

#include <deque>
#include <memory>
#include <utility>
#include <vector>

namespace Gringo::Ground {

using Assignment = std::vector<std::optional<Symbol>>;

class Matcher {
  public:
    virtual ~Matcher() = default;
    virtual void match(Assignment &ass) = 0;
    virtual auto next(Assignment &ass) -> bool = 0;
};
using UMatcher = std::unique_ptr<Matcher>;

class Queue;

class InstanceCallback {
  public:
    virtual ~InstanceCallback() = default;
    //! Notify a statement that instantiation starts.
    virtual void init() = 0;
    //! Report an assignment giving rise to an instance for a statement.
    virtual void report(Assignment const &ass) = 0;
    //! Notify a statement that instantiation has finished.
    virtual void propagate(Queue &queue) = 0;
};

//! An instantiator implementinig the basic grounding algorithm.
class Instantiator {
  public:
    //! A vector of Matcher indices.
    using DependVec = std::vector<size_t>;
    //! Construct an instantiator with the given instance callback and number of variables.
    Instantiator(InstanceCallback &icb, size_t priority, size_t vars) : icb_{&icb}, ass_{vars}, priority_{priority} {};
    //! Finalize the instantiator.
    //!
    //! The depend vector must point to matchers that bind relevant variables for the matcher.
    //! For example, if the given matcher depends on variables X and Y,
    //! then we can backjump to the closest matcher that provides a binding for X or Y.
    void add(UMatcher matcher, DependVec depends);
    //! Finalize the instantiator.
    //!
    //! The depend vector must point to matchers that bind relevant variables.
    //! Relevant variables are variables in rule heads but also variables bound by non-domain predicates.
    void finalize(DependVec depends);
    //! Mark enqueued for instantiation.
    //!
    //! This returns true if the instantiator was previously not enqueued.
    //! The enqueued flag is rest when calling instantiate.
    [[nodiscard]] auto enqueue() -> bool;
    //! Find all assignments for the added matchers.
    //!
    //! Assignments are reported via the InstanceCallback.
    void instantiate();
    //! Add instantiators that need grounding to queue.
    void propagate(Queue &queue) { icb_->propagate(queue); }
    //! The priority of the instantiator.
    [[nodiscard]] auto priority() const { return priority_; }

  private:
    class BackjumpMatcher {
      public:
        BackjumpMatcher(UMatcher matcher, DependVec depend)
            : matcher_{std::move(matcher)}, depend_{std::move(depend)} {}
        void match(Assignment &ass) { matcher_->match(ass); }
        auto next(Assignment &ass) -> bool {
            if (matcher_->next(ass)) {
                backjumpable_ = true;
                return true;
            }
            return false;
        }
        auto first(Assignment &ass) -> bool {
            matcher_->match(ass);
            return next(ass);
        }
        [[nodiscard]] auto depend() const -> DependVec const & { return depend_; }
        [[nodiscard]] auto backjumpable() const -> bool { return backjumpable_; }
        void block() { backjumpable_ = false; }

      private:
        UMatcher matcher_;
        std::vector<size_t> depend_;
        bool backjumpable_ = true;
    };
    InstanceCallback *icb_;
    Assignment ass_;
    std::vector<BackjumpMatcher> binders_;
    size_t priority_;
    bool enqueued_ = false;
};

class Queue {
  public:
    void add(Instantiator &inst);
    void process();

  private:
    std::deque<Instantiator *> queue_;
};

} // namespace Gringo::Ground
