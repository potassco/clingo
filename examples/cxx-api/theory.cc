#include <clingo/control.hh>
#include <clingo/solve.hh>
#include <clingo/stats.hh>
#include <clingo/theory.hh>

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <string_view>
#include <tuple>

namespace {

// {{{1 theory

//! A partial theory counting models.
//!
//! Theories implement the C interface ::clingo_theory_t so that they can be
//! loaded from shared libraries. Only some of the callbacks are implemented
//! here; the remaining ones are set to NULL indicating that they are not
//! supported.
class Counter {
  public:
    //! Create the theory.
    static auto create([[maybe_unused]] clingo_lib_t *lib, clingo_theory_t *theory) -> bool {
        CLINGO_TRY {
            auto counter = std::make_unique<Counter>();
            *theory = clingo_theory_t{};
            theory->info = &info;
            theory->destroy = &destroy;
            theory->register_theory = &register_theory;
            theory->prepare = &prepare;
            theory->on_model = &on_model;
            theory->on_stats = &on_stats;
            theory->self = counter.release();
        }
        CLINGO_CATCH;
    }

  private:
    //! Get the name and version of the theory.
    static auto info([[maybe_unused]] void *self, clingo_string_t *name, int *major, int *minor, int *revision,
                     clingo_string_t *hash) -> bool {
        // all output parameters are optional
        if (name != nullptr) {
            static constexpr auto theory_name = std::string_view{"counter"};
            *name = clingo_string_t{theory_name.data(), theory_name.size()};
        }
        if (major != nullptr) {
            *major = 1;
        }
        if (minor != nullptr) {
            *minor = 0;
        }
        if (revision != nullptr) {
            *revision = 0;
        }
        if (hash != nullptr) {
            *hash = clingo_string_t{nullptr, 0};
        }
        return true;
    }

    //! Free the state of the theory.
    static void destroy(void *self) { std::ignore = std::unique_ptr<Counter>{static_cast<Counter *>(self)}; }

    //! Register the theory with the control object.
    static auto register_theory([[maybe_unused]] void *self, [[maybe_unused]] clingo_control_t *control) -> bool {
        // a theory might register propagators or add theory definitions here
        return true;
    }

    //! Prepare the theory before solving.
    static auto prepare(void *self, [[maybe_unused]] clingo_control_t *control) -> bool {
        static_cast<Counter *>(self)->models_ = 0;
        return true;
    }

    //! Count the models found.
    static auto on_model(void *self, [[maybe_unused]] clingo_model_t *model) -> bool {
        static_cast<Counter *>(self)->models_ += 1;
        return true;
    }

    //! Add the number of models found to the user statistics.
    static auto on_stats(void *self, clingo_stats_t *stats) -> bool {
        CLINGO_TRY {
            uint64_t root = 0;
            if (!clingo_stats_root(stats, &root)) {
                return false;
            }
            auto step = Clingo::Stats{stats, root}.map().insert(Clingo::stats_step_root(), Clingo::StatsType::map);
            auto counter = step.map().insert("counter", Clingo::StatsType::value);
            counter.value(static_cast<double>(static_cast<Counter *>(self)->models_));
        }
        CLINGO_CATCH;
    }

    uint64_t models_ = 0;
};

// {{{1 application

//! Forward models and statistics to the theory.
class Handler : public Clingo::SolveEventHandler {
  public:
    explicit Handler(Clingo::Theory const &theory) : theory_{&theory} {}

  private:
    auto do_model(Clingo::Model model) -> bool override {
        theory_->model(model);
        return true;
    }

    void do_stats(Clingo::Stats step, Clingo::Stats accu) override { theory_->stats(step, accu); }

    Clingo::Theory const *theory_;
};

} // namespace

//! Simple example showing how to implement a theory and how to drive it.
auto main() -> int {
    try {
        // user statistics are only reported with option --stats
        auto lib = Clingo::Library{};
        auto ctl = Clingo::Control{lib, {"0", "--stats"}};

        // create the theory and print its name and version
        auto theory = Clingo::Theory{lib, &Counter::create};
        auto [major, minor, revision] = theory.version();
        std::cout << "theory " << theory.name() << " version " << major << "." << minor << "." << revision << "\n";

        // register the theory, ground, and prepare the theory for solving
        theory.register_theory(ctl);
        ctl.parse_string("1 {a; b; c}.");
        ctl.ground();
        theory.prepare(ctl);

        // solve passing models and statistics to the theory
        std::ignore = ctl.solve({}, Handler{theory});

        // print the statistics added by the theory
        auto counted = ctl.stats()[Clingo::stats_step_root()]["counter"].value();
        std::cout << "models counted by theory: " << counted << "\n";
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
