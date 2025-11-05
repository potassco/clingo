#pragma once

#include <clingo/ground/profile.hh>

#include <clingo/core/logger.hh>
#include <clingo/core/output.hh>
#include <clingo/core/symbol.hh>

#include <clingo/util/thread.hh>

#include <memory>
#include <utility>
#include <vector>

namespace CppClingo::Ground {

//! @addtogroup ground_instantiator
//! @{

class Instantiator;

//! Context object to capture state used during instantiation.
class InstantiationContext {
  public:
    //! Construct an instantiation state.
    InstantiationContext(Logger &log, SymbolStore &store, OutputStm &out) : log_{&log}, store_{&store}, out_{&out} {}
    //! Get the logger.
    [[nodiscard]] auto log() const -> Logger & { return *log_; }
    //! Get the store.
    [[nodiscard]] auto store() const -> SymbolStore & { return *store_; }
    //! Get the output.
    [[nodiscard]] auto out() const -> OutputStm & { return *out_; }

  private:
    Logger *log_;
    SymbolStore *store_;
    OutputStm *out_;
};

//! Context object to capture state used during instantiation.
class EvalContext : public InstantiationContext {
  public:
    //! Construct an instantiation state.
    EvalContext(Logger &log, SymbolStore &store, OutputStm &out, Assignment &ass)
        : InstantiationContext{log, store, out}, ass_{&ass} {}
    //! Get the assignment.
    [[nodiscard]] auto ass() const -> Assignment & { return *ass_; }

  private:
    Assignment *ass_;
};

//! Enumeration of matcher types.
enum class MatcherType : uint8_t {
    new_atoms, //! Indicates a matcher for freshly added atoms.
    old_atoms, //! Indicates a matcher for previously added atoms.
    all_atoms  //! Indicates a matcher for all atoms.
};

//! Print a short indicator for the matcher type.
inline auto operator<<(std::ostream &out, MatcherType type) -> std::ostream & {
    switch (type) {
        case MatcherType::all_atoms: {
            out << "a";
            break;
        }
        case MatcherType::old_atoms: {
            out << "o";
            break;
        }
        case MatcherType::new_atoms: {
            out << "n";
            break;
        }
    }
    return out;
}

//! A matcher to match expressions.
class Matcher {
  public:
    //! Destroy the matcher.
    virtual ~Matcher() = default;
    //! Notify that instantiation starts.
    void init(InstantiationContext const &ctx, size_t gen) { do_init(ctx, gen); }
    //! Initialize matching.
    void match(EvalContext const &ctx) { do_match(ctx); }
    //! Obtain the next match.
    //!
    //! Has to be called to obtain the first match.
    //! Returns true if there is a match.
    [[nodiscard]] auto next(EvalContext const &ctx) -> bool { return do_next(ctx); }
    //! Print the matcher to the given stream.
    void print(std::ostream &out) const { do_print(out); }
    //! Get the type of the matcher.
    [[nodiscard]] auto type() const -> MatcherType { return do_type(); }

  private:
    virtual void do_init(InstantiationContext const &ctx, size_t gen) = 0;
    virtual void do_match(EvalContext const &ctx) = 0;
    [[nodiscard]] virtual auto do_next(EvalContext const &ctx) -> bool = 0;
    virtual void do_print(std::ostream &out) const = 0;
    [[nodiscard]] virtual auto do_type() const -> MatcherType { return MatcherType::all_atoms; }
};
//! A unique pointer to a matcher.
using UMatcher = std::unique_ptr<Matcher>;
//! A vector of matchers.
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
    [[nodiscard]] auto report(EvalContext const &ctx) -> bool { return do_report(ctx); }
    //! Notify a statement that instantiation has finished.
    void propagate(SymbolStore &store, OutputStm &out, Queue &queue) { do_propagate(store, out, queue); }
    //! The priority of the callback.
    [[nodiscard]] auto priority() const -> size_t { return do_priority(); }
    //! Print representation for debugging.
    void print_head(std::ostream &out) const { do_print_head(out); }
    //! Check if the literal with the given index is important.
    [[nodiscard]] auto is_important(size_t index) const -> bool { return do_is_important(index); }
    //! Get a node to store profiling data for this statement.
    //!
    //! The returned pointer might be null if profiling is not enabled.
    [[nodiscard]] auto profile_node() const -> ProfileNodeInternal * { return do_profile_node(); }

