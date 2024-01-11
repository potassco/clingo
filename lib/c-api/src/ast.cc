#include <cstdarg>
#include <cstring>

#include "lib.hh"

#include <gringo/util/ordered_map.hh>
#include <gringo/util/ordered_set.hh>

#include <gringo/input/algo/parse.hh>

namespace {
class ASTVec;

template <class T> auto ast_convert(clingo_ast const *ast) -> T = delete;

template <class T> auto ast_vec_convert(clingo_ast const **ast, size_t size) -> std::vector<T> {
    std::vector<T> res;
    res.reserve(size);
    for (auto it = ast, ie = ast + size; it != ie; ++it) {
        res.emplace_back(ast_convert<T>(*it));
    }
    return res;
}
} // namespace

struct clingo_ast {
    [[nodiscard]] virtual auto copy() const -> std::unique_ptr<clingo_ast_t> = 0;
    [[nodiscard]] virtual auto get_type() const -> clingo_ast_type_e = 0;
    [[nodiscard]] virtual auto get_number(clingo_ast_attribute_t attr) const -> std::optional<int>;
    [[nodiscard]] virtual auto get_symbol(clingo_ast_attribute_t attr) const -> std::optional<clingo_symbol_t>;
    [[nodiscard]] virtual auto get_location(clingo_ast_attribute_t attr) const -> std::optional<clingo_location_t>;
    [[nodiscard]] virtual auto get_string(clingo_ast_attribute_t attr) const -> std::optional<char const *>;
    [[nodiscard]] virtual auto get_ast(clingo_ast_attribute_t attr) const
        -> std::optional<std::unique_ptr<clingo_ast_t>>;
    [[nodiscard]] virtual auto get_ast_vec(clingo_ast_attribute_t attr) const -> std::optional<ASTVec>;
    virtual ~clingo_ast() = default;
};

