#include <any>
#include <cstdarg>
#include <cstring>
#include <forward_list>

#include "lib.hh"
#include "streams.hh"

#include <gringo/util/algorithm.hh>
#include <gringo/util/ordered_map.hh>
#include <gringo/util/ordered_set.hh>

#include <gringo/input/algo/parse.hh>
#include <gringo/input/algo/print.hh>
#include <gringo/input/algo/rewrite.hh>
#include <gringo/input/algo/substitute.hh>

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
auto make_ast(Owner const &owner, Gringo::Input::TermTuple::Element const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::Literal const &lit) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TheoryTermUnparsed::Element const &elem)
    -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TheoryElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::SetAggregateElement const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::BodyAggregate::Element const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::HeadAggregate::Element const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::Disjunction::Element const &elem) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::BodyLiteral const &lit) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::HeadLiteral const &lit) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TheoryOpDefinition const &def) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TheoryRGuardDefinition const &def) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TheoryTermDefinition const &def) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::TheoryAtomDefinition const &def) -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::StatementOptimize::Tuple const &tuple)
    -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::StatementOptimize::Element const &elem)
    -> std::unique_ptr<clingo_ast_t>;
auto make_ast(Owner const &owner, Gringo::Input::StatementEdge::Edge const &edge) -> std::unique_ptr<clingo_ast_t>;

template <class T> auto make_ast_vec(Owner const &owner, tcb::span<T> vec) -> ASTVec;
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
    std::transform(array, array + size, std::back_inserter(ret), [lib](auto str) { return lib->store->string(str); });
    return ret;
}

auto make_loc(Gringo::Input::Location const &loc) -> clingo_location_t {
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

template <class T> auto make_ast_vec(Owner const &owner, tcb::span<T> vec) -> ASTVec {
    ASTVec res{vec.size()};
    size_t i = 0;
    for (auto const &elem : vec) {
        res[i] = make_ast(owner, elem).release();
        ++i;
    }
    return res;
}

template <class T> auto make_ast_vec(Owner const &owner, std::vector<T> const &vec) -> ASTVec {
    return make_ast_vec(owner, tcb::make_span(vec));
}

template <class T> auto make_ast_vec(Owner const &owner, Gringo::Util::immutable_array<T> const &vec) -> ASTVec {
    return make_ast_vec(owner, tcb::make_span(vec));
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

auto make_ast(Owner const &owner, Gringo::Input::ArgumentTuple const &tuple) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_argument_tuple, static_cast<void const *>(&tuple));
}

auto make_ast(Owner const &owner, Gringo::Input::TermTuple::Element const &elem) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner](auto const &x) {
            GRINGO_MATCH(x, Gringo::Input::Term) { return make_ast(owner, x); }
            GRINGO_MATCH(x, Gringo::Input::ArgumentTuple) {
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

auto make_ast(Owner const &owner, Gringo::Input::HeadLiteral const &lit) -> std::unique_ptr<clingo_ast_t> {
    using namespace Gringo::Input;
    return std::visit(
        [&owner](auto &x) {
            GRINGO_MATCH(x, SimpleHeadLiteral) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_head_simple_literal, &x);
            }
            GRINGO_MATCH(x, Disjunction) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_head_disjunction, &x);
            }
            GRINGO_MATCH(x, HeadSetAggregate) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_head_set_aggregate, &x);
            }
            GRINGO_MATCH(x, HeadTheoryAtom) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_head_theory_atom, &x);
            }
            GRINGO_MATCH(x, HeadAggregate) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_head_aggregate, &x);
            }
        },
        lit);
}

auto make_ast(Owner const &owner, Gringo::Input::BodyLiteral const &lit) -> std::unique_ptr<clingo_ast_t> {
    using namespace Gringo::Input;
    return std::visit(
        [&owner](auto &x) {
            GRINGO_MATCH(x, SimpleBodyLiteral) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_body_simple_literal, &x);
            }
            GRINGO_MATCH(x, Conjunction) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_body_conditional_literal, &x);
            }
            GRINGO_MATCH(x, BodySetAggregate) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_body_set_aggregate, &x);
            }
            GRINGO_MATCH(x, BodyTheoryAtom) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_body_theory_atom, &x);
            }
            GRINGO_MATCH(x, BodyAggregate) {
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

auto make_ast(Owner const &owner, Gringo::Input::StatementOptimize::Tuple const &tuple)
    -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_optimize_tuple, &tuple);
}

auto make_ast(Owner const &owner, Gringo::Input::StatementOptimize::Element const &elem)
    -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_optimize_element, &elem);
}

auto make_ast(Owner const &owner, Gringo::Input::StatementEdge::Edge const &edge) -> std::unique_ptr<clingo_ast_t> {
    return std::make_unique<clingo_ast>(owner, clingo_ast_type_edge, &edge);
}

