#include <clingo/control/literal.hh>
#include <clingo/control/term.hh>
#include <clingo/control/theory.hh>

#include <clingo/ground/theory_atom.hh>

namespace CppClingo::Control {

// Outline for body atoms:
//
// H :- B, &pred { E1; ...; En }.
// - theory atoms always match
// - there is no need to apply seminaive evaluation to conditions
// - elements can be grounded just before output
// - the theory atom T always matches
//     H :- B, T.
// - when the atom is output
//     accu :- T, E1.
//     ...
//     accu :- T, En.
// - differences to aggregates:
//   - linearization of E1 to En has to be avoided
//   - positive literals are marked as single pass
//   - negative literals stay keep their recursive index
//
// Head theory atoms are handled in a very similar way.

namespace {

template <bool HasSign>
auto build_state(BuildContext &ctx, Input::TheoryAtom<HasSign> const &lit) -> Ground::StateTheory & {
    auto vars_body = Ground::VariableSet{};
    auto vars_global = Ground::VariableSet{};
    for (auto const &lit : ctx.body()) {
        lit->vars(vars_body, Ground::VarSelectMode::all);
    }

    // handle guard
    auto guard = std::optional<std::pair<String, Ground::UTheoryTerm>>{};
    if (auto const &rhs = lit.rhs()) {
        guard.emplace(rhs->op(), build_theory_term(ctx.var_map(), rhs->term()));
        guard->second->vars(vars_global);
    }

    // handle name
    auto name = build_term(ctx.var_map(), lit.name());
    name->vars(vars_global);

    // handle elems
    auto elems = std::vector<std::pair<Ground::UTheoryTermVec, Ground::ULitVec>>{};
    elems.reserve(lit.elems().size());
    for (auto const &elem : lit.elems()) {
        elems.emplace_back();
        auto &[tuple, cond] = elems.back();
        auto vars = Ground::VariableSet{};
        // tuple
        tuple.reserve(elem.tuple().size());
        for (auto const &term : elem.tuple()) {
            tuple.emplace_back(build_theory_term(ctx.var_map(), term));
            tuple.back()->vars(vars);
        }
        // condition
        cond.reserve(elem.cond().size() + 1);
        for (auto const &slit : elem.cond()) {
            build_stratified_lit(ctx, slit, [&cond, &vars]<class Lit>(Lit &&glit) {
                glit->vars(vars, Ground::VarSelectMode::all);
                cond.emplace_back(std::forward<Lit>(glit));
            });
        }
        // global variables
        for (auto const &var : vars) {
            if (vars_body.contains(var)) {
                vars_global.emplace(var);
            }
        }
    }

    // add theory state
    auto type = HasSign ? OutputTheory::AtomType::body : OutputTheory::AtomType::head;
    if constexpr (!HasSign) {
        if (auto sig = Input::signature(lit.name()); sig && ctx.is_theory_directive({get<0>(*sig), get<1>(*sig)})) {
            type = OutputTheory::AtomType::directive;
        }
    }
    auto &state =
        ctx.state<Ground::StateTheory>(ctx.mbr(), vars_global.release(), std::move(name), std::move(guard), type);

    // add elements to state
    auto stms = Ground::UStmVec{};
    stms.reserve(elems.size());
    for (auto &elem : elems) {
        elem.second.emplace_back(std::make_unique<Ground::LitMatchTheory>(state));
        stms.emplace_back(
            std::make_unique<Ground::StmTheoryElement>(state, std::move(elem.first), std::move(elem.second)));
    }
    state.elems(std::move(stms));

    return state;
}

} // namespace

void build_hd_lit(BuildContext &ctx, Input::HdLitTheoryAtom const &lit, Ground::ProfileNodeInternal *node) {
    auto &state = build_state(ctx, lit);
    ctx.gcomp().add(std::make_unique<Ground::StmHdTheory>(state, std::move(ctx.body())));
}

//! Translate a body theory atom.
void build_bd_lit(BuildContext &ctx, Input::BdLitTheoryAtom const &lit, Ground::ProfileNodeInternal *node) {
    auto &state = build_state(ctx, lit);
    ctx.body().emplace_back(std::make_unique<Ground::LitBdTheory>(state, lit.sign()));
}

} // namespace CppClingo::Control
