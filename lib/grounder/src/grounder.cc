#include <gringo/grounder/grounder.hh>

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
    Impl(Logger &log, SymbolStore &store, Input::RewriteOptions opts) : log_{&log}, store_{&store}, prg_{opts} {}

    auto add_project(Ground::UTerm const &term) -> Ground::UTerm {
        size_t vars = 0;
        auto [it, ins] = map_.try_emplace(term->rename(*store_, Ground::RenameMode::rename_vars, nullptr, &vars));
        if (ins) {
            it.value() = store_->string("#p_" + std::to_string(map_.size()));
            auto head = it->first->rename(*store_, Ground::RenameMode::drop_projection, &it.value(), nullptr);
            auto body = it->first->rename(*store_, Ground::RenameMode::rename_projection, nullptr, &vars);
            std::cerr << "  TODO: add projection rule:\n";
            std::cerr << "    " << *head << " :- " << *body << "." << '\n';
        }
        return term->rename(*store_, Ground::RenameMode::drop_projection, &it.value(), nullptr);
    }

    //! The logger used by the grounder.
    Logger *log_;
    //! The store used by the grounder.
    SymbolStore *store_;
    //! The current unprocessed program not yet added to the program.
    Input::UnprocessedProgram unprocessed_prg;
    //! The program stored in the grounder.
    Input::Program prg_;
    //! Dictionary to map terms with projections to their replacement predicates.
    Util::ordered_map<Ground::UTerm, String> map_;
    //! The atom base.
    Util::ordered_map<std::tuple<String, size_t, bool>, std::unique_ptr<Ground::Base>> atom_base_;
};

namespace {

//! Helper for parsing.
struct Parser {
    // NOLINTBEGIN(cppcoreguidelines-missing-std-forward,bugprone-unchecked-optional-access)
    //! Scan statements.
    template <class Scanner> void process(std::filesystem::path const &dir, Scanner &&scanner) {
        prg->ensure_base = true;
        for (auto stm = scanner.scan(); stm; stm = scanner.scan()) {
            if (auto *include = std::get_if<Input::StmInclude>(&*stm); include != nullptr) {
                includes.emplace_back(dir, *include);
            } else {
                prg->add(*store, *std::move(stm));
            }
        }
    }
    // NOLINTEND(cppcoreguidelines-missing-std-forward,bugprone-unchecked-optional-access)

    //! Parse a program from the given path.
    auto process_path(auto &&path, bool required) -> bool {
        if (std::filesystem::exists(path)) {
            path = std::filesystem::canonical(path);
            auto rel = path.lexically_relative(root);
            if (!std::filesystem::is_directory(path)) {
                if (seen.emplace(path).second) {
                    process(path.parent_path(), Input::scan_file(*log, *store, rel.c_str()));
                } else {
                    GRINGO_REPORT(*log, info_file_included) << "file already included: " << rel;
                }
            } else {
                GRINGO_REPORT(*log, error) << "cannot include directory: " << rel;
            }
            return true;
        }
        if (required) {
            GRINGO_REPORT(*log, error) << "file not found: " << path;
        }
        return false;
    }

    //! Parse a program from stdin.
    void process_stdin() {
        if (!processed_stdin) {
            processed_stdin = true;
            process(root, Input::scan_stream(*log, *store, std::cin));
        } else {
            GRINGO_REPORT(*log, info_file_included) << "file already included: -";
        }
    }

    //! Process includes encountered while parsing.
    void process_includes() {
        for (; !includes.empty(); includes.pop_front()) {
            auto const &[parent, include] = includes.front();
            if (include.type() == Input::IncludeType::system) {
                auto path = std::filesystem::path(include.value());
                if (path.is_relative() && parent != root) {
                    if (process_path(parent / path, false)) {
                        continue;
                    }
                }
                process_path(path, true);
            }
        }
    }

