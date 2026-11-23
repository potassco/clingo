#include <clingo/ast.hh>
#include <clingo/control.hh>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <tuple>
#include <vector>

namespace {

using Clingo::AST::Attribute;
using Clingo::AST::Node;
using Clingo::AST::NodeType;

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

//! Build ":- not a." as a statement AST.
auto build_constraint(Clingo::Library const &lib, Clingo::Location const &loc) -> Node {
    // construct empty argument tuple "()"
    auto args = Node::create<NodeType::argument_tuple>(lib, std::vector<Node>{});
    // construct function term "a()" equivalent to "a"
    auto term = Node::create<NodeType::term_function>(lib, loc, "a", std::vector{args}, false);
    // construct simple literal "not a"
    auto atom = Node::create<NodeType::literal_symbolic>(lib, loc, Clingo::AST::Sign::single, term);
    // construct body literal "not a"
    auto body = Node::create<NodeType::body_simple_literal>(lib, atom);
    // construct simple literal "#false"
    auto fail = Node::create<NodeType::literal_boolean>(lib, loc, Clingo::AST::Sign::no_sign, false);
    // construct head literal "#false"
    auto head = Node::create<NodeType::head_simple_literal>(lib, fail);
    // construct integrity constraint ":- not a."
    return Node::create<NodeType::statement_rule>(lib, loc, head, std::vector{body});
}

} // namespace

//! Simple example that shows how to work with ASTs in C++.
auto main() -> int {
    try {
        auto lib = Clingo::Library{};
        auto ctl = Clingo::Control{lib, {"0"}};

        // parse "{a}." as a statement AST
        auto parsed = Clingo::AST::parse(lib, "{a}.");
        auto loc = parsed.location(Attribute::location);

        // build ":- not a." as a statement AST
        auto built = build_constraint(lib, loc);

        // create a program, add both statements, and add the program to the control object
        auto prg = Clingo::AST::Program{lib};
        prg.add(parsed);
        prg.add(built);
        ctl.join(prg);

        // ground and solve
        ctl.ground();
        std::ignore = ctl.solve({}, ModelPrinter{});
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
