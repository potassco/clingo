#include <any>
#include <cstdarg>
#include <cstring>

#include "lib.hh"
#include "streams.hh"

#include <gringo/util/algorithm.hh>
#include <gringo/util/ordered_map.hh>
#include <gringo/util/ordered_set.hh>

#include <gringo/input/algo/parse.hh>
#include <gringo/input/algo/print.hh>

namespace {

class ASTVec;

using Owner = Gringo::Util::shared_ptr<std::any>;

template <class T>
auto make_ast(Owner const &owner, Gringo::Util::shared_ptr<T> const &ptr) -> std::unique_ptr<clingo_ast_t>;
template <class T> auto make_ast(Owner const &owner, std::optional<T> const &opt) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::LGuard::value_type const &guard) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::RGuard::value_type const &guard) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TheoryRGuard::value_type const &guard)
    -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::Term const &term) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TheoryTerm const &term) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TupleVec const &tuple) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TermTuple::Element const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::Literal const &lit) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TheoryTermUnparsed::Element const &elem)
    -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TheoryElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::SetAggregateElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::BodyAggregate::Element const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::HeadAggregate::Element const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::Disjunction::Element const &elem) -> std::unique_ptr<clingo_ast_t>;

template <class T> auto make_ast_vec(Owner const &owner, std::vector<T> const &vec) -> ASTVec;

template <class T> auto convert_ast_vec(clingo_ast const **ast, size_t size) -> std::vector<T>;

auto convert_loc(clingo_lib_t *lib, clingo_location_t const *loc) -> Gringo::Input::Location {
    return {{lib->store->string(loc->begin_file), loc->begin_line, loc->begin_column},
            {lib->store->string(loc->end_file), loc->end_line, loc->end_column}};
}

auto convert_string_array(clingo_lib_t *lib, char const **array, size_t size) -> Gringo::StringVec {
    auto ret = Gringo::StringVec{};
    ret.reserve(size);
    std::transform(array, array + size, std::back_inserter(ret), [lib](auto str) { return lib->store->string(str); });
    return ret;
}

// Note: it is actualy used
[[maybe_unused]] auto convert_loc(Gringo::Input::Location const &loc) -> clingo_location_t {
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
        -> std::optional<tcb::span<Gringo::String const>>;
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
        for (auto it = data_, ie = data_ + size_; it != ie; ++it) {
            delete *it;
        }
        delete[] data_;
    }
    [[nodiscard]] auto empty() const -> bool { return size_ == 0; }
    [[nodiscard]] auto size() const -> size_t { return size_; }
    [[nodiscard]] auto begin() const -> clingo_ast_t ** { return data_; }
    [[nodiscard]] auto end() const -> clingo_ast_t ** { return data_ + size_; }
    auto operator[](size_t i) const -> clingo_ast_t *& {
        return data_[i]; // NOLINT
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
            ret[i] = data[i]->copy().release();
        }
        return ret;
    }

  private:
    ASTVec(clingo_ast_t **data, size_t size) : data_{data}, size_{size} {}

    clingo_ast_t **data_ = nullptr;
    size_t size_ = 0;
};

template <class T> auto make_ast_vec(Owner const &owner, std::vector<T> const &vec) -> ASTVec {
    ASTVec res{vec.size()};
    size_t i = 0;
    for (auto const &elem : vec) {
        res[i] = make_ast(owner, elem).release();
        ++i;
    }
    return res;
}

template <class T> auto make_ast_vec(Owner const &owner, Gringo::Util::immutable_vector<T> const &vec) -> ASTVec {
    ASTVec res{vec.size()};
    size_t i = 0;
    for (auto const &elem : vec) {
        res[i] = make_ast(owner, elem).release();
        ++i;
    }
    return res;
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
    for (auto it = ast, ie = ast + size; it != ie; ++it) {
        res.emplace_back((*it)->convert<T>());
    }
    return res;
}

template <class T>
auto make_ast(Owner const &owner, Gringo::Util::shared_ptr<T> const &ptr) -> std::unique_ptr<clingo_ast_t> {
    return make_ast(owner, *ptr);
}

template <class T> auto make_ast(Owner const &owner, std::optional<T> const &opt) -> std::unique_ptr<clingo_ast_t> {
    if (opt.has_value()) {
        return make_ast(owner, opt.value());
    }
    return nullptr;
}

auto make_ast(Owner const &owner, Gringo::Input::LGuard::value_type const &guard) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_left_guard, static_cast<void const *>(&guard));
}

auto make_ast(Owner const &owner, Gringo::Input::RGuard::value_type const &guard) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_right_guard, static_cast<void const *>(&guard));
}

auto make_ast(Owner const &owner, Gringo::Input::TheoryRGuard::value_type const &guard)
    -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_right_guard, static_cast<void const *>(&guard));
}

auto make_ast(Owner const &owner, Gringo::Input::Term const &term) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner](auto const &x) {
            GRINGO_MATCH(x, Gringo::Input::TermVariable) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_variable, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::TermSymbol) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_symbolic, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::TermTuple) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_tuple, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::TermFunction) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_function, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::TermAbs) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_absolute, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::TermUnary) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_unary_operation, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::TermBinary) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_term_binary_operation, &x);
            }
        },
        term);
}

auto make_ast(Owner const &owner, Gringo::Input::TheoryTerm const &term) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner](auto const &x) {
            GRINGO_MATCH(x, Gringo::Input::TheoryTermVariable) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_term_variable, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::TheoryTermSymbol) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_term_symbolic, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::TheoryTermTuple) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_term_tuple, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::TheoryTermFunction) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_term_function, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::TheoryTermUnparsed) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_theory_term_unparsed, &x);
            }
        },
        term);
}

