#include "ast.hh"
#include "core.hh"
#include "lib.hh"

#include <clingo/input/parser.hh>
#include <clingo/input/print.hh>
#include <clingo/input/rewrite.hh>

#include <clingo/input/rewrite/rewrite_theory.hh>
#include <clingo/input/rewrite/substitute.hh>

#include <clingo/util/algorithm.hh>
#include <clingo/util/ordered_map.hh>
#include <clingo/util/ordered_set.hh>
#include <clingo/util/type_traits.hh>

#include <algorithm>
#include <cstdarg>
#include <cstring>
#include <forward_list>
#include <fstream>
#include <ranges>
#include <span>

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

using namespace CppClingo::CAPI;

namespace CppClingo::CAPI {
namespace {

class ASTVec;

}
} // namespace CppClingo::CAPI

struct clingo_ast {
  public:
    template <class T>
    clingo_ast(std::shared_ptr<T> owner, clingo_ast_type_e type, void const *ptr)
        : type_{type}, ptr_{std::move(owner), ptr} {}
    [[nodiscard]] auto copy() const -> std::unique_ptr<clingo_ast_t>;
    template <class T> void print(T &out) const;
    [[nodiscard]] auto hash() const -> size_t;
    [[nodiscard]] auto equal(clingo_ast_t const &other) const -> bool;
    [[nodiscard]] auto compare(clingo_ast_t const &other) const -> std::strong_ordering;
    [[nodiscard]] auto get_type() const -> clingo_ast_type_e;
    [[nodiscard]] auto get_number(clingo_ast_attribute_t attr) const -> std::optional<int>;
    [[nodiscard]] auto get_symbol(clingo_ast_attribute_t attr) const -> std::optional<clingo_symbol_t>;
    [[nodiscard]] auto get_location(clingo_ast_attribute_t attr) const -> std::optional<clingo_location_t const *>;
    [[nodiscard]] auto get_string(clingo_ast_attribute_t attr) const -> std::optional<std::string_view>;
    [[nodiscard]] auto get_string_vec(clingo_ast_attribute_t attr) const -> std::optional<CppClingo::StringSpan>;
    [[nodiscard]] auto get_symbol_vec(clingo_ast_attribute_t attr) const -> std::optional<CppClingo::SymbolSpan>;

    [[nodiscard]] auto get_ast(clingo_ast_attribute_t attr) const -> std::optional<std::unique_ptr<clingo_ast_t>>;
    [[nodiscard]] auto get_ast_vec(clingo_ast_attribute_t attr) const -> std::optional<ASTVec>;

    template <typename T> [[nodiscard]] auto cast() const -> T const & { return *static_cast<T const *>(ptr_.get()); }
    template <class V> auto visit(V &&visit) const -> std::invoke_result_t<V, CppClingo::Input::Projection const &>;
    template <class T> friend auto operator<<(T &out, clingo_ast_t const &ast) -> T & {
        ast.print(out);
        return out;
    }

  private:
    clingo_ast_type_e type_;
    std::shared_ptr<void const> ptr_;
};

namespace CppClingo::CAPI {
namespace {

class ASTVec {
  public:
    ASTVec() = default;
    ASTVec(size_t size) {
        if (size > 0) {
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
            data_ = new clingo_ast_t *[size] { nullptr };
            size_ = size;
        }
    }
    ASTVec(ASTVec const &other) : ASTVec{other.size()} {
        for (size_t i = 0; i < size_; ++i) {
            operator[](i) = other.operator[](i)->copy().release();
        }
    }
    ASTVec(ASTVec &&other) noexcept {
        std::swap(other.data_, data_);
        std::swap(other.size_, size_);
    }
    auto operator=(ASTVec const &other) -> ASTVec & {
        if (this != &other) {
            *this = ASTVec{other};
        }
        return *this;
    }
    auto operator=(ASTVec &&other) noexcept -> ASTVec & {
        if (this != &other) {
            std::swap(other.data_, data_);
            std::swap(other.size_, size_);
        }
        return *this;
    }
    ~ASTVec() {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        for (auto it = data_, ie = data_ + size_; it != ie; ++it) {
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
            delete *it;
        }
        delete[] data_;
    }
    [[nodiscard]] auto empty() const -> bool { return size_ == 0; }
    [[nodiscard]] auto size() const -> size_t { return size_; }
    [[nodiscard]] auto begin() const -> clingo_ast_t ** { return data_; }
    [[nodiscard]] auto end() const -> clingo_ast_t ** {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return data_ + size_;
    }
    auto operator[](size_t i) const -> clingo_ast_t *& {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic,clang-analyzer-core.uninitialized.UndefReturn)
        return data_[i];
    }
    auto release() -> std::pair<clingo_ast_t **, size_t> {
        auto res = std::make_pair(data_, size_);
        data_ = nullptr;
        size_ = 0;
        return res;
    }
    static auto acquire(clingo_ast_t **data, size_t size) -> ASTVec { return ASTVec{data, size}; }
    static auto copy(clingo_ast_t const **data, size_t size) -> ASTVec {
        auto ret = ASTVec{size};
        for (size_t i = 0; i < size; ++i) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            ret[i] = data[i]->copy().release();
        }
        return ret;
    }

  private:
    ASTVec(clingo_ast_t **data, size_t size) : data_{data}, size_{size} {}

    clingo_ast_t **data_ = nullptr;
    size_t size_ = 0;
};

// {{{ make_ast

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::LGuard::value_type const &guard)
    -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_left_guard, &guard);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::RGuard::value_type const &guard)
    -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_right_guard, &guard);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::TheoryRGuard const &guard) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_theory_right_guard, &guard);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::Projection const &projection)
    -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_projection, &projection);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::Term const &term) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, CppClingo::Input::TermVariable>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_term_variable, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::TermSymbol>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_term_symbolic, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::TermTuple>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_term_tuple, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::TermFunction>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_term_function, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::TermAbs>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_term_absolute, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::TermUnary>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_term_unary_operation, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::TermBinary>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_term_binary_operation, &x);
            }
        },
        term);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::TheoryTerm const &term) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, CppClingo::Input::TheoryTermVariable>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_theory_term_variable, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::TheoryTermSymbol>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_theory_term_symbolic, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::TheoryTermTuple>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_theory_term_tuple, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::TheoryTermFunction>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_theory_term_function, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::TheoryTermUnparsed>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_theory_term_unparsed, &x);
            }
        },
        term);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::UnparsedElement const &elem)
    -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_unparsed_element, &elem);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::ArgumentTuple const &tuple) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_argument_tuple, &tuple);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::TupleElement const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, CppClingo::Input::Term>) {
                return make_ast(std::move(owner), x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::ArgumentTuple>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_argument_tuple, &x);
            }
        },
        elem);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::Lit const &lit) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, CppClingo::Input::LitBool>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_literal_boolean, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::LitSymbolic>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_literal_symbolic, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::LitComparison>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_literal_comparison, &x);
            }
        },
        lit);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::HdLit const &lit) -> std::unique_ptr<clingo_ast_t> {
    using namespace CppClingo::Input;
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, HdLitSimple>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_head_simple_literal, &x);
            }
            if constexpr (std::is_same_v<T, HdLitDisjunction>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_head_disjunction, &x);
            }
            if constexpr (std::is_same_v<T, HdLitSetAggregate>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_head_set_aggregate, &x);
            }
            if constexpr (std::is_same_v<T, HdLitTheoryAtom>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_head_theory_atom, &x);
            }
            if constexpr (std::is_same_v<T, HdLitAggregate>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_head_aggregate, &x);
            }
        },
        lit);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::BdLit const &lit) -> std::unique_ptr<clingo_ast_t> {
    using namespace CppClingo::Input;
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, BdLitSimple>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_body_simple_literal, &x);
            }
            if constexpr (std::is_same_v<T, BdLitConjunction>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_body_conditional_literal, &x);
            }
            if constexpr (std::is_same_v<T, BdLitSetAggregate>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_body_set_aggregate, &x);
            }
            if constexpr (std::is_same_v<T, BdLitTheoryAtom>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_body_theory_atom, &x);
            }
            if constexpr (std::is_same_v<T, BdLitAggregate>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_body_aggregate, &x);
            }
        },
        lit);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::TheoryElement const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_theory_atom_element, &elem);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::SetAggregateElement const &elem)
    -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_set_aggregate_element, &elem);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::BdLitAggregateElement const &elem)
    -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_body_aggregate_element, &elem);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::HdLitAggregateElement const &elem)
    -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_head_aggregate_element, &elem);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::HdLitDisjunctionElement const &elem)
    -> std::unique_ptr<clingo_ast_t> {
    using namespace CppClingo::Input;
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, CondLit>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_head_conditional_literal, &x);
            } else {
                return make_ast(std::move(owner), x);
            }
        },
        elem);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::TheoryOpDefinition const &def)
    -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_theory_operator_definition, &def);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::TheoryRGuardDefinition const &def)
    -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_theory_guard_definition, &def);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::TheoryTermDefinition const &def)
    -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_theory_term_definition, &def);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::TheoryAtomDefinition const &def)
    -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_theory_atom_definition, &def);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::OptimizeTuple const &tuple) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_optimize_tuple, &tuple);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::OptimizeElement const &elem)
    -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_optimize_element, &elem);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::ProgramParam const &part) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_program_part, &part);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::Edge const &edge) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_edge, &edge);
}

template <class U>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Input::Stm const &term) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, CppClingo::Input::StmRule>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_rule, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::StmTheory>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_theory, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::StmOptimize>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_optimize, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::StmWeakConstraint>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_weak_constraint, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::StmShow>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_show, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::StmShowNothing>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_show_nothing, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::StmShowSig>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_show_signature, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::StmProject>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_project, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::StmProjectSig>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_project_signature,
                                                      &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::StmDefined>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_defined, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::StmExternal>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_external, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::StmEdge>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_edge, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::StmHeuristic>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_heuristic, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::StmScript>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_script, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::StmInclude>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_include, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::StmProgram>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_program, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::StmConst>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_const, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::StmParts>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_parts, &x);
            }
            if constexpr (std::is_same_v<T, CppClingo::Input::StmComment>) {
                return std::make_unique<clingo_ast_t>(std::move(owner), clingo_ast_type_statement_comment, &x);
            }
        },
        term);
}

template <class U, class... T>
auto make_ast(std::shared_ptr<U> owner, std::variant<T...> const &var) -> std::unique_ptr<clingo_ast_t> {
    return std::visit([&owner](auto const &x) { return make_ast(std::move(owner), x); }, var);
}

template <class U, class T>
auto make_ast(std::shared_ptr<U> owner, CppClingo::Util::immutable_value<T> const &ptr)
    -> std::unique_ptr<clingo_ast_t> {
    return make_ast(std::move(owner), *ptr);
}

template <class U, class T>
auto make_ast(std::shared_ptr<U> owner, std::optional<T> const &opt) -> std::unique_ptr<clingo_ast_t> {
    if (opt) {
        return make_ast(std::move(owner), *opt);
    }
    return nullptr;
}

