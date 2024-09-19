#include <gringo/grounder/grounder.hh>

#include <gringo/ground/assignment_aggregate.hh>
#include <gringo/ground/body_aggregate.hh>
#include <gringo/ground/condlit.hh>
#include <gringo/ground/head_aggregate.hh>
#include <gringo/ground/program.hh>

#include <gringo/input/parser.hh>
#include <gringo/input/print.hh>

#include <gringo/input/rewrite/analyze.hh>
#include <gringo/input/rewrite/evaluate.hh>
#include <gringo/input/rewrite/unpool_relations.hh>
#include <gringo/input/rewrite/visit_variables.hh>

#include <gringo/util/print.hh>
#include <gringo/util/type_traits.hh>
#include <gringo/util/unordered_map.hh>

#ifdef PARSER_PROFILE
#include <gperftools/profiler.h>
#endif

#include <filesystem>
#include <forward_list>
#include <fstream>
#include <iostream>

namespace Gringo {

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
    //! A map from signatures to atom bases.
    using BaseMap = Util::ordered_map<std::tuple<String, size_t, bool>, std::unique_ptr<Ground::Base>>;
    //! A map from a terms with projections associated state used during grounding.
    //!
    //! The terms represensts a class of similar terms that can reuse the same projection state.
    using ProjectMap = Util::ordered_map<Ground::UTerm, std::unique_ptr<Ground::LitProject::State>>;

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
    void post_ground() {
        for (auto const &[key, base] : atom_base) {
            base->clear_context();
        }
        for (auto const &[key, state] : project_base) {
            state->p_base().clear_context();
        }
        aux_base.clear();
        mbr.release();
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

    //! Add an atom base for the given signature.
    //!
    //! If the name of the signature starts with a `#`, it is added to the
    //! auxiliary base, which is deleted after grounding.
    auto add_base(std::tuple<String, size_t, bool> sig) {
        bool aux = std::get<0>(sig).starts_with("#");
        auto dom_it = (aux ? aux_base : atom_base).try_emplace(std::move(sig), nullptr).first;
        if (dom_it->second == nullptr) {
            dom_it.value() = std::make_unique<Ground::Base>();
        }
        return dom_it;
    }

    //! Add an atom base for the given signature.
    auto add_base(String name, size_t arity, bool sign) { return add_base({name, arity, sign}); }

    //! Memory resource for efficient allocation.
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
    //! A base for auxiliary atoms.
    BaseMap aux_base;
    //! The output.
    OutputStm *out;
    //! Indicate that the logic program might still be satisfiable.
    bool is_sat = true;
};

namespace {

//! A helper for parsing.
//!
//! This class manages include directives.
class ParseHelper {
  public:
    //! Construct the helper.
    ParseHelper(Logger &log, SymbolStore &store, Input::UnprocessedProgram &prg)
        : log_{&log}, store_{&store}, parser_{log, store}, prg_{&prg} {}

    //! Parse a program from the given string.
    void process_string(std::string_view str) {
        parser_.init(str, *store_->string("<string>"));
        process_();
    }