namespace {

/*
static_assert(clingo_ast_theory_sequence_type_tuple ==
              static_cast<clingo_ast_theory_sequence_type_e>(Gringo::Input::TheoryTermTupleType::tuple));
static_assert(clingo_ast_theory_sequence_type_list ==
              static_cast<clingo_ast_theory_sequence_type_e>(Gringo::Input::TheoryTermTupleType::list));
static_assert(clingo_ast_theory_sequence_type_set ==
              static_cast<clingo_ast_theory_sequence_type_e>(Gringo::Input::TheoryTermTupleType::set));

static_assert(clingo_ast_comparison_operator_equal ==
              static_cast<clingo_ast_comparison_operator_e>(Gringo::Input::Relation::equal));
static_assert(clingo_ast_comparison_operator_not_equal ==
              static_cast<clingo_ast_comparison_operator_e>(Gringo::Input::Relation::inequal));
static_assert(clingo_ast_comparison_operator_less_than ==
              static_cast<clingo_ast_comparison_operator_e>(Gringo::Input::Relation::less));
static_assert(clingo_ast_comparison_operator_less_equal ==
              static_cast<clingo_ast_comparison_operator_e>(Gringo::Input::Relation::less_equal));
static_assert(clingo_ast_comparison_operator_greater_than ==
              static_cast<clingo_ast_comparison_operator_e>(Gringo::Input::Relation::greater));
static_assert(clingo_ast_comparison_operator_greater_equal ==
              static_cast<clingo_ast_comparison_operator_e>(Gringo::Input::Relation::greater_equal));

static_assert(clingo_ast_sign_no_sign == static_cast<clingo_ast_sign_e>(Gringo::Input::Sign::none));
static_assert(clingo_ast_sign_negation == static_cast<clingo_ast_sign_e>(Gringo::Input::Sign::once));
static_assert(clingo_ast_sign_double_negation == static_cast<clingo_ast_sign_e>(Gringo::Input::Sign::twice));

static_assert(clingo_ast_unary_operator_minus ==
              static_cast<clingo_ast_unary_operator_e>(Gringo::Input::UnaryOperator::negate));
static_assert(clingo_ast_unary_operator_negation ==
              static_cast<clingo_ast_unary_operator_e>(Gringo::Input::UnaryOperator::invert));

static_assert(clingo_ast_binary_operator_and ==
              static_cast<clingo_ast_binary_operator_e>(Gringo::Input::BinaryOperator::and_));
static_assert(clingo_ast_binary_operator_division ==
              static_cast<clingo_ast_binary_operator_e>(Gringo::Input::BinaryOperator::div));
static_assert(clingo_ast_binary_operator_minus ==
              static_cast<clingo_ast_binary_operator_e>(Gringo::Input::BinaryOperator::minus));
static_assert(clingo_ast_binary_operator_modulo ==
              static_cast<clingo_ast_binary_operator_e>(Gringo::Input::BinaryOperator::mod));
static_assert(clingo_ast_binary_operator_multiplication ==
              static_cast<clingo_ast_binary_operator_e>(Gringo::Input::BinaryOperator::times));
static_assert(clingo_ast_binary_operator_or ==
              static_cast<clingo_ast_binary_operator_e>(Gringo::Input::BinaryOperator::or_));
static_assert(clingo_ast_binary_operator_plus ==
              static_cast<clingo_ast_binary_operator_e>(Gringo::Input::BinaryOperator::plus));
static_assert(clingo_ast_binary_operator_power ==
              static_cast<clingo_ast_binary_operator_e>(Gringo::Input::BinaryOperator::pow));
static_assert(clingo_ast_binary_operator_xor ==
              static_cast<clingo_ast_binary_operator_e>(Gringo::Input::BinaryOperator::xor_));

static_assert(clingo_ast_aggregate_function_count ==
              static_cast<clingo_ast_aggregate_function_e>(Gringo::Input::AggregateFunction::count));
static_assert(clingo_ast_aggregate_function_sum ==
              static_cast<clingo_ast_aggregate_function_e>(Gringo::Input::AggregateFunction::sum));
static_assert(clingo_ast_aggregate_function_sump ==
              static_cast<clingo_ast_aggregate_function_e>(Gringo::Input::AggregateFunction::sump));
static_assert(clingo_ast_aggregate_function_min ==
              static_cast<clingo_ast_aggregate_function_e>(Gringo::Input::AggregateFunction::min));
static_assert(clingo_ast_aggregate_function_max ==
              static_cast<clingo_ast_aggregate_function_e>(Gringo::Input::AggregateFunction::max));

static_assert(clingo_ast_theory_operator_type_unary ==
              static_cast<clingo_ast_theory_operator_type_e>(Gringo::Input::TheoryOpType::unary));
static_assert(clingo_ast_theory_operator_type_binary_left ==
              static_cast<clingo_ast_theory_operator_type_e>(Gringo::Input::TheoryOpType::binary_left));
static_assert(clingo_ast_theory_operator_type_binary_right ==
              static_cast<clingo_ast_theory_operator_type_e>(Gringo::Input::TheoryOpType::binary_right));

static_assert(clingo_ast_theory_atom_definition_type_head ==
              static_cast<clingo_ast_theory_atom_definition_type_e>(Gringo::Input::TheoryAtomType::head));
static_assert(clingo_ast_theory_atom_definition_type_body ==
              static_cast<clingo_ast_theory_atom_definition_type_e>(Gringo::Input::TheoryAtomType::body));
static_assert(clingo_ast_theory_atom_definition_type_any ==
              static_cast<clingo_ast_theory_atom_definition_type_e>(Gringo::Input::TheoryAtomType::any));
static_assert(clingo_ast_theory_atom_definition_type_directive ==
              static_cast<clingo_ast_theory_atom_definition_type_e>(Gringo::Input::TheoryAtomType::directive));

*/

class ASTVec {
  public:
    ASTVec() = default;
    ASTVec(size_t size) {
        if (size > 0) {
            data_ = new clingo_ast_t *[size] { nullptr };
            size_ = size;
        }
    }
    ASTVec(clingo_ast_t const **data, size_t size) : ASTVec{size} {
        for (size_t i = 0; i < size; ++i) {
            operator[](i) = data[i]->copy().release();
        }
    }
    ASTVec(ASTVec const &other) : ASTVec{other.data_, other.size()} {}
    ASTVec(ASTVec &&other) noexcept {
        std::swap(other.data_, data_);
        std::swap(other.size_, size_);
    }
    auto operator=(ASTVec const &other) -> ASTVec & {
        *this = ASTVec{other};
        return *this;
    }
    auto operator=(ASTVec &&other) noexcept -> ASTVec & {
        std::swap(other.data_, data_);
        std::swap(other.size_, size_);
        return *this;
    }
    ~ASTVec() {
        // TODO: check nolints
        for (auto it = data_, ie = data_ + size_; it != ie; ++it) {
            delete *it; // NOLINT
        }
        delete[] data_; // NOLINT
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

  private:
    ASTVec(clingo_ast_t **data, size_t size) : data_{data}, size_{size} {}

    clingo_ast_t **data_ = nullptr;
    size_t size_ = 0;
};

struct GetType {
    // terms
    auto operator()(Gringo::Input::TermVariable const &term) const -> clingo_ast_type_e {
        static_cast<void>(term);
        return clingo_ast_type_term_variable;
    }
    auto operator()(Gringo::Input::TermSymbol const &term) const -> clingo_ast_type_e {
        static_cast<void>(term);
        return clingo_ast_type_term_symbolic;
    }
    auto operator()(Gringo::Input::TermTuple const &term) const -> clingo_ast_type_e {
        static_cast<void>(term);
        return clingo_ast_type_term_tuple;
    }
    auto operator()(Gringo::Input::TermFunction const &term) const -> clingo_ast_type_e {
        static_cast<void>(term);
        return clingo_ast_type_term_function;
    }
    auto operator()(Gringo::Input::TermAbs const &term) const -> clingo_ast_type_e {
        static_cast<void>(term);
        return clingo_ast_type_term_absolute;
    }
    auto operator()(Gringo::Input::TermUnary const &term) const -> clingo_ast_type_e {
        static_cast<void>(term);
        return clingo_ast_type_term_unary;
    }
    auto operator()(Gringo::Input::TermBinary const &term) const -> clingo_ast_type_e {
        static_cast<void>(term);
        return clingo_ast_type_term_binary;
    }
};

struct GetNumber {
    // default
    template <class T> auto operator()(T const &term) const -> std::optional<int> {
        static_cast<void>(term);
        return std::nullopt;
    }
    // terms
    auto operator()(Gringo::Input::TermVariable const &term) const -> std::optional<int> {
        switch (attr) {
            case clingo_ast_attribute_anonymous: {
                return static_cast<int>(term.is_anonymous);
            }
            default: {
                return std::nullopt;
            }
        }
    }
    auto operator()(Gringo::Input::TermFunction const &term) const -> std::optional<int> {
        switch (attr) {
            case clingo_ast_attribute_external: {
                return static_cast<int>(term.external);
            }
            default: {
                return std::nullopt;
            }
        }
    }
    auto operator()(Gringo::Input::TermUnary const &term) const -> std::optional<int> {
        switch (attr) {
            case clingo_ast_attribute_operator_type: {
                return static_cast<int>(term.op);
            }
            default: {
                return std::nullopt;
            }
        }
    }
    auto operator()(Gringo::Input::TermBinary const &term) const -> std::optional<int> {
        switch (attr) {
            case clingo_ast_attribute_operator_type: {
                return static_cast<int>(term.op);
            }
            default: {
                return std::nullopt;
            }
        }
    }
    clingo_ast_attribute_t attr;
};

struct GetSymbol {
    // default
    template <class T> auto operator()(T const &term) const -> std::optional<clingo_symbol_t> {
        static_cast<void>(term);
        return std::nullopt;
    }
    // terms
    auto operator()(Gringo::Input::TermSymbol const &term) const -> std::optional<clingo_symbol_t> {
        switch (attr) {
            case clingo_ast_attribute_symbol: {
                return static_cast<clingo_symbol_t>(Gringo::Symbol::to_rep(term.value));
            }
            default: {
                return std::nullopt;
            }
        }
    }
    clingo_ast_attribute_t attr;
};

struct GetString {
    // default
    template <class T> auto operator()(T const &term) const -> std::optional<char const *> {
        static_cast<void>(term);
        return std::nullopt;
    }
    // terms
    auto operator()(Gringo::Input::TermVariable const &term) const -> std::optional<char const *> {
        switch (attr) {
            case clingo_ast_attribute_name: {
                return term.name.c_str();
            }
            default: {
                return std::nullopt;
            }
        }
    }
    auto operator()(Gringo::Input::TermFunction const &term) const -> std::optional<char const *> {
        switch (attr) {
            case clingo_ast_attribute_name: {
                return term.name.c_str();
            }
            default: {
                return std::nullopt;
            }
        }
    }
    clingo_ast_attribute_t attr;
};

auto convert_loc(clingo_lib_t *lib, clingo_location_t const *loc) -> Gringo::Input::Location {
    return {{lib->store->string(loc->begin_file), loc->begin_line, loc->begin_column},
            {lib->store->string(loc->end_file), loc->end_line, loc->end_column}};
}

auto convert_loc(Gringo::Input::Location const &loc) -> clingo_location_t {
    return {loc.begin.file.c_str(), loc.end.file.c_str(), loc.begin.line,
            loc.end.line,           loc.begin.column,     loc.end.column};
}

auto make_ast(Gringo::Input::Term const &term) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Gringo::Input::TupleVec const &tuple) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Gringo::Input::TermTuple::Element const &tuple) -> std::unique_ptr<clingo_ast_t>;
auto make_ast_vec(Gringo::Input::TermVec const &terms) -> ASTVec;
auto make_ast_vec(Gringo::Input::PoolVec const &pool) -> ASTVec;
auto make_ast_vec(Gringo::Input::TermTuple::ElementVec const &pool) -> ASTVec;

