#include <gringo/grounder/grounder.hh>

#include <gringo/ground/aggregate.hh>
#include <gringo/ground/condlit.hh>
#include <gringo/ground/program.hh>

#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/parse.hh>
#include <gringo/input/algo/print.hh>
#include <gringo/input/algo/unpool_relations.hh>
#include <gringo/input/algo/visit_variables.hh>

#include <gringo/util/type_traits.hh>
#include <gringo/util/unordered_map.hh>

#ifdef PARSER_PROFILE
#include <gperftools/profiler.h>
#endif

#include <filesystem>
#include <forward_list>
#include <iostream>

namespace Gringo {

#ifdef PARSER_PROFILE
namespace {
class Profiler {
  public:
    Profiler(char const *path) { ProfilerStart(path); }
    ~Profiler() { ProfilerStop(); }
};
} // namespace
#endif

struct Grounder::Impl : Gringo::SymbolOwner {
    using BaseMap = Util::ordered_map<std::tuple<String, size_t, bool>, std::unique_ptr<Ground::Base>>;
    using ProjectMap = Util::ordered_map<Ground::UTerm, std::unique_ptr<Ground::LitProject::State>>;

    Impl(Logger &log, SymbolStore &store, Input::RewriteOptions opts, OutputStm &out)
        : log{&log}, store{&store}, prg{opts}, out{&out} {
        this->store->gc_add_owner(*this);
    }
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
        for (auto const &[key, base] : aux_base) {
            GRINGO_REPORT(*log, trace) << "  mark aux domain: " << (std::get<2>(key) ? "-" : "") << std::get<0>(key)
                                       << "/" << std::get<1>(key);
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
    //! - Clear indices associated with domains.
    //! - Inform output that grounding is finished.
    void post_ground() {
        for (auto const &[key, base] : atom_base) {
            base->clear_context();
        }
        for (auto const &[key, state] : project_base) {
            state->p_base().clear_context();
        }
        aux_base.clear();
    }

    //! Clear indices associated with domains.
    auto add_project(Ground::UTerm const &term,
                     Ground::Base &base) -> std::pair<Ground::UTerm, Ground::LitProject::State *> {
        size_t vars = 0;
        auto [it, ins] =
            project_base.try_emplace(term->rename(*store, Ground::RenameMode::rename_vars, nullptr, &vars));
        auto const &p_key = *it->first;
        auto &state = it.value();
        if (ins) {
            auto p_name = store->string_ref("#p_" + std::to_string(project_base.size()));
            auto p_head = p_key.rename(*store, Ground::RenameMode::drop_projection, &p_name, nullptr);
            auto p_body = p_key.rename(*store, Ground::RenameMode::rename_projection, nullptr, &vars);
            state =
                std::make_unique<Ground::LitProject::State>(p_name, vars, base, std::move(p_head), std::move(p_body));
        }
        return {term->rename(*store, Ground::RenameMode::drop_projection, &state->name(), nullptr), state.get()};
    }

    auto add_base(std::tuple<String, size_t, bool> sig) {
        bool aux = std::get<0>(sig).starts_with("#");
        auto dom_it = (aux ? aux_base : atom_base).try_emplace(std::move(sig), nullptr).first;
        if (dom_it->second == nullptr) {
            dom_it.value() = std::make_unique<Ground::Base>();
        }
        return dom_it;
    }

    auto add_base(String name, size_t arity, bool sign) { return add_base({name, arity, sign}); }

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
    //! A base for auxiliary atoms.
    BaseMap aux_base;
    //! The output.
    OutputStm *out;
    //! Indicate that the logic program might still be satisfiable.
    bool is_sat = true;
};

namespace {

//! Helper for parsing.
class Parser {
  public:
    Parser(Logger &log, SymbolStore &store, Input::UnprocessedProgram &prg) : log_{&log}, store_{&store}, prg_{&prg} {}

    template <class Scanner> void process(Scanner &&scanner) { return process(root_, std::forward<Scanner>(scanner)); }

