#include <sstream>

#include <util/print.hh>

#include <input/theory.hh>

#include <input/algo/project_anonymous.hh>
#include <input/algo/unpool.hh>
#include <input/algo/visit_variables.hh>

#include "algo/transform.hh"
#include "algo/unpool.hh"
#include "algo/variables.hh"

namespace Gringo::Input {

auto TheoryAtom::unpool() const -> std::optional<std::vector<TheoryAtom>> {
    using Gringo::Input::unpool;
    return unpool_crossproducts(
        [&](auto name, auto elems) {
            return TheoryAtom{std::move(name), std::move(elems), rhs_};
        },
        Util::overloaded{
            [](ElementVec const &elems) -> std::optional<std::vector<ElementVec>> {
                return Util::map_opt(unpool_union(elems,
                                                  [](auto elem) {
                                                      return unpool_crossproducts(
                                                          [&elem](auto cond) {
                                                              return Element{std::get<0>(elem), std::move(cond)};
                                                          },
                                                          Util::overloaded{[](LiteralVec const &lits) {
                                                              return unpool_crossproduct(
                                                                  lits, [](auto const &lit) { return unpool(lit); });
                                                          }},
                                                          std::get<1>(elem));
                                                  }),
                                     [](auto elems) { return Util::make_vec<ElementVec>(std::move(elems)); });
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
    using Gringo::Input::project_anonymous;
    auto fun = [](Literal const &lit) { return project_anonymous(lit); };
    return transform_construct<TheoryAtom>(name_, Trans{elems_, fun}, rhs_);
}

} // namespace Gringo::Input
