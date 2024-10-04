#include <gringo/grounder/grounder.hh>

#include <gringo/ground/assignment_aggregate.hh>
#include <gringo/ground/body_aggregate.hh>
#include <gringo/ground/condlit.hh>
#include <gringo/ground/disjunction.hh>
#include <gringo/ground/head_aggregate.hh>
#include <gringo/ground/program.hh>
#include <gringo/ground/theory_atom.hh>

#include <gringo/input/parser.hh>
#include <gringo/input/print.hh>

#include <gringo/input/rewrite/analyze.hh>
#include <gringo/input/rewrite/evaluate.hh>
#include <gringo/input/rewrite/unpool_relations.hh>
#include <gringo/input/rewrite/visit_variables.hh>

#include <gringo/util/print.hh>
#include <gringo/util/type_traits.hh>
#include <gringo/util/unordered_map.hh>

#include "context.hh"
#include "lit_builder.hh"
#include "parse_helper.hh"

#ifdef PARSER_PROFILE
#include <gperftools/profiler.h>
#endif

#include <iostream>

namespace Gringo::Grounder {

#ifdef PARSER_PROFILE
namespace {
//! Simple profiler to restrict profiling to selected scopes.
class Profiler {
  public:
    //! Construct the profile writing data to the given path.
    Profiler(char const *path) { ProfilerStart(path); }
    //! Stop profiling.
    ~Profiler() { ProfilerStop(); }
};
} // namespace
#endif

//! The actual grounder implementation.
struct Grounder::Impl : Gringo::SymbolOwner {

    //! Construct the grounder implementation.
    Impl(Logger &log, SymbolStore &store, Input::RewriteOptions opts, OutputStm &out)
        : log{&log}, store{&store}, prg{opts}, out{&out} {
        this->store->gc_add_owner(*this);
    }
    //! Destroy the grounder implementation.
    //!
    //! This also releases all symbols held.
    ~Impl() override { store->gc_del_owner(*this); }

    //! Mark symbols held by the grounder protecting them from garbage collection.
    void mark(SymbolCollector &gc) const override {
        GRINGO_REPORT(*log, trace) << "mark owners";
        for (auto const &[key, base] : atom_base) {
            GRINGO_REPORT(*log, trace) << "  mark domain: " << (std::get<2>(key) ? "-" : "") << std::get<0>(key) << "/"
                                       << std::get<1>(key);
            gc.mark(std::get<0>(key));
            base->mark(gc);
        }
        for (auto const &[key, state] : project_base) {
            GRINGO_REPORT(*log, trace) << "  mark projection domain: " << *key;
            state->p_base().mark(gc);
        }
        unprocessed_prg.mark(gc);
        prg.mark(gc);
        out->mark(gc);
    }

    //! Cleanup step-local state accumulated during grounding.
    //!
    //! Clears indices associated with domains.
    void clear() {
        for (auto const &[key, base] : atom_base) {
            base->clear_context();
        }
        for (auto const &[key, state] : project_base) {
            state->p_base().clear_context();
        }
        mbr.release();
    }

    //! Memory resource for efficient allocation.
    //!
    //! This memory resources is used to build the indices of literals that are
    //! potentially used throughout the whole grounding step.
    std::pmr::monotonic_buffer_resource mbr;
    //! The logger used by the grounder.
    Logger *log;
    //! The store used by the grounder.
    SymbolStore *store;
    //! The current unprocessed program not yet added to the program.
    Input::UnprocessedProgram unprocessed_prg;
    //! The program stored in the grounder.
    Input::Program prg;
    //! Dictionary to map terms with projections to their replacement predicates.
    ProjectMap project_base;
    //! The atom base.
    BaseMap atom_base;
    //! The output.
    OutputStm *out;
    //! Indicate that the logic program might still be satisfiable.
    bool is_sat = true;
};

namespace {

//! Translator for head literals.
class BuilderHdLit {
  public:
    //! Construct the translator.
    BuilderHdLit(BuildContext &ctx) : ctx_{&ctx} {}

    //! Translate set aggregates.
    void operator()(Input::HdLitSetAggregate const &lit) const {
        GRINGO_REPORT_LOC(ctx_->logger(), error, lit.loc()) << "unexpected set aggregate " << lit;
        throw std::logic_error("unexpected set aggregate");
    }

