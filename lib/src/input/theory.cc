#include <sstream>

#include <util/print.hh>

#include <input/theory.hh>

#include <input/algo/unpool.hh>
#include <input/algo/visit_variables.hh>

#include "algo/transform.hh"
#include "algo/unpool.hh"
#include "algo/variables.hh"

namespace Gringo::Input {

////////// TheoryTermUnparsed //////////

void TheoryTermUnparsed::accept(TheoryTermVisitor const &visitor) const { visitor.visit(*this); }

void TheoryTermUnparsed::visit_variables(VarVisitFun fun) const {
    for (auto const &elem : elems_) {
        elem.second->visit_variables(fun);
    }
}

////////// TheoryTermTuple //////////

void TheoryTermTuple::accept(TheoryTermVisitor const &visitor) const { visitor.visit(*this); }

void TheoryTermTuple::visit_variables(VarVisitFun fun) const {
    for (auto const &term : elems_) {
        term->visit_variables(fun);
    }
}

////////// TheoryTermConstant //////////

void TheoryTermSymbol::accept(TheoryTermVisitor const &visitor) const { visitor.visit(*this); }

void TheoryTermSymbol::visit_variables(VarVisitFun fun) const { static_cast<void>(fun); }

////////// TheoryTermVariable //////////

void TheoryTermVariable::accept(TheoryTermVisitor const &visitor) const { visitor.visit(*this); }

void TheoryTermVariable::visit_variables(VarVisitFun fun) const { fun(name_); }

////////// TheoryTermFunction //////////

void TheoryTermFunction::accept(TheoryTermVisitor const &visitor) const { visitor.visit(*this); }

void TheoryTermFunction::visit_variables(VarVisitFun fun) const {
    for (auto const &term : args_) {
        term->visit_variables(fun);
    }
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
            [](Term const &name) {
                using Gringo::Input::unpool;
                return unpool(name);
            },
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

auto TheoryAtom::project_anonymous() const -> std::optional<TheoryAtom> {
    auto fun = [](SLiteral const &lit) { return lit->project_anonymous(); };
    return transform_construct<TheoryAtom>(name_, Trans{elems_, fun}, rhs_);
}

} // namespace Gringo::Input
