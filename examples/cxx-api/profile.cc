#include <clingo/control.hh>
#include <clingo/profile.hh>

#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <variant>

namespace {

//! Print the given profile node and its children.
//!
//! The timings are not printed because they differ from run to run.
void print(Clingo::ProfileNode const &node, size_t depth) {
    auto indent = std::string(2 * depth, ' ');
    if (auto const *internal = std::get_if<Clingo::ProfileNodeInternal>(&node); internal != nullptr) {
        std::cout << indent << internal->key << "\n";
        for (auto const &child : internal->children) {
            print(child, depth + 1);
        }
    } else {
        auto const &leaf = std::get<Clingo::ProfileNodeLeaf>(node);
        std::cout << indent << (leaf.type == Clingo::ProfileType::step ? "step" : "accu")
                  << ": matches=" << leaf.matches << " instances=" << leaf.instances << "\n";
    }
}

} // namespace

//! Simple example showing how to inspect profiling data gathered during grounding.
auto main() -> int {
    try {
        // profiling has to be enabled via the --profile option
        auto lib = Clingo::Library{};
        auto ctl = Clingo::Control{lib, {"--profile"}};
        ctl.parse_string("p(1..3). q(X) :- p(X).");
        ctl.ground();

        // print the profile tree
        for (auto const &node : ctl.profile()) {
            print(node, 0);
        }
    } catch (std::exception const &e) {
        std::cerr << "Error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
