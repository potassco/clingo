#include "lib.hh"
#include "streams.hh"

#include <gringo/input/algo/parse.hh>
#include <gringo/input/algo/print.hh>
#include <gringo/input/algo/rewrite.hh>
#include <gringo/input/algo/rewrite_theory.hh>
#include <gringo/input/algo/substitute.hh>

#include "gringo/util/type_traits.hh"
#include <gringo/util/algorithm.hh>
#include <gringo/util/ordered_map.hh>
#include <gringo/util/ordered_set.hh>

#include <any>
#include <cstdarg>
#include <cstring>
#include <forward_list>
#include <span>

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

namespace {

class ASTVec;

using Owner = Gringo::Util::immutable_value<std::any>;

template <class T>
auto make_ast(Owner const &owner, Gringo::Util::immutable_value<T> const &ptr) -> std::unique_ptr<clingo_ast_t>;
template <class T> auto make_ast(Owner const &owner, std::optional<T> const &opt) -> std::unique_ptr<clingo_ast_t>;
template <class... T> auto make_ast(Owner const &owner, std::variant<T...> const &var) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::LGuard::value_type const &guard) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::RGuard::value_type const &guard) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TheoryRGuard::value_type const &guard)
    -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::Projection const &projection) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::Term const &term) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TheoryTerm const &term) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::ArgumentTuple const &tuple) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TupleElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::Lit const &lit) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::UnparsedElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TheoryElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::SetAggregateElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::BdLitAggregateElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::HdLitAggregateElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::HdLitDisjunctionElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::BdLit const &lit) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::HdLit const &lit) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TheoryOpDefinition const &def) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TheoryRGuardDefinition const &def) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TheoryTermDefinition const &def) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TheoryAtomDefinition const &def) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::OptimizeTuple const &tuple) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::OptimizeElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::Edge const &edge) -> std::unique_ptr<clingo_ast_t>;

template <class T> auto make_ast_vec(Owner const &owner, std::span<T> vec) -> ASTVec;
template <class T> auto make_ast_vec(Owner const &owner, std::vector<T> const &vec) -> ASTVec;
template <class T> auto make_ast_vec(Owner const &owner, Gringo::Util::immutable_array<T> const &vec) -> ASTVec;

template <class T> auto convert_ast_vec(clingo_ast const **ast, size_t size) -> std::vector<T>;

auto convert_loc(clingo_lib_t *lib, clingo_location_t const *loc) -> Gringo::Input::Location {
    return {{lib->store->string(loc->begin_file), loc->begin_line, loc->begin_column},
            {lib->store->string(loc->end_file), loc->end_line, loc->end_column}};
}

auto convert_string_array(clingo_lib_t *lib, char const **array, size_t size) -> Gringo::StringVec {
    auto ret = Gringo::StringVec{};
    ret.reserve(size);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::transform(array, array + size, std::back_inserter(ret), [lib](auto str) { return lib->store->string(str); });
    return ret;
}

[[maybe_unused]] auto make_loc(Gringo::Input::Location const &loc) -> clingo_location_t {
    return {loc.begin.file.c_str(), loc.end.file.c_str(), loc.begin.line,
            loc.end.line,           loc.begin.column,     loc.end.column};
}

} // namespace

struct clingo_ast {
  public:
    clingo_ast(Owner owner, clingo_ast_type_e type, void const *ptr)
        : owner_{std::move(owner)}, type_{type}, ptr_{ptr} {}
    [[nodiscard]] auto copy() const -> std::unique_ptr<clingo_ast_t>;
    void print(std::ostream &out) const;
    [[nodiscard]] auto hash() const -> size_t;
    [[nodiscard]] auto equal_to(clingo_ast_t const &other) const -> bool;
    [[nodiscard]] auto less_than(clingo_ast_t const &other) const -> bool;
    [[nodiscard]] auto get_type() const -> clingo_ast_type_e;
    [[nodiscard]] auto get_number(clingo_ast_attribute_t attr) const -> std::optional<int>;
    [[nodiscard]] auto get_symbol(clingo_ast_attribute_t attr) const -> std::optional<clingo_symbol_t>;
    [[nodiscard]] auto get_location(clingo_ast_attribute_t attr) const -> std::optional<clingo_location_t>;
    [[nodiscard]] auto get_string(clingo_ast_attribute_t attr) const -> std::optional<char const *>;
    [[nodiscard]] auto get_string_vec(clingo_ast_attribute_t attr) const
        -> std::optional<std::span<Gringo::String const>>;
    [[nodiscard]] auto get_ast(clingo_ast_attribute_t attr) const -> std::optional<std::unique_ptr<clingo_ast_t>>;
    [[nodiscard]] auto get_ast_vec(clingo_ast_attribute_t attr) const -> std::optional<ASTVec>;

    template <class T> [[nodiscard]] auto convert() const -> T = delete;
    template <class V> auto visit(V &&visit) const -> std::invoke_result_t<V, Gringo::Input::Projection const &>;

    friend auto operator<<(std::ostream &out, clingo_ast_t const &ast) -> std::ostream & {
        ast.print(out);
        return out;
    }

  private:
    template <typename T> [[nodiscard]] auto cast() const -> T const & { return *static_cast<T const *>(ptr_); }
    Owner owner_;
    clingo_ast_type_e type_;
    void const *ptr_;
};

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

template <class T> auto make_ast_vec(Owner const &owner, std::span<T> vec) -> ASTVec {
    ASTVec res{vec.size()};
    size_t i = 0;
    for (auto const &elem : vec) {
        res[i] = make_ast(owner, elem).release();
        ++i;
    }
    return res;
}

template <class T> auto make_ast_vec(Owner const &owner, std::vector<T> const &vec) -> ASTVec {
    return make_ast_vec(owner, std::span{vec});
}

template <class T> auto make_ast_vec(Owner const &owner, Gringo::Util::immutable_array<T> const &vec) -> ASTVec {
    return make_ast_vec(owner, std::span{vec});
}

template <class T> auto convert_ast_opt(clingo_ast const *ast) -> std::optional<T> {
    std::optional<T> res;
    if (ast != nullptr) {
        res = ast->convert<T>();
    }
    return res;
}

template <class T> auto convert_ast_vec(clingo_ast const **ast, size_t size) -> std::vector<T> {
    std::vector<T> res;
    res.reserve(size);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    for (auto it = ast, ie = ast + size; it != ie; ++it) {
        res.emplace_back((*it)->convert<T>());
    }
    return res;
}

template <class... T>
auto make_ast(Owner const &owner, std::variant<T...> const &var) -> std::unique_ptr<clingo_ast_t> {
    return std::visit([&owner](auto const &x) { return make_ast(owner, x); }, var);
}

template <class T>
auto make_ast(Owner const &owner, Gringo::Util::immutable_value<T> const &ptr) -> std::unique_ptr<clingo_ast_t> {
    return make_ast(owner, *ptr);
}

template <class T> auto make_ast(Owner const &owner, std::optional<T> const &opt) -> std::unique_ptr<clingo_ast_t> {
    if (opt) {
        return make_ast(owner, *opt);
    }
    return nullptr;
}

auto make_ast(Owner const &owner, Gringo::Input::LGuard::value_type const &guard) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_left_guard, &guard);
}

auto make_ast(Owner const &owner, Gringo::Input::RGuard::value_type const &guard) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_right_guard, &guard);
}

auto make_ast(Owner const &owner, Gringo::Input::TheoryRGuard::value_type const &guard)
    -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_right_guard, &guard);
}

auto make_ast(Owner const &owner, Gringo::Input::Projection const &projection) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_projection, &projection);
}

auto make_ast(Owner const &owner, Gringo::Input::Term const &term) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, Gringo::Input::TermVariable>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_variable, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::TermSymbol>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_symbolic, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::TermTuple>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_tuple, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::TermFunction>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_function, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::TermAbs>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_absolute, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::TermUnary>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_unary_operation, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::TermBinary>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_binary_operation, &x);
            }
        },
        term);
}

