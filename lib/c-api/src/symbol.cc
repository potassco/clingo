#include "core.hh"
#include "lib.hh"

#include <clingo/input/parser.hh>

#include <clingo/input/rewrite/evaluate.hh>

using namespace CppClingo::CAPI;

extern "C" void clingo_symbol_acquire(clingo_symbol_t symbol) {
    if (symbol != 0) {
        CppClingo::Symbol::from_rep(symbol).acquire();
    }
}

extern "C" void clingo_symbol_release(clingo_symbol_t symbol) {
    if (symbol != 0) {
        CppClingo::Symbol::from_rep(symbol).release();
    }
}

extern "C" auto clingo_symbol_create_infimum() -> clingo_symbol_t {
    // NOTE: does not need reference counting
    return CppClingo::Symbol::to_rep(CppClingo::SymbolStore::inf());
}

extern "C" auto clingo_symbol_create_supremum() -> clingo_symbol_t {
    // NOTE: does not need reference counting
    return CppClingo::Symbol::to_rep(CppClingo::SymbolStore::sup());
}

extern "C" auto clingo_symbol_create_number(int32_t number) -> clingo_symbol_t {
    // NOTE: does not need reference counting
    return CppClingo::Symbol::to_rep(CppClingo::SymbolStore::num_ref(number));
}

