#include <cstdarg>
#include <cstring>

#include "lib.hh"
#include "streams.hh"

#include <gringo/util/ordered_map.hh>
#include <gringo/util/ordered_set.hh>

#include <gringo/input/algo/parse.hh>
#include <gringo/input/algo/print.hh>

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
    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] virtual auto hash() const -> size_t = 0;
    [[nodiscard]] virtual auto equal_to(clingo_ast_t const &other) const -> bool = 0;
    [[nodiscard]] virtual auto less_than(clingo_ast_t const &other) const -> bool = 0;
    [[nodiscard]] virtual auto get_type() const -> clingo_ast_type_e = 0;
    [[nodiscard]] virtual auto get_number(clingo_ast_attribute_t attr) const -> std::optional<int>;
    [[nodiscard]] virtual auto get_symbol(clingo_ast_attribute_t attr) const -> std::optional<clingo_symbol_t>;
    [[nodiscard]] virtual auto get_location(clingo_ast_attribute_t attr) const -> std::optional<clingo_location_t>;
    [[nodiscard]] virtual auto get_string(clingo_ast_attribute_t attr) const -> std::optional<char const *>;
    [[nodiscard]] virtual auto get_ast(clingo_ast_attribute_t attr) const
        -> std::optional<std::unique_ptr<clingo_ast_t>>;
    [[nodiscard]] virtual auto get_ast_vec(clingo_ast_attribute_t attr) const -> std::optional<ASTVec>;
    virtual ~clingo_ast() = default;

    friend auto operator<<(std::ostream &out, clingo_ast_t const &ast) -> std::ostream & {
        ast.print(out);
        return out;
    }
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
        return clingo_ast_type_term_unary_operation;
    }
    auto operator()(Gringo::Input::TermBinary const &term) const -> clingo_ast_type_e {
        static_cast<void>(term);
        return clingo_ast_type_term_binary_operation;
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

class ASTProjection : public clingo_ast {
  public:
    ASTProjection(Gringo::Input::Projection projection) : projection_{std::move(projection)} {}
    [[nodiscard]] auto copy() const -> std::unique_ptr<clingo_ast_t> override {
        return std::make_unique<ASTProjection>(projection_);
    }
    void print(std::ostream &out) const override { out << "*"; }
    [[nodiscard]] auto get_type() const -> clingo_ast_type_e override { return clingo_ast_type_projection; }
    [[nodiscard]] auto get_location(clingo_ast_attribute_t attr) const -> std::optional<clingo_location_t> override {
        if (attr == clingo_ast_attribute_location) {
            return convert_loc(projection_.loc);
        }
        return std::nullopt;
    }

    [[nodiscard]] auto equal_to(clingo_ast_t const &other) const -> bool override {
        return dynamic_cast<ASTProjection const *>(&other) != nullptr;
    }

    [[nodiscard]] auto hash() const -> size_t override { return typeid(projection_).hash_code(); }

    [[nodiscard]] auto less_than(clingo_ast_t const &other) const -> bool override {
        return get_type() < other.get_type();
    }
    template <class T> friend auto ast_convert(clingo_ast const *ast) -> T;

  private:
    Gringo::Input::Projection projection_;
};

// Note: the AST could simply store the library object for error reporting.
// This would allow for better error reporting at the expense of a tiny memory overhead.
class ASTTerm : public clingo_ast {
  public:
    ASTTerm(Gringo::Input::Term term) : term{std::move(term)} {}
    [[nodiscard]] auto copy() const -> std::unique_ptr<clingo_ast_t> override {
        return std::make_unique<ASTTerm>(term);
    }
    void print(std::ostream &out) const override { out << term; }
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
    [[nodiscard]] auto get_ast_vec(clingo_ast_attribute_t attr) const -> std::optional<ASTVec> override {
        return std::visit(GetASTVec{attr}, term);
    }

    [[nodiscard]] auto hash() const -> size_t override { return Gringo::Util::value_hash(term); }

    [[nodiscard]] auto equal_to(clingo_ast_t const &other) const -> bool override {
        auto const *b = dynamic_cast<ASTTerm const *>(&other);
        if (b == nullptr) {
            return false;
        }
        return term == b->term;
    }

    [[nodiscard]] auto less_than(clingo_ast_t const &other) const -> bool override {
        auto t_a = get_type();
        auto t_b = other.get_type();
        if (t_a != t_b) {
            return t_a < t_b;
        }
        return term < static_cast<ASTTerm const &>(other).term;
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

class ASTArgumentTuple : public clingo_ast {
  public:
    ASTArgumentTuple(ASTVec tuple) : tuple_{std::move(tuple)} {}
    [[nodiscard]] auto copy() const -> std::unique_ptr<clingo_ast_t> override {
        return std::make_unique<ASTArgumentTuple>(tuple_);
    }
    void print(std::ostream &out) const override {
        bool comma = false;
        for (auto &term : tuple_) {
            if (comma) {
                out << ",";
            } else {
                comma = true;
            }
            out << term;
        }
    }
    [[nodiscard]] auto get_type() const -> clingo_ast_type_e override { return clingo_ast_type_argument_tuple; }

    [[nodiscard]] auto hash() const -> size_t override {
        size_t hash = typeid(ASTArgumentTuple).hash_code();
        for (auto const *elem : tuple_) {
            hash = Gringo::Util::hash_combine({hash, elem->hash()});
        }
        return hash;
    }

    [[nodiscard]] auto equal_to(clingo_ast_t const &other) const -> bool override {
        auto const *b = dynamic_cast<ASTArgumentTuple const *>(&other);
        if (b == nullptr) {
            return false;
        }
        return std::equal(tuple_.begin(), tuple_.end(), b->tuple_.begin(), b->tuple_.end(),
                          [](auto const *a, auto const *b) { return a->equal_to(*b); });
    }

    [[nodiscard]] auto less_than(clingo_ast_t const &other) const -> bool override {
        auto t_a = get_type();
        auto t_b = other.get_type();
        if (t_a != t_b) {
            return t_a < t_b;
        }
        auto const &b = static_cast<ASTArgumentTuple const &>(other);
        return std::lexicographical_compare(tuple_.begin(), tuple_.end(), b.tuple_.begin(), b.tuple_.end(),
                                            [](auto const *a, auto const *b) { return a->less_than(*b); });
    }

    template <class T> friend auto ast_convert(clingo_ast const *ast) -> T;

  private:
    ASTVec tuple_;
};

template <> auto ast_convert<Gringo::Input::TupleVec>(clingo_ast const *ast) -> Gringo::Input::TupleVec {
    if (auto const *res = dynamic_cast<ASTArgumentTuple const *>(ast); res != nullptr) {
        Gringo::Input::TupleVec tuple;
        tuple.reserve(res->tuple_.size());
        for (auto const *elem : res->tuple_) {
            if (auto const *projection = dynamic_cast<ASTProjection const *>(elem)) {
                tuple.emplace_back(projection->projection_);
            } else {
                tuple.emplace_back(ast_convert<Gringo::Input::Term>(elem));
            }
        }
        return tuple;
    }
    throw std::runtime_error("invalid type: argument tuple expected");
}

template <>
auto ast_convert<Gringo::Input::TermTuple::Element>(clingo_ast const *ast) -> Gringo::Input::TermTuple::Element {
    if (auto const *res = dynamic_cast<ASTTerm const *>(ast); res != nullptr) {
        return res->term;
    }
    return ast_convert<Gringo::Input::TupleVec>(ast);
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
        } else {
            res[i] = std::make_unique<ASTProjection>(std::get<Gringo::Input::Projection>(term_or_projection)).release();
        }
        ++i;
    }
    return std::make_unique<ASTArgumentTuple>(std::move(res));
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
            case clingo_ast_type_projection: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                va_end(args);
                *ast = new ASTProjection{Gringo::Input::Projection{convert_loc(lib, loc)}};
                break;
            }
            case clingo_ast_type_term_variable: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *name = va_arg(args, char const *);
                auto anonymous = va_arg(args, int);
                va_end(args);
                *ast = new ASTTerm{
                    Gringo::Input::TermVariable{convert_loc(lib, loc), lib->store->string(name), anonymous != 0}};
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
            case clingo_ast_type_term_unary_operation: {
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
            case clingo_ast_type_term_binary_operation: {
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
            case clingo_ast_type_argument_tuple: {
                std::va_list args;
                va_start(args, ast);
                auto const **tuple = va_arg(args, clingo_ast_t const **);
                auto size = va_arg(args, size_t);
                va_end(args);
                *ast = new ASTArgumentTuple{ASTVec::copy(tuple, size)};
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

extern "C" auto clingo_ast_type_info_yaml() -> char const * {
    return R"yaml(- name: unary_operator
  type: enum
  doc: Available unary operators.
  values:
    minus:
      value: 0
      doc: Operator `-`.
    negation:
      value: 1
      doc: Operator `~`.
- name: binary_operator
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
- name: term_variable
  type: forward
- name: term_symbolic
  type: forward
- name: term_absolute
  type: forward
- name: term_unary_operation
  type: forward
- name: term_binary_operation
  type: forward
- name: term_tuple
  type: forward
- name: term_function
  type: forward
- name: term_variable
  type: forward
- name: term
  type: union
  types:
  - term_variable
  - term_symbolic
  - term_absolute
  - term_unary_operation
  - term_binary_operation
  - term_tuple
  - term_function
- name: term_array
  type: array
  value_type: term
- name: projection
  type: record
  doc: A placeholder for an argument to project.
  arguments:
  - name: location
    type: location
    doc: The location of the placeholder.
- name: term_or_projection
  type: union
  types:
  - term
  - projection
- name: term_or_projection_array
  type: array
  value_type: term_or_projection
- name: argument_tuple
  type: forward
- name: argument_tuple_array
  type: array
  value_type: argument_tuple
- name: term_or_argument_tuple
  type: union
  types:
  - term
  - argument_tuple
- name: term_or_argument_tuple_array
  type: array
  value_type: term_or_argument_tuple
- name: term_variable
  type: record
  doc: A term representing a variable.
  arguments:
  - name: location
    type: location
    doc: The location of the variable.
  - name: name
    type: string
    doc: The name of the variable.
  - name: anonymous
    type: bool
    default: false
    doc: >-
      Whether the variable is anonymous.

      Anonymous variables receive a unique name during preprocessing.
- name: term_symbolic
  type: record
  doc: A term representing a symbol.
  arguments:
  - name: location
    type: location
    doc: The location of the symbol.
  - name: symbol
    type: symbol
    doc: The symbol.
- name: term_absolute
  type: record
  doc: A term representing the absolute operation.
  arguments:
  - name: location
    type: location
    doc: The location of the operation.
  - name: pool
    type: term_array
    doc: >-
      The argument pool.

      If there is more than one argument in the pool, the term is unpooled during preprocessing.
- name: term_unary_operation
  type: record
  doc: A term representing a unary operation.
  arguments:
  - name: location
    type: location
    doc: The location of the operation.
  - name: operator_type
    type: unary_operator
    doc: The type of the operation.
  - name: right
    type: term
    doc: The argument of the operation.
- name: term_binary_operation
  type: record
  doc: A term representing a binary operation.
  arguments:
  - name: location
    type: location
    doc: The location of the operation.
  - name: left
    type: term
    doc: The left argument of the operation.
  - name: operator_type
    type: binary_operator
    doc: The type of the operation.
  - name: right
    type: term
    doc: The right argument of the operation.
- name: term_tuple
  type: record
  doc: A term representing a tuple.
  arguments:
  - name: location
    type: location
    doc: The location of the tuple.
  - name: pool
    type: term_or_argument_tuple_array
    doc: >-
      The argument pool of the tuple.

      If there is more than one element in the pool, the term is unpooled during preprocessing.
- name: term_function
  type: record
  doc: A term representing a function.
  arguments:
  - name: location
    type: location
    doc: The location of the function.
  - name: name
    type: string
    doc: The name of the function.
  - name: pool
    type: argument_tuple_array
    doc: >-
      The argument pool of the tuple.

      If there is more than one element in the pool, the term is unpooled during preprocessing.
  - name: external
    type: bool
    default: false
    doc: Whether the function is external.
- name: argument_tuple
  type: record
  doc: A list of arguments for a function or tuple.
  arguments:
  - name: arguments
    type: term_or_projection_array
    default: empty
    doc: The arguments of the tuple.
)yaml";
}