    Logger *log;
    SymbolStore *store;
    Input::UnprocessedProgram *prg;
    std::filesystem::path root = std::filesystem::current_path();
    std::deque<std::pair<std::filesystem::path, Input::StmInclude>> includes = {};
    Util::unordered_set<std::filesystem::path> seen = {};
    bool processed_stdin = false;
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

struct BuilderTerm {
    auto operator()(Input::TermVariable const &term) const -> Ground::UTerm {
        assert(var_map->find(term.name()) != var_map->end());
        return std::make_unique<Ground::TermVariable>(var_map->find(term.name())->second);
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
                        *has_projection = true;
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
            assert(var_map->find(lin->x()) != var_map->end());
            return std::make_unique<Ground::TermLinear>(*lin->m(), var_map->find(lin->x())->second, lin->n());
        }
        return std::make_unique<Ground::TermBinary>(std::visit(*this, *term.lhs()), map_binary_op(term.op()),
                                                    std::visit(*this, *term.rhs()));
    }
    bool *has_projection;
    Util::unordered_map<String, size_t> *var_map;
};

auto map_sign(Input::Sign sign) {
    switch (sign) {
        case Input::Sign::none: {
            return Ground::Sign::none;
        }
        case Input::Sign::once: {
            return Ground::Sign::once;
        }
        case Input::Sign::twice: {
            break;
        }
    }
    return Ground::Sign::twice;
}

struct BuildContext {
    Grounder::Impl *impl = nullptr;
    Input::Component const *comp = nullptr;
    Util::unordered_map<Input::Term const *, std::vector<size_t>> *def_map = nullptr;
    Ground::Component *gcomp = nullptr;
    Util::unordered_map<String, size_t> *var_map = nullptr;
    Ground::ULitVec *body = nullptr;
};

struct BuilderLit {
    template <class T> auto operator()([[maybe_unused]] T const &lit) const -> Ground::ULit {
        throw std::logic_error("implement me!!!");
    }
    auto operator()(Input::LitSymbolic const &lit) const -> Ground::ULit {
        auto has_projection = false;
        auto bld_term = BuilderTerm{&has_projection, ctx->var_map};
        auto term = std::visit(bld_term, lit.term());
        if (has_projection) {
            term = ctx->impl->add_project(term);
        }
        auto it = ctx->comp->incomplete.find(&lit.term());
        // the index referring to a set of heads defining this literal
        // - a literal that is recursive and inside a non-domain component is non-domain
        // - a literal whole contains a non-fact is non-domain
        auto idx = std::numeric_limits<size_t>::max();
        if (it != ctx->comp->incomplete.end()) {
            idx = it - ctx->comp->incomplete.begin();
        }
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        auto sig = *Input::signature(lit.term());
        auto dom_it = ctx->impl->atom_base_.try_emplace(sig, nullptr).first;
        if (dom_it->second == nullptr) {
            dom_it.value() = std::make_unique<Ground::Base>();
        }
        return std::make_unique<Ground::LitSymbolic>(*dom_it.value(), map_sign(lit.sign()), std::move(term), idx);
    }
    BuildContext *ctx;
};

struct BuilderHdLit {
    template <class T> void operator()([[maybe_unused]] T const &lit) const {
        throw std::logic_error("implement me!!!");
    }
    void operator()(Input::HdLitSimple const &lit) const {
        std::vector<size_t> provides;
        auto head = std::visit(
            [&]<class T>(T const &lit) -> Ground::UTerm {
                if constexpr (Util::matches<T, Input::LitSymbolic>) {
                    assert(lit.sign() == Input::Sign::none);
                    if (auto it = ctx->def_map->find(&lit.term()); it != ctx->def_map->end()) {
                        provides = it->second;
                    }
                    auto has_projection = false;
                    auto term = std::visit(BuilderTerm{&has_projection, ctx->var_map}, lit.term());
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
        ctx->gcomp->add(std::make_unique<Ground::StmRule>(std::move(head), std::move(provides), std::move(*ctx->body)));
    }
    BuildContext *ctx;
};

struct BuilderBdLit {
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
        ctx->body->emplace_back(std::visit(BuilderLit{ctx}, lit.lit()));
    }
    BuildContext *ctx;
};

struct BuilderStm {
    template <class T> void operator()([[maybe_unused]] T const &stm) const {
        throw std::logic_error("implement me!!!");
    }

    void operator()(Input::StmRule const &stm) const {
        auto bld_bd = BuilderBdLit{ctx};
        auto bld_hd = BuilderHdLit{ctx};
        ctx->body->reserve(stm.body().size());
        for (auto const &lit : stm.body()) {
            std::visit(bld_bd, lit);
        }
        std::visit(bld_hd, stm.head());
    }

    BuildContext *ctx;
};

// TODO: here transform statements into grounding directives
struct Builder : Input::DependencyBuilder {
    Builder(Grounder::Impl &impl) : impl{&impl} {}

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
            std::cerr << fact << ".\n";
        }
    }

    void components(Input::Components const &comps) override {
        auto lin = Ground::Linearizer{};
        for (auto const &ref_comps : comps) {
            std::cerr << "% component\n";
            for (auto const &ref_comp : ref_comps) {
                std::cerr << "% refined component\n";
                auto gcomp = Ground::Component{static_cast<Ground::ComponentType>(ref_comp.type)};
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
                    auto ctx = BuildContext{impl, &ref_comp, &def_map, &gcomp, &var_map, &body};
                    auto bld_stm = BuilderStm{&ctx};
                    std::visit(bld_stm, *stm);
                }
                auto insts = Ground::InstantiatorVec{};
                lin.start(insts, test(gcomp.type(), Ground::ComponentType::domain));
                for (auto const &stm : gcomp.stms()) {
                    std::cerr << "  TODO: ground\n";
                    std::cerr << "    " << *stm << '\n';
                    lin.prepare(*stm);
                }
                Ground::Queue queue;
                for (auto &inst : insts) {
                    queue.add(inst);
                }
                queue.process();
            }
        }
    }

    Grounder::Impl *impl;
};

} // namespace

Grounder::Grounder(Logger &log, SymbolStore &store, Input::RewriteOptions opts)
    : impl_{std::make_unique<Impl>(log, store, opts)} {}

Grounder::~Grounder() noexcept = default;

void Grounder::parse(std::string_view prg) {
    GRINGO_REPORT(*impl_->log_, debug) << "parsing...";
    auto prs = Parser{impl_->log_, impl_->store_, &impl_->unprocessed_prg};
    auto scanner = Input::scan_string(*impl_->log_, *impl_->store_, prg);
    prs.process(prs.root, scanner);
    prs.process_includes();
}

void Grounder::parse(std::vector<std::string> const &files) {
    GRINGO_REPORT(*impl_->log_, debug) << "parsing...";
    auto prs = Parser{impl_->log_, impl_->store_, &impl_->unprocessed_prg};
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
    GRINGO_REPORT(*impl_->log_, debug) << "preparing...";
    impl_->prg_.join(*impl_->log_, *impl_->store_, std::move(impl_->unprocessed_prg));
    impl_->unprocessed_prg.clear();
}

void Grounder::ground(Input::ProgramParamVec const &params) {
    GRINGO_REPORT(*impl_->log_, debug) << "grounding...";
    auto bld = Builder{*impl_};
    impl_->prg_.analyze(*impl_->store_, params, bld);
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
    impl_->prg_.visit_stms(*impl_->store_, [&out](auto const &stm) { out << stm << "\n"; });
    out.flush();
}

} // namespace Gringo
