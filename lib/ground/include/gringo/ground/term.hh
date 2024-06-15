#pragma once

#include <gringo/core/symbol.hh>
#include <gringo/util/ordered_set.hh>

#include <gringo/util/enum.hh>
#include <gringo/util/unordered_map.hh>

namespace Gringo::Ground {

//! @addtogroup ground_term
//! @{

//! Modes determining how to handle variables in terms.
enum class RenameMode : uint8_t {
    rename_vars,       //!< Succesively rename variables in order of traversal.
    rename_projection, //!< Succesively introduce variables for projections in order of traversal.
    drop_projection,   //!< Drop projections from tuples and functions.
};

using VariableSet = Util::ordered_set<size_t>;
using VariableVec = VariableSet::values_container_type;

class Term;
using UTerm = std::unique_ptr<Term>;
using UTermVec = std::vector<UTerm>;

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
    [[nodiscard]] auto match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool {
        return do_match(store, sym, ass);
    }
    //! Evaluates a term w.r.t. the given assignment.
    //!
    //! A term might fail to evaluate if a unary or binary operation is not defined for its arguments.
    [[nodiscard]] auto eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> {
        return do_eval(store, ass);
    }
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
    //! Output the term.
    void print(std::ostream &out) const { do_print(out); }
    //! Create a copy of the term.
    [[nodiscard]] auto copy() const -> UTerm { return do_copy(); }
    [[nodiscard]] auto hash() const -> size_t { return do_hash(); }

    //! Compute a siganture of the term.
    //!
    //! This renames variables in the term in ascending order and then return a pair of the term and the bound
    //! variables.
    [[nodiscard]] auto signature(VariableSet const &bound,
                                 VariableSet const &bind) const -> std::pair<UTerm, VariableVec> {
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

    friend auto operator==(Term const &a, Term const &b) -> bool { return a.do_equal_to(b); }
    friend auto operator<=>(Term const &a, Term const &b) -> std::strong_ordering { return a.do_compare_to(b); }
    friend auto operator<<(std::ostream &out, Term const &term) -> std::ostream & {
        term.print(out);
        return out;
    }

  private:
    [[nodiscard]] virtual auto do_score(double size, std::vector<bool> const &bound) const -> double = 0;
    [[nodiscard]] virtual auto do_match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool = 0;
    [[nodiscard]] virtual auto do_eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> = 0;
    [[nodiscard]] virtual auto do_rename(SymbolStore &store, RenameMode mode, String const *name,
                                         size_t *vars) const -> UTerm = 0;
    [[nodiscard]] virtual auto do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm = 0;
    virtual void do_vars(VariableSet &vars, bool provide) const = 0;
    virtual void do_print(std::ostream &out) const = 0;
    [[nodiscard]] virtual auto do_copy() const -> UTerm = 0;
    [[nodiscard]] virtual auto do_hash() const -> size_t = 0;
    [[nodiscard]] virtual auto do_equal_to(Term const &other) const -> bool = 0;
    [[nodiscard]] virtual auto do_compare_to(Term const &other) const -> std::strong_ordering = 0;
};

class TermProjection : public Term {
  public:
    TermProjection() = default;

  private:
    [[nodiscard]] auto do_score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto do_match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto do_eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto do_rename(SymbolStore &store, RenameMode mode, String const *name,
                                 size_t *vars) const -> UTerm override;
    [[nodiscard]] auto do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void do_vars(VariableSet &vars, bool provide) const override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_copy() const -> UTerm override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Term const &other) const -> std::strong_ordering override;
};

class TermSymbol : public Term {
  public:
    TermSymbol(Symbol sym) : sym_{sym} {}

    [[nodiscard]] auto symbol() const -> Symbol const & { return *sym_; }

  private:
    [[nodiscard]] auto do_score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto do_match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto do_eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto do_rename(SymbolStore &store, RenameMode mode, String const *name,
                                 size_t *vars) const -> UTerm override;
    [[nodiscard]] auto do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void do_vars(VariableSet &vars, bool provide) const override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_copy() const -> UTerm override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Term const &other) const -> std::strong_ordering override;

    SharedSymbol sym_;
};