auto make_ast(Owner const &owner, Gringo::Input::Statement const &term) -> std::unique_ptr<clingo_ast_t> {
    return std::visit(
        [&owner](auto const &x) {
            GRINGO_MATCH(x, Gringo::Input::Rule) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_rule, &x);
            }
            // TODO...
            GRINGO_MATCH(x, Gringo::Input::TheoryDefinition) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_theory, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::StatementOptimize) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_optimize, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::StatementWeakConstraint) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_weak_constraint, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::StatementShow) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_show, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::StatementShowSig) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_show_signature, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::StatementProject) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_project, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::StatementProjectSig) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_project_signature, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::StatementDefined) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_defined, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::StatementExternal) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_external, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::StatementEdge) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_edge, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::StatementHeuristic) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_heuristic, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::StatementScript) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_script, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::StatementInclude) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_include, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::StatementProgram) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_program, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::StatementConst) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_const, &x);
            }
            GRINGO_MATCH(x, Gringo::Input::Comment) {
                return std::make_unique<clingo_ast>(owner, clingo_ast_type_statement_comment, &x);
            }
        },
        term);
}

template <class T, class... A> auto construct_ast(clingo_ast_type_t type, A &&...args) -> clingo_ast * {
    auto owner = Gringo::Util::make_immutable<std::any>(T{std::forward<A>(args)...});
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
            return std::invoke(std::move(visit), cast<ArgumentTuple>());
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
            return std::invoke(std::move(visit), cast<TheoryRGuard::value_type>());
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
        case clingo_ast_type_statement_rule: {
            return std::invoke(std::move(visit), cast<Rule>());
        }
        case clingo_ast_type_theory_operator_definition: {
            return std::invoke(std::move(visit), cast<TheoryOpDefinition>());
        }
        case clingo_ast_type_theory_term_definition: {
            return std::invoke(std::move(visit), cast<TheoryTermDefinition>());
        }
        case clingo_ast_type_theory_guard_definition: {
            return std::invoke(std::move(visit), cast<TheoryRGuardDefinition>());
        }
        case clingo_ast_type_theory_atom_definition: {
            return std::invoke(std::move(visit), cast<TheoryAtomDefinition>());
        }
        case clingo_ast_type_statement_theory: {
            return std::invoke(std::move(visit), cast<TheoryDefinition>());
        }
        case clingo_ast_type_optimize_tuple: {
            return std::invoke(std::move(visit), cast<StatementOptimize::Tuple>());
        }
        case clingo_ast_type_optimize_element: {
            return std::invoke(std::move(visit), cast<StatementOptimize::Element>());
        }
        case clingo_ast_type_statement_optimize: {
            return std::invoke(std::move(visit), cast<StatementOptimize>());
        }
        case clingo_ast_type_statement_weak_constraint: {
            return std::invoke(std::move(visit), cast<StatementWeakConstraint>());
        }
        case clingo_ast_type_edge: {
            return std::invoke(std::move(visit), cast<StatementEdge::Edge>());
        }
        case clingo_ast_type_statement_show: {
            return std::invoke(std::move(visit), cast<StatementShow>());
        }
        case clingo_ast_type_statement_show_signature: {
            return std::invoke(std::move(visit), cast<StatementShowSig>());
        }
        case clingo_ast_type_statement_project: {
            return std::invoke(std::move(visit), cast<StatementProject>());
        }
        case clingo_ast_type_statement_project_signature: {
            return std::invoke(std::move(visit), cast<StatementProjectSig>());
        }
        case clingo_ast_type_statement_defined: {
            return std::invoke(std::move(visit), cast<StatementDefined>());
        }
        case clingo_ast_type_statement_external: {
            return std::invoke(std::move(visit), cast<StatementExternal>());
        }
        case clingo_ast_type_statement_edge: {
            return std::invoke(std::move(visit), cast<StatementEdge>());
        }
        case clingo_ast_type_statement_heuristic: {
            return std::invoke(std::move(visit), cast<StatementHeuristic>());
        }
        case clingo_ast_type_statement_include: {
            return std::invoke(std::move(visit), cast<StatementInclude>());
        }
        case clingo_ast_type_statement_program: {
            return std::invoke(std::move(visit), cast<StatementProgram>());
        }
        case clingo_ast_type_statement_script: {
            return std::invoke(std::move(visit), cast<StatementScript>());
        }
        case clingo_ast_type_statement_const: {
            return std::invoke(std::move(visit), cast<StatementConst>());
        }
        case clingo_ast_type_statement_comment: {
            return std::invoke(std::move(visit), cast<Comment>());
        }
    }
    throw std::invalid_argument("invalid ast type");
}