auto make_ast(Owner const &owner, Gringo::Input::TheoryTerm const &term) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, Gringo::Input::TheoryTermVariable>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_term_variable, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::TheoryTermSymbol>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_term_symbolic, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::TheoryTermTuple>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_term_tuple, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::TheoryTermFunction>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_term_function, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::TheoryTermUnparsed>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_term_unparsed, &x);
            }
        },
        term);
}

auto make_ast(Owner const &owner, Gringo::Input::UnparsedElement const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_unparsed_element, &elem);
}

auto make_ast(Owner const &owner, Gringo::Input::ArgumentTuple const &tuple) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_argument_tuple, static_cast<void const *>(&tuple));
}

auto make_ast(Owner const &owner, Gringo::Input::TupleElement const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, Gringo::Input::Term>) {
                return make_ast(owner, x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::ArgumentTuple>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_argument_tuple, &x);
            }
        },
        elem);
}

auto make_ast(Owner const &owner, Gringo::Input::Lit const &lit) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, Gringo::Input::LitBool>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_literal_boolean, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::LitSymbolic>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_literal_symbolic, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::LitComparison>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_literal_comparison, &x);
            }
        },
        lit);
}

auto make_ast(Owner const &owner, Gringo::Input::HdLit const &lit) -> std::unique_ptr<clingo_ast_t> {
    using namespace Gringo::Input;
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, HdLitSimple>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_head_simple_literal, &x);
            }
            if constexpr (std::is_same_v<T, HdLitDisjunction>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_head_disjunction, &x);
            }
            if constexpr (std::is_same_v<T, HdLitSetAggregate>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_head_set_aggregate, &x);
            }
            if constexpr (std::is_same_v<T, HdLitTheoryAtom>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_head_theory_atom, &x);
            }
            if constexpr (std::is_same_v<T, HdLitAggregate>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_head_aggregate, &x);
            }
        },
        lit);
}

auto make_ast(Owner const &owner, Gringo::Input::BdLit const &lit) -> std::unique_ptr<clingo_ast_t> {
    using namespace Gringo::Input;
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, BdLitSimple>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_body_simple_literal, &x);
            }
            if constexpr (std::is_same_v<T, BdLitConjunction>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_body_conditional_literal, &x);
            }
            if constexpr (std::is_same_v<T, BdLitSetAggregate>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_body_set_aggregate, &x);
            }
            if constexpr (std::is_same_v<T, BdLitTheoryAtom>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_body_theory_atom, &x);
            }
            if constexpr (std::is_same_v<T, BdLitAggregate>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_body_aggregate, &x);
            }
        },
        lit);
}

auto make_ast(Owner const &owner, Gringo::Input::TheoryElement const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_atom_element, &elem);
}

auto make_ast(Owner const &owner, Gringo::Input::SetAggregateElement const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_set_aggregate_element, &elem);
}

auto make_ast(Owner const &owner, Gringo::Input::BdLitAggregateElement const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_body_aggregate_element, &elem);
}

auto make_ast(Owner const &owner, Gringo::Input::HdLitAggregateElement const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_head_aggregate_element, &elem);
}

auto make_ast(Owner const &owner, Gringo::Input::HdLitDisjunctionElement const &elem) -> std::unique_ptr<clingo_ast_t> {
    using namespace Gringo::Input;
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, CondLit>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_head_conditional_literal, &x);
            } else {
                return make_ast(owner, x);
            }
        },
        elem);
}

auto make_ast(Owner const &owner, Gringo::Input::TheoryOpDefinition const &def) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_operator_definition, &def);
}

auto make_ast(Owner const &owner, Gringo::Input::TheoryRGuardDefinition const &def) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_guard_definition, &def);
}

auto make_ast(Owner const &owner, Gringo::Input::TheoryTermDefinition const &def) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_term_definition, &def);
}

auto make_ast(Owner const &owner, Gringo::Input::TheoryAtomDefinition const &def) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_atom_definition, &def);
}

auto make_ast(Owner const &owner, Gringo::Input::OptimizeTuple const &tuple) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_optimize_tuple, &tuple);
}

auto make_ast(Owner const &owner, Gringo::Input::OptimizeElement const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_optimize_element, &elem);
}

auto make_ast(Owner const &owner, Gringo::Input::Edge const &edge) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_edge, &edge);
}

auto make_ast(Owner const &owner, Gringo::Input::Stm const &term) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, Gringo::Input::StmRule>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_rule, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::StmTheory>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_theory, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::StmOptimize>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_optimize, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::StmWeakConstraint>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_weak_constraint, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::StmShow>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_show, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::StmShowSig>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_show_signature, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::StmProject>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_project, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::StmProjectSig>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_project_signature, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::StmDefined>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_defined, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::StmExternal>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_external, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::StmEdge>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_edge, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::StmHeuristic>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_heuristic, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::StmScript>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_script, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::StmInclude>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_include, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::StmProgram>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_program, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::StmConst>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_const, &x);
            }
            if constexpr (std::is_same_v<T, Gringo::Input::StmComment>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_comment, &x);
            }
        },
        term);
}

template <class T, class... A> auto construct_ast(clingo_ast_type_t type, A &&...args) -> clingo_ast * {
    auto owner = Gringo::Util::make_immutable<std::any>(T{std::forward<A>(args)...});
    auto *ptr = std::any_cast<T>(&owner.get());
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    return new clingo_ast{std::move(owner), static_cast<clingo_ast_type_e>(type), ptr};
}

} // namespace

template <class V>
auto clingo_ast::visit(V &&visit) const -> std::invoke_result_t<V, Gringo::Input::Projection const &> {
    using namespace Gringo::Input;
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
            return std::invoke(std::forward<V>(visit), cast<TheoryRGuard::value_type>());
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
        case clingo_ast_type_edge: {
            return std::invoke(std::forward<V>(visit), cast<Edge>());
        }
        case clingo_ast_type_statement_show: {
            return std::invoke(std::forward<V>(visit), cast<StmShow>());
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
        case clingo_ast_type_statement_comment: {
            return std::invoke(std::forward<V>(visit), cast<StmComment>());
        }
    }
    throw std::invalid_argument("invalid ast type");
}

auto clingo_ast::get_type() const -> clingo_ast_type_e { return type_; }

auto clingo_ast::get_location(clingo_ast_attribute_t attr) const -> std::optional<clingo_location_t> {
    using namespace Gringo::Input;
    if (attr != clingo_ast_attribute_location) {
        return std::nullopt;
    }
    return visit([]<class T>(T const &x) -> std::optional<clingo_location_t> {
        if constexpr (requires(T const &x) { x.loc(); }) {
            return make_loc(x.loc());
        }
        return std::nullopt;
    });
}

#define SWITCH(...)                                                                                                    \
    using namespace Gringo::Input;                                                                                     \
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

auto clingo_ast::get_number(clingo_ast_attribute_t attr) const -> std::optional<int> {
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
            ATTR(const_type, type()))
        TYPE(statement_comment, StmComment,
            ATTR(comment_type, type())))
    // clang-format on
}

#undef ATTR
#define ATTR(attr, value)                                                                                              \
    case clingo_ast_attribute_##attr: {                                                                                \
        return static_cast<clingo_symbol_t>(Gringo::Symbol::to_rep(cast<Type>().value));                               \
    }

[[nodiscard]] auto clingo_ast::get_symbol(clingo_ast_attribute_t attr) const -> std::optional<clingo_symbol_t> {
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
        return cast<Type>().value.c_str();                                                                             \
    }

auto clingo_ast::get_string(clingo_ast_attribute_t attr) const -> std::optional<char const *> {
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
        TYPE(theory_right_guard, TheoryRGuard::value_type,
            ATTR(theory_operator, first))
        TYPE(theory_operator_definition, TheoryOpDefinition,
            ATTR(name, op()))
        TYPE(theory_term_definition, TheoryTermDefinition,
            ATTR(name, name()))
        TYPE(theory_guard_definition, TheoryRGuardDefinition,
            ATTR(term, second))
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
        TYPE(statement_comment, StmComment,
            ATTR(value, value())))
    // clang-format on
}