template <class U, class T> auto make_ast_vec(std::shared_ptr<U> const &owner, std::span<T> vec) -> ASTVec {
    ASTVec res{vec.size()};
    size_t i = 0;
    for (auto const &elem : vec) {
        res[i] = make_ast(owner, elem).release();
        ++i;
    }
    return res;
}

template <class U> auto make_ast(U &&value) -> std::unique_ptr<clingo_ast_t> {
    auto owner = std::make_shared<std::remove_cvref_t<U>>(std::forward<U>(value));
    auto *ptr = owner.get();
    return make_ast(std::move(owner), *ptr);
}

template <class U, class T> auto make_ast_vec(std::shared_ptr<U> const &owner, std::vector<T> const &vec) -> ASTVec {
    return make_ast_vec(owner, std::span{vec});
}

template <class U, class T>
auto make_ast_vec(std::shared_ptr<U> const &owner, CppClingo::Util::immutable_array<T> const &vec) -> ASTVec {
    return make_ast_vec(owner, std::span{vec});
}

// }}} make_ast
// {{{ convert

template <class T> constexpr bool is_optional_v = false;

template <class T> constexpr bool is_optional_v<std::optional<T>> = true;

template <class T> constexpr bool is_immutable_v = false;

template <class T> constexpr bool is_immutable_v<CppClingo::Util::immutable_value<T>> = true;

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::Term>
auto convert(clingo_ast_t const *ast) -> T {
    switch (ast->get_type()) {
        case clingo_ast_type_term_variable: {
            return ast->cast<CppClingo::Input::TermVariable>();
        }
        case clingo_ast_type_term_symbolic: {
            return ast->cast<CppClingo::Input::TermSymbol>();
        }
        case clingo_ast_type_term_tuple: {
            return ast->cast<CppClingo::Input::TermTuple>();
        }
        case clingo_ast_type_term_function: {
            return ast->cast<CppClingo::Input::TermFunction>();
        }
        case clingo_ast_type_term_absolute: {
            return ast->cast<CppClingo::Input::TermAbs>();
        }
        case clingo_ast_type_term_unary_operation: {
            return ast->cast<CppClingo::Input::TermUnary>();
        }
        case clingo_ast_type_term_binary_operation: {
            return ast->cast<CppClingo::Input::TermBinary>();
        }
        default: {
            throw std::runtime_error("term expected");
        }
    }
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::TheoryTerm>
auto convert(clingo_ast_t const *ast) -> T {
    switch (ast->get_type()) {
        case clingo_ast_type_theory_term_variable: {
            return ast->cast<CppClingo::Input::TheoryTermVariable>();
        }
        case clingo_ast_type_theory_term_symbolic: {
            return ast->cast<CppClingo::Input::TheoryTermSymbol>();
        }
        case clingo_ast_type_theory_term_tuple: {
            return ast->cast<CppClingo::Input::TheoryTermTuple>();
        }
        case clingo_ast_type_theory_term_function: {
            return ast->cast<CppClingo::Input::TheoryTermFunction>();
        }
        case clingo_ast_type_theory_term_unparsed: {
            return ast->cast<CppClingo::Input::TheoryTermUnparsed>();
        }
        default: {
            throw std::runtime_error("theory term expected");
        }
    }
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::Lit>
auto convert(clingo_ast_t const *ast) -> T {
    switch (ast->get_type()) {
        case clingo_ast_type_literal_boolean: {
            return ast->cast<CppClingo::Input::LitBool>();
        }
        case clingo_ast_type_literal_symbolic: {
            return ast->cast<CppClingo::Input::LitSymbolic>();
        }
        case clingo_ast_type_literal_comparison: {
            return ast->cast<CppClingo::Input::LitComparison>();
        }
        default: {
            throw std::runtime_error("literal expected");
        }
    }
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::Argument>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_projection) {
        return ast->cast<CppClingo::Input::Projection>();
    }
    return convert<CppClingo::Input::Term>(ast);
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::ArgumentTuple>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_argument_tuple) {
        return ast->cast<CppClingo::Input::ArgumentTuple>();
    }
    throw std::runtime_error("argument tuple expected");
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::TupleElement>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_argument_tuple) {
        return ast->cast<CppClingo::Input::ArgumentTuple>();
    }
    return convert<CppClingo::Input::Term>(ast);
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::UnparsedElement>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_unparsed_element) {
        return ast->cast<CppClingo::Input::UnparsedElement>();
    }
    throw std::runtime_error("unparsed element expected");
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::LGuard::value_type>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_left_guard) {
        return ast->cast<CppClingo::Input::LGuard::value_type>();
    }
    throw std::runtime_error("left guard expected");
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::RGuard::value_type>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_right_guard) {
        return ast->cast<CppClingo::Input::RGuard::value_type>();
    }
    throw std::runtime_error("right guard expected");
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::TheoryRGuard>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_theory_right_guard) {
        return ast->cast<CppClingo::Input::TheoryRGuard>();
    }
    throw std::runtime_error("theory right guard expected");
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::SetAggregateElement>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_set_aggregate_element) {
        return ast->cast<CppClingo::Input::SetAggregateElement>();
    }
    throw std::runtime_error("set aggregate element expected");
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::TheoryElement>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_theory_atom_element) {
        return ast->cast<CppClingo::Input::TheoryElement>();
    }
    throw std::runtime_error("theory atom element expected");
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::BdLitAggregateElement>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_body_aggregate_element) {
        return ast->cast<CppClingo::Input::BdLitAggregateElement>();
    }
    throw std::runtime_error("body aggregate element expected");
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::HdLitAggregateElement>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_head_aggregate_element) {
        return ast->cast<CppClingo::Input::HdLitAggregateElement>();
    }
    throw std::runtime_error("body aggregate element expected");
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::HdLitDisjunctionElement>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_head_conditional_literal) {
        return ast->cast<CppClingo::Input::CondLit>();
    }
    return ast->cast<CppClingo::Input::Lit>();
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::HdLit>
auto convert(clingo_ast_t const *ast) -> T {
    switch (ast->get_type()) {
        case clingo_ast_type_head_simple_literal: {
            return ast->cast<CppClingo::Input::HdLitSimple>();
        }
        case clingo_ast_type_head_disjunction: {
            return ast->cast<CppClingo::Input::HdLitDisjunction>();
        }
        case clingo_ast_type_head_theory_atom: {
            return ast->cast<CppClingo::Input::HdLitTheoryAtom>();
        }
        case clingo_ast_type_head_set_aggregate: {
            return ast->cast<CppClingo::Input::HdLitSetAggregate>();
        }
        case clingo_ast_type_head_aggregate: {
            return ast->cast<CppClingo::Input::HdLitAggregate>();
        }
        default: {
            throw std::invalid_argument("head literal expected");
        }
    }
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::BdLit>
auto convert(clingo_ast_t const *ast) -> T {
    switch (ast->get_type()) {
        case clingo_ast_type_body_simple_literal: {
            return ast->cast<CppClingo::Input::BdLitSimple>();
        }
        case clingo_ast_type_body_conditional_literal: {
            return ast->cast<CppClingo::Input::BdLitConjunction>();
        }
        case clingo_ast_type_body_theory_atom: {
            return ast->cast<CppClingo::Input::BdLitTheoryAtom>();
        }
        case clingo_ast_type_body_set_aggregate: {
            return ast->cast<CppClingo::Input::BdLitSetAggregate>();
        }
        case clingo_ast_type_body_aggregate: {
            return ast->cast<CppClingo::Input::BdLitAggregate>();
        }
        default: {
            throw std::invalid_argument("body literal expected");
        }
    }
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::TheoryOpDefinition>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_theory_operator_definition) {
        return ast->cast<CppClingo::Input::TheoryOpDefinition>();
    }
    throw std::runtime_error("theory operator definition expected");
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::TheoryRGuardDefinition>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_theory_guard_definition) {
        return ast->cast<CppClingo::Input::TheoryRGuardDefinition>();
    }
    throw std::runtime_error("theory guard definition expected");
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::TheoryTermDefinition>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_theory_term_definition) {
        return ast->cast<CppClingo::Input::TheoryTermDefinition>();
    }
    throw std::runtime_error("theory term definition expected");
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::TheoryAtomDefinition>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_theory_atom_definition) {
        return ast->cast<CppClingo::Input::TheoryAtomDefinition>();
    }
    throw std::runtime_error("theory atom definition expected");
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::OptimizeTuple>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_optimize_tuple) {
        return ast->cast<CppClingo::Input::OptimizeTuple>();
    }
    throw std::runtime_error("optimize tuple expected");
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::OptimizeElement>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_optimize_element) {
        return ast->cast<CppClingo::Input::OptimizeElement>();
    }
    throw std::runtime_error("optimize tuple expected");
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::ProgramParam>
auto convert(clingo_ast_t const *ast) -> T const & {
    if (ast->get_type() == clingo_ast_type_program_part) {
        return ast->cast<CppClingo::Input::ProgramParam>();
    }
    throw std::runtime_error("program part expected");
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::Edge>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() == clingo_ast_type_edge) {
        return ast->cast<CppClingo::Input::Edge>();
    }
    throw std::runtime_error("edge expected");
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::StmTheory>
auto convert(clingo_ast_t const *ast) -> T {
    if (ast->get_type() != clingo_ast_type_statement_theory) {
        throw std::runtime_error("theory expected");
    }
    return ast->cast<CppClingo::Input::StmTheory>();
}

template <typename T>
    requires std::is_same_v<T, CppClingo::Input::Stm>
auto convert(clingo_ast_t const *ast) -> T {
    switch (ast->get_type()) {
        case clingo_ast_type_statement_rule: {
            return ast->cast<CppClingo::Input::StmRule>();
        }
        case clingo_ast_type_statement_theory: {
            return ast->cast<CppClingo::Input::StmTheory>();
        }
        case clingo_ast_type_statement_optimize: {
            return ast->cast<CppClingo::Input::StmOptimize>();
        }
        case clingo_ast_type_statement_weak_constraint: {
            return ast->cast<CppClingo::Input::StmWeakConstraint>();
        }
        case clingo_ast_type_statement_show: {
            return ast->cast<CppClingo::Input::StmShow>();
        }
        case clingo_ast_type_statement_show_nothing: {
            return ast->cast<CppClingo::Input::StmShowNothing>();
        }
        case clingo_ast_type_statement_show_signature: {
            return ast->cast<CppClingo::Input::StmShowSig>();
        }
        case clingo_ast_type_statement_defined: {
            return ast->cast<CppClingo::Input::StmDefined>();
        }
        case clingo_ast_type_statement_external: {
            return ast->cast<CppClingo::Input::StmExternal>();
        }
        case clingo_ast_type_statement_edge: {
            return ast->cast<CppClingo::Input::StmEdge>();
        }
        case clingo_ast_type_statement_heuristic: {
            return ast->cast<CppClingo::Input::StmHeuristic>();
        }
        case clingo_ast_type_statement_script: {
            return ast->cast<CppClingo::Input::StmScript>();
        }
        case clingo_ast_type_statement_program: {
            return ast->cast<CppClingo::Input::StmProgram>();
        }
        case clingo_ast_type_statement_include: {
            return ast->cast<CppClingo::Input::StmInclude>();
        }
        case clingo_ast_type_statement_const: {
            return ast->cast<CppClingo::Input::StmConst>();
        }
        case clingo_ast_type_statement_parts: {
            return ast->cast<CppClingo::Input::StmParts>();
        }
        case clingo_ast_type_statement_comment: {
            return ast->cast<CppClingo::Input::StmComment>();
        }
        default: {
            throw std::runtime_error("statement expected");
        }
    }
}