    // NOLINTBEGIN(cppcoreguidelines-missing-std-forward,bugprone-unchecked-optional-access)
    //! Scan statements.
    template <class Scanner> void process(std::filesystem::path const &dir, Scanner &&scanner) {
        prg_->ensure_base();
        for (auto stm = scanner.scan(); stm; stm = scanner.scan()) {
            if (auto *include = std::get_if<Input::StmInclude>(&*stm); include != nullptr) {
                includes_.emplace_back(dir, *include);
            } else {
                prg_->add(*store_, *std::move(stm));
            }
        }
        parse_error_ = parse_error_ || scanner.has_error();
    }
    // NOLINTEND(cppcoreguidelines-missing-std-forward,bugprone-unchecked-optional-access)

    //! Parse a program from the given path.
    auto process_path(auto &&path, bool required) -> bool {
        if (std::filesystem::exists(path)) {
            path = std::filesystem::canonical(path);
            auto rel = path.lexically_relative(root_);
            if (!std::filesystem::is_directory(path)) {
                if (seen_.emplace(path).second) {
                    process(path.parent_path(), Input::scan_file(*log_, *store_, rel.c_str()));
                } else {
                    GRINGO_REPORT(*log_, info_file_included) << "file already included: " << rel;
                }
            } else {
                GRINGO_REPORT(*log_, error) << "cannot include directory: " << rel;
                parse_error_ = true;
            }
            return true;
        }
        if (required) {
            GRINGO_REPORT(*log_, error) << "file not found: " << path;
            parse_error_ = true;
        }
        return false;
    }

    //! Parse a program from stdin.
    void process_stdin() {
        if (!processed_stdin_) {
            processed_stdin_ = true;
            process(root_, Input::scan_stream(*log_, *store_, std::cin));
        } else {
            GRINGO_REPORT(*log_, info_file_included) << "file already included: -";
        }
    }

    //! Process includes encountered while parsing.
    void process_includes() {
        for (; !includes_.empty(); includes_.pop_front()) {
            auto const &[parent, include] = includes_.front();
            if (include.type() == Input::IncludeType::system) {
                auto path = std::filesystem::path(include.value().c_str());
                if (path.is_relative() && parent != root_) {
                    if (process_path(parent / path, false)) {
                        continue;
                    }
                }
                process_path(path, true);
            }
        }
    }

    void check() const {
        if (parse_error_) {
            throw std::runtime_error("parsing failed");
        }
    }

  private:
    Logger *log_;
    SymbolStore *store_;
    Input::UnprocessedProgram *prg_;
    std::filesystem::path root_ = std::filesystem::current_path();
    std::deque<std::pair<std::filesystem::path, Input::StmInclude>> includes_;
    Util::unordered_set<std::filesystem::path> seen_;
    bool processed_stdin_ = false;
    bool parse_error_ = false;
};

auto map_binary_op(Input::BinaryOperator op) -> Ground::BinaryOperator {
    switch (op) {
        case Input::BinaryOperator::and_: {
            return Ground::BinaryOperator::and_;
        }
        case Input::BinaryOperator::div: {
            return Ground::BinaryOperator::div;
        }
        case Input::BinaryOperator::dots: {
            break;
        }
        case Input::BinaryOperator::minus: {
            return Ground::BinaryOperator::minus;
        }
        case Input::BinaryOperator::mod: {
            return Ground::BinaryOperator::mod;
        }
        case Input::BinaryOperator::or_: {
            return Ground::BinaryOperator::or_;
        }
        case Input::BinaryOperator::plus: {
            return Ground::BinaryOperator::plus;
        }
        case Input::BinaryOperator::pow: {
            return Ground::BinaryOperator::pow;
        }
        case Input::BinaryOperator::times: {
            return Ground::BinaryOperator::times;
        }
        case Input::BinaryOperator::xor_: {
            return Ground::BinaryOperator::xor_;
        }
    }
    throw std::runtime_error("unsupported binary operator");
}

class BuilderTerm {
  public:
    BuilderTerm(bool &has_projection, Util::unordered_map<String, size_t> &var_map)
        : has_projection_{&has_projection}, var_map_{&var_map} {}

