#include <clingo.h>

#include "core.hh"

namespace Clingo::AST2 {

namespace py = pybind11;

template <class T> struct ast_type_info;

enum class Type : uint8_t { record, variant, location, string, number, optional, array, enum_, symbol };

// TODO: optional, array, variant, and enum_ are not yet handled

template <clingo_ast_type_e T> class Node {
  public:
    // Note: for pybind
    Node() = default;

    explicit Node(clingo_ast_t *ast) : ast_{ast} {}

    Node(Node const &x) { Core::handle_error(clingo_ast_copy(x.ast_, &ast_)); }

    template <class... U>
    Node(Core::Library &lib, U &&...args)
        : ast_{construct_(std::forward<U>(args)..., static_cast<clingo_lib_t *>(lib))} {}

    Node(Node &&x) noexcept { std::swap(ast_, x.ast_); }

    ~Node() { clingo_ast_free(ast_); }

    // NOLINTNEXTLINE(bugprone-unhandled-self-assignment)
    auto operator=(Node const &x) -> Node & {
        if (ast_ != x.ast_) {
            clingo_ast_free(ast_);
            ast_ = nullptr;
            Core::handle_error(clingo_ast_copy(x.ast_, &ast_));
        }
        return *this;
    }

    auto operator=(Node &&x) noexcept -> Node & {
        std::swap(ast_, x.ast_);
        return *this;
    }

    [[nodiscard]] auto hash() const -> size_t { return clingo_ast_hash(ast_); }

    friend auto operator==(Node const &a, Node const &b) -> bool { return clingo_ast_equal(a.ast_, b.ast_); }

    friend auto operator<=>(Node const &a, Node const &b) -> std::strong_ordering {
        return clingo_ast_compare(a.ast_, b.ast_) <=> 0;
    }

    auto to_string() -> std::string {
        size_t len = 0;
        Core::handle_error(clingo_ast_to_string_size(ast_, &len));
        std::string str;
        str.resize(len);
        Core::handle_error(clingo_ast_to_string(ast_, str.data(), len));
        if (!str.empty() && str.back() == '\0') {
            str.pop_back();
        }
        return str;
    }

    template <clingo_ast_attribute_e name, class U> auto get() -> decltype(auto) { return get_<name, U>(); }

    template <size_t i> auto get() -> decltype(auto) { return get<names[i], std::tuple_element_t<i, arguments>>(); }

    template <size_t i> static constexpr auto attr_name() -> char const * { return strings[i]; }

    static constexpr auto init_doc() -> char const * { return ast_type_info<Node>::doc; }

    template <size_t i> static constexpr auto attr_doc() -> char const * { return ast_type_info<Node>::docs[i]; }

    void visit(py::handle visitor, py::args const &args, py::kwargs const &kwargs) { visit_<0>(visitor, args, kwargs); }

    auto update(Core::Library &lib, py::kwargs const &kwargs) -> Node {
        return [&]<size_t... I>([[maybe_unused]] std::index_sequence<I...> seq) {
            return Node{lib, update_value_<I>(kwargs)...};
        }(std::make_index_sequence<std::tuple_size_v<arguments>>{});
    }

    auto transform(Core::Library &lib, py::handle transform, py::args const &args,
                   py::kwargs const &kwargs) -> std::optional<Node> {
        return [&]<size_t... I>([[maybe_unused]] std::index_sequence<I...> seq) {
            return transform_<I...>(lib, transform_value_<I>(transform, args, kwargs)...);
        }(std::make_index_sequence<std::tuple_size_v<arguments>>{});
    }

    friend auto c_cast(Node const &x) -> clingo_ast_t *;

  private:
    using arguments = ast_type_info<Node>::arguments;
    static constexpr auto names = ast_type_info<Node>::names;
    static constexpr auto strings = ast_type_info<Node>::strings;

    // getters

    template <clingo_ast_attribute_e name, class U>
        requires(ast_type_info<U>::type == Type::location)
    auto get_() -> U {
        clingo_location_t const *ret = nullptr;
        Core::handle_error(clingo_ast_attribute_get_location(ast_, name, &ret));
        return Core::Location{ret};
    }

    template <clingo_ast_attribute_e name, class U>
        requires(ast_type_info<U>::type == Type::string)
    auto get_() -> U {
        char const *ret = nullptr;
        Core::handle_error(clingo_ast_attribute_get_string(ast_, name, &ret));
        return ret;
    }

    template <clingo_ast_attribute_e name, class U>
        requires(ast_type_info<U>::type == Type::number)
    auto get_() -> U {
        int ret = 0;
        Core::handle_error(clingo_ast_attribute_get_number(ast_, clingo_ast_attribute_anonymous, &ret));
        return static_cast<U>(ret);
    }

    template <clingo_ast_attribute_e name, class U>
        requires(ast_type_info<U>::type == Type::record)
    auto get_() -> U {
        clingo_ast_t *ast = nullptr;
        Core::handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_term, &ast));
        return U{ast};
    }

    template <clingo_ast_attribute_e name, class U>
        requires(ast_type_info<U>::type == Type::variant)
    auto get_() -> U {
        clingo_ast_t *ast = nullptr;
        Core::handle_error(clingo_ast_attribute_get_ast(ast_, clingo_ast_attribute_term, &ast));
        clingo_ast_type_t type = 0;
        if (clingo_ast_get_type(ast, &type) != clingo_result_success) {
            clingo_ast_free(ast);
            throw std::runtime_error("could not get type");
        }
        return get_alt_<U, 0>(type, ast);
    }

    template <class U, size_t i> static auto get_alt_(clingo_ast_type_t type, clingo_ast_t *ast) -> U {
        if constexpr (i < std::variant_size_v<U>) {
            using A = std::variant_alternative_t<i, U>;
            if (ast_type_info<A>::ast_type == type) {
                return A{ast};
            }
            return get_alt_<U, i + 1>(type, ast);
        } else {
            clingo_ast_free(ast);
            throw std::runtime_error("unexpected ast type");
        }
    };

    // conversion from C++ to C

    static auto convert_(Core::Location const &loc) -> clingo_location_t const * {
        return static_cast<clingo_location_t const *>(loc);
    }

    static auto convert_(std::string const &str) -> char const * { return str.c_str(); }

    static auto convert_(char const *str) -> char const * { return str; }

    static auto convert_(bool b) -> int { return static_cast<int>(b); }

    // construction

    template <class... U> static auto construct_(clingo_lib_t *lib, U &&...args) -> clingo_ast_t * {
        clingo_ast_t *ast = nullptr;
        Core::handle_error(
            clingo_ast_construct(lib, static_cast<clingo_ast_type_t>(T), &ast, std::forward<U>(args)...));
        return ast;
    }

    template <class U, class... Us> static auto construct_(U &&arg, Us &&...args) -> clingo_ast_t * {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        return construct_(std::forward<Us>(args)..., convert_(std::forward<U>(arg)));
    }

    // visit

    template <size_t i> void visit_(py::handle &visitor, py::args const &args, py::kwargs const &kwargs) {
        if constexpr (i < std::tuple_size_v<arguments>) {
            if constexpr (ast_type_info<std::tuple_element_t<i, arguments>>::type == Type::record) {
                visitor(get<i>(), *args, **kwargs);
            }
            visit_<i + 1>(visitor, args, kwargs);
        }
    }

    // update

    template <size_t i> auto update_value_(py::kwargs const &kwargs) {
        static constexpr char const *attr = strings[i];
        using E = std::tuple_element_t<i, arguments>;
        if constexpr (std::is_same_v<E, char const *>) {
            if (kwargs.contains(attr)) {
                return py::cast<std::string>(kwargs[attr]);
            }
            return std::string{get<i>()};
        } else {
            if (kwargs.contains(attr)) {
                return py::cast<E>(kwargs[attr]);
            }
            return get<i>();
        }
    }

    // transform

    template <size_t i, class U> auto transformed_value_(U opt) {
        if (opt) {
            return *std::move(opt);
        }
        // TODO: maybe needs conversion for string
        return get<i>();
    }

    template <size_t... I, class... U> auto transform_(Core::Library &lib, U &&...args) -> std::optional<Node> {
        if ((false || ... || args.has_value())) {
            return std::make_optional<Node>(lib, transformed_value_<I>(std::forward<U>(args))...);
        }
        return std::nullopt;
    }

    template <size_t i> auto transform_value_(py::handle &transform, py::args const &args, py::kwargs const &kwargs) {
        using E = std::tuple_element_t<i, arguments>;
        if constexpr (ast_type_info<E>::type == Type::record) {
            if (auto val = transform(get<i>(), *args, **kwargs); !val.is_none()) {
                if constexpr (std::is_same_v<E, char const *>) {
                    return std::make_optional(py::cast<std::string>(val));
                } else {
                    return std::make_optional(py::cast<E>(val));
                }
            }
        }
        return std::optional<E>{};
    }

    clingo_ast_t *ast_ = nullptr;
};

template <clingo_ast_type_e T> inline auto c_cast(Node<T> const &x) -> clingo_ast_t * { return x.ast_; }

} // namespace Clingo::AST2
