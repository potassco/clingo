#include <gringo/grounder/aggregate.hh>
#include <gringo/grounder/condlit.hh>
#include <gringo/grounder/context.hh>
#include <gringo/grounder/grounder.hh>
#include <gringo/grounder/literal.hh>
#include <gringo/grounder/parse.hh>
#include <gringo/grounder/theory.hh>

#include <gringo/ground/program.hh>

#include <gringo/input/print.hh>

#include <gringo/input/rewrite/evaluate.hh>
#include <gringo/input/rewrite/visit_variables.hh>

#include <gringo/util/print.hh>
#include <gringo/util/type_traits.hh>
#include <gringo/util/unordered_map.hh>

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
    //! potentially used throughout a whole grounding step but are cleared afterward.
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
    BuilderHdLit(BuildContext &ctx) : ctx_{&ctx} {}

    void operator()(Input::HdLitSetAggregate const &lit) const {
        GRINGO_REPORT_LOC(ctx_->logger(), error, lit.loc()) << "unexpected set aggregate " << lit;
        throw std::logic_error("unexpected set aggregate");
    }
    void operator()(Input::HdLitTheoryAtom const &lit) const { build_hd_lit(*ctx_, lit); }
    void operator()(Input::HdLitDisjunction const &lit) const { build_hd_lit(*ctx_, lit); }
    void operator()(Input::HdLitAggregate const &lit) const { build_hd_lit(*ctx_, lit); }
    void operator()(Input::HdLitSimple const &lit) const {
        ctx_->gcomp().add(
            std::make_unique<Ground::StmRule>(ctx_->simple_lit(lit.lit()), std::move(ctx_->body()), false));
    }

  private:
    BuildContext *ctx_;
};

//! Translator for body literals.
class BuilderBdLit {
  public:
    BuilderBdLit(BuildContext &ctx) : ctx_{&ctx} {}
    void operator()(Input::BdLitSetAggregate const &lit) const {
        GRINGO_REPORT_LOC(ctx_->logger(), error, lit.loc()) << "unexpected set aggregate " << lit;
        throw std::logic_error("unexpected set aggregate");
    }
    void operator()(Input::BdLitTheoryAtom const &lit) const { build_bd_lit(*ctx_, lit); }
    void operator()(Input::BdLitAggregate const &lit) const { build_bd_lit(*ctx_, lit); }
    void operator()(Input::BdLitSimple const &lit) const {
        build_lit(*ctx_, lit.lit(),
                  [this]<class Lit>(Lit &&glit) { ctx_->body().emplace_back(std::forward<Lit>(glit)); });
    }
    void operator()(Input::BdLitConjunction const &lit) const { build_bd_lit(*ctx_, lit); }

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
                auto states = Ground::UStateVec{};
                for (auto const &stm : ref_comp.stms) {
                    Util::unordered_map<String, size_t> var_map;
                    Input::visit_variables(
                        *stm,
                        [&var_map]([[maybe_unused]] Input::Location const &loc, String var) {
                            var_map.try_emplace(var, var_map.size());
                        },
                        Input::VariableContext::all);
                    auto body = Ground::ULitVec{};
                    auto def_map = Util::unordered_map<Input::Term const *, std::vector<size_t>>{};
                    auto i = size_t{0};
                    for (auto const &[bd, hds] : ref_comp.incomplete) {
                        for (auto const &hd : hds) {
                            def_map[hd].emplace_back(i);
                        }
                        ++i;
                    }
                    auto ctx =
                        BuildContext{*mbr_, *log_, *store_, base_, ref_comp, def_map, gcomp, var_map, body, states};
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
                    state->output(*log_, *store_, *out_);
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