  private:
    virtual void do_init(size_t gen) = 0;
    [[nodiscard]] virtual auto do_report(EvalContext const &ctx) -> bool = 0;
    virtual void do_propagate(SymbolStore &store, OutputStm &out, Queue &queue) = 0;
    [[nodiscard]] virtual auto do_priority() const -> size_t = 0;
    virtual void do_print_head(std::ostream &out) const = 0;
    [[nodiscard]] virtual auto do_is_important([[maybe_unused]] size_t index) const -> bool { return true; }
    [[nodiscard]] virtual auto do_profile_node() const -> ProfileNodeInternal * = 0;
};

//! An instantiator implementing the basic grounding algorithm.
class Instantiator {
  public:
    //! A vector of Matcher indices.
    using DependVec = std::vector<size_t>;
    //! Construct an instantiator with the given instance callback and number of variables.
    Instantiator(InstanceCallback &icb, size_t vars, size_t n) : icb_{&icb}, stats_{profile_node_(icb)}, ass_{vars} {
        matchers_.reserve(n + 1);
    }
    //! Prepare the instantiator for the first grounding step (with generation 0).
    //!
    //! This resets all involved domains.
    void init(InstantiationContext const &ctx, size_t gen);
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
    [[nodiscard]] auto instantiate(Logger &log, SymbolStore &store, OutputStm &out, Util::StopFlag *stop)
        -> GroundResult;
    //! Add instantiators that need grounding to queue.
    void propagate(SymbolStore &store, OutputStm &out, Queue &queue);
    //! Print the instantiator.
    void print(std::ostream &out) const;
    //! The priority of the instantiator.
    [[nodiscard]] auto priority() const { return icb_->priority(); }

    //! Print the instantiator.
    friend auto operator<<(std::ostream &out, Instantiator const &inst) -> std::ostream & {
        inst.print(out);
        return out;
    }

  private:
    static auto profile_node_(InstanceCallback const &icb) -> ProfileStats * {
        auto *node = icb.profile_node();
        return node != nullptr ? &node->add_child(std::make_unique<ProfileData>()).step_ : nullptr;
    }
    class BackjumpMatcher {
      public:
        BackjumpMatcher(UMatcher matcher, DependVec depend)
            : matcher_{std::move(matcher)}, depend_{std::move(depend)} {}
        void init(InstantiationContext const &ctx, size_t gen);
        void match(EvalContext const &ctx);
        auto next(EvalContext const &ctx) -> bool;
        auto first(EvalContext const &ctx) -> bool;
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
    ProfileStats *stats_;
    Assignment ass_;
    std::vector<BackjumpMatcher> matchers_;
    bool enqueued_ = false;
};

//! A vector of instantiators.
using InstantiatorVec = std::vector<Instantiator>;

//! A queue to process instantiators.
class Queue {
  public:
    //! Construct an empty queue.
    Queue(Util::StopFlag *stop = nullptr) : stop_{stop} {}
    //! Register an instantiator with the queue.
    void insert(Instantiator inst, std::optional<size_t> index);
    //! Propagate instantiators with the given index.
    void propagate(size_t index);
    //! Process previously enqueued instantiators.
    [[nodiscard]] auto process(Logger &log, SymbolStore &store, OutputStm &out) -> GroundResult;
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
    Util::StopFlag *stop_;
};

//! @}

} // namespace CppClingo::Ground
