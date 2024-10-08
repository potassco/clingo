#include "lib.hh"
#include "streams.hh"

#include <clingo/input/parser.hh>
#include <clingo/input/print.hh>
#include <clingo/input/rewrite.hh>

#include <clingo/input/rewrite/rewrite_theory.hh>
#include <clingo/input/rewrite/substitute.hh>

#include <clingo/util/algorithm.hh>
#include <clingo/util/ordered_map.hh>
#include <clingo/util/ordered_set.hh>
#include <clingo/util/type_traits.hh>

#include <any>
#include <cstdarg>
#include <cstring>
#include <forward_list>
#include <fstream>
#include <span>

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

namespace {

class ASTVec;

using Owner = Clingo::Util::immutable_value<std::any>;

template <class T>
auto make_ast(Owner const &owner, Clingo::Util::immutable_value<T> const &ptr) -> std::unique_ptr<clingo_ast_t>;
template <class T> auto make_ast(Owner const &owner, std::optional<T> const &opt) -> std::unique_ptr<clingo_ast_t>;
template <class... T> auto make_ast(Owner const &owner, std::variant<T...> const &var) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::LGuard::value_type const &guard) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::RGuard::value_type const &guard) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::TheoryRGuard const &guard) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::Projection const &projection) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::Term const &term) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::TheoryTerm const &term) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::ArgumentTuple const &tuple) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::TupleElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::Lit const &lit) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::UnparsedElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::TheoryElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::SetAggregateElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::BdLitAggregateElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::HdLitAggregateElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::HdLitDisjunctionElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::BdLit const &lit) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::HdLit const &lit) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::TheoryOpDefinition const &def) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::TheoryRGuardDefinition const &def) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::TheoryTermDefinition const &def) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::TheoryAtomDefinition const &def) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::OptimizeTuple const &tuple) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::OptimizeElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Clingo::Input::Edge const &edge) -> std::unique_ptr<clingo_ast_t>;

template <class T> auto make_ast_vec(Owner const &owner, std::span<T> vec) -> ASTVec;
template <class T> auto make_ast_vec(Owner const &owner, std::vector<T> const &vec) -> ASTVec;
template <class T> auto make_ast_vec(Owner const &owner, Clingo::Util::immutable_array<T> const &vec) -> ASTVec;

template <class T> auto convert_ast_vec(clingo_ast const **ast, size_t size) -> std::vector<T>;

auto convert_loc(clingo_lib_t *lib, clingo_location_t const *loc) -> Clingo::Input::Location {
    return {{*lib->store->string(loc->begin_file), loc->begin_line, loc->begin_column},
            {*lib->store->string(loc->end_file), loc->end_line, loc->end_column}};
}

auto convert_string_array(clingo_lib_t *lib, char const **array, size_t size) -> Clingo::StringVec {
    auto ret = Clingo::StringVec{};
    ret.reserve(size);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    std::transform(array, array + size, std::back_inserter(ret),
                   [lib](auto str) { return lib->store->string_ref(str); });
    return ret;
}

[[maybe_unused]] auto make_loc(Clingo::Input::Location const &loc) -> clingo_location_t {
    return {loc.begin().file().c_str(), loc.end().file().c_str(), loc.begin().line(),
            loc.end().line(),           loc.begin().column(),     loc.end().column()};
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
    [[nodiscard]] auto get_string_vec(clingo_ast_attribute_t attr) const -> std::optional<Clingo::StringSpan>;
    [[nodiscard]] auto get_ast(clingo_ast_attribute_t attr) const -> std::optional<std::unique_ptr<clingo_ast_t>>;
    [[nodiscard]] auto get_ast_vec(clingo_ast_attribute_t attr) const -> std::optional<ASTVec>;

    template <class T> [[nodiscard]] auto convert() const -> T = delete;
    template <class V> auto visit(V &&visit) const -> std::invoke_result_t<V, Clingo::Input::Projection const &>;

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

template <class T> auto make_ast_vec(Owner const &owner, Clingo::Util::immutable_array<T> const &vec) -> ASTVec {
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
auto make_ast(Owner const &owner, Clingo::Util::immutable_value<T> const &ptr) -> std::unique_ptr<clingo_ast_t> {
    return make_ast(owner, *ptr);
}

template <class T> auto make_ast(Owner const &owner, std::optional<T> const &opt) -> std::unique_ptr<clingo_ast_t> {
    if (opt) {
        return make_ast(owner, *opt);
    }
    return nullptr;
}

auto make_ast(Owner const &owner, Clingo::Input::LGuard::value_type const &guard) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_left_guard, &guard);
}

auto make_ast(Owner const &owner, Clingo::Input::RGuard::value_type const &guard) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_right_guard, &guard);
}

auto make_ast(Owner const &owner, Clingo::Input::TheoryRGuard const &guard) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_right_guard, &guard);
}

auto make_ast(Owner const &owner, Clingo::Input::Projection const &projection) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_projection, &projection);
}

auto make_ast(Owner const &owner, Clingo::Input::Term const &term) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, Clingo::Input::TermVariable>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_variable, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::TermSymbol>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_symbolic, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::TermTuple>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_tuple, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::TermFunction>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_function, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::TermAbs>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_absolute, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::TermUnary>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_unary_operation, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::TermBinary>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_binary_operation, &x);
            }
        },
        term);
}

auto make_ast(Owner const &owner, Clingo::Input::TheoryTerm const &term) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, Clingo::Input::TheoryTermVariable>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_term_variable, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::TheoryTermSymbol>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_term_symbolic, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::TheoryTermTuple>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_term_tuple, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::TheoryTermFunction>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_term_function, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::TheoryTermUnparsed>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_term_unparsed, &x);
            }
        },
        term);
}

auto make_ast(Owner const &owner, Clingo::Input::UnparsedElement const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_unparsed_element, &elem);
}