auto clingo_ast::get_type() const -> clingo_ast_type_e { return type_; }

auto clingo_ast::get_location(clingo_ast_attribute_t attr) const -> std::optional<clingo_location_t> {
    // Note: to indicate that make_loc is used
    static_cast<void>(&make_loc);
    using namespace Gringo::Input;
    if (attr != clingo_ast_attribute_location) {
        return std::nullopt;
    }
    return visit([](auto &x) -> std::optional<clingo_location_t> {
        if constexpr (Detail::has_loc<std::decay_t<decltype(x)>>) {
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
#define ATTR_OLD(attr, value)                                                                                          \
    case clingo_ast_attribute_##attr: {                                                                                \
        return static_cast<int>(cast<Type>().value);                                                                   \
    }

#define ATTR(attr, value)                                                                                              \
    case clingo_ast_attribute_##attr: {                                                                                \
        return static_cast<int>(cast<Type>().value());                                                                 \
    }

auto clingo_ast::get_number(clingo_ast_attribute_t attr) const -> std::optional<int> {
    // clang-format off
    SWITCH(
        TYPE(term_variable, TermVariable,
            ATTR(anonymous, is_anonymous))
        TYPE(theory_term_variable, TheoryTermVariable,
            ATTR_OLD(anonymous, is_anonymous_))
        TYPE(term_function, TermFunction,
            ATTR(external, external))
        TYPE(term_unary_operation, TermUnary,
            ATTR(operator_type, op))
        TYPE(term_binary_operation, TermBinary,
            ATTR(operator_type, op))
        TYPE(theory_term_tuple, TheoryTermTuple,
            ATTR_OLD(tuple_type, type_))
        TYPE(literal_boolean, LiteralBoolean,
            ATTR_OLD(sign, sign_)
            ATTR_OLD(value, value_))
        TYPE(literal_symbolic, LiteralSymbolic,
            ATTR_OLD(sign, sign_))
        TYPE(literal_comparison, LiteralRelation,
            ATTR_OLD(sign, sign_))
        TYPE(left_guard, LGuard::value_type,
            ATTR_OLD(relation, second))
        TYPE(right_guard, RGuard::value_type,
            ATTR_OLD(relation, first))
        TYPE(body_theory_atom, BodyTheoryAtom,
            ATTR_OLD(sign, sign_))
        TYPE(body_set_aggregate, BodySetAggregate,
            ATTR_OLD(sign, sign_))
        TYPE(head_aggregate, HeadAggregate,
            ATTR_OLD(function, fun_))
        TYPE(body_aggregate, BodyAggregate,
            ATTR_OLD(sign, sign_)
            ATTR_OLD(function, fun_))
        TYPE(theory_operator_definition, TheoryOpDefinition,
            ATTR_OLD(priority, prio_)
            ATTR_OLD(operator_type, type_))
        TYPE(theory_atom_definition, TheoryAtomDefinition,
            ATTR_OLD(arity, arity_)
            ATTR_OLD(atom_type, type_))
        TYPE(statement_optimize, StatementOptimize,
            ATTR_OLD(optimize_type, type_))
        TYPE(statement_show_signature, StatementShowSig,
            ATTR_OLD(sign, has_sign_)
            ATTR_OLD(arity, arity_))
        TYPE(statement_project_signature, StatementProjectSig,
            ATTR_OLD(sign, has_sign_)
            ATTR_OLD(arity, arity_))
        TYPE(statement_defined, StatementDefined,
            ATTR_OLD(sign, has_sign_)
            ATTR_OLD(arity, arity_))
        TYPE(statement_include, StatementInclude,
            ATTR_OLD(include_type, type_))
        TYPE(statement_const, StatementConst,
            ATTR_OLD(const_type, type_))
        TYPE(statement_comment, Comment,
            ATTR_OLD(comment_type, type_)))
    // clang-format on
}

#undef ATTR_OLD
#define ATTR_OLD(attr, value)                                                                                          \
    case clingo_ast_attribute_##attr: {                                                                                \
        return static_cast<clingo_symbol_t>(Gringo::Symbol::to_rep(cast<Type>().value));                               \
    }
#undef ATTR
#define ATTR(attr, value)                                                                                              \
    case clingo_ast_attribute_##attr: {                                                                                \
        return static_cast<clingo_symbol_t>(Gringo::Symbol::to_rep(cast<Type>().value()));                             \
    }

[[nodiscard]] auto clingo_ast::get_symbol(clingo_ast_attribute_t attr) const -> std::optional<clingo_symbol_t> {
    // clang-format off
    SWITCH(
        TYPE(term_symbolic, TermSymbol,
            ATTR(symbol, value))
        TYPE(theory_term_symbolic, TheoryTermSymbol,
            ATTR_OLD(symbol, value_)))
    // clang-format on
}

