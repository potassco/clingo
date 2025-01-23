#include <clingo/base.h>

#include <clingo/ground/base.hh>

#include "lib.hh"

// TODO: alternatives
// - store an external getter in the bases (meh)
// - stick to the current version where the second pointer can be clasp's facade
// - store external bit in the bases (meh)
// - allocate (meh)
using clingo_bases_t = struct clingo_bases {
    uintptr_t a;
    uintptr_t b;
};

using clingo_atom_base_t = struct clingo_atom_base {
    uintptr_t a;
    uintptr_t b;
};

auto cpp_cast(clingo_bases_t const *bases) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Ground::Bases *>(bases->a);
}

auto cpp_cast(clingo_atom_base_t const *atoms) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Ground::AtomBase *>(atoms->a);
}

using clingo_term_base_t = struct clingo_term_base {
    Clingo::Ground::TermBaseMap *base;
};

// Inspect the bases

extern "C" auto clingo_base_atoms_size(clingo_bases_t const *bases, size_t *size) -> clingo_result_t {
    CLINGO_TRY {
        if (size != nullptr) {
            *size = cpp_cast(bases)->atoms().size();
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_atoms_at(clingo_bases_t const *bases, size_t index, char const **name, size_t *arity,
                                     bool *sign, clingo_atom_base_t *atoms) -> clingo_result_t {
    CLINGO_TRY {
        auto it = cpp_cast(bases)->atoms().nth(index);
        if (atoms != nullptr) {
            atoms->a = reinterpret_cast<uintptr_t>(it->second.get()); // NOLINT
            atoms->b = bases->b;
        }
        if (name != nullptr) {
            *name = get<0>(it.key()).c_str();
        }
        if (arity != nullptr) {
            *arity = get<1>(it.key());
        }
        if (sign != nullptr) {
            *sign = get<2>(it.key());
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_atoms_find(clingo_bases_t const *bases, clingo_symbol_t symbol, clingo_atom_base_t *atoms,
                                       size_t *size) -> clingo_result_t {
    CLINGO_TRY {
        auto const &atms = cpp_cast(bases)->atoms();
        auto sym = cpp_cast(symbol);
        if (auto sig = sym.signature()) {
            auto it = atms.find(*sig);
            if (atoms != nullptr) {
                atoms->a = reinterpret_cast<uintptr_t>(it->second.get()); // NOLINT
                atoms->b = bases->b;
            }
            if (size != nullptr) {
                *size = std::distance(it, atms.end());
            }
        } else {
            throw std::invalid_argument{"function symbol expected"};
        }
    }
    CLINGO_CATCH;
}

// Inspect an atom base

extern "C" auto clingo_atom_base_size(clingo_atom_base_t const *atoms, size_t *size) -> clingo_result_t {
    CLINGO_TRY { *size = cpp_cast(atoms)->size(); }
    CLINGO_CATCH;
}

extern "C" auto clingo_atom_base_is_fact(clingo_atom_base_t const *atoms, size_t index, bool *fact) -> clingo_result_t {
    CLINGO_TRY { *fact = cpp_cast(atoms)->nth(index)->second.state == Clingo::Ground::StateAtom::fact; }
    CLINGO_CATCH;
}

extern "C" auto clingo_atom_base_is_external(clingo_atom_base_t const *atoms, size_t index, bool *is_external)
    -> clingo_result_t {
    CLINGO_TRY {
        auto id = cpp_cast(atoms)->nth(index)->second.id;
        // TODO: needs help of the solver
        *is_external = id >= 0;
        // *external = solver->is_external(id);
        throw std::logic_error("implement me: is_external");
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_atom_base_symbol(clingo_atom_base_t const *atoms, size_t index, clingo_symbol_t *symbol)
    -> clingo_result_t {
    CLINGO_TRY {
        if (index < cpp_cast(atoms)->size()) {
            *symbol = Clingo::Symbol::to_rep(cpp_cast(atoms)->nth(index)->first);
        } else {
            throw std::out_of_range{"index out of range"};
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_atom_base_literal(clingo_atom_base_t const *atoms, size_t index, clingo_literal_t *literal)
    -> clingo_result_t {
    CLINGO_TRY {
        if (index < cpp_cast(atoms)->size()) {
            *literal = static_cast<clingo_literal_t>(cpp_cast(atoms)->nth(index)->second.id);
        } else {
            throw std::out_of_range{"index out of range"};
        }
    }
    CLINGO_CATCH;
}

// Inspect the term base

extern "C" auto clingo_term_base_size(clingo_term_base_t const *terms, size_t *size) -> clingo_result_t {
    CLINGO_TRY { *size = terms->base->size(); }
    CLINGO_CATCH;
}

// TODO:
// - get symbol
// - get conditions
