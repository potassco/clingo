#include <clingo/ground/theory_term.hh>

#include <typeindex>

namespace CppClingo::Ground {

// definition of TheoryTermSymbol

void TheoryTermSymbol::do_vars([[maybe_unused]] VariableSet &vars) const {
}

void TheoryTermSymbol::do_print(std::ostream &out) const {
    out << sym_;
}

auto TheoryTermSymbol::do_output([[maybe_unused]] EvalContext const &ctx, OutputTheory &out) const -> size_t {
    return out.sym(sym_);
}

auto TheoryTermSymbol::do_copy() const -> UTheoryTerm {
    return std::make_unique<TheoryTermSymbol>(sym_);
}

auto TheoryTermSymbol::do_hash() const -> size_t {
    return Util::value_hash_record<TheoryTermSymbol>(sym_);
}

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

void TheoryTermVariable::do_vars([[maybe_unused]] VariableSet &vars) const {
    vars.emplace(var_);
}

void TheoryTermVariable::do_print(std::ostream &out) const {
    out << "X_" << var_;
}

auto TheoryTermVariable::do_output(EvalContext const &ctx, OutputTheory &out) const -> size_t {
    assert(ctx.ass()[var_]);
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    return out.sym(*ctx.ass()[var_]);
}

auto TheoryTermVariable::do_copy() const -> UTheoryTerm {
    return std::make_unique<TheoryTermVariable>(var_);
}

auto TheoryTermVariable::do_hash() const -> size_t {
    return Util::value_hash_record<TheoryTermVariable>(var_);
}

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

// definition of TheoryTermTuple

void TheoryTermTuple::do_vars(VariableSet &vars) const {
    for (auto const &arg : args_) {
        arg->vars(vars);
    }
}

void TheoryTermTuple::do_print(std::ostream &out) const {
    out << "(" << Util::p_range(args_, [](auto &out, auto const &x) { out << *x; }) << (args_.size() == 1 ? ",)" : ")");
}

auto TheoryTermTuple::do_output(EvalContext const &ctx, OutputTheory &out) const -> size_t {
    auto args = std::vector<size_t>{};
    args.reserve(args_.size());
    for (auto const &arg : args_) {
        args.emplace_back(arg->output(ctx, out));
    }
    return out.tup(type_, args);
}

auto TheoryTermTuple::do_copy() const -> UTheoryTerm {
    return std::make_unique<TheoryTermTuple>(type_, copy_uvec(args_));
}

auto TheoryTermTuple::do_hash() const -> size_t {
    return Util::value_hash_record<TheoryTermTuple>(args_);
}

auto TheoryTermTuple::do_equal_to(TheoryTerm const &other) const -> bool {
    auto const *x = dynamic_cast<TheoryTermTuple const *>(&other);
    return x != nullptr && type_ == x->type_ && Util::value_equal_to{}(args_, x->args_);
}

auto TheoryTermTuple::do_compare_to(TheoryTerm const &other) const -> std::strong_ordering {
    auto const *x = dynamic_cast<TheoryTermTuple const *>(&other);
    if (x != nullptr) {
        if (auto n = type_ <=> x->type_; std::is_neq(n)) {
            return n;
        }
        return std::lexicographical_compare_three_way(args_.begin(), args_.end(), x->args_.begin(), x->args_.end(),
                                                      [](auto const &a, auto const &b) { return *a <=> *b; });
    }
    return std::type_index(typeid(this)) <=> std::type_index(typeid(other));
}

// definition of TheoryTermFunction

void TheoryTermFunction::do_vars(VariableSet &vars) const {
    for (auto const &arg : args_) {
        arg->vars(vars);
    }
}

void TheoryTermFunction::do_print(std::ostream &out) const {
    out << name_;
    if (!args_.empty()) {
        out << "(" << Util::p_range(args_, [](auto &out, auto const &x) { out << *x; }) << ")";
    }
}

auto TheoryTermFunction::do_output(EvalContext const &ctx, OutputTheory &out) const -> size_t {
    auto args = std::vector<size_t>{};
    args.reserve(args_.size());
    for (auto const &arg : args_) {
        args.emplace_back(arg->output(ctx, out));
    }
    return out.fun(name_, args);
}

auto TheoryTermFunction::do_copy() const -> UTheoryTerm {
    return std::make_unique<TheoryTermFunction>(name_, copy_uvec(args_));
}

auto TheoryTermFunction::do_hash() const -> size_t {
    return Util::value_hash_record<TheoryTermFunction>(args_);
}

auto TheoryTermFunction::do_equal_to(TheoryTerm const &other) const -> bool {
    auto const *x = dynamic_cast<TheoryTermFunction const *>(&other);
    return x != nullptr && name_ == x->name_ && Util::value_equal_to{}(args_, x->args_);
}

auto TheoryTermFunction::do_compare_to(TheoryTerm const &other) const -> std::strong_ordering {
    auto const *x = dynamic_cast<TheoryTermFunction const *>(&other);
    if (x != nullptr) {
        if (auto n = name_ <=> x->name_; std::is_neq(n)) {
            return n;
        }
        return std::lexicographical_compare_three_way(args_.begin(), args_.end(), x->args_.begin(), x->args_.end(),
                                                      [](auto const &a, auto const &b) { return *a <=> *b; });
    }
    return std::type_index(typeid(this)) <=> std::type_index(typeid(other));
}

} // namespace CppClingo::Ground