auto make_ast(Owner const &owner, Gringo::Input::TheoryTermUnparsed::Element const &elem)
    -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_unparsed_element, &elem);
}

auto make_ast(Owner const &owner, Gringo::Input::TupleVec const &tuple) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_argument_tuple, static_cast<void const *>(&tuple));
}

auto make_ast(Owner const &owner, Gringo::Input::TermTuple::Element const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner](auto const &x) {
            GRINGO_MATCH(x, Gringo::Input::Term) { return make_ast(owner, x); }
            GRINGO_MATCH(x, Gringo::Input::TupleVec) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_argument_tuple, &x);
            }
        },
        elem);
}

auto make_ast(Owner const &owner, Gringo::Input::Literal const &lit) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner](auto const &x) {
            GRINGO_MATCH(x, Gringo::Input::LiteralBoolean) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_literal_boolean, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::LiteralSymbolic) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_literal_symbolic, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::LiteralRelation) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_literal_comparison, &x);
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

auto make_ast(Owner const &owner, Gringo::Input::BodyAggregate::Element const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_body_aggregate_element, &elem);
}

auto make_ast(Owner const &owner, Gringo::Input::HeadAggregate::Element const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_head_aggregate_element, &elem);
}

auto make_ast(Owner const &owner, Gringo::Input::Disjunction::Element const &elem) -> std::unique_ptr<clingo_ast_t> {
    using namespace Gringo::Input;
    return std::visit(
        [&owner](auto const &x) {
            GRINGO_MATCH(x, ConditionalLiteral) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_head_conditional_literal, &x);
            }
            else {
                return make_ast(owner, x);
            }
        },
        elem);
}

template <class T, class... A> auto construct_ast(clingo_ast_type_t type, A &&...args) -> clingo_ast * {
    auto owner = Gringo::Util::construct_shared<std::any>(T{std::forward<A>(args)...});
    auto *ptr = std::any_cast<T>(owner.get());
    return new clingo_ast{std::move(owner), static_cast<clingo_ast_type_e>(type), ptr};
}

} // namespace

