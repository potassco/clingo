#include "core.hh"
#include "lib.hh"

#include <clingo/input/parser.hh>

#include <clingo/input/rewrite/evaluate.hh>

extern "C" void clingo_symbol_acquire(clingo_symbol_t symbol) {
    Clingo::Symbol::from_rep(symbol).acquire();
}

extern "C" void clingo_symbol_release(clingo_symbol_t symbol) {
    if (symbol != 0) {
        Clingo::Symbol::from_rep(symbol).release();
    }
}

extern "C" auto clingo_symbol_create_infimum() -> clingo_symbol_t {
    // NOTE: does not need reference counting
    return Clingo::Symbol::to_rep(Clingo::SymbolStore::inf());
}

extern "C" auto clingo_symbol_create_supremum() -> clingo_symbol_t {
    // NOTE: does not need reference counting
    return Clingo::Symbol::to_rep(Clingo::SymbolStore::sup());
}

extern "C" auto clingo_symbol_create_number(int32_t number) -> clingo_symbol_t {
    // NOTE: does not need reference counting
    return Clingo::Symbol::to_rep(Clingo::SymbolStore::num_ref(number));
}

extern "C" auto clingo_symbol_create_number_str(clingo_lib_t *lib, char const *number, size_t size,
                                                clingo_symbol_t *symbol) -> bool {
    CLINGO_TRY {
        if (lib == nullptr || (number == nullptr && size > 0) || symbol == nullptr) {
            return fail_arguments();
        }
        *symbol = Clingo::SharedSymbol::to_rep(lib->store->num(Clingo::Number{std::string_view{number, size}}));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_symbol_create_string(clingo_lib_t *lib, char const *string, size_t size, clingo_symbol_t *symbol)
    -> bool {
    CLINGO_TRY {
        if (lib == nullptr || (string == nullptr && size > 0) || symbol == nullptr) {
            return fail_arguments();
        }
        *symbol = Clingo::SharedSymbol::to_rep(Clingo::SymbolStore::str(*lib->store->string({string, size})));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_symbol_create_id(clingo_lib_t *lib, char const *name, size_t size, bool is_positive,
                                        clingo_symbol_t *symbol) -> bool {
    CLINGO_TRY {
        if (lib == nullptr || (name == nullptr && size > 0) || symbol == nullptr) {
            return fail_arguments();
        }
        *symbol = Clingo::SharedSymbol::to_rep(lib->store->fun(lib->store->string({name, size}), {}, !is_positive));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_symbol_create_tuple(clingo_lib_t *lib, clingo_symbol_t const *arguments, size_t arguments_size,
                                           clingo_symbol_t *symbol) -> bool {
    CLINGO_TRY {
        if (lib == nullptr || symbol == nullptr) {
            return fail_arguments();
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto const *c_args = reinterpret_cast<Clingo::Symbol const *>(arguments);
        *symbol = Clingo::SharedSymbol::to_rep(lib->store->tup(Clingo::SymbolSpan{c_args, arguments_size}));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_symbol_create_function(clingo_lib_t *lib, char const *name, size_t name_size,
                                              clingo_symbol_t const *arguments, size_t arguments_size, bool is_positive,
                                              clingo_symbol_t *symbol) -> bool {
    CLINGO_TRY {
        if (lib == nullptr || (name == nullptr && name_size > 0) || symbol == nullptr) {
            return fail_arguments();
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        auto const *c_args = reinterpret_cast<Clingo::Symbol const *>(arguments);
        *symbol = Clingo::SharedSymbol::to_rep(lib->store->fun(
            *lib->store->string({name, name_size}), Clingo::SymbolSpan{c_args, arguments_size}, !is_positive));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_symbol_number(clingo_symbol_t symbol, int32_t *number) -> bool {
    auto sym = Clingo::Symbol::from_rep(symbol);
    if (number == nullptr || sym.type() != Clingo::SymbolType::number) {
        return fail_arguments();
    }
    auto num = sym.num().as_int();
    if (!num) {
        return fail_with(clingo_result_range, "number out of range");
    }
    *number = *num;
    return true;
}

extern "C" auto clingo_symbol_name(clingo_symbol_t symbol, clingo_string_t *name) -> bool {
    auto sym = Clingo::Symbol::from_rep(symbol);
    if (name == nullptr || name == nullptr || sym.type() != Clingo::SymbolType::function) {
        return fail_arguments();
    }
    auto res = sym.name().view();
    name->data = res.data();
    name->size = res.size();
    return true;
}

extern "C" auto clingo_symbol_string(clingo_symbol_t symbol, clingo_string_t *string) -> bool {
    auto sym = Clingo::Symbol::from_rep(symbol);
    if (string == nullptr || string == nullptr || sym.type() != Clingo::SymbolType::string) {
        return fail_arguments();
    }
    auto res = sym.str().view();
    string->data = res.data();
    string->size = res.size();
    return true;
}

extern "C" auto clingo_symbol_is_positive(clingo_symbol_t symbol, bool *is_positive) -> bool {
    auto sym = Clingo::Symbol::from_rep(symbol);
    if (is_positive == nullptr ||
        (sym.type() != Clingo::SymbolType::function && sym.type() != Clingo::SymbolType::number)) {
        return fail_arguments();
    }
    *is_positive = !sym.has_sign();
    return true;
}

extern "C" auto clingo_symbol_arguments(clingo_symbol_t symbol, clingo_symbol_t const **arguments,
                                        size_t *arguments_size) -> bool {
    auto sym = Clingo::Symbol::from_rep(symbol);
    if (arguments == nullptr || arguments_size == nullptr ||
        (sym.type() != Clingo::SymbolType::function && sym.type() != Clingo::SymbolType::tuple)) {
        return fail_arguments();
    }
    auto args = sym.args();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    *arguments = reinterpret_cast<clingo_symbol_t const *>(args.data());
    *arguments_size = args.size();
    return true;
}

static_assert(static_cast<int>(Clingo::SymbolType::function) == clingo_symbol_type_function);
static_assert(static_cast<int>(Clingo::SymbolType::inf) == clingo_symbol_type_infimum);
static_assert(static_cast<int>(Clingo::SymbolType::sup) == clingo_symbol_type_supremum);
static_assert(static_cast<int>(Clingo::SymbolType::number) == clingo_symbol_type_number);
static_assert(static_cast<int>(Clingo::SymbolType::tuple) == clingo_symbol_type_tuple);
static_assert(static_cast<int>(Clingo::SymbolType::string) == clingo_symbol_type_string);

extern "C" auto clingo_symbol_type(clingo_symbol_t symbol) -> clingo_symbol_type_t {
    auto sym = Clingo::Symbol::from_rep(symbol);
    return static_cast<clingo_symbol_type_t>(sym.type());
}

extern "C" auto clingo_symbol_to_string(clingo_symbol_t symbol, clingo_string_builder_t *builder) -> bool {
    CLINGO_TRY {
        if (builder == nullptr) {
            return fail_arguments();
        }
        auto sym = Clingo::Symbol::from_rep(symbol);
        *cpp_cast(builder) << sym;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_symbol_equal(clingo_symbol_t a, clingo_symbol_t b) -> bool {
    auto sym_a = Clingo::Symbol::from_rep(a);
    auto sym_b = Clingo::Symbol::from_rep(b);
    return sym_a == sym_b;
}

extern "C" auto clingo_symbol_compare(clingo_symbol_t a, clingo_symbol_t b) -> int {
    auto sym_a = Clingo::Symbol::from_rep(a);
    auto sym_b = Clingo::Symbol::from_rep(b);
    return c_cast(sym_a <=> sym_b);
}

extern "C" auto clingo_symbol_hash(clingo_symbol_t symbol) -> size_t {
    return Clingo::Symbol::from_rep(symbol).hash();
}

extern "C" auto clingo_parse_term(clingo_lib_t *lib, char const *string, size_t size, clingo_symbol_t *symbol) -> bool {
    CLINGO_TRY {
        if (lib == nullptr || (string == nullptr && size > 0) || symbol == nullptr) {
            return fail_arguments();
        }
        Clingo::Input::Parser p{lib->log, *lib->store};
        p.init({string, size}, *lib->store->string("<string>"));
        auto sym = p.parse_symbol();
        if (!sym) {
            throw std::runtime_error("parsing term failed");
        }
        *symbol = Clingo::SharedSymbol::to_rep(*sym);
    }
    CLINGO_CATCH;
}
