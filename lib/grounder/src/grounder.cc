#include <gringo/grounder/grounder.hh>

#include <gringo/ground/aggregate.hh>
#include <gringo/ground/program.hh>

#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/parse.hh>
#include <gringo/input/algo/print.hh>
#include <gringo/input/algo/visit_variables.hh>

#include <gringo/util/type_traits.hh>
#include <gringo/util/unordered_map.hh>

#include <filesystem>
#include <iostream>

namespace Gringo {

struct Grounder::Impl {
    Impl(Logger &log, SymbolStore &store, Input::RewriteOptions opts) : log{&log}, store{&store}, prg{opts} {}

    auto add_project(Ground::UTerm const &term,
                     Ground::Base &base) -> std::pair<Ground::UTerm, Ground::LitProject::State *> {
        size_t vars = 0;
        auto [it, ins] = map.try_emplace(term->rename(*store, Ground::RenameMode::rename_vars, nullptr, &vars));
        auto const &p_key = *it->first;
        auto &state = it.value();
        if (ins) {
            auto p_name = store->string("#p_" + std::to_string(map.size()));
            auto p_head = p_key.rename(*store, Ground::RenameMode::drop_projection, &p_name, nullptr);
            auto p_body = p_key.rename(*store, Ground::RenameMode::rename_projection, nullptr, &vars);
            state =
                std::make_unique<Ground::LitProject::State>(p_name, vars, base, std::move(p_head), std::move(p_body));
        }
        return {term->rename(*store, Ground::RenameMode::drop_projection, &state->name(), nullptr), state.get()};
    }

    //! The logger used by the grounder.
    Logger *log;
    //! The store used by the grounder.
    SymbolStore *store;
    //! The current unprocessed program not yet added to the program.
    Input::UnprocessedProgram unprocessed_prg;
    //! The program stored in the grounder.
    Input::Program prg;
    //! Dictionary to map terms with projections to their replacement predicates.
    Util::ordered_map<Ground::UTerm, std::unique_ptr<Ground::LitProject::State>> map;
    //! The atom base.
    Util::ordered_map<std::tuple<String, size_t, bool>, std::unique_ptr<Ground::Base>> atom_base;
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
        prg_->ensure_base = true;
        for (auto stm = scanner.scan(); stm; stm = scanner.scan()) {
            if (auto *include = std::get_if<Input::StmInclude>(&*stm); include != nullptr) {
                includes_.emplace_back(dir, *include);
            } else {
                prg_->add(*store_, *std::move(stm));
            }
        }
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
            }
            return true;
        }
        if (required) {
            GRINGO_REPORT(*log_, error) << "file not found: " << path;
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
                auto path = std::filesystem::path(include.value());
                if (path.is_relative() && parent != root_) {
                    if (process_path(parent / path, false)) {
                        continue;
                    }
                }
                process_path(path, true);
            }
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
            return std::make_unique<Ground::TermLinear>(*lin->m(), var_map_->find(lin->x())->second, lin->n());
        }
        return std::make_unique<Ground::TermBinary>(std::visit(*this, *term.lhs()), map_binary_op(term.op()),
                                                    std::visit(*this, *term.rhs()));
    }

  private:
    bool *has_projection_;
    Util::unordered_map<String, size_t> *var_map_;
};

struct BuildContext {
    Grounder::Impl *impl = nullptr;
    Input::Component const *comp = nullptr;
    Util::unordered_map<Input::Term const *, std::vector<size_t>> *def_map = nullptr;
    Ground::Component *gcomp = nullptr;
    Util::unordered_map<String, size_t> *var_map = nullptr;
    Ground::ULitVec *body = nullptr;
    size_t priority = 0;
};