template <class V>
auto clingo_ast::visit(V &&visit) const -> std::invoke_result_t<V, Gringo::Input::Projection const &> {
    using namespace Gringo::Input;
    switch (type_) {
        case clingo_ast_type_projection: {
            return std::invoke(std::move(visit), cast<Projection>());
        }
        case clingo_ast_type_term_variable: {
            return std::invoke(std::move(visit), cast<TermVariable>());
        }
        case clingo_ast_type_term_symbolic: {
            return std::invoke(std::move(visit), cast<TermSymbol>());
        }
        case clingo_ast_type_term_absolute: {
            return std::invoke(std::move(visit), cast<TermAbs>());
        }
        case clingo_ast_type_term_unary_operation: {
            return std::invoke(std::move(visit), cast<TermUnary>());
        }
        case clingo_ast_type_term_binary_operation: {
            return std::invoke(std::move(visit), cast<TermBinary>());
        }
        case clingo_ast_type_term_tuple: {
            return std::invoke(std::move(visit), cast<TermTuple>());
        }
        case clingo_ast_type_term_function: {
            return std::invoke(std::move(visit), cast<TermFunction>());
        }
        case clingo_ast_type_argument_tuple: {
            return std::invoke(std::move(visit), cast<TupleVec>());
        }
        case clingo_ast_type_left_guard: {
            return std::invoke(std::move(visit), cast<LGuard::value_type>());
        }
        case clingo_ast_type_right_guard: {
            return std::invoke(std::move(visit), cast<RGuard::value_type>());
        }
        case clingo_ast_type_unparsed_element: {
            return std::invoke(std::move(visit), cast<TheoryTermUnparsed::Element>());
        }
        case clingo_ast_type_theory_term_variable: {
            return std::invoke(std::move(visit), cast<TheoryTermVariable>());
        }
        case clingo_ast_type_theory_term_symbolic: {
            return std::invoke(std::move(visit), cast<TheoryTermSymbol>());
        }
        case clingo_ast_type_theory_term_tuple: {
            return std::invoke(std::move(visit), cast<TheoryTermTuple>());
        }
        case clingo_ast_type_theory_term_function: {
            return std::invoke(std::move(visit), cast<TheoryTermFunction>());
        }
        case clingo_ast_type_theory_term_unparsed: {
            return std::invoke(std::move(visit), cast<TheoryTermUnparsed>());
        }
        case clingo_ast_type_literal_boolean: {
            return std::invoke(std::move(visit), cast<LiteralBoolean>());
        }
        case clingo_ast_type_literal_comparison: {
            return std::invoke(std::move(visit), cast<LiteralRelation>());
        }
        case clingo_ast_type_literal_symbolic: {
            return std::invoke(std::move(visit), cast<LiteralSymbolic>());
        }
        case clingo_ast_type_set_aggregate_element: {
            return std::invoke(std::move(visit), cast<SetAggregateElement>());
        }
        case clingo_ast_type_theory_atom_element: {
            return std::invoke(std::move(visit), cast<TheoryElement>());
        }
        case clingo_ast_type_theory_right_guard: {
            return std::invoke(std::move(visit), cast<TheoryRGuard>());
        }
        case clingo_ast_type_body_simple_literal: {
            return std::invoke(std::move(visit), cast<SimpleBodyLiteral>());
        }
        case clingo_ast_type_body_aggregate_element: {
            return std::invoke(std::move(visit), cast<BodyAggregate::Element>());
        }
        case clingo_ast_type_body_aggregate: {
            return std::invoke(std::move(visit), cast<BodyAggregate>());
        }
        case clingo_ast_type_body_set_aggregate: {
            return std::invoke(std::move(visit), cast<BodySetAggregate>());
        }
        case clingo_ast_type_body_theory_atom: {
            return std::invoke(std::move(visit), cast<BodyTheoryAtom>());
        }
        case clingo_ast_type_body_conditional_literal: {
            return std::invoke(std::move(visit), cast<Conjunction>());
        }
        case clingo_ast_type_head_simple_literal: {
            return std::invoke(std::move(visit), cast<SimpleHeadLiteral>());
        }
        case clingo_ast_type_head_aggregate_element: {
            return std::invoke(std::move(visit), cast<HeadAggregate::Element>());
        }
        case clingo_ast_type_head_aggregate: {
            return std::invoke(std::move(visit), cast<HeadAggregate>());
        }
        case clingo_ast_type_head_set_aggregate: {
            return std::invoke(std::move(visit), cast<HeadSetAggregate>());
        }
        case clingo_ast_type_head_theory_atom: {
            return std::invoke(std::move(visit), cast<HeadTheoryAtom>());
        }
        case clingo_ast_type_head_conditional_literal: {
            return std::invoke(std::move(visit), cast<ConditionalLiteral>());
        }
        case clingo_ast_type_head_disjunction: {
            return std::invoke(std::move(visit), cast<Disjunction>());
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
    return visit([](auto &x) -> std::optional<clingo_location_t> {
        if constexpr (Detail::has_loc<std::decay_t<decltype(x)>>) {
            return convert_loc(x.loc);
        }
        GRINGO_MATCH(x, Conjunction) { return convert_loc(x.lit.loc); }
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
            ATTR(anonymous, is_anonymous))
        TYPE(theory_term_variable, TheoryTermVariable,
            ATTR(anonymous, is_anonymous))
        TYPE(term_function, TermFunction,
            ATTR(external, external))
        TYPE(term_unary_operation, TermUnary,
            ATTR(operator_type, op))
        TYPE(term_binary_operation, TermBinary,
            ATTR(operator_type, op))
        TYPE(theory_term_tuple, TheoryTermTuple,
            ATTR(tuple_type, type))
        TYPE(literal_boolean, LiteralBoolean,
            ATTR(sign, sign)
            ATTR(value, value))
        TYPE(literal_symbolic, LiteralSymbolic,
            ATTR(sign, sign))
        TYPE(literal_comparison, LiteralRelation,
            ATTR(sign, sign))
        TYPE(left_guard, LGuard::value_type,
            ATTR(relation, second))
        TYPE(right_guard, RGuard::value_type,
            ATTR(relation, first))
        TYPE(body_theory_atom, BodyTheoryAtom,
            ATTR(sign, sign))
        TYPE(body_set_aggregate, BodySetAggregate,
            ATTR(sign, sign))
        TYPE(head_aggregate, HeadAggregate,
            ATTR(function, fun))
        TYPE(body_aggregate, BodyAggregate,
            ATTR(sign, sign)
            ATTR(function, fun)))
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
            ATTR(symbol, value))
        TYPE(theory_term_symbolic, TheoryTermSymbol,
            ATTR(symbol, value)))
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
            ATTR(name, name))
        TYPE(theory_term_variable, TheoryTermVariable,
            ATTR(name, name))
        TYPE(term_function, TermFunction,
            ATTR(name, name))
        TYPE(theory_term_function, TheoryTermFunction,
            ATTR(name, name))
        TYPE(theory_right_guard, TheoryRGuard::value_type,
            ATTR(theory_operator, first)))
    // clang-format on
}

#undef ATTR
#define ATTR(attr, value)                                                                                              \
    case clingo_ast_attribute_##attr: {                                                                                \
        return tcb::make_span(cast<Type>().value);                                                                     \
    }

