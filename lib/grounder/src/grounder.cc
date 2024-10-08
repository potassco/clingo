#include <gringo/grounder/grounder.hh>
#include <gringo/grounder/parse.hh>
#include <gringo/grounder/statement.hh>

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

namespace {

#ifdef PARSER_PROFILE

//! Simple profiler to restrict profiling to selected scopes.
class Profiler {
  public:
    //! Construct the profile writing data to the given path.
    Profiler(char const *path) { ProfilerStart(path); }
    //! Stop profiling.
    ~Profiler() { ProfilerStop(); }
};

#endif

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
    void do_meta([[maybe_unused]] std::vector<Input::Stm> const &stms) override {}

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
                    build_stm(ctx, *stm);
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

//! Class storing/hiding relevant state for grounding.
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

    //! Handle meta statements after grounding.
    //!
    //! This currently handles show statements and projection directives.
    void meta() {
        for (auto const &stm : prg.meta_stms()) {
            bool show_all = true;
            auto show_base = [this](auto &base) {
                for (auto i = base.mark_shown(), n = base.size(); i != n; ++i) {
                    auto it = base.nth(i);
                    auto const &atom = it.key();
                    out->body().lit(Sign::none, atom);
                    out->show_term(atom);
                }
            };
            std::visit(
                [&, this]<class T>(T const &stm) {
                    if constexpr (Util::matches<T, Input::StmProjectSig>) {
                        if (auto it = atom_base.find(Input::Sig{stm.name(), stm.arity(), stm.sign()});
                            it != atom_base.end()) {
                            auto &base = *it->second;
                            for (auto i = base.mark_projected(), n = base.size(); i != n; ++i) {
                                auto jt = base.nth(i);
                                auto const &atom = jt.key();
                                out->project(atom);
                            }
                        }
                    } else if constexpr (Util::matches<T, Input::StmShowNothing>) {
                        show_all = false;
                    } else if constexpr (Util::matches<T, Input::StmShowSig>) {
                        show_all = false;
                        if (auto it = atom_base.find(Input::Sig{stm.name(), stm.arity(), stm.sign()});
                            it != atom_base.end()) {
                            show_base(*it->second);
                        }
                    }
                },
                stm);
            if (!show_all) {
                for (auto const &[sig, base] : atom_base) {
                    show_base(*base);
                }
            }
        }
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

void Grounder::parse(std::string_view str) {
    GRINGO_REPORT(*impl_->log, debug) << "parsing...";
    if (impl_->is_sat) {
        GCLock lock{*impl_->store};
        auto prs = ParseHelper{*impl_->log, *impl_->store, impl_->unprocessed_prg};
        prs.process_string(str);
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
    auto prof = Profiler{"clingo-ground.prof"};
#endif
    if (impl_->is_sat) {
        // TODO:
        // - StmDefined
        //   1. could be handled here
        //     -> only statements from added parts are considered
        //     - there might be warnings about parts that are only relevant later
        //   2. could be handled before
        //     -> all statements could be considered
        //     - warnings about parts that have not been added might be omitted
        //   - I have a tendency toward solution 2.
        //   - implementation:
        //     - iterate over program constructing two sets of signatures of defined and required predicates
        //     - before grounding check that the required are a subset of the defined predicates
        //     - the functionality could be provided by Input::Program
        // - StmScript
        //   - this must be handled earlier

        impl_->prg.check(*impl_->log);
        auto bld = Builder{impl_->mbr, *impl_->log, *impl_->store, impl_->atom_base, impl_->project_base, *impl_->out};
        impl_->is_sat = impl_->prg.analyze(*impl_->store, params, bld);
        impl_->meta();
        impl_->clear();
    }
    impl_->out->end_step();
    return impl_->is_sat;
}

void Grounder::output_unprocessed_program(std::ostream &out) {
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
