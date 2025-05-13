#pragma once

#include <clingo/ground/instantiator.hh>

#include <clingo/core/core.hh>
#include <clingo/core/location.hh>
#include <clingo/core/logger.hh>
#include <clingo/core/symbol.hh>

#include <clingo/util/enum.hh>
#include <clingo/util/ordered_set.hh>
#include <clingo/util/unordered_map.hh>

namespace CppClingo::Ground {

//! @addtogroup ground_term
//! @{

//! A set of variables.
using VariableSet = Util::ordered_set<size_t>;
//! A vector of variables.
using VariableVec = VariableSet::values_container_type;

//! Modes determining how to handle variables in terms.
enum class RenameMode : uint8_t {
    rename_vars,       //!< Successively rename variables in order of traversal.
    rename_projection, //!< Successively introduce variables for projections in order of traversal.
    drop_projection,   //!< Drop projections from tuples and functions.
};

class Term;
//! A unique pointer holding a term.
using UTerm = std::unique_ptr<Term>;
//! A vector of terms.
using UTermVec = std::vector<UTerm>;
//! A vector of guards.
using GuardVec = std::vector<std::pair<Relation, UTerm>>;

//! Helper to copy vectors with copyable elements.
template <class T>
    requires requires(T x) {
        { x.copy() } -> std::convertible_to<std::unique_ptr<T>>;
    }
auto copy_uvec(std::vector<std::unique_ptr<T>> const &vec) {
    std::vector<std::unique_ptr<T>> res;
    res.reserve(vec.size());
    for (auto const &x : vec) {
        res.emplace_back(x->copy());
    }
    return res;
}
//! Helper to copy vectors with copyable elements.
template <class T>
    requires requires(T x) {
        { x.copy() } -> std::convertible_to<std::unique_ptr<T>>;
    }
auto copy_uvec(std::vector<std::vector<std::unique_ptr<T>>> const &vec) {
    std::vector<std::vector<std::unique_ptr<T>>> res;
    res.reserve(vec.size());
    for (auto const &x : vec) {
        res.emplace_back(copy_uvec(x));
    }
    return res;
}

//! Term interface.
class Term {
  public:
    //! Key for the concept only matcher interface.
    using Key = Symbol;

    //! Destructor.
    virtual ~Term() = default;
    //! Compute an estimate how often the term can match size symbols given the bound variables.
    [[nodiscard]] auto score(double size, std::vector<bool> const &bound) const -> double {
        return do_score(size, bound);
    }
    //! Match a term with the given symbol.
    //!
    //! Returns true if the term can match the symbol.
    //! Variables in the term are assigned to symbols and stored in the given assignment.
    [[nodiscard]] auto match(EvalContext const &ctx, Symbol sym) const -> bool { return do_match(ctx, sym); }
    //! Evaluates a term w.r.t. the given assignment.
    //!
    //! A term might fail to evaluate if a unary or binary operation is not defined for its arguments.
    [[nodiscard]] auto eval(EvalContext const &ctx) const -> std::optional<Symbol> { return do_eval(ctx); }
    //! Create a copy of the term renaming/replacing parts of it.
    //!
    //! If a name is given, the name of the outermost function symbol is changed.
    //! Otherwise, variables and projection are replaced according to the given mode.
    [[nodiscard]] auto rename(SymbolStore &store, RenameMode mode, String const *name, size_t *vars) const -> UTerm {
        return do_rename(store, mode, name, vars);
    }
    //! Create a copy of the term renaming variables in order of occurrence.
    [[nodiscard]] auto rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm { return do_rename(vars); }
    //! Collect all variables in the term.
    void vars(VariableSet &vars, bool provide = false) const { do_vars(vars, provide); }
    //! Create a copy of the term.
    [[nodiscard]] auto copy() const -> UTerm { return do_copy(); }
    //! Compute a hash for the term.
    [[nodiscard]] auto hash() const -> size_t { return do_hash(); }

    //! Compute a signature of the term.
    //!
    //! This renames variables in the term in ascending order and then return a pair of the term and the bound
    //! variables.
    [[nodiscard]] auto signature(VariableSet const &bound, VariableSet const &bind) const
        -> std::pair<UTerm, VariableVec> {
        auto names = Util::unordered_map<size_t, size_t>{};
        names.reserve(bind.size() + bound.size());
        auto sig_term = rename(names);
        auto sig_lookup = std::vector<size_t>{};
        sig_lookup.reserve(bound.size());
        for (auto const &var : bound) {
            sig_lookup.emplace_back(names[var]);
        }
        return {std::move(sig_term), std::move(sig_lookup)};
    }
    //! Collect all variables in the term.
    [[nodiscard]] auto vars() const -> VariableSet {
        VariableSet set;
        vars(set);
        return set;
    }