template <class T>
    requires is_optional_v<T>
auto convert(clingo_ast_t const *ast) -> T {
    T res;
    if (ast != nullptr) {
        res = convert<typename T::value_type>(ast);
    }
    return res;
}

template <class T>
    requires is_immutable_v<T>
auto convert(clingo_ast_t const *ast) -> T {
    return CppClingo::Util::make_immutable<typename T::element_type>(convert<typename T::element_type>(ast));
}

template <class T> auto convert(clingo_ast_t const **ast, size_t size) -> std::vector<T> {
    std::vector<T> res;
    res.reserve(size);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (auto it = ast, ie = ast + size; it != ie; ++it) {
        res.emplace_back(convert<T>(*it));
    }
    return res;
}

auto convert(clingo_location_t const *loc) -> CppClingo::Location const & {
    return *cpp_cast(loc);
}

auto convert(clingo_symbol_t const *symbols, size_t size) -> CppClingo::SharedSymbolVec {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return CppClingo::Util::to_vec(symbols, symbols + size,
                                   [](auto sym) { return CppClingo::SharedSymbol{CppClingo::Symbol::from_rep(sym)}; });
}

auto convert(clingo_lib_t *lib, clingo_string_t const *array, size_t size) -> CppClingo::SharedStringArray {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    return {array, array + size, [lib](auto str) { return lib->store->string({str.data, str.size}); }};
}

// }}} convert

template <class T, class... A> auto construct_ast(clingo_ast_type_t type, A &&...args) -> clingo_ast_t * {
    auto owner = std::make_shared<T>(std::forward<A>(args)...);
    auto *ptr = owner.get();
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    return new clingo_ast_t{std::move(owner), static_cast<clingo_ast_type_e>(type), ptr};
}

} // namespace
} // namespace CppClingo::CAPI

template <class V>
auto clingo_ast::visit(V &&visit) const -> std::invoke_result_t<V, CppClingo::Input::Projection const &> {
    using namespace CppClingo::Input;
    switch (type_) {
        case clingo_ast_type_projection: {
            return std::invoke(std::forward<V>(visit), cast<Projection>());
        }
        case clingo_ast_type_term_variable: {
            return std::invoke(std::forward<V>(visit), cast<TermVariable>());
        }
        case clingo_ast_type_term_symbolic: {
            return std::invoke(std::forward<V>(visit), cast<TermSymbol>());
        }
        case clingo_ast_type_term_absolute: {
            return std::invoke(std::forward<V>(visit), cast<TermAbs>());
        }
        case clingo_ast_type_term_unary_operation: {
            return std::invoke(std::forward<V>(visit), cast<TermUnary>());
        }
        case clingo_ast_type_term_binary_operation: {
            return std::invoke(std::forward<V>(visit), cast<TermBinary>());
        }
        case clingo_ast_type_term_tuple: {
            return std::invoke(std::forward<V>(visit), cast<TermTuple>());
        }
        case clingo_ast_type_term_function: {
            return std::invoke(std::forward<V>(visit), cast<TermFunction>());
        }
        case clingo_ast_type_argument_tuple: {
            return std::invoke(std::forward<V>(visit), cast<ArgumentTuple>());
        }
        case clingo_ast_type_left_guard: {
            return std::invoke(std::forward<V>(visit), cast<LGuard::value_type>());
        }
        case clingo_ast_type_right_guard: {
            return std::invoke(std::forward<V>(visit), cast<RGuard::value_type>());
        }
        case clingo_ast_type_unparsed_element: {
            return std::invoke(std::forward<V>(visit), cast<UnparsedElement>());
        }
        case clingo_ast_type_theory_term_variable: {
            return std::invoke(std::forward<V>(visit), cast<TheoryTermVariable>());
        }
        case clingo_ast_type_theory_term_symbolic: {
            return std::invoke(std::forward<V>(visit), cast<TheoryTermSymbol>());
        }
        case clingo_ast_type_theory_term_tuple: {
            return std::invoke(std::forward<V>(visit), cast<TheoryTermTuple>());
        }
        case clingo_ast_type_theory_term_function: {
            return std::invoke(std::forward<V>(visit), cast<TheoryTermFunction>());
        }
        case clingo_ast_type_theory_term_unparsed: {
            return std::invoke(std::forward<V>(visit), cast<TheoryTermUnparsed>());
        }
        case clingo_ast_type_literal_boolean: {
            return std::invoke(std::forward<V>(visit), cast<LitBool>());
        }
        case clingo_ast_type_literal_comparison: {
            return std::invoke(std::forward<V>(visit), cast<LitComparison>());
        }
        case clingo_ast_type_literal_symbolic: {
            return std::invoke(std::forward<V>(visit), cast<LitSymbolic>());
        }
        case clingo_ast_type_set_aggregate_element: {
            return std::invoke(std::forward<V>(visit), cast<SetAggregateElement>());
        }
        case clingo_ast_type_theory_atom_element: {
            return std::invoke(std::forward<V>(visit), cast<TheoryElement>());
        }
        case clingo_ast_type_theory_right_guard: {
            return std::invoke(std::forward<V>(visit), cast<TheoryRGuard>());
        }
        case clingo_ast_type_body_simple_literal: {
            return std::invoke(std::forward<V>(visit), cast<BdLitSimple>());
        }
        case clingo_ast_type_body_aggregate_element: {
            return std::invoke(std::forward<V>(visit), cast<BdLitAggregateElement>());
        }
        case clingo_ast_type_body_aggregate: {
            return std::invoke(std::forward<V>(visit), cast<BdLitAggregate>());
        }
        case clingo_ast_type_body_set_aggregate: {
            return std::invoke(std::forward<V>(visit), cast<BdLitSetAggregate>());
        }
        case clingo_ast_type_body_theory_atom: {
            return std::invoke(std::forward<V>(visit), cast<BdLitTheoryAtom>());
        }
        case clingo_ast_type_body_conditional_literal: {
            return std::invoke(std::forward<V>(visit), cast<BdLitConjunction>());
        }
        case clingo_ast_type_head_simple_literal: {
            return std::invoke(std::forward<V>(visit), cast<HdLitSimple>());
        }
        case clingo_ast_type_head_aggregate_element: {
            return std::invoke(std::forward<V>(visit), cast<HdLitAggregateElement>());
        }
        case clingo_ast_type_head_aggregate: {
            return std::invoke(std::forward<V>(visit), cast<HdLitAggregate>());
        }
        case clingo_ast_type_head_set_aggregate: {
            return std::invoke(std::forward<V>(visit), cast<HdLitSetAggregate>());
        }
        case clingo_ast_type_head_theory_atom: {
            return std::invoke(std::forward<V>(visit), cast<HdLitTheoryAtom>());
        }
        case clingo_ast_type_head_conditional_literal: {
            return std::invoke(std::forward<V>(visit), cast<CondLit>());
        }
        case clingo_ast_type_head_disjunction: {
            return std::invoke(std::forward<V>(visit), cast<HdLitDisjunction>());
        }
        case clingo_ast_type_statement_rule: {
            return std::invoke(std::forward<V>(visit), cast<StmRule>());
        }
        case clingo_ast_type_theory_operator_definition: {
            return std::invoke(std::forward<V>(visit), cast<TheoryOpDefinition>());
        }
        case clingo_ast_type_theory_term_definition: {
            return std::invoke(std::forward<V>(visit), cast<TheoryTermDefinition>());
        }
        case clingo_ast_type_theory_guard_definition: {
            return std::invoke(std::forward<V>(visit), cast<TheoryRGuardDefinition>());
        }
        case clingo_ast_type_theory_atom_definition: {
            return std::invoke(std::forward<V>(visit), cast<TheoryAtomDefinition>());
        }
        case clingo_ast_type_statement_theory: {
            return std::invoke(std::forward<V>(visit), cast<StmTheory>());
        }
        case clingo_ast_type_optimize_tuple: {
            return std::invoke(std::forward<V>(visit), cast<OptimizeTuple>());
        }
        case clingo_ast_type_optimize_element: {
            return std::invoke(std::forward<V>(visit), cast<OptimizeElement>());
        }
        case clingo_ast_type_statement_optimize: {
            return std::invoke(std::forward<V>(visit), cast<StmOptimize>());
        }
        case clingo_ast_type_statement_weak_constraint: {
            return std::invoke(std::forward<V>(visit), cast<StmWeakConstraint>());
        }
        case clingo_ast_type_program_part: {
            return std::invoke(std::forward<V>(visit), cast<ProgramParam>());
        }
        case clingo_ast_type_edge: {
            return std::invoke(std::forward<V>(visit), cast<Edge>());
        }
        case clingo_ast_type_statement_show: {
            return std::invoke(std::forward<V>(visit), cast<StmShow>());
        }
        case clingo_ast_type_statement_show_nothing: {
            return std::invoke(std::forward<V>(visit), cast<StmShowNothing>());
        }
        case clingo_ast_type_statement_show_signature: {
            return std::invoke(std::forward<V>(visit), cast<StmShowSig>());
        }
        case clingo_ast_type_statement_project: {
            return std::invoke(std::forward<V>(visit), cast<StmProject>());
        }
        case clingo_ast_type_statement_project_signature: {
            return std::invoke(std::forward<V>(visit), cast<StmProjectSig>());
        }
        case clingo_ast_type_statement_defined: {
            return std::invoke(std::forward<V>(visit), cast<StmDefined>());
        }
        case clingo_ast_type_statement_external: {
            return std::invoke(std::forward<V>(visit), cast<StmExternal>());
        }
        case clingo_ast_type_statement_edge: {
            return std::invoke(std::forward<V>(visit), cast<StmEdge>());
        }
        case clingo_ast_type_statement_heuristic: {
            return std::invoke(std::forward<V>(visit), cast<StmHeuristic>());
        }
        case clingo_ast_type_statement_include: {
            return std::invoke(std::forward<V>(visit), cast<StmInclude>());
        }
        case clingo_ast_type_statement_program: {
            return std::invoke(std::forward<V>(visit), cast<StmProgram>());
        }
        case clingo_ast_type_statement_script: {
            return std::invoke(std::forward<V>(visit), cast<StmScript>());
        }
        case clingo_ast_type_statement_const: {
            return std::invoke(std::forward<V>(visit), cast<StmConst>());
        }
        case clingo_ast_type_statement_parts: {
            return std::invoke(std::forward<V>(visit), cast<StmParts>());
        }
        case clingo_ast_type_statement_comment: {
            return std::invoke(std::forward<V>(visit), cast<StmComment>());
        }
    }
    throw std::invalid_argument("invalid ast type");
}

auto clingo_ast::get_type() const -> clingo_ast_type_e {
    return type_;
}

