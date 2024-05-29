#pragma once

#include <gringo/core/logger.hh>
#include <gringo/core/output.hh>
#include <gringo/core/symbol.hh>

#include <memory>
#include <utility>
#include <vector>

namespace Gringo::Ground {

using Assignment = std::vector<std::optional<Symbol>>;

class InstantiationContext {
  public:
    InstantiationContext(Logger &log, OutputStm &out, SymbolStore &store, Assignment &ass)
        : log_{&log}, out_{&out}, store_{&store}, ass_{&ass} {}
    [[nodiscard]] auto log() const -> Logger & { return *log_; }
    [[nodiscard]] auto out() const -> OutputStm & { return *out_; }
    [[nodiscard]] auto store() const -> SymbolStore & { return *store_; }
    [[nodiscard]] auto ass() const -> Assignment & { return *ass_; }

  private:
    Logger *log_;
    OutputStm *out_;
    SymbolStore *store_;
    Assignment *ass_;
};

//! A matcher to match expressions.
class Matcher {
  public:
    //! Destroy the matcher.
    virtual ~Matcher() = default;
    //! Notify that instantiation starts.
    void init(SymbolStore &store, size_t gen) { do_init(store, gen); }
    //! Initialize matching.
    void match(InstantiationContext &ctx) { do_match(ctx); }
    //! Obtain the next match.
    //!
    //! Has to be called to obtain the first match.
    //! Returns true if there is a match.
    [[nodiscard]] auto next(InstantiationContext &ctx) -> bool { return do_next(ctx); }
    //! Print the matcher to the given stream.
    void print(std::ostream &out) const { do_print(out); }

  private:
    virtual void do_init(SymbolStore &store, size_t gen) = 0;
    virtual void do_match(InstantiationContext &ctx) = 0;
    virtual auto do_next(InstantiationContext &ctx) -> bool = 0;
    virtual void do_print(std::ostream &out) const = 0;
};
using UMatcher = std::unique_ptr<Matcher>;
using UMatcherVec = std::vector<UMatcher>;

class Queue;

//! Callbacks to notify statements during instantiations.
class InstanceCallback {
  public:
    //! Destroy the callback.
    virtual ~InstanceCallback() = default;
    //! Notify a statement that instantiation starts.
    void init(size_t gen) { do_init(gen); }
    //! Report an assignment giving rise to an instance for a statement.
    [[nodiscard]] auto report(InstantiationContext &ctx) -> bool { return do_report(ctx); }
    //! Notify a statement that instantiation has finished.
    void propagate(Queue &queue) { do_propagate(queue); }
    //! The priority of the callback.
    [[nodiscard]] auto priority() const -> size_t { return do_priority(); }
    //! Print representation for debugging.
    void print_head(std::ostream &out) const { do_print_head(out); }

  private:
    //! Notify a statement that instantiation starts.
    virtual void do_init(size_t gen) = 0;
    //! Report an assignment giving rise to an instance for a statement.
    [[nodiscard]] virtual auto do_report(InstantiationContext &ctx) -> bool = 0;
    //! Notify a statement that instantiation has finished.
    virtual void do_propagate(Queue &queue) = 0;
    //! The priority of the callback.
    [[nodiscard]] virtual auto do_priority() const -> size_t = 0;
    //! Print representation for debugging.
    virtual void do_print_head(std::ostream &out) const = 0;
};

//! An instantiator implementinig the basic grounding algorithm.
class Instantiator {
  public:
    //! A vector of Matcher indices.
    using DependVec = std::vector<size_t>;
    //! Construct an instantiator with the given instance callback and number of variables.
    Instantiator(InstanceCallback &icb, size_t vars, size_t n) : icb_{&icb}, ass_{vars} { matchers_.reserve(n + 1); }
    //! Prepare the instantiator for the first grounding step (with generation 0).
    //!
    //! This resets all involved domains.
    void init(SymbolStore &store, size_t gen);
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
    [[nodiscard]] auto instantiate(Logger &log, SymbolStore &store, OutputStm &out) -> bool;
    //! Add instantiators that need grounding to queue.
    void propagate(Queue &queue);
    //! The priority of the instantiator.
    [[nodiscard]] auto priority() const { return icb_->priority(); }

  private:
    class BackjumpMatcher {
      public:
        BackjumpMatcher(UMatcher matcher, DependVec depend)
            : matcher_{std::move(matcher)}, depend_{std::move(depend)} {}
        void init(SymbolStore &store, size_t gen);
        void match(InstantiationContext &ctx);
        auto next(InstantiationContext &ctx) -> bool;
        auto first(InstantiationContext &ctx) -> bool;
        void print(std::ostream &out, size_t index) const;
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
    std::vector<BackjumpMatcher> matchers_;
    bool enqueued_ = false;
};

using InstantiatorVec = std::vector<Instantiator>;

//! A queue to proccess instantiators.
class Queue {
  public:
    Queue() = default;
    //! Register an instantiator with the queue.
    void insert(Instantiator inst, std::optional<size_t> index);
    void propagate(size_t index);
    //! Process previously enqueued instantiators.
    [[nodiscard]] auto process(Logger &log, SymbolStore &store, OutputStm &out) -> bool;
    //! Release the contained instantiators.
    auto release() -> std::vector<Instantiator> { return std::move(insts_); }

  private:
    //! Append an instantiator to the queue.
    void enter_(size_t i);

    std::vector<Instantiator> insts_;
    std::vector<std::vector<size_t>> update_;
    std::vector<std::vector<Instantiator *>> queues_;
    size_t size_ = 0;
    size_t max_prio_ = 0;
};

} // namespace Gringo::Ground
