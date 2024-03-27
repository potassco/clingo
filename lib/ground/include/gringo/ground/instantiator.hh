#pragma once

#include <gringo/core/symbol.hh>

#include <deque>
#include <memory>
#include <utility>
#include <vector>

namespace Gringo::Ground {

using Assignment = std::vector<std::optional<Symbol>>;

//! A matcher to match expressions.
class Matcher {
  public:
    //! Destroy the matcher.
    virtual ~Matcher() = default;
    //! Initialize matching.
    virtual void match(Assignment &ass) = 0;
    //! Obtain the next match.
    //!
    //! Has to be called to obtain the first match.
    //! Returns true if there is a match.
    virtual auto next(Assignment &ass) -> bool = 0;
};
using UMatcher = std::unique_ptr<Matcher>;

class Queue;

//! Callbacks to notify statements during instantiations.
class InstanceCallback {
  public:
    //! Destroy the callback.
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
        void match(Assignment &ass);
        auto next(Assignment &ass) -> bool;
        auto first(Assignment &ass) -> bool;
        [[nodiscard]] auto depend() const -> DependVec const &;
        [[nodiscard]] auto backjumpable() const -> bool;
        void block();

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

using InstantiatorVec = std::vector<Instantiator>;

//! A queue to proccess instantiators.
class Queue {
  public:
    //! Add an instantiator to the queue.
    void add(Instantiator &inst);
    //! Process previously enqueued instantiators.
    void process();

  private:
    std::deque<Instantiator *> queue_;
};

} // namespace Gringo::Ground