struct GetAST {
    // default
    template <class T> auto operator()(T const &term) const -> std::optional<std::unique_ptr<clingo_ast_t>> {
        static_cast<void>(term);
        return std::nullopt;
    }
    // terms
    auto operator()(Gringo::Input::TermUnary const &term) const -> std::optional<std::unique_ptr<clingo_ast_t>> {
        switch (attr) {
            case clingo_ast_attribute_right: {
                return make_ast(*term.rhs);
            }
            default: {
                return std::nullopt;
            }
        }
    }
    auto operator()(Gringo::Input::TermBinary const &term) const -> std::optional<std::unique_ptr<clingo_ast_t>> {
        switch (attr) {
            case clingo_ast_attribute_left: {
                return make_ast(*term.lhs);
            }
            case clingo_ast_attribute_right: {
                return make_ast(*term.rhs);
            }
            default: {
                return std::nullopt;
            }
        }
    }
    clingo_ast_attribute_t attr;
};

struct GetASTVec {
    // default
    template <class T> auto operator()(T const &term) const -> std::optional<ASTVec> {
        static_cast<void>(term);
        return std::nullopt;
    }
    // terms
    auto operator()(Gringo::Input::TermAbs const &term) const -> std::optional<ASTVec> {
        switch (attr) {
            case clingo_ast_attribute_pool: {
                return make_ast_vec(term.pool);
            }
            default: {
                return std::nullopt;
            }
        }
    }
    auto operator()(Gringo::Input::TermTuple const &term) const -> std::optional<ASTVec> {
        switch (attr) {
            case clingo_ast_attribute_pool: {
                return make_ast_vec(term.pool);
            }
            default: {
                return std::nullopt;
            }
        }
    }
    auto operator()(Gringo::Input::TermFunction const &term) const -> std::optional<ASTVec> {
        switch (attr) {
            case clingo_ast_attribute_pool: {
                return make_ast_vec(term.pool);
            }
            default: {
                return std::nullopt;
            }
        }
    }
    clingo_ast_attribute_t attr;
};

