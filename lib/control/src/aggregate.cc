#include <clingo/control/aggregate.hh>
#include <clingo/control/literal.hh>

#include <clingo/ground/assignment_aggregate.hh>
#include <clingo/ground/body_aggregate.hh>
#include <clingo/ground/head_aggregate.hh>

namespace CppClingo::Control {

namespace {

auto init_vars(BuildContext &ctx) {
    auto vars_body = Ground::VariableSet{};
    for (auto const &lit : ctx.body()) {
        lit->vars(vars_body, Ground::VarSelectMode::all);
    }
    return vars_body;
}

auto init_guards(BuildContext &ctx, Ground::VariableSet &vars_global, auto &lit) {
    auto guards = Ground::GuardVec{};
    guards.reserve((lit.lhs() ? 1 : 0) + (lit.rhs() ? 1 : 0));
    if (auto const &lhs = lit.lhs()) {
        guards.emplace_back(flip(lhs->second), build_term(ctx.var_map(), lhs->first));
        guards.back().second->vars(vars_global);
    }
    if (auto const &rhs = lit.rhs()) {
        guards.emplace_back(rhs->first, build_term(ctx.var_map(), rhs->second));
        guards.back().second->vars(vars_global);
    }
    return guards;
}

} // namespace

void build_hd_lit(BuildContext &ctx, Input::HdLitAggregate const &lit, Ground::ProfileNodeInternal *node) {
    Ground::ProfileNodeInternal *node_lit = nullptr;
    auto vars_global = Ground::VariableSet{};
    auto vars_body = init_vars(ctx);
    auto guards = init_guards(ctx, vars_global, lit);

    if (guards.empty()) {
        size_t n = lit.elems().size();
        for (auto const &elem : lit.elems()) {
            --n;
            // ignore anything that is not a symbolic atom
            if (!is_atom(elem.lit())) {
                continue;
            }
            // get terms that can fail
            std::vector<Input::Term> can_fail;
            for (auto const &term : elem.tuple()) {
                Input::extract_can_fail(term, can_fail);
            }
            // body literals
            auto body = Ground::ULitVec{};
            auto size = ctx.body().size() + elem.cond().size() + (can_fail.empty() ? 0 : 1);
            if (n > 0) {
                body.reserve(size);
                for (auto const &lit : ctx.body()) {
                    body.emplace_back(lit->copy());
                }
            } else {
                body = std::move(ctx.body());
                body.reserve(size);
            }
            // condition
            for (auto const &lit : elem.cond()) {
                build_lit(ctx, lit, [&body]<class Lit>(Lit &&glit) { body.emplace_back(std::forward<Lit>(glit)); });
            }
            // fail check
            if (!can_fail.empty()) {
                Ground::UTermVec terms;
                for (auto const &term : can_fail) {
                    terms.emplace_back(build_term(ctx.var_map(), term));
                }
                body.emplace_back(std::make_unique<Ground::LitFailCheck>(std::move(terms)));
            }
            // choice rule
            ctx.gcomp().add(std::make_unique<Ground::StmRule>(ctx.simple_lit(elem.lit()), std::move(body),
                                                              Ground::RuleType::choice, node));
        }
        return;
    }

    auto pos = lit.fun() == AggregateFunction::sum; // sum aggregate can be turned into a sum+ aggregate
    using TermBase = std::optional<std::pair<Ground::UTerm, Ground::AtomBase *>>;
    auto elems = std::vector<std::tuple<Ground::UTermVec, TermBase, Location, Ground::ULitVec>>{};
    elems.reserve(lit.elems().size());
    Ground::BaseVec bases;
    bases.reserve(elems.size());
    for (auto const &elem : lit.elems()) {
        auto elem_vars = Ground::VariableSet{};
        // tuple
        auto tuple = Ground::UTermVec{};
        auto loc = elem.tuple().empty() ? elem.loc() : location(elem.tuple().front());
        if (lit.fun() == AggregateFunction::count) {
            tuple.reserve(elem.tuple().size() + 1);
            tuple.emplace_back(std::make_unique<Ground::TermSymbol>(SymbolStore::num_ref(1)));
        } else {
            tuple.reserve(elem.tuple().size());
        }
        for (auto const &term : elem.tuple()) {
            tuple.emplace_back(build_term(ctx.var_map(), term));
            tuple.back()->vars(elem_vars);
        }
        pos = pos && std::visit(
                         []<class T>(T const &weight) {
                             if constexpr (Util::matches<T, Input::TermSymbol>) {
                                 auto sym = weight.value();
                                 return sym.type() != SymbolType::number || sym.num() >= 0;
                             }
                             return false;
                         },
                         elem.tuple().front());
        // head
        auto head = TermBase{};
        ctx.with_simple_lit(
            elem.lit(),
            [&](auto sig, auto term, auto &base, auto provides) {
                bases.emplace_back(sig, &base, std::move(provides));
                head.emplace(std::make_pair(std::move(term), &base));
            },
            true);
        // condition
        auto cond = Ground::ULitVec{};
        cond.reserve(elem.cond().size() + 1);
        for (auto const &lit : elem.cond()) {
            build_lit(ctx, lit, [&cond, &elem_vars]<class Lit>(Lit &&glit) {
                glit->vars(elem_vars, Ground::VarSelectMode::all);
                cond.emplace_back(std::forward<Lit>(glit));
            });
        }
        // compute global variables
        for (auto const &var : elem_vars) {
            if (vars_body.contains(var)) {
                vars_global.emplace(var);
            }
        };
        // append element
        elems.emplace_back(std::move(tuple), std::move(head), std::move(loc), std::move(cond));
    }
    auto fun = pos ? AggregateFunction::sump : lit.fun();
    // Note that this slightly increases the required storage for count
    // aggregates.
    if (fun == AggregateFunction::count) {
        fun = AggregateFunction::sump;
    }

    auto sp_body = ctx.single_pass_body();
    auto elem_priority = ctx.inc_priority();
    auto index = sp_body ? Ground::stratified_index : ctx.next_index();

    // initialize state
    std::ranges::sort(bases, [](auto const &x, auto const &y) { return std::get<0>(x) < std::get<0>(y); });

    bases.erase(
        std::ranges::unique(bases, [](auto const &x, auto const &y) { return std::get<0>(x) == std::get<0>(y); })
            .begin(),
        bases.end());
    auto &state = ctx.state<Ground::StateHdAggr>(ctx.mbr(), std::move(bases), vars_global.release(), std::move(guards),
                                                 fun, index, sp_body);

    // add accumulation rules for tuples
    auto add_elem = [&](auto &state) {
        for (auto &[tuple, head, loc, cond] : elems) {
            if (node != nullptr && node_lit == nullptr) {
                node_lit =
                    &node->add_child(std::make_unique<Ground::ProfileNodeExpression<Input::HdLitAggregate>>(lit));
            }
            cond.emplace_back(std::make_unique<Ground::LitHdAggr>(state));
            ctx.gcomp().add(std::make_unique<Ground::StmHdAggrElem>(state, std::move(head), std::move(loc),
                                                                    std::move(tuple), std::move(cond), node_lit));
        }
    };

    add_elem(state);
    ctx.gcomp().add(std::make_unique<Ground::StmHdAggr>(state, std::move(ctx.body()), elem_priority, node));
}

void build_bd_lit(BuildContext &ctx, Input::BdLitAggregate const &lit, Ground::ProfileNodeInternal *node) {
    auto vars_global = Ground::VariableSet{};
    auto vars_body = init_vars(ctx);
    auto guards = init_guards(ctx, vars_global, lit);

    // check for assignment aggregates
    auto assign = !std::ranges::all_of(vars_global, [&vars_body](auto const &var) { return vars_body.contains(var); });

    if (assign) {
        vars_global.clear();
    }

    auto dom = true;                                // all literals in conditions are domain
    auto sp_elems = true;                           // all conditions are single-pass
    auto has_cond = false;                          // at least one element has a condition
    auto pos = lit.fun() == AggregateFunction::sum; // sum aggregate can be turned into a sum+ aggregate
    auto elems = std::vector<std::tuple<Location, Ground::UTermVec, Ground::ULitVec>>{};
    elems.reserve(lit.elems().size());
    for (auto const &elem : lit.elems()) {
        auto elem_vars = Ground::VariableSet{};
        // tuple
        auto loc = elem.tuple().empty() ? elem.loc() : location(elem.tuple().front());
        auto tuple = Ground::UTermVec{};
        if (lit.fun() == AggregateFunction::count) {
            tuple.reserve(elem.tuple().size() + 1);
            tuple.emplace_back(std::make_unique<Ground::TermSymbol>(SymbolStore::num_ref(1)));
        } else {
            tuple.reserve(elem.tuple().size());
        }
        for (auto const &term : elem.tuple()) {
            tuple.emplace_back(build_term(ctx.var_map(), term));
            tuple.back()->vars(elem_vars);
        }
        pos = pos && std::visit(
                         []<class T>(T const &weight) {
                             if constexpr (Util::matches<T, Input::TermSymbol>) {
                                 auto sym = weight.value();
                                 return sym.type() != SymbolType::number || sym.num() >= 0;
                             }
                             return false;
                         },
                         elem.tuple().front());
        // condition
        auto cond = Ground::ULitVec{};
        cond.reserve(elem.cond().size());
        for (auto const &lit : elem.cond()) {
            sp_elems = sp_elems && ctx.single_pass(lit);
            build_lit(ctx, lit, [&cond, &elem_vars]<class Lit>(Lit &&glit) {
                glit->vars(elem_vars, Ground::VarSelectMode::all);
                cond.emplace_back(std::forward<Lit>(glit));
            });
        }
        dom = dom && std::ranges::all_of(cond, [](auto const &glit) { return glit->domain(); });
        for (auto const &var : elem_vars) {
            if (vars_body.contains(var)) {
                vars_global.emplace(var);
            }
        };
        has_cond = has_cond || !cond.empty();
        elems.emplace_back(std::move(loc), std::move(tuple), std::move(cond));
    }
    auto fun = pos ? AggregateFunction::sump : lit.fun();
    // Note that this slightly increases the required storage for count
    // aggregates.
    if (fun == AggregateFunction::count) {
        fun = AggregateFunction::sump;
    }
    auto mon = Input::reduct_is_monotone(lit.lhs(), fun, lit.rhs());

    if (!has_cond) {
        for (auto &guard : guards) {
            auto cmp = guard.first;
            if (lit.sign() != Sign::once) {
                cmp = flip(cmp);
            }
            auto tuples = std::vector<Ground::UTermVec>{};
            for (auto const &elem : elems) {
                tuples.emplace_back(Ground::copy_uvec(get<1>(elem)));
            }
            ctx.body().emplace_back(
                std::make_unique<Ground::LitSimpleAggr>(std::move(guard.second), cmp, fun, std::move(tuples)));
        }
        return;
    }

    auto elem_priority = ctx.inc_priority();
    auto index = sp_elems ? Ground::stratified_index : ctx.next_index();

    auto create_body = [&](size_t reserve) {
        auto body = Ground::ULitVec{};
        body.reserve(reserve);
        for (auto const &lit : ctx.body()) {
            body.emplace_back(lit->copy());
        }
        return body;
    };

    Ground::ProfileNodeInternal *sub_node = nullptr;
    auto get_node = [&](bool nested) -> Ground::ProfileNodeInternal * {
        if (node != nullptr && sub_node == nullptr) {
            sub_node =
                &node->add_child(std::make_unique<Ground::ProfileNodeExpression<Input::BdLitAggregate>>(lit, nested));
        }
        return sub_node;
    };

    // add accumulation rule for neutral tuples
    auto add_empty = [&]<class T>(auto &state, Symbol neutral, Ground::ULitVec &&body) {
        ctx.gcomp().add(std::make_unique<T>(
            state, lit.loc(), Util::make_vec<Ground::UTerm>(std::make_unique<Ground::TermSymbol>(neutral)),
            std::move(body), 0, elem_priority, get_node(false)));
    };

    // add accumulation rules for tuples
    auto add_elem = [&]<class T>(auto &state) {
        for (auto &[loc, tuple, cond] : elems) {
            auto num = cond.size();
            cond.reserve(ctx.body().size() + cond.size());
            for (auto const &lit : ctx.body()) {
                cond.emplace_back(lit->copy());
            }
            ctx.gcomp().add(std::make_unique<T>(state, loc, std::move(tuple), std::move(cond), num, elem_priority,
                                                get_node(false)));
        }
    };

    // create accumulation rules for stratified aggregates
    auto add_sp_elems = [&]<class T>(auto &state) {
        std::vector<T> stms;
        stms.reserve(elems.size());
        for (auto &[loc, tuple, cond] : elems) {
            if (sub_node == nullptr && node != nullptr) {
                sub_node =
                    &node->add_child(std::make_unique<Ground::ProfileNodeExpression<Input::BdLitAggregate>>(lit, true));
            }
            auto num = cond.size();
            cond.emplace_back(std::make_unique<Ground::LitTuple>(state.global(), state.symbols()));
            stms.emplace_back(state, loc, std::move(tuple), std::move(cond), num, elem_priority, get_node(true));
        }
        return stms;
    };

    if (assign) {
        assert(lit.sign() == Sign::none && guards.size() == 1 && guards.front().first == Relation::equal);
        auto &state = ctx.state<Ground::StateAssignAggr>(ctx.mbr(), vars_global.release(),
                                                         std::move(guards.front().second), fun, index, dom, sp_elems);

        if (sp_elems) {
            ctx.body().emplace_back(std::make_unique<Ground::LitAssignAggrStrat>(
                state, add_sp_elems.operator()<Ground::StmAssignAggrElem>(state)));
        } else {
            // add rule for empty case
            auto body = create_body(ctx.body().size());
            auto neutral = neutral_val(fun);
            add_empty.operator()<Ground::StmAssignAggrElem>(state, neutral, std::move(body));
            add_elem.operator()<Ground::StmAssignAggrElem>(state);
            ctx.body().emplace_back(std::make_unique<Ground::LitAssignAggr>(state));
        }
    } else {
        // initialize state
        auto &state = ctx.state<Ground::StateBdAggr>(ctx.mbr(), vars_global.release(), std::move(guards), fun, index,
                                                     dom, mon, sp_elems);
        if (sp_elems) {
            ctx.body().emplace_back(std::make_unique<Ground::LitBdAggrStrat>(
                state, add_sp_elems.operator()<Ground::StmBdAggrElem>(state), lit.sign()));
        } else {
            auto body = create_body(ctx.body().size() + state.guards().size());
            auto neutral = neutral_val(fun);
            // detect if accumulation rule for empty case is necessary
            auto add_neutral = true;
            for (auto const &guard : state.guards()) {
                if (auto const *rhs = dynamic_cast<Ground::TermSymbol const *>(guard.second.get()); rhs != nullptr) {
                    if (!evaluate(neutral, guard.first, rhs->symbol())) {
                        add_neutral = false;
                        break;
                    }
                } else {
                    body.emplace_back(std::make_unique<Ground::LitComparison>(
                        std::make_unique<Ground::TermSymbol>(neutral), guard.first, guard.second->copy()));
                }
            }
            if (add_neutral) {
                add_empty.operator()<Ground::StmBdAggrElem>(state, neutral, std::move(body));
            }
            add_elem.operator()<Ground::StmBdAggrElem>(state);
            ctx.body().emplace_back(std::make_unique<Ground::LitBdAggr>(state, lit.sign()));
        }
    }
}

} // namespace CppClingo::Control