#undef ATTR
#define ATTR(attr, value)                                                                                              \
    case clingo_ast_attribute_##attr: {                                                                                \
        return std::span{cast<Type>().value};                                                                          \
    }

auto clingo_ast::get_string_vec(clingo_ast_attribute_t attr) const -> std::optional<std::span<Gringo::String const>> {
    // clang-format off
    SWITCH(
        TYPE(unparsed_element, UnparsedElement,
            ATTR(operators, first))
        TYPE(theory_guard_definition, TheoryRGuardDefinition,
            ATTR(operators, first))
        TYPE(statement_program, StmProgram,
            ATTR(arguments, args())))
    // clang-format on
}

#undef ATTR
#define ATTR(attr, value)                                                                                              \
    case clingo_ast_attribute_##attr: {                                                                                \
        return make_ast(owner_, cast<Type>().value);                                                                   \
    }

auto clingo_ast::get_ast(clingo_ast_attribute_t attr) const -> std::optional<std::unique_ptr<clingo_ast_t>> {
    // clang-format off
    SWITCH(
        TYPE(term_unary_operation, TermUnary,
            ATTR(right, rhs()))
        TYPE(term_binary_operation, TermBinary,
            ATTR(left, lhs())
            ATTR(right, rhs()))
        TYPE(unparsed_element, UnparsedElement,
            ATTR(term, second))
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
        TYPE(theory_right_guard, TheoryRGuard::value_type,
            ATTR(term, second))
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
        return make_ast_vec(owner_, cast<Type>().value);                                                               \
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
            ATTR(body, body())))
    // clang-format on
}

#undef ATTR
#undef TYPE
#undef SWITCH

auto clingo_ast::copy() const -> std::unique_ptr<clingo_ast_t> { return std::make_unique<clingo_ast>(*this); }

void clingo_ast::print(std::ostream &out) const {
    using namespace Gringo::Input;
    visit([&out]<class T>(T const &x) {
        if constexpr (Gringo::Util::matches<T, TheoryRGuard::value_type, RGuard::value_type>) {
            out << " " << x.first << " " << x.second;
        } else if constexpr (std::is_same_v<T, UnparsedElement>) {
            for (auto const &op : x.first) {
                out << op << " ";
            }
            out << x.second;
        } else if constexpr (std::is_same_v<T, ArgumentTuple>) {
            bool comma = false;
            for (auto const &elem : x.elems()) {
                if (comma) {
                    out << ",";
                } else {
                    comma = true;
                }
                std::visit([&out](auto &x) { out << x; }, elem);
            }
        } else if constexpr (std::is_same_v<T, LGuard::value_type>) {
            out << x.first << " " << x.second << " ";
        } else {
            out << x;
        }
    });
}

auto clingo_ast::hash() const -> size_t {
    using namespace Gringo::Input;
    return visit(
        [this](auto &x) { return Gringo::Util::hash_mix(Gringo::Util::value_hash_record<clingo_ast>(type_, x)); });
}

auto clingo_ast::equal_to(clingo_ast_t const &other) const -> bool {
    return type_ == other.type_ &&
           visit([&other](auto const &x) { return x == other.cast<std::decay_t<decltype(x)>>(); });
}

auto clingo_ast::less_than(clingo_ast_t const &other) const -> bool {
    if (type_ != other.type_) {
        return type_ < other.type_;
    }
    return visit([&other](auto const &x) { return x < other.cast<std::decay_t<decltype(x)>>(); });
}

template <> [[nodiscard]] auto clingo_ast::convert<Gringo::Input::Term>() const -> Gringo::Input::Term {
    switch (type_) {
        case clingo_ast_type_term_variable: {
            return cast<Gringo::Input::TermVariable>();
        }
        case clingo_ast_type_term_symbolic: {
            return cast<Gringo::Input::TermSymbol>();
        }
        case clingo_ast_type_term_tuple: {
            return cast<Gringo::Input::TermTuple>();
        }
        case clingo_ast_type_term_function: {
            return cast<Gringo::Input::TermFunction>();
        }
        case clingo_ast_type_term_absolute: {
            return cast<Gringo::Input::TermAbs>();
        }
        case clingo_ast_type_term_unary_operation: {
            return cast<Gringo::Input::TermUnary>();
        }
        case clingo_ast_type_term_binary_operation: {
            return cast<Gringo::Input::TermBinary>();
        }
        default: {
            throw std::runtime_error("term expected");
        }
    }
}

template <> [[nodiscard]] auto clingo_ast::convert<Gringo::Input::TheoryTerm>() const -> Gringo::Input::TheoryTerm {
    switch (type_) {
        case clingo_ast_type_theory_term_variable: {
            return cast<Gringo::Input::TheoryTermVariable>();
        }
        case clingo_ast_type_theory_term_symbolic: {
            return cast<Gringo::Input::TheoryTermSymbol>();
        }
        case clingo_ast_type_theory_term_tuple: {
            return cast<Gringo::Input::TheoryTermTuple>();
        }
        case clingo_ast_type_theory_term_function: {
            return cast<Gringo::Input::TheoryTermFunction>();
        }
        case clingo_ast_type_theory_term_unparsed: {
            return cast<Gringo::Input::TheoryTermUnparsed>();
        }
        default: {
            throw std::runtime_error("theory term expected");
        }
    }
}

template <> [[nodiscard]] auto clingo_ast::convert<Gringo::Input::Lit>() const -> Gringo::Input::Lit {
    switch (type_) {
        case clingo_ast_type_literal_boolean: {
            return cast<Gringo::Input::LitBool>();
        }
        case clingo_ast_type_literal_symbolic: {
            return cast<Gringo::Input::LitSymbolic>();
        }
        case clingo_ast_type_literal_comparison: {
            return cast<Gringo::Input::LitComparison>();
        }
        default: {
            throw std::runtime_error("literal expected");
        }
    }
}