auto clingo_ast::get_string_vec(clingo_ast_attribute_t attr) const -> std::optional<tcb::span<Gringo::String const>> {
    // clang-format off
    SWITCH(
        TYPE(unparsed_element, TheoryTermUnparsed::Element,
            ATTR(operators, first)))
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
            ATTR(right, rhs))
        TYPE(term_binary_operation, TermBinary,
            ATTR(left, lhs)
            ATTR(right, rhs))
        TYPE(unparsed_element, TheoryTermUnparsed::Element,
            ATTR(term, second))
        TYPE(literal_comparison, LiteralRelation,
            ATTR(left, lhs))
        TYPE(literal_symbolic, LiteralSymbolic,
            ATTR(atom, term))
        TYPE(head_simple_literal, SimpleHeadLiteral,
            ATTR(literal, lit))
        TYPE(body_simple_literal, SimpleBodyLiteral,
            ATTR(literal, lit))
        TYPE(head_conditional_literal, ConditionalLiteral,
            ATTR(literal, lit))
        TYPE(body_conditional_literal, Conjunction,
            ATTR(literal, lit.lit))
        TYPE(left_guard, LGuard::value_type,
            ATTR(term, first))
        TYPE(right_guard, RGuard::value_type,
            ATTR(term, second))
        TYPE(theory_right_guard, TheoryRGuard::value_type,
            ATTR(term, second))
        TYPE(set_aggregate_element, SetAggregateElement,
            ATTR(literal, lit))
        TYPE(head_aggregate_element, HeadAggregate::Element,
            ATTR(literal, lit))
        TYPE(body_theory_atom, BodyTheoryAtom,
            ATTR(name, name)
            ATTR(right, rhs))
        TYPE(head_theory_atom, HeadTheoryAtom,
            ATTR(name, name)
            ATTR(right, rhs))
        TYPE(head_set_aggregate, HeadSetAggregate,
            ATTR(left, lhs)
            ATTR(right, rhs))
        TYPE(head_aggregate, HeadAggregate,
            ATTR(left, lhs)
            ATTR(right, rhs))
        TYPE(body_set_aggregate, BodySetAggregate,
            ATTR(left, lhs)
            ATTR(right, rhs))
        TYPE(body_aggregate, BodyAggregate,
            ATTR(left, lhs)
            ATTR(right, rhs)))
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
        TYPE(term_absolute, TermAbs,
            ATTR(pool, pool))
        TYPE(term_tuple, TermTuple,
            ATTR(pool, pool))
        TYPE(term_function, TermFunction,
            ATTR(pool, pool))
        TYPE(theory_term_tuple, TheoryTermTuple,
            ATTR(arguments, elems))
        TYPE(theory_term_function, TheoryTermFunction,
            ATTR(arguments, args))
        TYPE(theory_term_unparsed, TheoryTermUnparsed,
            ATTR(elements, elems))
        TYPE(literal_comparison, LiteralRelation,
            ATTR(right, rhs))
        TYPE(head_conditional_literal, ConditionalLiteral,
            ATTR(condition, cond))
        TYPE(body_conditional_literal, Conjunction,
            ATTR(condition, lit.cond))
        TYPE(set_aggregate_element, SetAggregateElement,
            ATTR(condition, cond))
        TYPE(theory_atom_element, TheoryElement,
            ATTR(tuple, tuple)
            ATTR(condition, cond))
        TYPE(head_aggregate_element, HeadAggregate::Element,
            ATTR(tuple, tuple)
            ATTR(condition, cond))
        TYPE(body_aggregate_element, BodyAggregate::Element,
            ATTR(tuple, tuple)
            ATTR(condition, cond))
        TYPE(head_disjunction, Disjunction,
            ATTR(elements, elems))
        TYPE(body_theory_atom, BodyTheoryAtom,
            ATTR(elements, elems))
        TYPE(head_theory_atom, HeadTheoryAtom,
            ATTR(elements, elems))
        TYPE(head_set_aggregate, HeadSetAggregate,
            ATTR(elements, elems))
        TYPE(head_aggregate, HeadAggregate,
            ATTR(elements, elems))
        TYPE(body_set_aggregate, BodySetAggregate,
            ATTR(elements, elems))
        TYPE(body_aggregate, BodyAggregate,
            ATTR(elements, elems)))
    // clang-format on
}

#undef ATTR
#undef TYPE
#undef SWITCH

auto clingo_ast::copy() const -> std::unique_ptr<clingo_ast_t> { return std::make_unique<clingo_ast>(*this); }

void clingo_ast::print(std::ostream &out) const {
    using namespace Gringo::Input;
    visit([&out](auto &x) {
        GRINGO_MATCH(x, TheoryRGuard) {
            if (x) {
                out << " " << x->first << " " << x->second;
            }
        }
        else GRINGO_MATCH(x, TheoryTermUnparsed::Element) {
            for (auto const &op : x.first) {
                out << op << " ";
            }
            out << x.second;
        }
        else GRINGO_MATCH(x, TupleVec) {
            bool comma = false;
            for (auto const &elem : x) {
                if (comma) {
                    out << ",";
                } else {
                    comma = true;
                }
                std::visit([&out](auto &x) { out << x; }, elem);
            }
        }
        else GRINGO_MATCH(x, LGuard::value_type) {
            out << x.first << " " << x.second << " ";
        }
        else GRINGO_MATCH(x, RGuard::value_type) {
            out << " " << x.first << " " << x.second;
        }
        else {
            out << x;
        }
    });
}

