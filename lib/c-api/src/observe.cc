#include <clingo/backend.h>
#include <clingo/observe.h>

#include <clingo/control/solver.hh>

#include <potassco/aspif.h>

#include "control.hh" // IWYU pragma: keep
#include "lib.hh"

#include <fstream>

namespace {

auto map(Potassco::WeightLitSpan lits) -> clingo_weighted_literal_t * {
    // NOTE: a reinterpret_cast should be safe as well
    thread_local auto ret = std::vector<clingo_weighted_literal_t>{};
    ret.clear();
    for (auto const &lit : lits) {
        ret.emplace_back(lit.lit, lit.weight);
    }
    return ret.data();
}

auto map(Potassco::TruthValue val) -> clingo_external_type_t {
    switch (val) {
        case Potassco::TruthValue::false_: {
            return clingo_external_type_false;
        }
        case Potassco::TruthValue::true_: {
            return clingo_external_type_true;
        }
        case Potassco::TruthValue::free: {
            return clingo_external_type_free;
        }
        case Potassco::TruthValue::release: {
            return clingo_external_type_release;
        }
    }
    Clingo::Util::unreachable();
}

auto map(Potassco::DomModifier val) -> clingo_heuristic_type_t {
    switch (val) {
        case Potassco::DomModifier::factor: {
            return clingo_heuristic_type_factor;
        }
        case Potassco::DomModifier::false_: {
            return clingo_heuristic_type_false;
        }
        case Potassco::DomModifier::init: {
            return clingo_heuristic_type_init;
        }
        case Potassco::DomModifier::level: {
            return clingo_heuristic_type_level;
        }
        case Potassco::DomModifier::sign: {
            return clingo_heuristic_type_sign;
        }
        case Potassco::DomModifier::true_: {
            return clingo_heuristic_type_true;
        }
    }
    Clingo::Util::unreachable();
}

class Observer : public Potassco::AbstractProgram {
  public:
    Observer(clingo_base_t const &base, clingo_observer_t const &obs, void *data)
        : base_{&base}, obs_{&obs}, data_{data} {}
    void initProgram(bool incremental) override {
        if (obs_->init_program != nullptr) {
            handle_error(obs_->init_program(incremental, data_));
        }
    }
    void beginStep() override {
        if (obs_->init_program != nullptr) {
            handle_error(obs_->begin_step(data_));
        }
    }

    void rule(Potassco::HeadType ht, Potassco::AtomSpan head, Potassco::LitSpan body) override {
        if (obs_->rule != nullptr) {
            handle_error(obs_->rule(ht == Potassco::HeadType::choice, head.data(), head.size(), body.data(),
                                    body.size(), data_));
        }
    }
    void rule(Potassco::HeadType ht, Potassco::AtomSpan head, Potassco::Weight_t bound,
              Potassco::WeightLitSpan body) override {
        if (obs_->weight_rule != nullptr) {
            handle_error(obs_->weight_rule(ht == Potassco::HeadType::choice, head.data(), head.size(), bound, map(body),
                                           body.size(), data_));
        }
    }
    void minimize(Potassco::Weight_t prio, Potassco::WeightLitSpan lits) override {
        if (obs_->minimize != nullptr) {
            handle_error(obs_->minimize(prio, map(lits), lits.size(), data_));
        }
    }

    void project(Potassco::AtomSpan atoms) override {
        if (obs_->project != nullptr) {
            handle_error(obs_->project(atoms.data(), atoms.size(), data_));
        }
    }
    void external(Potassco::Atom_t a, Potassco::TruthValue v) override {
        if (obs_->external != nullptr) {
            handle_error(obs_->external(a, map(v), data_));
        }
    }
    void assume(Potassco::LitSpan lits) override {
        if (obs_->assume != nullptr) {
            handle_error(obs_->assume(lits.data(), lits.size(), data_));
        }
    }
    void heuristic(Potassco::Atom_t a, Potassco::DomModifier t, int bias, unsigned prio,
                   Potassco::LitSpan condition) override {
        if (obs_->heuristic != nullptr) {
            handle_error(obs_->heuristic(a, map(t), bias, prio, condition.data(), condition.size(), data_));
        }
    }
    void acycEdge(int s, int t, Potassco::LitSpan condition) override {
        if (obs_->acyc_edge != nullptr) {
            handle_error(obs_->acyc_edge(s, t, condition.data(), condition.size(), data_));
        }
    }

    void endStep() override {
        if (obs_->end_step != nullptr) {
            handle_error(obs_->end_step(base_, data_));
        }
    }

    // NOTE: the functions below are currently not used because there are other
    // means to inspect atoms and theory data.
    void outputAtom([[maybe_unused]] Potassco::Atom_t a, [[maybe_unused]] std::string_view str) override {}
    void outputTerm([[maybe_unused]] Potassco::Id_t termId, [[maybe_unused]] std::string_view str) override {}
    void output([[maybe_unused]] Potassco::Id_t termId, [[maybe_unused]] Potassco::LitSpan condition) override {}

