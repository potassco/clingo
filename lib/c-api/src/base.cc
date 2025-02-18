#include <clingo/base.h> // IWYU pragma: export

#include <clingo/ground/base.hh>

#include <potassco/theory_data.h>

#include "control.hh" // IWYU pragma: keep
#include "core.hh"
#include "lib.hh"

auto get_base(clingo_base_t const *base) -> Clingo::Ground::Bases const & {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Control::BaseControl const *>(base)->bases();
}

auto get_program(clingo_base_t const *base) -> Clasp::Asp::LogicProgram const & {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Control::BaseControl const *>(base)->clasp_program();
}

auto cpp_cast(clingo_atom_base_t const *atoms) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Ground::AtomBase const *>(atoms);
}

auto cpp_cast(clingo_term_base_t const *terms) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Ground::TermBaseMap const *>(terms);
}

auto get_theory(clingo_theory_base_t const *theory) -> Potassco::TheoryData const & {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Control::BaseControl const *>(theory)->clasp_theory();
}

auto get_program(clingo_theory_base_t const *theory) -> Clasp::Asp::LogicProgram const & {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Control::BaseControl const *>(theory)->clasp_program();
}

extern "C" auto clingo_base_atoms_size(clingo_base_t const *base, size_t *size) -> clingo_result_t {
    CLINGO_TRY {
        if (base == nullptr || size == nullptr) {
            return clingo_result_invalid;
        }
        *size = get_base(base).atoms().size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_atoms_at(clingo_base_t const *bases, size_t index, clingo_signature_t *signature,
                                     clingo_atom_base_t const **atoms) -> clingo_result_t {
    CLINGO_TRY {
        if (bases == nullptr || signature == nullptr) {
            return clingo_result_invalid;
        }
        auto it = get_base(bases).atoms().nth(index);
        if (atoms != nullptr) {
            *atoms = reinterpret_cast<clingo_atom_base_t const *>(it->second.get()); // NOLINT
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
                                       clingo_atom_base_t const **atoms, bool *found) -> clingo_result_t {
    CLINGO_TRY {
        if (bases == nullptr || signature == nullptr) {
            return clingo_result_invalid;
        }
        auto const &atms = get_base(bases).atoms();
        auto sig = std::tuple{signature->name, signature->arity, signature->sign};
        auto it = atms.find(sig);
        if (found != nullptr) {
            *found = it != atms.end();
        }
        if (atoms != nullptr && it != atms.end()) {
            *atoms = reinterpret_cast<clingo_atom_base_t const *>(it->second.get()); // NOLINT
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

extern "C" auto clingo_base_is_fact(clingo_base_t const *atoms, clingo_literal_t literal, bool *is_fact)
    -> clingo_result_t {
    CLINGO_TRY {
        if (atoms == nullptr || is_fact == nullptr) {
            return clingo_result_invalid;
        }
        *is_fact = get_program(atoms).isFact(std::abs(literal));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_is_external(clingo_base_t const *atoms, clingo_literal_t literal, bool *is_external)
    -> clingo_result_t {
    CLINGO_TRY {
        if (atoms == nullptr || is_external == nullptr) {
            return clingo_result_invalid;
        }
        *is_external = get_program(atoms).isExternal(std::abs(literal));
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
        *terms = reinterpret_cast<clingo_term_base_t const *>(&get_base(base).terms());
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

extern "C" auto clingo_control_base(clingo_control_t const *control, clingo_base_t const **base) -> clingo_result_t {
    CLINGO_TRY {
        if (control == nullptr || base == nullptr) {
            return clingo_result_invalid;
        }
        // NOLINTNEXTLINE
        *base = reinterpret_cast<clingo_base_t const *>(control->slv);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_theory(clingo_base_t const *base, clingo_theory_base_t const **theory) -> clingo_result_t {
    CLINGO_TRY {
        if (base == nullptr || theory == nullptr) {
            return clingo_result_invalid;
        }
        // NOLINTNEXTLINE
        *theory = reinterpret_cast<clingo_theory_base_t const *>(base);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_term_type(clingo_theory_base_t const *theory, clingo_id_t term,
                                             clingo_theory_term_type_t *type) -> clingo_result_t {
    CLINGO_TRY {
        if (theory == nullptr || type == nullptr) {
            return clingo_result_invalid;
        }
        auto const &x = get_theory(theory).getTerm(term);
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

extern "C" auto clingo_theory_base_term_number(clingo_theory_base_t const *theory, clingo_id_t term, int *number)
    -> clingo_result_t {
    CLINGO_TRY {
        if (theory == nullptr || number == nullptr) {
            return clingo_result_invalid;
        }
        *number = get_theory(theory).getTerm(term).number();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_term_name(clingo_theory_base_t const *theory, clingo_id_t term, char const **name)
    -> clingo_result_t {
    CLINGO_TRY {
        if (theory == nullptr || name == nullptr) {
            return clingo_result_invalid;
        }
        auto const &t = get_theory(theory);
        auto x = t.getTerm(term);
        *name = x.isFunction() ? t.getTerm(x.function()).symbol() : x.symbol();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_term_arguments(clingo_theory_base_t const *theory, clingo_id_t term,
                                                  clingo_id_t const **arguments, size_t *size) -> clingo_result_t {
    CLINGO_TRY {
        if (theory == nullptr || arguments == nullptr || size == nullptr) {
            return clingo_result_invalid;
        }
        auto args = get_theory(theory).getTerm(term).terms();
        *arguments = args.data();
        *size = args.size();
    }
    CLINGO_CATCH;
}

namespace {

class TheoryPrinter {
  public:
    TheoryPrinter(clingo_theory_base_t const *theory, clingo_string_builder_t *builder)
        : data_{&get_theory(theory)}, out_{cpp_cast(builder)} {}

    void term(clingo_id_t id) const {
        auto x = data_->getTerm(id);
        switch (x.type()) {
            case Potassco::TheoryTermType::number: {
                *out_ << x.number();
                break;
            }
            case Potassco::TheoryTermType::symbol: {
                *out_ << x.symbol();
                break;
            }
            case Potassco::TheoryTermType::compound: {
                auto args = x.terms();
                if (x.isFunction()) {
                    auto const *name = data_->getTerm(x.function()).symbol();
                    if (args.size() == 2 && Clingo::Input::is_theory_operator(name)) {
                        *out_ << "(";
                        term(args.front());
                        *out_ << name;
                        term(args.back());
                        *out_ << ")";
                        break;
                    }
                    *out_ << name;
                }
                auto p = Potassco::parens(x.isFunction() ? Potassco::TupleType::paren
                                                         : static_cast<Potassco::TupleType>(x.compound()));
                *out_ << p[0];
                Clingo::Util::p_range(args, [this]([[maybe_unused]] auto const &out, auto const &y) { term(y); });

                *out_ << p[1];
                break;
            }
        }
    }
    void elem(clingo_id_t id) const {
        auto const &x = data_->getElement(id);
        Clingo::Util::p_range(x.terms(), [this]([[maybe_unused]] auto const &out, auto const &y) { term(y); });
        *out_ << ": ";
        *out_ << "<cond>";
    }

  private:
    Potassco::TheoryData const *data_;
    Clingo::Util::OutputBuffer *out_;
};

} // namespace

extern "C" auto clingo_theory_base_term_to_string(clingo_theory_base_t const *theory, clingo_id_t term,
                                                  clingo_string_builder_t *builder) -> clingo_result_t {
    CLINGO_TRY {
        if (theory == nullptr || builder == nullptr) {
            return clingo_result_invalid;
        }
        TheoryPrinter{theory, builder}.term(term);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_element_tuple(clingo_theory_base_t const *theory, clingo_id_t element,
                                                 clingo_id_t const **tuple, size_t *size) -> clingo_result_t {
    CLINGO_TRY {
        if (theory == nullptr) {
            return clingo_result_invalid;
        }
        auto cond = get_theory(theory).getElement(element).terms();
        if (tuple != nullptr) {
            *tuple = cond.data();
        }
        if (size != nullptr) {
            *size = cond.size();
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_element_condition(clingo_theory_base_t const *theory, clingo_id_t element,
                                                     clingo_literal_t const **condition, size_t *size)
    -> clingo_result_t {
    CLINGO_TRY {
        if (theory == nullptr) {
            return clingo_result_invalid;
        }
        auto cond = get_theory(theory).getElement(element).condition();
        // FIXME: condition getter
        static_cast<void>(cond);
        if (condition != nullptr) {
            *condition = nullptr;
        }
        if (size != nullptr) {
            *size = 0;
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_element_condition_id(clingo_theory_base_t const *theory, clingo_id_t element,
                                                        clingo_literal_t *condition) -> clingo_result_t {
    CLINGO_TRY {
        if (theory == nullptr || condition == nullptr) {
            return clingo_result_invalid;
        }
        *condition = static_cast<clingo_literal_t>(get_theory(theory).getElement(element).condition());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_element_to_string(clingo_theory_base_t const *theory, clingo_id_t element,
                                                     clingo_string_builder_t *builder) -> clingo_result_t {
    CLINGO_TRY {
        if (theory == nullptr || builder == nullptr) {
            return clingo_result_invalid;
        }
        TheoryPrinter{theory, builder}.elem(element);
    }
    CLINGO_CATCH;
}