class TermVariable : public Term {
  public:
    TermVariable(size_t var) : var_{var} {}

  private:
    [[nodiscard]] auto do_score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto do_match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto do_eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto do_rename(SymbolStore &store, RenameMode mode, String const *name,
                                 size_t *vars) const -> UTerm override;
    [[nodiscard]] auto do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void do_vars(VariableSet &vars, bool provide) const override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_copy() const -> UTerm override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Term const &other) const -> std::strong_ordering override;

    size_t var_;
};

class TermLinear : public Term {
  public:
    TermLinear(Number m, size_t var, Number n) : m_{std::move(m)}, n_{std::move(n)}, var_{var} {}

  private:
    [[nodiscard]] auto do_score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto do_match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto do_eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto do_rename(SymbolStore &store, RenameMode mode, String const *name,
                                 size_t *vars) const -> UTerm override;
    [[nodiscard]] auto do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void do_vars(VariableSet &vars, bool provide) const override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_copy() const -> UTerm override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Term const &other) const -> std::strong_ordering override;

    Number m_;
    Number n_;
    size_t var_;
};

//! Available unary operations.
enum class UnaryOperator : uint8_t {
    minus = 0,  //!< The unary arithmetic minus operation.
    invert = 1, //!< The bitwise negation operation.
    abs = 2,    //!< The arithmetic absolute operation.
};

class TermUnary : public Term {
  public:
    TermUnary(UnaryOperator op, UTerm rhs) : rhs_{std::move(rhs)}, op_{op} {}

  private:
    [[nodiscard]] auto do_score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto do_match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto do_eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto do_rename(SymbolStore &store, RenameMode mode, String const *name,
                                 size_t *vars) const -> UTerm override;
    [[nodiscard]] auto do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void do_vars(VariableSet &vars, bool provide) const override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_copy() const -> UTerm override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Term const &other) const -> std::strong_ordering override;

    UTerm rhs_;
    UnaryOperator op_;
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

class TermBinary : public Term {
  public:
    TermBinary(UTerm lhs, BinaryOperator op, UTerm rhs) : lhs_{std::move(lhs)}, rhs_{std::move(rhs)}, op_{op} {}

  private:
    [[nodiscard]] auto do_score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto do_match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto do_eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto do_rename(SymbolStore &store, RenameMode mode, String const *name,
                                 size_t *vars) const -> UTerm override;
    [[nodiscard]] auto do_rename(Util::unordered_map<size_t, size_t> &vars) const -> UTerm override;
    void do_vars(VariableSet &vars, bool provide) const override;
    void do_print(std::ostream &out) const override;
    [[nodiscard]] auto do_copy() const -> UTerm override;
    [[nodiscard]] auto do_hash() const -> size_t override;
    [[nodiscard]] auto do_equal_to(Term const &other) const -> bool override;
    [[nodiscard]] auto do_compare_to(Term const &other) const -> std::strong_ordering override;

    UTerm lhs_;
    UTerm rhs_;
    BinaryOperator op_;
};

class TermTuple : public Term {
  public:
    TermTuple(UTermVec args) : args_{std::move(args)} { eval_.reserve(args_.size()); }

  private:
    [[nodiscard]] auto do_score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto do_match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto do_eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto do_rename(SymbolStore &store, RenameMode mode, String const *name,
                                 size_t *vars) const -> UTerm override;
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

class TermFunction : public Term {
  public:
    TermFunction(String name, UTermVec args) : name_{name}, args_{std::move(args)} { eval_.reserve(args_.size()); }

  private:
    [[nodiscard]] auto do_score(double size, std::vector<bool> const &bound) const -> double override;
    [[nodiscard]] auto do_match(SymbolStore &store, Symbol sym, Assignment &ass) const -> bool override;
    [[nodiscard]] auto do_eval(SymbolStore &store, Assignment const &ass) const -> std::optional<Symbol> override;
    [[nodiscard]] auto do_rename(SymbolStore &store, RenameMode mode, String const *name,
                                 size_t *vars) const -> UTerm override;
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

//! @}

} // namespace Gringo::Ground