template <> [[nodiscard]] auto clingo_ast::convert<Gringo::Input::Argument>() const -> Gringo::Input::Argument {
    if (type_ == clingo_ast_type_projection) {
        return cast<Gringo::Input::Projection>();
    }
    return convert<Gringo::Input::Term>();
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::ArgumentTuple>() const -> Gringo::Input::ArgumentTuple {
    if (type_ == clingo_ast_type_argument_tuple) {
        return cast<Gringo::Input::ArgumentTuple>();
    }
    throw std::runtime_error("argument tuple expected");
}

template <> [[nodiscard]] auto clingo_ast::convert<Gringo::Input::TupleElement>() const -> Gringo::Input::TupleElement {
    if (type_ == clingo_ast_type_argument_tuple) {
        return cast<Gringo::Input::ArgumentTuple>();
    }
    return convert<Gringo::Input::Term>();
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::UnparsedElement>() const -> Gringo::Input::UnparsedElement {
    if (type_ == clingo_ast_type_unparsed_element) {
        return cast<Gringo::Input::UnparsedElement>();
    }
    throw std::runtime_error("unparsed element expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::LGuard::value_type>() const -> Gringo::Input::LGuard::value_type {
    if (type_ == clingo_ast_type_left_guard) {
        return cast<Gringo::Input::LGuard::value_type>();
    }
    throw std::runtime_error("left guard expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::RGuard::value_type>() const -> Gringo::Input::RGuard::value_type {
    if (type_ == clingo_ast_type_right_guard) {
        return cast<Gringo::Input::RGuard::value_type>();
    }
    throw std::runtime_error("right guard expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::TheoryRGuard::value_type>() const
    -> Gringo::Input::TheoryRGuard::value_type {
    if (type_ == clingo_ast_type_theory_right_guard) {
        return cast<Gringo::Input::TheoryRGuard::value_type>();
    }
    throw std::runtime_error("theory right guard expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::SetAggregateElement>() const
    -> Gringo::Input::SetAggregateElement {
    if (type_ == clingo_ast_type_set_aggregate_element) {
        return cast<Gringo::Input::SetAggregateElement>();
    }
    throw std::runtime_error("set aggregate element expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::TheoryElement>() const -> Gringo::Input::TheoryElement {
    if (type_ == clingo_ast_type_theory_atom_element) {
        return cast<Gringo::Input::TheoryElement>();
    }
    throw std::runtime_error("theory atom element expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::BdLitAggregateElement>() const
    -> Gringo::Input::BdLitAggregateElement {
    if (type_ == clingo_ast_type_body_aggregate_element) {
        return cast<Gringo::Input::BdLitAggregateElement>();
    }
    throw std::runtime_error("body aggregate element expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::HdLitAggregateElement>() const
    -> Gringo::Input::HdLitAggregateElement {
    if (type_ == clingo_ast_type_head_aggregate_element) {
        return cast<Gringo::Input::HdLitAggregateElement>();
    }
    throw std::runtime_error("body aggregate element expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::HdLitDisjunctionElement>() const
    -> Gringo::Input::HdLitDisjunctionElement {
    if (type_ == clingo_ast_type_head_conditional_literal) {
        return cast<Gringo::Input::CondLit>();
    }
    return cast<Gringo::Input::Lit>();
}

template <> [[nodiscard]] auto clingo_ast::convert<Gringo::Input::HdLit>() const -> Gringo::Input::HdLit {
    switch (type_) {
        case clingo_ast_type_head_simple_literal: {
            return cast<Gringo::Input::HdLitSimple>();
        }
        case clingo_ast_type_head_disjunction: {
            return cast<Gringo::Input::HdLitDisjunction>();
        }
        case clingo_ast_type_head_theory_atom: {
            return cast<Gringo::Input::HdLitTheoryAtom>();
        }
        case clingo_ast_type_head_set_aggregate: {
            return cast<Gringo::Input::HdLitSetAggregate>();
        }
        case clingo_ast_type_head_aggregate: {
            return cast<Gringo::Input::HdLitAggregate>();
        }
        default: {
            throw std::invalid_argument("head literal expected");
        }
    }
}

template <> [[nodiscard]] auto clingo_ast::convert<Gringo::Input::BdLit>() const -> Gringo::Input::BdLit {
    switch (type_) {
        case clingo_ast_type_body_simple_literal: {
            return cast<Gringo::Input::BdLitSimple>();
        }
        case clingo_ast_type_body_conditional_literal: {
            return cast<Gringo::Input::BdLitConjunction>();
        }
        case clingo_ast_type_body_theory_atom: {
            return cast<Gringo::Input::BdLitTheoryAtom>();
        }
        case clingo_ast_type_body_set_aggregate: {
            return cast<Gringo::Input::BdLitSetAggregate>();
        }
        case clingo_ast_type_body_aggregate: {
            return cast<Gringo::Input::BdLitAggregate>();
        }
        default: {
            throw std::invalid_argument("body literal expected");
        }
    }
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::TheoryOpDefinition>() const -> Gringo::Input::TheoryOpDefinition {
    if (type_ == clingo_ast_type_theory_operator_definition) {
        return cast<Gringo::Input::TheoryOpDefinition>();
    }
    throw std::runtime_error("theory operator definition expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::TheoryRGuardDefinition>() const
    -> Gringo::Input::TheoryRGuardDefinition {
    if (type_ == clingo_ast_type_theory_guard_definition) {
        return cast<Gringo::Input::TheoryRGuardDefinition>();
    }
    throw std::runtime_error("theory guard definition expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::TheoryTermDefinition>() const
    -> Gringo::Input::TheoryTermDefinition {
    if (type_ == clingo_ast_type_theory_term_definition) {
        return cast<Gringo::Input::TheoryTermDefinition>();
    }
    throw std::runtime_error("theory term definition expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::TheoryAtomDefinition>() const
    -> Gringo::Input::TheoryAtomDefinition {
    if (type_ == clingo_ast_type_theory_atom_definition) {
        return cast<Gringo::Input::TheoryAtomDefinition>();
    }
    throw std::runtime_error("theory atom definition expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::OptimizeTuple>() const -> Gringo::Input::OptimizeTuple {
    if (type_ == clingo_ast_type_optimize_tuple) {
        return cast<Gringo::Input::OptimizeTuple>();
    }
    throw std::runtime_error("optimize tuple expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::OptimizeElement>() const -> Gringo::Input::OptimizeElement {
    if (type_ == clingo_ast_type_optimize_element) {
        return cast<Gringo::Input::OptimizeElement>();
    }
    throw std::runtime_error("optimize tuple expected");
}

template <> [[nodiscard]] auto clingo_ast::convert<Gringo::Input::Edge>() const -> Gringo::Input::Edge {
    if (type_ == clingo_ast_type_edge) {
        return cast<Gringo::Input::Edge>();
    }
    throw std::runtime_error("edge expected");
}

template <> [[nodiscard]] auto clingo_ast::convert<Gringo::Input::StmTheory>() const -> Gringo::Input::StmTheory {
    if (type_ != clingo_ast_type_statement_theory) {
        throw std::runtime_error("theory expected");
    }
    return cast<Gringo::Input::StmTheory>();
}

template <> [[nodiscard]] auto clingo_ast::convert<Gringo::Input::Stm>() const -> Gringo::Input::Stm {
    switch (type_) {
        case clingo_ast_type_statement_rule: {
            return cast<Gringo::Input::StmRule>();
        }
        case clingo_ast_type_statement_theory: {
            return cast<Gringo::Input::StmTheory>();
        }
        case clingo_ast_type_statement_optimize: {
            return cast<Gringo::Input::StmOptimize>();
        }
        case clingo_ast_type_statement_weak_constraint: {
            return cast<Gringo::Input::StmWeakConstraint>();
        }
        case clingo_ast_type_statement_show: {
            return cast<Gringo::Input::StmShow>();
        }
        case clingo_ast_type_statement_show_signature: {
            return cast<Gringo::Input::StmShowSig>();
        }
        case clingo_ast_type_statement_defined: {
            return cast<Gringo::Input::StmDefined>();
        }
        case clingo_ast_type_statement_external: {
            return cast<Gringo::Input::StmExternal>();
        }
        case clingo_ast_type_statement_edge: {
            return cast<Gringo::Input::StmEdge>();
        }
        case clingo_ast_type_statement_heuristic: {
            return cast<Gringo::Input::StmHeuristic>();
        }
        case clingo_ast_type_statement_script: {
            return cast<Gringo::Input::StmScript>();
        }
        case clingo_ast_type_statement_program: {
            return cast<Gringo::Input::StmProgram>();
        }
        case clingo_ast_type_statement_include: {
            return cast<Gringo::Input::StmInclude>();
        }
        case clingo_ast_type_statement_const: {
            return cast<Gringo::Input::StmConst>();
        }
        case clingo_ast_type_statement_comment: {
            return cast<Gringo::Input::StmComment>();
        }
        default: {
            throw std::runtime_error("statement expected");
        }
    }
}

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
extern "C" auto clingo_ast_construct(clingo_lib_t *lib, clingo_ast_type_t type, clingo_ast_t **ast, ...) -> bool {
    using namespace Gringo::Input;
    CLINGO_TRY {
        if (lib == nullptr || ast == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        *ast = nullptr;
        switch (static_cast<clingo_ast_type_e>(type)) {
            case clingo_ast_type_projection: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                va_end(args);
                *ast = construct_ast<Gringo::Input::Projection>(type, convert_loc(lib, loc));
                break;
            }
            case clingo_ast_type_term_variable: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto anonymous = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<Gringo::Input::TermVariable>(type, convert_loc(lib, loc), lib->store->string(name),
                                                                  anonymous != 0);
                break;
            }
            case clingo_ast_type_term_symbolic: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sym = va_arg(args, clingo_symbol_t);
                va_end(args);
                *ast = construct_ast<Gringo::Input::TermSymbol>(type, convert_loc(lib, loc),
                                                                Gringo::Symbol::from_rep(sym));
                break;
            }
            case clingo_ast_type_term_tuple: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **pool = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<Gringo::Input::TermTuple>(
                    type, convert_loc(lib, loc), convert_ast_vec<Gringo::Input::TupleElement>(pool, size));
                return true;
            }
            case clingo_ast_type_term_function: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto const **pool = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                auto sign = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<Gringo::Input::TermFunction>(
                    type, convert_loc(lib, loc), lib->store->string(name),
                    convert_ast_vec<Gringo::Input::ArgumentTuple>(pool, size), sign != 0);
                break;
            }
            case clingo_ast_type_term_absolute: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **pool = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<Gringo::Input::TermAbs>(type, convert_loc(lib, loc),
                                                             convert_ast_vec<Gringo::Input::Term>(pool, size));
                break;
            }
            case clingo_ast_type_term_unary_operation: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto op = va_arg(args, int);
                auto const *rhs = va_arg(args, clingo_ast_t *);
                va_end(args);
                *ast = construct_ast<Gringo::Input::TermUnary>(
                    type, convert_loc(lib, loc), static_cast<Gringo::Input::UnaryOperator>(op),
                    Gringo::Util::make_immutable<Gringo::Input::Term>(rhs->convert<Gringo::Input::Term>()));
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
                *ast = construct_ast<Gringo::Input::TermBinary>(
                    type, convert_loc(lib, loc),
                    Gringo::Util::make_immutable<Gringo::Input::Term>(lhs->convert<Gringo::Input::Term>()),
                    static_cast<Gringo::Input::BinaryOperator>(op),
                    Gringo::Util::make_immutable<Gringo::Input::Term>(rhs->convert<Gringo::Input::Term>()));
                break;
            }
            case clingo_ast_type_argument_tuple: {
                std::va_list args;
                va_start(args, ast);
                auto const **tuple = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<Gringo::Input::ArgumentTuple>(
                    type, convert_ast_vec<Gringo::Input::Argument>(tuple, size));
                break;
            }
            case clingo_ast_type_left_guard: {
                std::va_list args;
                va_start(args, ast);
                auto const *left = va_arg(args, clingo_ast const *);
                auto right = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<Gringo::Input::LGuard::value_type>(type, left->convert<Gringo::Input::Term>(),
                                                                        static_cast<Gringo::Input::Relation>(right));
                break;
            }
            case clingo_ast_type_right_guard: {
                std::va_list args;
                va_start(args, ast);
                auto left = va_arg(args, int);
                auto const *right = va_arg(args, clingo_ast const *);
                va_end(args);
                *ast = construct_ast<Gringo::Input::RGuard::value_type>(
                    type, static_cast<Gringo::Input::Relation>(left), right->convert<Gringo::Input::Term>());
                break;
            }
            case clingo_ast_type_literal_boolean: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sign = va_arg(args, int);
                auto value = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<Gringo::Input::LitBool>(type, convert_loc(lib, loc),
                                                             static_cast<Gringo::Input::Sign>(sign), value != 0);
                break;
            }
            case clingo_ast_type_literal_symbolic: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sign = va_arg(args, int);
                auto const *atom = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<Gringo::Input::LitSymbolic>(type, convert_loc(lib, loc),
                                                                 static_cast<Gringo::Input::Sign>(sign),
                                                                 atom->convert<Gringo::Input::Term>());
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
                *ast = construct_ast<Gringo::Input::LitComparison>(
                    type, convert_loc(lib, loc), static_cast<Gringo::Input::Sign>(sign),
                    left->convert<Gringo::Input::Term>(), convert_ast_vec<Gringo::Input::Guard>(right, size));
                break;
            }
            case clingo_ast_type_unparsed_element: {
                std::va_list args;
                va_start(args, ast);
                auto const **ops = va_arg(args, char const **);
                auto size = va_arg(args, size_t);
                auto const *term = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<Gringo::Input::UnparsedElement>(type, convert_string_array(lib, ops, size),
                                                                     term->convert<Gringo::Input::TheoryTerm>());
                break;
            }
            case clingo_ast_type_theory_term_variable: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto anonymous = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<Gringo::Input::TheoryTermVariable>(type, convert_loc(lib, loc),
                                                                        lib->store->string(name), anonymous != 0);
                break;
            }
            case clingo_ast_type_theory_term_symbolic: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sym = va_arg(args, clingo_symbol_t);
                va_end(args);
                *ast = construct_ast<Gringo::Input::TheoryTermSymbol>(type, convert_loc(lib, loc),
                                                                      Gringo::Symbol::from_rep(sym));
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
                *ast = construct_ast<Gringo::Input::TheoryTermTuple>(
                    type, convert_loc(lib, loc), static_cast<TheoryTermTupleType>(tuple_type),
                    convert_ast_vec<Gringo::Input::TheoryTerm>(arguments, size));
                break;
            }
            case clingo_ast_type_theory_term_function: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto const **arguments = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<Gringo::Input::TheoryTermFunction>(
                    type, convert_loc(lib, loc), lib->store->string(name),
                    convert_ast_vec<Gringo::Input::TheoryTerm>(arguments, size));
                break;
            }
            case clingo_ast_type_theory_term_unparsed: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **elems = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<Gringo::Input::TheoryTermUnparsed>(
                    type, convert_loc(lib, loc), convert_ast_vec<Gringo::Input::UnparsedElement>(elems, size));
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
                *ast = construct_ast<Gringo::Input::SetAggregateElement>(
                    type, convert_loc(lib, loc), lit->convert<Lit>(), convert_ast_vec<Gringo::Input::Lit>(cond, size));
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
                *ast = construct_ast<Gringo::Input::TheoryElement>(
                    type, convert_loc(lib, loc), convert_ast_vec<Gringo::Input::TheoryTerm>(tuple, tuple_size),
                    convert_ast_vec<Gringo::Input::Lit>(cond, cond_size));
                break;
            }
            case clingo_ast_type_theory_right_guard: {
                std::va_list args;
                va_start(args, ast);
                auto const *op = va_arg(args, char const *);
                auto const *term = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<Gringo::Input::TheoryRGuard::value_type>(
                    type, lib->store->string(op), term->convert<Gringo::Input::TheoryTerm>());
                break;
            }
            case clingo_ast_type_body_simple_literal: {
                std::va_list args;
                va_start(args, ast);
                auto const *lit = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<Gringo::Input::BdLitSimple>(type, lit->convert<Gringo::Input::Lit>());
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
                *ast = construct_ast<Gringo::Input::BdLitAggregateElement>(
                    type, convert_loc(lib, loc), convert_ast_vec<Gringo::Input::Term>(tuple, tuple_size),
                    convert_ast_vec<Gringo::Input::Lit>(cond, cond_size));
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
                *ast = construct_ast<Gringo::Input::BdLitAggregate>(
                    type, convert_loc(lib, loc), static_cast<Gringo::Input::Sign>(sign),
                    convert_ast_opt<Gringo::Input::LGuard::value_type>(lhs),
                    static_cast<Gringo::Input::AggregateFunction>(fun),
                    convert_ast_vec<Gringo::Input::BdLitAggregateElement>(elems, elems_size),
                    convert_ast_opt<Gringo::Input::RGuard::value_type>(rhs));
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
                *ast = construct_ast<Gringo::Input::BdLitSetAggregate>(
                    type, convert_loc(lib, loc), static_cast<Gringo::Input::Sign>(sign),
                    convert_ast_opt<Gringo::Input::LGuard::value_type>(lhs),
                    convert_ast_vec<Gringo::Input::SetAggregateElement>(elems, elems_size),
                    convert_ast_opt<Gringo::Input::RGuard::value_type>(rhs));
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
                *ast = construct_ast<Gringo::Input::BdLitTheoryAtom>(
                    type, convert_loc(lib, loc), static_cast<Gringo::Input::Sign>(sign),
                    term->convert<Gringo::Input::Term>(),
                    convert_ast_vec<Gringo::Input::TheoryElement>(elems, elems_size),
                    convert_ast_opt<Gringo::Input::TheoryRGuard::value_type>(rhs));
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
                *ast = construct_ast<Gringo::Input::BdLitConjunction>(
                    type, Gringo::Input::CondLit{convert_loc(lib, loc), lit->convert<Gringo::Input::Lit>(),
                                                 convert_ast_vec<Gringo::Input::Lit>(cond, cond_size)});
                break;
            }
            case clingo_ast_type_head_simple_literal: {
                std::va_list args;
                va_start(args, ast);
                auto const *lit = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<Gringo::Input::HdLitSimple>(type, lit->convert<Gringo::Input::Lit>());
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
                *ast = construct_ast<Gringo::Input::HdLitAggregateElement>(
                    type, convert_loc(lib, loc), convert_ast_vec<Gringo::Input::Term>(tuple, tuple_size),
                    lit->convert<Lit>(), convert_ast_vec<Gringo::Input::Lit>(cond, cond_size));
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
                *ast = construct_ast<Gringo::Input::HdLitAggregate>(
                    type, convert_loc(lib, loc), convert_ast_opt<Gringo::Input::LGuard::value_type>(lhs),
                    static_cast<Gringo::Input::AggregateFunction>(fun),
                    convert_ast_vec<Gringo::Input::HdLitAggregateElement>(elems, elems_size),
                    convert_ast_opt<Gringo::Input::RGuard::value_type>(rhs));
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
                *ast = construct_ast<Gringo::Input::HdLitSetAggregate>(
                    type, convert_loc(lib, loc), convert_ast_opt<Gringo::Input::LGuard::value_type>(lhs),
                    convert_ast_vec<Gringo::Input::SetAggregateElement>(elems, elems_size),
                    convert_ast_opt<Gringo::Input::RGuard::value_type>(rhs));
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
                *ast = construct_ast<Gringo::Input::HdLitTheoryAtom>(
                    type, convert_loc(lib, loc), term->convert<Gringo::Input::Term>(),
                    convert_ast_vec<Gringo::Input::TheoryElement>(elems, elems_size),
                    convert_ast_opt<Gringo::Input::TheoryRGuard::value_type>(rhs));
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
                *ast = construct_ast<Gringo::Input::CondLit>(
                    type, Gringo::Input::CondLit{convert_loc(lib, loc), lit->convert<Gringo::Input::Lit>(),
                                                 convert_ast_vec<Gringo::Input::Lit>(cond, cond_size)});
                break;
            }
            case clingo_ast_type_head_disjunction: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **elems = va_arg(args, clingo_ast_t const **);
                auto elems_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<Gringo::Input::HdLitDisjunction>(
                    type, convert_loc(lib, loc),
                    convert_ast_vec<Gringo::Input::HdLitDisjunctionElement>(elems, elems_size));
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
                *ast = construct_ast<Gringo::Input::StmRule>(type, convert_loc(lib, loc), head->convert<HdLit>(),
                                                             convert_ast_vec<BdLit>(body, body_size));
                break;
            }
            case clingo_ast_type_theory_operator_definition: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto priority = va_arg(args, int);
                auto op_type = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<Gringo::Input::TheoryOpDefinition>(type, convert_loc(lib, loc),
                                                                        lib->store->string(name), priority,
                                                                        static_cast<TheoryOpType>(op_type));
                break;
            }
            case clingo_ast_type_theory_term_definition: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto const **ops = va_arg(args, clingo_ast_t const **);
                auto ops_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<Gringo::Input::TheoryTermDefinition>(
                    type, convert_loc(lib, loc), lib->store->string(name),
                    convert_ast_vec<TheoryOpDefinition>(ops, ops_size));
                break;
            }
            case clingo_ast_type_theory_guard_definition: {
                std::va_list args;
                va_start(args, ast);
                auto const **ops = va_arg(args, char const **);
                auto ops_size = va_arg(args, size_t);
                auto const *term = va_arg(args, char const *);
                va_end(args);
                *ast = construct_ast<Gringo::Input::TheoryRGuardDefinition>(
                    type, convert_string_array(lib, ops, ops_size), lib->store->string(term));
                break;
            }
            case clingo_ast_type_theory_atom_definition: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto arity = va_arg(args, int);
                auto const *term = va_arg(args, char const *);
                auto const *guard = va_arg(args, clingo_ast_t const *);
                auto atom_type = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<Gringo::Input::TheoryAtomDefinition>(
                    type, convert_loc(lib, loc), lib->store->string(name), static_cast<int>(arity),
                    lib->store->string(term), convert_ast_opt<TheoryRGuardDefinition>(guard),
                    static_cast<TheoryAtomType>(atom_type));
                break;
            }
            case clingo_ast_type_statement_theory: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto const **terms = va_arg(args, clingo_ast_t const **);
                auto terms_size = va_arg(args, size_t);
                auto const **atoms = va_arg(args, clingo_ast_t const **);
                auto atoms_size = va_arg(args, size_t);
                va_end(args);
                *ast =
                    construct_ast<Gringo::Input::StmTheory>(type, convert_loc(lib, loc), lib->store->string(name),
                                                            convert_ast_vec<TheoryTermDefinition>(terms, terms_size),
                                                            convert_ast_vec<TheoryAtomDefinition>(atoms, atoms_size));
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
                *ast = construct_ast<Gringo::Input::OptimizeTuple>(type, weight->convert<Term>(),
                                                                   convert_ast_opt<Term>(prio),
                                                                   convert_ast_vec<Term>(terms, terms_size));
                break;
            }
            case clingo_ast_type_optimize_element: {
                std::va_list args;
                va_start(args, ast);
                auto const *tuple = va_arg(args, clingo_ast_t const *);
                auto const **cond = va_arg(args, clingo_ast_t const **);
                auto cond_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<Gringo::Input::OptimizeElement>(type, tuple->convert<OptimizeTuple>(),
                                                                     convert_ast_vec<Lit>(cond, cond_size));
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
                *ast = construct_ast<Gringo::Input::StmOptimize>(type, convert_loc(lib, loc),
                                                                 static_cast<OptimizeType>(optimize_type),
                                                                 convert_ast_vec<OptimizeElement>(elems, elems_size));
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
                *ast = construct_ast<Gringo::Input::StmWeakConstraint>(type, convert_loc(lib, loc),
                                                                       convert_ast_vec<BdLit>(body, body_size),
                                                                       tuple->convert<OptimizeTuple>());
                break;
            }
            case clingo_ast_type_edge: {
                std::va_list args;
                va_start(args, ast);
                auto const *u = va_arg(args, clingo_ast_t const *);
                auto const *v = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<Gringo::Input::Edge>(type, u->convert<Term>(), v->convert<Term>());
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
                *ast = construct_ast<Gringo::Input::StmShow>(type, convert_loc(lib, loc), term->convert<Term>(),
                                                             convert_ast_vec<BdLit>(body, body_size));
                break;
            }
            case clingo_ast_type_statement_show_signature: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto arity = va_arg(args, int);
                auto sign = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<Gringo::Input::StmShowSig>(type, convert_loc(lib, loc), sign != 0,
                                                                lib->store->string(name), arity);
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
                *ast = construct_ast<Gringo::Input::StmProject>(type, convert_loc(lib, loc), atom->convert<Term>(),
                                                                convert_ast_vec<BdLit>(body, body_size));
                break;
            }
            case clingo_ast_type_statement_project_signature: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto arity = va_arg(args, int);
                auto sign = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<Gringo::Input::StmProjectSig>(type, convert_loc(lib, loc), sign != 0,
                                                                   lib->store->string(name), arity);
                break;
            }
            case clingo_ast_type_statement_defined: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto arity = va_arg(args, int);
                auto sign = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<Gringo::Input::StmDefined>(type, convert_loc(lib, loc), sign != 0,
                                                                lib->store->string(name), arity);
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
                *ast = construct_ast<Gringo::Input::StmExternal>(type, convert_loc(lib, loc), atom->convert<Term>(),
                                                                 convert_ast_vec<BdLit>(body, body_size),
                                                                 convert_ast_opt<Term>(external_type));
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
                *ast = construct_ast<Gringo::Input::StmEdge>(type, convert_loc(lib, loc),
                                                             convert_ast_vec<Edge>(edges, edges_size),
                                                             convert_ast_vec<BdLit>(body, body_size));
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
                *ast = construct_ast<Gringo::Input::StmHeuristic>(
                    type, convert_loc(lib, loc), atom->convert<Term>(), convert_ast_vec<BdLit>(body, body_size),
                    weight->convert<Term>(), convert_ast_opt<Term>(priority), modifier->convert<Term>());
                break;
            }
            case clingo_ast_type_statement_include: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *value = va_arg(args, char const *);
                auto include_type = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<Gringo::Input::StmInclude>(type, convert_loc(lib, loc),
                                                                static_cast<IncludeType>(include_type), value);
                break;
            }
            case clingo_ast_type_statement_program: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto const **arguments = va_arg(args, char const **);
                auto arguments_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<Gringo::Input::StmProgram>(type, convert_loc(lib, loc), lib->store->string(name),
                                                                convert_string_array(lib, arguments, arguments_size));
                break;
            }
            case clingo_ast_type_statement_script: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *value = va_arg(args, char const *);
                auto const *script_type = va_arg(args, char const *);
                va_end(args);
                *ast = construct_ast<Gringo::Input::StmScript>(type, convert_loc(lib, loc),
                                                               lib->store->string(script_type), value);
                break;
            }
            case clingo_ast_type_statement_const: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto const *term = va_arg(args, clingo_ast_t const *);
                auto const_type = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<Gringo::Input::StmConst>(type, convert_loc(lib, loc),
                                                              static_cast<ConstType>(const_type),
                                                              lib->store->string(name), term->convert<Term>());
                break;
            }
            case clingo_ast_type_statement_comment: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *value = va_arg(args, char const *);
                auto comment_type = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<Gringo::Input::StmComment>(type, convert_loc(lib, loc),
                                                                static_cast<CommentType>(comment_type), value);
                break;
            }
        }
    }
    CLINGO_CATCH(lib);
}
// NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay)

