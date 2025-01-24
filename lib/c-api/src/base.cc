#include <clingo/base.h> // IWYU pragma: export

#include <clingo/ground/base.hh>

#include "control.hh" // IWYU pragma: keep
#include "lib.hh"

auto cpp_cast(clingo_base_t const *bases) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Ground::Bases *>(bases->a);
}

auto cpp_cast(clingo_atom_base_t const *atoms) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Ground::AtomBase *>(atoms->a);
}

auto cpp_cast_alt(clingo_atom_base_t const *atoms) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clasp::Asp::LogicProgram *>(atoms->b);
}

auto cpp_cast(clingo_term_base_t const *terms) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Ground::TermBaseMap const *>(terms);
}

extern "C" auto clingo_base_atoms_size(clingo_base_t const *base, size_t *size) -> clingo_result_t {
    CLINGO_TRY {
        if (size != nullptr) {
            *size = cpp_cast(base)->atoms().size();
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_atoms_at(clingo_base_t const *bases, size_t index, clingo_signature_t *signature,
                                     clingo_atom_base_t *atoms) -> clingo_result_t {
    CLINGO_TRY {
        auto it = cpp_cast(bases)->atoms().nth(index);
        if (atoms != nullptr) {
            atoms->a = reinterpret_cast<uintptr_t>(it->second.get()); // NOLINT
            atoms->b = bases->b;
        }
        if (signature != nullptr) {
            signature->name = get<0>(it.key()).c_str();
            signature->arity = get<1>(it.key());
            signature->sign = get<2>(it.key());
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_atoms_find(clingo_base_t const *bases, clingo_signature_t const *signature,
                                       clingo_atom_base_t *atoms) -> clingo_result_t {
    CLINGO_TRY {
        auto const &atms = cpp_cast(bases)->atoms();
        auto sig = std::tuple{signature->name, signature->arity, signature->sign};
        auto it = atms.find(sig);
        if (atoms != nullptr) {
            atoms->a = reinterpret_cast<uintptr_t>(it->second.get()); // NOLINT
            atoms->b = bases->b;
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_atom_base_size(clingo_atom_base_t const *atoms, size_t *size) -> clingo_result_t {
    CLINGO_TRY {
        if (size != nullptr) {
            *size = cpp_cast(atoms)->size();
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_atom_base_is_fact(clingo_atom_base_t const *atoms, size_t index, bool *fact) -> clingo_result_t {
    CLINGO_TRY {
        if (fact != nullptr) {
            *fact = cpp_cast(atoms)->nth(index)->second.state == Clingo::Ground::StateAtom::fact;
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_atom_base_is_external(clingo_atom_base_t const *atoms, size_t index, bool *is_external)
    -> clingo_result_t {
    CLINGO_TRY {
        if (is_external != nullptr) {
            auto lit = static_cast<Clingo::Output::lit_t>(cpp_cast(atoms)->nth(index)->second.id);
            *is_external = atoms->b != 0 && cpp_cast_alt(atoms)->isExternal(lit);
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_atom_base_symbol(clingo_atom_base_t const *atoms, size_t index, clingo_symbol_t *symbol)
    -> clingo_result_t {
    CLINGO_TRY {
        if (index < cpp_cast(atoms)->size()) {
            if (symbol != nullptr) {
                *symbol = Clingo::Symbol::to_rep(cpp_cast(atoms)->nth(index)->first);
            }
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
            if (literal != nullptr) {
                *literal = static_cast<clingo_literal_t>(cpp_cast(atoms)->nth(index)->second.id);
            }
        } else {
            throw std::out_of_range{"index out of range"};
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_terms(clingo_base_t const *base, clingo_term_base_t const **terms) -> clingo_result_t {
    CLINGO_TRY {
        if (terms != nullptr) {
            /// NOLINTNEXTLINE
            *terms = reinterpret_cast<clingo_term_base_t const *>(&cpp_cast(base)->terms());
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_term_base_size(clingo_term_base_t const *terms, size_t *size) -> clingo_result_t {
    CLINGO_TRY {
        if (size != nullptr) {
            *size = cpp_cast(terms)->size();
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_term_base_symbol(clingo_term_base_t const *terms, size_t index, clingo_symbol_t *term)
    -> clingo_result_t {
    CLINGO_TRY {
        if (term != nullptr) {
            *term = *c_cast(&cpp_cast(terms)->nth(index)->first);
        }
    }
    CLINGO_CATCH;
}

// TODO:
// - get conditions

extern "C" auto clingo_control_base(clingo_control_t const *control, clingo_base_t *base) -> clingo_result_t {
    CLINGO_TRY {
        if (base != nullptr) {
            // NOLINTBEGIN
            base->a = reinterpret_cast<uintptr_t>(&control->slv->bases());
            base->b = reinterpret_cast<uintptr_t>(control->slv->clasp_program());
            // NOLINTEND
        }
    }
    CLINGO_CATCH;
}