// Note: the AST could simply store the library object for error reporting.
// This would allow for better error reporting at the expense of a tiny memory overhead.
class ASTTerm : public clingo_ast {
  public:
    ASTTerm(Gringo::Input::Term term) : term{std::move(term)} {}
    [[nodiscard]] auto copy() const -> std::unique_ptr<clingo_ast_t> override {
        return std::make_unique<ASTTerm>(term);
    }
    [[nodiscard]] auto get_type() const -> clingo_ast_type_e override { return std::visit(GetType{}, term); }
    [[nodiscard]] auto get_number(clingo_ast_attribute_t attr) const -> std::optional<int> override {
        return std::visit(GetNumber{attr}, term);
    }
    [[nodiscard]] auto get_symbol(clingo_ast_attribute_t attr) const -> std::optional<clingo_symbol_t> override {
        return std::visit(GetSymbol{attr}, term);
    }
    [[nodiscard]] auto get_location(clingo_ast_attribute_t attr) const -> std::optional<clingo_location_t> override {
        if (attr == clingo_ast_attribute_location) {
            return convert_loc(location(term));
        }
        return std::nullopt;
    }
    [[nodiscard]] auto get_string(clingo_ast_attribute_t attr) const -> std::optional<char const *> override {
        return std::visit(GetString{attr}, term);
    }
    [[nodiscard]] auto get_ast(clingo_ast_attribute_t attr) const
        -> std::optional<std::unique_ptr<clingo_ast_t>> override {
        return std::visit(GetAST{attr}, term);
    }

    template <class T> friend auto ast_convert(clingo_ast const *ast) -> T;

  private:
    Gringo::Input::Term term;
};

template <> auto ast_convert<Gringo::Input::Term>(clingo_ast const *ast) -> Gringo::Input::Term {
    if (auto const *res = dynamic_cast<ASTTerm const *>(ast); res != nullptr) {
        return res->term;
    }
    throw std::runtime_error("invalid type: term expected");
}

template <>
auto ast_convert<Gringo::Util::shared_ptr<Gringo::Input::Term>>(clingo_ast const *ast)
    -> Gringo::Util::shared_ptr<Gringo::Input::Term> {
    return Gringo::Util::construct_shared<Gringo::Input::Term>(ast_convert<Gringo::Input::Term>(ast));
}

class ASTTermPool : public clingo_ast {
  public:
    ASTTermPool(ASTVec tuple) : tuple_{std::move(tuple)} {}
    [[nodiscard]] auto copy() const -> std::unique_ptr<clingo_ast_t> override {
        return std::make_unique<ASTTermPool>(tuple_);
    }
    [[nodiscard]] auto get_type() const -> clingo_ast_type_e override { return clingo_ast_type_pool; }

    template <class T> friend auto ast_convert(clingo_ast const *ast) -> T;

  private:
    ASTVec tuple_;
};