auto clingo_ast::get_location(clingo_ast_attribute_t attr) const -> std::optional<clingo_location_t const *> {
    using namespace CppClingo::Input;
    if (attr != clingo_ast_attribute_location) {
        return std::nullopt;
    }
    return visit([]<class T>(T const &x) -> std::optional<clingo_location_t const *> {
        if constexpr (requires(T const &x) { x.loc(); }) {
            return c_cast(&x.loc());
        }
        return std::nullopt;
    });
}

#define SWITCH(...)                                                                                                    \
    using namespace CppClingo::Input;                                                                                  \
    switch (type_) {                                                                                                   \
        __VA_ARGS__                                                                                                    \
        default: {                                                                                                     \
            return std::nullopt;                                                                                       \
        }                                                                                                              \
    }
#define TYPE(type, type_name, ...)                                                                                     \
    case clingo_ast_type_##type: {                                                                                     \
        using Type = type_name;                                                                                        \
        switch (attr) {                                                                                                \
            __VA_ARGS__                                                                                                \
            default: {                                                                                                 \
                return std::nullopt;                                                                                   \
            }                                                                                                          \
        }                                                                                                              \
    }
#define ATTR(attr, value)                                                                                              \
    case clingo_ast_attribute_##attr: {                                                                                \
        return static_cast<int>(cast<Type>().value);                                                                   \
    }

auto clingo_ast::get_number(clingo_ast_attribute_t attr) const -> std::optional<int>{
    // clang-format off
    SWITCH(
        TYPE(term_variable, TermVariable,
            ATTR(anonymous, anonymous()))
        TYPE(theory_term_variable, TheoryTermVariable,
            ATTR(anonymous, anonymous()))
        TYPE(term_function, TermFunction,
            ATTR(external, external()))
        TYPE(term_unary_operation, TermUnary,
            ATTR(operator_type, op()))
        TYPE(term_binary_operation, TermBinary,
            ATTR(operator_type, op()))
        TYPE(theory_term_tuple, TheoryTermTuple,
            ATTR(tuple_type, type()))
        TYPE(literal_boolean, LitBool,
            ATTR(sign, sign())
            ATTR(value, value()))
        TYPE(literal_symbolic, LitSymbolic,
            ATTR(sign, sign()))
        TYPE(literal_comparison, LitComparison,
            ATTR(sign, sign()))
        TYPE(left_guard, LGuard::value_type,
            ATTR(relation, second))
        TYPE(right_guard, RGuard::value_type,
            ATTR(relation, first))
        TYPE(body_theory_atom, BdLitTheoryAtom,
            ATTR(sign, sign()))
        TYPE(body_set_aggregate, BdLitSetAggregate,
            ATTR(sign, sign()))
        TYPE(head_aggregate, HdLitAggregate,
            ATTR(function, fun()))
        TYPE(body_aggregate, BdLitAggregate,
            ATTR(sign, sign())
            ATTR(function, fun()))
        TYPE(theory_operator_definition, TheoryOpDefinition,
            ATTR(priority, prio())
            ATTR(operator_type, type()))
        TYPE(theory_atom_definition, TheoryAtomDefinition,
            ATTR(arity, arity())
            ATTR(atom_type, type()))
        TYPE(statement_optimize, StmOptimize,
            ATTR(optimize_type, type()))
        TYPE(statement_show_signature, StmShowSig,
            ATTR(sign, sign())
            ATTR(arity, arity()))
        TYPE(statement_project_signature, StmProjectSig,
            ATTR(sign, sign())
            ATTR(arity, arity()))
        TYPE(statement_defined, StmDefined,
            ATTR(sign, sign())
            ATTR(arity, arity()))
        TYPE(statement_include, StmInclude,
            ATTR(include_type, type()))
        TYPE(statement_const, StmConst,
            ATTR(precedence, type()))
        TYPE(statement_parts, StmParts,
            ATTR(precedence, type()))
        TYPE(statement_comment, StmComment,
            ATTR(comment_type, type())))
    // clang-format on
}

#undef ATTR
#define ATTR(attr, value)                                                                                              \
    case clingo_ast_attribute_##attr: {                                                                                \
        return static_cast<clingo_symbol_t>(CppClingo::Symbol::to_rep(cast<Type>().value));                            \
    }

[[nodiscard]] auto clingo_ast::get_symbol(clingo_ast_attribute_t attr) const -> std::optional<clingo_symbol_t>{
    // clang-format off
    SWITCH(
        TYPE(term_symbolic, TermSymbol,
            ATTR(symbol, value()))
        TYPE(theory_term_symbolic, TheoryTermSymbol,
            ATTR(symbol, value())))
    // clang-format on
}

#undef ATTR
#define ATTR(attr, value)                                                                                              \
    case clingo_ast_attribute_##attr: {                                                                                \
        return cast<Type>().value.view();                                                                              \
    }

auto clingo_ast::get_string(clingo_ast_attribute_t attr) const -> std::optional<std::string_view>{
    // clang-format off
    SWITCH(
        TYPE(term_variable, TermVariable,
            ATTR(name, name()))
        TYPE(theory_term_variable, TheoryTermVariable,
            ATTR(name, name()))
        TYPE(term_function, TermFunction,
            ATTR(name, name()))
        TYPE(theory_term_function, TheoryTermFunction,
            ATTR(name, name()))
        TYPE(theory_right_guard, TheoryRGuard,
            ATTR(theory_operator, op()))
        TYPE(theory_operator_definition, TheoryOpDefinition,
            ATTR(name, op()))
        TYPE(theory_term_definition, TheoryTermDefinition,
            ATTR(name, name()))
        TYPE(theory_guard_definition, TheoryRGuardDefinition,
            ATTR(term, term()))
        TYPE(theory_atom_definition, TheoryAtomDefinition,
            ATTR(name, name())
            ATTR(term, term()))
        TYPE(statement_theory, StmTheory,
            ATTR(name, name()))
        TYPE(statement_show_signature, StmShowSig,
            ATTR(name, name()))
        TYPE(statement_project_signature, StmProjectSig,
            ATTR(name, name()))
        TYPE(statement_defined, StmDefined,
            ATTR(name, name()))
        TYPE(statement_include, StmInclude,
            ATTR(value, value()))
        TYPE(statement_program, StmProgram,
            ATTR(name, name()))
        TYPE(statement_script, StmScript,
            ATTR(script_type, type())
            ATTR(value, value()))
        TYPE(statement_const, StmConst,
            ATTR(name, name()))
        TYPE(program_part, ProgramParam,
            ATTR(name, first.get()))
        TYPE(statement_comment, StmComment,
            ATTR(value, value())))
    // clang-format on
}

#undef ATTR
#define ATTR(attr, value)                                                                                              \
    case clingo_ast_attribute_##attr: {                                                                                \
        return cast<Type>().value;                                                                                     \
    }

auto clingo_ast::get_string_vec(clingo_ast_attribute_t attr) const -> std::optional<CppClingo::StringSpan>{
    // clang-format off
    SWITCH(
        TYPE(unparsed_element, UnparsedElement,
            ATTR(operators, ops()))
        TYPE(theory_guard_definition, TheoryRGuardDefinition,
            ATTR(operators, ops()))
        TYPE(statement_program, StmProgram,
            ATTR(arguments, args())))
    // clang-format on
}

#undef ATTR
#define ATTR(attr, value)                                                                                              \
    case clingo_ast_attribute_##attr: {                                                                                \
        return as_symbol_span(cast<Type>().value);                                                                     \
    }

auto clingo_ast::get_symbol_vec(clingo_ast_attribute_t attr) const -> std::optional<CppClingo::SymbolSpan>{
    // clang-format off
    SWITCH(
        TYPE(program_part, ProgramParam,
            ATTR(arguments, second)))
    // clang-format on
}

#undef ATTR
#define ATTR(attr, value)                                                                                              \
    case clingo_ast_attribute_##attr: {                                                                                \
        return make_ast(ptr_, cast<Type>().value);                                                                     \
    }

auto clingo_ast::get_ast(clingo_ast_attribute_t attr) const -> std::optional<std::unique_ptr<clingo_ast_t>>{
    // clang-format off
    SWITCH(
        TYPE(term_unary_operation, TermUnary,
            ATTR(right, rhs()))
        TYPE(term_binary_operation, TermBinary,
            ATTR(left, lhs())
            ATTR(right, rhs()))
        TYPE(unparsed_element, UnparsedElement,
            ATTR(term, term()))
        TYPE(literal_comparison, LitComparison,
            ATTR(left, lhs()))
        TYPE(literal_symbolic, LitSymbolic,
            ATTR(atom, term()))
        TYPE(head_simple_literal, HdLitSimple,
            ATTR(literal, lit()))
        TYPE(body_simple_literal, BdLitSimple,
            ATTR(literal, lit()))
        TYPE(head_conditional_literal, CondLit,
            ATTR(literal, lit()))
        TYPE(body_conditional_literal, BdLitConjunction,
            ATTR(literal, lit().lit()))
        TYPE(left_guard, LGuard::value_type,
            ATTR(term, first))
        TYPE(right_guard, RGuard::value_type,
            ATTR(term, second))
        TYPE(theory_right_guard, TheoryRGuard,
            ATTR(term, term()))
        TYPE(set_aggregate_element, SetAggregateElement,
            ATTR(literal, lit()))
        TYPE(head_aggregate_element, HdLitAggregateElement,
            ATTR(literal, lit()))
        TYPE(body_theory_atom, BdLitTheoryAtom,
            ATTR(name, name())
            ATTR(right, rhs()))
        TYPE(head_theory_atom, HdLitTheoryAtom,
            ATTR(name, name())
            ATTR(right, rhs()))
        TYPE(head_set_aggregate, HdLitSetAggregate,
            ATTR(left, lhs())
            ATTR(right, rhs()))
        TYPE(head_aggregate, HdLitAggregate,
            ATTR(left, lhs())
            ATTR(right, rhs()))
        TYPE(body_set_aggregate, BdLitSetAggregate,
            ATTR(left, lhs())
            ATTR(right, rhs()))
        TYPE(body_aggregate, BdLitAggregate,
            ATTR(left, lhs())
            ATTR(right, rhs()))
        TYPE(statement_rule, StmRule,
            ATTR(head, head()))
        TYPE(theory_atom_definition, TheoryAtomDefinition,
            ATTR(guard, rhs()))
        TYPE(optimize_tuple, OptimizeTuple,
            ATTR(weight, weight())
            ATTR(priority, prio()))
        TYPE(optimize_element, OptimizeElement,
            ATTR(tuple, tuple()))
        TYPE(statement_weak_constraint, StmWeakConstraint,
            ATTR(tuple, tuple()))
        TYPE(statement_show, StmShow,
            ATTR(term, term()))
        TYPE(statement_project, StmProject,
            ATTR(atom, atom()))
        TYPE(statement_external, StmExternal,
            ATTR(atom, atom())
            ATTR(external_type, type()))
        TYPE(edge, Edge,
            ATTR(u, src())
            ATTR(v, dst()))
        TYPE(statement_heuristic, StmHeuristic,
            ATTR(atom, atom())
            ATTR(weight, weight())
            ATTR(priority, prio())
            ATTR(modifier, type()))
        TYPE(statement_const, StmConst,
            ATTR(value, value())))
    // clang-format on
}