auto make_ast(Owner const &owner, Clingo::Input::ArgumentTuple const &tuple) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_argument_tuple, static_cast<void const *>(&tuple));
}

auto make_ast(Owner const &owner, Clingo::Input::TupleElement const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, Clingo::Input::Term>) {
                return make_ast(owner, x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::ArgumentTuple>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_argument_tuple, &x);
            }
        },
        elem);
}

auto make_ast(Owner const &owner, Clingo::Input::Lit const &lit) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, Clingo::Input::LitBool>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_literal_boolean, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::LitSymbolic>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_literal_symbolic, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::LitComparison>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_literal_comparison, &x);
            }
        },
        lit);
}

auto make_ast(Owner const &owner, Clingo::Input::HdLit const &lit) -> std::unique_ptr<clingo_ast_t> {
    using namespace Clingo::Input;
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

auto make_ast(Owner const &owner, Clingo::Input::BdLit const &lit) -> std::unique_ptr<clingo_ast_t> {
    using namespace Clingo::Input;
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

auto make_ast(Owner const &owner, Clingo::Input::TheoryElement const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_atom_element, &elem);
}

auto make_ast(Owner const &owner, Clingo::Input::SetAggregateElement const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_set_aggregate_element, &elem);
}

auto make_ast(Owner const &owner, Clingo::Input::BdLitAggregateElement const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_body_aggregate_element, &elem);
}

auto make_ast(Owner const &owner, Clingo::Input::HdLitAggregateElement const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_head_aggregate_element, &elem);
}

auto make_ast(Owner const &owner, Clingo::Input::HdLitDisjunctionElement const &elem) -> std::unique_ptr<clingo_ast_t> {
    using namespace Clingo::Input;
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

auto make_ast(Owner const &owner, Clingo::Input::TheoryOpDefinition const &def) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_operator_definition, &def);
}

auto make_ast(Owner const &owner, Clingo::Input::TheoryRGuardDefinition const &def) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_guard_definition, &def);
}

auto make_ast(Owner const &owner, Clingo::Input::TheoryTermDefinition const &def) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_term_definition, &def);
}

auto make_ast(Owner const &owner, Clingo::Input::TheoryAtomDefinition const &def) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_atom_definition, &def);
}

auto make_ast(Owner const &owner, Clingo::Input::OptimizeTuple const &tuple) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_optimize_tuple, &tuple);
}

auto make_ast(Owner const &owner, Clingo::Input::OptimizeElement const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_optimize_element, &elem);
}

auto make_ast(Owner const &owner, Clingo::Input::Edge const &edge) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_edge, &edge);
}

auto make_ast(Owner const &owner, Clingo::Input::Stm const &term) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner]<class T>(T const &x) {
            if constexpr (std::is_same_v<T, Clingo::Input::StmRule>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_rule, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::StmTheory>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_theory, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::StmOptimize>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_optimize, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::StmWeakConstraint>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_weak_constraint, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::StmShow>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_show, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::StmShowNothing>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_show_nothing, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::StmShowSig>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_show_signature, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::StmProject>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_project, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::StmProjectSig>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_project_signature, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::StmDefined>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_defined, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::StmExternal>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_external, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::StmEdge>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_edge, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::StmHeuristic>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_heuristic, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::StmScript>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_script, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::StmInclude>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_include, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::StmProgram>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_program, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::StmConst>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_const, &x);
            }
            if constexpr (std::is_same_v<T, Clingo::Input::StmComment>) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_comment, &x);
            }
        },
        term);
}

template <class T, class... A> auto construct_ast(clingo_ast_type_t type, A &&...args) -> clingo_ast * {
    auto owner = Clingo::Util::make_immutable<std::any>(T{std::forward<A>(args)...});
    auto *ptr = std::any_cast<T>(&owner.get());
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    return new clingo_ast{std::move(owner), static_cast<clingo_ast_type_e>(type), ptr};
}

} // namespace

template <class V>
auto clingo_ast::visit(V &&visit) const -> std::invoke_result_t<V, Clingo::Input::Projection const &> {
    using namespace Clingo::Input;
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
        case clingo_ast_type_statement_comment: {
            return std::invoke(std::forward<V>(visit), cast<StmComment>());
        }
    }
    throw std::invalid_argument("invalid ast type");
}

auto clingo_ast::get_type() const -> clingo_ast_type_e { return type_; }

auto clingo_ast::get_location(clingo_ast_attribute_t attr) const -> std::optional<clingo_location_t> {
    using namespace Clingo::Input;
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
    using namespace Clingo::Input;                                                                                     \
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
        return static_cast<clingo_symbol_t>(Clingo::Symbol::to_rep(cast<Type>().value));                               \
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
        TYPE(statement_comment, StmComment,
            ATTR(value, value())))
    // clang-format on
}

#undef ATTR
#define ATTR(attr, value)                                                                                              \
    case clingo_ast_attribute_##attr: {                                                                                \
        return cast<Type>().value;                                                                                     \
    }

