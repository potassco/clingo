#include <clingo/input/rewrite/lower_sort.hh>

#include <clingo/input/rewrite/visit_variables.hh>

namespace CppClingo::Input {

namespace {

class LowerContext {
  public:
    LowerContext(SymbolStore &store, size_t &aux_predicate_id, Stm const &stm)
        : gen_{store, select_variables(stm, VariableContext::all), "__A_"}, aux_predicate_id_{&aux_predicate_id} {}

    [[nodiscard]] auto new_variable() -> String { return gen_.new_name(); }
    [[nodiscard]] auto new_aux_predicate(std::string_view prefix) -> String {
        auto name = std::string{prefix};
        name += std::to_string((*aux_predicate_id_)++);
        return gen_.store().string_ref(name);
    }

  private:
    NameGen gen_;
    size_t *aux_predicate_id_;
};

auto make_atom(Location const &loc, String name, std::span<Term const> terms, Sign sign = Sign::none) -> Lit {
    auto args = std::vector<Argument>{};
    args.reserve(terms.size());
    for (auto const &term : terms) {
        args.emplace_back(term);
    }
    auto fun =
        TermFunction{loc, name, Util::make_immutable_array<ArgumentTuple>(ArgumentTuple{std::move(args)}), false};
    return LitSymbolic{loc, sign, std::move(fun)};
}

auto make_body_atom(Location const &loc, String name, std::span<Term const> terms) -> BdLit {
    return BdLitSimple{make_atom(loc, name, terms)};
}

auto make_head_atom(Location const &loc, String name, std::span<Term const> terms) -> HdLit {
    return HdLitSimple{make_atom(loc, name, terms)};
}

auto output_terms(BdLitSort const &sort) -> std::pair<Term, Term> {
    auto const &tuple = std::get<TermTuple>(sort.outputs());
    auto const &args = std::get<ArgumentTuple>(tuple.pool().front()).elems();
    return {std::get<Term>(args[0]), std::get<Term>(args[1])};
}

template <class Stm> auto key_terms(Stm const &stm, BdLitSort const &sort) -> std::vector<Term> {
    auto global = select_variables(stm, VariableContext::global);
    auto seen = VariableSet{};
    auto keys = std::vector<Term>{};
    for (auto const &elem : sort.elems()) {
        visit_variables(elem, [&](Location const &loc, String var) {
            if (global.contains(var) && seen.emplace(var).second) {
                keys.emplace_back(TermVariable{loc, var});
            }
        });
    }
    return keys;
}

void append_element_rules(StmVec &rules, BdLitSort const &sort, String elem_name, std::span<Term const> keys,
                          std::span<BdLit const> prefix) {
    for (auto const &elem : sort.elems()) {
        auto args = std::vector<Term>{keys.begin(), keys.end()};
        args.emplace_back(elem.tuple().front());
        auto body = std::vector<BdLit>{prefix.begin(), prefix.end()};
        body.reserve(prefix.size() + elem.cond().size());
        for (auto const &lit : elem.cond()) {
            body.emplace_back(BdLitSimple{lit});
        }
        rules.emplace_back(StmRule{sort.loc(), make_head_atom(sort.loc(), elem_name, args), std::move(body)});
    }
}

void append_chain_rule(LowerContext &ctx, StmVec &rules, BdLitSort const &sort, String elem_name, String chain_name,
                       std::span<Term const> keys) {
    auto prev = Term{TermVariable{sort.loc(), ctx.new_variable()}};
    auto next = Term{TermVariable{sort.loc(), ctx.new_variable()}};
    auto middle = Term{TermVariable{sort.loc(), ctx.new_variable()}};

    auto prev_args = std::vector<Term>{keys.begin(), keys.end()};
    prev_args.emplace_back(prev);
    auto next_args = std::vector<Term>{keys.begin(), keys.end()};
    next_args.emplace_back(next);
    auto middle_args = std::vector<Term>{keys.begin(), keys.end()};
    middle_args.emplace_back(middle);
    auto head_args = std::vector<Term>{keys.begin(), keys.end()};
    head_args.emplace_back(prev);
    head_args.emplace_back(next);

    auto body = std::vector<BdLit>{};
    body.emplace_back(make_body_atom(sort.loc(), elem_name, prev_args));
    body.emplace_back(make_body_atom(sort.loc(), elem_name, next_args));
    body.emplace_back(
        BdLitSimple{LitComparison{sort.loc(), Sign::none, prev, Util::make_vec<Guard>(Guard{Relation::less, next})}});
    auto condition = Util::make_vec<Lit>(
        make_atom(sort.loc(), elem_name, middle_args),
        LitComparison{sort.loc(), Sign::none, prev, Util::make_vec<Guard>(Guard{Relation::less, middle})},
        LitComparison{sort.loc(), Sign::none, middle, Util::make_vec<Guard>(Guard{Relation::less, next})});
    body.emplace_back(
        BdLitConjunction{CondLit{sort.loc(), LitBool{sort.loc(), Sign::none, false}, std::move(condition)}});
    rules.emplace_back(StmRule{sort.loc(), make_head_atom(sort.loc(), chain_name, head_args), std::move(body)});
}

template <class Stm> auto lower_statement(LowerContext &ctx, Stm const &stm) -> std::optional<StmVec> {
    auto body = std::vector<BdLit>{};
    auto rules = StmVec{};
    auto changed = false;
    body.reserve(stm.body().size());
    for (auto const &lit : stm.body()) {
        auto const *sort = std::get_if<BdLitSort>(&lit);
        if (sort == nullptr) {
            body.emplace_back(lit);
            continue;
        }
        changed = true;
        auto elem_name = ctx.new_aux_predicate("#sort_elem_");
        auto chain_name = ctx.new_aux_predicate("#sort_chain_");
        auto keys = key_terms(stm, *sort);
        append_element_rules(rules, *sort, elem_name, keys, body);
        append_chain_rule(ctx, rules, *sort, elem_name, chain_name, keys);

        auto [prev, next] = output_terms(*sort);
        auto args = std::move(keys);
        args.emplace_back(std::move(prev));
        args.emplace_back(std::move(next));
        body.emplace_back(make_body_atom(sort->loc(), chain_name, args));
    }
    if (!changed) {
        return std::nullopt;
    }
    rules.emplace_back(stm.update(a_body = std::move(body)));
    return rules;
}

} // namespace

auto lower_sort(SymbolStore &store, size_t &aux_predicate_id, Stm const &stm) -> std::optional<StmVec> {
    auto ctx = LowerContext{store, aux_predicate_id, stm};
    return std::visit(
        [&ctx]<class T>(T const &value) -> std::optional<StmVec> {
            if constexpr (requires { value.body(); }) {
                return lower_statement(ctx, value);
            }
            return std::nullopt;
        },
        stm);
}

} // namespace CppClingo::Input