#undef ATTR
#define ATTR(attr, value)                                                                                              \
    case clingo_ast_attribute_##attr: {                                                                                \
        return make_ast_vec(ptr_, cast<Type>().value);                                                                 \
    }

auto clingo_ast::get_ast_vec(clingo_ast_attribute_t attr) const -> std::optional<ASTVec> {
    // clang-format off
    SWITCH(
        TYPE(argument_tuple, ArgumentTuple,
            ATTR(arguments, elems()))
        TYPE(term_absolute, TermAbs,
            ATTR(pool, pool()))
        TYPE(term_tuple, TermTuple,
            ATTR(pool, pool()))
        TYPE(term_function, TermFunction,
            ATTR(pool, pool()))
        TYPE(theory_term_tuple, TheoryTermTuple,
            ATTR(arguments, elems()))
        TYPE(theory_term_function, TheoryTermFunction,
            ATTR(arguments, args()))
        TYPE(theory_term_unparsed, TheoryTermUnparsed,
            ATTR(elements, elems()))
        TYPE(literal_comparison, LitComparison,
            ATTR(right, rhs()))
        TYPE(head_conditional_literal, CondLit,
            ATTR(condition, cond()))
        TYPE(body_conditional_literal, BdLitConjunction,
            ATTR(condition, lit().cond()))
        TYPE(set_aggregate_element, SetAggregateElement,
            ATTR(condition, cond()))
        TYPE(theory_atom_element, TheoryElement,
            ATTR(tuple, tuple())
            ATTR(condition, cond()))
        TYPE(head_aggregate_element, HdLitAggregateElement,
            ATTR(tuple, tuple())
            ATTR(condition, cond()))
        TYPE(body_aggregate_element, BdLitAggregateElement,
            ATTR(tuple, tuple())
            ATTR(condition, cond()))
        TYPE(head_disjunction, HdLitDisjunction,
            ATTR(elements, elems()))
        TYPE(body_theory_atom, BdLitTheoryAtom,
            ATTR(elements, elems()))
        TYPE(head_theory_atom, HdLitTheoryAtom,
            ATTR(elements, elems()))
        TYPE(head_set_aggregate, HdLitSetAggregate,
            ATTR(elements, elems()))
        TYPE(head_aggregate, HdLitAggregate,
            ATTR(elements, elems()))
        TYPE(body_set_aggregate, BdLitSetAggregate,
            ATTR(elements, elems()))
        TYPE(body_aggregate, BdLitAggregate,
            ATTR(elements, elems()))
        TYPE(statement_rule, StmRule,
            ATTR(body, body()))
        TYPE(theory_term_definition, TheoryTermDefinition,
            ATTR(operators, op_defs()))
        TYPE(statement_theory, StmTheory,
            ATTR(terms, term_defs())
            ATTR(atoms, atom_defs()))
        TYPE(optimize_tuple, OptimizeTuple,
            ATTR(terms, terms()))
        TYPE(optimize_element, OptimizeElement,
            ATTR(condition, cond()))
        TYPE(statement_optimize, StmOptimize,
            ATTR(elements, elems()))
        TYPE(statement_weak_constraint, StmWeakConstraint,
            ATTR(body, body()))
        TYPE(statement_show, StmShow,
            ATTR(body, body()))
        TYPE(statement_project, StmProject,
            ATTR(body, body()))
        TYPE(statement_external, StmExternal,
            ATTR(body, body()))
        TYPE(statement_edge, StmEdge,
            ATTR(pool, edges())
            ATTR(body, body()))
        TYPE(statement_heuristic, StmHeuristic,
            ATTR(body, body()))
        TYPE(statement_parts, StmParts,
            ATTR(elements, elems())))
    // clang-format on
}

#undef ATTR
#undef TYPE
#undef SWITCH

auto clingo_ast::copy() const -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(*this);
}

template <class T> void clingo_ast::print(T &out) const {
    using namespace CppClingo::Input;
    visit([&out]<class U>(U const &x) {
        if constexpr (CppClingo::Util::matches<U, RGuard::value_type>) {
            out << " " << x.first << " " << x.second;
        } else if constexpr (CppClingo::Util::matches<U, TheoryRGuard>) {
            out << " " << x.op() << " " << x.term();
        } else if constexpr (std::is_same_v<U, UnparsedElement>) {
            for (auto const &op : x.ops()) {
                out << op << " ";
            }
            out << x.term();
        } else if constexpr (std::is_same_v<U, ArgumentTuple>) {
            bool comma = false;
            for (auto const &elem : x.elems()) {
                if (comma) {
                    out << ",";
                } else {
                    comma = true;
                }
                std::visit([&out](auto &x) { out << x; }, elem);
            }
        } else if constexpr (std::is_same_v<U, LGuard::value_type>) {
            out << x.first << " " << x.second << " ";
        } else if constexpr (std::is_same_v<U, ProgramParam>) {
            out << *x.first;
            if (!x.second.empty()) {
                out << "(" << *x.second.front();
                for (auto it = x.second.begin() + 1, ie = x.second.end(); it != ie; ++it) {
                    out << **it;
                }
                out << ")";
            }
        } else {
            out << x;
        }
    });
}

auto clingo_ast::hash() const -> size_t {
    using namespace CppClingo::Input;
    return visit([this](auto &x) {
        return CppClingo::Util::hash_mix(CppClingo::Util::value_hash_record<clingo_ast>(type_, x));
    });
}

auto clingo_ast::equal(clingo_ast_t const &other) const -> bool {
    return type_ == other.type_ &&
           visit([&other](auto const &x) { return x == other.cast<std::decay_t<decltype(x)>>(); });
}

