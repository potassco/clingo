#include <clingo/control/grounder.hh>
#include <clingo/control/parse.hh>
#include <clingo/control/statement.hh>

#include <clingo/ground/program.hh>

#include <clingo/input/print.hh>

#include <clingo/input/rewrite/evaluate.hh>
#include <clingo/input/rewrite/visit_variables.hh>

#include <clingo/util/print.hh>
#include <clingo/util/type_traits.hh>
#include <clingo/util/unordered_map.hh>

#ifdef CLINGO_PROFILE
#include <gperftools/profiler.h>
#endif

#include <iostream>

namespace CppClingo::Control {

namespace {

#ifdef CLINGO_PROFILE

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
    Builder(std::pmr::monotonic_buffer_resource &mbr, Logger &log, SymbolStore &store, TheorySigVec theory_directives,
            Ground::Bases &base, Ground::ScriptCallback *context, OutputStm &out)
        : mbr_{&mbr}, log_{&log}, store_{&store}, theory_directives_{std::move(theory_directives)}, bases_{&base},
          context_{context}, out_{&out} {}

  private:
    //! Handle program parameters.
    void do_param(Input::ProgramParam const &param) override {
        buf_.str({});
        buf_ << "#program_" << *param.first;
        auto name = store_->string_ref(buf_.view());
        auto &base = bases_->add_base(std::make_tuple(name, param.second.size(), false));
        base.add(store_->fun_ref(name, as_symbol_span(param.second), false), Ground::StateAtom::fact,
                 []() { return size_t{1}; });
    }

    //! Handle meta statements.
    void do_meta([[maybe_unused]] std::vector<Input::Stm> const &stms) override {}

    //! Handle facts.
    void do_fact(std::vector<Symbol> const &facts) override {
        for (auto const &fact : facts) {
            auto &base = bases_->add_base(std::make_tuple(fact.name(), fact.args().size(), fact.has_sign()));
            auto [it, state] = base.add(fact, Ground::StateAtom::fact, [this]() { return out_->uid(true); });
            out_->fact(fact, it->second.id);
        }
    }