extern "C" auto clingo_ast_to_string_size(clingo_ast_t *ast, size_t *size) -> bool {
    CLINGO_TRY {
        if (ast == nullptr || size == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        *size = print_size(*ast);
    }
    CLINGO_CATCH(nullptr);
}

extern "C" auto clingo_ast_to_string(clingo_ast_t *ast, char *string, size_t size) -> bool {
    CLINGO_TRY {
        if (ast == nullptr || string == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        print(string, size, *ast);
    }
    CLINGO_CATCH(nullptr);
}

extern "C" auto clingo_ast_less_than(clingo_ast_t *a, clingo_ast_t *b) -> bool { return a->less_than(*b); }

extern "C" auto clingo_ast_equal(clingo_ast_t *a, clingo_ast_t *b) -> bool { return a->equal_to(*b); }

extern "C" auto clingo_ast_hash(clingo_ast_t *ast) -> size_t { return ast->hash(); }

extern "C" auto clingo_ast_copy(clingo_ast_t *ast, clingo_ast_t **copy) -> bool {
    CLINGO_TRY {
        if (ast == nullptr || copy == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        *copy = ast->copy().release();
    }
    CLINGO_CATCH(nullptr);
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
            throw std::invalid_argument("invalid arguments");
        }
        *type = ast->get_type();
        return true;
    }
    CLINGO_CATCH(nullptr);
}

extern "C" auto clingo_ast_attribute_get_number(clingo_ast_t *ast, clingo_ast_attribute_t attribute, int *value)
    -> bool {
    CLINGO_TRY {
        if (ast == nullptr || value == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        if (auto num = ast->get_number(attribute); num) {
            *value = *num;
            return true;
        }
        return false;
    }
    CLINGO_CATCH(nullptr);
}

extern "C" auto clingo_ast_attribute_get_symbol(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                clingo_symbol_t *value) -> bool {
    CLINGO_TRY {
        if (ast == nullptr || value == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        if (auto sym = ast->get_symbol(attribute); sym) {
            *value = *sym;
            return true;
        }
        return false;
    }
    CLINGO_CATCH(nullptr);
}

extern "C" auto clingo_ast_attribute_get_location(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                  clingo_location_t *value) -> bool {
    CLINGO_TRY {
        if (ast == nullptr || value == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        if (auto loc = ast->get_location(attribute); loc) {
            *value = *loc;
            return true;
        }
        return false;
    }
    CLINGO_CATCH(nullptr);
}

extern "C" auto clingo_ast_attribute_get_string(clingo_ast_t *ast, clingo_ast_attribute_t attribute, char const **value)
    -> bool {
    CLINGO_TRY {
        if (ast == nullptr || value == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        if (auto str = ast->get_string(attribute); str) {
            *value = *str;
            return true;
        }
        return false;
    }
    CLINGO_CATCH(nullptr);
}

extern "C" auto clingo_ast_attribute_get_string_array(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                      char const **value, size_t *size) -> bool {
    CLINGO_TRY {
        if (ast == nullptr || size == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        if (auto vec = ast->get_string_vec(attribute); vec) {
            *size = vec->size();
            if (value != nullptr) {
                std::transform(vec->begin(), vec->end(), value, [](auto str) { return str.c_str(); });
            }
            return true;
        }
        return false;
    }
    CLINGO_CATCH(nullptr);
}

extern "C" auto clingo_ast_attribute_get_ast(clingo_ast_t *ast, clingo_ast_attribute_t attribute, clingo_ast_t **value)
    -> bool {
    CLINGO_TRY {
        if (ast == nullptr || value == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        if (auto val = ast->get_ast(attribute); val) {
            *value = val->release();
            return true;
        }
        return false;
    }
    CLINGO_CATCH(nullptr);
}

extern "C" auto clingo_ast_attribute_get_ast_array(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                   clingo_ast_t ***value, size_t *size) -> bool {
    CLINGO_TRY {
        if (ast == nullptr || value == nullptr || size == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        if (auto val = ast->get_ast_vec(attribute); val) {
            std::tie(*value, *size) = val->release();
            return true;
        }
        return false;
    }
    CLINGO_CATCH(nullptr);
}

extern "C" auto clingo_ast_parse_expression(clingo_lib_t *lib, clingo_ast_parse_type_t type, char const *string,
                                            clingo_ast_t **ast) -> bool {
    CLINGO_TRY {
        if (ast == nullptr || string == nullptr || ast == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        switch (type) {
            case clingo_ast_parse_type_term: {
                auto term = Gringo::Input::parse_term(lib->log, *lib->store, string);
                if (lib->log.has_error() || !term) {
                    lib->log.reset();
                    throw std::runtime_error("parsing term failed");
                }
                auto owner = Gringo::Util::make_immutable<std::any>(std::move(term).value());
                auto const *ptr = std::any_cast<Gringo::Input::Term>(&owner.get());
                *ast = make_ast(owner, *ptr).release();
                break;
            }
            case clingo_ast_parse_type_theory_term: {
                auto term = Gringo::Input::parse_theory_term(lib->log, *lib->store, string);
                if (lib->log.has_error() || !term) {
                    lib->log.reset();
                    throw std::runtime_error("parsing theory term failed");
                }
                auto owner = Gringo::Util::make_immutable<std::any>(std::move(term).value());
                auto const *ptr = std::any_cast<Gringo::Input::TheoryTerm>(&owner.get());
                *ast = make_ast(owner, *ptr).release();
                break;
            }
            case clingo_ast_parse_type_literal: {
                auto lit = Gringo::Input::parse_literal(lib->log, *lib->store, string);
                if (lib->log.has_error() || !lit) {
                    lib->log.reset();
                    throw std::runtime_error("parsing literal failed");
                }
                auto owner = Gringo::Util::make_immutable<std::any>(std::move(lit).value());
                auto const *ptr = std::any_cast<Gringo::Input::Lit>(&owner.get());
                *ast = make_ast(owner, *ptr).release();
                break;
            }
            case clingo_ast_parse_type_head_literal: {
                auto lit = Gringo::Input::parse_head_literal(lib->log, *lib->store, string);
                if (lib->log.has_error() || !lit) {
                    lib->log.reset();
                    throw std::runtime_error("parsing head literal failed");
                }
                auto owner = Gringo::Util::make_immutable<std::any>(std::move(lit).value());
                auto const *ptr = std::any_cast<Gringo::Input::HdLit>(&owner.get());
                *ast = make_ast(owner, *ptr).release();
                break;
            }
            case clingo_ast_parse_type_body_literal: {
                auto lit = Gringo::Input::parse_body_literal(lib->log, *lib->store, string);
                if (lib->log.has_error() || !lit) {
                    lib->log.reset();
                    throw std::runtime_error("parsing body literal failed");
                }
                auto owner = Gringo::Util::make_immutable<std::any>(std::move(lit).value());
                auto const *ptr = std::any_cast<Gringo::Input::BdLit>(&owner.get());
                *ast = make_ast(owner, *ptr).release();
                break;
            }
            case clingo_ast_parse_type_statement: {
                auto lit = Gringo::Input::parse_statement(lib->log, *lib->store, string);
                if (lib->log.has_error() || !lit) {
                    lib->log.reset();
                    throw std::runtime_error("parsing statement failed");
                }
                auto owner = Gringo::Util::make_immutable<std::any>(std::move(lit).value());
                auto const *ptr = std::any_cast<Gringo::Input::Stm>(&owner.get());
                *ast = make_ast(owner, *ptr).release();
                break;
            }
            default: {
                throw std::invalid_argument("invalid arguments");
            }
        }
    }
    CLINGO_CATCH(lib);
}

struct clingo_ast_scanner {
  public:
    clingo_ast_scanner(clingo_lib_t *lib) : lib_{lib} {}
    [[nodiscard]] auto next() -> std::unique_ptr<clingo_ast_t> {
        while (!scanners_.empty()) {
            auto stm = scanners_.front().scan();
            if (stm) {
                auto owner = Gringo::Util::make_immutable<std::any>(*std::move(stm));
                auto const *ptr = std::any_cast<Gringo::Input::Stm>(&owner.get());
                return make_ast(owner, *ptr);
            }
            scanners_.pop_front();
        }
        if (lib_->last_code != clingo_error_success) {
            throw std::runtime_error("parsing failed");
        }
        return nullptr;
    }
    [[nodiscard]] auto lib() const -> clingo_lib_t * { return lib_; }
    auto scan_string(std::string str) {
        strings_.emplace_front(std::move(str));
        scanners_.emplace_front(Gringo::Input::scan_string(lib_->log, *lib_->store, strings_.front()));
    }
    auto scan_file(char const *path) {
        if (std::strcmp(path, "-") == 0) {
            scanners_.emplace_front(Gringo::Input::scan_stream(lib_->log, *lib_->store, std::cin));
        } else {
            scanners_.emplace_front(Gringo::Input::scan_file(lib_->log, *lib_->store, path));
        }
    }

  private:
    clingo_lib_t *lib_;
    std::forward_list<std::string> strings_;
    std::forward_list<Gringo::Input::Scanner> scanners_;
};

extern "C" auto clingo_ast_scan_string(clingo_lib_t *lib, char const *program, clingo_ast_scanner_t **scanner) -> bool {
    CLINGO_TRY {
        if (lib == nullptr || program == nullptr || scanner == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        auto res = std::make_unique<clingo_ast_scanner>(lib);
        res->scan_string(program);
        *scanner = res.release();
    }
    CLINGO_CATCH(lib);
}

extern "C" auto clingo_ast_scan_files(clingo_lib_t *lib, char const *const *files, size_t size,
                                      clingo_ast_scanner_t **scanner) -> bool {
    CLINGO_TRY {
        if (lib == nullptr || (files == nullptr && size != 0) || scanner == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        auto res = std::make_unique<clingo_ast_scanner>(lib);
        auto span = std::span(files, size);
        if (span.empty()) {
            res->scan_file("-");
        } else {
            std::for_each(span.rbegin(), span.rend(), [&res](auto const *path) { res->scan_file(path); });
        }
        *scanner = res.release();
    }
    CLINGO_CATCH(lib);
}

extern "C" auto clingo_ast_scanner_next(clingo_ast_scanner_t *scanner, clingo_ast_t **ast) -> bool {
    CLINGO_TRY {
        if (scanner == nullptr || scanner == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        *ast = scanner->next().release();
    }
    CLINGO_CATCH(scanner != nullptr ? scanner->lib() : nullptr);
}

extern "C" void clingo_ast_scanner_close(clingo_ast_scanner_t *scanner) {
    if (scanner != nullptr) {
        scanner->lib()->log.reset();
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        delete scanner;
    }
}

struct clingo_ast_rewrite_context {
    clingo_lib *lib;
    Gringo::Input::TheoryAtomParser parser = {};
    Gringo::Input::ParamMap param_map = {};
    Gringo::Input::ConstMap const_map = {};
    Gringo::Input::RewriteOptions options = {};
    Gringo::Input::RewriteContext ctx = {lib->log, *lib->store, options, parser, param_map, const_map};
    Gringo::Util::ordered_map<Gringo::String, Gringo::String> param_unmap = {};
};

extern "C" auto clingo_ast_rewrite_context_create(clingo_lib_t *lib, clingo_ast_rewrite_context_t **context) -> bool {
    CLINGO_TRY {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        *context = new clingo_ast_rewrite_context{lib};
    }
    CLINGO_CATCH(lib);
}

extern "C" void clingo_ast_rewrite_context_free(clingo_ast_rewrite_context_t *context) {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    delete context;
}

extern "C" auto clingo_ast_rewrite_context_add_param(clingo_ast_rewrite_context_t *context, char const *param) -> bool {
    auto *lib = context->lib;
    CLINGO_TRY {
        if (auto prm = lib->store->string(param); context->param_map.emplace(prm).second) {
            auto var = lib->store->string("$" + std::to_string(context->param_unmap.size()));
            context->param_unmap.emplace(var, prm);
        }
    }
    CLINGO_CATCH(lib);
}

extern "C" void clingo_ast_rewrite_context_clear_params(clingo_ast_rewrite_context_t *context) {
    context->param_map.clear();
    context->param_unmap.clear();
}

extern "C" auto clingo_ast_rewrite_context_add_theory(clingo_ast_rewrite_context_t *context, clingo_ast_t const *theory)
    -> bool {
    auto *lib = context->lib;
    CLINGO_TRY {
        auto stm = theory->convert<Gringo::Input::StmTheory>();
        context->parser.add_theory(lib->log, stm);
        if (lib->log.has_error()) {
            lib->log.reset();
            throw std::runtime_error("adding theory failed");
        }
    }
    CLINGO_CATCH(lib);
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
    context->options.project_mode = static_cast<Gringo::Input::ProjectionMode>(value);
}

extern "C" auto clingo_ast_rewrite_context_get_lib(clingo_ast_rewrite_context_t *context) -> clingo_lib_t * {
    return context->lib;
}

extern "C" auto clingo_ast_rewrite(clingo_ast_rewrite_context_t *context, clingo_ast_t *statement,
                                   clingo_ast_t ***result, size_t *result_size) -> bool {
    using namespace Gringo::Input;
    auto *lib = context->lib;
    CLINGO_TRY {
        *result = nullptr;
        *result_size = 0;
        auto stms = StmVec{};
        auto stm = statement->convert<Stm>();
        rewrite(context->ctx, stm, stms);
        if (lib->log.has_error()) {
            lib->log.reset();
            throw std::runtime_error("rewriting statement failed");
        }
        ASTVec res{stms.size()};
        int i = 0;
        for (auto &stm : stms) {
            if (auto res_stm = unmap_params(*lib->store, context->param_unmap, stm); res_stm) {
                stm = *std::move(res_stm);
            }
            auto owner = Gringo::Util::make_immutable<std::any>(std::move(stm));
            auto const *ptr = std::any_cast<Gringo::Input::Stm>(&owner.get());
            res[i] = make_ast(owner, *ptr).release();
            ++i;
        }
        std::tie(*result, *result_size) = res.release();
    }
    CLINGO_CATCH(lib);
}

// NOLINTEND(cppcoreguidelines-macro-usage)