template <> auto ast_convert<Gringo::Input::TupleVec>(clingo_ast const *ast) -> Gringo::Input::TupleVec {
    if (auto const *res = dynamic_cast<ASTTermPool const *>(ast); res != nullptr) {
        Gringo::Input::TupleVec tuple;
        tuple.reserve(res->tuple_.size());
        for (auto const *elem : res->tuple_) {
            if (elem != nullptr) {
                tuple.emplace_back(ast_convert<Gringo::Input::Term>(elem));
            } else {
                tuple.emplace_back(Gringo::Input::Projection{});
            }
        }
        return tuple;
    }
    throw std::runtime_error("invalid type: pool expected");
}

template <>
auto ast_convert<Gringo::Input::TermTuple::Element>(clingo_ast const *ast) -> Gringo::Input::TermTuple::Element {
    if (auto const *res = dynamic_cast<ASTTerm const *>(ast); res != nullptr) {
        return res->term;
    }
    if (auto const *res = dynamic_cast<ASTTermPool const *>(ast); res != nullptr) {
        Gringo::Input::TupleVec tuple;
        tuple.reserve(res->tuple_.size());
        for (auto const *elem : res->tuple_) {
            if (elem != nullptr) {
                tuple.emplace_back(ast_convert<Gringo::Input::Term>(elem));
            } else {
                tuple.emplace_back(Gringo::Input::Projection{});
            }
        }
        return tuple;
    }
    throw std::runtime_error("invalid type: term or pool expected");
}

auto make_ast(Gringo::Input::Term const &term) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<ASTTerm>(term);
}

auto make_ast(Gringo::Input::TupleVec const &tuple) -> std::unique_ptr<clingo_ast_t> {
    ASTVec res{tuple.size()};
    size_t i = 0;
    for (auto const &term_or_projection : tuple) {
        if (auto const *term = std::get_if<Gringo::Input::Term>(&term_or_projection); term != nullptr) {
            res[i] = make_ast(*term).release();
        }
        ++i;
    }
    return std::make_unique<ASTTermPool>(std::move(res));
}

auto make_ast(Gringo::Input::TermTuple::Element const &term_or_tuple) -> std::unique_ptr<clingo_ast_t> {
    return std::visit([](auto const &x) { return make_ast(x); }, term_or_tuple);
}

auto make_ast_vec(Gringo::Input::PoolVec const &pool) -> ASTVec {
    ASTVec res{pool.size()};
    size_t i = 0;
    for (auto const &tuple : pool) {
        res[i] = make_ast(tuple).release();
        ++i;
    }
    return res;
}

auto make_ast_vec(Gringo::Input::TermTuple::ElementVec const &pool) -> ASTVec {
    ASTVec res{pool.size()};
    size_t i = 0;
    for (auto const &tuple : pool) {
        res[i] = make_ast(tuple).release();
        ++i;
    }
    return res;
}

auto make_ast_vec(Gringo::Input::TermVec const &terms) -> ASTVec {
    ASTVec res{terms.size()};
    size_t i = 0;
    for (auto const &term : terms) {
        res[i] = make_ast(term).release();
        ++i;
    }
    return res;
}

}; // namespace

auto clingo_ast::get_number(clingo_ast_attribute_t attr) const -> std::optional<int> {
    static_cast<void>(attr);
    return std::nullopt;
}
auto clingo_ast::get_symbol(clingo_ast_attribute_t attr) const -> std::optional<clingo_symbol_t> {
    static_cast<void>(attr);
    return std::nullopt;
}
auto clingo_ast::get_location(clingo_ast_attribute_t attr) const -> std::optional<clingo_location_t> {
    static_cast<void>(attr);
    return std::nullopt;
}
auto clingo_ast::get_string(clingo_ast_attribute_t attr) const -> std::optional<char const *> {
    static_cast<void>(attr);
    return std::nullopt;
}
auto clingo_ast::get_ast(clingo_ast_attribute_t attr) const -> std::optional<std::unique_ptr<clingo_ast_t>> {
    static_cast<void>(attr);
    return std::nullopt;
}
auto clingo_ast::get_ast_vec(clingo_ast_attribute_t attr) const -> std::optional<ASTVec> {
    static_cast<void>(attr);
    return std::nullopt;
}

