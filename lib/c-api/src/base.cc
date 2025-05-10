#include <clingo/base.h> // IWYU pragma: export

#include <clingo/ground/base.hh>

#include <potassco/theory_data.h>

#include "control.hh" // IWYU pragma: keep
#include "core.hh"
#include "lib.hh"

auto cpp_cast(clingo_base_t const *base) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Control::BaseView const *>(base);
}

auto cpp_cast(clingo_atom_base_t const *base) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Ground::AtomBase const *>(base);
}

auto cpp_cast(clingo_term_base_t const *base) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Control::BaseView const *>(base);
}

auto cpp_cast(clingo_theory_base_t const *base) {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Control::BaseView const *>(base);
}

auto get_base(clingo_base_t const *base) -> Clingo::Ground::Bases const & {
    return cpp_cast(base)->bases();
}

auto get_program(clingo_base_t const *base) -> Clasp::Asp::LogicProgram const & {
    return cpp_cast(base)->clasp_program();
}

auto get_theory(clingo_theory_base_t const *theory) -> Potassco::TheoryData const & {
    return cpp_cast(theory)->clasp_theory();
}

auto get_program(clingo_theory_base_t const *theory) -> Clasp::Asp::LogicProgram const & {
    return cpp_cast(theory)->clasp_program();
}

