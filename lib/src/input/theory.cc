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