extern "C" auto clingo_symbol_create_number_str(clingo_lib_t *lib, char const *number, size_t size,
                                                clingo_symbol_t *symbol) -> bool {
    CLINGO_TRY {
        require(not_null(lib, symbol) && valid_span(number, size));
        *symbol = CppClingo::SharedSymbol::to_rep(lib->store->num(CppClingo::Number{std::string_view{number, size}}));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_symbol_create_string(clingo_lib_t *lib, char const *string, size_t size, clingo_symbol_t *symbol)
    -> bool {
    CLINGO_TRY {
        require(not_null(lib, symbol) && valid_span(string, size));
        *symbol = CppClingo::SharedSymbol::to_rep(CppClingo::SymbolStore::str(*lib->store->string({string, size})));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_symbol_create_id(clingo_lib_t *lib, char const *name, size_t size, bool is_positive,
                                        clingo_symbol_t *symbol) -> bool {
    CLINGO_TRY {
        require(not_null(lib, symbol) && valid_span(name, size));
        *symbol = CppClingo::SharedSymbol::to_rep(lib->store->fun(lib->store->string({name, size}), {}, !is_positive));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_symbol_create_tuple(clingo_lib_t *lib, clingo_symbol_t const *arguments, size_t arguments_size,
                                           clingo_symbol_t *symbol) -> bool {
    CLINGO_TRY {
        require(not_null(lib, symbol) && valid_span(arguments, arguments_size));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto const *c_args = reinterpret_cast<CppClingo::Symbol const *>(arguments);
        *symbol = CppClingo::SharedSymbol::to_rep(lib->store->tup(CppClingo::SymbolSpan{c_args, arguments_size}));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_symbol_create_function(clingo_lib_t *lib, char const *name, size_t name_size,
                                              clingo_symbol_t const *arguments, size_t arguments_size, bool is_positive,
                                              clingo_symbol_t *symbol) -> bool {
    CLINGO_TRY {
        require(not_null(lib, symbol) && valid_span(name, name_size) && valid_span(arguments, arguments_size));
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto const *c_args = reinterpret_cast<CppClingo::Symbol const *>(arguments);
        *symbol = CppClingo::SharedSymbol::to_rep(lib->store->fun(
            *lib->store->string({name, name_size}), CppClingo::SymbolSpan{c_args, arguments_size}, !is_positive));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_symbol_number(clingo_symbol_t symbol, int32_t *number) -> bool {
    CLINGO_TRY {
        auto sym = CppClingo::Symbol::from_rep(symbol);
        require(number != nullptr && sym.type() == CppClingo::SymbolType::number);
        auto num = sym.num().as_int();
        if (!num.has_value()) {
            return fail_with(clingo_result_range, "number out of range");
        }
        *number = *num;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_symbol_name(clingo_symbol_t symbol, clingo_string_t *name) -> bool {
    CLINGO_TRY {
        auto sym = CppClingo::Symbol::from_rep(symbol);
        require(name != nullptr && sym.type() == CppClingo::SymbolType::function);
        auto res = sym.name().view();
        name->data = res.data();
        name->size = res.size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_symbol_string(clingo_symbol_t symbol, clingo_string_t *string) -> bool {
    CLINGO_TRY {
        auto sym = CppClingo::Symbol::from_rep(symbol);
        require(string != nullptr && sym.type() == CppClingo::SymbolType::string);
        auto res = sym.str().view();
        string->data = res.data();
        string->size = res.size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_symbol_is_positive(clingo_symbol_t symbol, bool *is_positive) -> bool {
    CLINGO_TRY {
        auto sym = CppClingo::Symbol::from_rep(symbol);
        require(is_positive != nullptr &&
                (sym.type() == CppClingo::SymbolType::function || sym.type() == CppClingo::SymbolType::number));
        *is_positive = !sym.has_sign();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_symbol_arguments(clingo_symbol_t symbol, clingo_symbol_t const **arguments,
                                        size_t *arguments_size) -> bool {
    CLINGO_TRY {
        auto sym = CppClingo::Symbol::from_rep(symbol);
        require(sym.type() == CppClingo::SymbolType::function || sym.type() == CppClingo::SymbolType::tuple);
        auto args = sym.args();
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        set_opt(arguments, reinterpret_cast<clingo_symbol_t const *>(args.data()));
        set_opt(arguments_size, args.size());
    }
    CLINGO_CATCH;
}

static_assert(static_cast<int>(CppClingo::SymbolType::function) == clingo_symbol_type_function);
static_assert(static_cast<int>(CppClingo::SymbolType::inf) == clingo_symbol_type_infimum);
static_assert(static_cast<int>(CppClingo::SymbolType::sup) == clingo_symbol_type_supremum);
static_assert(static_cast<int>(CppClingo::SymbolType::number) == clingo_symbol_type_number);
static_assert(static_cast<int>(CppClingo::SymbolType::tuple) == clingo_symbol_type_tuple);
static_assert(static_cast<int>(CppClingo::SymbolType::string) == clingo_symbol_type_string);

extern "C" auto clingo_symbol_type(clingo_symbol_t symbol) -> clingo_symbol_type_t {
    auto sym = CppClingo::Symbol::from_rep(symbol);
    return static_cast<clingo_symbol_type_t>(sym.type());
}

extern "C" auto clingo_symbol_to_string(clingo_symbol_t symbol, clingo_string_builder_t *builder) -> bool {
    CLINGO_TRY {
        require(builder != nullptr);
        auto sym = CppClingo::Symbol::from_rep(symbol);
        *cpp_cast(builder) << sym;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_symbol_equal(clingo_symbol_t a, clingo_symbol_t b) -> bool {
    auto sym_a = CppClingo::Symbol::from_rep(a);
    auto sym_b = CppClingo::Symbol::from_rep(b);
    return sym_a == sym_b;
}

extern "C" auto clingo_symbol_compare(clingo_symbol_t a, clingo_symbol_t b) -> int {
    auto sym_a = CppClingo::Symbol::from_rep(a);
    auto sym_b = CppClingo::Symbol::from_rep(b);
    return c_cast(sym_a <=> sym_b);
}

extern "C" auto clingo_symbol_hash(clingo_symbol_t symbol) -> size_t {
    return CppClingo::Symbol::from_rep(symbol).hash();
}

extern "C" auto clingo_parse_term(clingo_lib_t *lib, char const *string, size_t size, clingo_symbol_t *symbol) -> bool {
    CLINGO_TRY {
        require(not_null(lib, symbol) && valid_span(string, size));
        CppClingo::Input::Parser p{lib->log, *lib->store};
        p.init({string, size}, *lib->store->string("<string>"));
        auto sym = p.parse_symbol();
        if (!sym) {
            throw std::runtime_error("parsing term failed");
        }
        *symbol = CppClingo::SharedSymbol::to_rep(*sym);
    }
    CLINGO_CATCH;
}