auto clingo_ast::compare(clingo_ast_t const &other) const -> std::strong_ordering {
    if (type_ != other.type_) {
        return type_ <=> other.type_;
    }
    return visit([&other](auto const &x) { return x <=> other.cast<std::decay_t<decltype(x)>>(); });
}

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
extern "C" auto clingo_ast_construct(clingo_lib_t *lib, clingo_ast_type_t type, clingo_ast_t **ast, ...) -> bool {
    using namespace CppClingo::Input;
    CLINGO_TRY {
        if (lib == nullptr || ast == nullptr) {
            return fail_arguments();
        }
        *ast = nullptr;
        switch (static_cast<clingo_ast_type_e>(type)) {
            case clingo_ast_type_projection: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::Projection>(type, convert(loc));
                break;
            }
            case clingo_ast_type_term_variable: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                size_t size = va_arg(args, size_t);
                auto anonymous = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::TermVariable>(type, convert(loc),
                                                                     *lib->store->string({name, size}), anonymous != 0);
                break;
            }
            case clingo_ast_type_term_symbolic: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sym = va_arg(args, clingo_symbol_t);
                va_end(args);
                *ast =
                    construct_ast<CppClingo::Input::TermSymbol>(type, convert(loc), CppClingo::Symbol::from_rep(sym));
                break;
            }
            case clingo_ast_type_term_tuple: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **pool = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::TermTuple>(type, convert(loc),
                                                                  convert<CppClingo::Input::TupleElement>(pool, size));
                break;
            }
            case clingo_ast_type_term_function: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                size_t name_size = va_arg(args, size_t);
                auto const **pool = va_arg(args, clingo_ast_t const **);
                auto pool_size = va_arg(args, size_t);
                auto sign = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::TermFunction>(
                    type, convert(loc), *lib->store->string({name, name_size}),
                    convert<CppClingo::Input::ArgumentTuple>(pool, pool_size), sign != 0);
                break;
            }
            case clingo_ast_type_term_absolute: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **pool = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::TermAbs>(type, convert(loc),
                                                                convert<CppClingo::Input::Term>(pool, size));
                break;
            }
            case clingo_ast_type_term_unary_operation: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto op = va_arg(args, int);
                auto const *rhs = va_arg(args, clingo_ast_t *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::TermUnary>(
                    type, convert(loc), static_cast<CppClingo::Input::UnaryOperator>(op),
                    convert<CppClingo::Util::immutable_value<CppClingo::Input::Term>>(rhs));
                break;
            }
            case clingo_ast_type_term_binary_operation: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *lhs = va_arg(args, clingo_ast_t *);
                auto op = va_arg(args, int);
                auto const *rhs = va_arg(args, clingo_ast_t *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::TermBinary>(
                    type, convert(loc), convert<CppClingo::Util::immutable_value<CppClingo::Input::Term>>(lhs),
                    static_cast<CppClingo::Input::BinaryOperator>(op),
                    convert<CppClingo::Util::immutable_value<CppClingo::Input::Term>>(rhs));
                break;
            }
            case clingo_ast_type_argument_tuple: {
                std::va_list args;
                va_start(args, ast);
                auto const **tuple = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::ArgumentTuple>(type,
                                                                      convert<CppClingo::Input::Argument>(tuple, size));
                break;
            }
            case clingo_ast_type_left_guard: {
                std::va_list args;
                va_start(args, ast);
                auto const *left = va_arg(args, clingo_ast_t const *);
                auto right = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::LGuard::value_type>(type, convert<CppClingo::Input::Term>(left),
                                                                           static_cast<CppClingo::Relation>(right));
                break;
            }
            case clingo_ast_type_right_guard: {
                std::va_list args;
                va_start(args, ast);
                auto left = va_arg(args, int);
                auto const *right = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::RGuard::value_type>(type, static_cast<CppClingo::Relation>(left),
                                                                           convert<CppClingo::Input::Term>(right));
                break;
            }
            case clingo_ast_type_literal_boolean: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sign = va_arg(args, int);
                auto value = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::LitBool>(type, convert(loc), static_cast<CppClingo::Sign>(sign),
                                                                value != 0);
                break;
            }
            case clingo_ast_type_literal_symbolic: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sign = va_arg(args, int);
                auto const *atom = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::LitSymbolic>(
                    type, convert(loc), static_cast<CppClingo::Sign>(sign), convert<CppClingo::Input::Term>(atom));
                break;
            }
            case clingo_ast_type_literal_comparison: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sign = va_arg(args, int);
                auto const *left = va_arg(args, clingo_ast_t const *);
                auto const **right = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::LitComparison>(
                    type, convert(loc), static_cast<CppClingo::Sign>(sign), convert<CppClingo::Input::Term>(left),
                    convert<CppClingo::Input::Guard>(right, size));
                break;
            }
            case clingo_ast_type_unparsed_element: {
                std::va_list args;
                va_start(args, ast);
                auto const *ops = va_arg(args, clingo_string_t const *);
                auto size = va_arg(args, size_t);
                auto const *term = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::UnparsedElement>(type, convert(lib, ops, size),
                                                                        convert<CppClingo::Input::TheoryTerm>(term));
                break;
            }
            case clingo_ast_type_theory_term_variable: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto name_size = va_arg(args, size_t);
                auto anonymous = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::TheoryTermVariable>(
                    type, convert(loc), *lib->store->string({name, name_size}), anonymous != 0);
                break;
            }
            case clingo_ast_type_theory_term_symbolic: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sym = va_arg(args, clingo_symbol_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::TheoryTermSymbol>(type, convert(loc),
                                                                         CppClingo::Symbol::from_rep(sym));
                break;
            }
            case clingo_ast_type_theory_term_tuple: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto tuple_type = va_arg(args, int);
                auto const **arguments = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::TheoryTermTuple>(
                    type, convert(loc), static_cast<CppClingo::TheoryTermTupleType>(tuple_type),
                    convert<CppClingo::Input::TheoryTerm>(arguments, size));
                break;
            }
            case clingo_ast_type_theory_term_function: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto name_size = va_arg(args, size_t);
                auto const **arguments = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::TheoryTermFunction>(
                    type, convert(loc), *lib->store->string({name, name_size}),
                    convert<CppClingo::Input::TheoryTerm>(arguments, size));
                break;
            }
            case clingo_ast_type_theory_term_unparsed: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **elems = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::TheoryTermUnparsed>(
                    type, convert(loc), convert<CppClingo::Input::UnparsedElement>(elems, size));
                break;
            }
            case clingo_ast_type_set_aggregate_element: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *lit = va_arg(args, clingo_ast_t const *);
                auto const **cond = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::SetAggregateElement>(type, convert(loc), convert<Lit>(lit),
                                                                            convert<CppClingo::Input::Lit>(cond, size));
                break;
            }
            case clingo_ast_type_theory_atom_element: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **tuple = va_arg(args, clingo_ast_t const **);
                auto tuple_size = va_arg(args, size_t);
                auto const **cond = va_arg(args, clingo_ast_t const **);
                auto cond_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::TheoryElement>(
                    type, convert(loc), convert<CppClingo::Input::TheoryTerm>(tuple, tuple_size),
                    convert<CppClingo::Input::Lit>(cond, cond_size));
                break;
            }
            case clingo_ast_type_theory_right_guard: {
                std::va_list args;
                va_start(args, ast);
                auto const *op = va_arg(args, char const *);
                auto op_size = va_arg(args, size_t);
                auto const *term = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::TheoryRGuard>(type, *lib->store->string({op, op_size}),
                                                                     convert<CppClingo::Input::TheoryTerm>(term));
                break;
            }
            case clingo_ast_type_body_simple_literal: {
                std::va_list args;
                va_start(args, ast);
                auto const *lit = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::BdLitSimple>(type, convert<CppClingo::Input::Lit>(lit));
                break;
            }
            case clingo_ast_type_body_aggregate_element: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **tuple = va_arg(args, clingo_ast_t const **);
                auto tuple_size = va_arg(args, size_t);
                auto const **cond = va_arg(args, clingo_ast_t const **);
                auto cond_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::BdLitAggregateElement>(
                    type, convert(loc), convert<CppClingo::Input::Term>(tuple, tuple_size),
                    convert<CppClingo::Input::Lit>(cond, cond_size));
                break;
            }
            case clingo_ast_type_body_aggregate: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sign = va_arg(args, int);
                auto const *lhs = va_arg(args, clingo_ast_t const *);
                auto fun = va_arg(args, int);
                auto const **elems = va_arg(args, clingo_ast_t const **);
                auto elems_size = va_arg(args, size_t);
                auto const *rhs = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::BdLitAggregate>(
                    type, convert(loc), static_cast<CppClingo::Sign>(sign), convert<CppClingo::Input::LGuard>(lhs),
                    static_cast<CppClingo::AggregateFunction>(fun),
                    convert<CppClingo::Input::BdLitAggregateElement>(elems, elems_size),
                    convert<CppClingo::Input::RGuard>(rhs));
                break;
            }
            case clingo_ast_type_body_set_aggregate: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sign = va_arg(args, int);
                auto const *lhs = va_arg(args, clingo_ast_t const *);
                auto const **elems = va_arg(args, clingo_ast_t const **);
                auto elems_size = va_arg(args, size_t);
                auto const *rhs = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::BdLitSetAggregate>(
                    type, convert(loc), static_cast<CppClingo::Sign>(sign), convert<CppClingo::Input::LGuard>(lhs),
                    convert<CppClingo::Input::SetAggregateElement>(elems, elems_size),
                    convert<CppClingo::Input::RGuard>(rhs));
                break;
            }
            case clingo_ast_type_body_theory_atom: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sign = va_arg(args, int);
                auto const *term = va_arg(args, clingo_ast_t const *);
                auto const **elems = va_arg(args, clingo_ast_t const **);
                auto elems_size = va_arg(args, size_t);
                auto const *rhs = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::BdLitTheoryAtom>(
                    type, convert(loc), static_cast<CppClingo::Sign>(sign), convert<CppClingo::Input::Term>(term),
                    convert<CppClingo::Input::TheoryElement>(elems, elems_size),
                    convert<std::optional<CppClingo::Input::TheoryRGuard>>(rhs));
                break;
            }
            case clingo_ast_type_body_conditional_literal: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *lit = va_arg(args, clingo_ast_t const *);
                auto const **cond = va_arg(args, clingo_ast_t const **);
                auto cond_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::BdLitConjunction>(
                    type, CppClingo::Input::CondLit{convert(loc), convert<CppClingo::Input::Lit>(lit),
                                                    convert<CppClingo::Input::Lit>(cond, cond_size)});
                break;
            }
            case clingo_ast_type_head_simple_literal: {
                std::va_list args;
                va_start(args, ast);
                auto const *lit = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::HdLitSimple>(type, convert<CppClingo::Input::Lit>(lit));
                break;
            }
            case clingo_ast_type_head_aggregate_element: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **tuple = va_arg(args, clingo_ast_t const **);
                auto tuple_size = va_arg(args, size_t);
                auto const *lit = va_arg(args, clingo_ast_t const *);
                auto const **cond = va_arg(args, clingo_ast_t const **);
                auto cond_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::HdLitAggregateElement>(
                    type, convert(loc), convert<CppClingo::Input::Term>(tuple, tuple_size), convert<Lit>(lit),
                    convert<CppClingo::Input::Lit>(cond, cond_size));
                break;
            }
            case clingo_ast_type_head_aggregate: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *lhs = va_arg(args, clingo_ast_t const *);
                auto fun = va_arg(args, int);
                auto const **elems = va_arg(args, clingo_ast_t const **);
                auto elems_size = va_arg(args, size_t);
                auto const *rhs = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::HdLitAggregate>(
                    type, convert(loc), convert<CppClingo::Input::LGuard>(lhs),
                    static_cast<CppClingo::AggregateFunction>(fun),
                    convert<CppClingo::Input::HdLitAggregateElement>(elems, elems_size),
                    convert<CppClingo::Input::RGuard>(rhs));
                break;
            }
            case clingo_ast_type_head_set_aggregate: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *lhs = va_arg(args, clingo_ast_t const *);
                auto const **elems = va_arg(args, clingo_ast_t const **);
                auto elems_size = va_arg(args, size_t);
                auto const *rhs = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::HdLitSetAggregate>(
                    type, convert(loc), convert<CppClingo::Input::LGuard>(lhs),
                    convert<CppClingo::Input::SetAggregateElement>(elems, elems_size),
                    convert<CppClingo::Input::RGuard>(rhs));
                break;
            }
            case clingo_ast_type_head_theory_atom: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *term = va_arg(args, clingo_ast_t const *);
                auto const **elems = va_arg(args, clingo_ast_t const **);
                auto elems_size = va_arg(args, size_t);
                auto const *rhs = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::HdLitTheoryAtom>(
                    type, convert(loc), convert<CppClingo::Input::Term>(term),
                    convert<CppClingo::Input::TheoryElement>(elems, elems_size),
                    convert<std::optional<CppClingo::Input::TheoryRGuard>>(rhs));
                break;
            }
            case clingo_ast_type_head_conditional_literal: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *lit = va_arg(args, clingo_ast_t const *);
                auto const **cond = va_arg(args, clingo_ast_t const **);
                auto cond_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::CondLit>(
                    type, CppClingo::Input::CondLit{convert(loc), convert<CppClingo::Input::Lit>(lit),
                                                    convert<CppClingo::Input::Lit>(cond, cond_size)});
                break;
            }
            case clingo_ast_type_head_disjunction: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **elems = va_arg(args, clingo_ast_t const **);
                auto elems_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::HdLitDisjunction>(
                    type, convert(loc), convert<CppClingo::Input::HdLitDisjunctionElement>(elems, elems_size));
                break;
            }
            case clingo_ast_type_statement_rule: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *head = va_arg(args, clingo_ast_t const *);
                auto const **body = va_arg(args, clingo_ast_t const **);
                auto body_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmRule>(type, convert(loc), convert<HdLit>(head),
                                                                convert<BdLit>(body, body_size));
                break;
            }
            case clingo_ast_type_theory_operator_definition: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto name_size = va_arg(args, size_t);
                auto priority = va_arg(args, int);
                auto op_type = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::TheoryOpDefinition>(
                    type, convert(loc), *lib->store->string({name, name_size}), priority,
                    static_cast<TheoryOpType>(op_type));
                break;
            }
            case clingo_ast_type_theory_term_definition: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto name_size = va_arg(args, size_t);
                auto const **ops = va_arg(args, clingo_ast_t const **);
                auto ops_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::TheoryTermDefinition>(
                    type, convert(loc), *lib->store->string({name, name_size}),
                    convert<TheoryOpDefinition>(ops, ops_size));
                break;
            }
            case clingo_ast_type_theory_guard_definition: {
                std::va_list args;
                va_start(args, ast);
                auto const *ops = va_arg(args, clingo_string_t const *);
                auto ops_size = va_arg(args, size_t);
                auto const *term = va_arg(args, char const *);
                auto term_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::TheoryRGuardDefinition>(type, convert(lib, ops, ops_size),
                                                                               *lib->store->string({term, term_size}));
                break;
            }
            case clingo_ast_type_theory_atom_definition: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto name_size = va_arg(args, size_t);
                auto arity = va_arg(args, int);
                auto const *term = va_arg(args, char const *);
                auto term_size = va_arg(args, size_t);
                auto const *guard = va_arg(args, clingo_ast_t const *);
                auto atom_type = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::TheoryAtomDefinition>(
                    type, convert(loc), *lib->store->string({name, name_size}), arity,
                    *lib->store->string({term, term_size}), convert<std::optional<TheoryRGuardDefinition>>(guard),
                    static_cast<TheoryAtomType>(atom_type));
                break;
            }
            case clingo_ast_type_statement_theory: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto name_size = va_arg(args, size_t);
                auto const **terms = va_arg(args, clingo_ast_t const **);
                auto terms_size = va_arg(args, size_t);
                auto const **atoms = va_arg(args, clingo_ast_t const **);
                auto atoms_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmTheory>(
                    type, convert(loc), *lib->store->string({name, name_size}),
                    convert<TheoryTermDefinition>(terms, terms_size), convert<TheoryAtomDefinition>(atoms, atoms_size));
                break;
            }
            case clingo_ast_type_optimize_tuple: {
                std::va_list args;
                va_start(args, ast);
                auto const *weight = va_arg(args, clingo_ast_t const *);
                auto const *prio = va_arg(args, clingo_ast_t const *);
                auto const **terms = va_arg(args, clingo_ast_t const **);
                auto terms_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::OptimizeTuple>(
                    type, convert<Term>(weight), convert<std::optional<Term>>(prio), convert<Term>(terms, terms_size));
                break;
            }
            case clingo_ast_type_optimize_element: {
                std::va_list args;
                va_start(args, ast);
                auto const *tuple = va_arg(args, clingo_ast_t const *);
                auto const **cond = va_arg(args, clingo_ast_t const **);
                auto cond_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::OptimizeElement>(type, convert<OptimizeTuple>(tuple),
                                                                        convert<Lit>(cond, cond_size));
                break;
            }
            case clingo_ast_type_statement_optimize: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **elems = va_arg(args, clingo_ast_t const **);
                auto elems_size = va_arg(args, size_t);
                auto optimize_type = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmOptimize>(type, convert(loc),
                                                                    static_cast<OptimizeType>(optimize_type),
                                                                    convert<OptimizeElement>(elems, elems_size));
                break;
            }
            case clingo_ast_type_statement_weak_constraint: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **body = va_arg(args, clingo_ast_t const **);
                auto body_size = va_arg(args, size_t);
                auto const *tuple = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmWeakConstraint>(
                    type, convert(loc), convert<BdLit>(body, body_size), convert<OptimizeTuple>(tuple));
                break;
            }
            case clingo_ast_type_program_part: {
                std::va_list args;
                va_start(args, ast);
                auto const *name = va_arg(args, char const *);
                auto name_size = va_arg(args, size_t);
                const auto *syms = va_arg(args, clingo_symbol_t const *);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::ProgramParam>(type, *lib->store->string({name, name_size}),
                                                                     convert(syms, size));
                break;
            }
            case clingo_ast_type_edge: {
                std::va_list args;
                va_start(args, ast);
                auto const *u = va_arg(args, clingo_ast_t const *);
                auto const *v = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::Edge>(type, convert<Term>(u), convert<Term>(v));
                break;
            }
            case clingo_ast_type_statement_show: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *term = va_arg(args, clingo_ast_t const *);
                auto const **body = va_arg(args, clingo_ast_t const **);
                auto body_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmShow>(type, convert(loc), convert<Term>(term),
                                                                convert<BdLit>(body, body_size));
                break;
            }
            case clingo_ast_type_statement_show_nothing: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmShowNothing>(type, convert(loc));
                break;
            }
            case clingo_ast_type_statement_show_signature: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto name_size = va_arg(args, size_t);
                auto arity = va_arg(args, int);
                auto sign = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmShowSig>(type, convert(loc), sign != 0,
                                                                   *lib->store->string({name, name_size}), arity);
                break;
            }
            case clingo_ast_type_statement_project: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *atom = va_arg(args, clingo_ast_t const *);
                auto const **body = va_arg(args, clingo_ast_t const **);
                auto body_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmProject>(type, convert(loc), convert<Term>(atom),
                                                                   convert<BdLit>(body, body_size));
                break;
            }
            case clingo_ast_type_statement_project_signature: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto name_size = va_arg(args, size_t);
                auto arity = va_arg(args, int);
                auto sign = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmProjectSig>(type, convert(loc), sign != 0,
                                                                      *lib->store->string({name, name_size}), arity);
                break;
            }
            case clingo_ast_type_statement_defined: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto name_size = va_arg(args, size_t);
                auto arity = va_arg(args, int);
                auto sign = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmDefined>(type, convert(loc), sign != 0,
                                                                   *lib->store->string({name, name_size}), arity);
                break;
            }
            case clingo_ast_type_statement_external: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *atom = va_arg(args, clingo_ast_t const *);
                auto const **body = va_arg(args, clingo_ast_t const **);
                auto body_size = va_arg(args, size_t);
                auto const *external_type = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmExternal>(type, convert(loc), convert<Term>(atom),
                                                                    convert<BdLit>(body, body_size),
                                                                    convert<std::optional<Term>>(external_type));
                break;
            }
            case clingo_ast_type_statement_edge: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **edges = va_arg(args, clingo_ast_t const **);
                auto edges_size = va_arg(args, size_t);
                auto const **body = va_arg(args, clingo_ast_t const **);
                auto body_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmEdge>(type, convert(loc), convert<Edge>(edges, edges_size),
                                                                convert<BdLit>(body, body_size));
                break;
            }
            case clingo_ast_type_statement_heuristic: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *atom = va_arg(args, clingo_ast_t const *);
                auto const **body = va_arg(args, clingo_ast_t const **);
                auto body_size = va_arg(args, size_t);
                auto const *weight = va_arg(args, clingo_ast_t const *);
                auto const *modifier = va_arg(args, clingo_ast_t const *);
                auto const *priority = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmHeuristic>(
                    type, convert(loc), convert<Term>(atom), convert<BdLit>(body, body_size), convert<Term>(weight),
                    convert<std::optional<Term>>(priority), convert<Term>(modifier));
                break;
            }
            case clingo_ast_type_statement_include: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *value = va_arg(args, char const *);
                auto value_size = va_arg(args, size_t);
                auto include_type = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmInclude>(type, convert(loc),
                                                                   static_cast<IncludeType>(include_type),
                                                                   *lib->store->string({value, value_size}));
                break;
            }
            case clingo_ast_type_statement_program: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto name_size = va_arg(args, size_t);
                auto const *arguments = va_arg(args, clingo_string_t const *);
                auto arguments_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmProgram>(type, convert(loc),
                                                                   *lib->store->string({name, name_size}),
                                                                   convert(lib, arguments, arguments_size));
                break;
            }
            case clingo_ast_type_statement_script: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *value = va_arg(args, char const *);
                auto value_size = va_arg(args, size_t);
                auto const *script_type = va_arg(args, char const *);
                auto script_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmScript>(type, convert(loc),
                                                                  *lib->store->string({script_type, script_size}),
                                                                  *lib->store->string({value, value_size}));
                break;
            }
            case clingo_ast_type_statement_const: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto name_size = va_arg(args, size_t);
                auto const *term = va_arg(args, clingo_ast_t const *);
                auto prec = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmConst>(type, convert(loc), static_cast<Precedence>(prec),
                                                                 *lib->store->string({name, name_size}),
                                                                 convert<Term>(term));
                break;
            }
            case clingo_ast_type_statement_parts: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **elems = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                // auto elems = CppClingo::Input::ProgramParamVec{};
                auto prec = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmParts>(type, convert(loc), static_cast<Precedence>(prec),
                                                                 convert<CppClingo::Input::ProgramParam>(elems, size));
                break;
            }
            case clingo_ast_type_statement_comment: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *value = va_arg(args, char const *);
                auto value_size = va_arg(args, size_t);
                auto comment_type = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<CppClingo::Input::StmComment>(type, convert(loc),
                                                                   static_cast<CommentType>(comment_type),
                                                                   *lib->store->string({value, value_size}));
                break;
            }
        }
    }
    CLINGO_CATCH;
}
// NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

