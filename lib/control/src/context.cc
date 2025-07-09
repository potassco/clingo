#include <algorithm>
#include <clingo/control/context.hh>

#include <clingo/input/print.hh>

namespace CppClingo::Control {

auto ProfileProgram::add(Input::Stm const &stm) -> Ground::ProfileNodeInternal & {
    return *nodes_
                .emplace(&stm,
                         std::make_unique<Ground::ProfileNodeExpression<std::reference_wrapper<Input::Stm const>>>(stm))
                .first.value();
}

void ProfileProgram::print(std::ostream &out) {
    std::vector<Ground::ProfileNodeInternal const *> sorted_nodes;
    sorted_nodes.reserve(nodes_.size());
    for (auto const &kv : nodes_) {
        sorted_nodes.push_back(kv.second.get());
    }
    std::ranges::stable_sort(sorted_nodes, [](auto const *a, auto const *b) { return a->score() > b->score(); });
    for (auto const *node : sorted_nodes) {
        node->print(out, Ground::ProfileIndent{0});
    }
}

} // namespace CppClingo::Control
