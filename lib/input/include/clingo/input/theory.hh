#pragma once

#include <clingo/input/literal.hh>
#include <clingo/input/term.hh>

#include <utility>

namespace CppClingo::Input {

//! @addtogroup input_theory
//! @{

class TheoryTermSymbol;
class TheoryTermVariable;
class TheoryTermTuple;
class TheoryTermFunction;
class TheoryTermUnparsed;
class UnparsedElement;

//! A variant for the different theory terms.
using TheoryTerm =
    std::variant<TheoryTermSymbol, TheoryTermVariable, TheoryTermTuple, TheoryTermFunction, TheoryTermUnparsed>;
//! A vector of theory terms.
using TheoryTermArray = Util::immutable_array<TheoryTerm>;

//! A symbolic theory term.
//!
//! For example: <tt>1</tt>.
class TheoryTermSymbol : public Expression<TheoryTermSymbol> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TheoryTermSymbol::loc_, a_value = &TheoryTermSymbol::value};
    }

    //! Construct a symbolic theory term.
    explicit TheoryTermSymbol(Location loc, Symbol value) : loc_{std::move(loc)}, value_{value} {}

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The symbol.
    [[nodiscard]] auto value() const -> Symbol const & { return *value_; }

  private:
    Location loc_;
    SharedSymbol value_;
};

//! A variable theory term.
//!
//! For example: <tt>X</tt>.
class TheoryTermVariable : public Expression<TheoryTermVariable> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TheoryTermVariable::loc_, a_name = &TheoryTermVariable::name,
                          a_anonymous = &TheoryTermVariable::anonymous_};
    }

    //! Construct a variable theory term.
    explicit TheoryTermVariable(Location loc, String name, bool is_anonymous = false)
        : loc_{std::move(loc)}, name_{name}, anonymous_{is_anonymous} {}

    //! The location of the variable.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the variable.
    [[nodiscard]] auto name() const -> String const & { return *name_; }
    //! Whether the variable is anonymous.
    [[nodiscard]] auto anonymous() const -> bool { return anonymous_; }

  private:
    Location loc_;
    SharedString name_;
    bool anonymous_;
};

//! A tuple (set or list) theory term.
//!
//! For example: <tt>f(X,y)</tt>.
class TheoryTermTuple : public RecursiveExpression<TheoryTermTuple> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TheoryTermTuple::loc_, a_type = &TheoryTermTuple::type_,
                          a_elems = &TheoryTermTuple::elems_};
    }

    //! Construct a tuple theory term.
    explicit TheoryTermTuple(Location loc, TheoryTermTupleType type, TheoryTermArray elems);

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The type of the term.
    [[nodiscard]] auto type() const -> TheoryTermTupleType { return type_; }
    //! The elements of the tuple.
    [[nodiscard]] auto elems() const -> TheoryTermArray const & { return elems_; }

  private:
    Location loc_;
    TheoryTermTupleType type_;
    TheoryTermArray elems_;
};

//! A theory term function.
//!
//! For example: <tt>f(X,y)</tt>.
class TheoryTermFunction : public RecursiveExpression<TheoryTermFunction> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TheoryTermFunction::loc_, a_name = &TheoryTermFunction::name,
                          a_args = &TheoryTermFunction::args_};
    }

    //! Construct a function theory term.
    explicit TheoryTermFunction(Location loc, String name);
    //! Construct a function theory term.
    explicit TheoryTermFunction(Location loc, String name, TheoryTermArray args);

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the function.
    [[nodiscard]] auto name() const -> String const & { return *name_; }
    //! The arguments of the function.
    [[nodiscard]] auto args() const -> TheoryTermArray const & { return args_; }

  private:
    Location loc_;
    SharedString name_;
    TheoryTermArray args_;
};

//! A vector of elements.
//!
//! In this context, it has to have at least length one.
//! Furthermore, all but the first element must have at least one operator.
using UnparsedElementArray = Util::immutable_array<UnparsedElement>;

//! An unparsed theory term.
//!
//! The priorities and associativities of the operators have not yet been applied.
//! They are simply stored as a list.
//!
//! For example: <tt>- X ++ Y << Z</tt>.
class TheoryTermUnparsed : public RecursiveExpression<TheoryTermUnparsed> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TheoryTermUnparsed::loc_, a_elems = &TheoryTermUnparsed::elems_};
    }

    //! Construct an unparsed theory term.
    explicit TheoryTermUnparsed(Location loc, UnparsedElementArray elems);

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The vector of elements.
    [[nodiscard]] auto elems() const -> UnparsedElementArray const & { return elems_; }

  private:
    Location loc_;
    UnparsedElementArray elems_;
};

//! An element having the form of a right guard.
class UnparsedElement : public Expression<UnparsedElement> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_ops = &UnparsedElement::ops, a_term = &UnparsedElement::term_};
    }

    //! Construct the element.
    UnparsedElement(StringSpan ops, TheoryTerm term) : ops_{ops.begin(), ops.end()}, term_{std::move(term)} {}
    //! Construct the element.
    UnparsedElement(SharedStringArray ops, TheoryTerm term) : ops_{std::move(ops)}, term_{std::move(term)} {}

    //! The list of operator names.
    [[nodiscard]] auto ops() const -> StringSpan { return as_string_span(ops_); }
    //! The term.
    [[nodiscard]] auto term() const -> TheoryTerm const & { return term_; }

  private:
    SharedStringArray ops_;
    TheoryTerm term_;
};

//! The right guard of the theory atom.
class TheoryRGuard : public Expression<TheoryRGuard> {
  public:
    //! The record attributes.
    static constexpr auto attributes() { return std::tuple{a_op = &TheoryRGuard::op, a_term = &TheoryRGuard::term_}; }