#undef ATTR_OLD
#define ATTR_OLD(attr, value)                                                                                          \
    case clingo_ast_attribute_##attr: {                                                                                \
        return cast<Type>().value.c_str();                                                                             \
    }
#undef ATTR
#define ATTR(attr, value)                                                                                              \
    case clingo_ast_attribute_##attr: {                                                                                \
        return cast<Type>().value().c_str();                                                                           \
    }

auto clingo_ast::get_string(clingo_ast_attribute_t attr) const -> std::optional<char const *> {
    // clang-format off
    SWITCH(
        TYPE(term_variable, TermVariable,
            ATTR(name, name))
        TYPE(theory_term_variable, TheoryTermVariable,
            ATTR_OLD(name, name_))
        TYPE(term_function, TermFunction,
            ATTR(name, name))
        TYPE(theory_term_function, TheoryTermFunction,
            ATTR_OLD(name, name_))
        TYPE(theory_right_guard, TheoryRGuard::value_type,
            ATTR_OLD(theory_operator, first))
        TYPE(theory_operator_definition, TheoryOpDefinition,
            ATTR_OLD(name, op_))
        TYPE(theory_term_definition, TheoryTermDefinition,
            ATTR_OLD(name, name_))
        TYPE(theory_guard_definition, TheoryRGuardDefinition,
            ATTR_OLD(term, second))
        TYPE(theory_atom_definition, TheoryAtomDefinition,
            ATTR_OLD(name, name_)
            ATTR_OLD(term, term_))
        TYPE(statement_theory, TheoryDefinition,
            ATTR_OLD(name, name_))
        TYPE(statement_show_signature, StatementShowSig,
            ATTR_OLD(name, name_))
        TYPE(statement_project_signature, StatementProjectSig,
            ATTR_OLD(name, name_))
        TYPE(statement_defined, StatementDefined,
            ATTR_OLD(name, name_))
        TYPE(statement_include, StatementInclude,
            ATTR_OLD(value, path_))
        TYPE(statement_program, StatementProgram,
            ATTR_OLD(name, name_))
        TYPE(statement_script, StatementScript,
            ATTR_OLD(script_type, type_)
            ATTR_OLD(value, content_))
        TYPE(statement_const, StatementConst,
            ATTR_OLD(name, name_))
        TYPE(statement_comment, Comment,
            ATTR_OLD(value, value_)))
    // clang-format on
}

#undef ATTR_OLD
#define ATTR_OLD(attr, value)                                                                                          \
    case clingo_ast_attribute_##attr: {                                                                                \
        return tcb::make_span(cast<Type>().value);                                                                     \
    }

auto clingo_ast::get_string_vec(clingo_ast_attribute_t attr) const -> std::optional<tcb::span<Gringo::String const>> {
    // clang-format off
    SWITCH(
        TYPE(unparsed_element, TheoryTermUnparsed::Element,
            ATTR_OLD(operators, first))
        TYPE(theory_guard_definition, TheoryRGuardDefinition,
            ATTR_OLD(operators, first))
        TYPE(statement_program, StatementProgram,
            ATTR_OLD(arguments, args_)))
    // clang-format on
}

#undef ATTR_OLD
#define ATTR_OLD(attr, value)                                                                                          \
    case clingo_ast_attribute_##attr: {                                                                                \
        return make_ast(owner_, cast<Type>().value);                                                                   \
    }