    auto operator()(Input::TermVariable const &term) const -> Ground::UTerm {
        assert(var_map_->find(term.name()) != var_map_->end());
        return std::make_unique<Ground::TermVariable>(var_map_->find(term.name())->second);
    }
    auto operator()(Input::TermSymbol const &term) const -> Ground::UTerm {
        return std::make_unique<Ground::TermSymbol>(term.value());
    }
    [[nodiscard]] auto handle_args(Input::ArgumentArray const &args) const -> Ground::UTermVec {
        Ground::UTermVec g_args;
        g_args.reserve(args.size());
        for (auto const &arg : args) {
            g_args.emplace_back(std::visit(
                [this]<class T>(T const &arg) -> Ground::UTerm {
                    if constexpr (Util::matches<T, Input::Projection>) {
                        *has_projection_ = true;
                        return std::make_unique<Ground::TermProjection>();
                    } else {
                        return std::visit(*this, arg);
                    }
                },
                arg));
        }
        return g_args;
    }
    auto operator()(Input::TermTuple const &term) const -> Ground::UTerm {
        assert(term.pool().size() == 1 && std::holds_alternative<Input::ArgumentTuple>(term.pool().front()));
        return std::make_unique<Ground::TermTuple>(
            handle_args(std::get<Input::ArgumentTuple>(term.pool().front()).elems()));
    }
    auto operator()(Input::TermFunction const &term) const -> Ground::UTerm {
        assert(!term.external() && term.pool().size() == 1);
        return std::make_unique<Ground::TermFunction>(term.name(), handle_args(term.pool().front().elems()));
    }
    auto operator()(Input::TermAbs const &term) const -> Ground::UTerm {
        assert(term.pool().size() == 1);
        return std::make_unique<Ground::TermUnary>(Ground::UnaryOperator::abs, std::visit(*this, term.pool().front()));
    }
    auto operator()(Input::TermUnary const &term) const -> Ground::UTerm {
        Ground::UnaryOperator op =
            term.op() == Input::UnaryOperator::negate ? Ground::UnaryOperator::minus : Ground::UnaryOperator::invert;
        return std::make_unique<Ground::TermUnary>(op, std::visit(*this, *term.rhs()));
    }
    auto operator()(Input::TermBinary const &term) const -> Ground::UTerm {
        assert(term.op() != Input::BinaryOperator::dots);
        if (auto lin = Input::check_linear(term); lin) {
            assert(var_map_->find(lin->x()) != var_map_->end());
            return std::make_unique<Ground::TermLinear>(lin->m(), var_map_->find(lin->x())->second, lin->n());
        }
        return std::make_unique<Ground::TermBinary>(std::visit(*this, *term.lhs()), map_binary_op(term.op()),
                                                    std::visit(*this, *term.rhs()));
    }

  private:
    bool *has_projection_;
    Util::unordered_map<String, size_t> *var_map_;
};

struct BuildContext {
    BuildContext(Grounder::Impl &impl, Input::Component const &comp,
                 Util::unordered_map<Input::Term const *, std::vector<size_t>> &def_map, Ground::Component &gcomp,
                 Util::unordered_map<String, size_t> &var_map, Ground::ULitVec &body,
                 std::forward_list<Ground::StateCondLit> &clit_base)
        : impl{&impl}, comp{&comp}, def_map{&def_map}, gcomp{&gcomp}, var_map{&var_map}, body{&body},
          clit_base_{&clit_base} {}

    //! Get the index of the given symbolic literal.
    [[nodiscard]] auto index(Input::LitSymbolic const &lit) const -> size_t {
        auto it = comp->incomplete.find(&lit.term());
        if (it != comp->incomplete.end()) {
            return static_cast<size_t>(it - comp->incomplete.begin());
        }
        return Ground::stratified_index;
    }
    //! Check if the given input literal is recursive.
    [[nodiscard]] auto is_recursive(Input::Lit const &lit) const -> bool {
        if (test(comp->type, Input::ComponentType::single_pass)) {
            return false;
        }
        if (auto const *slit = std::get_if<Input::LitSymbolic>(&lit); slit != nullptr) {
            return slit->sign() == Sign::none && index(*slit) != Ground::stratified_index;
        }
        return false;
    }
    [[nodiscard]] auto next_index() -> size_t { return comp->incomplete.size() + index_++; }

