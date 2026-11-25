#include <clingo/control.hh>
#include <clingo/propagate.hh>

#include <array>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <tuple>

namespace {

//! Propagator enforcing that atom a holds if and only if atom b holds.
class AIFFB : public Clingo::Propagator {
  private:
    //! Get the solver literal of the given propositional atom and watch it.
    static auto watch(Clingo::PropagateInit &init, std::string_view name) -> Clingo::SolverLiteral {
        auto atom = init.base().get(Clingo::Function(init.library(), name));
        if (!atom) {
            throw std::logic_error("atom not found in base");
        }
        auto lit = init.solver_literal(atom->literal());
        init.add_watch(lit);
        return lit;
    }

    //! Initialize the propagator watching atoms a and b.
    void do_init([[maybe_unused]] Clingo::Assignment assignment, Clingo::PropagateInit init) override {
        a_ = watch(init, "a");
        b_ = watch(init, "b");
    }

    //! Propagate that a implies b and b implies a.
    void do_propagate([[maybe_unused]] Clingo::Assignment assignment, Clingo::PropagateControl ctl,
                      Clingo::SolverLiteralSpan changes) override {
        for (auto lit : changes) {
            // if a is true, then b has to be true and vice versa
            auto clause = lit == a_ ? std::array{-a_, b_} : std::array{-b_, a_};
            // stop propagation if the clause caused a conflict
            if (!ctl.add_clause(clause)) {
                return;
            }
        }
    }

    Clingo::SolverLiteral a_ = 0;
    Clingo::SolverLiteral b_ = 0;
};

//! Print the models found during solving.
class ModelPrinter : public Clingo::SolveEventHandler {
  private:
    auto do_model(Clingo::Model model) -> bool override {
        std::cout << "Model:";
        for (auto const &sym : model.symbols()) {
            std::cout << " " << sym;
        }
        std::cout << "\n";
        return true;
    }
};

} // namespace

//! Simple example showing how to implement a propagator enforcing that a holds if and only if b holds.
auto main() -> int {
    try {
        auto lib = Clingo::Library{};
        auto ctl = Clingo::Control{lib, {"0"}};
        // propagators can be passed by value, std::ref, or std::unique_ptr
        ctl.register_propagator(AIFFB{});
        ctl.parse_string("1 {a; b}.");
        ctl.ground();
        std::ignore = ctl.solve({}, ModelPrinter{});
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
