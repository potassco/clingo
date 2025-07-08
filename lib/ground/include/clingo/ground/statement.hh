#pragma once

#include <clingo/ground/literal.hh>

#include <clingo/ground/instantiator.hh>

#include <clingo/core/core.hh>

#include <memory_resource>

namespace CppClingo::Ground {

//! @addtogroup ground_stm
//! @{

//! Helper to print indentation.
struct ProfileIndent {
  public:
    //! Construct the indent with the given amount.
    explicit ProfileIndent(size_t level, size_t width) : level{level}, width{width} {}

    //! Print the given indentation to the given output stream.
    friend auto operator<<(std::ostream &out, ProfileIndent const &indent) -> std::ostream & {
        for (size_t i = 0; i < indent.level * indent.width; ++i) {
            out << ' ';
        }
        return out;
    }

    //! Add indentation to the given indent.
    friend auto operator+(ProfileIndent const &indent, size_t add) -> ProfileIndent {
        return ProfileIndent{indent.level + add, indent.width};
    }

    //! Add indentation to the current indent (inplace).
    friend auto operator+=(ProfileIndent &indent, size_t add) -> ProfileIndent & {
        indent.level += add;
        return indent;
    }

  private:
    //! The indentation level.
    size_t level;
    //! The width of the indentation.
    size_t width;
};

//! Base class for profiling data.
class ProfileNode {
  public:
    //! Destructor.
    virtual ~ProfileNode() = default;

    //! Print the profiling data to the given output stream.
    //!
    //! The data is indented by the given amount. Nested profiling data is
    //! printed with increased indentation.
    void print(std::ostream &out, ProfileIndent indent) const { do_print(out, indent); }

  private:
    //! Print the profiling data to the given output stream.
    virtual void do_print(std::ostream &out, ProfileIndent indent) const = 0;
};

//! Profile node that can hold children.
class ProfileNodeInternal : public ProfileNode {
  public:
    //! Add a child profile node.
    //!
    //! Returns a reference to the child node that was added.
    template <class T> auto add_child(std::unique_ptr<T> child) -> T & {
        assert(child != nullptr);
        auto *ret = child.get();
        do_add_child(std::move(child));
        return *ret;
    }

  private:
    //! Add a child profile node.
    virtual void do_add_child(std::unique_ptr<ProfileNode> child) = 0;
};

//! A profile node that holds a printable expression and children.
template <typename T> class ProfileNodeExpression : public ProfileNodeInternal {
  private:
    //! Add a child profile node.
    void do_add_child(std::unique_ptr<ProfileNode> child) override { children_.emplace_back(std::move(child)); }

    //! Print the profiling data to the given output stream.
    void do_print(std::ostream &out, ProfileIndent indent) const override {
        out << indent << expr_ << "\n";
        for (auto const &child : children_) {
            child->print(out, indent + 1);
        }
    }

    //! The expression to print.
    T expr_;
    //! The children of this profile node.
    std::vector<std::unique_ptr<ProfileNode>> children_;
};

//! Base class for groundable statements.
class Stm : public InstanceCallback {
  public:
    //! Get the body of the statement.
    [[nodiscard]] auto body() const -> ULitVec const & { return do_body(); }
    //! Get the important variables in the statement.
    //!
    //! This comprises all variables that have to be bound to provide relevant
    //! instances. Relevant important variables from the body are gathered
    //! separately. For example, normal rules should provide the variables in
    //! the rule head, and integrity constraint can return an empty set.
    [[nodiscard]] auto important() const -> VariableSet { return do_important(); }

    //! Print the statement.
    friend auto operator<<(std::ostream &out, Stm const &stm) -> std::ostream & {
        stm.do_print(out);
        return out;
    }

  private:
    virtual void do_print(std::ostream &out) const = 0;
    [[nodiscard]] virtual auto do_body() const -> ULitVec const & = 0;
    [[nodiscard]] virtual auto do_important() const -> VariableSet = 0;
};

//! A unique pointer holding a statement.
using UStm = std::unique_ptr<Stm>;
//! A vector of statements.
using UStmVec = std::vector<UStm>;

//! Helper class to prepare statements for grounding.
class Linearizer {
  public:
    //! Construct the linearizer.
    Linearizer(std::pmr::monotonic_buffer_resource &mbr) : mbr_{&mbr} {}
    //! Indicate that a new domain is being prepared.
    void start(Queue &queue);
    //! Prepare a statement for grounding.
    void prepare(InstanceCallback &cb, ULitVec const &body, VariableSet important);