#undef ATTR
#define ATTR(attr, value)                                                                                              \
    case clingo_ast_attribute_##attr: {                                                                                \
        return make_ast(owner_, cast<Type>().value());                                                                 \
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
            ATTR_OLD(term, second))
        TYPE(literal_comparison, LiteralRelation,
            ATTR_OLD(left, lhs_))
        TYPE(literal_symbolic, LiteralSymbolic,
            ATTR_OLD(atom, term_))
        TYPE(head_simple_literal, SimpleHeadLiteral,
            ATTR_OLD(literal, lit_))
        TYPE(body_simple_literal, SimpleBodyLiteral,
            ATTR_OLD(literal, lit_))
        TYPE(head_conditional_literal, ConditionalLiteral,
            ATTR_OLD(literal, lit_))
        TYPE(body_conditional_literal, Conjunction,
            ATTR_OLD(literal, lit_.lit_))
        TYPE(left_guard, LGuard::value_type,
            ATTR_OLD(term, first))
        TYPE(right_guard, RGuard::value_type,
            ATTR_OLD(term, second))
        TYPE(theory_right_guard, TheoryRGuard::value_type,
            ATTR_OLD(term, second))
        TYPE(set_aggregate_element, SetAggregateElement,
            ATTR_OLD(literal, lit_))
        TYPE(head_aggregate_element, HeadAggregate::Element,
            ATTR_OLD(literal, lit_))
        TYPE(body_theory_atom, BodyTheoryAtom,
            ATTR_OLD(name, name_)
            ATTR_OLD(right, rhs_))
        TYPE(head_theory_atom, HeadTheoryAtom,
            ATTR_OLD(name, name_)
            ATTR_OLD(right, rhs_))
        TYPE(head_set_aggregate, HeadSetAggregate,
            ATTR_OLD(left, lhs_)
            ATTR_OLD(right, rhs_))
        TYPE(head_aggregate, HeadAggregate,
            ATTR_OLD(left, lhs_)
            ATTR_OLD(right, rhs_))
        TYPE(body_set_aggregate, BodySetAggregate,
            ATTR_OLD(left, lhs_)
            ATTR_OLD(right, rhs_))
        TYPE(body_aggregate, BodyAggregate,
            ATTR_OLD(left, lhs_)
            ATTR_OLD(right, rhs_))
        TYPE(statement_rule, Rule,
            ATTR_OLD(head, head_))
        TYPE(theory_atom_definition, TheoryAtomDefinition,
            ATTR_OLD(guard, rhs_))
        TYPE(optimize_tuple, StatementOptimize::Tuple,
            ATTR_OLD(weight, weight_)
            ATTR_OLD(priority, priority_))
        TYPE(optimize_element, StatementOptimize::Element,
            ATTR_OLD(tuple, first))
        TYPE(statement_weak_constraint, StatementWeakConstraint,
            ATTR_OLD(tuple, tuple_))
        TYPE(statement_show, StatementShow,
            ATTR_OLD(term, term_))
        TYPE(statement_project, StatementProject,
            ATTR_OLD(atom, term_))
        TYPE(statement_external, StatementExternal,
            ATTR_OLD(atom, term_)
            ATTR_OLD(external_type, type_))
        TYPE(edge, StatementEdge::Edge,
            ATTR_OLD(u, u_)
            ATTR_OLD(v, v_))
        TYPE(statement_heuristic, StatementHeuristic,
            ATTR_OLD(atom, atom_)
            ATTR_OLD(weight, type_)
            ATTR_OLD(priority, prio_)
            ATTR_OLD(modifier, mod_))
        TYPE(statement_const, StatementConst,
            ATTR_OLD(value, value_)))
    // clang-format on
}

#undef ATTR_OLD
#define ATTR_OLD(attr, value)                                                                                          \
    case clingo_ast_attribute_##attr: {                                                                                \
        return make_ast_vec(owner_, cast<Type>().value);                                                               \
    }
#undef ATTR
#define ATTR(attr, value)                                                                                              \
    case clingo_ast_attribute_##attr: {                                                                                \
        return make_ast_vec(owner_, cast<Type>().value());                                                             \
    }

auto clingo_ast::get_ast_vec(clingo_ast_attribute_t attr) const -> std::optional<ASTVec> {
    // clang-format off
    SWITCH(
        TYPE(argument_tuple, ArgumentTuple,
            ATTR(arguments, elems))
        TYPE(term_absolute, TermAbs,
            ATTR(pool, pool))
        TYPE(term_tuple, TermTuple,
            ATTR(pool, pool))
        TYPE(term_function, TermFunction,
            ATTR(pool, pool))
        TYPE(theory_term_tuple, TheoryTermTuple,
            ATTR_OLD(arguments, elems_))
        TYPE(theory_term_function, TheoryTermFunction,
            ATTR_OLD(arguments, args_))
        TYPE(theory_term_unparsed, TheoryTermUnparsed,
            ATTR_OLD(elements, elems_))
        TYPE(literal_comparison, LiteralRelation,
            ATTR_OLD(right, rhs_))
        TYPE(head_conditional_literal, ConditionalLiteral,
            ATTR_OLD(condition, cond_))
        TYPE(body_conditional_literal, Conjunction,
            ATTR_OLD(condition, lit_.cond_))
        TYPE(set_aggregate_element, SetAggregateElement,
            ATTR_OLD(condition, cond_))
        TYPE(theory_atom_element, TheoryElement,
            ATTR_OLD(tuple, tuple_)
            ATTR_OLD(condition, cond_))
        TYPE(head_aggregate_element, HeadAggregate::Element,
            ATTR_OLD(tuple, tuple_)
            ATTR_OLD(condition, cond_))
        TYPE(body_aggregate_element, BodyAggregate::Element,
            ATTR_OLD(tuple, tuple_)
            ATTR_OLD(condition, cond_))
        TYPE(head_disjunction, Disjunction,
            ATTR_OLD(elements, elems_))
        TYPE(body_theory_atom, BodyTheoryAtom,
            ATTR_OLD(elements, elems_))
        TYPE(head_theory_atom, HeadTheoryAtom,
            ATTR_OLD(elements, elems_))
        TYPE(head_set_aggregate, HeadSetAggregate,
            ATTR_OLD(elements, elems_))
        TYPE(head_aggregate, HeadAggregate,
            ATTR_OLD(elements, elems_))
        TYPE(body_set_aggregate, BodySetAggregate,
            ATTR_OLD(elements, elems_))
        TYPE(body_aggregate, BodyAggregate,
            ATTR_OLD(elements, elems_))
        TYPE(statement_rule, Rule,
            ATTR_OLD(body, body_))
        TYPE(theory_term_definition, TheoryTermDefinition,
            ATTR_OLD(operators, op_defs_))
        TYPE(statement_theory, TheoryDefinition,
            ATTR_OLD(terms, term_defs_)
            ATTR_OLD(atoms, atom_defs_))
        TYPE(optimize_tuple, StatementOptimize::Tuple,
            ATTR_OLD(terms, terms_))
        TYPE(optimize_element, StatementOptimize::Element,
            ATTR_OLD(condition, second))
        TYPE(statement_optimize, StatementOptimize,
            ATTR_OLD(elements, elems_))
        TYPE(statement_weak_constraint, StatementWeakConstraint,
            ATTR_OLD(body, body_))
        TYPE(statement_show, StatementShow,
            ATTR_OLD(body, body_))
        TYPE(statement_project, StatementProject,
            ATTR_OLD(body, body_))
        TYPE(statement_external, StatementExternal,
            ATTR_OLD(body, body_))
        TYPE(statement_edge, StatementEdge,
            ATTR_OLD(pool, edges_)
            ATTR_OLD(body, body_))
        TYPE(statement_heuristic, StatementHeuristic,
            ATTR_OLD(body, body_)))
    // clang-format on
}