    //! Analyze the given conditional literal and return the required indices for grounding.
    [[nodiscard]] auto analyze(Input::CondLit const &lit) -> std::tuple<bool, bool, bool, size_t, size_t, size_t> {
        assert(!Input::is_fixed(lit.lit()).value_or(false));

        auto has_conclusion = !Input::is_fixed(lit.lit()).has_value();
        auto rec_comp = comp->type != Input::ComponentType::single_pass;
        auto rec_body =
            rec_comp && std::any_of(body->begin(), body->end(), [](auto const &lit) { return lit->recursive(); });
        auto rec_premise = rec_comp && std::any_of(lit.cond().begin(), lit.cond().end(),
                                                   [this](auto const &lit) { return is_recursive(lit); });
        bool rec_conclusion = rec_comp && is_recursive(lit.lit());

        auto empty_index = Ground::stratified_index;
        auto premise_index = Ground::stratified_index;
        auto lit_index = Ground::stratified_index;

        if (rec_premise || rec_conclusion) {
            if (rec_body) {
                empty_index = Ground::stratified_index;
            }
            if (rec_body || rec_premise) {
                premise_index = next_index();
            }
            lit_index = has_conclusion ? next_index() : premise_index;
        }

        return {has_conclusion, rec_conclusion, rec_premise, empty_index, premise_index, lit_index};
    }

    Grounder::Impl *impl;
    Input::Component const *comp;
    Util::unordered_map<Input::Term const *, std::vector<size_t>> *def_map;
    Ground::Component *gcomp;
    Util::unordered_map<String, size_t> *var_map;
    Ground::ULitVec *body;
    std::forward_list<Ground::StateCondLit> *clit_base_;
    size_t priority = 0;
    size_t index_ = 0;
};

template <class F> class BuilderLit {
  public:
    BuilderLit(BuildContext &ctx, F cb) : cb_{std::move(cb)}, ctx_{&ctx} {}
    void operator()(Input::LitBool const &lit) const {
        // Note: this should actually never be called
        cb_(std::make_unique<Ground::LitBool>(lit.value()));
    }
    void operator()(Input::LitComparison const &lit) const {
        auto has_projection = false;
        auto bld_term = BuilderTerm{has_projection, *ctx_->var_map};
        if (Input::is_interval(lit.rhs().front().second)) {
            auto lhs = std::visit(bld_term, lit.lhs());
            auto const &rng = std::get<Input::TermBinary>(lit.rhs().front().second);
            auto lower = std::visit(bld_term, *rng.lhs());
            auto upper = std::visit(bld_term, *rng.rhs());
            cb_(std::make_unique<Ground::LitInterval>(std::move(lhs), std::move(lower), std::move(upper)));
        } else if (Input::is_external(lit.rhs().front().second)) {
            std::ostringstream oss;
            oss << "implement me: handle external function call " << lit;
            throw std::logic_error(oss.str());
        } else {
            auto add_cmp = [this, &bld_term](auto const &lhs, auto rel, auto const &rhs) {
                auto l = std::visit(bld_term, lhs);
                auto r = std::visit(bld_term, rhs);
                cb_(std::make_unique<Ground::LitComparison>(std::move(l), rel, std::move(r)));
            };
            auto const &lhs = lit.lhs();
            auto const &rhs = lit.rhs().front().second;
            auto rel = lit.rhs().front().first;
            add_cmp(lhs, rel, rhs);
            if (rel == Relation::equal && Input::is_matchable(rhs) && !Input::is_symbol(rhs)) {
                add_cmp(rhs, rel, lhs);
            }
        }
    }
    void operator()(Input::LitSymbolic const &lit) const {
        auto has_projection = false;
        auto bld_term = BuilderTerm{has_projection, *ctx_->var_map};
        auto term = std::visit(bld_term, lit.term());
        auto idx = ctx_->index(lit);
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        auto dom_it = ctx_->impl->add_base(Input::signature(lit.term()).value());
        if (has_projection) {
            auto [p_term, state] = ctx_->impl->add_project(term, *dom_it.value());
            cb_(std::make_unique<Ground::LitProject>(*state, lit.sign(), std::move(term), std::move(p_term), idx));
        } else {
            cb_(std::make_unique<Ground::LitSymbolic>(*dom_it.value(), lit.sign(), std::move(term), idx));
        }
    }

  private:
    F cb_;
    BuildContext *ctx_;
};

class BuilderHdLit {
  public:
    BuilderHdLit(BuildContext &ctx) : ctx_{&ctx} {}

