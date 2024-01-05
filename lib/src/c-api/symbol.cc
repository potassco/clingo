#include "lib.hh"

extern "C" auto clingo_symbol_create_number(int32_t number) -> clingo_symbol_t {
    return Gringo::Number::to_repr(Gringo::Number{number});
}

extern "C" auto clingo_symbol_create_number_str(clingo_lib_t *lib, char const *number, clingo_symbol_t *symbol)
    -> bool {
    CLINGO_TRY {
        if (lib == nullptr || number == nullptr || symbol == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        *symbol = Gringo::Symbol::to_rep(lib->store->num(Gringo::Number{number}));
    }
    CLINGO_CATCH(lib);
}

extern "C" auto clingo_symbol_create_infimum() -> clingo_symbol_t {
    return Gringo::Symbol::to_rep(Gringo::SymbolStore::sup());
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

extern "C" auto clingo_symbol_create_function(clingo_lib_t *lib, char const *name, clingo_symbol_t const *arguments,
                                              size_t arguments_size, bool positive, clingo_symbol_t *symbol) -> bool {
    CLINGO_TRY {
        if (lib == nullptr || name == nullptr || symbol == nullptr) {
            throw std::invalid_argument("invalid arguments");
        }
        *symbol = Gringo::Symbol::to_rep(lib->store->fun(
            lib->store->string(name),
            Gringo::SymbolSpan{reinterpret_cast<Gringo::Symbol const *>(arguments), arguments_size}, positive));
    }
    CLINGO_CATCH(lib);
}
