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

auto make_ast(Gringo::Input::LGuard::value_type const &guard) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Gringo::Input::RGuard::value_type const &guard) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Gringo::Input::Term const &term) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Gringo::Input::TupleVec const &tuple) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Gringo::Input::TermTuple::Element const &tuple) -> std::unique_ptr<clingo_ast_t>;

template <class T> auto make_ast_vec(std::vector<T> const &vec) -> ASTVec;

template <class T> auto convert_ast(clingo_ast const *ast) -> T = delete;

template <class T> auto convert_ast_vec(clingo_ast const **ast, size_t size) -> std::vector<T>;

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

template <class T> auto make_ast_vec(std::vector<T> const &vec) -> ASTVec {
    ASTVec res{vec.size()};
    size_t i = 0;
    for (auto const &elem : vec) {
        res[i] = make_ast(elem).release();
        ++i;
    }
    return res;
}

template <class T> auto make_ast_vec(Gringo::Util::immutable_vector<T> const &vec) -> ASTVec {
    ASTVec res{vec.size()};
    size_t i = 0;
    for (auto const &elem : vec) {
        res[i] = make_ast(elem).release();
        ++i;
    }
    return res;
}

template <class T> auto convert_ast_vec(clingo_ast const **ast, size_t size) -> std::vector<T> {
    std::vector<T> res;
    res.reserve(size);
    for (auto it = ast, ie = ast + size; it != ie; ++it) {
        res.emplace_back(convert_ast<T>(*it));
    }
    return res;
}

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
    // literals
    auto operator()(Gringo::Input::LiteralBoolean const &lit) const -> clingo_ast_type_e {
        static_cast<void>(lit);
        return clingo_ast_type_literal_boolean;
    }
    auto operator()(Gringo::Input::LiteralSymbolic const &lit) const -> clingo_ast_type_e {
        static_cast<void>(lit);
        return clingo_ast_type_literal_symbolic;
    }
    auto operator()(Gringo::Input::LiteralRelation const &lit) const -> clingo_ast_type_e {
        static_cast<void>(lit);
        return clingo_ast_type_literal_comparison;
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
    auto operator()(Gringo::Input::LiteralBoolean const &lit) const -> std::optional<int> {
        switch (attr) {
            case clingo_ast_attribute_sign: {
                return static_cast<int>(lit.sign);
            }
            case clingo_ast_attribute_value: {
                return static_cast<int>(lit.sign);
            }
            default: {
                return std::nullopt;
            }
        }
    }
    auto operator()(Gringo::Input::LiteralRelation const &lit) const -> std::optional<int> {
        switch (attr) {
            case clingo_ast_attribute_sign: {
                return static_cast<int>(lit.sign);
            }
            default: {
                return std::nullopt;
            }
        }
    }
    auto operator()(Gringo::Input::LiteralSymbolic const &lit) const -> std::optional<int> {
        switch (attr) {
            case clingo_ast_attribute_sign: {
                return static_cast<int>(lit.sign);
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
    auto operator()(Gringo::Input::LiteralRelation const &lit) const -> std::optional<std::unique_ptr<clingo_ast_t>> {
        switch (attr) {
            case clingo_ast_attribute_left: {
                return make_ast(lit.lhs);
            }
            default: {
                return std::nullopt;
            }
        }
    }
    auto operator()(Gringo::Input::LiteralSymbolic const &lit) const -> std::optional<std::unique_ptr<clingo_ast_t>> {
        switch (attr) {
            case clingo_ast_attribute_atom: {
                return make_ast(lit.term);
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
    auto operator()(Gringo::Input::LiteralRelation const &lit) const -> std::optional<ASTVec> {
        switch (attr) {
            case clingo_ast_attribute_right: {
                return make_ast_vec(lit.rhs);
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
    template <class T> friend auto convert_ast(clingo_ast const *ast) -> T;

  private:
    Gringo::Input::Projection projection_;
};

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

    template <class T> friend auto convert_ast(clingo_ast const *ast) -> T;

  private:
    Gringo::Input::Term term;
};

template <> auto convert_ast<Gringo::Input::Term>(clingo_ast const *ast) -> Gringo::Input::Term {
    if (auto const *res = dynamic_cast<ASTTerm const *>(ast); res != nullptr) {
        return res->term;
    }
    throw std::runtime_error("invalid type: term expected");
}

template <>
auto convert_ast<Gringo::Util::shared_ptr<Gringo::Input::Term>>(clingo_ast const *ast)
    -> Gringo::Util::shared_ptr<Gringo::Input::Term> {
    return Gringo::Util::construct_shared<Gringo::Input::Term>(convert_ast<Gringo::Input::Term>(ast));
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

    template <class T> friend auto convert_ast(clingo_ast const *ast) -> T;

  private:
    ASTVec tuple_;
};

template <> auto convert_ast<Gringo::Input::TupleVec>(clingo_ast const *ast) -> Gringo::Input::TupleVec {
    if (auto const *res = dynamic_cast<ASTArgumentTuple const *>(ast); res != nullptr) {
        Gringo::Input::TupleVec tuple;
        tuple.reserve(res->tuple_.size());
        for (auto const *elem : res->tuple_) {
            if (auto const *projection = dynamic_cast<ASTProjection const *>(elem)) {
                tuple.emplace_back(projection->projection_);
            } else {
                tuple.emplace_back(convert_ast<Gringo::Input::Term>(elem));
            }
        }
        return tuple;
    }
    throw std::runtime_error("invalid type: argument tuple expected");
}

template <>
auto convert_ast<Gringo::Input::TermTuple::Element>(clingo_ast const *ast) -> Gringo::Input::TermTuple::Element {
    if (auto const *res = dynamic_cast<ASTTerm const *>(ast); res != nullptr) {
        return res->term;
    }
    return convert_ast<Gringo::Input::TupleVec>(ast);
}

class ASTLGuard : public clingo_ast {
  public:
    ASTLGuard(Gringo::Input::LGuard::value_type guard) : guard_{std::move(guard)} {}
    [[nodiscard]] auto copy() const -> std::unique_ptr<clingo_ast_t> override {
        return std::make_unique<ASTLGuard>(guard_);
    }
    void print(std::ostream &out) const override { out << guard_.first << " " << guard_.second; }
    [[nodiscard]] auto get_type() const -> clingo_ast_type_e override { return clingo_ast_type_left_guard; }
    [[nodiscard]] auto get_ast(clingo_ast_attribute_t attr) const
        -> std::optional<std::unique_ptr<clingo_ast_t>> override {
        if (attr == clingo_ast_attribute_term) {
            return make_ast(guard_.first);
        }
        return std::nullopt;
    }
    [[nodiscard]] auto get_number(clingo_ast_attribute_t attr) const -> std::optional<int> override {
        if (attr == clingo_ast_attribute_relation) {
            return static_cast<int>(guard_.second);
        }
        return std::nullopt;
    }

    [[nodiscard]] auto equal_to(clingo_ast_t const &other) const -> bool override {
        auto const *x = dynamic_cast<ASTLGuard const *>(&other);
        return x != nullptr && guard_ == x->guard_;
    }

    [[nodiscard]] auto hash() const -> size_t override {
        return Gringo::Util::value_hash(typeid(ASTLGuard).hash_code(), guard_);
    }

    [[nodiscard]] auto less_than(clingo_ast_t const &other) const -> bool override {
        auto t_a = get_type();
        auto t_b = other.get_type();
        if (t_a != t_b) {
            return t_a < t_b;
        }
        return guard_ < static_cast<ASTLGuard const &>(other).guard_;
    }
    template <class T> friend auto convert_ast(clingo_ast const *ast) -> T;

  private:
    Gringo::Input::LGuard::value_type guard_;
};

template <>
[[maybe_unused]] auto convert_ast<Gringo::Input::LGuard::value_type>(clingo_ast const *ast)
    -> Gringo::Input::LGuard::value_type {
    if (auto const *res = dynamic_cast<ASTLGuard const *>(ast); res != nullptr) {
        return res->guard_;
    }
    throw std::runtime_error("invalid type: left guard expected");
}

class ASTRGuard : public clingo_ast {
  public:
    ASTRGuard(Gringo::Input::RGuard::value_type guard) : guard_{std::move(guard)} {}
    [[nodiscard]] auto copy() const -> std::unique_ptr<clingo_ast_t> override {
        return std::make_unique<ASTRGuard>(guard_);
    }
    void print(std::ostream &out) const override { out << guard_.first << " " << guard_.second; }
    [[nodiscard]] auto get_type() const -> clingo_ast_type_e override { return clingo_ast_type_right_guard; }
    [[nodiscard]] auto get_ast(clingo_ast_attribute_t attr) const
        -> std::optional<std::unique_ptr<clingo_ast_t>> override {
        if (attr == clingo_ast_attribute_term) {
            return make_ast(guard_.second);
        }
        return std::nullopt;
    }
    [[nodiscard]] auto get_number(clingo_ast_attribute_t attr) const -> std::optional<int> override {
        if (attr == clingo_ast_attribute_relation) {
            return static_cast<int>(guard_.first);
        }
        return std::nullopt;
    }

    [[nodiscard]] auto equal_to(clingo_ast_t const &other) const -> bool override {
        auto const *x = dynamic_cast<ASTRGuard const *>(&other);
        return x != nullptr && guard_ == x->guard_;
    }

    [[nodiscard]] auto hash() const -> size_t override {
        return Gringo::Util::value_hash(typeid(ASTRGuard).hash_code(), guard_);
    }

    [[nodiscard]] auto less_than(clingo_ast_t const &other) const -> bool override {
        auto t_a = get_type();
        auto t_b = other.get_type();
        if (t_a != t_b) {
            return t_a < t_b;
        }
        return guard_ < static_cast<ASTRGuard const &>(other).guard_;
    }
    template <class T> friend auto convert_ast(clingo_ast const *ast) -> T;

  private:
    Gringo::Input::RGuard::value_type guard_;
};

template <>
auto convert_ast<Gringo::Input::RGuard::value_type>(clingo_ast const *ast) -> Gringo::Input::RGuard::value_type {
    if (auto const *res = dynamic_cast<ASTRGuard const *>(ast); res != nullptr) {
        return res->guard_;
    }
    throw std::runtime_error("invalid type: right guard expected");
}

class ASTLiteral : public clingo_ast {
  public:
    ASTLiteral(Gringo::Input::Literal literal) : literal{std::move(literal)} {}
    [[nodiscard]] auto copy() const -> std::unique_ptr<clingo_ast_t> override {
        return std::make_unique<ASTLiteral>(literal);
    }
    void print(std::ostream &out) const override { out << literal; }
    [[nodiscard]] auto get_type() const -> clingo_ast_type_e override { return std::visit(GetType{}, literal); }
    [[nodiscard]] auto get_number(clingo_ast_attribute_t attr) const -> std::optional<int> override {
        return std::visit(GetNumber{attr}, literal);
    }
    [[nodiscard]] auto get_symbol(clingo_ast_attribute_t attr) const -> std::optional<clingo_symbol_t> override {
        return std::visit(GetSymbol{attr}, literal);
    }
    [[nodiscard]] auto get_location(clingo_ast_attribute_t attr) const -> std::optional<clingo_location_t> override {
        if (attr == clingo_ast_attribute_location) {
            return convert_loc(location(literal));
        }
        return std::nullopt;
    }
    [[nodiscard]] auto get_string(clingo_ast_attribute_t attr) const -> std::optional<char const *> override {
        return std::visit(GetString{attr}, literal);
    }
    [[nodiscard]] auto get_ast(clingo_ast_attribute_t attr) const
        -> std::optional<std::unique_ptr<clingo_ast_t>> override {
        return std::visit(GetAST{attr}, literal);
    }
    [[nodiscard]] auto get_ast_vec(clingo_ast_attribute_t attr) const -> std::optional<ASTVec> override {
        return std::visit(GetASTVec{attr}, literal);
    }

    [[nodiscard]] auto hash() const -> size_t override { return Gringo::Util::value_hash(literal); }

    [[nodiscard]] auto equal_to(clingo_ast_t const &other) const -> bool override {
        auto const *b = dynamic_cast<ASTLiteral const *>(&other);
        if (b == nullptr) {
            return false;
        }
        return literal == b->literal;
    }

    [[nodiscard]] auto less_than(clingo_ast_t const &other) const -> bool override {
        auto t_a = get_type();
        auto t_b = other.get_type();
        if (t_a != t_b) {
            return t_a < t_b;
        }
        return literal < static_cast<ASTLiteral const &>(other).literal;
    }

    template <class T> friend auto convert_ast(clingo_ast const *ast) -> T;

  private:
    Gringo::Input::Literal literal;
};

// TODO: remove attribute once used
template <> [[maybe_unused]] auto convert_ast<Gringo::Input::Literal>(clingo_ast const *ast) -> Gringo::Input::Literal {
    if (auto const *res = dynamic_cast<ASTLiteral const *>(ast); res != nullptr) {
        return res->literal;
    }
    throw std::runtime_error("invalid type: literal expected");
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

// TODO
[[maybe_unused]] auto make_ast(Gringo::Input::LGuard::value_type const &guard) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<ASTLGuard>(guard);
}

auto make_ast(Gringo::Input::RGuard::value_type const &guard) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<ASTRGuard>(guard);
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
                    convert_loc(lib, loc), convert_ast_vec<Gringo::Input::TermTuple::Element>(pool, size)}};
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
                                                               convert_ast_vec<Gringo::Input::TupleVec>(pool, size),
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
                    Gringo::Input::TermAbs{convert_loc(lib, loc), convert_ast_vec<Gringo::Input::Term>(pool, size)}};
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
                                             convert_ast<Gringo::Util::shared_ptr<Gringo::Input::Term>>(rhs)}};
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
                    convert_loc(lib, loc), convert_ast<Gringo::Util::shared_ptr<Gringo::Input::Term>>(lhs),
                    static_cast<Gringo::Input::BinaryOperator>(op),
                    convert_ast<Gringo::Util::shared_ptr<Gringo::Input::Term>>(rhs)}};
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
            case clingo_ast_type_left_guard: {
                std::va_list args;
                va_start(args, ast);
                auto const *left = va_arg(args, clingo_ast const *);
                auto right = va_arg(args, int);
                va_end(args);
                *ast = new ASTLGuard{Gringo::Input::LGuard::value_type{convert_ast<Gringo::Input::Term>(left),
                                                                       static_cast<Gringo::Input::Relation>(right)}};
                break;
            }
            case clingo_ast_type_right_guard: {
                std::va_list args;
                va_start(args, ast);
                auto left = va_arg(args, int);
                auto const *right = va_arg(args, clingo_ast const *);
                va_end(args);
                *ast = new ASTRGuard{Gringo::Input::RGuard::value_type{static_cast<Gringo::Input::Relation>(left),
                                                                       convert_ast<Gringo::Input::Term>(right)}};
                break;
            }
            case clingo_ast_type_literal_boolean: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sign = va_arg(args, int);
                auto value = va_arg(args, int);
                va_end(args);
                *ast = new ASTLiteral{Gringo::Input::LiteralBoolean{
                    convert_loc(lib, loc), static_cast<Gringo::Input::Sign>(sign), value != 0}};
                break;
            }
            case clingo_ast_type_literal_symbolic: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto sign = va_arg(args, int);
                auto const *atom = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = new ASTLiteral{Gringo::Input::LiteralSymbolic{convert_loc(lib, loc),
                                                                     static_cast<Gringo::Input::Sign>(sign),
                                                                     convert_ast<Gringo::Input::Term>(atom)}};
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
                *ast = new ASTLiteral{Gringo::Input::LiteralRelation{
                    convert_loc(lib, loc), static_cast<Gringo::Input::Sign>(sign),
                    convert_ast<Gringo::Input::Term>(left), convert_ast_vec<Gringo::Input::Guard>(right, size)}};
                break;
            }
            case clingo_ast_type_theory_term_variable: {
                throw std::runtime_error("implement me!!!");
            }
            case clingo_ast_type_theory_term_symbolic: {
                throw std::runtime_error("implement me!!!");
            }
            case clingo_ast_type_theory_term_tuple: {
                throw std::runtime_error("implement me!!!");
            }
            case clingo_ast_type_theory_term_function: {
                throw std::runtime_error("implement me!!!");
            }
            case clingo_ast_type_theory_term_unparsed: {
                throw std::runtime_error("implement me!!!");
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
                *ast = std::make_unique<ASTTerm>(std::move(term).value()).release();
                break;
            }
            case clingo_ast_parse_type_literal: {
                auto lit = Gringo::Input::parse_literal(lib->log, *lib->store, string);
                if (lib->log.has_error() || !lit) {
                    lib->log.reset();
                    throw std::runtime_error("parsing term failed");
                }
                *ast = std::make_unique<ASTLiteral>(std::move(lit).value()).release();
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
- name: sign
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
- name: relation
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
- name: aggregate_function
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
- name: theory_operator
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
- name: theory_tuple_type
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
- name: theory_atom_type
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
      The argument pool of the function.

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
- name: literal_boolean
  type: forward
- name: literal_comparison
  type: forward
- name: literal_symbolic
  type: forward
- name: literal
  type: union
  types:
  - literal_boolean
  - literal_comparison
  - literal_symbolic
- name: left_guard
  type: record
  doc: A right hand side guard consisting of a term and a relation.
  arguments:
  - name: term
    type: term
    doc: The term of the guard.
  - name: relation
    type: relation
    doc: The relation of the guard.
- name: right_guard
  type: record
  doc: A right hand side guard consisting of a relation and term.
  arguments:
  - name: relation
    type: relation
    doc: The relation of the guard.
  - name: term
    type: term
    doc: The term of the guard.
- name: right_guard_array
  type: array
  value_type: right_guard
- name: literal_boolean
  type: record
  doc: A literal representing a Boolean constant.
  arguments:
  - name: location
    type: location
    doc: The location of the symbol.
  - name: sign
    type: sign
    doc: The sign of the literal.
  - name: value
    type: bool
    doc: The fixed value of the literal.
- name: literal_comparison
  type: record
  doc: A literal representing a (chain of) comparison(s).
  arguments:
  - name: location
    type: location
    doc: The location of the symbol.
  - name: sign
    type: sign
    doc: The sign of the literal.
  - name: left
    type: term
    doc: The first term of the comparison.
  - name: right
    type: right_guard_array
    doc: >-
      The chain of comparisons.

      Note that the chain must have at least length one.
- name: literal_symbolic
  type: record
  doc: A literal representing a symbolic literal.
  arguments:
  - name: location
    type: location
    doc: The location of the symbol.
  - name: sign
    type: sign
    doc: The sign of the literal.
  - name: atom
    type: term
    doc: The term representing the atom.
- name: theory_term_variable
  type: forward
- name: theory_term_symbolic
  type: forward
- name: theory_term_tuple
  type: forward
- name: theory_term_function
  type: forward
- name: theory_term_unparsed
  type: forward
- name: theory_term
  type: union
  types:
  - theory_term_variable
  - theory_term_symbolic
  - theory_term_tuple
  - theory_term_function
  - theory_term_unparsed
- name: theory_term_array
  type: array
  value_type: theory_term
- name: theory_term_variable
  type: record
  doc: A theory term representing a variable.
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
- name: theory_term_symbolic
  type: record
  doc: A theory term representing a symbol.
  arguments:
  - name: location
    type: location
    doc: The location of the symbol.
  - name: symbol
    type: symbol
    doc: The symbol.
- name: theory_term_tuple
  type: record
  doc: A theory term representing a tuple.
  arguments:
  - name: location
    type: location
    doc: The location of the tuple.
  - name: tuple_type
    type: theory_tuple_type
    doc: The type of the tuple.
  - name: arguments
    type: theory_term_array
    doc: The arguments of the tuple.
- name: theory_term_function
  type: record
  doc: A theory term representing a function.
  arguments:
  - name: location
    type: location
    doc: The location of the function.
  - name: name
    type: string
    doc: The name of the function.
  - name: arguments
    type: theory_term_array
    doc: The arguments of the function.
- name: theory_term_unparsed
  type: record
  doc: A theory term representing an unparsed theory term.
  arguments:
  - name: location
    type: location
    doc: The location of the theory term.
)yaml";
}
