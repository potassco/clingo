#include <gringo/ground/theory.hh>

#include <typeindex>

namespace Gringo::Ground {

// definition of TheoryTermSymbol

void TheoryTermSymbol::do_vars([[maybe_unused]] VariableSet &vars) const {}

void TheoryTermSymbol::do_print(std::ostream &out) const { out << sym_; }

namespace {

auto output_symbol(SymbolStore &store, OutputTheory &out, Symbol sym) -> size_t {
    switch (sym.type()) {
        case SymbolType::inf: {
            return out.str(store.string_ref("#inf"));
        }
        case SymbolType::sup: {
            return out.str(store.string_ref("#sup"));
        }
        case SymbolType::string: {
            throw std::logic_error("implement me: string!!!");
        }
        case SymbolType::function: {
            throw std::logic_error("implement me: function!!!");
        }
        case SymbolType::tuple: {
            throw std::logic_error("implement me: tuple!!!");
        }
        case SymbolType::number: {
            // What to do? Maybe simply represent large numbers as string?
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            return out.num(sym.num().as_int().value());
        }
    }
}

} // namespace

auto TheoryTermSymbol::do_output(SymbolStore &store, [[maybe_unused]] Assignment const &ass,
                                 OutputTheory &out) const -> size_t {
    return output_symbol(store, out, sym_);
}

auto TheoryTermSymbol::do_copy() const -> UTheoryTerm { return std::make_unique<TheoryTermSymbol>(sym_); }

auto TheoryTermSymbol::do_hash() const -> size_t { return Util::value_hash_record<TheoryTermSymbol>(sym_); }

auto TheoryTermSymbol::do_equal_to(TheoryTerm const &other) const -> bool {
    auto const *x = dynamic_cast<TheoryTermSymbol const *>(&other);
    return x != nullptr && sym_ == x->sym_;
}

auto TheoryTermSymbol::do_compare_to(TheoryTerm const &other) const -> std::strong_ordering {
    auto const *x = dynamic_cast<TheoryTermSymbol const *>(&other);
    if (x != nullptr) {
        return sym_ <=> x->sym_;
    }
    return std::type_index(typeid(this)) <=> std::type_index(typeid(other));
}

// definition of TheoryTermVariable

void TheoryTermVariable::do_vars([[maybe_unused]] VariableSet &vars) const {}

void TheoryTermVariable::do_print(std::ostream &out) const { out << "X_" << var_; }

auto TheoryTermVariable::do_output(SymbolStore &store, Assignment const &ass, OutputTheory &out) const -> size_t {
    assert(ass[var_]);
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return output_symbol(store, out, *ass[var_]);
}

auto TheoryTermVariable::do_copy() const -> UTheoryTerm { return std::make_unique<TheoryTermVariable>(var_); }

auto TheoryTermVariable::do_hash() const -> size_t { return Util::value_hash_record<TheoryTermVariable>(var_); }

auto TheoryTermVariable::do_equal_to(TheoryTerm const &other) const -> bool {
    auto const *x = dynamic_cast<TheoryTermVariable const *>(&other);
    return x != nullptr && var_ == x->var_;
}

auto TheoryTermVariable::do_compare_to(TheoryTerm const &other) const -> std::strong_ordering {
    auto const *x = dynamic_cast<TheoryTermVariable const *>(&other);
    if (x != nullptr) {
        return var_ <=> x->var_;
    }
    return std::type_index(typeid(this)) <=> std::type_index(typeid(other));
}

} // namespace Gringo::Ground