    //! Compare two terms.
    friend auto operator==(Term const &a, Term const &b) -> bool { return a.do_equal_to(b); }
    //! Compare two terms.
    friend auto operator<=>(Term const &a, Term const &b) -> std::strong_ordering { return a.do_compare_to(b); }
    //! Print the term.
    friend auto operator<<(std::ostream &out, Term const &term) -> std::ostream & {
        term.do_print(out);
        return out;
    }

  private:
    [[nodiscard]] virtual auto do_score(double size, std::vector<bool> const &bound) const -> double = 0;
    [[nodiscard]] virtual auto do_match(EvalContext const &ctx, Symbol sym) const -> bool = 0;
    [[nodiscard]] virtual auto do_eval(EvalContext const &ctx) const -> std::optional<Symbol> = 0;
    [[nodiscard]] virtual auto do_rename(SymbolStore &store, RenameMode mode, String const *name, size_t *vars) const
        -> UTerm = 0;
    [[nodiscard]] virtual auto do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm = 0;
    virtual void do_vars(VariableSet &vars, bool provide) const = 0;
    virtual void do_print(std::ostream &out) const = 0;
    [[nodiscard]] virtual auto do_copy() const -> UTerm = 0;
    [[nodiscard]] virtual auto do_hash() const -> size_t = 0;
    [[nodiscard]] virtual auto do_equal_to(Term const &other) const -> bool = 0;
    [[nodiscard]] virtual auto do_compare_to(Term const &other) const -> std::strong_ordering = 0;
};

//! A term capturing a projection.
class TermProjection : public Term {
  public:
    //! Construct the term.
    TermProjection() = default;

  private:
    [[nodiscard]] auto do_score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto do_match(EvalContext const &ctx, Symbol sym) const -> bool override;
    [[nodiscard]] auto do_eval(EvalContext const &ctx) const -> std::optional<Symbol> override;
    [[nodiscard]] auto do_rename(SymbolStore &store, RenameMode mode, String const *name, size_t *vars) const
        -> UTerm override;
    [[nodiscard]] auto do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void do_vars(VariableSet &vars, bool provide) const override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_copy() const -> UTerm override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Term const &other) const -> std::strong_ordering override;
};

//! A term capturing a symbol.
class TermSymbol : public Term {
  public:
    //! Construct the term.
    TermSymbol(Symbol sym) : sym_{sym} {}

    //! Get the symbol of the term.
    [[nodiscard]] auto symbol() const -> Symbol const & { return *sym_; }

  private:
    [[nodiscard]] auto do_score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto do_match(EvalContext const &ctx, Symbol sym) const -> bool override;
    [[nodiscard]] auto do_eval(EvalContext const &ctx) const -> std::optional<Symbol> override;
    [[nodiscard]] auto do_rename(SymbolStore &store, RenameMode mode, String const *name, size_t *vars) const
        -> UTerm override;
    [[nodiscard]] auto do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void do_vars(VariableSet &vars, bool provide) const override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_copy() const -> UTerm override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Term const &other) const -> std::strong_ordering override;

    SharedSymbol sym_;
};

//! A term capturing a variable.
class TermVariable : public Term {
  public:
    //! Construct the term.
    TermVariable(size_t var) : var_{var} {}

  private:
    [[nodiscard]] auto do_score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto do_match(EvalContext const &ctx, Symbol sym) const -> bool override;
    [[nodiscard]] auto do_eval(EvalContext const &ctx) const -> std::optional<Symbol> override;
    [[nodiscard]] auto do_rename(SymbolStore &store, RenameMode mode, String const *name, size_t *vars) const
        -> UTerm override;
    [[nodiscard]] auto do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void do_vars(VariableSet &vars, bool provide) const override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_copy() const -> UTerm override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Term const &other) const -> std::strong_ordering override;

    size_t var_;
};

//! A term capturing a linear term.
class TermLinear : public Term {
  public:
    //! Construct the term.
    TermLinear(Location loc, Number m, size_t var, Number n)
        : loc_{std::move(loc)}, m_{std::move(m)}, n_{std::move(n)}, var_{var} {}

  private:
    [[nodiscard]] auto do_score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto do_match(EvalContext const &ctx, Symbol sym) const -> bool override;
    [[nodiscard]] auto do_eval(EvalContext const &ctx) const -> std::optional<Symbol> override;
    [[nodiscard]] auto do_rename(SymbolStore &store, RenameMode mode, String const *name, size_t *vars) const
        -> UTerm override;
    [[nodiscard]] auto do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void do_vars(VariableSet &vars, bool provide) const override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_copy() const -> UTerm override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Term const &other) const -> std::strong_ordering override;

    Location loc_;
    Number m_;
    Number n_;
    size_t var_;
    mutable bool logged_ = false;
};

//! Available unary operations.
enum class UnaryOperator : uint8_t {
    minus = 0,  //!< The unary arithmetic minus operation.
    negate = 1, //!< The bitwise negation operation.
    abs = 2,    //!< The arithmetic absolute operation.
};

