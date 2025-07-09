#include <clingo/control/condlit.hh>
#include <clingo/control/literal.hh>

#include <clingo/ground/condlit.hh>
#include <clingo/ground/disjunction.hh>

#include <clingo/input/rewrite/unpool_relations.hh>

#include <clingo/util/type_traits.hh>

namespace CppClingo::Control {

void build_hd_lit(BuildContext &ctx, Input::HdLitDisjunction const &lit, Ground::ProfileNodeInternal *node) {
    auto vars_body = Ground::VariableSet{};
    for (auto const &lit : ctx.body()) {
        lit->vars(vars_body, Ground::VarSelectMode::all);
    }

    auto vars_global = Ground::VariableSet{};

    using TermBase = std::pair<Ground::UTerm, Ground::AtomBase *>;
    auto elems = std::vector<std::tuple<Ground::UTerm, Ground::AtomBase *, Ground::ULitVec>>{};
    elems.reserve(lit.elems().size());
    Ground::BaseVec bases;
    bases.reserve(elems.size());
    for (auto const &elem : lit.elems()) {
        auto elem_vars = Ground::VariableSet{};
        // head
        auto head = std::optional<TermBase>{};
        std::visit(
            [&]<class T>(T const &lit) {
                if constexpr (Util::matches<T, Input::Lit>) {
                    ctx.with_simple_lit(lit, [&](auto sig, auto term, auto &base, auto provides) {
                        bases.emplace_back(sig, &base, std::move(provides));
                        head.emplace(std::make_pair(std::move(term), &base));
                    });
                } else {
                    ctx.with_simple_lit(lit.lit(), [&](auto sig, auto term, auto &base, auto provides) {
                        bases.emplace_back(sig, &base, std::move(provides));
                        head.emplace(std::make_pair(std::move(term), &base));
                    });
                }
            },
            elem);
        assert(head);
        head->first->vars(elem_vars);
        // condition
        auto cond = Ground::ULitVec{};
        if (auto const *clit = std::get_if<Input::CondLit>(&elem)) {
            cond.reserve(clit->cond().size() + 1);
            for (auto const &lit : clit->cond()) {
                build_lit(ctx, lit, [&cond, &elem_vars]<class Lit>(Lit &&glit) {
                    glit->vars(elem_vars, Ground::VarSelectMode::all);
                    cond.emplace_back(std::forward<Lit>(glit));
                });
            }
        }
        // compute global variables
        for (auto const &var : elem_vars) {
            if (vars_body.contains(var)) {
                vars_global.emplace(var);
            }
        };
        // append element
        elems.emplace_back(std::move(head->first), head->second, std::move(cond));
    }

    auto sp_body = ctx.single_pass_body();
    auto elem_priority = ctx.inc_priority();
    auto index = sp_body ? Ground::stratified_index : ctx.next_index();

    // initialize state
    std::ranges::sort(bases, std::less<>{}, [](auto const &x) -> decltype(auto) { return std::get<0>(x); });
    bases.erase(
        std::ranges::unique(bases, std::equal_to<>{}, [](auto const &x) -> decltype(auto) { return std::get<0>(x); })
            .begin(),
        bases.end());
    auto &state =
        ctx.state<Ground::StateDisjunction>(ctx.mbr(), std::move(bases), vars_global.release(), index, sp_body);

    // add accumulation rules for tuples
    auto add_elem = [&](auto &state) {
        for (auto &[head, base, cond] : elems) {
            cond.emplace_back(std::make_unique<Ground::LitDisjunction>(state));
            ctx.gcomp().add(
                std::make_unique<Ground::StmDisjunctionElem>(state, std::move(head), *base, std::move(cond)));
        }
    };

    add_elem(state);
    ctx.gcomp().add(std::make_unique<Ground::StmDisjunction>(state, std::move(ctx.body()), elem_priority));
}

namespace {

//! Analyze the given conditional literal and return the required indices for grounding.
[[nodiscard]] auto analyze(BuildContext &ctx, Input::CondLit const &lit)
    -> std::tuple<bool, bool, bool, size_t, size_t, size_t> {
    assert(!Input::is_fixed(lit.lit()).value_or(false));

    auto has_conclusion = !Input::is_fixed(lit.lit()).has_value();
    auto sp_body = ctx.single_pass_body();
    auto sp_premise = intersects(ctx.type(), Input::ComponentType::single_pass) ||
                      std::ranges::all_of(lit.cond(), [&](auto const &lit) { return ctx.single_pass(lit); });
    auto sp_conclusion = ctx.single_pass(lit.lit());

    auto empty_index = Ground::stratified_index;
    auto premise_index = Ground::stratified_index;
    auto lit_index = Ground::stratified_index;

    if (!sp_premise || !sp_conclusion) {
        if (!sp_body) {
            empty_index = ctx.next_index();
        }
        if (!sp_body || !sp_premise) {
            premise_index = ctx.next_index();
        }
        lit_index = has_conclusion ? ctx.next_index() : premise_index;
    }

    return {has_conclusion, sp_conclusion, sp_premise, empty_index, premise_index, lit_index};
}

} // namespace

void build_bd_lit(BuildContext &ctx, Input::BdLitConjunction const &lit, Ground::ProfileNodeInternal *node) {
    auto [has_conclusion, sp_conclusion, sp_premise, empty_index, premise_index, lit_index] = analyze(ctx, lit.lit());
    bool domain = true;
    auto add_lit = [&](auto &body, auto &vars, auto const &lit) {
        build_lit(ctx, lit, [&body, &vars, &domain]<class Lit>(Lit &&glit) {
            glit->vars(vars, Ground::VarSelectMode::all);
            body.emplace_back(std::forward<Lit>(glit));
            if (domain && !body.back()->domain()) {
                domain = false;
            }
        });
    };

    // convert conclusion and premise
    bool shift = sp_conclusion && has_conclusion;
    auto vars_lit = Ground::VariableSet{};
    auto premise = Ground::ULitVec{};
    premise.reserve(lit.lit().cond().size() + 1 + static_cast<size_t>(shift));
    for (auto const &clit : lit.lit().cond()) {
        add_lit(premise, vars_lit, clit);
    }

    if (shift) {
        has_conclusion = false;
        add_lit(premise, vars_lit, Input::negate(lit.lit().lit()));
    }

    auto conclusion = Ground::ULitVec{};
    if (has_conclusion) {
        conclusion.reserve(2);
        add_lit(conclusion, vars_lit, lit.lit().lit());
    }

    auto vars_body = Ground::VariableSet{};
    for (auto const &lit : ctx.body()) {
        lit->vars(vars_body, Ground::VarSelectMode::all);
    }

    // initialize base
    auto vars_local = Ground::VariableVec{};
    auto vars_global = Ground::VariableVec{};
    for (auto const &x : vars_lit) {
        if (vars_body.contains(x)) {
            vars_global.emplace_back(x);
        } else {
            vars_local.emplace_back(x);
        }
    }

    auto &base = ctx.state<Ground::StateCondLit>(ctx.mbr(), std::move(vars_local), std::move(vars_global), lit_index,
                                                 has_conclusion, sp_premise, domain);

    // handle the single-pass case
    if (sp_conclusion && sp_premise) {
        assert(!has_conclusion);
        premise.insert(premise.begin(), std::make_unique<Ground::LitCondLit>(Ground::LitCondLitType::empty, base, 1));
        ctx.body().emplace_back(std::make_unique<Ground::LitCondLitStrat>(base, std::move(premise)));
    }
    // handle the multi-pass case
    else {
        // convert body
        auto body = Ground::copy_uvec(ctx.body());

        // create: empty(clit(G)) :- B1.
        ctx.gcomp().add(std::make_unique<Ground::StmCondLit>(Ground::StmCondLitType::empty, base, std::move(body),
                                                             ctx.inc_priority(), empty_index));

        // create: premise(clit(G),L) :- empty(clit(G)), P.
        premise.insert(premise.begin(),
                       std::make_unique<Ground::LitCondLit>(Ground::LitCondLitType::empty, base, empty_index));
        ctx.gcomp().add(std::make_unique<Ground::StmCondLit>(Ground::StmCondLitType::premise, base, std::move(premise),
                                                             ctx.inc_priority(), premise_index));

        // create: conclusion(clit(G),L) :- premise(clit(G),L), C.
        if (has_conclusion) {
            conclusion.insert(conclusion.begin(), std::make_unique<Ground::LitCondLit>(Ground::LitCondLitType::premise,
                                                                                       base, premise_index));
            ctx.gcomp().add(std::make_unique<Ground::StmCondLit>(Ground::StmCondLitType::conclusion, base,
                                                                 std::move(conclusion), ctx.inc_priority(), lit_index));
        }

        // create: H :- B1, clit(G), B2.
        ctx.body().emplace_back(std::make_unique<Ground::LitCondLit>(Ground::LitCondLitType::lit, base, base.index()));
    }
}

} // namespace CppClingo::Control
