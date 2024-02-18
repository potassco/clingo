#include "lib.hh"
#include "streams.hh"

#include <gringo/input/algo/evaluate.hh>
#include <gringo/input/algo/parse.hh>

extern "C" auto clingo_add_string(clingo_lib_t *lib, char const *string, char const **result) -> bool {
    CLINGO_TRY {
        if (lib == nullptr || string == nullptr || result == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        *result = lib->store->string(string).c_str();
    }
    CLINGO_CATCH(lib);
}

extern "C" auto clingo_symbol_create_number(int32_t number) -> clingo_symbol_t {
    return Gringo::Number::to_repr(Gringo::Number{number});
}

extern "C" auto clingo_symbol_create_number_str(clingo_lib_t *lib, char const *number, clingo_symbol_t *symbol)
    -> bool {
    CLINGO_TRY {
        if (lib == nullptr || number == nullptr || symbol == nullptr) {
            std::cerr << "going to throw invalid arguments!!!" << std::endl;
            throw std::invalid_argument("invalid arguments");
        }
        *symbol = Gringo::Symbol::to_rep(lib->store->num(Gringo::Number{number}));
    }
    CLINGO_CATCH(lib);
}

extern "C" auto clingo_symbol_create_infimum() -> clingo_symbol_t {
    return Gringo::Symbol::to_rep(Gringo::SymbolStore::inf());
}

extern "C" auto clingo_symbol_create_supremum() -> clingo_symbol_t {
    return Gringo::Symbol::to_rep(Gringo::SymbolStore::sup());
}

extern "C" auto clingo_symbol_create_string(clingo_lib_t *lib, char const *string, clingo_symbol_t *symbol) -> bool {
    CLINGO_TRY {
        if (lib == nullptr || string == nullptr || symbol == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        *symbol = Gringo::Symbol::to_rep(lib->store->str(lib->store->string(string)));
    }
    CLINGO_CATCH(lib);
}

extern "C" auto clingo_symbol_create_id(clingo_lib_t *lib, char const *name, bool positive, clingo_symbol_t *symbol)
    -> bool {
    CLINGO_TRY {
        if (lib == nullptr || name == nullptr || symbol == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        *symbol = Gringo::Symbol::to_rep(lib->store->fun(lib->store->string(name), {}, positive));
    }
    CLINGO_CATCH(lib);
}

extern "C" auto clingo_symbol_create_tuple(clingo_lib_t *lib, clingo_symbol_t const *arguments, size_t arguments_size,
                                           clingo_symbol_t *symbol) -> bool {
    CLINGO_TRY {
        if (lib == nullptr || symbol == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        *symbol = Gringo::Symbol::to_rep(
            lib->store->tup(Gringo::SymbolSpan{reinterpret_cast<Gringo::Symbol const *>(arguments), arguments_size}));
    }
    CLINGO_CATCH(lib);
}

extern "C" auto clingo_symbol_create_function(clingo_lib_t *lib, char const *name, clingo_symbol_t const *arguments,
                                              size_t arguments_size, bool sign, clingo_symbol_t *symbol) -> bool {
    CLINGO_TRY {
        if (lib == nullptr || name == nullptr || symbol == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        *symbol = Gringo::Symbol::to_rep(lib->store->fun(
            lib->store->string(name),
            Gringo::SymbolSpan{reinterpret_cast<Gringo::Symbol const *>(arguments), arguments_size}, sign));
    }
    CLINGO_CATCH(lib);
}

extern "C" auto clingo_symbol_number(clingo_symbol_t symbol, int32_t *number) -> bool {
    auto sym = Gringo::Symbol::from_rep(symbol);
    if (number == nullptr || sym.type() != Gringo::SymbolType::number) {
        return false;
    }
    auto num = sym.num()->as_int();
    if (!num) {
        return false;
    }
    *number = *num;
    return true;
}

extern "C" auto clingo_symbol_name(clingo_symbol_t symbol, char const **name) -> bool {
    auto sym = Gringo::Symbol::from_rep(symbol);
    if (name == nullptr || sym.type() != Gringo::SymbolType::function) {
        return false;
    }
    *name = sym.name().c_str();
    return true;
}

extern "C" auto clingo_symbol_string(clingo_symbol_t symbol, char const **string) -> bool {
    auto sym = Gringo::Symbol::from_rep(symbol);
    if (string == nullptr || sym.type() != Gringo::SymbolType::string) {
        return false;
    }
    *string = sym.str().c_str();
    return true;
}

extern "C" auto clingo_symbol_has_sign(clingo_symbol_t symbol, bool *has_sign) -> bool {
    auto sym = Gringo::Symbol::from_rep(symbol);
    if (has_sign == nullptr ||
        (sym.type() != Gringo::SymbolType::function && sym.type() != Gringo::SymbolType::number)) {
        return false;
    }
    *has_sign = sym.has_sign();
    return true;
}

extern "C" auto clingo_symbol_arguments(clingo_symbol_t symbol, clingo_symbol_t const **arguments,
                                        size_t *arguments_size) -> bool {
    auto sym = Gringo::Symbol::from_rep(symbol);
    if (arguments == nullptr || arguments_size == nullptr ||
        (sym.type() != Gringo::SymbolType::function && sym.type() != Gringo::SymbolType::tuple)) {
        return false;
    }
    auto args = sym.args();
    *arguments = reinterpret_cast<clingo_symbol_t const *>(args.data());
    *arguments_size = args.size();
    return true;
}

static_assert(static_cast<int>(Gringo::SymbolType::function) == clingo_symbol_type_function);
static_assert(static_cast<int>(Gringo::SymbolType::inf) == clingo_symbol_type_infimum);
static_assert(static_cast<int>(Gringo::SymbolType::sup) == clingo_symbol_type_supremum);
static_assert(static_cast<int>(Gringo::SymbolType::number) == clingo_symbol_type_number);
static_assert(static_cast<int>(Gringo::SymbolType::tuple) == clingo_symbol_type_tuple);
static_assert(static_cast<int>(Gringo::SymbolType::string) == clingo_symbol_type_string);

extern "C" auto clingo_symbol_type(clingo_symbol_t symbol) -> clingo_symbol_type_t {
    auto sym = Gringo::Symbol::from_rep(symbol);
    return static_cast<clingo_symbol_type_t>(sym.type());
}

extern "C" auto clingo_symbol_to_string_size(clingo_symbol_t symbol, size_t *size) -> bool {
    if (size == nullptr) {
        return false;
    }
    try {
        auto sym = Gringo::Symbol::from_rep(symbol);
        *size = print_size(sym);
        return true;
    } catch (...) {
        return false;
    }
}

extern "C" auto clingo_symbol_to_string(clingo_symbol_t symbol, char *string, size_t size) -> bool {
    if (string == nullptr) {
        return false;
    }
    try {
        auto sym = Gringo::Symbol::from_rep(symbol);
        print(string, size, sym);
        return true;
    } catch (...) {
        return false;
    }
}

extern "C" auto clingo_symbol_is_equal_to(clingo_symbol_t a, clingo_symbol_t b) -> bool {
    auto sym_a = Gringo::Symbol::from_rep(a);
    auto sym_b = Gringo::Symbol::from_rep(b);
    return sym_a == sym_b;
}

extern "C" auto clingo_symbol_is_less_than(clingo_symbol_t a, clingo_symbol_t b) -> bool {
    auto sym_a = Gringo::Symbol::from_rep(a);
    auto sym_b = Gringo::Symbol::from_rep(b);
    return sym_a < sym_b;
}

extern "C" auto clingo_symbol_hash(clingo_symbol_t symbol) -> size_t {
    return Gringo::Util::hash_mix(Gringo::Util::value_hash_record<clingo_symbol_t>(Gringo::Symbol::from_rep(symbol)));
}

extern "C" auto clingo_parse_term(clingo_lib_t *lib, char const *string, clingo_symbol_t *symbol) -> bool {
    CLINGO_TRY {
        if (lib == nullptr || string == nullptr || symbol == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        auto term = Gringo::Input::parse_term(lib->log, *lib->store, string);
        if (lib->log.has_error() || !term) {
            lib->log.reset();
            throw std::runtime_error("parsing term failed");
        }
        auto sym = Gringo::Input::evaluate(lib->log, *lib->store, {}, *term);
        if (lib->log.has_error() || !sym) {
            lib->log.reset();
            throw std::runtime_error("parsing term failed");
        }
        *symbol = Gringo::Symbol::to_rep(*sym);
    }
    CLINGO_CATCH(lib);
}