    void theoryTerm([[maybe_unused]] Potassco::Id_t termId, [[maybe_unused]] int number) override {}
    void theoryTerm([[maybe_unused]] Potassco::Id_t termId, [[maybe_unused]] std::string_view name) override {}
    void theoryTerm([[maybe_unused]] Potassco::Id_t termId, [[maybe_unused]] int cId,
                    [[maybe_unused]] Potassco::IdSpan args) override {}
    void theoryElement([[maybe_unused]] Potassco::Id_t elementId, [[maybe_unused]] Potassco::IdSpan terms,
                       [[maybe_unused]] Potassco::LitSpan cond) override {}
    void theoryAtom([[maybe_unused]] Potassco::Id_t atomOrZero, [[maybe_unused]] Potassco::Id_t termId,
                    [[maybe_unused]] Potassco::IdSpan elements) override {}
    void theoryAtom([[maybe_unused]] Potassco::Id_t atomOrZero, [[maybe_unused]] Potassco::Id_t termId,
                    [[maybe_unused]] Potassco::IdSpan elements, [[maybe_unused]] Potassco::Id_t op,
                    [[maybe_unused]] Potassco::Id_t rhs) override {}

  private:
    clingo_base_t const *base_;
    clingo_observer_t const *obs_;
    void *data_;
};

} // namespace

extern "C" auto clingo_control_observe(clingo_control_t *control, clingo_observer_t const *observer, void *data,
                                       bool preprocess) -> bool {
    CLINGO_TRY {
        if (control == nullptr || observer == nullptr) {
            return fail_arguments();
        }
        // NOLINTNEXTLINE
        auto &prg = const_cast<Clasp::Asp::LogicProgram &>(control->slv->clasp_program());
        if (preprocess) {
            prg.endProgram();
        }

        // NOLINTNEXTLINE
        auto const &base = reinterpret_cast<clingo_base_t const &>(*control->slv);
        Observer obs{base, *observer, data};
        prg.accept(obs, true);
    }
    CLINGO_CATCH;
}

namespace {

class ExtendedAspifWriter : public Potassco::AspifOutput {
  public:
    ExtendedAspifWriter(Clingo::Control::SymbolTable &sym_tab, Clingo::Control::BaseView &view, std::ostream &out)
        : Potassco::AspifOutput{out}, sym_tab_{&sym_tab} {
        sym_tab_->init(view, out);
    }
    void initProgram(bool inc) override {
        sym_tab_->out() << "asp 2 0 0";
        if (inc) {
            sym_tab_->out() << " incremental";
        }
        sym_tab_->out() << " symbols\n";
    }

    //! Disable output table.
    void outputAtom([[maybe_unused]] Potassco::Atom_t a, [[maybe_unused]] std::string_view str) override {}
    //! Disable output table.
    void outputTerm([[maybe_unused]] Potassco::Id_t termId, [[maybe_unused]] std::string_view str) override {}
    //! Output shown term ids before outputting their conditions.
    void beginStep() override {
        AspifOutput::beginStep();
        sym_tab_->begin_step();
    }
    //! Output shown atoms before the end step directive.
    void endStep() override {
        sym_tab_->end_step();
        AspifOutput::endStep();
    }

  private:
    Clingo::Control::SymbolTable *sym_tab_;
};

} // namespace

extern "C" auto clingo_control_write_aspif(clingo_control_t *control, char const *path, size_t size,
                                           clingo_write_aspif_mode_t mode) -> bool {
    CLINGO_TRY {
        auto path_view = std::string_view{path, size};
        if (control == nullptr || path == nullptr) {
            return fail_arguments();
        }
        // NOLINTNEXTLINE
        auto &prg = const_cast<Clasp::Asp::LogicProgram &>(control->slv->clasp_program());
        if ((mode & clingo_write_aspif_mode_preprocess) != 0) {
            prg.endProgram();
        }
        auto app = (mode & clingo_write_aspif_mode_append) != 0 && std::filesystem::exists(path_view);
        auto pre = (mode & clingo_write_aspif_mode_preamble) != 0;
        if ((mode & clingo_write_aspif_mode_preamble_auto) != 0) {
            pre = !app;
        }
        auto out = std::ofstream{std::string{path_view}, app ? std::ios::app : std::ios::out};
        out.exceptions(std::ios::failbit | std::ios::badbit);
        if ((mode & clingo_write_aspif_mode_symbols) != 0) {
            auto obs = ExtendedAspifWriter{control->slv->sym_tab(), *control->slv, out};
            prg.accept(obs, pre);
            if (!pre) {
                obs.endStep();
            }
        } else {
            auto obs = Potassco::AspifOutput{out};
            prg.accept(obs, pre);
            // Clingo's aspif reader requires all programs to be zero terminated.
            // Thus, we call endStep here.
            if (!pre) {
                obs.endStep();
            }
        }
    }
    CLINGO_CATCH;
}