  private:
    //! Build the dependency graph among literals and variables.
    void build_(ULitVec const &lits);
    //! Create matchers for literals ordering them heuristically.
    auto order_(InstanceCallback &cb, std::vector<MatcherType> const &todo, VariableSet const &important,
                ULitVec const &lits) -> std::pair<Instantiator, std::optional<size_t>>;

    Queue *iqueue_ = nullptr;
    std::pmr::monotonic_buffer_resource *mbr_;
    std::vector<size_t> rec_;
    std::vector<std::vector<MatcherType>> todos_;
    std::vector<std::tuple<size_t, size_t, double>> queue_;
    //! A map from literal indices to provided variables.
    std::vector<std::tuple<size_t, std::vector<size_t>, std::vector<size_t>>> lit_map_;
    //! A map from variables to provided literals.
    std::vector<std::vector<size_t>> var_map_;
};

//! Enumeration of available rule types.
enum class RuleType : uint8_t {
    normal, //!< A normal rule.
    choice  //!< A choice rule.
};

//! A statement capturing normal rules and integrity constraints.
class StmRule : public Stm {
  public:
    //! Construct the statement.
    StmRule(AtomSimple head, ULitVec body, RuleType type)
        : head_{head ? std::move(std::get<0>(*head)) : nullptr}, base_{head ? &std::get<1>(*head) : nullptr},
          indices_{head ? std::move(std::get<2>(*head)) : std::vector<size_t>{}}, body_{std::move(body)}, type_{type} {
        init_();
    }

  private:
    void init_();

    // Stm interface
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;

    // InstanceCallback interface
    void do_print_head(std::ostream &out) const override;
    void do_init(size_t gen) override;
    [[nodiscard]] auto do_report(EvalContext const &ctx) -> bool override;
    void do_propagate(SymbolStore &store, OutputStm &out, Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override { return std::numeric_limits<size_t>::max(); }

    //! The head of the rule.
    //!
    //! Note that this unique pointer is zero in case of constraints.
    UTerm head_;
    AtomBase *base_;
    //! A list of indices.
    //!
    //! Recursive body literals that unify with the rule head have matching indices.
    //! This allows for updating the indices of these literals while grounding.
    std::vector<size_t> indices_;
    ULitVec body_;
    Symbol atom_ = SymbolStore::sup();
    RuleType type_;
};

//! A statement capturing normal rules and integrity constraints.
class StmExternal : public Stm {
  public:
    //! Construct the statement.
    StmExternal(Ground::UTerm atom, AtomBase &base, std::vector<size_t> indices, ULitVec body,
                std::optional<std::pair<Location, UTerm>> type)
        : type_{std::move(type)}, atom_{std::move(atom)}, base_{&base}, indices_{std::move(indices)},
          body_{std::move(body)} {
        init_();
    }

  private:
    void init_();

    // Stm interface
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;

    // InstanceCallback interface
    void do_print_head(std::ostream &out) const override;
    void do_init(size_t gen) override;
    [[nodiscard]] auto do_report(EvalContext const &ctx) -> bool override;
    void do_propagate(SymbolStore &store, OutputStm &out, Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override { return std::numeric_limits<size_t>::max(); }
    [[nodiscard]] auto do_is_important([[maybe_unused]] size_t index) const -> bool override { return false; }

    std::optional<std::pair<Location, UTerm>> type_;
    //! The head of the rule.
    //!
    //! Note that this unique pointer is zero in case of constraints.
    UTerm atom_;
    AtomBase *base_;
    //! A list of indices.
    //!
    //! Recursive body literals that unify with the rule head have matching indices.
    //! This allows for updating the indices of these literals while grounding.
    std::vector<size_t> indices_;
    ULitVec body_;
    Symbol res_atom_ = SymbolStore::sup();
    ExternalType res_type_ = ExternalType::free;
};

//! A statement capturing weak constraints.
class StmWeakConstraint : public Stm {
  public:
    //! Construct the statement.
    StmWeakConstraint(Location loc_weight, UTerm weight, std::optional<std::pair<Location, UTerm>> prio, UTermVec terms,
                      ULitVec body)
        : loc_weight_{std::move(loc_weight)}, loc_prio_{prio ? std::move(prio->first) : loc_weight_},
          weight_{std::move(weight)}, prio_{prio ? std::move(prio->second) : nullptr}, terms_{std::move(terms)},
          body_{std::move(body)} {
        init_();
    }

  private:
    void init_();
    // Stm interface
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;

