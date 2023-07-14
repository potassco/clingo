#include <sstream>

#include <util/print.hh>

#include <input/theory.hh>

#include "transform.hh"
#include "unpool.hh"
#include "variables.hh"

namespace Gringo::Input {

////////// TheoryTermUnparsed //////////

void TheoryTermUnparsed::accept(TheoryTermVisitor const &visitor) const { visitor.visit(*this); }

void TheoryTermUnparsed::visit_variables(VarVisitFun fun) const {
    for (auto const &elem : elems_) {
        elem.second->visit_variables(fun);
    }
}

[[nodiscard]] auto TheoryTermUnparsed::rewrite_anonymous(NameGen &gen) const -> std::optional<STheoryTerm> {
    auto fun = [&gen](STheoryTerm const &term) { return term->rewrite_anonymous(gen); };
    return transform_construct_shared<TheoryTermUnparsed, TheoryTerm>(Trans{elems_, fun});
}

////////// TheoryTermTuple //////////

void TheoryTermTuple::accept(TheoryTermVisitor const &visitor) const { visitor.visit(*this); }

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

void TheoryTermSymbol::accept(TheoryTermVisitor const &visitor) const { visitor.visit(*this); }

void TheoryTermSymbol::visit_variables(VarVisitFun fun) const { static_cast<void>(fun); }

[[nodiscard]] auto TheoryTermSymbol::rewrite_anonymous(NameGen &gen) const -> std::optional<STheoryTerm> {
    static_cast<void>(gen);
    return std::nullopt;
}

////////// TheoryTermVariable //////////

void TheoryTermVariable::accept(TheoryTermVisitor const &visitor) const { visitor.visit(*this); }

void TheoryTermVariable::visit_variables(VarVisitFun fun) const { fun(name_); }

[[nodiscard]] auto TheoryTermVariable::rewrite_anonymous(NameGen &gen) const -> std::optional<STheoryTerm> {
    if (is_anonymous_) {
        return Util::construct_shared<TheoryTermVariable, TheoryTerm>(gen.new_name(), true);
    }
    return std::nullopt;
}

////////// TheoryTermFunction //////////

void TheoryTermFunction::accept(TheoryTermVisitor const &visitor) const { visitor.visit(*this); }

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
        Util::overloaded{
            [](ElementVec const &elems) -> std::optional<std::vector<ElementVec>> {
                return map_opt(unpool_union(elems,
                                            [](auto elem) {
                                                return unpool_crossproducts(
                                                    [&elem](auto cond) {
                                                        return Element{std::get<0>(elem), std::move(cond)};
                                                    },
                                                    Util::overloaded{[](SLiteralVec const &lits) {
                                                        return unpool_crossproduct(lits);
                                                    }},
                                                    std::get<1>(elem));
                                            }),
                               [](auto elems) { return make_vec<ElementVec>(std::move(elems)); });
            },
            [](STerm const &name) { return name->unpool(); },
        },
        name_, elems_);
}

void TheoryAtom::visit_variables(VarVisitFun fun, VariableContext ctx) const {
    VarVisitor visit{std::move(fun)};
    visit.add(name_, rhs_);
    if (ctx == VariableContext::all) {
        visit.add(elems_);
    }
}

auto TheoryAtom::rewrite_anonymous(NameGen &gen) const -> std::optional<TheoryAtom> {
    auto fun = Util::overloaded{[&gen](STerm const &term) { return term->rewrite_anonymous(gen); },
                                [&gen](STheoryTerm const &term) { return term->rewrite_anonymous(gen); },
                                [&gen](SLiteral const &lit) { return lit->rewrite_anonymous(gen); }};
    return transform_construct<TheoryAtom>(Trans{name_, fun}, Trans{elems_, fun}, Trans{rhs_, fun});
}

auto TheoryAtom::project_anonymous() const -> std::optional<TheoryAtom> {
    auto fun = [](SLiteral const &lit) { return lit->project_anonymous(); };
    return transform_construct<TheoryAtom>(name_, Trans{elems_, fun}, rhs_);
}

} // namespace Gringo::Input
