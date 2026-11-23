#include <clingo/base.hh>
#include <clingo/control.hh>

#include <cstdlib>
#include <exception>
#include <iostream>

//! Simple example showing how to iterate over the symbolic atoms of a grounded program.
auto main() -> int {
    try {
        auto lib = Clingo::Library{};
        auto ctl = Clingo::Control{lib};
        ctl.parse_string("a. {b}. #external c.");
        ctl.ground();

        // iterate over the signatures in the base and print their atoms
        auto base = ctl.base();
        std::cout << "Symbolic atoms:\n";
        for (auto [sig, atoms] : base) {
            auto [name, arity, positive] = sig;
            std::cout << "  " << (positive ? "" : "-") << name << "/" << arity << ":\n";
            for (auto [sym, atom] : atoms) {
                std::cout << "    " << sym;
                // check whether the atom is a fact or an external
                if (base.is_fact(atom.literal())) {
                    std::cout << ", fact";
                }
                if (base.is_external(atom.literal())) {
                    std::cout << ", external";
                }
                std::cout << "\n";
            }
        }
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