auto clingo_ast::get_string_vec(clingo_ast_attribute_t attr) const -> std::optional<Clingo::StringSpan> {
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
    using namespace Clingo::Input;
    visit([&out]<class T>(T const &x) {
        if constexpr (Clingo::Util::matches<T, RGuard::value_type>) {
            out << " " << x.first << " " << x.second;
        } else if constexpr (Clingo::Util::matches<T, TheoryRGuard>) {
            out << " " << x.op() << " " << x.term();
        } else if constexpr (std::is_same_v<T, UnparsedElement>) {
            for (auto const &op : x.ops()) {
                out << op << " ";
            }
            out << x.term();
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
    using namespace Clingo::Input;
    return visit(
        [this](auto &x) { return Clingo::Util::hash_mix(Clingo::Util::value_hash_record<clingo_ast>(type_, x)); });
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

template <> [[nodiscard]] auto clingo_ast::convert<Clingo::Input::Term>() const -> Clingo::Input::Term {
    switch (type_) {
        case clingo_ast_type_term_variable: {
            return cast<Clingo::Input::TermVariable>();
        }
        case clingo_ast_type_term_symbolic: {
            return cast<Clingo::Input::TermSymbol>();
        }
        case clingo_ast_type_term_tuple: {
            return cast<Clingo::Input::TermTuple>();
        }
        case clingo_ast_type_term_function: {
            return cast<Clingo::Input::TermFunction>();
        }
        case clingo_ast_type_term_absolute: {
            return cast<Clingo::Input::TermAbs>();
        }
        case clingo_ast_type_term_unary_operation: {
            return cast<Clingo::Input::TermUnary>();
        }
        case clingo_ast_type_term_binary_operation: {
            return cast<Clingo::Input::TermBinary>();
        }
        default: {
            throw std::runtime_error("term expected");
        }
    }
}

template <> [[nodiscard]] auto clingo_ast::convert<Clingo::Input::TheoryTerm>() const -> Clingo::Input::TheoryTerm {
    switch (type_) {
        case clingo_ast_type_theory_term_variable: {
            return cast<Clingo::Input::TheoryTermVariable>();
        }
        case clingo_ast_type_theory_term_symbolic: {
            return cast<Clingo::Input::TheoryTermSymbol>();
        }
        case clingo_ast_type_theory_term_tuple: {
            return cast<Clingo::Input::TheoryTermTuple>();
        }
        case clingo_ast_type_theory_term_function: {
            return cast<Clingo::Input::TheoryTermFunction>();
        }
        case clingo_ast_type_theory_term_unparsed: {
            return cast<Clingo::Input::TheoryTermUnparsed>();
        }
        default: {
            throw std::runtime_error("theory term expected");
        }
    }
}

template <> [[nodiscard]] auto clingo_ast::convert<Clingo::Input::Lit>() const -> Clingo::Input::Lit {
    switch (type_) {
        case clingo_ast_type_literal_boolean: {
            return cast<Clingo::Input::LitBool>();
        }
        case clingo_ast_type_literal_symbolic: {
            return cast<Clingo::Input::LitSymbolic>();
        }
        case clingo_ast_type_literal_comparison: {
            return cast<Clingo::Input::LitComparison>();
        }
        default: {
            throw std::runtime_error("literal expected");
        }
    }
}

template <> [[nodiscard]] auto clingo_ast::convert<Clingo::Input::Argument>() const -> Clingo::Input::Argument {
    if (type_ == clingo_ast_type_projection) {
        return cast<Clingo::Input::Projection>();
    }
    return convert<Clingo::Input::Term>();
}

template <>
[[nodiscard]] auto clingo_ast::convert<Clingo::Input::ArgumentTuple>() const -> Clingo::Input::ArgumentTuple {
    if (type_ == clingo_ast_type_argument_tuple) {
        return cast<Clingo::Input::ArgumentTuple>();
    }
    throw std::runtime_error("argument tuple expected");
}

template <> [[nodiscard]] auto clingo_ast::convert<Clingo::Input::TupleElement>() const -> Clingo::Input::TupleElement {
    if (type_ == clingo_ast_type_argument_tuple) {
        return cast<Clingo::Input::ArgumentTuple>();
    }
    return convert<Clingo::Input::Term>();
}

template <>
[[nodiscard]] auto clingo_ast::convert<Clingo::Input::UnparsedElement>() const -> Clingo::Input::UnparsedElement {
    if (type_ == clingo_ast_type_unparsed_element) {
        return cast<Clingo::Input::UnparsedElement>();
    }
    throw std::runtime_error("unparsed element expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Clingo::Input::LGuard::value_type>() const -> Clingo::Input::LGuard::value_type {
    if (type_ == clingo_ast_type_left_guard) {
        return cast<Clingo::Input::LGuard::value_type>();
    }
    throw std::runtime_error("left guard expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Clingo::Input::RGuard::value_type>() const -> Clingo::Input::RGuard::value_type {
    if (type_ == clingo_ast_type_right_guard) {
        return cast<Clingo::Input::RGuard::value_type>();
    }
    throw std::runtime_error("right guard expected");
}

template <> [[nodiscard]] auto clingo_ast::convert<Clingo::Input::TheoryRGuard>() const -> Clingo::Input::TheoryRGuard {
    if (type_ == clingo_ast_type_theory_right_guard) {
        return cast<Clingo::Input::TheoryRGuard>();
    }
    throw std::runtime_error("theory right guard expected");
}

template <>
[[nodiscard]] auto
clingo_ast::convert<Clingo::Input::SetAggregateElement>() const -> Clingo::Input::SetAggregateElement {
    if (type_ == clingo_ast_type_set_aggregate_element) {
        return cast<Clingo::Input::SetAggregateElement>();
    }
    throw std::runtime_error("set aggregate element expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Clingo::Input::TheoryElement>() const -> Clingo::Input::TheoryElement {
    if (type_ == clingo_ast_type_theory_atom_element) {
        return cast<Clingo::Input::TheoryElement>();
    }
    throw std::runtime_error("theory atom element expected");
}

template <>
[[nodiscard]] auto
clingo_ast::convert<Clingo::Input::BdLitAggregateElement>() const -> Clingo::Input::BdLitAggregateElement {
    if (type_ == clingo_ast_type_body_aggregate_element) {
        return cast<Clingo::Input::BdLitAggregateElement>();
    }
    throw std::runtime_error("body aggregate element expected");
}

template <>
[[nodiscard]] auto
clingo_ast::convert<Clingo::Input::HdLitAggregateElement>() const -> Clingo::Input::HdLitAggregateElement {
    if (type_ == clingo_ast_type_head_aggregate_element) {
        return cast<Clingo::Input::HdLitAggregateElement>();
    }
    throw std::runtime_error("body aggregate element expected");
}

template <>
[[nodiscard]] auto
clingo_ast::convert<Clingo::Input::HdLitDisjunctionElement>() const -> Clingo::Input::HdLitDisjunctionElement {
    if (type_ == clingo_ast_type_head_conditional_literal) {
        return cast<Clingo::Input::CondLit>();
    }
    return cast<Clingo::Input::Lit>();
}

template <> [[nodiscard]] auto clingo_ast::convert<Clingo::Input::HdLit>() const -> Clingo::Input::HdLit {
    switch (type_) {
        case clingo_ast_type_head_simple_literal: {
            return cast<Clingo::Input::HdLitSimple>();
        }
        case clingo_ast_type_head_disjunction: {
            return cast<Clingo::Input::HdLitDisjunction>();
        }
        case clingo_ast_type_head_theory_atom: {
            return cast<Clingo::Input::HdLitTheoryAtom>();
        }
        case clingo_ast_type_head_set_aggregate: {
            return cast<Clingo::Input::HdLitSetAggregate>();
        }
        case clingo_ast_type_head_aggregate: {
            return cast<Clingo::Input::HdLitAggregate>();
        }
        default: {
            throw std::invalid_argument("head literal expected");
        }
    }
}

template <> [[nodiscard]] auto clingo_ast::convert<Clingo::Input::BdLit>() const -> Clingo::Input::BdLit {
    switch (type_) {
        case clingo_ast_type_body_simple_literal: {
            return cast<Clingo::Input::BdLitSimple>();
        }
        case clingo_ast_type_body_conditional_literal: {
            return cast<Clingo::Input::BdLitConjunction>();
        }
        case clingo_ast_type_body_theory_atom: {
            return cast<Clingo::Input::BdLitTheoryAtom>();
        }
        case clingo_ast_type_body_set_aggregate: {
            return cast<Clingo::Input::BdLitSetAggregate>();
        }
        case clingo_ast_type_body_aggregate: {
            return cast<Clingo::Input::BdLitAggregate>();
        }
        default: {
            throw std::invalid_argument("body literal expected");
        }
    }
}

template <>
[[nodiscard]] auto clingo_ast::convert<Clingo::Input::TheoryOpDefinition>() const -> Clingo::Input::TheoryOpDefinition {
    if (type_ == clingo_ast_type_theory_operator_definition) {
        return cast<Clingo::Input::TheoryOpDefinition>();
    }
    throw std::runtime_error("theory operator definition expected");
}

template <>
[[nodiscard]] auto
clingo_ast::convert<Clingo::Input::TheoryRGuardDefinition>() const -> Clingo::Input::TheoryRGuardDefinition {
    if (type_ == clingo_ast_type_theory_guard_definition) {
        return cast<Clingo::Input::TheoryRGuardDefinition>();
    }
    throw std::runtime_error("theory guard definition expected");
}

template <>
[[nodiscard]] auto
clingo_ast::convert<Clingo::Input::TheoryTermDefinition>() const -> Clingo::Input::TheoryTermDefinition {
    if (type_ == clingo_ast_type_theory_term_definition) {
        return cast<Clingo::Input::TheoryTermDefinition>();
    }
    throw std::runtime_error("theory term definition expected");
}

template <>
[[nodiscard]] auto
clingo_ast::convert<Clingo::Input::TheoryAtomDefinition>() const -> Clingo::Input::TheoryAtomDefinition {
    if (type_ == clingo_ast_type_theory_atom_definition) {
        return cast<Clingo::Input::TheoryAtomDefinition>();
    }
    throw std::runtime_error("theory atom definition expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Clingo::Input::OptimizeTuple>() const -> Clingo::Input::OptimizeTuple {
    if (type_ == clingo_ast_type_optimize_tuple) {
        return cast<Clingo::Input::OptimizeTuple>();
    }
    throw std::runtime_error("optimize tuple expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Clingo::Input::OptimizeElement>() const -> Clingo::Input::OptimizeElement {
    if (type_ == clingo_ast_type_optimize_element) {
        return cast<Clingo::Input::OptimizeElement>();
    }
    throw std::runtime_error("optimize tuple expected");
}

template <> [[nodiscard]] auto clingo_ast::convert<Clingo::Input::Edge>() const -> Clingo::Input::Edge {
    if (type_ == clingo_ast_type_edge) {
        return cast<Clingo::Input::Edge>();
    }
    throw std::runtime_error("edge expected");
}

template <> [[nodiscard]] auto clingo_ast::convert<Clingo::Input::StmTheory>() const -> Clingo::Input::StmTheory {
    if (type_ != clingo_ast_type_statement_theory) {
        throw std::runtime_error("theory expected");
    }
    return cast<Clingo::Input::StmTheory>();
}

template <> [[nodiscard]] auto clingo_ast::convert<Clingo::Input::Stm>() const -> Clingo::Input::Stm {
    switch (type_) {
        case clingo_ast_type_statement_rule: {
            return cast<Clingo::Input::StmRule>();
        }
        case clingo_ast_type_statement_theory: {
            return cast<Clingo::Input::StmTheory>();
        }
        case clingo_ast_type_statement_optimize: {
            return cast<Clingo::Input::StmOptimize>();
        }
        case clingo_ast_type_statement_weak_constraint: {
            return cast<Clingo::Input::StmWeakConstraint>();
        }
        case clingo_ast_type_statement_show: {
            return cast<Clingo::Input::StmShow>();
        }
        case clingo_ast_type_statement_show_nothing: {
            return cast<Clingo::Input::StmShowNothing>();
        }
        case clingo_ast_type_statement_show_signature: {
            return cast<Clingo::Input::StmShowSig>();
        }
        case clingo_ast_type_statement_defined: {
            return cast<Clingo::Input::StmDefined>();
        }
        case clingo_ast_type_statement_external: {
            return cast<Clingo::Input::StmExternal>();
        }
        case clingo_ast_type_statement_edge: {
            return cast<Clingo::Input::StmEdge>();
        }
        case clingo_ast_type_statement_heuristic: {
            return cast<Clingo::Input::StmHeuristic>();
        }
        case clingo_ast_type_statement_script: {
            return cast<Clingo::Input::StmScript>();
        }
        case clingo_ast_type_statement_program: {
            return cast<Clingo::Input::StmProgram>();
        }
        case clingo_ast_type_statement_include: {
            return cast<Clingo::Input::StmInclude>();
        }
        case clingo_ast_type_statement_const: {
            return cast<Clingo::Input::StmConst>();
        }
        case clingo_ast_type_statement_comment: {
            return cast<Clingo::Input::StmComment>();
        }
        default: {
            throw std::runtime_error("statement expected");
        }
    }
}

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
extern "C" auto clingo_ast_construct(clingo_lib_t *lib, clingo_ast_type_t type, clingo_ast_t **ast, ...) -> bool {
    using namespace Clingo::Input;
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
                *ast = construct_ast<Clingo::Input::Projection>(type, convert_loc(lib, loc));
                break;
            }
            case clingo_ast_type_term_variable: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto anonymous = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<Clingo::Input::TermVariable>(type, convert_loc(lib, loc),
                                                                  lib->store->string_ref(name), anonymous != 0);
                break;
            }
            case clingo_ast_type_term_symbolic: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sym = va_arg(args, clingo_symbol_t);
                va_end(args);
                *ast = construct_ast<Clingo::Input::TermSymbol>(type, convert_loc(lib, loc),
                                                                Clingo::Symbol::from_rep(sym));
                break;
            }
            case clingo_ast_type_term_tuple: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **pool = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<Clingo::Input::TermTuple>(
                    type, convert_loc(lib, loc), convert_ast_vec<Clingo::Input::TupleElement>(pool, size));
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
                *ast = construct_ast<Clingo::Input::TermFunction>(
                    type, convert_loc(lib, loc), lib->store->string_ref(name),
                    convert_ast_vec<Clingo::Input::ArgumentTuple>(pool, size), sign != 0);
                break;
            }
            case clingo_ast_type_term_absolute: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **pool = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<Clingo::Input::TermAbs>(type, convert_loc(lib, loc),
                                                             convert_ast_vec<Clingo::Input::Term>(pool, size));
                break;
            }
            case clingo_ast_type_term_unary_operation: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto op = va_arg(args, int);
                auto const *rhs = va_arg(args, clingo_ast_t *);
                va_end(args);
                *ast = construct_ast<Clingo::Input::TermUnary>(
                    type, convert_loc(lib, loc), static_cast<Clingo::Input::UnaryOperator>(op),
                    Clingo::Util::make_immutable<Clingo::Input::Term>(rhs->convert<Clingo::Input::Term>()));
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
                *ast = construct_ast<Clingo::Input::TermBinary>(
                    type, convert_loc(lib, loc),
                    Clingo::Util::make_immutable<Clingo::Input::Term>(lhs->convert<Clingo::Input::Term>()),
                    static_cast<Clingo::Input::BinaryOperator>(op),
                    Clingo::Util::make_immutable<Clingo::Input::Term>(rhs->convert<Clingo::Input::Term>()));
                break;
            }
            case clingo_ast_type_argument_tuple: {
                std::va_list args;
                va_start(args, ast);
                auto const **tuple = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<Clingo::Input::ArgumentTuple>(
                    type, convert_ast_vec<Clingo::Input::Argument>(tuple, size));
                break;
            }
            case clingo_ast_type_left_guard: {
                std::va_list args;
                va_start(args, ast);
                auto const *left = va_arg(args, clingo_ast const *);
                auto right = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<Clingo::Input::LGuard::value_type>(type, left->convert<Clingo::Input::Term>(),
                                                                        static_cast<Clingo::Relation>(right));
                break;
            }
            case clingo_ast_type_right_guard: {
                std::va_list args;
                va_start(args, ast);
                auto left = va_arg(args, int);
                auto const *right = va_arg(args, clingo_ast const *);
                va_end(args);
                *ast = construct_ast<Clingo::Input::RGuard::value_type>(type, static_cast<Clingo::Relation>(left),
                                                                        right->convert<Clingo::Input::Term>());
                break;
            }
            case clingo_ast_type_literal_boolean: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sign = va_arg(args, int);
                auto value = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<Clingo::Input::LitBool>(type, convert_loc(lib, loc),
                                                             static_cast<Clingo::Sign>(sign), value != 0);
                break;
            }
            case clingo_ast_type_literal_symbolic: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sign = va_arg(args, int);
                auto const *atom = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<Clingo::Input::LitSymbolic>(
                    type, convert_loc(lib, loc), static_cast<Clingo::Sign>(sign), atom->convert<Clingo::Input::Term>());
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
                *ast = construct_ast<Clingo::Input::LitComparison>(
                    type, convert_loc(lib, loc), static_cast<Clingo::Sign>(sign), left->convert<Clingo::Input::Term>(),
                    convert_ast_vec<Clingo::Input::Guard>(right, size));
                break;
            }
            case clingo_ast_type_unparsed_element: {
                std::va_list args;
                va_start(args, ast);
                auto const **ops = va_arg(args, char const **);
                auto size = va_arg(args, size_t);
                auto const *term = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<Clingo::Input::UnparsedElement>(type, convert_string_array(lib, ops, size),
                                                                     term->convert<Clingo::Input::TheoryTerm>());
                break;
            }
            case clingo_ast_type_theory_term_variable: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto anonymous = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<Clingo::Input::TheoryTermVariable>(type, convert_loc(lib, loc),
                                                                        lib->store->string_ref(name), anonymous != 0);
                break;
            }
            case clingo_ast_type_theory_term_symbolic: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sym = va_arg(args, clingo_symbol_t);
                va_end(args);
                *ast = construct_ast<Clingo::Input::TheoryTermSymbol>(type, convert_loc(lib, loc),
                                                                      Clingo::Symbol::from_rep(sym));
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
                *ast = construct_ast<Clingo::Input::TheoryTermTuple>(
                    type, convert_loc(lib, loc), static_cast<Clingo::TheoryTermTupleType>(tuple_type),
                    convert_ast_vec<Clingo::Input::TheoryTerm>(arguments, size));
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
                *ast = construct_ast<Clingo::Input::TheoryTermFunction>(
                    type, convert_loc(lib, loc), lib->store->string_ref(name),
                    convert_ast_vec<Clingo::Input::TheoryTerm>(arguments, size));
                break;
            }
            case clingo_ast_type_theory_term_unparsed: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **elems = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<Clingo::Input::TheoryTermUnparsed>(
                    type, convert_loc(lib, loc), convert_ast_vec<Clingo::Input::UnparsedElement>(elems, size));
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
                *ast = construct_ast<Clingo::Input::SetAggregateElement>(
                    type, convert_loc(lib, loc), lit->convert<Lit>(), convert_ast_vec<Clingo::Input::Lit>(cond, size));
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
                *ast = construct_ast<Clingo::Input::TheoryElement>(
                    type, convert_loc(lib, loc), convert_ast_vec<Clingo::Input::TheoryTerm>(tuple, tuple_size),
                    convert_ast_vec<Clingo::Input::Lit>(cond, cond_size));
                break;
            }
            case clingo_ast_type_theory_right_guard: {
                std::va_list args;
                va_start(args, ast);
                auto const *op = va_arg(args, char const *);
                auto const *term = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<Clingo::Input::TheoryRGuard>(type, lib->store->string_ref(op),
                                                                  term->convert<Clingo::Input::TheoryTerm>());
                break;
            }
            case clingo_ast_type_body_simple_literal: {
                std::va_list args;
                va_start(args, ast);
                auto const *lit = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<Clingo::Input::BdLitSimple>(type, lit->convert<Clingo::Input::Lit>());
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
                *ast = construct_ast<Clingo::Input::BdLitAggregateElement>(
                    type, convert_loc(lib, loc), convert_ast_vec<Clingo::Input::Term>(tuple, tuple_size),
                    convert_ast_vec<Clingo::Input::Lit>(cond, cond_size));
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
                *ast = construct_ast<Clingo::Input::BdLitAggregate>(
                    type, convert_loc(lib, loc), static_cast<Clingo::Sign>(sign),
                    convert_ast_opt<Clingo::Input::LGuard::value_type>(lhs),
                    static_cast<Clingo::AggregateFunction>(fun),
                    convert_ast_vec<Clingo::Input::BdLitAggregateElement>(elems, elems_size),
                    convert_ast_opt<Clingo::Input::RGuard::value_type>(rhs));
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
                *ast = construct_ast<Clingo::Input::BdLitSetAggregate>(
                    type, convert_loc(lib, loc), static_cast<Clingo::Sign>(sign),
                    convert_ast_opt<Clingo::Input::LGuard::value_type>(lhs),
                    convert_ast_vec<Clingo::Input::SetAggregateElement>(elems, elems_size),
                    convert_ast_opt<Clingo::Input::RGuard::value_type>(rhs));
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
                *ast = construct_ast<Clingo::Input::BdLitTheoryAtom>(
                    type, convert_loc(lib, loc), static_cast<Clingo::Sign>(sign), term->convert<Clingo::Input::Term>(),
                    convert_ast_vec<Clingo::Input::TheoryElement>(elems, elems_size),
                    convert_ast_opt<Clingo::Input::TheoryRGuard>(rhs));
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
                *ast = construct_ast<Clingo::Input::BdLitConjunction>(
                    type, Clingo::Input::CondLit{convert_loc(lib, loc), lit->convert<Clingo::Input::Lit>(),
                                                 convert_ast_vec<Clingo::Input::Lit>(cond, cond_size)});
                break;
            }
            case clingo_ast_type_head_simple_literal: {
                std::va_list args;
                va_start(args, ast);
                auto const *lit = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<Clingo::Input::HdLitSimple>(type, lit->convert<Clingo::Input::Lit>());
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
                *ast = construct_ast<Clingo::Input::HdLitAggregateElement>(
                    type, convert_loc(lib, loc), convert_ast_vec<Clingo::Input::Term>(tuple, tuple_size),
                    lit->convert<Lit>(), convert_ast_vec<Clingo::Input::Lit>(cond, cond_size));
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
                *ast = construct_ast<Clingo::Input::HdLitAggregate>(
                    type, convert_loc(lib, loc), convert_ast_opt<Clingo::Input::LGuard::value_type>(lhs),
                    static_cast<Clingo::AggregateFunction>(fun),
                    convert_ast_vec<Clingo::Input::HdLitAggregateElement>(elems, elems_size),
                    convert_ast_opt<Clingo::Input::RGuard::value_type>(rhs));
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
                *ast = construct_ast<Clingo::Input::HdLitSetAggregate>(
                    type, convert_loc(lib, loc), convert_ast_opt<Clingo::Input::LGuard::value_type>(lhs),
                    convert_ast_vec<Clingo::Input::SetAggregateElement>(elems, elems_size),
                    convert_ast_opt<Clingo::Input::RGuard::value_type>(rhs));
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
                *ast = construct_ast<Clingo::Input::HdLitTheoryAtom>(
                    type, convert_loc(lib, loc), term->convert<Clingo::Input::Term>(),
                    convert_ast_vec<Clingo::Input::TheoryElement>(elems, elems_size),
                    convert_ast_opt<Clingo::Input::TheoryRGuard>(rhs));
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
                *ast = construct_ast<Clingo::Input::CondLit>(
                    type, Clingo::Input::CondLit{convert_loc(lib, loc), lit->convert<Clingo::Input::Lit>(),
                                                 convert_ast_vec<Clingo::Input::Lit>(cond, cond_size)});
                break;
            }
            case clingo_ast_type_head_disjunction: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **elems = va_arg(args, clingo_ast_t const **);
                auto elems_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<Clingo::Input::HdLitDisjunction>(
                    type, convert_loc(lib, loc),
                    convert_ast_vec<Clingo::Input::HdLitDisjunctionElement>(elems, elems_size));
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
                *ast = construct_ast<Clingo::Input::StmRule>(type, convert_loc(lib, loc), head->convert<HdLit>(),
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
                *ast = construct_ast<Clingo::Input::TheoryOpDefinition>(type, convert_loc(lib, loc),
                                                                        lib->store->string_ref(name), priority,
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
                *ast = construct_ast<Clingo::Input::TheoryTermDefinition>(
                    type, convert_loc(lib, loc), lib->store->string_ref(name),
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
                *ast = construct_ast<Clingo::Input::TheoryRGuardDefinition>(
                    type, convert_string_array(lib, ops, ops_size), lib->store->string_ref(term));
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
                *ast = construct_ast<Clingo::Input::TheoryAtomDefinition>(
                    type, convert_loc(lib, loc), lib->store->string_ref(name), arity, lib->store->string_ref(term),
                    convert_ast_opt<TheoryRGuardDefinition>(guard), static_cast<TheoryAtomType>(atom_type));
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
                    construct_ast<Clingo::Input::StmTheory>(type, convert_loc(lib, loc), lib->store->string_ref(name),
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
                *ast = construct_ast<Clingo::Input::OptimizeTuple>(type, weight->convert<Term>(),
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
                *ast = construct_ast<Clingo::Input::OptimizeElement>(type, tuple->convert<OptimizeTuple>(),
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
                *ast = construct_ast<Clingo::Input::StmOptimize>(type, convert_loc(lib, loc),
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
                *ast = construct_ast<Clingo::Input::StmWeakConstraint>(type, convert_loc(lib, loc),
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
                *ast = construct_ast<Clingo::Input::Edge>(type, u->convert<Term>(), v->convert<Term>());
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
                *ast = construct_ast<Clingo::Input::StmShow>(type, convert_loc(lib, loc), term->convert<Term>(),
                                                             convert_ast_vec<BdLit>(body, body_size));
                break;
            }
            case clingo_ast_type_statement_show_nothing: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                va_end(args);
                *ast = construct_ast<Clingo::Input::StmShowNothing>(type, convert_loc(lib, loc));
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
                *ast = construct_ast<Clingo::Input::StmShowSig>(type, convert_loc(lib, loc), sign != 0,
                                                                lib->store->string_ref(name), arity);
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
                *ast = construct_ast<Clingo::Input::StmProject>(type, convert_loc(lib, loc), atom->convert<Term>(),
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
                *ast = construct_ast<Clingo::Input::StmProjectSig>(type, convert_loc(lib, loc), sign != 0,
                                                                   lib->store->string_ref(name), arity);
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
                *ast = construct_ast<Clingo::Input::StmDefined>(type, convert_loc(lib, loc), sign != 0,
                                                                lib->store->string_ref(name), arity);
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
                *ast = construct_ast<Clingo::Input::StmExternal>(type, convert_loc(lib, loc), atom->convert<Term>(),
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
                *ast = construct_ast<Clingo::Input::StmEdge>(type, convert_loc(lib, loc),
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
                *ast = construct_ast<Clingo::Input::StmHeuristic>(
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
                *ast = construct_ast<Clingo::Input::StmInclude>(
                    type, convert_loc(lib, loc), static_cast<IncludeType>(include_type), lib->store->string_ref(value));
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
                *ast =
                    construct_ast<Clingo::Input::StmProgram>(type, convert_loc(lib, loc), lib->store->string_ref(name),
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
                *ast = construct_ast<Clingo::Input::StmScript>(
                    type, convert_loc(lib, loc), lib->store->string_ref(script_type), lib->store->string_ref(value));
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
                *ast = construct_ast<Clingo::Input::StmConst>(type, convert_loc(lib, loc),
                                                              static_cast<ConstType>(const_type),
                                                              lib->store->string_ref(name), term->convert<Term>());
                break;
            }
            case clingo_ast_type_statement_comment: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *value = va_arg(args, char const *);
                auto comment_type = va_arg(args, int);
                va_end(args);
                *ast = construct_ast<Clingo::Input::StmComment>(
                    type, convert_loc(lib, loc), static_cast<CommentType>(comment_type), lib->store->string_ref(value));
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

extern "C" auto clingo_ast_attribute_get_number(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                int *value) -> bool {
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

extern "C" auto clingo_ast_attribute_get_string(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                char const **value) -> bool {
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
                std::transform(vec->begin(), vec->end(), value, [](auto const &str) { return str.c_str(); });
            }
            return true;
        }
        return false;
    }
    CLINGO_CATCH(nullptr);
}

extern "C" auto clingo_ast_attribute_get_ast(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                             clingo_ast_t **value) -> bool {
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
        auto p = Clingo::Input::Parser{lib->log, *lib->store};
        p.init(string, *lib->store->string("<string>"));
        switch (type) {
            case clingo_ast_parse_type_term: {
                auto term = p.parse_term();
                if (!term) {
                    lib->log.reset();
                    throw std::runtime_error("parsing term failed");
                }
                auto owner = Clingo::Util::make_immutable<std::any>(std::move(term).value());
                auto const *ptr = std::any_cast<Clingo::Input::Term>(&owner.get());
                *ast = make_ast(owner, *ptr).release();
                break;
            }
            case clingo_ast_parse_type_theory_term: {
                auto term = p.parse_theory_term();
                if (!term) {
                    lib->log.reset();
                    throw std::runtime_error("parsing theory term failed");
                }
                auto owner = Clingo::Util::make_immutable<std::any>(std::move(term).value());
                auto const *ptr = std::any_cast<Clingo::Input::TheoryTerm>(&owner.get());
                *ast = make_ast(owner, *ptr).release();
                break;
            }
            case clingo_ast_parse_type_literal: {
                auto lit = p.parse_literal();
                if (!lit) {
                    lib->log.reset();
                    throw std::runtime_error("parsing literal failed");
                }
                auto owner = Clingo::Util::make_immutable<std::any>(std::move(lit).value());
                auto const *ptr = std::any_cast<Clingo::Input::Lit>(&owner.get());
                *ast = make_ast(owner, *ptr).release();
                break;
            }
            case clingo_ast_parse_type_head_literal: {
                auto lit = p.parse_head_literal();
                if (!lit) {
                    lib->log.reset();
                    throw std::runtime_error("parsing head literal failed");
                }
                auto owner = Clingo::Util::make_immutable<std::any>(std::move(lit).value());
                auto const *ptr = std::any_cast<Clingo::Input::HdLit>(&owner.get());
                *ast = make_ast(owner, *ptr).release();
                break;
            }
            case clingo_ast_parse_type_body_literal: {
                auto lit = p.parse_body_literal();
                if (!lit) {
                    lib->log.reset();
                    throw std::runtime_error("parsing body literal failed");
                }
                auto owner = Clingo::Util::make_immutable<std::any>(std::move(lit).value());
                auto const *ptr = std::any_cast<Clingo::Input::BdLit>(&owner.get());
                *ast = make_ast(owner, *ptr).release();
                break;
            }
            case clingo_ast_parse_type_statement: {
                auto lit = p.parse_statement();
                if (!lit) {
                    lib->log.reset();
                    throw std::runtime_error("parsing statement failed");
                }
                auto owner = Clingo::Util::make_immutable<std::any>(std::move(lit).value());
                auto const *ptr = std::any_cast<Clingo::Input::Stm>(&owner.get());
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
    clingo_ast_scanner(clingo_lib_t *lib) : lib_{lib}, parser_{lib->log, *lib->store} {}
    [[nodiscard]] auto next() -> std::unique_ptr<clingo_ast_t> {
        while (true) {
            if (active_) {
                auto [stm, res] = parser_.scan();
                parse_error_ = parse_error_ || !res;
                if (stm) {
                    auto owner = Clingo::Util::make_immutable<std::any>(*std::move(stm));
                    auto const *ptr = std::any_cast<Clingo::Input::Stm>(&owner.get());
                    return make_ast(owner, *ptr);
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
            throw Clingo::parse_error();
        }
        return nullptr;
    }
    [[nodiscard]] auto lib() const -> clingo_lib_t * { return lib_; }
    auto scan_string(std::string str) { strings_.emplace_front(std::move(str), false); }
    auto scan_file(char const *path) { strings_.emplace_front(path, true); }
    [[nodiscard]] auto has_error() const -> bool { return parse_error_; }

  private:
    clingo_lib_t *lib_;
    Clingo::Input::Parser parser_;
    std::forward_list<std::pair<std::string, bool>> strings_;
    std::ifstream cur_;
    bool active_ = false;
    bool parse_error_ = false;
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
    Clingo::Input::TheoryAtomParser parser;
    Clingo::Input::ParamMap param_map;
    Clingo::Input::ConstMap const_map;
    Clingo::Input::RewriteOptions options;
    Clingo::Input::RewriteContext ctx = {lib->log, *lib->store, options, parser, param_map, const_map};
    Clingo::Util::ordered_map<Clingo::String, Clingo::String> param_unmap;
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
        if (auto prm = lib->store->string_ref(param); context->param_map.emplace(prm).second) {
            auto var = lib->store->string_ref("$" + std::to_string(context->param_unmap.size()));
            context->param_unmap.emplace(var, prm);
        }
    }
    CLINGO_CATCH(lib);
}

extern "C" void clingo_ast_rewrite_context_clear_params(clingo_ast_rewrite_context_t *context) {
    context->param_map.clear();
    context->param_unmap.clear();
}

extern "C" auto clingo_ast_rewrite_context_add_theory(clingo_ast_rewrite_context_t *context,
                                                      clingo_ast_t const *theory) -> bool {
    auto *lib = context->lib;
    CLINGO_TRY {
        auto stm = theory->convert<Clingo::Input::StmTheory>();
        context->parser.add_theory(lib->log, stm);
        if (context->parser.has_error()) {
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

extern "C" auto
clingo_ast_rewrite_context_get_project_mode(clingo_ast_rewrite_context_t *context) -> clingo_projection_mode_t {
    return static_cast<clingo_projection_mode_t>(context->options.project_mode);
}

extern "C" void clingo_ast_rewrite_context_set_project_mode(clingo_ast_rewrite_context_t *context,
                                                            clingo_projection_mode_t value) {
    context->options.project_mode = static_cast<Clingo::Input::ProjectionMode>(value);
}

extern "C" auto clingo_ast_rewrite_context_get_lib(clingo_ast_rewrite_context_t *context) -> clingo_lib_t * {
    return context->lib;
}

extern "C" auto clingo_ast_rewrite(clingo_ast_rewrite_context_t *context, clingo_ast_t *statement,
                                   clingo_ast_t ***result, size_t *result_size) -> bool {
    using namespace Clingo::Input;
    auto *lib = context->lib;
    CLINGO_TRY {
        *result = nullptr;
        *result_size = 0;
        auto stms = StmVec{};
        auto stm = statement->convert<Stm>();
        rewrite(context->ctx, stm, stms);
        ASTVec res{stms.size()};
        int i = 0;
        for (auto &stm : stms) {
            if (auto res_stm = unmap_params(*lib->store, context->param_unmap, stm); res_stm) {
                stm = *std::move(res_stm);
            }
            auto owner = Clingo::Util::make_immutable<std::any>(std::move(stm));
            auto const *ptr = std::any_cast<Clingo::Input::Stm>(&owner.get());
            res[i] = make_ast(owner, *ptr).release();
            ++i;
        }
        std::tie(*result, *result_size) = res.release();
    }
    CLINGO_CATCH(lib);
}

// NOLINTEND(cppcoreguidelines-macro-usage)