extern "C" auto clingo_base_atoms_size(clingo_base_t const *base, size_t *size) -> bool {
    CLINGO_TRY {
        if (base == nullptr || size == nullptr) {
            return fail_arguments();
        }
        *size = get_base(base).atoms().size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_atoms_at(clingo_base_t const *bases, size_t index, clingo_signature_t *signature,
                                     clingo_atom_base_t const **atoms) -> bool {
    CLINGO_TRY {
        if (bases == nullptr || signature == nullptr) {
            return fail_arguments();
        }
        auto it = get_base(bases).atoms().nth(index);
        if (atoms != nullptr) {
            *atoms = reinterpret_cast<clingo_atom_base_t const *>(it->second.get()); // NOLINT
        }
        if (signature != nullptr) {
            signature->name = get<0>(it.key()).view().data();
            signature->size = get<0>(it.key()).size();
            signature->arity = get<1>(it.key());
            signature->is_positive = !get<2>(it.key());
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_atoms_find(clingo_base_t const *bases, clingo_signature_t const *signature,
                                       clingo_atom_base_t const **atoms, bool *found) -> bool {
    CLINGO_TRY {
        if (bases == nullptr || signature == nullptr) {
            return fail_arguments();
        }
        auto const &atms = get_base(bases).atoms();
        auto sig =
            std::tuple{std::string_view{signature->name, signature->size}, signature->arity, !signature->is_positive};
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

extern "C" auto clingo_atom_base_size(clingo_atom_base_t const *atoms, size_t *size) -> bool {
    CLINGO_TRY {
        if (atoms == nullptr || size == nullptr) {
            return fail_arguments();
        }
        *size = cpp_cast(atoms)->size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_is_fact(clingo_base_t const *atoms, clingo_literal_t literal, bool *is_fact) -> bool {
    CLINGO_TRY {
        if (atoms == nullptr || is_fact == nullptr) {
            return fail_arguments();
        }
        *is_fact = get_program(atoms).isFact(std::abs(literal));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_is_shown(clingo_base_t const *atoms, clingo_literal_t literal, bool *is_shown) -> bool {
    CLINGO_TRY {
        if (atoms == nullptr || is_shown == nullptr) {
            return fail_arguments();
        }
        *is_shown = get_program(atoms).isShown(std::abs(literal));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_is_projected(clingo_base_t const *atoms, clingo_literal_t literal, bool *is_projected)
    -> bool {
    CLINGO_TRY {
        if (atoms == nullptr || is_projected == nullptr) {
            return fail_arguments();
        }
        *is_projected = get_program(atoms).isProjected(std::abs(literal));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_is_external(clingo_base_t const *atoms, clingo_literal_t literal, bool *is_external)
    -> bool {
    CLINGO_TRY {
        if (atoms == nullptr || is_external == nullptr) {
            return fail_arguments();
        }
        *is_external = get_program(atoms).isExternal(std::abs(literal));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_is_current(clingo_base_t const *atoms, clingo_literal_t literal, bool *is_current) -> bool {
    CLINGO_TRY {
        if (atoms == nullptr || is_current == nullptr) {
            return fail_arguments();
        }
        auto atom = static_cast<clingo_atom_t>(std::abs(literal));
        *is_current = get_program(atoms).isNew(atom);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_atom_base_symbol(clingo_atom_base_t const *atoms, size_t index, clingo_symbol_t *symbol)
    -> bool {
    CLINGO_TRY {
        if (atoms == nullptr || symbol == nullptr) {
            return fail_arguments();
        }
        if (index < cpp_cast(atoms)->size()) {
            *symbol = Clingo::Symbol::to_rep(cpp_cast(atoms)->nth(index)->first);
        } else {
            return fail_with(clingo_result_range, "index out of range");
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_atom_base_literal(clingo_atom_base_t const *atoms, size_t index, clingo_literal_t *literal)
    -> bool {
    CLINGO_TRY {
        if (atoms == nullptr || literal == nullptr) {
            return fail_arguments();
        }
        if (index < cpp_cast(atoms)->size()) {
            *literal = static_cast<clingo_literal_t>(cpp_cast(atoms)->nth(index)->second.id);
        } else {
            return fail_with(clingo_result_range, "index out of range");
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_atom_base_find(clingo_atom_base_t const *atoms, clingo_symbol_t symbol, size_t *index) -> bool {
    CLINGO_TRY {
        if (atoms == nullptr || index == nullptr) {
            return fail_arguments();
        }
        *index = cpp_cast(atoms)->index(cpp_cast(symbol));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_terms(clingo_base_t const *base, clingo_term_base_t const **terms) -> bool {
    CLINGO_TRY {
        if (base == nullptr || terms == nullptr) {
            return fail_arguments();
        }
        // NOLINTNEXTLINE
        *terms = reinterpret_cast<clingo_term_base_t const *>(base);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_term_base_size(clingo_term_base_t const *terms, size_t *size) -> bool {
    CLINGO_TRY {
        if (terms == nullptr || size == nullptr) {
            return fail_arguments();
        }
        *size = cpp_cast(terms)->term_base().size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_term_base_symbol(clingo_term_base_t const *terms, size_t index, clingo_symbol_t *term) -> bool {
    CLINGO_TRY {
        if (terms == nullptr || term == nullptr) {
            return fail_arguments();
        }
        if (index < cpp_cast(terms)->term_base().size()) {
            *term = Clingo::Symbol::to_rep(cpp_cast(terms)->term_base().symbol(index));
        } else {
            return fail_with(clingo_result_range, "index out of range");
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_term_base_condition(clingo_term_base_t const *terms, size_t index, size_t const **sizes,
                                           clingo_literal_t const *const **literals, size_t *size) -> bool {
    CLINGO_TRY {
        if (terms == nullptr || literals == nullptr || size == nullptr) {
            return fail_arguments();
        }
        auto term_id = cpp_cast(terms)->term_base().term_id(index);
        thread_local auto res_lits = std::vector<clingo_literal_t const *>{};
        thread_local auto res_sizes = std::vector<size_t>{};
        res_lits.clear();
        res_sizes.clear();
        uint32_t sz = 0;
        for (auto cond : cpp_cast(terms)->clasp_program().getShowTerm(term_id).conditions()) {
            res_lits.emplace_back(std::bit_cast<clingo_literal_t const *>(cond.data()));
            res_sizes.emplace_back(cond.size());
            ++sz;
        }
        *literals = res_lits.data();
        *sizes = res_sizes.data();
        *size = sz;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_term_base_find(clingo_term_base_t const *terms, clingo_symbol_t symbol, size_t *index) -> bool {
    CLINGO_TRY {
        if (terms == nullptr || index == nullptr) {
            return fail_arguments();
        }
        *index = cpp_cast(terms)->term_base().index(cpp_cast(symbol));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_base(clingo_control_t const *control, clingo_base_t const **base) -> bool {
    CLINGO_TRY {
        if (control == nullptr || base == nullptr) {
            return fail_arguments();
        }
        // NOLINTNEXTLINE
        *base = reinterpret_cast<clingo_base_t const *>(control->slv);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_base_theory(clingo_base_t const *base, clingo_theory_base_t const **theory) -> bool {
    CLINGO_TRY {
        if (base == nullptr || theory == nullptr) {
            return fail_arguments();
        }
        // NOLINTNEXTLINE
        *theory = reinterpret_cast<clingo_theory_base_t const *>(base);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_term_type(clingo_theory_base_t const *theory, clingo_id_t term,
                                             clingo_theory_term_type_t *type) -> bool {
    CLINGO_TRY {
        if (theory == nullptr || type == nullptr) {
            return fail_arguments();
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
    -> bool {
    CLINGO_TRY {
        if (theory == nullptr || number == nullptr) {
            return fail_arguments();
        }
        *number = get_theory(theory).getTerm(term).number();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_term_name(clingo_theory_base_t const *theory, clingo_id_t term, char const **name,
                                             size_t *size) -> bool {
    CLINGO_TRY {
        if (theory == nullptr || name == nullptr || name == nullptr) {
            return fail_arguments();
        }
        auto const &t = get_theory(theory);
        auto x = t.getTerm(term);
        auto str = std::string_view{x.isFunction() ? t.getTerm(x.function()).symbol() : x.symbol()};
        *name = str.data();
        *size = str.size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_term_arguments(clingo_theory_base_t const *theory, clingo_id_t term,
                                                  clingo_id_t const **arguments, size_t *size) -> bool {
    CLINGO_TRY {
        if (theory == nullptr || arguments == nullptr || size == nullptr) {
            return fail_arguments();
        }
        auto args = get_theory(theory).getTerm(term).terms();
        *arguments = args.data();
        *size = args.size();
    }
    CLINGO_CATCH;
}

namespace {

[[nodiscard]] auto element_condition(clingo_theory_base_t const *theory, clingo_id_t id)
    -> std::span<clingo_literal_t const> {
    static thread_local auto cond = Potassco::LitVec{};
    cond.clear();
    get_program(theory).extractCondition(get_theory(theory).getElement(id).condition(), cond);
    return cond;
}

[[nodiscard]] auto get_atom(clingo_theory_base_t const *theory, clingo_id_t id) -> Potassco::TheoryAtom const & {
    auto atoms = get_theory(theory).atoms();
    return id < atoms.size() ? *atoms[id] : throw std::range_error("atom index out of range");
}

class TheoryPrinter {
  public:
    TheoryPrinter(clingo_theory_base_t const *theory, clingo_string_builder_t *builder)
        : theory_{theory}, out_{cpp_cast(builder)} {}

    void term(clingo_id_t id) const {
        auto x = data_().getTerm(id);
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
                using Clingo::Util::p_range;
                auto args = x.terms();
                if (x.isFunction()) {
                    auto const *name = data_().getTerm(x.function()).symbol();
                    if (Clingo::Input::is_theory_operator(name)) {
                        assert(!args.empty() && args.size() <= 2);
                        *out_ << "(";
                        if (args.size() >= 2) {
                            term(args.front());
                        }
                        *out_ << name;
                        if (!args.empty()) {
                            term(args.back());
                        }
                        *out_ << ")";
                        break;
                    }
                    *out_ << name;
                }
                auto p = Potassco::parens(x.isFunction() ? Potassco::TupleType::paren : x.tuple());
                *out_ << p[0] << p_range(args, [this]([[maybe_unused]] auto &out, auto const &y) { term(y); }) << p[1];
                break;
            }
        }
    }
    void elem(clingo_id_t id) const {
        using Clingo::Util::p_range;
        auto const &x = data_().getElement(id);
        *out_ << p_range(x.terms(), [this]([[maybe_unused]] auto &out, auto const &y) { term(y); });
        auto cond = element_condition(theory_, id);
        if (!cond.empty()) {
            *out_ << ": " << p_range(cond, ", ", [](auto &out, auto const &y) {
                // NOTE: the previous clingo version made more effort here
                // a straight-forward implementation would have to loop over the atom base
                out << "<literal: " << y << ">";
            });
        }
    }

    void atom(clingo_id_t id) const {
        auto const &atom = get_atom(theory_, id);
        *out_ << "&";
        term(atom.term());
        *out_ << " {";
        char const *sep = " ";
        for (auto const &x : atom.elements()) {
            *out_ << sep;
            sep = "; ";
            elem(x);
        }
        *out_ << " }";
        if (auto const *guard = atom.guard(); guard != nullptr) {
            *out_ << " ";
            term(*guard);
        }
        if (auto const *rhs = atom.rhs(); rhs != nullptr) {
            *out_ << " ";
            term(*rhs);
        }
    }

    [[nodiscard]] auto data_() const -> Potassco::TheoryData const & { return get_theory(theory_); }

  private:
    clingo_theory_base_t const *theory_;
    Clingo::Util::OutputBuffer *out_;
};

} // namespace

extern "C" auto clingo_theory_base_term_to_string(clingo_theory_base_t const *theory, clingo_id_t term,
                                                  clingo_string_builder_t *builder) -> bool {
    CLINGO_TRY {
        if (theory == nullptr || builder == nullptr) {
            return fail_arguments();
        }
        TheoryPrinter{theory, builder}.term(term);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_element_tuple(clingo_theory_base_t const *theory, clingo_id_t element,
                                                 clingo_id_t const **tuple, size_t *size) -> bool {
    CLINGO_TRY {
        if (theory == nullptr) {
            return fail_arguments();
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
                                                     clingo_literal_t const **condition, size_t *size) -> bool {
    CLINGO_TRY {
        if (theory == nullptr) {
            return fail_arguments();
        }
        auto cond = element_condition(theory, element);
        if (condition != nullptr) {
            *condition = cond.data();
        }
        if (size != nullptr) {
            *size = cond.size();
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_element_condition_id(clingo_theory_base_t const *theory, clingo_id_t element,
                                                        clingo_literal_t *condition) -> bool {
    CLINGO_TRY {
        if (theory == nullptr || condition == nullptr) {
            return fail_arguments();
        }
        *condition = static_cast<clingo_literal_t>(get_theory(theory).getElement(element).condition());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_element_to_string(clingo_theory_base_t const *theory, clingo_id_t element,
                                                     clingo_string_builder_t *builder) -> bool {
    CLINGO_TRY {
        if (theory == nullptr || builder == nullptr) {
            return fail_arguments();
        }
        TheoryPrinter{theory, builder}.elem(element);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_size(clingo_theory_base_t const *theory, size_t *size) -> bool {
    CLINGO_TRY {
        if (theory == nullptr || size == nullptr) {
            return fail_arguments();
        }
        *size = get_theory(theory).numAtoms();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_atom_term(clingo_theory_base_t const *theory, clingo_id_t atom, clingo_id_t *term)
    -> bool {
    CLINGO_TRY {
        if (theory == nullptr || term == nullptr) {
            return fail_arguments();
        }
        *term = get_theory(theory).atoms()[atom]->term();
        *term = get_atom(theory, atom).term();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_atom_elements(clingo_theory_base_t const *theory, clingo_id_t atom,
                                                 clingo_id_t const **elements, size_t *size) -> bool {
    CLINGO_TRY {
        if (theory == nullptr) {
            return fail_arguments();
        }
        auto elems = get_atom(theory, atom).elements();
        if (elements != nullptr) {
            *elements = elems.data();
        }
        if (size != nullptr) {
            *size = elems.size();
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_atom_has_guard(clingo_theory_base_t const *theory, clingo_id_t atom, bool *has_guard)
    -> bool {
    CLINGO_TRY {
        if (theory == nullptr || has_guard == nullptr) {
            return fail_arguments();
        }
        *has_guard = get_atom(theory, atom).guard() != nullptr;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_atom_guard(clingo_theory_base_t const *theory, clingo_id_t atom,
                                              char const **connective, size_t *size, clingo_id_t *term) -> bool {
    CLINGO_TRY {
        if (theory == nullptr) {
            return fail_arguments();
        }
        auto const &x = get_atom(theory, atom);
        if (connective != nullptr || size != nullptr) {
            auto const *guard = x.guard();
            if (guard == nullptr) {
                return fail_with(clingo_result_runtime, "guard expected");
            }
            auto str = std::string_view{get_theory(theory).getTerm(*guard).symbol()};
            if (connective != nullptr) {
                *connective = str.data();
            }
            if (connective != nullptr) {
                *size = str.size();
            }
        }
        if (term != nullptr) {
            auto const *rhs = x.rhs();
            if (rhs == nullptr) {
                return fail_with(clingo_result_runtime, "guard expected");
            }
            *term = *rhs;
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_atom_literal(clingo_theory_base_t const *theory, clingo_id_t atom,
                                                clingo_literal_t *literal) -> bool {
    CLINGO_TRY {
        if (theory == nullptr || literal == nullptr) {
            return fail_arguments();
        }
        *literal = static_cast<clingo_literal_t>(get_atom(theory, atom).atom());
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_theory_base_atom_to_string(clingo_theory_base_t const *theory, clingo_id_t atom,
                                                  clingo_string_builder_t *builder) -> bool {
    CLINGO_TRY {
        if (theory == nullptr || builder == nullptr) {
            return fail_arguments();
        }
        TheoryPrinter{theory, builder}.atom(atom);
    }
    CLINGO_CATCH;
}