    //! Parse a program from the given path.
    //!
    //! Returns false if the file was not found or raises an error if it was
    //! required.
    auto process_path(std::filesystem::path path, bool required) -> bool {
        if (std::filesystem::exists(path)) {
            path = std::filesystem::canonical(path);
            auto rel = path.lexically_relative(root_);
            if (!std::filesystem::is_directory(path)) {
                if (seen_.emplace(path).second) {
                    fin_.open(rel);
                    parser_.init(fin_, *store_->string(rel.c_str()));
                    process_(path.parent_path());
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
            parser_.init(std::cin, *store_->string("-"));
            process_(root_);
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

    //! Throws if there was an error during parsing.
    void check() const {
        if (parse_error_) {
            throw parse_error();
        }
    }

  private:
    //! Scan statements.
    void process_() { process_(root_); }

    // NOLINTBEGIN(cppcoreguidelines-missing-std-forward,bugprone-unchecked-optional-access)
    //! Scan statements.
    void process_(std::filesystem::path const &dir) {
        prg_->ensure_base();
        while (true) {
            auto [stm, res] = parser_.scan();
            parse_error_ = parse_error_ || !res;
            if (!stm) {
                fin_.close();
                break;
            }
            if (auto *include = std::get_if<Input::StmInclude>(&*stm); include != nullptr) {
                includes_.emplace_back(dir, *include);
            } else {
                prg_->add(*store_, *std::move(stm));
            }
        }
    }
    // NOLINTEND(cppcoreguidelines-missing-std-forward,bugprone-unchecked-optional-access)

    Logger *log_;
    SymbolStore *store_;
    std::ifstream fin_;
    Input::Parser parser_;
    Input::UnprocessedProgram *prg_;
    std::filesystem::path root_ = std::filesystem::current_path();
    std::deque<std::pair<std::filesystem::path, Input::StmInclude>> includes_;
    Util::unordered_set<std::filesystem::path> seen_;
    bool processed_stdin_ = false;
    bool parse_error_ = false;
};

//! Maps a binary input operator to the corresponding ground one.
//!
//! @note: The input interval operator has no matching ground version.
//! @note: Candidate for core.
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

//! Translates input terms to their ground representation.
//!
//! Input terms must be rewritten before translation.
class BuilderTerm {
  public:
    //! Construct a term builder.
    //!
    //! The reference has_projection is used to track, whether a projection
    //! star occurred in the term. The var_map dictionary is used to map
    //! variables to integers. Each name must have been assigned an integer
    //! beforehand.
    BuilderTerm(bool &has_projection, Util::unordered_map<String, size_t> const &var_map)
        : has_projection_{&has_projection}, var_map_{&var_map} {}

    //! Translate a variable.
    auto operator()(Input::TermVariable const &term) const -> Ground::UTerm {
        assert(var_map_->find(term.name()) != var_map_->end());
        return std::make_unique<Ground::TermVariable>(var_map_->find(term.name())->second);
    }
    //! Translate a symbol.
    auto operator()(Input::TermSymbol const &term) const -> Ground::UTerm {
        return std::make_unique<Ground::TermSymbol>(term.value());
    }
    //! Translate arguments of tuples and functions.
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
    //! Translate tuples.
    //!
    //! Assumes that the arguments consists of a single pool.
    auto operator()(Input::TermTuple const &term) const -> Ground::UTerm {
        assert(term.pool().size() == 1 && std::holds_alternative<Input::ArgumentTuple>(term.pool().front()));
        return std::make_unique<Ground::TermTuple>(
            handle_args(std::get<Input::ArgumentTuple>(term.pool().front()).elems()));
    }
    //! Translate a function.
    //!
    //! Assumes that the arguments consist of a single pool.
    auto operator()(Input::TermFunction const &term) const -> Ground::UTerm {
        assert(!term.external() && term.pool().size() == 1);
        return std::make_unique<Ground::TermFunction>(term.name(), handle_args(term.pool().front().elems()));
    }
    //! Translate a function.
    //!
    //! Assumes that there is a single argument.
    auto operator()(Input::TermAbs const &term) const -> Ground::UTerm {
        assert(term.pool().size() == 1);
        return std::make_unique<Ground::TermUnary>(Ground::UnaryOperator::abs, std::visit(*this, term.pool().front()));
    }
    //! Translate a unary term.
    auto operator()(Input::TermUnary const &term) const -> Ground::UTerm {
        Ground::UnaryOperator op =
            term.op() == Input::UnaryOperator::negate ? Ground::UnaryOperator::minus : Ground::UnaryOperator::invert;
        return std::make_unique<Ground::TermUnary>(op, std::visit(*this, *term.rhs()));
    }
    //! Translate a unary term.
    //!
    //! Intervals must be removed by rewriting beforehand.
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
    Util::unordered_map<String, size_t> const *var_map_;
};

using StateList = std::forward_list<
    std::variant<Ground::StateCondLit, Ground::StateHdAggr, Ground::StateBdAggr, Ground::StateAssignAggr>>;

//! Context object holding necessary data for translating from input to ground
//! representation.
class BuildContext {
  public:
    using DefMap = Util::unordered_map<Input::Term const *, std::vector<size_t>>;
    BuildContext(Grounder::Impl &impl, std::pmr::monotonic_buffer_resource &mbr, Input::Component const &comp,
                 Util::unordered_map<Input::Term const *, std::vector<size_t>> &def_map, Ground::Component &gcomp,
                 Util::unordered_map<String, size_t> &var_map, Ground::ULitVec &body, StateList &state)
        : impl_{&impl}, mbr_{&mbr}, comp_{&comp}, def_map_{&def_map}, gcomp_{&gcomp}, var_map_{&var_map}, body_{&body},
          state_{&state} {}

    //! Get the index of the given symbolic literal.
    [[nodiscard]] auto index(Input::LitSymbolic const &lit) const -> size_t {
        auto it = comp_->incomplete.find(&lit.term());
        if (it != comp_->incomplete.end()) {
            return static_cast<size_t>(it - comp_->incomplete.begin());
        }
        return Ground::stratified_index;
    }
    //! Check if the given input literal is single pass.
    [[nodiscard]] auto single_pass(Input::Lit const &lit) const -> bool {
        if (test(comp_->type, Input::ComponentType::single_pass)) {
            return true;
        }
        if (auto const *slit = std::get_if<Input::LitSymbolic>(&lit); slit != nullptr) {
            return slit->sign() != Sign::none || index(*slit) == Ground::stratified_index;
        }
        return true;
    }

    //! Check if the (current) body can be grounded in a single pass.
    [[nodiscard]] auto single_pass_body() const -> bool {
        return test(comp_->type, Input::ComponentType::single_pass) ||
               std::all_of(body_->begin(), body_->end(), [](auto const &lit) { return lit->single_pass(); });
    }

    [[nodiscard]] auto next_index() -> size_t { return comp_->incomplete.size() + index_++; }

    //! Analyze the given conditional literal and return the required indices for grounding.
    [[nodiscard]] auto analyze(Input::CondLit const &lit) -> std::tuple<bool, bool, bool, size_t, size_t, size_t> {
        assert(!Input::is_fixed(lit.lit()).value_or(false));

        auto has_conclusion = !Input::is_fixed(lit.lit()).has_value();
        auto sp_body = single_pass_body();
        auto sp_premise =
            test(comp_->type, Input::ComponentType::single_pass) ||
            std::all_of(lit.cond().begin(), lit.cond().end(), [this](auto const &lit) { return single_pass(lit); });
        bool sp_conclusion = single_pass(lit.lit());

        auto empty_index = Ground::stratified_index;
        auto premise_index = Ground::stratified_index;
        auto lit_index = Ground::stratified_index;

        if (!sp_premise || !sp_conclusion) {
            if (!sp_body) {
                empty_index = next_index();
            }
            if (!sp_body || !sp_premise) {
                premise_index = next_index();
            }
            lit_index = has_conclusion ? next_index() : premise_index;
        }

        return {has_conclusion, sp_conclusion, sp_premise, empty_index, premise_index, lit_index};
    }

    //! Get the grounder implementation.
    [[nodiscard]] auto impl() const -> Grounder::Impl & { return *impl_; }

    //! Get the monotonic allocator for the component.
    [[nodiscard]] auto mbr() const -> std::pmr::monotonic_buffer_resource & { return *mbr_; }

    //! Get the definition map.
    [[nodiscard]] auto def_map() const -> DefMap & { return *def_map_; }
    //! Get the variable map.
    [[nodiscard]] auto var_map() const -> Util::unordered_map<String, size_t> & { return *var_map_; }

    //! Get the current component.
    [[nodiscard]] auto gcomp() const -> Ground::Component & { return *gcomp_; }
    //! Get the current statement body.
    [[nodiscard]] auto body() const -> Ground::ULitVec & { return *body_; }

    //! Add a new state object for a body aggregate literal.
    template <class T, class... Args> [[nodiscard]] auto state(Args &&...args) -> T & {
        state_->emplace_front(std::in_place_type<T>, std::forward<Args>(args)...);
        return std::get<T>(state_->front());
    }

    //! Increment the priority and return its previous value.
    auto inc_priority() -> size_t { return priority++; }

  private:
    Grounder::Impl *impl_;
    std::pmr::monotonic_buffer_resource *mbr_;
    Input::Component const *comp_;
    Util::unordered_map<Input::Term const *, std::vector<size_t>> *def_map_;
    Ground::Component *gcomp_;
    Util::unordered_map<String, size_t> *var_map_;
    Ground::ULitVec *body_;
    StateList *state_;
    size_t priority = 0;
    size_t index_ = 0;
};

//! Translate input literals to their ground representation.
//!
//! Assumes that literals have been rewritten.
template <class F> class BuilderLit {
  public:
    //! Construct the translator.
    BuilderLit(BuildContext &ctx, F cb) : cb_{std::move(cb)}, ctx_{&ctx} {}
    //! Translate Boolean literals.
    //!
    //! @note: This should never be called on rewritten programs.
    void operator()(Input::LitBool const &lit) const { cb_(std::make_unique<Ground::LitBool>(lit.value())); }
    //! Translate comparision literals.
    //!
    //! This function also handles intervals and external functions.
    //!
    //! @todo: External functions have not yet been implemented.
    void operator()(Input::LitComparison const &lit) const {
        auto has_projection = false;
        auto bld_term = BuilderTerm{has_projection, ctx_->var_map()};
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
    //! Translate symbolic literals.
    void operator()(Input::LitSymbolic const &lit) const {
        auto has_projection = false;
        auto bld_term = BuilderTerm{has_projection, ctx_->var_map()};
        auto term = std::visit(bld_term, lit.term());
        auto idx = ctx_->index(lit);
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        auto dom_it = ctx_->impl().add_base(Input::signature(lit.term()).value());
        if (has_projection) {
            auto [p_term, state] = ctx_->impl().add_project(term, *dom_it.value());
            cb_(std::make_unique<Ground::LitProject>(*state, lit.sign(), std::move(term), std::move(p_term), idx,
                                                     ctx_->gcomp().domain()));
        } else {
            cb_(std::make_unique<Ground::LitSymbolic>(*dom_it.value(), lit.sign(), std::move(term), idx,
                                                      ctx_->gcomp().domain()));
        }
    }

  private:
    F cb_;
    BuildContext *ctx_;
};

//! Translator for head literals.
class BuilderHdLit {
  public:
    //! Construct the translator.
    BuilderHdLit(BuildContext &ctx) : ctx_{&ctx} {}

    template <class T> void operator()(T const &lit) const {
        std::ostringstream oss;
        oss << "implement me: handle head literal " << lit;
        throw std::logic_error(oss.str());
    }

    //! Translate head aggregates.
    void operator()(Input::HdLitAggregate const &lit) const {
        // TODO:
        // - LitHdAggr is missing
        // - maybe count aggregates can be supported directly because no neutral element is required
        // - a special case for aggregates without guards is in order
        //   - the rule class can be used for simple choice rules
        //     (import because common)
        //   - the (future) disjunction statement could be used for choice rules with more than one element
        //     (would provide nice output)
        auto vars_body = Ground::VariableSet{};
        for (auto const &lit : ctx_->body()) {
            lit->vars(vars_body, Ground::VarSelectMode::all);
        }

        auto vars_global = Ground::VariableSet{};

        // handle guards
        auto guards = Ground::GuardVec{};
        guards.reserve((lit.lhs() ? 1 : 0) + (lit.rhs() ? 1 : 0));
        if (lit.lhs()) {
            bool has_projection = false;
            guards.emplace_back(flip(lit.lhs()->second),
                                std::visit(BuilderTerm{has_projection, ctx_->var_map()}, lit.lhs()->first));
            guards.back().second->vars(vars_global);
        }
        if (lit.rhs()) {
            bool has_projection = false;
            guards.emplace_back(lit.rhs()->first,
                                std::visit(BuilderTerm{has_projection, ctx_->var_map()}, lit.rhs()->second));
            guards.back().second->vars(vars_global);
        }

        auto sp_elems = true;                           // all conditions are single-pass
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
                bool has_projection = false;
                tuple.emplace_back(std::visit(BuilderTerm{has_projection, ctx_->var_map()}, term));
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
                sp_elems = sp_elems && ctx_->single_pass(lit);
                std::visit(BuilderLit{*ctx_,
                                      [&cond, &elem_vars]<class Lit>(Lit &&glit) {
                                          glit->vars(elem_vars, Ground::VarSelectMode::all);
                                          cond.emplace_back(std::forward<Lit>(glit));
                                      }},
                           lit);
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

        auto elem_priority = ctx_->inc_priority();
        auto index = sp_elems ? Ground::stratified_index : ctx_->next_index();

        // initialize state
        std::sort(bases.begin(), bases.end(),
                  [](auto const &x, auto const &y) { return std::get<0>(x) < std::get<0>(y); }),
            bases.end();
        bases.erase(std::unique(bases.begin(), bases.end(),
                                [](auto const &x, auto const &y) { return std::get<0>(x) == std::get<0>(y); }),
                    bases.end());
        auto &state = ctx_->state<Ground::StateHdAggr>(ctx_->mbr(), std::move(bases), vars_global.release(),
                                                       std::move(guards), fun, index, sp_elems);

        auto create_body = [this]() {
            auto body = Ground::ULitVec{};
            body.reserve(ctx_->body().size());
            for (auto const &lit : ctx_->body()) {
                body.emplace_back(lit->copy());
            }
            return body;
        };

        // add accumulation rules for tuples
        auto add_elem = [&, this](auto &state) {
            for (auto &[tuple, head, cond] : elems) {
                GRINGO_REPORT(*ctx_->impl().log, error) << "TODO: LitHdAggr is missing";
                // cond.emplace_back(std::make_unique<Ground::LitHdAggr>(state));
                ctx_->gcomp().add(
                    std::make_unique<Ground::StmHdAggrElem>(state, std::move(head), std::move(tuple), std::move(cond)));
            }
        };

        auto body = create_body();
        add_elem(state);
        ctx_->gcomp().add(std::make_unique<Ground::StmHdAggr>(state, std::move(body), elem_priority));
    }

    //! Translate simple head literals.
    void operator()(Input::HdLitSimple const &lit) const {
        ctx_->gcomp().add(std::make_unique<Ground::StmRule>(simple_lit_(lit.lit()), std::move(ctx_->body())));
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
                    auto dom_it = ctx_->impl().add_base(sig);
                    auto &base = *dom_it->second;
                    assert(lit.sign() == Sign::none);
                    if (auto it = ctx_->def_map().find(&lit.term()); it != ctx_->def_map().end()) {
                        provides = it->second;
                    }
                    auto has_projection = false;
                    auto term = std::visit(BuilderTerm{has_projection, ctx_->var_map()}, lit.term());
                    assert(!has_projection);
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
    template <class T> void operator()(T const &lit) const {
        std::ostringstream oss;
        oss << "implement me: handle body literal " << lit;
        throw std::logic_error(oss.str());
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
            bool has_projection = false;
            guards.emplace_back(flip(lit.lhs()->second),
                                std::visit(BuilderTerm{has_projection, ctx_->var_map()}, lit.lhs()->first));
            guards.back().second->vars(vars_global);
        }
        if (lit.rhs()) {
            bool has_projection = false;
            guards.emplace_back(lit.rhs()->first,
                                std::visit(BuilderTerm{has_projection, ctx_->var_map()}, lit.rhs()->second));
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
                bool has_projection = false;
                tuple.emplace_back(std::visit(BuilderTerm{has_projection, ctx_->var_map()}, term));
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
                std::visit(BuilderLit{*ctx_,
                                      [&cond, &elem_vars]<class Lit>(Lit &&glit) {
                                          glit->vars(elem_vars, Ground::VarSelectMode::all);
                                          cond.emplace_back(std::forward<Lit>(glit));
                                      }},
                           lit);
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
        std::visit(
            BuilderLit{*ctx_, [this]<class Lit>(Lit &&glit) { ctx_->body().emplace_back(std::forward<Lit>(glit)); }},
            lit.lit());
    }
    //! Translate conditional literals.
    void operator()(Input::BdLitConjunction const &lit) const {
        auto [has_conclusion, sp_conclusion, sp_premise, empty_index, premise_index, lit_index] =
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
        bool shift = sp_conclusion && has_conclusion;
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
    Builder(Grounder::Impl &impl) : impl_{&impl} {}

  private:
    //! Handle program parameters.
    void do_param(Input::ProgramParam const &param) override {
        buf_.str({});
        buf_ << "#program_" << *param.first;
        auto dom_it = impl_->add_base(impl_->store->string_ref(buf_.view()), param.second.size(), false);
        dom_it.value()->add(impl_->store->fun_ref(std::get<0>(dom_it.key()), as_symbol_span(param.second), false),
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
            auto dom_it = impl_->add_base(fact.name(), fact.args().size(), fact.has_sign());
            dom_it->second->add(fact, Ground::StateAtom::fact);
            impl_->out->fact(fact);
        }
    }

    //! Translate components.
    auto do_components(Input::Components const &comps) -> bool override {
        auto lin = Ground::Linearizer{impl_->mbr};
        for (auto const &ref_comps : comps) {
            GRINGO_REPORT(*impl_->log, debug) << "  component";
            for (auto const &ref_comp : ref_comps) {
                GRINGO_REPORT(*impl_->log, debug) << "    refined component";
                // A component is classified w.r.t. to previously accumulated
                // atoms. It is domain if it is positive (i.e., contains no
                // negative cycle) and all bases it depends on are domain.
                // A domain component only derives facts.
                bool domain = test(ref_comp.type, Input::ComponentType::positive) &&
                              std::all_of(ref_comp.depend.begin(), ref_comp.depend.end(),
                                          [this](auto const &sig) { return impl_->add_base(sig)->second->domain(); });
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
                    auto ctx = BuildContext{*impl_, mbr, ref_comp, def_map, gcomp, var_map, body, states};
                    auto bld_stm = BuilderStm{ctx};
                    std::visit(bld_stm, *stm);
                }
                auto queue = Ground::Queue{};
                lin.start(queue);
                for (auto const &stm : gcomp.stms()) {
                    GRINGO_REPORT(*impl_->log, debug) << "      " << *stm;
                    lin.prepare(*stm, stm->body(), stm->important());
                }
                if (!queue.process(*impl_->log, *impl_->store, *impl_->out)) {
                    return false;
                }
                for (auto &state : states) {
                    std::visit([this](auto &state) { state.output(*impl_->out); }, state);
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