    // InstanceCallback interface
    void do_print_head(std::ostream &out) const override;
    void do_init(size_t gen) override;
    [[nodiscard]] auto do_report(EvalContext const &ctx) -> bool override;
    void do_propagate(SymbolStore &store, OutputStm &out, Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override { return std::numeric_limits<size_t>::max(); }

    Location loc_weight_;
    Location loc_prio_;
    UTerm weight_;
    UTerm prio_;
    UTermVec terms_;
    ULitVec body_;
    Symbol res_weight_;
    std::optional<Symbol> res_prio_;
    SymbolVec res_terms_;
};

//! Statement capturing heuristic directives.
class StmHeuristic : public Stm {
  public:
    //! Construct the statement.
    StmHeuristic(UTerm atom, AtomBase &base, ULitVec body, Location loc_weight, UTerm weight,
                 std::optional<std::pair<Location, UTerm>> prio, Location loc_type, UTerm type)
        : loc_weight_{std::move(loc_weight)}, loc_prio_{prio ? std::move(prio->first) : loc_weight_},
          loc_type_{std::move(loc_type)}, atom_{std::move(atom)}, base_{&base}, weight_{std::move(weight)},
          prio_{prio ? std::move(prio->second) : nullptr}, type_{std::move(type)}, body_{std::move(body)} {
        init_();
    }

  private:
    void init_();
    // Stm interface
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;

    // InstanceCallback interface
    void do_print_head(std::ostream &out) const override;
    void do_init(size_t gen) override;
    [[nodiscard]] auto do_report(EvalContext const &ctx) -> bool override;
    void do_propagate(SymbolStore &store, OutputStm &out, Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override { return std::numeric_limits<size_t>::max(); }

    Location loc_weight_;
    Location loc_prio_;
    Location loc_type_;
    UTerm atom_;
    AtomBase *base_;
    UTerm weight_;
    UTerm prio_;
    UTerm type_;
    ULitVec body_;
    size_t offset_ = 0;
    Symbol res_weight_;
    Symbol res_prio_;
    HeuristicType res_type_ = HeuristicType::level;
};

//! A statement edge directives.
class StmEdge : public Stm {
  public:
    //! Construct the statement.
    StmEdge(UTerm src, UTerm dst, ULitVec body) : src_{std::move(src)}, dst_{std::move(dst)}, body_{std::move(body)} {
        init_();
    }

  private:
    void init_();
    // Stm interface
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;

    // InstanceCallback interface
    void do_print_head(std::ostream &out) const override;
    void do_init(size_t gen) override;
    [[nodiscard]] auto do_report(EvalContext const &ctx) -> bool override;
    void do_propagate(SymbolStore &store, OutputStm &out, Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override { return std::numeric_limits<size_t>::max(); }

    UTerm src_;
    UTerm dst_;
    ULitVec body_;
    Symbol res_src_;
    Symbol res_dst_;
};

//! A statement for show directives.
class StmShow : public Stm {
  public:
    //! Construct the statement.
    StmShow(UTerm term, ULitVec body) : term_{std::move(term)}, body_{std::move(body)} { init_(); }

  private:
    void init_();
    // Stm interface
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;

    // InstanceCallback interface
    void do_print_head(std::ostream &out) const override;
    void do_init(size_t gen) override;
    [[nodiscard]] auto do_report(EvalContext const &ctx) -> bool override;
    void do_propagate(SymbolStore &store, OutputStm &out, Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override { return std::numeric_limits<size_t>::max(); }

    UTerm term_;
    ULitVec body_;
    Symbol res_term_;
};

//! Statement capturing project directives.
class StmProject : public Stm {
  public:
    //! Construct the statement.
    StmProject(UTerm atom, AtomBase &base, ULitVec body)
        : atom_{std::move(atom)}, base_{&base}, body_{std::move(body)} {
        init_();
    }

  private:
    void init_();
    // Stm interface
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_body() const -> ULitVec const & override;
    [[nodiscard]] auto do_important() const -> VariableSet override;

    // InstanceCallback interface
    void do_print_head(std::ostream &out) const override;
    void do_init(size_t gen) override;
    [[nodiscard]] auto do_report(EvalContext const &ctx) -> bool override;
    void do_propagate(SymbolStore &store, OutputStm &out, Queue &queue) override;
    [[nodiscard]] auto do_priority() const -> size_t override { return std::numeric_limits<size_t>::max(); }
    [[nodiscard]] auto do_is_important([[maybe_unused]] size_t index) const -> bool override { return false; }

    UTerm atom_;
    AtomBase *base_;
    ULitVec body_;
    size_t offset_ = 0;
};

//! @}

} // namespace CppClingo::Ground