    template <class T> void operator()(T const &lit) const {
        std::ostringstream oss;
        oss << "implement me: handle head literal " << lit;
        throw std::logic_error(oss.str());
    }
    void operator()(Input::HdLitSimple const &lit) const {
        std::vector<size_t> provides;
        Ground::UTerm blub;
        auto head = std::visit(
            [&]<class T>(T const &lit) -> std::optional<std::pair<Ground::UTerm, Ground::Base &>> {
                if constexpr (Util::matches<T, Input::LitSymbolic>) {
                    auto dom_it = ctx_->impl->add_base(*signature(lit.term()));
                    auto &base = *dom_it->second;
                    assert(lit.sign() == Sign::none);
                    if (auto it = ctx_->def_map->find(&lit.term()); it != ctx_->def_map->end()) {
                        provides = it->second;
                    }
                    auto has_projection = false;
                    auto term = std::visit(BuilderTerm{has_projection, *ctx_->var_map}, lit.term());
                    assert(!has_projection);
                    return std::make_pair(std::move(term), std::ref(base));
                } else if constexpr (Util::matches<T, Input::LitBool>) {
                    if (!lit.value()) {
                        return std::nullopt;
                    }
                }
                throw std::runtime_error("unexpected literal in rule head");
            },
            lit.lit());
        ctx_->gcomp->add(
            std::make_unique<Ground::StmRule>(std::move(head), std::move(provides), std::move(*ctx_->body)));
    }