extern "C" auto clingo_ast_construct(clingo_lib_t *lib, clingo_ast_type_t type, clingo_ast_t **ast, ...) -> bool {
    CLINGO_TRY {
        if (lib == nullptr || ast == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        *ast = nullptr;
        switch (static_cast<clingo_ast_type_e>(type)) {
            case clingo_ast_type_term_variable: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                va_end(args);
                *ast = new ASTTerm{Gringo::Input::TermVariable{convert_loc(lib, loc), lib->store->string(name)}};
                break;
            }
            case clingo_ast_type_term_symbolic: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sym = va_arg(args, clingo_symbol_t);
                va_end(args);
                *ast = new ASTTerm{Gringo::Input::TermSymbol{convert_loc(lib, loc), Gringo::Symbol::from_rep(sym)}};
                break;
            }
            case clingo_ast_type_term_tuple: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **pool = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = new ASTTerm{Gringo::Input::TermTuple{
                    convert_loc(lib, loc), ast_vec_convert<Gringo::Input::TermTuple::Element>(pool, size)}};
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
                *ast = new ASTTerm{Gringo::Input::TermFunction{convert_loc(lib, loc), lib->store->string(name),
                                                               ast_vec_convert<Gringo::Input::TupleVec>(pool, size),
                                                               sign != 0}};
                break;
            }
            case clingo_ast_type_term_absolute: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const **pool = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = new ASTTerm{
                    Gringo::Input::TermAbs{convert_loc(lib, loc), ast_vec_convert<Gringo::Input::Term>(pool, size)}};
                break;
            }
            case clingo_ast_type_term_unary: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto op = va_arg(args, int);
                auto const *rhs = va_arg(args, clingo_ast_t *);
                va_end(args);
                *ast = new ASTTerm{
                    Gringo::Input::TermUnary{convert_loc(lib, loc), static_cast<Gringo::Input::UnaryOperator>(op),
                                             ast_convert<Gringo::Util::shared_ptr<Gringo::Input::Term>>(rhs)}};
                break;
            }
            case clingo_ast_type_term_binary: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *lhs = va_arg(args, clingo_ast_t *);
                auto op = va_arg(args, int);
                auto const *rhs = va_arg(args, clingo_ast_t *);
                va_end(args);
                *ast = new ASTTerm{Gringo::Input::TermBinary{
                    convert_loc(lib, loc), ast_convert<Gringo::Util::shared_ptr<Gringo::Input::Term>>(lhs),
                    static_cast<Gringo::Input::BinaryOperator>(op),
                    ast_convert<Gringo::Util::shared_ptr<Gringo::Input::Term>>(rhs)}};
                break;
            }
            case clingo_ast_type_pool: {
                std::va_list args;
                va_start(args, ast);
                auto const **pool = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = new ASTTermPool{ASTVec{pool, size}};
                break;
            }
        }
    }
    CLINGO_CATCH(lib);
}

extern "C" void clingo_ast_free(clingo_ast_t *ast) { delete ast; }

extern "C" void clingo_ast_array_free(clingo_ast_t **ast, size_t size) { ASTVec::acquire(ast, size); }

extern "C" auto clingo_ast_attribute_get_number(clingo_ast_t *ast, clingo_ast_attribute_t attribute, int *value)
    -> bool {
    // TODO: check args and error handling
    if (auto num = ast->get_number(attribute); num) {
        *value = *num;
        return true;
    }
    return false;
}

extern "C" auto clingo_ast_attribute_get_symbol(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                clingo_symbol_t *value) -> bool {
    // TODO: check args and error handling
    if (auto sym = ast->get_symbol(attribute); sym) {
        *value = *sym;
        return true;
    }
    return false;
}

extern "C" auto clingo_ast_attribute_get_location(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                  clingo_location_t *value) -> bool {
    // TODO: check args and error handling
    if (auto loc = ast->get_location(attribute); loc) {
        *value = *loc;
        return true;
    }
    return false;
}

extern "C" auto clingo_ast_attribute_get_string(clingo_ast_t *ast, clingo_ast_attribute_t attribute, char const **value)
    -> bool {
    // TODO: check args and error handling
    if (auto str = ast->get_string(attribute); str) {
        *value = *str;
        return true;
    }
    return false;
}

extern "C" auto clingo_ast_attribute_get_ast(clingo_ast_t *ast, clingo_ast_attribute_t attribute, clingo_ast_t **value)
    -> bool {
    // TODO: check args and error handling
    if (auto val = ast->get_ast(attribute); val) {
        *value = val->release();
        return true;
    }
    return false;
}

extern "C" auto clingo_ast_attribute_get_ast_array(clingo_ast_t *ast, clingo_ast_attribute_t attribute,
                                                   clingo_ast_t ***value, size_t *size) -> bool {
    // TODO: check args and error handling
    if (auto val = ast->get_ast_vec(attribute); val) {
        std::tie(*value, *size) = val->release();
        return true;
    }
    return false;
}

template <class T> struct Q {
    Q(T const &value) : value{value} {}
    friend auto operator<<(std::ostream &out, Q const &q) -> std::ostream & {
        out << '"' << q.value << '"';
        return out;
    }
    T const &value;
};

struct Comma {
    friend auto operator<<(std::ostream &out, Comma &q) -> std::ostream & {
        if (q.comma) {
            out << ",";
        } else {
            q.comma = true;
        }
        return out;
    }
    bool comma = false;
};