//! A term capturing a unary term.
class TermUnary : public Term {
  public:
    //! Construct the term.
    TermUnary(UnaryOperator op, Location loc_rhs, UTerm rhs)
        : loc_rhs_{std::move(loc_rhs)}, rhs_{std::move(rhs)}, op_{op} {}

  private:
    [[nodiscard]] auto do_score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto do_match(EvalContext const &ctx, Symbol sym) const -> bool override;
    [[nodiscard]] auto do_eval(EvalContext const &ctx) const -> std::optional<Symbol> override;
    [[nodiscard]] auto do_rename(SymbolStore &store, RenameMode mode, String const *name, size_t *vars) const
        -> UTerm override;
    [[nodiscard]] auto do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void do_vars(VariableSet &vars, bool provide) const override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_copy() const -> UTerm override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Term const &other) const -> std::strong_ordering override;

    Location loc_rhs_;
    UTerm rhs_;
    UnaryOperator op_;
    mutable bool logged_ = false;
};

//! Available binary operations.
enum class BinaryOperator : uint8_t {
    and_,  //!< The AND bit operation.
    div,   //!< The (integer) divide arithmetic operation.
    minus, //!< The minus arithmetic operation.
    mod,   //!< The modulo arithmetic operation.
    times, //!< The multiply arithmetic operation.
    or_,   //!< The OR bit operation.
    plus,  //!< The plus arithmetic operation.
    pow,   //!< The exponentiation arithmetic operation.
    xor_,  //!< The XOR bit operation.
};

//! A term capturing a binary term.
class TermBinary : public Term {
  public:
    //! Construct the term.
    TermBinary(Location loc_lhs, UTerm lhs, BinaryOperator op, Location loc_rhs, UTerm rhs)
        : loc_lhs_{std::move(loc_lhs)}, loc_rhs_{std::move(loc_rhs)}, lhs_{std::move(lhs)}, rhs_{std::move(rhs)},
          op_{op} {}

  private:
    [[nodiscard]] auto do_score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto do_match(EvalContext const &ctx, Symbol sym) const -> bool override;
    [[nodiscard]] auto do_eval(EvalContext const &ctx) const -> std::optional<Symbol> override;
    [[nodiscard]] auto do_rename(SymbolStore &store, RenameMode mode, String const *name, size_t *vars) const
        -> UTerm override;
    [[nodiscard]] auto do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void do_vars(VariableSet &vars, bool provide) const override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_copy() const -> UTerm override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Term const &other) const -> std::strong_ordering override;

    Location loc_lhs_;
    Location loc_rhs_;
    UTerm lhs_;
    UTerm rhs_;
    BinaryOperator op_;
    mutable bool logged_ = false;
};

//! A term capturing a tuple.
class TermTuple : public Term {
  public:
    //! Construct the term.
    TermTuple(UTermVec args) : args_{std::move(args)} { eval_.reserve(args_.size()); }

  private:
    [[nodiscard]] auto do_score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto do_match(EvalContext const &ctx, Symbol sym) const -> bool override;
    [[nodiscard]] auto do_eval(EvalContext const &ctx) const -> std::optional<Symbol> override;
    [[nodiscard]] auto do_rename(SymbolStore &store, RenameMode mode, String const *name, size_t *vars) const
        -> UTerm override;
    [[nodiscard]] auto do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void do_vars(VariableSet &vars, bool provide) const override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_copy() const -> UTerm override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Term const &other) const -> std::strong_ordering override;

    UTermVec args_;
    std::vector<Symbol> mutable eval_;
};

//! A term capturing a tuple.
class TermFunction : public Term {
  public:
    //! Construct the term.
    TermFunction(String name, UTermVec args) : name_{name}, args_{std::move(args)} { eval_.reserve(args_.size()); }

  private:
    [[nodiscard]] auto do_score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto do_match(EvalContext const &ctx, Symbol sym) const -> bool override;
    [[nodiscard]] auto do_eval(EvalContext const &ctx) const -> std::optional<Symbol> override;
    [[nodiscard]] auto do_rename(SymbolStore &store, RenameMode mode, String const *name, size_t *vars) const
        -> UTerm override;
    [[nodiscard]] auto do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void do_vars(VariableSet &vars, bool provide) const override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_copy() const -> UTerm override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Term const &other) const -> std::strong_ordering override;

    SharedString name_;
    UTermVec args_;
    std::vector<Symbol> mutable eval_;
};

//! Helper to report a message only once.
template <class... T>
inline auto expect(EvalContext const &ctx, Location const &loc, bool &logged, T &&...args) -> bool {
    if (!logged && ctx.log().check(MessageCode::info_operation_undefined)) {
        logged = true;
        (CppClingo::Report(ctx.log(), MessageCode::info_operation_undefined, loc).out()
         << ... << std::forward<T>(args));
    }
    return false;
}

//! @}

} // namespace CppClingo::Ground