  private:
    BuildContext *ctx_;
};

class BuilderBdLit {
  public:
    BuilderBdLit(BuildContext &ctx) : ctx_{&ctx} {}
    template <class T> void operator()(T const &lit) const {
        std::ostringstream oss;
        oss << "implement me: handle body literal " << lit;
        throw std::logic_error(oss.str());
    }
    void operator()(Input::BdLitAggregate const &lit) const {
        /*
        - monotone or recursive
          - example
            h(X) :- b(X), #count { Y: e(X,Y) } >= 1, c(X).
          - translation
            #elem(X,0) :- b(X), 0 >= 1.
            #elem(X,1,Y) :- b(X), e(X,Y).
            h(X) :- b(X), #aggr(X), c(X).
          - propagate
            - accumulating elements also adds #aggr domain elements
            - such elements can be added whenever the necessary threshold is reached
        - not monotone and not recursive
          - example
            h(X) :- b(X), #count { Y: e(X,Y) } >= 1, c(X).
          - translation
            h(X) :- b(X), #aggr(X), c(X).
            - nested
              #elem(X,1,Y) :- #aggr(X), e(X,Y).
        - assignment
          - special case for later
        */
        // analyze: stratified, monotonicity, domain (per cond?), assign, indices, priorities
        // TODO: ...
        auto state = std::make_unique<Ground::StateAggr>();
        // TODO: store state in ctx
        // #elem(X,0) :- b(X), 0 >= 1.
        // TODO: ...
        for (auto const &elem : lit.elems()) {
            static_cast<void>(elem);
            // #elem(X,num,tuple) :- body(vars), elem(vars)
            // TODO: ...
        }

        ctx_->body->emplace_back(std::make_unique<Ground::LitAggr>(*state));
        std::ostringstream oss;
        oss << "implement me: handle body aggregate " << lit;
        throw std::logic_error(oss.str());
    }
    void operator()(Input::BdLitSimple const &lit) const {
        // we need to know whether the literal is recursive
        // if it is, then it has to be updated while grounding
        // if not then its index can be created ahead of time
        // we might also want to persue a different strategy here
        // an index only has to be updated until it contains at least one value that justifies grounding
        // the remaining of the index can be updated while grounding
        // in case the index is never grounded, this can safe some computation
        std::visit(
            BuilderLit{*ctx_, [this]<class Lit>(Lit &&glit) { ctx_->body->emplace_back(std::forward<Lit>(glit)); }},
            lit.lit());
    }
    void operator()(Input::BdLitConjunction const &lit) const {
        auto [has_conclusion, rec_conclusion, rec_premise, empty_index, premise_index, lit_index] =
            ctx_->analyze(lit.lit());
        bool domain = true;
        auto build_lit = [this, &domain](auto &body, auto &vars, auto const &lit) {
            std::visit(BuilderLit{*ctx_,
                                  [&body, &vars, &domain]<class Lit>(Lit &&glit) {
                                      glit->vars(vars, Ground::VarSelectMode::all);
                                      body.emplace_back(std::forward<Lit>(glit));
                                      if (domain && !body.back()->domain()) {
                                          domain = false;
                                      }
                                  }},
                       lit);
        };

        // convert conclusion and premise
        bool shift = !rec_conclusion && has_conclusion;
        auto vars_lit = Ground::VariableSet{};
        auto premise = Ground::ULitVec{};
        premise.reserve(lit.lit().cond().size() + 1 + static_cast<size_t>(shift));
        for (auto const &clit : lit.lit().cond()) {
            build_lit(premise, vars_lit, clit);
        }

        if (shift) {
            has_conclusion = false;
            build_lit(premise, vars_lit, Input::negate(lit.lit().lit()));
        }

        auto conclusion = Ground::ULitVec{};
        if (has_conclusion) {
            conclusion.reserve(2);
            build_lit(conclusion, vars_lit, lit.lit().lit());
        }

        auto vars_body = Ground::VariableSet{};
        for (auto const &lit : *ctx_->body) {
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

        auto &base = ctx_->clit_base_->emplace_front(std::move(vars_local), std::move(vars_global), lit_index,
                                                     has_conclusion, rec_premise, domain);

        // handle the stratified case
        if (!rec_conclusion && !rec_premise) {
            assert(!has_conclusion);
            premise.insert(premise.begin(),
                           std::make_unique<Ground::LitCondLit>(Ground::LitCondLitType::empty, base, 1));
            ctx_->body->emplace_back(std::make_unique<Ground::LitCondLitStrat>(base, std::move(premise)));
        }
        // handle the recursive case
        else {
            // convert body
            auto body = Ground::ULitVec{};
            body.reserve(ctx_->body->size());
            for (auto const &lit : *ctx_->body) {
                body.emplace_back(lit->copy());
            }

            // create: empty(clit(G)) :- B1.
            ctx_->gcomp->add(std::make_unique<Ground::StmCondLit>(Ground::StmCondLitType::empty, base, std::move(body),
                                                                  ctx_->priority, empty_index));
            ctx_->priority += 1;

            // create: premise(clit(G),L) :- empty(clit(G)), P.
            premise.insert(premise.begin(),
                           std::make_unique<Ground::LitCondLit>(Ground::LitCondLitType::empty, base, empty_index));
            ctx_->gcomp->add(std::make_unique<Ground::StmCondLit>(Ground::StmCondLitType::premise, base,
                                                                  std::move(premise), ctx_->priority, premise_index));
            ctx_->priority += 1;

            // create: conclusion(clit(G),L) :- premise(clit(G),L), C.
            if (has_conclusion) {
                conclusion.insert(conclusion.begin(), std::make_unique<Ground::LitCondLit>(
                                                          Ground::LitCondLitType::premise, base, premise_index));
                ctx_->gcomp->add(std::make_unique<Ground::StmCondLit>(
                    Ground::StmCondLitType::conclusion, base, std::move(conclusion), ctx_->priority, lit_index));
                ctx_->priority += 1;
            }

            // create: H :- B1, clit(G), B2.
            ctx_->body->emplace_back(
                std::make_unique<Ground::LitCondLit>(Ground::LitCondLitType::lit, base, base.index()));
        }
    }

  private:
    BuildContext *ctx_;
};

class BuilderStm {
  public:
    BuilderStm(BuildContext &ctx) : ctx_{&ctx} {}
    template <class T> void operator()(T const &stm) const {
        std::ostringstream oss;
        oss << "implement me: handle statement " << stm;
        throw std::logic_error(oss.str());
    }

