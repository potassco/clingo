#include <sstream>

#include <util/print.hh>

#include <theory.hh>

#include "transform.hh"
#include "unpool.hh"
#include "variables.hh"

////////// TheoryTerm //////////

[[nodiscard]] auto TheoryTerm::to_string() const -> std::string {
    std::ostringstream out;
    out << *this;
    return out.str();
}

auto operator<<(std::ostream &out, TheoryTerm const &term) -> std::ostream & {
    term.print(out);
    return out;
}

////////// TheoryTermUnparsed //////////

void TheoryTermUnparsed::print(std::ostream &out) const {
    bool needs_parens = !ops_.empty() || !rhs_.empty();
    if (needs_parens) {
        out << "(";
    }
    if (!ops_.empty()) {
        out << p_range(ops_, " ") << " ";
    }
    out << *term_ << p_range_with(rhs_, [](std::ostream &out, RHS const &guard) {
        out << " " << p_range(guard.first, " ") << " " << *guard.second;
    });
    if (needs_parens) {
        out << ")";
    }
}

void TheoryTermUnparsed::visit_variables(VarVisitFun fun) const {
    term_->visit_variables(fun);
    for (auto const &guard : rhs_) {
        guard.second->visit_variables(fun);
    }
}

[[nodiscard]] auto TheoryTermUnparsed::rewrite_anonymous(NameGen &gen) const -> std::optional<STheoryTerm> {
    auto fun = [&gen](STheoryTerm const &term) { return term->rewrite_anonymous(gen); };
    return transform_construct_shared<TheoryTermUnparsed, TheoryTerm>(ops_, Trans{term_, fun}, Trans{rhs_, fun});
}

////////// TheoryTermTuple //////////

auto left_bracket(TheoryTermTupleType type) -> char {
    switch (type) {
        case TheoryTermTupleType::Tuple: {
            return '(';
        }
        case TheoryTermTupleType::Set: {
            return '{';
        }
        case TheoryTermTupleType::List: {
            break;
        }
    }
    return '[';
}

auto right_bracket(TheoryTermTupleType type) -> char {
    switch (type) {
        case TheoryTermTupleType::Tuple: {
            return ')';
        }
        case TheoryTermTupleType::Set: {
            return '}';
        }
        case TheoryTermTupleType::List: {
            break;
        }
    }
    return ']';
}

void TheoryTermTuple::print(std::ostream &out) const {
    out << left_bracket(type_) << p_range(elems_);
    if (type_ == TheoryTermTupleType::Tuple && elems_.size() == 1) {
        out << ",";
    }
    out << right_bracket(type_);
}

void TheoryTermTuple::visit_variables(VarVisitFun fun) const {
    for (auto const &term : elems_) {
        term->visit_variables(fun);
    }
}

[[nodiscard]] auto TheoryTermTuple::rewrite_anonymous(NameGen &gen) const -> std::optional<STheoryTerm> {
    auto fun = [&gen](STheoryTerm const &term) { return term->rewrite_anonymous(gen); };
    return transform_construct_shared<TheoryTermTuple, TheoryTerm>(type_, Trans{elems_, fun});
}

////////// TheoryTermConstant //////////

void TheoryTermSymbol::print(std::ostream &out) const { out << value_; }

void TheoryTermSymbol::visit_variables(VarVisitFun fun) const { static_cast<void>(fun); }

[[nodiscard]] auto TheoryTermSymbol::rewrite_anonymous(NameGen &gen) const -> std::optional<STheoryTerm> {
    return std::nullopt;
}

////////// TheoryTermVariable //////////

void TheoryTermVariable::print(std::ostream &out) const { out << name_; }

void TheoryTermVariable::visit_variables(VarVisitFun fun) const { fun(name_); }

[[nodiscard]] auto TheoryTermVariable::rewrite_anonymous(NameGen &gen) const -> std::optional<STheoryTerm> {
    if (name_ == "_") {
        return construct_shared<TheoryTermVariable, TheoryTerm>(gen.new_name());
    }
    return std::nullopt;
}

////////// TheoryTermFunction //////////

void TheoryTermFunction::print(std::ostream &out) const {
    out << name_;
    if (!args_.empty()) {
        out << "(" << p_range(args_) << ")";
    }
}

void TheoryTermFunction::visit_variables(VarVisitFun fun) const {
    for (auto const &term : args_) {
        term->visit_variables(fun);
    }
}

[[nodiscard]] auto TheoryTermFunction::rewrite_anonymous(NameGen &gen) const -> std::optional<STheoryTerm> {
    auto fun = [&gen](STheoryTerm const &term) { return term->rewrite_anonymous(gen); };
    return transform_construct_shared<TheoryTermFunction, TheoryTerm>(name_, Trans{args_, fun});
}

////////// TheoryAtom //////////

auto TheoryAtom::unpool() const -> std::optional<std::vector<TheoryAtom>> {
    return unpool_crossproducts(
        [&](auto name, auto elems) {
            return TheoryAtom{std::move(name), std::move(elems), rhs_};
        },
        overloaded{
            [](ElementVec const &elems) -> std::optional<std::vector<ElementVec>> {
                return map_opt(
                    unpool_union(elems,
                                 [](auto elem) {
                                     return unpool_crossproducts(
                                         [&elem](auto cond) {
                                             return Element{std::get<0>(elem), std::move(cond)};
                                         },
                                         overloaded{[](SLiteralVec const &lits) { return unpool_crossproduct(lits); }},
                                         std::get<1>(elem));
                                 }),
                    [](auto elems) { return make_vec<ElementVec>(std::move(elems)); });
            },
            [](STerm const &name) { return name->unpool(); },
        },
        name_, elems_);
}

auto operator<<(std::ostream &out, TheoryAtom const &atom) -> std::ostream & {
    out << "&" << *atom.name_;
    if (!atom.elems_.empty() || atom.rhs_.has_value()) {
        out << " { " << p_range_with(atom.elems_, "; ", [](std::ostream &out, TheoryAtom::Element const &elem) {
            out << p_range(elem.first);
            if (!elem.second.empty() || elem.first.empty()) {
                out << ": " << p_range(elem.second, ", ");
            }
        }) << (atom.elems_.empty() ? "}" : " }");
    }
    if (atom.rhs_.has_value()) {
        out << " " << atom.rhs_.value().first << " " << *atom.rhs_.value().second;
    }
    return out;
}

void TheoryAtom::visit_variables(VarVisitFun fun, VariableContext ctx) const {
    VarVisitor visit{std::move(fun)};
    visit.add(name_, rhs_);
    if (ctx == VariableContext::all) {
        visit.add(elems_);
    }
}

auto TheoryAtom::rewrite_anonymous(NameGen &gen) const -> std::optional<TheoryAtom> {
    auto fun = overloaded{[&gen](STerm const &term) { return term->rewrite_anonymous(gen); },
                          [&gen](STheoryTerm const &term) { return term->rewrite_anonymous(gen); },
                          [&gen](SLiteral const &lit) { return lit->rewrite_anonymous(gen); }};
    return transform_construct<TheoryAtom>(Trans{name_, fun}, Trans{elems_, fun}, Trans{rhs_, fun});
}