#undef ATTR_OLD
#undef TYPE
#undef SWITCH

auto clingo_ast::copy() const -> std::unique_ptr<clingo_ast_t> { return std::make_unique<clingo_ast>(*this); }

void clingo_ast::print(std::ostream &out) const {
    using namespace Gringo::Input;
    visit([&out](auto &x) {
        GRINGO_MATCH(x, TheoryRGuard::value_type) { out << " " << x.first << " " << x.second; }
        else GRINGO_MATCH(x, TheoryTermUnparsed::Element) {
            for (auto const &op : x.first) {
                out << op << " ";
            }
            out << x.second;
        }
        else GRINGO_MATCH(x, ArgumentTuple) {
            bool comma = false;
            for (auto const &elem : x.elems()) {
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

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::ArgumentTuple::Element>() const
    -> Gringo::Input::ArgumentTuple::Element {
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

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::TermTuple::Element>() const -> Gringo::Input::TermTuple::Element {
    if (type_ == clingo_ast_type_argument_tuple) {
        return cast<Gringo::Input::ArgumentTuple>();
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

template <> [[nodiscard]] auto clingo_ast::convert<Gringo::Input::HeadLiteral>() const -> Gringo::Input::HeadLiteral {
    switch (type_) {
        case clingo_ast_type_head_simple_literal: {
            return cast<Gringo::Input::SimpleHeadLiteral>();
        }
        case clingo_ast_type_head_disjunction: {
            return cast<Gringo::Input::Disjunction>();
        }
        case clingo_ast_type_head_theory_atom: {
            return cast<Gringo::Input::HeadTheoryAtom>();
        }
        case clingo_ast_type_head_set_aggregate: {
            return cast<Gringo::Input::HeadSetAggregate>();
        }
        case clingo_ast_type_head_aggregate: {
            return cast<Gringo::Input::HeadAggregate>();
        }
        default: {
            throw std::invalid_argument("head literal expected");
        }
    }
}

template <> [[nodiscard]] auto clingo_ast::convert<Gringo::Input::BodyLiteral>() const -> Gringo::Input::BodyLiteral {
    switch (type_) {
        case clingo_ast_type_body_simple_literal: {
            return cast<Gringo::Input::SimpleBodyLiteral>();
        }
        case clingo_ast_type_body_conditional_literal: {
            return cast<Gringo::Input::Conjunction>();
        }
        case clingo_ast_type_body_theory_atom: {
            return cast<Gringo::Input::BodyTheoryAtom>();
        }
        case clingo_ast_type_body_set_aggregate: {
            return cast<Gringo::Input::BodySetAggregate>();
        }
        case clingo_ast_type_body_aggregate: {
            return cast<Gringo::Input::BodyAggregate>();
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
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::StatementOptimize::Tuple>() const
    -> Gringo::Input::StatementOptimize::Tuple {
    if (type_ == clingo_ast_type_optimize_tuple) {
        return cast<Gringo::Input::StatementOptimize::Tuple>();
    }
    throw std::runtime_error("optimize tuple expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::StatementOptimize::Element>() const
    -> Gringo::Input::StatementOptimize::Element {
    if (type_ == clingo_ast_type_optimize_element) {
        return cast<Gringo::Input::StatementOptimize::Element>();
    }
    throw std::runtime_error("optimize tuple expected");
}

template <>
[[nodiscard]] auto clingo_ast::convert<Gringo::Input::StatementEdge::Edge>() const
    -> Gringo::Input::StatementEdge::Edge {
    if (type_ == clingo_ast_type_edge) {
        return cast<Gringo::Input::StatementEdge::Edge>();
    }
    throw std::runtime_error("edge expected");
}

template <> [[nodiscard]] auto clingo_ast::convert<Gringo::Input::Statement>() const -> Gringo::Input::Statement {
    switch (type_) {
        case clingo_ast_type_statement_rule: {
            return cast<Gringo::Input::Rule>();
        }
        case clingo_ast_type_statement_theory: {
            return cast<Gringo::Input::TheoryDefinition>();
        }
        case clingo_ast_type_statement_optimize: {
            return cast<Gringo::Input::StatementOptimize>();
        }
        case clingo_ast_type_statement_weak_constraint: {
            return cast<Gringo::Input::StatementWeakConstraint>();
        }
        case clingo_ast_type_statement_show: {
            return cast<Gringo::Input::StatementShow>();
        }
        case clingo_ast_type_statement_show_signature: {
            return cast<Gringo::Input::StatementShowSig>();
        }
        case clingo_ast_type_statement_defined: {
            return cast<Gringo::Input::StatementDefined>();
        }
        case clingo_ast_type_statement_external: {
            return cast<Gringo::Input::StatementExternal>();
        }
        case clingo_ast_type_statement_edge: {
            return cast<Gringo::Input::StatementEdge>();
        }
        case clingo_ast_type_statement_heuristic: {
            return cast<Gringo::Input::StatementHeuristic>();
        }
        case clingo_ast_type_statement_script: {
            return cast<Gringo::Input::StatementScript>();
        }
        case clingo_ast_type_statement_program: {
            return cast<Gringo::Input::StatementProgram>();
        }
        case clingo_ast_type_statement_include: {
            return cast<Gringo::Input::StatementInclude>();
        }
        case clingo_ast_type_statement_const: {
            return cast<Gringo::Input::StatementConst>();
        }
        case clingo_ast_type_statement_comment: {
            return cast<Gringo::Input::Comment>();
        }
        default: {
            throw std::runtime_error("statement expected");
        }
    }
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
                    type, convert_ast_vec<Gringo::Input::ArgumentTuple::Element>(tuple, size));
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
            case clingo_ast_type_statement_rule: {
                std::va_list args;
                va_start(args, ast);
                auto const *loc = va_arg(args, clingo_location_t const *);
                auto const *head = va_arg(args, clingo_ast_t const *);
                auto const **body = va_arg(args, clingo_ast_t const **);
                auto body_size = va_arg(args, size_t);
                va_end(args);
                *ast = construct_ast<Gringo::Input::Rule>(type, convert_loc(lib, loc), head->convert<HeadLiteral>(),
                                                          convert_ast_vec<BodyLiteral>(body, body_size));
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
                *ast = construct_ast<Gringo::Input::TheoryDefinition>(
                    type, convert_loc(lib, loc), lib->store->string(name),
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
                *ast = construct_ast<Gringo::Input::StatementOptimize::Tuple>(type, weight->convert<Term>(),
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
                *ast = construct_ast<Gringo::Input::StatementOptimize::Element>(
                    type, tuple->convert<StatementOptimize::Tuple>(), convert_ast_vec<Literal>(cond, cond_size));
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
                *ast = construct_ast<Gringo::Input::StatementOptimize>(
                    type, convert_loc(lib, loc), static_cast<OptimizeType>(optimize_type),
                    convert_ast_vec<StatementOptimize::Element>(elems, elems_size));
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
                *ast = construct_ast<Gringo::Input::StatementWeakConstraint>(
                    type, convert_loc(lib, loc), convert_ast_vec<BodyLiteral>(body, body_size),
                    tuple->convert<StatementOptimize::Tuple>());
                break;
            }
            case clingo_ast_type_edge: {
                std::va_list args;
                va_start(args, ast);
                auto const *u = va_arg(args, clingo_ast_t const *);
                auto const *v = va_arg(args, clingo_ast_t const *);
                va_end(args);
                *ast = construct_ast<Gringo::Input::StatementEdge::Edge>(type, u->convert<Term>(), v->convert<Term>());
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
                *ast = construct_ast<Gringo::Input::StatementShow>(type, convert_loc(lib, loc), term->convert<Term>(),
                                                                   convert_ast_vec<BodyLiteral>(body, body_size));
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
                *ast = construct_ast<Gringo::Input::StatementShowSig>(type, convert_loc(lib, loc), sign != 0,
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
                *ast = construct_ast<Gringo::Input::StatementProject>(
                    type, convert_loc(lib, loc), atom->convert<Term>(), convert_ast_vec<BodyLiteral>(body, body_size));
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
                *ast = construct_ast<Gringo::Input::StatementProjectSig>(type, convert_loc(lib, loc), sign != 0,
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
                *ast = construct_ast<Gringo::Input::StatementDefined>(type, convert_loc(lib, loc), sign != 0,
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
                *ast = construct_ast<Gringo::Input::StatementExternal>(
                    type, convert_loc(lib, loc), atom->convert<Term>(), convert_ast_vec<BodyLiteral>(body, body_size),
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
                *ast = construct_ast<Gringo::Input::StatementEdge>(
                    type, convert_loc(lib, loc), convert_ast_vec<StatementEdge::Edge>(edges, edges_size),
                    convert_ast_vec<BodyLiteral>(body, body_size));
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
                *ast = construct_ast<Gringo::Input::StatementHeuristic>(
                    type, convert_loc(lib, loc), atom->convert<Term>(), convert_ast_vec<BodyLiteral>(body, body_size),
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
                *ast = construct_ast<Gringo::Input::StatementInclude>(type, convert_loc(lib, loc),
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
                *ast = construct_ast<Gringo::Input::StatementProgram>(
                    type, convert_loc(lib, loc), lib->store->string(name),
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
                *ast = construct_ast<Gringo::Input::StatementScript>(type, convert_loc(lib, loc),
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
                *ast = construct_ast<Gringo::Input::StatementConst>(type, convert_loc(lib, loc),
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
                *ast = construct_ast<Gringo::Input::Comment>(type, convert_loc(lib, loc),
                                                             static_cast<CommentType>(comment_type), value);
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
                auto owner = Gringo::Util::make_immutable<std::any>(std::move(term).value());
                auto const *ptr = std::any_cast<Gringo::Input::Term>(owner.get());
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
                auto const *ptr = std::any_cast<Gringo::Input::TheoryTerm>(owner.get());
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
                auto const *ptr = std::any_cast<Gringo::Input::Literal>(owner.get());
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
                auto const *ptr = std::any_cast<Gringo::Input::HeadLiteral>(owner.get());
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
                auto const *ptr = std::any_cast<Gringo::Input::BodyLiteral>(owner.get());
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
                auto const *ptr = std::any_cast<Gringo::Input::Statement>(owner.get());
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
                auto const *ptr = std::any_cast<Gringo::Input::Statement>(owner.get());
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
        auto span = tcb::span(files, size);
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
        delete scanner;
    }
}

extern "C" auto clingo_ast_rewrite(clingo_lib_t *lib, clingo_ast_t *statement,
                                   clingo_ast_rewrite_options_t const *options, char const **parameters,
                                   size_t parameters_size, clingo_ast_t ***result, size_t *result_size) -> bool {
    CLINGO_TRY {
        *result = nullptr;
        *result_size = 0;
        using namespace Gringo::Input;
        auto stms = StatementVec{};
        auto stm = statement->convert<Statement>();
        auto param_map = ParamMap{};
        param_map.reserve(parameters_size);
        std::for_each_n(parameters, parameters_size,
                        [&param_map, lib](auto const *str) { param_map.emplace(lib->store->string(str)); });
        RewriteOptions opts{static_cast<ProjectionMode>(options->project_mode), options->project_anonymous};
        auto const_map = ConstMap{};
        Gringo::Util::ordered_map<Gringo::String, Gringo::String> pum;
        size_t i = 0;
        for (auto const &id : param_map) {
            auto var = lib->store->string("$" + std::to_string(i));
            pum.emplace(var, id);
            ++i;
        }
        rewrite(lib->log, *lib->store, param_map, const_map, stm, opts, stms);
        if (lib->log.has_error()) {
            lib->log.reset();
            throw std::runtime_error("rewriting statement failed");
        }
        ASTVec res{stms.size()};
        i = 0;
        for (auto &stm : stms) {
            if (auto res_stm = unmap_params(*lib->store, pum, stm); res_stm) {
                stm = *std::move(res_stm);
            }
            auto owner = Gringo::Util::make_immutable<std::any>(std::move(stm));
            auto const *ptr = std::any_cast<Gringo::Input::Statement>(owner.get());
            res[i] = make_ast(owner, *ptr).release();
            ++i;
        }
        std::tie(*result, *result_size) = res.release();
    }
    CLINGO_CATCH(lib);
}