template <class F> class BuilderLit {
  public:
    BuilderLit(BuildContext &ctx, F cb) : cb_{std::move(cb)}, ctx_{&ctx} {}
    void operator()(Input::LitBool const &lit) const {
        static_cast<void>(lit);
        throw std::logic_error("literal boolean: implement me!!!");
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
            throw std::logic_error("literal comparison is external: implement me!!!");
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
        auto it = ctx_->comp->incomplete.find(&lit.term());
        // the index referring to a set of heads defining this literal
        // - a literal that is recursive and inside a non-domain component is non-domain
        // - a literal whole contains a non-fact is non-domain
        auto idx = Ground::stratified_index;
        if (it != ctx_->comp->incomplete.end()) {
            idx = it - ctx_->comp->incomplete.begin();
        }
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        auto sig = Input::signature(lit.term()).value();
        auto dom_it = ctx_->impl->atom_base.try_emplace(sig, nullptr).first;
        if (dom_it->second == nullptr) {
            dom_it.value() = std::make_unique<Ground::Base>();
        }
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

    template <class T> void operator()([[maybe_unused]] T const &lit) const {
        throw std::logic_error("head literal: implement me!!!");
    }
    void operator()(Input::HdLitSimple const &lit) const {
        std::vector<size_t> provides;
        Ground::UTerm blub;
        Ground::Base *base = nullptr;
        auto head = std::visit(
            [&]<class T>(T const &lit) -> Ground::UTerm {
                if constexpr (Util::matches<T, Input::LitSymbolic>) {
                    auto dom_it = ctx_->impl->atom_base.try_emplace(*signature(lit.term()), nullptr).first;
                    if (dom_it->second == nullptr) {
                        dom_it.value() = std::make_unique<Ground::Base>();
                    }
                    base = dom_it->second.get();
                    assert(lit.sign() == Sign::none);
                    if (auto it = ctx_->def_map->find(&lit.term()); it != ctx_->def_map->end()) {
                        provides = it->second;
                    }
                    auto has_projection = false;
                    auto term = std::visit(BuilderTerm{has_projection, *ctx_->var_map}, lit.term());
                    assert(!has_projection);
                    return term;
                } else if constexpr (Util::matches<T, Input::LitBool>) {
                    // TODO: either use a nullptr or handle contstraints in some specific way
                } else {
                    assert(false);
                }
                return nullptr;
            },
            lit.lit());
        ctx_->gcomp->add(
            std::make_unique<Ground::StmRule>(std::move(head), base, std::move(provides), std::move(*ctx_->body)));
    }

  private:
    BuildContext *ctx_;
};

class BuilderBdLit {
  public:
    BuilderBdLit(BuildContext &ctx) : ctx_{&ctx} {}
    template <class T> void operator()([[maybe_unused]] T const &lit) const {
        throw std::logic_error("implement me!!!");
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
        static_cast<void>(lit);
        // - needs auxiliary rules
        // - copy body until cond lit and mark as domain!
        // - G are the global variables in the cond lit
        // - L are the local variables in the head of the cond lit
        // - #accu(empty,G) :- body.
        // - #accu(cond,G,L) :- #accu(empty,G), cond.
        // - progate:
        //   - head & #accu(cond,G,L)
        //   - #aggr(cond,G)
        // - head :- body, #aggr(cond,G), ...
        // - body should not have #aggr literals in it
        //   (or only literals with a lower priority)
        // - consider separating base/and aggregate bases

        // splitting:
        // H :- B1, C : P, B2.
        // TODO: indices for enquing
        auto stms = Ground::UStmVec{};     // TODO: how to return
        auto base = Ground::BaseCondLit{}; // TODO: where to store
        //   empty(clit(G)) :- B1                             0
        auto body = Ground::ULitVec{};
        body.reserve(ctx_->body->size());
        for (auto const &lit : *ctx_->body) {
            body.emplace_back(lit->copy());
        }
        stms.emplace_back(
            std::make_unique<Ground::StmCondLit>(Ground::StmCondLitType::empty, base, std::move(body), ctx_->priority));
        ctx_->priority += 1;
        //   premise(clit(G),L) :- empty(clit(G)), P.         1
        body = Ground::ULitVec{};
        body.reserve(lit.lit().cond().size() + 1);
        body.emplace_back(std::make_unique<Ground::LitCondLit>(Ground::LitCondLitType::empty, base));
        for (auto const &clit : lit.lit().cond()) {
            std::visit(
                BuilderLit{*ctx_, [&body]<class Lit>(Lit &&glit) { body.emplace_back(std::forward<Lit>(glit)); }},
                clit);
        }
        stms.emplace_back(std::make_unique<Ground::StmCondLit>(Ground::StmCondLitType::premise, base, std::move(body),
                                                               ctx_->priority));
        ctx_->priority += 1;
        //   conclusion(clit(G),L) :- premise(clit(G),L), C.  2
        if (auto fixed = Input::is_fixed(lit.lit().lit()); !fixed || !*fixed) {
            body = Ground::ULitVec{};
            body.reserve(2);
            body.emplace_back(std::make_unique<Ground::LitCondLit>(Ground::LitCondLitType::premise, base));
            std::visit(
                BuilderLit{*ctx_, [&body]<class Lit>(Lit &&glit) { body.emplace_back(std::forward<Lit>(glit)); }},
                lit.lit().lit());
            stms.emplace_back(std::make_unique<Ground::StmCondLit>(Ground::StmCondLitType::conclusion, base,
                                                                   std::move(body), ctx_->priority));
            ctx_->priority += 1;
        }
        //   H :- B1, clit(G), B2.                            3
        ctx_->body->emplace_back(std::make_unique<Ground::LitCondLit>(Ground::LitCondLitType::lit, base));
        throw std::logic_error("implement me: cond lit");
    }

  private:
    BuildContext *ctx_;
};

class BuilderStm {
  public:
    BuilderStm(BuildContext &ctx) : ctx_{&ctx} {}
    template <class T> void operator()([[maybe_unused]] T const &stm) const {
        throw std::logic_error("implement me!!!");
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

    void param(Input::ProgramParam const &param) override {
        std::cerr << "#program_" << param.first << "(";
        bool comma = false;
        for (auto const &sym : param.second) {
            if (comma) {
                std::cerr << ", ";
            } else {
                comma = true;
            }
            std::cerr << sym;
        }
        std::cerr << ").\n";
    }

    void meta(std::vector<Input::Stm> const &stms) override {
        for (auto const &stm : stms) {
            std::cerr << stm << "\n";
        }
    }

    void fact(std::vector<Symbol> const &facts) override {
        for (auto const &fact : facts) {
            // TODO: remove c&p
            auto sig = std::tuple<String, size_t, bool>(fact.name(), fact.args().size(), fact.has_sign());
            auto dom_it = impl_->atom_base.try_emplace(sig, nullptr).first;
            if (dom_it->second == nullptr) {
                dom_it.value() = std::make_unique<Ground::Base>();
            }
            dom_it->second->add(fact, Ground::AtomState::fact);
            std::cerr << fact << ".\n";
        }
    }

    void components(Input::Components const &comps) override {
        auto lin = Ground::Linearizer{};
        for (auto const &ref_comps : comps) {
            GRINGO_REPORT(*impl_->log, debug) << "  component";
            for (auto const &ref_comp : ref_comps) {
                GRINGO_REPORT(*impl_->log, debug) << "    refined component";
                auto gcomp = Ground::Component{};
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
                    auto ctx = BuildContext{impl_, &ref_comp, &def_map, &gcomp, &var_map, &body, 0};
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
                            if (!lit->domain(true)) {
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
                    lin.prepare(*stm);
                }
                queue.process(*impl_->log, *impl_->store);
            }
        }
    }

  private:
    Grounder::Impl *impl_;
};

} // namespace

Grounder::Grounder(Logger &log, SymbolStore &store, Input::RewriteOptions opts)
    : impl_{std::make_unique<Impl>(log, store, opts)} {}

Grounder::~Grounder() noexcept = default;

void Grounder::parse(std::string_view prg) {
    GRINGO_REPORT(*impl_->log, debug) << "parsing...";
    auto prs = Parser{*impl_->log, *impl_->store, impl_->unprocessed_prg};
    auto scanner = Input::scan_string(*impl_->log, *impl_->store, prg);
    prs.process(scanner);
    prs.process_includes();
}

void Grounder::parse(std::vector<std::string> const &files) {
    GRINGO_REPORT(*impl_->log, debug) << "parsing...";
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
}

void Grounder::prepare() {
    GRINGO_REPORT(*impl_->log, debug) << "preparing...";
    impl_->prg.join(*impl_->log, *impl_->store, std::move(impl_->unprocessed_prg));
    impl_->unprocessed_prg.clear();
}

void Grounder::ground(Input::ProgramParamVec const &params) {
    GRINGO_REPORT(*impl_->log, debug) << "grounding...";
    auto bld = Builder{*impl_};
    impl_->prg.analyze(*impl_->store, params, bld);
    std::cerr.flush();
}

void Grounder::output_unprocessed_program(std::ostream &out) {
    for (auto const &stm : impl_->unprocessed_prg.const_stms) {
        out << stm << "\n";
    }
    for (auto const &stm : impl_->unprocessed_prg.thy_stms) {
        out << stm << "\n";
    }
    for (auto const &stm : impl_->unprocessed_prg.meta_stms) {
        out << stm << "\n";
    }
    for (auto const &[prg_stm, stms, facts] : impl_->unprocessed_prg.parts) {
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
    impl_->prg.visit_stms(*impl_->store, [&out](auto const &stm) { out << stm << "\n"; });
    out.flush();
}

} // namespace Gringo