extern "C" auto clingo_ast_to_string(clingo_ast_t *ast, clingo_string_builder_t *builder) -> bool {
    CLINGO_TRY {
        if (ast == nullptr || builder == nullptr) {
            return fail_arguments();
        }
        *cpp_cast(builder) << *ast;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_compare(clingo_ast_t *a, clingo_ast_t *b) -> int {
    return c_cast(a->compare(*b));
}

extern "C" auto clingo_ast_equal(clingo_ast_t *a, clingo_ast_t *b) -> bool {
    return a->equal(*b);
}

extern "C" auto clingo_ast_hash(clingo_ast_t *ast) -> size_t {
    return ast->hash();
}

extern "C" auto clingo_ast_copy(clingo_ast_t *ast, clingo_ast_t **copy) -> bool {
    CLINGO_TRY {
        if (copy == nullptr) {
            return fail_arguments();
        }
        *copy = ast != nullptr ? ast->copy().release() : nullptr;
    }
    CLINGO_CATCH;
}

extern "C" void clingo_ast_free(clingo_ast_t *ast) {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    delete ast;
}

extern "C" void clingo_ast_array_free(clingo_ast_t **ast, size_t size) {
    if (ast != nullptr) {
        ASTVec::acquire(ast, size);
    }
}

extern "C" auto clingo_ast_get_type(clingo_ast_t *ast, clingo_ast_type_t *type) -> bool {
    CLINGO_TRY {
        if (ast == nullptr || type == nullptr) {
            return fail_arguments();
        }
        *type = ast->get_type();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_attribute_get_number(clingo_ast_t *ast, clingo_ast_attribute_t attribute, int *value)
    -> bool {
    CLINGO_TRY {
        if (ast == nullptr || value == nullptr) {
            return fail_arguments();
        }
        if (auto num = ast->get_number(attribute); num) {
            *value = *num;
            return true;
        }
        return fail_with(clingo_result_invalid, "invalid attribute");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_attribute_get_symbol(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                clingo_symbol_t *value) -> bool {
    CLINGO_TRY {
        if (ast == nullptr || value == nullptr) {
            return fail_arguments();
        }
        if (auto sym = ast->get_symbol(attribute); sym) {
            *value = *sym;
            return true;
        }
        return fail_with(clingo_result_invalid, "invalid attribute");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_attribute_get_location(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                  clingo_location_t const **value) -> bool {
    CLINGO_TRY {
        if (ast == nullptr || value == nullptr) {
            return fail_arguments();
        }
        if (auto loc = ast->get_location(attribute); loc) {
            *value = *loc;
            return true;
        }
        return fail_with(clingo_result_invalid, "invalid attribute");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_attribute_get_string(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                clingo_string_t *value) -> bool {
    CLINGO_TRY {
        if (ast == nullptr || value == nullptr) {
            return fail_arguments();
        }
        if (auto str = ast->get_string(attribute); str) {
            value->data = str->data();
            value->size = str->size();
            return true;
        }
        return fail_with(clingo_result_invalid, "invalid attribute");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_attribute_get_string_array(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                      clingo_string_t const **value, size_t *size) -> bool {
    CLINGO_TRY {
        if (ast == nullptr || value == nullptr || size == nullptr) {
            return fail_arguments();
        }
        if (auto vec = ast->get_string_vec(attribute); vec) {
            thread_local auto res = std::vector<clingo_string_t>{};
            res.clear();
            std::transform(vec->begin(), vec->end(), std::back_inserter(res),
                           [](auto const &str) { return clingo_string_t{str.data(), str.size()}; });
            *value = res.data();
            *size = res.size();
            return true;
        }
        return fail_with(clingo_result_invalid, "invalid attribute");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_attribute_get_symbol_array(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                      clingo_symbol_t const **value, size_t *size) -> bool {
    CLINGO_TRY {
        if (ast == nullptr || value == nullptr || size == nullptr) {
            return fail_arguments();
        }
        if (auto vec = ast->get_symbol_vec(attribute); vec) {
            thread_local auto res = std::vector<clingo_symbol_t>{};
            res.clear();
            std::transform(vec->begin(), vec->end(), std::back_inserter(res),
                           [](auto const &sym) { return CppClingo::Symbol::to_rep(sym); });
            *value = res.data();
            *size = res.size();
            return true;
        }
        return fail_with(clingo_result_invalid, "invalid attribute");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_attribute_get_ast(clingo_ast_t *ast, clingo_ast_attribute_t attribute, clingo_ast_t **value)
    -> bool {
    CLINGO_TRY {
        if (ast == nullptr || value == nullptr) {
            return fail_arguments();
        }
        if (auto val = ast->get_ast(attribute); val) {
            *value = val->release();
            return true;
        }
        return fail_with(clingo_result_invalid, "invalid attribute");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_attribute_get_ast_array(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                   clingo_ast_t ***value, size_t *size) -> bool {
    CLINGO_TRY {
        if (ast == nullptr || value == nullptr || size == nullptr) {
            return fail_arguments();
        }
        if (auto val = ast->get_ast_vec(attribute); val) {
            std::tie(*value, *size) = val->release();
            return true;
        }
        return fail_with(clingo_result_invalid, "invalid attribute");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_parse_expression(clingo_lib_t *lib, clingo_ast_parse_type_t type, char const *string,
                                            size_t size, clingo_ast_t **ast) -> bool {
    CLINGO_TRY {
        if (ast == nullptr || (string == nullptr && size > 0) || ast == nullptr) {
            return fail_arguments();
        }
        auto p = CppClingo::Input::Parser{lib->log, *lib->store};
        p.init(std::string_view{string, size}, *lib->store->string("<string>"));
        switch (type) {
            case clingo_ast_parse_type_term: {
                auto term = p.parse_term();
                if (!term) {
                    return fail_with(clingo_result_runtime, "parsing failed");
                }
                *ast = make_ast(*std::move(term)).release();
                break;
            }
            case clingo_ast_parse_type_theory_term: {
                auto term = p.parse_theory_term();
                if (!term) {
                    return fail_with(clingo_result_runtime, "parsing failed");
                }
                *ast = make_ast(*std::move(term)).release();
                break;
            }
            case clingo_ast_parse_type_literal: {
                auto lit = p.parse_literal();
                if (!lit) {
                    return fail_with(clingo_result_runtime, "parsing failed");
                }
                *ast = make_ast(*std::move(lit)).release();
                break;
            }
            case clingo_ast_parse_type_head_literal: {
                auto lit = p.parse_head_literal();
                if (!lit) {
                    return fail_with(clingo_result_runtime, "parsing failed");
                }
                *ast = make_ast(*std::move(lit)).release();
                break;
            }
            case clingo_ast_parse_type_body_literal: {
                auto lit = p.parse_body_literal();
                if (!lit) {
                    return fail_with(clingo_result_runtime, "parsing failed");
                }
                *ast = make_ast(*std::move(lit)).release();
                break;
            }
            case clingo_ast_parse_type_statement: {
                auto lit = p.parse_statement();
                if (!lit) {
                    return fail_with(clingo_result_runtime, "parsing failed");
                }
                *ast = make_ast(*std::move(lit)).release();
                break;
            }
            default: {
                return fail_arguments();
            }
        }
    }
    CLINGO_CATCH;
}

struct clingo_ast_scanner {
  public:
    clingo_ast_scanner(clingo_lib_t *lib) : lib_{lib}, parser_{lib->log, *lib->store} {}
    [[nodiscard]] auto next() -> std::unique_ptr<clingo_ast_t> {
        while (true) {
            if (active_) {
                auto [stm, res] = parser_.scan();
                parse_error_ = parse_error_ || !res;
                if (stm) {
                    return make_ast(*std::move(stm));
                }
                cur_.close();
                active_ = false;
            }
            if (strings_.empty()) {
                break;
            }
            active_ = true;
            if (strings_.front().second) {
                if (strings_.front().first == "-") {
                    parser_.init(std::cin, *lib_->store->string("<string>"));
                } else {
                    cur_.open(strings_.front().first);
                    parser_.init(cur_, *lib_->store->string("<string>"));
                }
            } else {
                parser_.init(strings_.front().first, *lib_->store->string("<string>"));
            }
            strings_.pop_front();
        }
        if (parse_error_) {
            throw CppClingo::parse_error();
        }
        return nullptr;
    }
    [[nodiscard]] auto lib() const -> clingo_lib_t * { return lib_; }
    auto scan_string(std::string_view str) { strings_.emplace_front(str, false); }
    auto scan_file(std::string_view path) { strings_.emplace_front(path, true); }
    [[nodiscard]] auto has_error() const -> bool { return parse_error_; }

  private:
    clingo_lib_t *lib_;
    CppClingo::Input::Parser parser_;
    std::forward_list<std::pair<std::string, bool>> strings_;
    std::ifstream cur_;
    bool active_ = false;
    bool parse_error_ = false;
};

extern "C" auto clingo_ast_scan_string(clingo_lib_t *lib, char const *program, size_t size,
                                       clingo_ast_scanner_t **scanner) -> bool {
    CLINGO_TRY {
        if (lib == nullptr || (program == nullptr && size > 0) || scanner == nullptr) {
            return fail_arguments();
        }
        auto res = std::make_unique<clingo_ast_scanner>(lib);
        res->scan_string(std::string{program, size});
        *scanner = res.release();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_scan_files(clingo_lib_t *lib, clingo_string_t const *files, size_t size,
                                      clingo_ast_scanner_t **scanner) -> bool {
    CLINGO_TRY {
        if (lib == nullptr || (files == nullptr && size != 0) || scanner == nullptr) {
            return fail_arguments();
        }
        auto res = std::make_unique<clingo_ast_scanner>(lib);
        auto span = std::span(files, size);
        if (span.empty()) {
            res->scan_file("-");
        } else {
            std::ranges::for_each(std::ranges::reverse_view(span),
                                  [&res](clingo_string_t path) { res->scan_file({path.data, path.size}); });
        }
        *scanner = res.release();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_scanner_next(clingo_ast_scanner_t *scanner, clingo_ast_t **ast) -> bool {
    CLINGO_TRY {
        if (scanner == nullptr || scanner == nullptr) {
            return fail_arguments();
        }
        *ast = scanner->next().release();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_scanner_has_error(clingo_ast_scanner_t *scanner) -> bool {
    return scanner != nullptr && scanner->has_error();
}

extern "C" void clingo_ast_scanner_close(clingo_ast_scanner_t *scanner) {
    if (scanner != nullptr) {
        scanner->lib()->log.reset();
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        delete scanner;
    }
}

struct clingo_ast_rewrite_context {
    clingo_ast_rewrite_context(clingo_lib *lib) : lib{lib} {}
    clingo_lib *lib;
    CppClingo::Input::TheoryAtomParser parser;
    CppClingo::Input::ParamMap param_map;
    CppClingo::Input::ConstMap const_map;
    CppClingo::Input::RewriteOptions options;
    CppClingo::Input::RewriteContext ctx = {lib->log, *lib->store, options, parser, param_map, const_map};
    CppClingo::Input::ParamUnmap param_unmap;
};

extern "C" auto clingo_ast_rewrite_context_create(clingo_lib_t *lib, clingo_ast_rewrite_context_t **context) -> bool {
    CLINGO_TRY {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        *context = new clingo_ast_rewrite_context{lib};
    }
    CLINGO_CATCH;
}

extern "C" void clingo_ast_rewrite_context_free(clingo_ast_rewrite_context_t *context) {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    delete context;
}

extern "C" auto clingo_ast_rewrite_context_add_param(clingo_ast_rewrite_context_t *context, char const *param,
                                                     size_t size) -> bool {
    auto *lib = context->lib;
    CLINGO_TRY {
        if (auto prm = lib->store->string(std::string_view{param, size}); context->param_map.emplace(prm).second) {
            auto var = lib->store->string("$" + std::to_string(context->param_unmap.size()));
            context->param_unmap.emplace(var, prm);
        }
    }
    CLINGO_CATCH;
}

extern "C" void clingo_ast_rewrite_context_clear_params(clingo_ast_rewrite_context_t *context) {
    context->param_map.clear();
    context->param_unmap.clear();
}

extern "C" auto clingo_ast_rewrite_context_add_theory(clingo_ast_rewrite_context_t *context, clingo_ast_t const *theory)
    -> bool {
    auto *lib = context->lib;
    CLINGO_TRY {
        auto stm = convert<CppClingo::Input::StmTheory>(theory);
        context->parser.add_theory(lib->log, stm);
        if (context->parser.has_error()) {
            throw std::runtime_error("adding theory failed");
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_ast_rewrite_context_get_project_anonymous(clingo_ast_rewrite_context_t *context) -> bool {
    return context->options.project_anonymous;
}

extern "C" void clingo_ast_rewrite_context_set_project_anonymous(clingo_ast_rewrite_context_t *context, bool value) {
    context->options.project_anonymous = value;
}

extern "C" auto clingo_ast_rewrite_context_get_project_mode(clingo_ast_rewrite_context_t *context)
    -> clingo_projection_mode_t {
    return static_cast<clingo_projection_mode_t>(context->options.project_mode);
}

extern "C" void clingo_ast_rewrite_context_set_project_mode(clingo_ast_rewrite_context_t *context,
                                                            clingo_projection_mode_t value) {
    context->options.project_mode = static_cast<CppClingo::Input::ProjectionMode>(value);
}

extern "C" auto clingo_ast_rewrite_context_get_lib(clingo_ast_rewrite_context_t *context) -> clingo_lib_t * {
    return context->lib;
}

extern "C" auto clingo_ast_rewrite(clingo_ast_rewrite_context_t *context, clingo_ast_t *statement,
                                   clingo_ast_t ***result, size_t *result_size) -> bool {
    using namespace CppClingo::Input;
    auto *lib = context->lib;
    CLINGO_TRY {
        CppClingo::GCLock lock{*lib->store};
        *result = nullptr;
        *result_size = 0;
        auto stms = StmVec{};
        auto stm = convert<Stm>(statement);
        rewrite(context->ctx, stm, stms);
        ASTVec res{stms.size()};
        int i = 0;
        for (auto &stm : stms) {
            if (auto res_stm = unmap_params(*lib->store, context->param_unmap, stm); res_stm) {
                stm = *std::move(res_stm);
            }
            res[i] = make_ast(std::move(stm)).release();
            ++i;
        }
        std::tie(*result, *result_size) = res.release();
    }
    CLINGO_CATCH;
}

clingo_program::clingo_program(clingo_lib_t *lib) : lib{lib} {
    lib->store->gc_add_owner(*this);
}

clingo_program::~clingo_program() noexcept {
    lib->store->gc_del_owner(*this);
}

void clingo_program::mark(CppClingo::SymbolCollector &gc) const {
    program.mark(gc);
}

extern "C" auto clingo_program_new(clingo_lib_t *lib, clingo_program_t **program) -> bool {
    CLINGO_TRY {
        *program = std::make_unique<clingo_program>(lib).release();
    }
    CLINGO_CATCH;
}

extern "C" void clingo_program_free(clingo_program_t *program) {
    // NOLINTNEXTLINE
    delete program;
}

extern "C" auto clingo_program_add(clingo_program_t *program, clingo_ast_t *statement) -> bool {
    CLINGO_TRY {
        program->program.add(*program->lib->store, convert<CppClingo::Input::Stm>(statement));
    }
    CLINGO_CATCH;
}

// NOLINTEND(cppcoreguidelines-macro-usage)