    //! Translate components.
    auto do_components(Input::Components const &comps) -> bool override {
        auto lin = Ground::Linearizer{*mbr_};
        for (auto const &ref_comps : comps) {
            CLINGO_REPORT(*log_, debug) << "  component";
            for (auto const &ref_comp : ref_comps) {
                CLINGO_REPORT(*log_, debug) << "    refined component";
                // A component is classified w.r.t. to previously accumulated
                // atoms. It is domain if it is positive (i.e., contains no
                // negative cycle) and all bases it depends on are domain.
                // A domain component only derives facts.
                bool domain = intersects(ref_comp.type, Input::ComponentType::positive) &&
                              std::ranges::all_of(ref_comp.depend,
                                                  [this](auto const &sig) { return bases_->add_base(sig).domain(); });
                auto gcomp = Ground::Component{domain};
                auto states = Ground::UStateVec{};
                for (auto const &stm : ref_comp.stms) {
                    Util::unordered_map<String, size_t> var_map;
                    Input::visit_variables(
                        *stm,
                        [&var_map]([[maybe_unused]] Location const &loc, String var) {
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
                    auto ctx = BuildContext{*mbr_,   *log_,    *store_, theory_directives_,
                                            *bases_, ref_comp, def_map, gcomp,
                                            var_map, body,     states,  context_};
                    build_stm(ctx, *stm);
                }
                auto queue = Ground::Queue{};
                lin.start(queue);
                for (auto const &stm : gcomp.stms()) {
                    CLINGO_REPORT(*log_, debug) << "      " << *stm;
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
    TheorySigVec theory_directives_;
    Ground::Bases *bases_;
    Ground::ScriptCallback *context_;
    OutputStm *out_;
    std::ostringstream buf_;
};

} // namespace

//! Class storing/hiding relevant state for grounding.
struct Grounder::Impl : CppClingo::SymbolOwner {
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
        CLINGO_REPORT(*log, trace) << "mark owners";
        for (auto const &[key, base] : bases.atoms()) {
            CLINGO_REPORT(*log, trace) << "  mark domain: " << (std::get<2>(key) ? "-" : "") << std::get<0>(key) << "/"
                                       << std::get<1>(key);
            gc.mark(std::get<0>(key));
            base->mark(gc);
        }
        for (auto const &[key, state] : bases.projected()) {
            CLINGO_REPORT(*log, trace) << "  mark projection domain: " << *key;
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
        bool show_all = true;
        auto show_base = [this](auto &base) {
            for (auto i = base.mark_shown(), n = base.size(); i != n; ++i) {
                auto it = base.nth(i);
                auto const &atom = it.key();
                out->show_atom(atom, it.value().id);
            }
        };
        for (auto const &stm : prg.meta_stms()) {
            std::visit(
                [&, this]<class T>(T const &stm) {
                    if constexpr (Util::matches<T, Input::StmProjectSig>) {
                        if (auto *base = bases.get_base({stm.name(), stm.arity(), stm.sign()}); base != nullptr) {
                            for (auto i = base->mark_projected(), n = base->size(); i != n; ++i) {
                                auto atom = base->nth(i);
                                out->project(atom.key(), atom.value().id);
                            }
                        }
                    } else if constexpr (Util::matches<T, Input::StmShowNothing>) {
                        show_all = false;
                    } else if constexpr (Util::matches<T, Input::StmShowSig>) {
                        show_all = false;
                        if (auto *base = bases.get_base(Input::Sig{stm.name(), stm.arity(), stm.sign()});
                            base != nullptr) {
                            show_base(*base);
                        }
                    }
                },
                stm);
        }
        if (show_all) {
            for (auto const &[sig, base] : bases.atoms()) {
                show_base(*base);
            }
        }
        // handle classical negation
        for (auto const &[neg_sig, neg_base] : bases.atoms()) {
            if (get<2>(neg_sig)) {
                auto old_neg = neg_base->mark_negated();
                if (auto it = bases.atoms().find({get<0>(neg_sig), get<1>(neg_sig), false});
                    it != bases.atoms().end()) {
                    auto const &pos_base = it.value();
                    auto old_pos = pos_base->mark_negated();
                    // add constraints `:- -p, p.` for each fresh atom `-p`
                    for (auto i = old_neg, e = neg_base->size(); i != e; ++i) {
                        auto jt = neg_base->nth(i);
                        // NOLINTNEXTLINE
                        if (auto kt = pos_base->find(*jt->first.flip_classical_sign()); kt) {
                            out->classical_negation(jt->second.id, kt->value().id);
                        }
                    }
                    // add constraints `:- -p, p.` for each fresh atom `p` and old atom `-p`
                    if (old_neg > 0) {
                        for (auto i = old_pos, e = pos_base->size(); i != e; ++i) {
                            auto jt = pos_base->nth(i);
                            // NOLINTNEXTLINE
                            if (auto j = neg_base->index(*jt->first.flip_classical_sign()); j < old_neg) {
                                out->classical_negation(jt->second.id, neg_base->nth(j).value().id);
                            }
                        }
                    }
                }
            }
        }
    }

    //! Ensure that rules for all projected atoms have been added.
    void project() const {
        for (auto const &[term, base] : bases.projected()) {
            base->init(Ground::InstantiationContext{*log, *store, *out}, 0);
        }
    }

    //! Cleanup step-local state accumulated during grounding.
    //!
    //! Clears indices associated with domains.
    void clear() {
        for (auto const &[key, base] : bases.atoms()) {
            base->clear_context();
        }
        for (auto const &[key, state] : bases.projected()) {
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
    //! The atom and term bases.
    Ground::Bases bases;
    //! The output.
    OutputStm *out;
    //! The parts to ground.
    ProgramParams parts;
    //! Indicate that the logic program might still be satisfiable.
    bool is_sat = true;
};

Grounder::Grounder(Logger &log, SymbolStore &store, Input::RewriteOptions opts, OutputStm &out)
    : impl_{std::make_unique<Impl>(log, store, opts, out)} {
}

Grounder::~Grounder() noexcept = default;

void Grounder::add_const(String name, Symbol value) {
    if (impl_->is_sat) {
        auto lock = GCLock{*impl_->store};
        auto str = impl_->store->string_ref("<cli>");
        auto loc = Location(Position{str, 1, 1}, Position{str, 1, 1});
        auto val = Input::TermSymbol{loc, value};
        impl_->unprocessed_prg.add(*impl_->store,
                                   Input::StmConst{std::move(loc), Input::Precedence::override_, name, std::move(val)});
    }
}

void Grounder::join(Input::UnprocessedProgram const &prg) {
    if (impl_->is_sat) {
        impl_->unprocessed_prg.join(prg);
    }
}

auto Grounder::parse(std::string_view str, Ground::ScriptExec *code) -> BuiltinIncludes {
    CLINGO_REPORT(*impl_->log, debug) << "parsing...";
#ifdef CLINGO_PROFILE
    auto prof = Profiler{"clingo-parse.prof"};
#endif
    auto ret = BuiltinIncludes::empty;
    if (impl_->is_sat) {
        GCLock lock{*impl_->store};
        auto prs = ParseHelper{*impl_->log, *impl_->store, impl_->unprocessed_prg, impl_->parts, code};
        prs.process_string(str);
        ret |= prs.process_includes();
        prs.check();
        ret |= prs.process_includes();
    }
    return ret;
}

auto Grounder::parse(std::span<std::string_view const> const &files, Ground::ScriptExec *code, ProgramBackend *prg,
                     TheoryBackend *thy) -> BuiltinIncludes {
    CLINGO_REPORT(*impl_->log, debug) << "parsing...";
#ifdef CLINGO_PROFILE
    auto prof = Profiler{"clingo-parse.prof"};
#endif
    auto ret = BuiltinIncludes::empty;
    if (impl_->is_sat) {
        GCLock lock{*impl_->store};
        auto prs = ParseHelper{*impl_->log, *impl_->store, impl_->unprocessed_prg, impl_->parts, code, prg, thy};
        if (files.empty()) {
            prs.process_stdin();
            ret |= prs.process_includes();
        }
        for (auto const &file : files) {
            if (file == "-") {
                prs.process_stdin();
            } else {
                prs.process_path(file);
            }
            ret |= prs.process_includes();
        }
        prs.check();
    }
    return ret;
}

void Grounder::prepare_() {
    if (!impl_->unprocessed_prg.empty()) {
        CLINGO_REPORT(*impl_->log, debug) << "preparing...";
        if (impl_->is_sat) {
            GCLock lock{*impl_->store};
            impl_->prg.join(*impl_->log, *impl_->store, impl_->unprocessed_prg);
            impl_->unprocessed_prg.clear();
        }
    }
}

auto Grounder::const_map() -> Input::ConstMap const & {
    prepare_();
    return impl_->prg.const_map();
}

auto Grounder::ground(Input::ProgramParamVec const &params, Ground::ScriptCallback *context) -> bool {
    prepare_();
    CLINGO_REPORT(*impl_->log, debug) << "grounding...";
    GCLock lock{*impl_->store};
#ifdef CLINGO_PROFILE
    auto prof = Profiler{"clingo-ground.prof"};
#endif
    if (impl_->is_sat) {
        impl_->prg.check(*impl_->log);
        impl_->bases.clear_aux();
        auto bld = Builder{impl_->mbr,   *impl_->log, *impl_->store, impl_->prg.theory_directives(),
                           impl_->bases, context,     *impl_->out};
        impl_->is_sat = impl_->prg.analyze(*impl_->store, params, bld);
        impl_->meta();
        impl_->project();
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
    prepare_();
    GCLock lock{*impl_->store};
    impl_->prg.visit_stms(*impl_->store, [&out](auto const &stm) { out << stm << "\n"; });
    out.flush();
}

void Grounder::show(Input::SharedSig const &sig) {
    GCLock lock{*impl_->store};
    auto prg = Input::UnprocessedProgram{};
    auto pos = Position{*impl_->store->string("<cmd>"), 1, 1};
    auto loc = Location{pos, pos};
    prg.add(*impl_->store, Input::StmShowSig{loc, get<2>(sig), *get<0>(sig), static_cast<int>(get<1>(sig))});
    impl_->prg.join(*impl_->log, *impl_->store, prg);
}

void Grounder::mark_sig(Input::Sig const &sig) {
    impl_->prg.mark_sig(sig);
}

auto Grounder::get_parts() const -> std::optional<Input::ProgramParamVec> const & {
    return impl_->parts.second;
}

void Grounder::set_parts(std::optional<ProgramParamVec> parts, Input::Precedence prec) {
    impl_->parts.first = prec;
    impl_->parts.second = std::move(parts);
}

auto Grounder::base() -> Ground::Bases & {
    return impl_->bases;
}

auto Grounder::base() const -> Ground::Bases const & {
    return impl_->bases;
}

auto Grounder::store() const -> SymbolStore & {
    return *impl_->store;
}

auto Grounder::log() const -> Logger & {
    return *impl_->log;
}

} // namespace CppClingo::Control