    void operator()(Input::StmRule const &stm) const {
        auto bld_bd = BuilderBdLit{*ctx_};
        auto bld_hd = BuilderHdLit{*ctx_};
        ctx_->body->reserve(stm.body().size() + 1);
        for (auto const &lit : stm.body()) {
            std::visit(bld_bd, lit);
        }
        std::visit(bld_hd, stm.head());
    }

  private:
    BuildContext *ctx_;
};

class Builder : public Input::DependencyBuilder {
  public:
    Builder(Grounder::Impl &impl) : impl_{&impl} {}

  private:
    void do_param(Input::ProgramParam const &param) override {
        buf_.str({});
        buf_ << "#program_" << *param.first;
        auto dom_it = impl_->add_base(impl_->store->string_ref(buf_.view()), param.second.size(), false);
        dom_it.value()->add(impl_->store->fun_ref(std::get<0>(dom_it.key()), as_symbol_span(param.second), false),
                            Ground::AtomState::fact);
    }

    void do_meta(std::vector<Input::Stm> const &stms) override {
        for (auto const &stm : stms) {
            std::cout << stm << "\n";
        }
    }

    void do_fact(std::vector<Symbol> const &facts) override {
        for (auto const &fact : facts) {
            auto dom_it = impl_->add_base(fact.name(), fact.args().size(), fact.has_sign());
            dom_it->second->add(fact, Ground::AtomState::fact);
            impl_->out->fact(fact);
        }
    }

    auto do_components(Input::Components const &comps) -> bool override {
        auto lin = Ground::Linearizer{};
        for (auto const &ref_comps : comps) {
            GRINGO_REPORT(*impl_->log, debug) << "  component";
            for (auto const &ref_comp : ref_comps) {
                GRINGO_REPORT(*impl_->log, debug) << "    refined component";
                auto gcomp = Ground::Component{};
                auto clit_base = std::forward_list<Ground::StateCondLit>{};
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
                    auto ctx = BuildContext{*impl_, ref_comp, def_map, gcomp, var_map, body, clit_base};
                    auto bld_stm = BuilderStm{ctx};
                    std::visit(bld_stm, *stm);
                }
                // a component is domain if has be classified as such
                // and also does not contain a non-domain literal
                bool domain = [&ref_comp, &gcomp]() {
                    if (!test(ref_comp.type, Input::ComponentType::domain)) {
                        return false;
                    }
                    for (auto const &stm : gcomp.stms()) {
                        for (auto const &lit : stm->body()) {
                            if (!lit->domain()) {
                                return false;
                            }
                        }
                    }
                    return true;
                }();
                auto queue = Ground::Queue{};
                lin.start(queue, domain);
                for (auto const &stm : gcomp.stms()) {
                    GRINGO_REPORT(*impl_->log, debug) << "      " << *stm;
                    lin.prepare(*stm, stm->body(), stm->important());
                }
                if (!queue.process(*impl_->log, *impl_->store, *impl_->out)) {
                    return false;
                }
            }
            impl_->out->flush();
        }
        return true;
    }

    Grounder::Impl *impl_;
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
        auto prs = Parser{*impl_->log, *impl_->store, impl_->unprocessed_prg};
        auto scanner = Input::scan_string(*impl_->log, *impl_->store, prg);
        prs.process(scanner);
        prs.process_includes();
        prs.check();
    }
}

void Grounder::parse(std::vector<std::string> const &files) {
    GRINGO_REPORT(*impl_->log, debug) << "parsing...";
    if (impl_->is_sat) {
        GCLock lock{*impl_->store};
        auto prs = Parser{*impl_->log, *impl_->store, impl_->unprocessed_prg};
        if (files.empty()) {
            prs.process_stdin();
            prs.process_includes();
        }
        for (auto const &file : files) {
            if (file == "-") {
                prs.process_stdin();
            } else {
                prs.process_path(std::filesystem::path(file), true);
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
        auto bld = Builder{*impl_};
        impl_->is_sat = impl_->prg.analyze(*impl_->store, params, bld);
        impl_->post_ground();
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

} // namespace Gringo
