#include <clingo/control.hh>
#include <clingo/solve.hh>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <string_view>
#include <tuple>
#include <utility>

namespace {

//! Inspect the models found during solving.
class ModelInspector : public Clingo::SolveEventHandler {
  public:
    explicit ModelInspector(Clingo::Symbol atom) : atom_{std::move(atom)} {}

  private:
    //! Print the symbols of the given type in the model.
    static void print(Clingo::Model const &model, std::string_view label, Clingo::ShowFlags show) {
        std::cout << "  " << label << ":";
        for (auto const &sym : model.symbols(show)) {
            std::cout << " " << sym;
        }
        std::cout << "\n";
    }

    auto do_model(Clingo::Model model) -> bool override {
        // print the number and type of the model
        auto const *type = model.type() == Clingo::ModelType::stable_model ? "stable model" : "consequences";
        std::cout << "Model " << model.number() << " (" << type << "):\n";

        // print the symbols of the model selecting different types
        print(model, "shown", Clingo::ShowFlags::shown);
        print(model, "atoms", Clingo::ShowFlags::atoms);
        print(model, "terms", Clingo::ShowFlags::terms);

        // check if the atom is contained in the model
        std::cout << "  contains " << atom_ << ": " << (model.contains(atom_) ? "yes" : "no") << "\n";
        return true;
    }

    Clingo::Symbol atom_;
};

} // namespace

//! Simple example showing how to inspect models.
auto main() -> int {
    try {
        auto lib = Clingo::Library{};
        auto ctl = Clingo::Control{lib, {"0"}};
        ctl.parse_string("1 {a; b} 1. #show c : b. #show a/0.");
        ctl.ground();
        std::ignore = ctl.solve({}, ModelInspector{Clingo::Function(lib, "a")});
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
