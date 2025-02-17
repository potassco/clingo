#include <clingo/base.h> // IWYU pragma: export

#include <clingo/ground/base.hh>

#include "control.hh" // IWYU pragma: keep
#include "lib.hh"
#include "potassco/theory_data.h"

auto cpp_cast(clingo_base_t const *bases) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Ground::Bases *>(bases->a);
}

auto cpp_cast(clingo_atom_base_t const *atoms) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Ground::AtomBase *>(atoms->a);
}

auto cpp_cast_alt(clingo_base_t const *atoms) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clasp::Asp::LogicProgram *>(atoms->b);
}

auto cpp_cast_alt(clingo_atom_base_t const *atoms) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clasp::Asp::LogicProgram *>(atoms->b);
}

auto cpp_cast(clingo_term_base_t const *terms) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Ground::TermBaseMap const *>(terms);
}

auto cpp_cast(clingo_theory_base_t const *theory) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Potassco::TheoryData const *>(theory);
}

extern "C" auto clingo_base_atoms_size(clingo_base_t const *base, size_t *size) -> clingo_result_t {
    CLINGO_TRY {
        if (base == nullptr || size == nullptr) {
            return clingo_result_invalid;
        }
        *size = cpp_cast(base)->atoms().size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_atoms_at(clingo_base_t const *bases, size_t index, clingo_signature_t *signature,
                                     clingo_atom_base_t *atoms) -> clingo_result_t {
    CLINGO_TRY {
        if (bases == nullptr || signature == nullptr) {
            return clingo_result_invalid;
        }
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
                                       clingo_atom_base_t *atoms, bool *found) -> clingo_result_t {
    CLINGO_TRY {
        if (bases == nullptr || signature == nullptr) {
            return clingo_result_invalid;
        }
        auto const &atms = cpp_cast(bases)->atoms();
        auto sig = std::tuple{signature->name, signature->arity, signature->sign};
        auto it = atms.find(sig);
        if (found != nullptr) {
            *found = it != atms.end();
        }
        if (atoms != nullptr && it != atms.end()) {
            atoms->a = reinterpret_cast<uintptr_t>(it->second.get()); // NOLINT
            atoms->b = bases->b;
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_atom_base_size(clingo_atom_base_t const *atoms, size_t *size) -> clingo_result_t {
    CLINGO_TRY {
        if (atoms == nullptr || size == nullptr) {
            return clingo_result_invalid;
        }
        *size = cpp_cast(atoms)->size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_atom_base_is_fact(clingo_atom_base_t const *atoms, size_t index, bool *fact) -> clingo_result_t {
    CLINGO_TRY {
        if (atoms == nullptr || fact == nullptr) {
            return clingo_result_invalid;
        }
        if (index < cpp_cast(atoms)->size()) {
            *fact = cpp_cast(atoms)->nth(index)->second.state == Clingo::Ground::StateAtom::fact;
        } else {
            return clingo_result_range;
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_atom_base_is_external(clingo_atom_base_t const *atoms, size_t index, bool *is_external)
    -> clingo_result_t {
    CLINGO_TRY {
        if (atoms == nullptr || is_external == nullptr) {
            return clingo_result_invalid;
        }
        if (index < cpp_cast(atoms)->size()) {
            auto lit = static_cast<Clingo::Output::lit_t>(cpp_cast(atoms)->nth(index)->second.id);
            *is_external = atoms->b != 0 && cpp_cast_alt(atoms)->isExternal(lit);
        } else {
            return clingo_result_range;
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_atom_base_symbol(clingo_atom_base_t const *atoms, size_t index, clingo_symbol_t *symbol)
    -> clingo_result_t {
    CLINGO_TRY {
        if (atoms == nullptr || symbol == nullptr) {
            return clingo_result_invalid;
        }
        if (index < cpp_cast(atoms)->size()) {
            *symbol = Clingo::Symbol::to_rep(cpp_cast(atoms)->nth(index)->first);
        } else {
            return clingo_result_range;
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_atom_base_literal(clingo_atom_base_t const *atoms, size_t index, clingo_literal_t *literal)
    -> clingo_result_t {
    CLINGO_TRY {
        if (atoms == nullptr || literal == nullptr) {
            return clingo_result_invalid;
        }
        if (index < cpp_cast(atoms)->size()) {
            *literal = static_cast<clingo_literal_t>(cpp_cast(atoms)->nth(index)->second.id);
        } else {
            return clingo_result_range;
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_atom_base_find(clingo_atom_base_t const *atoms, clingo_symbol_t symbol, size_t *index)
    -> clingo_result_t {
    CLINGO_TRY {
        if (atoms == nullptr || index == nullptr) {
            return clingo_result_invalid;
        }
        *index = cpp_cast(atoms)->index(cpp_cast(symbol));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_terms(clingo_base_t const *base, clingo_term_base_t const **terms) -> clingo_result_t {
    CLINGO_TRY {
        if (base == nullptr || terms == nullptr) {
            return clingo_result_invalid;
        }
        // NOLINTNEXTLINE
        *terms = reinterpret_cast<clingo_term_base_t const *>(&cpp_cast(base)->terms());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_term_base_size(clingo_term_base_t const *terms, size_t *size) -> clingo_result_t {
    CLINGO_TRY {
        if (terms == nullptr || size == nullptr) {
            return clingo_result_invalid;
        }
        *size = cpp_cast(terms)->size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_term_base_symbol(clingo_term_base_t const *terms, size_t index, clingo_symbol_t *term)
    -> clingo_result_t {
    CLINGO_TRY {
        if (terms == nullptr || term == nullptr) {
            return clingo_result_invalid;
        }
        if (index < cpp_cast(terms)->size()) {
            *term = *c_cast(&cpp_cast(terms)->nth(index)->first);
        } else {
            return clingo_result_range;
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_term_base_condition(clingo_term_base_t const *terms, size_t index, clingo_literal_t **literals,
                                           size_t *size) -> clingo_result_t {
    CLINGO_TRY {
        if (terms == nullptr || literals == nullptr || size == nullptr) {
            return clingo_result_invalid;
        }
        auto const &[state, cond] = cpp_cast(terms)->nth(index).value();
        // NOTE: this seems to be the (safe) easiest way to convert from 64bit to 32bit here
        static thread_local auto result = std::vector<clingo_literal_t>{};
        result.reserve(cond.size());
        result.assign(cond.begin(), cond.end());
        *literals = result.data();
        *size = cond.size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_term_base_find(clingo_term_base_t const *terms, clingo_symbol_t symbol, size_t *index)
    -> clingo_result_t {
    CLINGO_TRY {
        if (terms == nullptr || index == nullptr) {
            return clingo_result_invalid;
        }
        auto it = cpp_cast(terms)->find(cpp_cast(symbol));
        *index = std::distance(cpp_cast(terms)->begin(), it);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_base(clingo_control_t const *control, clingo_base_t *base) -> clingo_result_t {
    CLINGO_TRY {
        if (control == nullptr || base == nullptr) {
            return clingo_result_invalid;
        }
        // NOLINTBEGIN
        base->a = reinterpret_cast<uintptr_t>(&control->slv->bases());
        base->b = reinterpret_cast<uintptr_t>(control->slv->clasp_program());
        // NOLINTEND
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_theory(clingo_base_t const *base, clingo_theory_base_t const **theory) -> clingo_result_t {
    CLINGO_TRY {
        if (base == nullptr || theory == nullptr) {
            return clingo_result_invalid;
        }
        // NOLINTNEXTLINE
        *theory = reinterpret_cast<clingo_theory_base_t const *>(&cpp_cast_alt(base)->theoryData());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_term_type(clingo_theory_base_t const *theory, clingo_id_t term,
                                             clingo_theory_term_type_t *type) -> clingo_result_t {
    CLINGO_TRY {
        if (theory == nullptr || type == nullptr) {
            return clingo_result_invalid;
        }
        auto const &x = cpp_cast(theory)->getTerm(term);
        switch (x.type()) {
            case Potassco::TheoryTermType::number: {
                *type = clingo_theory_term_type_number;
                break;
            }
            case Potassco::TheoryTermType::symbol: {
                *type = clingo_theory_term_type_symbol;
                break;
            }
            case Potassco::TheoryTermType::compound: {
                switch (x.compound()) {
                    case static_cast<int>(Potassco::TupleType::brace): {
                        *type = clingo_theory_term_type_set;
                        break;
                    }
                    case static_cast<int>(Potassco::TupleType::bracket): {
                        *type = clingo_theory_term_type_list;
                        break;
                    }
                    case static_cast<int>(Potassco::TupleType::paren): {
                        *type = clingo_theory_term_type_tuple;
                        break;
                    }
                    default: {
                        *type = clingo_theory_term_type_function;
                        break;
                    }
                }
                break;
            }
        }
    }
    CLINGO_CATCH;
}