    //! Translate theory atoms.
    void operator()(Input::HdLitTheoryAtom const &lit) const {
        auto vars_body = Ground::VariableSet{};
        auto vars_global = Ground::VariableSet{};
        for (auto const &lit : ctx_->body()) {
            lit->vars(vars_body, Ground::VarSelectMode::all);
        }

        // handle guard
        auto guard = std::optional<std::pair<String, Ground::UTheoryTerm>>{};
        if (auto const &rhs = lit.rhs()) {
            guard.emplace(rhs->op(), build_theory_term(ctx_->var_map(), rhs->term()));
            guard->second->vars(vars_global);
        }

        // handle name
        auto name = build_term(ctx_->var_map(), lit.name());
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
                tuple.emplace_back(build_theory_term(ctx_->var_map(), term));
                tuple.back()->vars(vars);
            }
            // condition
            cond.reserve(elem.cond().size() + 1);
            for (auto const &slit : elem.cond()) {
                build_stratified_lit(*ctx_, slit, [&cond, &vars]<class Lit>(Lit &&glit) {
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
        auto &state =
            ctx_->state<Ground::StateTheory>(ctx_->mbr(), vars_global.release(), std::move(name), std::move(guard));

        // add elements to state
        auto stms = Ground::UStmVec{};
        stms.reserve(elems.size());
        for (auto &elem : elems) {
            elem.second.emplace_back(std::make_unique<Ground::LitMatchTheory>(state));
            stms.emplace_back(
                std::make_unique<Ground::StmTheoryElement>(state, std::move(elem.first), std::move(elem.second)));
        }
        state.elems(std::move(stms));

        // add theory statement
        ctx_->gcomp().add(std::make_unique<Ground::StmHdTheory>(state, std::move(ctx_->body())));
    }

    //! Translate disjunctions.
    void operator()(Input::HdLitDisjunction const &lit) const {
        auto vars_body = Ground::VariableSet{};
        for (auto const &lit : ctx_->body()) {
            lit->vars(vars_body, Ground::VarSelectMode::all);
        }

        auto vars_global = Ground::VariableSet{};

        using TermBase = std::pair<Ground::UTerm, Ground::Base *>;
        auto elems = std::vector<std::tuple<Ground::UTerm, Ground::Base *, Ground::ULitVec>>{};
        elems.reserve(lit.elems().size());
        Ground::HdAggrBaseVec bases;
        bases.reserve(elems.size());
        for (auto const &elem : lit.elems()) {
            auto elem_vars = Ground::VariableSet{};
            // head
            auto head = std::optional<TermBase>{};
            std::visit(
                [&, this]<class T>(T const &lit) {
                    if constexpr (Util::matches<T, Input::Lit>) {
                        with_simple_lit_(lit, [&](auto sig, auto term, auto &base, auto provides) {
                            bases.emplace_back(sig, &base, std::move(provides));
                            head.emplace(std::make_pair(std::move(term), &base));
                        });
                    } else {
                        with_simple_lit_(lit.lit(), [&](auto sig, auto term, auto &base, auto provides) {
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
                    build_lit(*ctx_, lit, [&cond, &elem_vars]<class Lit>(Lit &&glit) {
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

        auto sp_body = ctx_->single_pass_body();
        auto elem_priority = ctx_->inc_priority();
        auto index = sp_body ? Ground::stratified_index : ctx_->next_index();

        // initialize state
        std::sort(bases.begin(), bases.end(),
                  [](auto const &x, auto const &y) { return std::get<0>(x) < std::get<0>(y); }),
            bases.end();
        bases.erase(std::unique(bases.begin(), bases.end(),
                                [](auto const &x, auto const &y) { return std::get<0>(x) == std::get<0>(y); }),
                    bases.end());
        auto &state =
            ctx_->state<Ground::StateDisjunction>(ctx_->mbr(), std::move(bases), vars_global.release(), index, sp_body);

        // add accumulation rules for tuples
        auto add_elem = [&, this](auto &state) {
            for (auto &[head, base, cond] : elems) {
                cond.emplace_back(std::make_unique<Ground::LitDisjunction>(state));
                ctx_->gcomp().add(
                    std::make_unique<Ground::StmDisjunctionElem>(state, std::move(head), *base, std::move(cond)));
            }
        };

        add_elem(state);
        ctx_->gcomp().add(std::make_unique<Ground::StmDisjunction>(state, std::move(ctx_->body()), elem_priority));
    }

    //! Translate head aggregates.
    void operator()(Input::HdLitAggregate const &lit) const {
        auto vars_body = Ground::VariableSet{};
        for (auto const &lit : ctx_->body()) {
            lit->vars(vars_body, Ground::VarSelectMode::all);
        }

        auto vars_global = Ground::VariableSet{};

        // handle guards
        auto guards = Ground::GuardVec{};
        guards.reserve((lit.lhs() ? 1 : 0) + (lit.rhs() ? 1 : 0));
        if (lit.lhs()) {
            guards.emplace_back(flip(lit.lhs()->second), build_term(ctx_->var_map(), lit.lhs()->first));
            guards.back().second->vars(vars_global);
        }
        if (lit.rhs()) {
            guards.emplace_back(lit.rhs()->first, build_term(ctx_->var_map(), lit.rhs()->second));
            guards.back().second->vars(vars_global);
        }

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
                auto size = ctx_->body().size() + elem.cond().size() + (can_fail.empty() ? 0 : 1);
                if (n > 0) {
                    body.reserve(size);
                    for (auto const &lit : ctx_->body()) {
                        body.emplace_back(lit->copy());
                    }
                } else {
                    body = std::move(ctx_->body());
                    body.reserve(size);
                }
                // condition
                for (auto const &lit : elem.cond()) {
                    build_lit(*ctx_, lit,
                              [&body]<class Lit>(Lit &&glit) { body.emplace_back(std::forward<Lit>(glit)); });
                }
                // fail check
                if (!can_fail.empty()) {
                    Ground::UTermVec terms;
                    for (auto const &term : can_fail) {
                        terms.emplace_back(build_term(ctx_->var_map(), term));
                    }
                    body.emplace_back(std::make_unique<Ground::LitFailCheck>(std::move(terms)));
                }
                // choice rule
                ctx_->gcomp().add(std::make_unique<Ground::StmRule>(simple_lit_(elem.lit()), std::move(body), true));
            }
            return;
        }

        auto pos = lit.fun() == AggregateFunction::sum; // sum aggregate can be turned into a sum+ aggregate
        using TermBase = std::optional<std::pair<Ground::UTerm, Ground::Base *>>;
        auto elems = std::vector<std::tuple<Ground::UTermVec, TermBase, Ground::ULitVec>>{};
        elems.reserve(lit.elems().size());
        Ground::HdAggrBaseVec bases;
        bases.reserve(elems.size());
        for (auto const &elem : lit.elems()) {
            auto elem_vars = Ground::VariableSet{};
            // tuple
            auto tuple = Ground::UTermVec{};
            if (lit.fun() == AggregateFunction::count) {
                tuple.reserve(elem.tuple().size() + 1);
                tuple.emplace_back(std::make_unique<Ground::TermSymbol>(SymbolStore::num_ref(1)));
            } else {
                tuple.reserve(elem.tuple().size());
            }
            for (auto const &term : elem.tuple()) {
                tuple.emplace_back(build_term(ctx_->var_map(), term));
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
            with_simple_lit_(elem.lit(), [&](auto sig, auto term, auto &base, auto provides) {
                bases.emplace_back(sig, &base, std::move(provides));
                head.emplace(std::make_pair(std::move(term), &base));
            });
            // condition
            auto cond = Ground::ULitVec{};
            cond.reserve(elem.cond().size() + 1);
            for (auto const &lit : elem.cond()) {
                build_lit(*ctx_, lit, [&cond, &elem_vars]<class Lit>(Lit &&glit) {
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
            elems.emplace_back(std::move(tuple), std::move(head), std::move(cond));
        }
        auto fun = pos ? AggregateFunction::sump : lit.fun();
        // Note that this slightly increases the required storage for count
        // aggregates.
        if (fun == AggregateFunction::count) {
            fun = AggregateFunction::sump;
        }

        auto sp_body = ctx_->single_pass_body();
        auto elem_priority = ctx_->inc_priority();
        auto index = sp_body ? Ground::stratified_index : ctx_->next_index();

        // initialize state
        std::sort(bases.begin(), bases.end(),
                  [](auto const &x, auto const &y) { return std::get<0>(x) < std::get<0>(y); }),
            bases.end();
        bases.erase(std::unique(bases.begin(), bases.end(),
                                [](auto const &x, auto const &y) { return std::get<0>(x) == std::get<0>(y); }),
                    bases.end());
        auto &state = ctx_->state<Ground::StateHdAggr>(ctx_->mbr(), std::move(bases), vars_global.release(),
                                                       std::move(guards), fun, index, sp_body);

        // add accumulation rules for tuples
        auto add_elem = [&, this](auto &state) {
            for (auto &[tuple, head, cond] : elems) {
                cond.emplace_back(std::make_unique<Ground::LitHdAggr>(state));
                ctx_->gcomp().add(
                    std::make_unique<Ground::StmHdAggrElem>(state, std::move(head), std::move(tuple), std::move(cond)));
            }
        };

        add_elem(state);
        ctx_->gcomp().add(std::make_unique<Ground::StmHdAggr>(state, std::move(ctx_->body()), elem_priority));
    }

    //! Translate simple head literals.
    void operator()(Input::HdLitSimple const &lit) const {
        ctx_->gcomp().add(std::make_unique<Ground::StmRule>(simple_lit_(lit.lit()), std::move(ctx_->body()), false));
    }

  private:
    using SigAtomSimple =
        std::optional<std::tuple<std::tuple<String, size_t, bool>, Ground::UTerm, Base &, std::vector<size_t>>>;

    [[nodiscard]] auto simple_lit_(Input::Lit const &lit) const -> Ground::AtomSimple {
        auto res = Ground::AtomSimple{};
        with_simple_lit_(lit, [&res]([[maybe_unused]] auto sig, auto term, auto &base, auto provides) {
            res.emplace(std::make_tuple(std::move(term), std::ref(base), std::move(provides)));
        });
        return res;
    }

    template <class F> void with_simple_lit_(Input::Lit const &lit, F fun) const {
        std::visit(
            [&]<class T>(T const &lit) {
                if constexpr (Util::matches<T, Input::LitSymbolic>) {
                    std::vector<size_t> provides;
                    auto sig = *signature(lit.term());
                    auto dom_it = ctx_->add_base(sig);
                    auto &base = *dom_it->second;
                    assert(lit.sign() == Sign::none);
                    if (auto it = ctx_->def_map().find(&lit.term()); it != ctx_->def_map().end()) {
                        provides = it->second;
                    }
                    auto term = build_term(ctx_->var_map(), lit.term());
                    fun(sig, std::move(term), base, std::move(provides));
                    return;
                } else if constexpr (Util::matches<T, Input::LitBool>) {
                    if (!lit.value()) {
                        return;
                    }
                }
                throw std::runtime_error("unexpected literal in rule head");
            },
            lit);
    }

    BuildContext *ctx_;
};

//! Translator for body literals.
class BuilderBdLit {
  public:
    //! Construct the translator.
    BuilderBdLit(BuildContext &ctx) : ctx_{&ctx} {}

    //! Translate set aggregates.
    void operator()(Input::BdLitSetAggregate const &lit) const {
        GRINGO_REPORT_LOC(ctx_->logger(), error, lit.loc()) << "unexpected set aggregate " << lit;
        throw std::logic_error("unexpected set aggregate");
    }

    //! Translate theory atoms.
    void operator()(Input::BdLitTheoryAtom const &lit) const {
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
        // - problems:
        //   - linearization of E1 has to be avoided
        //   - positive literals would have to be marked as single pass
        //   - negative literals must stay multi pass
        //
        auto vars_body = Ground::VariableSet{};
        auto vars_global = Ground::VariableSet{};
        for (auto const &lit : ctx_->body()) {
            lit->vars(vars_body, Ground::VarSelectMode::all);
        }

        // handle guard
        auto guard = std::optional<std::pair<String, Ground::UTheoryTerm>>{};
        if (auto const &rhs = lit.rhs()) {
            guard.emplace(rhs->op(), build_theory_term(ctx_->var_map(), rhs->term()));
            guard->second->vars(vars_global);
        }

        // handle name
        auto name = build_term(ctx_->var_map(), lit.name());
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
                tuple.emplace_back(build_theory_term(ctx_->var_map(), term));
                tuple.back()->vars(vars);
            }
            // condition
            cond.reserve(elem.cond().size() + 1);
            for (auto const &slit : elem.cond()) {
                build_stratified_lit(*ctx_, slit, [&cond, &vars]<class Lit>(Lit &&glit) {
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
        auto &state =
            ctx_->state<Ground::StateTheory>(ctx_->mbr(), vars_global.release(), std::move(name), std::move(guard));

        // add elements to state
        auto stms = Ground::UStmVec{};
        stms.reserve(elems.size());
        for (auto &elem : elems) {
            elem.second.emplace_back(std::make_unique<Ground::LitMatchTheory>(state));
            stms.emplace_back(
                std::make_unique<Ground::StmTheoryElement>(state, std::move(elem.first), std::move(elem.second)));
        }
        state.elems(std::move(stms));

        // add theory atom
        ctx_->body().emplace_back(std::make_unique<Ground::LitBdTheory>(state, lit.sign()));
    }

    //! Translate body aggregates.
    void operator()(Input::BdLitAggregate const &lit) const {
        auto vars_body = Ground::VariableSet{};
        for (auto const &lit : ctx_->body()) {
            lit->vars(vars_body, Ground::VarSelectMode::all);
        }

        auto vars_global = Ground::VariableSet{};

        // handle guards
        auto guards = Ground::GuardVec{};
        guards.reserve((lit.lhs() ? 1 : 0) + (lit.rhs() ? 1 : 0));
        if (lit.lhs()) {
            guards.emplace_back(flip(lit.lhs()->second), build_term(ctx_->var_map(), lit.lhs()->first));
            guards.back().second->vars(vars_global);
        }
        if (lit.rhs()) {
            guards.emplace_back(lit.rhs()->first, build_term(ctx_->var_map(), lit.rhs()->second));
            guards.back().second->vars(vars_global);
        }
        // check for assignment aggregates
        bool assign = !std::all_of(vars_global.begin(), vars_global.end(),
                                   [&vars_body](auto const &var) { return vars_body.contains(var); });

        if (assign) {
            vars_global.clear();
        }

        auto dom = true;                                // all literals in conditions are domain
        auto sp_elems = true;                           // all conditions are single-pass
        auto pos = lit.fun() == AggregateFunction::sum; // sum aggregate can be turned into a sum+ aggregate
        auto elems = std::vector<std::tuple<Ground::UTermVec, Ground::ULitVec>>{};
        elems.reserve(lit.elems().size());
        for (auto const &elem : lit.elems()) {
            auto elem_vars = Ground::VariableSet{};
            // tuple
            auto tuple = Ground::UTermVec{};
            if (lit.fun() == AggregateFunction::count) {
                tuple.reserve(elem.tuple().size() + 1);
                tuple.emplace_back(std::make_unique<Ground::TermSymbol>(SymbolStore::num_ref(1)));
            } else {
                tuple.reserve(elem.tuple().size());
            }
            for (auto const &term : elem.tuple()) {
                tuple.emplace_back(build_term(ctx_->var_map(), term));
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
                sp_elems = sp_elems && ctx_->single_pass(lit);
                build_lit(*ctx_, lit, [&cond, &elem_vars]<class Lit>(Lit &&glit) {
                    glit->vars(elem_vars, Ground::VarSelectMode::all);
                    cond.emplace_back(std::forward<Lit>(glit));
                });
            }
            dom = dom && std::all_of(cond.begin(), cond.end(), [](auto const &glit) { return glit->domain(); });
            for (auto const &var : elem_vars) {
                if (vars_body.contains(var)) {
                    vars_global.emplace(var);
                }
            };
            elems.emplace_back(std::move(tuple), std::move(cond));
        }
        auto fun = pos ? AggregateFunction::sump : lit.fun();
        // Note that this slightly increases the required storage for count
        // aggregates.
        if (fun == AggregateFunction::count) {
            fun = AggregateFunction::sump;
        }
        auto mon = Input::reduct_is_monotone(lit.lhs(), fun, lit.rhs());

        auto elem_priority = ctx_->inc_priority();
        auto index = sp_elems ? Ground::stratified_index : ctx_->next_index();

        auto create_body = [this](size_t reserve) {
            auto body = Ground::ULitVec{};
            body.reserve(reserve);
            for (auto const &lit : ctx_->body()) {
                body.emplace_back(lit->copy());
            }
            return body;
        };

        // add accumulation rule for neutral tuples
        auto add_empty = [&, this]<class T>(auto &state, Symbol neutral, Ground::ULitVec &&body) {
            ctx_->gcomp().add(
                std::make_unique<T>(state, Util::make_vec<Ground::UTerm>(std::make_unique<Ground::TermSymbol>(neutral)),
                                    std::move(body), 0, elem_priority));
        };

        // add accumulation rules for tuples
        auto add_elem = [&, this]<class T>(auto &state) {
            for (auto &[tuple, cond] : elems) {
                auto num = cond.size();
                cond.reserve(ctx_->body().size() + cond.size());
                for (auto const &lit : ctx_->body()) {
                    cond.emplace_back(lit->copy());
                }
                ctx_->gcomp().add(std::make_unique<T>(state, std::move(tuple), std::move(cond), num, elem_priority));
            }
        };

        // create accumulation rules for stratified aggregates
        auto add_sp_elems = [&]<class T>(auto &state) {
            std::vector<T> stms;
            stms.reserve(elems.size());
            for (auto &[tuple, cond] : elems) {
                auto num = cond.size();
                cond.emplace_back(std::make_unique<Ground::LitTuple>(state.global(), state.symbols()));
                stms.emplace_back(state, std::move(tuple), std::move(cond), num, elem_priority);
            }
            return stms;
        };

        if (assign) {
            assert(lit.sign() == Sign::none && guards.size() == 1 && guards.front().first == Relation::equal);
            auto &state = ctx_->state<Ground::StateAssignAggr>(
                ctx_->mbr(), vars_global.release(), std::move(guards.front().second), fun, index, dom, sp_elems);

            if (sp_elems) {
                ctx_->body().emplace_back(std::make_unique<Ground::LitAssignAggrStrat>(
                    state, add_sp_elems.operator()<Ground::StmAssignAggrElem>(state)));
            } else {
                // add rule for empty case
                auto body = create_body(ctx_->body().size());
                auto neutral = neutral_val(fun);
                add_empty.operator()<Ground::StmAssignAggrElem>(state, neutral, std::move(body));
                add_elem.operator()<Ground::StmAssignAggrElem>(state);
                ctx_->body().emplace_back(std::make_unique<Ground::LitAssignAggr>(state));
            }
        } else {
            // initialize state
            auto &state = ctx_->state<Ground::StateBdAggr>(ctx_->mbr(), vars_global.release(), std::move(guards), fun,
                                                           index, dom, mon, sp_elems);
            if (sp_elems) {
                ctx_->body().emplace_back(std::make_unique<Ground::LitBdAggrStrat>(
                    state, add_sp_elems.operator()<Ground::StmBdAggrElem>(state), lit.sign()));
            } else {
                auto body = create_body(ctx_->body().size() + state.guards().size());
                auto neutral = neutral_val(fun);
                // detect if accumulation rule for empty case is necessary
                bool add_neutral = true;
                for (auto const &guard : state.guards()) {
                    if (auto const *rhs = dynamic_cast<Ground::TermSymbol const *>(guard.second.get());
                        rhs != nullptr) {
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
                ctx_->body().emplace_back(std::make_unique<Ground::LitBdAggr>(state, lit.sign()));
            }
        }
    }
    //! Translate simple literals.
    void operator()(Input::BdLitSimple const &lit) const {
        build_lit(*ctx_, lit.lit(),
                  [this]<class Lit>(Lit &&glit) { ctx_->body().emplace_back(std::forward<Lit>(glit)); });
    }
    //! Translate conditional literals.
    void operator()(Input::BdLitConjunction const &lit) const {
        auto [has_conclusion, sp_conclusion, sp_premise, empty_index, premise_index, lit_index] =
            ctx_->analyze(lit.lit());
        bool domain = true;
        auto add_lit = [this, &domain](auto &body, auto &vars, auto const &lit) {
            build_lit(*ctx_, lit, [&body, &vars, &domain]<class Lit>(Lit &&glit) {
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
        for (auto const &lit : ctx_->body()) {
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

        auto &base = ctx_->state<Ground::StateCondLit>(ctx_->mbr(), std::move(vars_local), std::move(vars_global),
                                                       lit_index, has_conclusion, sp_premise, domain);

        // handle the single-pass case
        if (sp_conclusion && sp_premise) {
            assert(!has_conclusion);
            premise.insert(premise.begin(),
                           std::make_unique<Ground::LitCondLit>(Ground::LitCondLitType::empty, base, 1));
            ctx_->body().emplace_back(std::make_unique<Ground::LitCondLitStrat>(base, std::move(premise)));
        }
        // handle the multi-pass case
        else {
            // convert body
            auto body = Ground::ULitVec{};
            body.reserve(ctx_->body().size());
            for (auto const &lit : ctx_->body()) {
                body.emplace_back(lit->copy());
            }

            // create: empty(clit(G)) :- B1.
            ctx_->gcomp().add(std::make_unique<Ground::StmCondLit>(Ground::StmCondLitType::empty, base, std::move(body),
                                                                   ctx_->inc_priority(), empty_index));

            // create: premise(clit(G),L) :- empty(clit(G)), P.
            premise.insert(premise.begin(),
                           std::make_unique<Ground::LitCondLit>(Ground::LitCondLitType::empty, base, empty_index));
            ctx_->gcomp().add(std::make_unique<Ground::StmCondLit>(
                Ground::StmCondLitType::premise, base, std::move(premise), ctx_->inc_priority(), premise_index));

            // create: conclusion(clit(G),L) :- premise(clit(G),L), C.
            if (has_conclusion) {
                conclusion.insert(conclusion.begin(), std::make_unique<Ground::LitCondLit>(
                                                          Ground::LitCondLitType::premise, base, premise_index));
                ctx_->gcomp().add(std::make_unique<Ground::StmCondLit>(
                    Ground::StmCondLitType::conclusion, base, std::move(conclusion), ctx_->inc_priority(), lit_index));
            }

            // create: H :- B1, clit(G), B2.
            ctx_->body().emplace_back(
                std::make_unique<Ground::LitCondLit>(Ground::LitCondLitType::lit, base, base.index()));
        }
    }

  private:
    BuildContext *ctx_;
};

//! Translator for statements.
class BuilderStm {
  public:
    //! Construct the translator.
    BuilderStm(BuildContext &ctx) : ctx_{&ctx} {}
    template <class T> void operator()(T const &stm) const {
        std::ostringstream oss;
        oss << "implement me: handle statement " << stm;
        throw std::logic_error(oss.str());
    }

    //! Translate rules.
    void operator()(Input::StmRule const &stm) const {
        auto bld_bd = BuilderBdLit{*ctx_};
        auto bld_hd = BuilderHdLit{*ctx_};
        ctx_->body().reserve(stm.body().size() + 1);
        for (auto const &lit : stm.body()) {
            std::visit(bld_bd, lit);
        }
        std::visit(bld_hd, stm.head());
    }

  private:
    BuildContext *ctx_;
};

//! The builder for the ground representation.
class Builder : public Input::DependencyBuilder {
  public:
    //! Construct the builder.
    Builder(std::pmr::monotonic_buffer_resource &mbr, Logger &log, SymbolStore &store, BaseMap &base_atom,
            ProjectMap &base_project, OutputStm &out)
        : mbr_{&mbr}, log_{&log}, store_{&store}, base_{base_atom, base_aux_, base_project}, out_{&out} {}

  private:
    //! Handle program parameters.
    void do_param(Input::ProgramParam const &param) override {
        buf_.str({});
        buf_ << "#program_" << *param.first;
        auto dom_it = base_.add_base(std::make_tuple(store_->string_ref(buf_.view()), param.second.size(), false));
        dom_it.value()->add(store_->fun_ref(std::get<0>(dom_it.key()), as_symbol_span(param.second), false),
                            Ground::StateAtom::fact);
    }

    //! Handle meta statements.
    void do_meta(std::vector<Input::Stm> const &stms) override {
        for (auto const &stm : stms) {
            std::cout << stm << "\n";
        }
    }

    //! Handle facts.
    void do_fact(std::vector<Symbol> const &facts) override {
        for (auto const &fact : facts) {
            auto dom_it = base_.add_base(std::make_tuple(fact.name(), fact.args().size(), fact.has_sign()));
            dom_it->second->add(fact, Ground::StateAtom::fact);
            out_->fact(fact);
        }
    }

    //! Translate components.
    auto do_components(Input::Components const &comps) -> bool override {
        auto lin = Ground::Linearizer{*mbr_};
        for (auto const &ref_comps : comps) {
            GRINGO_REPORT(*log_, debug) << "  component";
            for (auto const &ref_comp : ref_comps) {
                GRINGO_REPORT(*log_, debug) << "    refined component";
                // A component is classified w.r.t. to previously accumulated
                // atoms. It is domain if it is positive (i.e., contains no
                // negative cycle) and all bases it depends on are domain.
                // A domain component only derives facts.
                bool domain = test(ref_comp.type, Input::ComponentType::positive) &&
                              std::all_of(ref_comp.depend.begin(), ref_comp.depend.end(),
                                          [this](auto const &sig) { return base_.add_base(sig)->second->domain(); });
                auto gcomp = Ground::Component{domain};
                auto mbr = std::pmr::monotonic_buffer_resource{};
                auto states = StateList{};
                for (auto const &stm : ref_comp.stms) {
                    Util::unordered_map<String, size_t> var_map;
                    Input::visit_variables(
                        *stm,
                        [&var_map]([[maybe_unused]] Input::Location const &loc, String var) {
                            var_map.try_emplace(var, var_map.size());
                        },
                        Input::VariableContext::all);
                    Ground::ULitVec body;
                    auto def_map = Util::unordered_map<Input::Term const *, std::vector<size_t>>{};
                    auto i = size_t{0};
                    for (auto const &[bd, hds] : ref_comp.incomplete) {
                        for (auto const &hd : hds) {
                            def_map[hd].emplace_back(i);
                        }
                        ++i;
                    }
                    auto ctx =
                        BuildContext{mbr, *log_, *store_, base_, ref_comp, def_map, gcomp, var_map, body, states};
                    auto bld_stm = BuilderStm{ctx};
                    std::visit(bld_stm, *stm);
                }
                auto queue = Ground::Queue{};
                lin.start(queue);
                for (auto const &stm : gcomp.stms()) {
                    GRINGO_REPORT(*log_, debug) << "      " << *stm;
                    lin.prepare(*stm, stm->body(), stm->important());
                }
                if (!queue.process(*log_, *store_, *out_)) {
                    return false;
                }
                for (auto &state : states) {
                    std::visit(
                        [this]<class State>(State &state) {
                            // it's probably better to unify the interface
                            if constexpr (Util::matches<State, Ground::StateTheory>) {
                                state.output(*log_, *store_, *out_);
                            } else {
                                state.output(*out_);
                            }
                        },
                        state);
                }
            }
            out_->flush();
        }
        return true;
    }

    std::pmr::monotonic_buffer_resource *mbr_;
    Logger *log_;
    SymbolStore *store_;
    BaseMap base_aux_;
    BaseHelper base_;
    OutputStm *out_;
    std::ostringstream buf_;
};

} // namespace

Grounder::Grounder(Logger &log, SymbolStore &store, Input::RewriteOptions opts, OutputStm &out)
    : impl_{std::make_unique<Impl>(log, store, opts, out)} {}

Grounder::~Grounder() noexcept = default;

void Grounder::add_const(String name, Symbol value) {
    if (impl_->is_sat) {
        auto lock = GCLock{*impl_->store};
        auto str = impl_->store->string_ref("<cli>");
        auto loc = Input::Location(Input::Position{str, 1, 1}, Input::Position{str, 1, 1});
        auto val = Input::TermSymbol{loc, value};
        impl_->unprocessed_prg.add(*impl_->store,
                                   Input::StmConst{std::move(loc), Input::ConstType::override_, name, std::move(val)});
    }
}

void Grounder::parse(std::string_view prg) {
    GRINGO_REPORT(*impl_->log, debug) << "parsing...";
    if (impl_->is_sat) {
        GCLock lock{*impl_->store};
        auto prs = ParseHelper{*impl_->log, *impl_->store, impl_->unprocessed_prg};
        prs.process_string(prg);
        prs.process_includes();
        prs.check();
    }
}

void Grounder::parse(std::vector<std::string> const &files) {
    GRINGO_REPORT(*impl_->log, debug) << "parsing...";
    if (impl_->is_sat) {
        GCLock lock{*impl_->store};
        auto prs = ParseHelper{*impl_->log, *impl_->store, impl_->unprocessed_prg};
        if (files.empty()) {
            prs.process_stdin();
            prs.process_includes();
        }
        for (auto const &file : files) {
            if (file == "-") {
                prs.process_stdin();
            } else {
                prs.process_path(file);
            }
            prs.process_includes();
        }
        prs.check();
    }
}

void Grounder::prepare() {
    GRINGO_REPORT(*impl_->log, debug) << "preparing...";
    if (impl_->is_sat) {
        GCLock lock{*impl_->store};
        impl_->prg.join(*impl_->log, *impl_->store, impl_->unprocessed_prg);
        impl_->unprocessed_prg.clear();
    }
}

auto Grounder::ground(Input::ProgramParamVec const &params) -> bool {
    GRINGO_REPORT(*impl_->log, debug) << "grounding...";
    GCLock lock{*impl_->store};
#ifdef PARSER_PROFILE
    Profiler prof{"clingo-ground.prof"};
#endif
    if (impl_->is_sat) {
        auto bld = Builder{impl_->mbr, *impl_->log, *impl_->store, impl_->atom_base, impl_->project_base, *impl_->out};
        impl_->is_sat = impl_->prg.analyze(*impl_->store, params, bld);
        impl_->clear();
    }
    impl_->out->end_step();
    return impl_->is_sat;
}

void Grounder::output_unprocessed_program(std::ostream &out) {
    for (auto const &stm : impl_->unprocessed_prg.const_stms()) {
        out << stm << "\n";
    }
    for (auto const &stm : impl_->unprocessed_prg.thy_stms()) {
        out << stm << "\n";
    }
    for (auto const &stm : impl_->unprocessed_prg.meta_stms()) {
        out << stm << "\n";
    }
    for (auto const &[prg_stm, stms, facts] : impl_->unprocessed_prg.parts()) {
        out << prg_stm << "\n";
        for (auto fact : facts) {
            out << fact << ".\n";
        }
        for (const auto &stm : stms) {
            out << stm << "\n";
        }
    }
    out.flush();
}

void Grounder::output_program(std::ostream &out) {
    GCLock lock{*impl_->store};
    impl_->prg.visit_stms(*impl_->store, [&out](auto const &stm) { out << stm << "\n"; });
    out.flush();
}

} // namespace Gringo::Grounder