    //! Construct the guard.
    explicit TheoryRGuard(String op, TheoryTerm term) : op_{op}, term_{std::move(term)} {}

    //! The tuple of the theory element.
    [[nodiscard]] auto op() const -> String const & { return *op_; }
    //! The condition of the theory element.
    [[nodiscard]] auto term() const -> TheoryTerm const & { return term_; }

  private:
    SharedString op_;
    TheoryTerm term_;
};

//! An element of the theory atom.
class TheoryElement : public Expression<TheoryElement> {
  public:
    //! The record attributes.
    static constexpr auto attributes() {
        return std::tuple{a_loc = &TheoryElement::loc_, a_tuple = &TheoryElement::tuple_,
                          a_cond = &TheoryElement::cond_};
    }

    //! Construct a theory element.
    explicit TheoryElement(Location loc, TheoryTermArray tuple, LitArray cond)
        : loc_{std::move(loc)}, tuple_{std::move(tuple)}, cond_{std::move(cond)} {}

    //! The location of the theory element.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The tuple of the theory element.
    [[nodiscard]] auto tuple() const -> TheoryTermArray const & { return tuple_; }
    //! The condition of the theory element.
    [[nodiscard]] auto cond() const -> LitArray const & { return cond_; }

  private:
    Location loc_;
    TheoryTermArray tuple_;
    LitArray cond_;
};
//! A vector of theory atom elements.
using TheoryElementArray = Util::immutable_array<TheoryElement>;

//! A theory atom.
//!
//! For example: <tt>&sum { X+Y: p(X), q(Y) } >= 0</tt>.
template <bool HasSign>
class TheoryAtom : public std::conditional_t<HasSign, Signed, Unsigned>, public Expression<TheoryAtom<HasSign>> {
  public:
    //! The parent class.
    using Base = std::conditional_t<HasSign, Signed, Unsigned>;

    //! The record attributes.
    static constexpr auto attributes() {
        if constexpr (HasSign) {
            return std::tuple{a_loc = &TheoryAtom::loc_, a_sign = &Signed::sign, a_name = &TheoryAtom::name_,
                              a_elems = &TheoryAtom::elems_, a_rhs = &TheoryAtom::rhs_};
        } else {
            return std::tuple{a_loc = &TheoryAtom::loc_, a_name = &TheoryAtom::name_, a_elems = &TheoryAtom::elems_,
                              a_rhs = &TheoryAtom::rhs_};
        }
    }

    //! Construct a theory atom.
    explicit TheoryAtom(Location loc, Term name, TheoryElementArray elems, std::optional<TheoryRGuard> rhs)
        : loc_{std::move(loc)}, name_{std::move(name)}, elems_{std::move(elems)}, rhs_{std::move(rhs)} {
        static_assert(!HasSign);
    }

    //! Construct a theory atom.
    explicit TheoryAtom(Location loc, Sign sign, Term name, TheoryElementArray elems, std::optional<TheoryRGuard> rhs)
        : Signed{sign}, loc_{std::move(loc)}, name_{std::move(name)}, elems_{std::move(elems)}, rhs_{std::move(rhs)} {
        static_assert(HasSign);
    }

    //! The location of the symbol.
    [[nodiscard]] auto loc() const -> Location const & { return loc_; }
    //! The name of the atom.
    [[nodiscard]] auto name() const -> Term const & { return name_; }
    //! The elements of the atom.
    [[nodiscard]] auto elems() const -> TheoryElementArray const & { return elems_; }
    //! The optional right guard of the atom.
    [[nodiscard]] auto rhs() const -> std::optional<TheoryRGuard> const & { return rhs_; }

  private:
    Location loc_;
    Term name_;
    TheoryElementArray elems_;
    std::optional<TheoryRGuard> rhs_;
};

//! A head theory atom.
using HdLitTheoryAtom = TheoryAtom<false>;

//! A body theory atom.
using BdLitTheoryAtom = TheoryAtom<true>;

//! @}

// TheoryTermTuple

inline TheoryTermTuple::TheoryTermTuple(Location loc, TheoryTermTupleType type, TheoryTermArray elems)
    : loc_{std::move(loc)}, type_(type), elems_{std::move(elems)} {
}

inline auto operator==(TheoryTermTuple const &a, TheoryTermTuple const &b) -> bool {
    return a.equal(b);
}

inline auto operator<=>(TheoryTermTuple const &a, TheoryTermTuple const &b) -> std::strong_ordering {
    return a.compare(b);
}

// TheoryTermFunction

inline TheoryTermFunction::TheoryTermFunction(Location loc, String name, TheoryTermArray args)
    : loc_{std::move(loc)}, name_(name), args_{std::move(args)} {
}

inline TheoryTermFunction::TheoryTermFunction(Location loc, String name)
    : TheoryTermFunction{std::move(loc), name, {}} {
}

inline auto operator==(TheoryTermFunction const &a, TheoryTermFunction const &b) -> bool {
    return a.equal(b);
}

inline auto operator<=>(TheoryTermFunction const &a, TheoryTermFunction const &b) -> std::strong_ordering {
    return a.compare(b);
}

// TheoryTermUnparsed

inline TheoryTermUnparsed::TheoryTermUnparsed(Location loc, UnparsedElementArray elems)
    : loc_{std::move(loc)}, elems_{std::move(elems)} {
}

inline auto operator==(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> bool {
    return a.equal(b);
}

inline auto operator<=>(TheoryTermUnparsed const &a, TheoryTermUnparsed const &b) -> std::strong_ordering {
    return a.compare(b);
}

} // namespace CppClingo::Input