extern "C" auto clingo_ast_type_info_json() -> char const * {
    return R"([
  {
    "name": "unary_operator",
    "type": "enum",
    "doc": "Available unary operators.",
    "values": {
      "minus": {
        "value": 0,
        "doc": "Operator `-`."
      },
      "negation": {
        "value": 1,
        "doc": "Operator `~`."
      }
    }
  },
  {
    "name": "binary_operator",
    "type": "enum",
    "doc": "Available binary operators.",
    "values": {
      "and": {
        "value": 0,
        "doc": "Operator `&`."
      },
      "division": {
        "value": 1,
        "doc": "Operator `/`."
      },
      "minus": {
        "value": 2,
        "doc": "Operator `-`."
      },
      "modulo": {
        "value": 3,
        "doc": "Operator `%`."
      },
      "multiplication": {
        "value": 4,
        "doc": "Operator `*`."
      },
      "or": {
        "value": 5,
        "doc": "Operator `|`."
      },
      "plus": {
        "value": 6,
        "doc": "Operator `+`."
      },
      "power": {
        "value": 7,
        "doc": "Operator `**`."
      },
      "xor": {
        "value": 8,
        "doc": "Operator `^`."
      }
    }
  },
  {
    "name": "term_variable",
    "type": "forward"
  },
  {
    "name": "term_symbolic",
    "type": "forward"
  },
  {
    "name": "term_absolute",
    "type": "forward"
  },
  {
    "name": "term_unary_operation",
    "type": "forward"
  },
  {
    "name": "term_binary_operation",
    "type": "forward"
  },
  {
    "name": "term_tuple",
    "type": "forward"
  },
  {
    "name": "term_function",
    "type": "forward"
  },
  {
    "name": "term_variable",
    "type": "forward"
  },
  {
    "name": "term",
    "type": "union",
    "types": [
      "term_variable",
      "term_symbolic",
      "term_absolute",
      "term_unary_operation",
      "term_binary_operation",
      "term_tuple",
      "term_function"
    ]
  },
  {
    "name": "term_array",
    "type": "array",
    "value_type": "term"
  },
  {
    "name": "projection",
    "type": "record",
    "arguments": [
      {
        "name": "location",
        "type": "location"
      }
    ]
  },
  {
    "name": "term_or_projection",
    "type": "union",
    "types": ["term", "projection"]
  },
  {
    "name": "term_or_projection_array",
    "type": "array",
    "value_type": "term_or_projection"
  },
  {
    "name": "pool",
    "type": "forward"
  },
  {
    "name": "pool_array",
    "type": "array",
    "value_type": "pool"
  },
  {
    "name": "term_or_pool",
    "type": "union",
    "types": [
      "term",
      "pool"
    ]
  },
  {
    "name": "term_or_pool_array",
    "type": "array",
    "value_type": "term_or_pool"
  },
  {
    "name": "term_variable",
    "type": "record",
    "arguments": [
      {
        "name": "location",
        "type": "location"
      },
      {
        "name": "name",
        "type": "string"
      }
    ]
  },
  {
    "name": "term_symbolic",
    "type": "record",
    "arguments": [
      {
        "name": "location",
        "type": "location"
      },
      {
        "name": "symbol",
        "type": "symbol"
      }
    ]
  },
  {
    "name": "term_absolute",
    "type": "record",
    "arguments": [
      {
        "name": "location",
        "type": "location"
      },
      {
        "name": "pool",
        "type": "term_array"
      }
    ]
  },
  {
    "name": "term_unary_operation",
    "type": "record",
    "arguments": [
      {
        "name": "location",
        "type": "location"
      },
      {
        "name": "operator_type",
        "type": "unary_operator"
      },
      {
        "name": "right",
        "type": "term"
      }
    ]
  },
  {
    "name": "term_binary_operation",
    "type": "record",
    "arguments": [
      {
        "name": "location",
        "type": "location"
      },
      {
        "name": "left",
        "type": "term"
      },
      {
        "name": "operator_type",
        "type": "binary_operator"
      },
      {
        "name": "right",
        "type": "term"
      }
    ]
  },
  {
    "name": "term_tuple",
    "type": "record",
    "arguments": [
      {
        "name": "location",
        "type": "location"
      },
      {
        "name": "arguments",
        "type": "term_or_pool_array"
      }
    ]
  },
  {
    "name": "term_function",
    "type": "record",
    "arguments": [
      {
        "name": "location",
        "type": "location"
      },
      {
        "name": "name",
        "type": "string"
      },
      {
        "name": "arguments",
        "type": "pool_array"
      },
      {
        "name": "external",
        "type": "bool"
      }
    ]
  },
  {
    "name": "pool",
    "type": "record",
    "arguments": [
      {
        "name": "arguments",
        "type": "term_or_projection_array"
      }
    ]
  }
])";
}