auto clingo_ast::hash() const -> size_t {
    using namespace Gringo::Input;
    return visit([this](auto &x) { return Gringo::Util::value_hash(type_, x); });
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

template <> [[nodiscard]] auto clingo_ast::convert<Gringo::Input::Literal>() const -> Gringo::Input::Literal {
    switch (type_) {
        case clingo_ast_type_literal_boolean: {
            return cast<Gringo::Input::LiteralBoolean>();
        }
        case clingo_ast_type_literal_symbolic: {
            return cast<Gringo::Input::LiteralSymbolic>();
        }
        case clingo_ast_type_literal_comparison: {
            return cast<Gringo::Input::LiteralSymbolic>();
        }
        default: {
            throw std::runtime_error("literal expected");
        }
    }
}

template <> [[nodiscard]] auto clingo_ast::convert<Gringo::Input::TupleElem>() const -> Gringo::Input::TupleElem {
    if (type_ == clingo_ast_type_projection) {
        return cast<Gringo::Input::Projection>();
    }
    return convert<Gringo::Input::Term>();
}

template <> [[nodiscard]] auto clingo_ast::convert<Gringo::Input::TupleVec>() const -> Gringo::Input::TupleVec {
    if (type_ == clingo_ast_type_argument_tuple) {
        return cast<Gringo::Input::TupleVec>();
    }
    throw std::runtime_error("argument tuple expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::TermTuple::Element>() const -> Gringo::Input::TermTuple::Element {
    if (type_ == clingo_ast_type_argument_tuple) {
        return cast<Gringo::Input::TupleVec>();
    }
    return convert<Gringo::Input::Term>();
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::TheoryTermUnparsed::Element>() const
    -> Gringo::Input::TheoryTermUnparsed::Element {
    if (type_ == clingo_ast_type_unparsed_element) {
        return cast<Gringo::Input::TheoryTermUnparsed::Element>();
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
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::BodyAggregate::Element>() const
    -> Gringo::Input::BodyAggregate::Element {
    if (type_ == clingo_ast_type_body_aggregate_element) {
        return cast<Gringo::Input::BodyAggregate::Element>();
    }
    throw std::runtime_error("body aggregate element expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::HeadAggregate::Element>() const
    -> Gringo::Input::HeadAggregate::Element {
    if (type_ == clingo_ast_type_head_aggregate_element) {
        return cast<Gringo::Input::HeadAggregate::Element>();
    }
    throw std::runtime_error("body aggregate element expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::Disjunction::Element>() const
    -> Gringo::Input::Disjunction::Element {
    if (type_ == clingo_ast_type_head_conditional_literal) {
        return cast<Gringo::Input::ConditionalLiteral>();
    }
    return cast<Gringo::Input::Literal>();
}

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
                    type, convert_loc(lib, loc), convert_ast_vec<Gringo::Input::TermTuple::Element>(pool, size));
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
                *ast = construct_ast<Gringo::Input::TermFunction>(type, convert_loc(lib, loc), lib->store->string(name),
                                                                  convert_ast_vec<Gringo::Input::TupleVec>(pool, size),
                                                                  sign != 0);
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
                    Gringo::Util::construct_shared<Gringo::Input::Term>(rhs->convert<Gringo::Input::Term>()));
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
                    Gringo::Util::construct_shared<Gringo::Input::Term>(lhs->convert<Gringo::Input::Term>()),
                    static_cast<Gringo::Input::BinaryOperator>(op),
                    Gringo::Util::construct_shared<Gringo::Input::Term>(rhs->convert<Gringo::Input::Term>()));
                break;
            }
            case clingo_ast_type_argument_tuple: {
                std::va_list args;
                va_start(args, ast);
                auto const **tuple = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<Gringo::Input::TupleVec>(type,
                                                              convert_ast_vec<Gringo::Input::TupleElem>(tuple, size));
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
                *ast = construct_ast<Gringo::Input::LiteralBoolean>(type, convert_loc(lib, loc),
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
                *ast = construct_ast<Gringo::Input::LiteralSymbolic>(type, convert_loc(lib, loc),
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
                *ast = construct_ast<Gringo::Input::LiteralRelation>(
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
                *ast = construct_ast<Gringo::Input::TheoryTermUnparsed::Element>(
                    type, convert_string_array(lib, ops, size), term->convert<Gringo::Input::TheoryTerm>());
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
                    type, convert_loc(lib, loc),
                    convert_ast_vec<Gringo::Input::TheoryTermUnparsed::Element>(elems, size));
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
                    type, convert_loc(lib, loc), lit->convert<Literal>(),
                    convert_ast_vec<Gringo::Input::Literal>(cond, size));
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
                    convert_ast_vec<Gringo::Input::Literal>(cond, cond_size));
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
                *ast = construct_ast<Gringo::Input::SimpleBodyLiteral>(type, lit->convert<Gringo::Input::Literal>());
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
                *ast = construct_ast<Gringo::Input::BodyAggregate::Element>(
                    type, convert_loc(lib, loc), convert_ast_vec<Gringo::Input::Term>(tuple, tuple_size),
                    convert_ast_vec<Gringo::Input::Literal>(cond, cond_size));
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
                *ast = construct_ast<Gringo::Input::BodyAggregate>(
                    type, convert_loc(lib, loc), static_cast<Gringo::Input::Sign>(sign),
                    convert_ast_opt<Gringo::Input::LGuard::value_type>(lhs),
                    static_cast<Gringo::Input::AggregateFunction>(fun),
                    convert_ast_vec<Gringo::Input::BodyAggregate::Element>(elems, elems_size),
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
                *ast = construct_ast<Gringo::Input::BodySetAggregate>(
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
                *ast = construct_ast<Gringo::Input::BodyTheoryAtom>(
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
                *ast = construct_ast<Gringo::Input::Conjunction>(
                    type,
                    Gringo::Input::ConditionalLiteral{convert_loc(lib, loc), lit->convert<Gringo::Input::Literal>(),
                                                      convert_ast_vec<Gringo::Input::Literal>(cond, cond_size)});
                break;
            }
            case clingo_ast_type_head_simple_literal: {
                std::va_list args;
                va_start(args, ast);
                auto const *lit = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<Gringo::Input::SimpleHeadLiteral>(type, lit->convert<Gringo::Input::Literal>());
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
                *ast = construct_ast<Gringo::Input::HeadAggregate::Element>(
                    type, convert_loc(lib, loc), convert_ast_vec<Gringo::Input::Term>(tuple, tuple_size),
                    lit->convert<Literal>(), convert_ast_vec<Gringo::Input::Literal>(cond, cond_size));
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
                *ast = construct_ast<Gringo::Input::HeadAggregate>(
                    type, convert_loc(lib, loc), convert_ast_opt<Gringo::Input::LGuard::value_type>(lhs),
                    static_cast<Gringo::Input::AggregateFunction>(fun),
                    convert_ast_vec<Gringo::Input::HeadAggregate::Element>(elems, elems_size),
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
                *ast = construct_ast<Gringo::Input::HeadSetAggregate>(
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
                *ast = construct_ast<Gringo::Input::HeadTheoryAtom>(
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
                *ast = construct_ast<Gringo::Input::ConditionalLiteral>(
                    type,
                    Gringo::Input::ConditionalLiteral{convert_loc(lib, loc), lit->convert<Gringo::Input::Literal>(),
                                                      convert_ast_vec<Gringo::Input::Literal>(cond, cond_size)});
                break;
            }
            case clingo_ast_type_head_disjunction: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **elems = va_arg(args, clingo_ast_t const **);
                auto elems_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<Gringo::Input::Disjunction>(
                    type, convert_loc(lib, loc),
                    convert_ast_vec<Gringo::Input::Disjunction::Element>(elems, elems_size));
                break;
            }
        }
    }
    CLINGO_CATCH(lib);
}

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

extern "C" void clingo_ast_free(clingo_ast_t *ast) { delete ast; }

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
                auto owner = Gringo::Util::construct_shared<std::any>(std::move(term).value());
                auto const *ptr = std::any_cast<Gringo::Input::Term>(owner.get());
                *ast = make_ast(owner, *ptr).release();
                break;
            }
            case clingo_ast_parse_type_literal: {
                auto lit = Gringo::Input::parse_literal(lib->log, *lib->store, string);
                if (lib->log.has_error() || !lit) {
                    lib->log.reset();
                    throw std::runtime_error("parsing term failed");
                }
                auto owner = Gringo::Util::construct_shared<std::any>(std::move(lit).value());
                auto const *ptr = std::any_cast<Gringo::Input::Literal>(owner.get());
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

extern "C" auto clingo_ast_type_info_yaml() -> char const * {
    return R"yaml(unary_operator:
  type: enum
  doc: Available unary operators.
  values:
    minus:
      value: 0
      doc: Operator `-`.
    negation:
      value: 1
      doc: Operator `~`.
binary_operator:
  type: enum
  doc: Available binary operators.
  values:
    and:
      value: 0
      doc: Operator `&`.
    division:
      value: 1
      doc: Operator `/`.
    minus:
      value: 2
      doc: Operator `-`.
    modulo:
      value: 3
      doc: Operator `%`.
    multiplication:
      value: 4
      doc: Operator `*`.
    or:
      value: 5
      doc: Operator `|`.
    plus:
      value: 6
      doc: Operator `+`.
    power:
      value: 7
      doc: Operator `**`.
    xor:
      value: 8
      doc: Operator `^`.
sign:
  type: enum
  doc: The available signs.
  values:
    no_sign:
      value: 0
      doc: No sign.
    single:
      value: 1
      doc: One sign.
    double:
      value: 2
      doc: Two signs.
relation:
  type: enum
  doc: Available relation symbols.
  values:
    equal:
      value: 0
      doc: The equal to relation.
    not_equal:
      value: 1
      doc: The not equal to relation.
    less:
      value: 2
      doc: The less than relation.
    less_equal:
      value: 3
      doc: The less than or equal to relation.
    greater:
      value: 4
      doc: The greater than relation.
    greater_equal:
      value: 5
      doc: The greater than or equal to relation.
aggregate_function:
  type: enum
  doc: Enumeration of aggregate functions.
  values:
    count:
      value: 0
      doc: Operator "^".
    sum:
      value: 1
      doc: Operator "?".
    sump:
      value: 2
      doc: Operator "&".
    min:
      value: 3
      doc: Operator "+".
    max:
      value: 4
      doc: Operator "-".
theory_operator:
  type: enum
  doc: Enumeration of theory operators.
  values:
    unary:
      value: 0
      doc: An unary theory operator.
    binary_left:
      value: 1
      doc: A left associative binary operator.
    binary_right:
      value: 2
      doc: A right associative binary operator.
theory_tuple_type:
  type: enum
  doc: Enumeration of theory tuple types.
  values:
    tuple:
      value: 0
      doc: Theory tuples "(t1,...,tn)".
    set:
      value: 1
      doc: Theory sets "{t1,...,tn}".
    list:
      value: 2
      doc: Theory lists "[t1,...,tn]".
theory_atom_type:
  type: enum
  doc: Enumeration of the theory atom types.
  values:
    head:
      value: 0
      doc: For theory atoms that can appear in the head.
    body:
      value: 1
      doc: For theory atoms that can appear in the body.
    any:
      value: 2
      doc: For theory atoms that can appear in both head and body.
    directive:
      value: 3
      doc: For theory atoms that must not have a body.
term:
  type: union
  types:
  - term_variable
  - term_symbolic
  - term_absolute
  - term_unary_operation
  - term_binary_operation
  - term_tuple
  - term_function
term_array:
  type: array
  value_type: term
projection:
  type: record
  doc: A placeholder for an argument to project.
  arguments:
    location:
      type: location
      doc: The location of the placeholder.
term_or_projection:
  type: union
  types:
  - term
  - projection
term_or_projection_array:
  type: array
  value_type: term_or_projection
argument_tuple_array:
  type: array
  value_type: argument_tuple
term_or_argument_tuple:
  type: union
  types:
  - term
  - argument_tuple
term_or_argument_tuple_array:
  type: array
  value_type: term_or_argument_tuple
term_variable:
  type: record
  doc: A term representing a variable.
  arguments:
    location:
      type: location
      doc: The location of the variable.
    name:
      type: string
      doc: The name of the variable.
    anonymous:
      type: bool
      default: false
      doc: >-
        Whether the variable is anonymous.

        Anonymous variables receive a unique name during preprocessing.
term_symbolic:
  type: record
  doc: A term representing a symbol.
  arguments:
    location:
      type: location
      doc: The location of the symbol.
    symbol:
      type: symbol
      doc: The symbol.
term_absolute:
  type: record
  doc: A term representing the absolute operation.
  arguments:
    location:
      type: location
      doc: The location of the operation.
    pool:
      type: term_array
      doc: >-
        The argument pool.

        If there is more than one argument in the pool, the term is unpooled during preprocessing.
term_unary_operation:
  type: record
  doc: A term representing a unary operation.
  arguments:
    location:
      type: location
      doc: The location of the operation.
    operator_type:
      type: unary_operator
      doc: The type of the operation.
    right:
      type: term
      doc: The argument of the operation.
term_binary_operation:
  type: record
  doc: A term representing a binary operation.
  arguments:
    location:
      type: location
      doc: The location of the operation.
    left:
      type: term
      doc: The left argument of the operation.
    operator_type:
      type: binary_operator
      doc: The type of the operation.
    right:
      type: term
      doc: The right argument of the operation.
term_tuple:
  type: record
  doc: A term representing a tuple.
  arguments:
    location:
      type: location
      doc: The location of the tuple.
    pool:
      type: term_or_argument_tuple_array
      doc: >-
        The argument pool of the tuple.

        If there is more than one element in the pool, the term is unpooled during preprocessing.
term_function:
  type: record
  doc: A term representing a function.
  arguments:
    location:
      type: location
      doc: The location of the function.
    name:
      type: string
      doc: The name of the function.
    pool:
      type: argument_tuple_array
      doc: >-
        The argument pool of the function.

        If there is more than one element in the pool, the term is unpooled during preprocessing.
    external:
      type: bool
      default: false
      doc: Whether the function is external.
argument_tuple:
  type: record
  doc: A list of arguments for a function or tuple.
  arguments:
    arguments:
      type: term_or_projection_array
      default: empty
      doc: The arguments of the tuple.
literal:
  type: union
  types:
  - literal_boolean
  - literal_comparison
  - literal_symbolic
left_guard:
  type: record
  doc: A right hand side guard consisting of a term and a relation.
  arguments:
    term:
      type: term
      doc: The term of the guard.
    relation:
      type: relation
      doc: The relation of the guard.
optional_left_guard:
  type: optional
  value_type: left_guard
right_guard:
  type: record
  doc: A right hand side guard consisting of a relation and term.
  arguments:
    relation:
      type: relation
      doc: The relation of the guard.
    term:
      type: term
      doc: The term of the guard.
optional_right_guard:
  type: optional
  value_type: right_guard
right_guard_array:
  type: array
  value_type: right_guard
literal_boolean:
  type: record
  doc: A literal representing a Boolean constant.
  arguments:
    location:
      type: location
      doc: The location of the symbol.
    sign:
      type: sign
      doc: The sign of the literal.
    value:
      type: bool
      doc: The fixed value of the literal.
literal_comparison:
  type: record
  doc: A literal representing a (chain of) comparison(s).
  arguments:
    location:
      type: location
      doc: The location of the symbol.
    sign:
      type: sign
      doc: The sign of the literal.
    left:
      type: term
      doc: The first term of the comparison.
    right:
      type: right_guard_array
      doc: >-
        The chain of comparisons.

        Note that the chain must have at least length one.
literal_symbolic:
  type: record
  doc: A literal representing a symbolic literal.
  arguments:
    location:
      type: location
      doc: The location of the symbol.
    sign:
      type: sign
      doc: The sign of the literal.
    atom:
      type: term
      doc: The term representing the atom.
theory_term:
  type: union
  types:
  - theory_term_variable
  - theory_term_symbolic
  - theory_term_tuple
  - theory_term_function
  - theory_term_unparsed
theory_term_array:
  type: array
  value_type: theory_term
unparsed_element:
  type: record
  doc: A list of unparsed theory terms and operators.
  arguments:
    operators:
      type: string_array
      doc: The list of theory operators.
    term:
      type: theory_term
      doc: The theory term.
unparsed_element_array:
  type: array
  value_type: unparsed_element
theory_term_variable:
  type: record
  doc: A theory term representing a variable.
  arguments:
    location:
      type: location
      doc: The location of the variable.
    name:
      type: string
      doc: The name of the variable.
    anonymous:
      type: bool
      default: false
      doc: >-
        Whether the variable is anonymous.

        Anonymous variables receive a unique name during preprocessing.
theory_term_symbolic:
  type: record
  doc: A theory term representing a symbol.
  arguments:
    location:
      type: location
      doc: The location of the symbol.
    symbol:
      type: symbol
      doc: The symbol.
theory_term_tuple:
  type: record
  doc: A theory term representing a tuple.
  arguments:
    location:
      type: location
      doc: The location of the tuple.
    tuple_type:
      type: theory_tuple_type
      doc: The type of the tuple.
    arguments:
      type: theory_term_array
      doc: The arguments of the tuple.
theory_term_function:
  type: record
  doc: A theory term representing a function.
  arguments:
    location:
      type: location
      doc: The location of the function.
    name:
      type: string
      doc: The name of the function.
    arguments:
      type: theory_term_array
      doc: The arguments of the function.
theory_term_unparsed:
  type: record
  doc: A theory term representing an unparsed theory term.
  arguments:
    location:
      type: location
      doc: The location of the theory term.
    elements:
      type: unparsed_element_array
      doc: The unparsed theory elements.
theory_right_guard:
  type: record
  doc: A right hand side guard consisting of a theory operator and theory term.
  arguments:
    theory_operator:
      type: string
      doc: The operator of the guard.
    term:
      type: theory_term
      doc: The theory term of the guard.
optional_theory_right_guard:
  type: optional
  value_type: theory_right_guard
literal_array:
  type: array
  value_type: literal
set_aggregate_element:
  type: record
  doc: An element of a set aggregate.
  arguments:
    location:
      type: location
      doc: The location of the element.
    literal:
      type: literal
      doc: The literal of the element.
    condition:
      type: literal_array
      doc: The condition of the element.
set_aggregate_element_array:
  type: array
  value_type: set_aggregate_element
body_aggregate_element:
  type: record
  doc: An element of a body aggregate.
  arguments:
    location:
      type: location
      doc: The location of the element.
    tuple:
      type: term_array
      doc: The term tuple of the element.
    condition:
      type: literal_array
      doc: The condition of the element.
body_aggregate_element_array:
  type: array
  value_type: body_aggregate_element
theory_atom_element:
  type: record
  doc: An element of a theory atom elements.
  arguments:
    location:
      type: location
      doc: The location of the element.
    tuple:
      type: theory_term_array
      doc: The theory term tuple of the element.
    condition:
      type: literal_array
      doc: The condition of the element.
theory_atom_element_array:
  type: array
  value_type: theory_atom_element
body_literal:
  type: union
  doc: The available body literals.
  types:
  - body_simple_literal
  - body_aggregate
  - body_set_aggregate
  - body_theory_atom
  - body_conditional_literal
body_simple_literal:
  type: record
  doc: A literal in a rule body.
  arguments:
    literal:
      type: literal
      doc: The literal.
body_aggregate:
  type: record
  doc: An aggregate in a rule body.
  arguments:
    location:
      type: location
      doc: The location of the element.
    sign:
      type: sign
      doc: The sign of the literal.
    left:
      type: optional_left_guard
      doc: The left guard of the aggregate.
    function:
      type: aggregate_function
      doc: The aggregate function.
    elements:
      type: body_aggregate_element_array
      doc: The aggregate elements.
    right:
      type: optional_right_guard
      doc: The right guard of the aggregate.
body_set_aggregate:
  type: record
  doc: A set aggregate.
  arguments:
    location:
      type: location
      doc: The location of the element.
    sign:
      type: sign
      doc: The sign of the literal.
    left:
      type: optional_left_guard
      doc: The left guard of the aggregate.
    elements:
      type: set_aggregate_element_array
      doc: The aggregate elements.
    right:
      type: optional_right_guard
      doc: The right guard of the aggregate.
body_theory_atom:
  type: record
  doc: A theory atom.
  arguments:
    location:
      type: location
      doc: The location of the element.
    sign:
      type: sign
      doc: The sign of the literal.
    name:
      type: term
      doc: The name of the theory atom.
    elements:
      type: theory_atom_element_array
      doc: The aggregate elements.
    right:
      type: optional_theory_right_guard
      doc: The right guard of the theory atom.
body_conditional_literal:
  type: record
  doc: A conditional_literal.
  arguments:
    location:
      type: location
      doc: The location of the element.
    literal:
      type: literal
      doc: The literal of the element.
    condition:
      type: literal_array
      doc: The condition of the element.
head_conditional_literal:
  type: record
  doc: A conditional_literal.
  arguments:
    location:
      type: location
      doc: The location of the element.
    literal:
      type: literal
      doc: The literal of the element.
    condition:
      type: literal_array
      doc: The condition of the element.
disjunction_element:
  type: union
  doc: An element of a disjunction.
  types:
  - literal
  - head_conditional_literal
disjunction_element_array:
  type: array
  value_type: disjunction_element
head_aggregate_element:
  type: record
  doc: An element of a head aggregate.
  arguments:
    location:
      type: location
      doc: The location of the element.
    tuple:
      type: term_array
      doc: The term tuple of the element.
    literal:
      type: literal
      doc: The literal of the element.
    condition:
      type: literal_array
      doc: The condition of the element.
head_aggregate_element_array:
  type: array
  value_type: head_aggregate_element
head_literal:
  type: union
  doc: The available head literals.
  types:
  - head_simple_literal
  - head_aggregate
  - head_set_aggregate
  - head_theory_atom
  - head_disjunction
head_simple_literal:
  type: record
  doc: A literal in a rule head.
  arguments:
    literal:
      type: literal
      doc: The literal.
head_aggregate:
  type: record
  doc: An aggregate in a rule head.
  arguments:
    location:
      type: location
      doc: The location of the element.
    left:
      type: optional_left_guard
      doc: The left guard of the aggregate.
    function:
      type: aggregate_function
      doc: The aggregate function.
    elements:
      type: head_aggregate_element_array
      doc: The aggregate elements.
    right:
      type: optional_right_guard
      doc: The right guard of the aggregate.
head_set_aggregate:
  type: record
  doc: A set aggregate.
  arguments:
    location:
      type: location
      doc: The location of the element.
    left:
      type: optional_left_guard
      doc: The left guard of the aggregate.
    elements:
      type: set_aggregate_element_array
      doc: The aggregate elements.
    right:
      type: optional_right_guard
      doc: The right guard of the aggregate.
head_theory_atom:
  type: record
  doc: A theory atom.
  arguments:
    location:
      type: location
      doc: The location of the element.
    name:
      type: term
      doc: The name of the theory atom.
    elements:
      type: theory_atom_element_array
      doc: The aggregate elements.
    right:
      type: optional_theory_right_guard
      doc: The right guard of the theory atom.
head_disjunction:
  type: record
  doc: A disjunction.
  arguments:
    location:
      type: location
      doc: The location of the element.
    elements:
      type: disjunction_element_array
      doc: The elements of the disjunction.
)yaml";
}
