#include <clingo/backend.hh>
#include <clingo/control.hh>

#include <array>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <vector>

namespace {

//! Print the models found during solving.
class ModelPrinter : public Clingo::SolveEventHandler {
  private:
    auto do_model(Clingo::Model model) -> bool override {
        std::cout << "Answer:";
        for (auto const &sym : model.symbols()) {
            std::cout << " " << sym;
        }
        std::cout << "\n";
        return true;
    }
};

//! Get the program literal of the given propositional atom.
auto lookup(Clingo::Library const &lib, Clingo::Base const &base, std::string_view name) -> Clingo::ProgramLiteral {
    auto atom = base.get(Clingo::Function(lib, name));
    if (!atom) {
        throw std::logic_error("atom not found in base");
    }
    return atom->literal();
}

} // namespace

//! Simple example showing how to use the backend to add rules to a grounded program.
auto main(int argc, char const *argv[]) -> int {
    try {
        auto args = std::vector<std::string_view>{};
        for (auto const *arg : std::span{argv, static_cast<size_t>(argc)}.subspan(1)) {
            args.emplace_back(arg);
        }

        auto lib = Clingo::Library{};
        auto ctl = Clingo::Control{lib, args};
        ctl.parse_string("{a; b; c}.");
        ctl.ground();

        // get the program literals of atoms a, b, and c
        auto base = ctl.base();
        auto lit_a = lookup(lib, base, "a");
        auto lit_b = lookup(lib, base, "b");
        auto lit_c = lookup(lib, base, "c");

        {
            // the backend is closed when it goes out of scope
            auto backend = ctl.backend();
            // add an additional atom (referred to as d below)
            auto atom_d = backend.atom(std::nullopt);
            auto lit_d = static_cast<Clingo::ProgramLiteral>(atom_d);
            // add rule: d :- a, b.
            backend.rule(std::array{atom_d}, std::array{lit_a, lit_b});
            // add rule: :- not d, c.
            backend.rule({}, std::array{-lit_d, lit_c});
        }

        std::ignore = ctl.solve({}, ModelPrinter{});
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