/*
#define CONSTRUCTORS                                                                                                   \
    C(id, TODO, A(location, location), A(name, string)) \
    C(variable, Gringo::Input::TermVariable, A(location, location), A(name, string)) \
    C(symbolic_term, TODO, A(location, location), A(symbol, symbol)) \
    C(unary_operation, TODO, A(location, location), A(operator_type, number), A(argument, ast)) \
    C(binary_operation, TODO, A(location, location), A(operator_type, number), A(left, ast), A(right, ast)) \
    C(interval, TODO, A(location, location), A(left, ast), A(right, ast)) \
    C(function, TODO, A(location, location), A(name, string), A(arguments, ast_array), A(external, number)) \
    C(pool, TODO, A(location, location), A(arguments, ast_array)) \
    C(boolean_constant, TODO, A(value, number)) \
    C(symbolic_atom, TODO, A(symbol, ast)) \
    C(comparison, TODO, A(term, ast), A(guards, ast_array)) \
    C(guard, TODO, A(comparison, number), A(term, ast)) \
    C(conditional_literal, TODO, A(location, location), A(literal, ast), A(condition, ast_array)) \
    C(aggregate, TODO, A(location, location), A(left_guard, optional_ast), A(elements, ast_array), \
      A(right_guard, optional_ast))                                                                                    \
    C(body_aggregate_element, TODO, A(terms, ast_array), A(condition, ast_array)) \
    C(body_aggregate, TODO, A(location, location), A(left_guard, optional_ast), A(function, number), A(elements,
ast_array), \
      A(right_guard, optional_ast))                                                                                    \
    C(head_aggregate_element, TODO, A(terms, ast_array), A(condition, ast)) \
    C(head_aggregate, TODO, A(location, location), A(left_guard, optional_ast), A(function, number), A(elements,
ast_array), \
      A(right_guard, optional_ast))                                                                                    \
    C(disjunction, TODO, A(location, location), A(elements, ast_array)) \
    C(theory_sequence, TODO, A(location, location), A(sequence_type, number), A(terms, ast_array)) \
    C(theory_function, TODO, A(location, location), A(name, string), A(arguments, ast_array)) \
    C(theory_unparsed_term_element, TODO, A(operators, string_array), A(term, ast)) \
    C(theory_unparsed_term, TODO, A(location, location), A(elements, ast_array)) \
    C(theory_guard, TODO, A(operator_name, string), A(term, ast)) \
    C(theory_atom_element, TODO, A(terms, ast_array), A(condition, ast_array)) \
    C(theory_atom, TODO, A(location, location), A(term, ast), A(elements, ast_array), A(guard, optional_ast)) \
    C(literal, TODO, A(location, location), A(sign, number), A(atom, ast)) \
    C(theory_operator_definition, TODO, A(location, location), A(name, string), A(priority, number), \
      A(operator_type, number))                                                                                        \
    C(theory_term_definition, TODO, A(location, location), A(name, string), A(operators, ast_array)) \
    C(theory_guard_definition, TODO, A(operators, string_array), A(term, string)) \
    C(theory_atom_definition, TODO, A(location, location), A(atom_type, number), A(name, string), A(arity, number), \
      A(term, string), A(guard, optional_ast))                                                                         \
    C(rule, TODO, A(location, location), A(head, ast), A(body, ast_array)) \
    C(definition, TODO, A(location, location), A(name, string), A(value, ast), A(is_default, number)) \
    C(show_signature, TODO, A(location, location), A(name, string), A(arity, number), A(positive, number)) \
    C(show_term, TODO, A(location, location), A(term, ast), A(body, ast_array)) \
    C(minimize, TODO, A(location, location), A(weight, ast), A(priority, ast), A(terms, ast_array), A(body, ast_array))
\
    C(script, TODO, A(location, location), A(name, string), A(code, string)) \
    C(program, TODO, A(location, location), A(name, string), A(parameters, ast_array)) \
    C(external, TODO, A(location, location), A(atom, ast), A(body, ast_array), A(external_type, ast)) \
    C(edge, TODO, A(location, location), A(node_u, ast), A(node_v, ast), A(body, ast_array)) \
    C(heuristic, TODO, A(location, location), A(atom, ast), A(body, ast_array), A(bias, ast), A(priority, ast), \
      A(modifier, ast))                                                                                                \
    C(project_atom, TODO, A(location, location), A(atom, ast), A(body, ast_array)) \
    C(project_signature, TODO, A(location, location), A(name, string), A(arity, number), A(positive, number)) \
    C(defined, TODO, A(location, location), A(name, string), A(arity, number), A(positive, number)) \
    C(theory_definition, TODO, A(location, location), A(name, string), A(terms, ast_array), A(atoms, ast_array)) \
    C(comment, TODO, A(location, location), A(value, string), A(comment_type, number))
*/
