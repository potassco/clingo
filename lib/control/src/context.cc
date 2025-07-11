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

void ProfileProgram::print(std::ostream &out, Ground::ProfileType type, Ground::ProfileDetail detail) const {
    auto sorted_nodes = std::vector<Ground::ProfileNodeInternal *>{};
    sorted_nodes.reserve(nodes_.size());
    for (auto const &kv : nodes_) {
        sorted_nodes.push_back(kv.second.get());
    }
    std::ranges::stable_sort(sorted_nodes,
                             [=](auto const *a, auto const *b) { return a->score(type) > b->score(type); });
    out << (type == Ground::ProfileType::step ? "Step Grounding Profile:\n" : "Grounding Profile:\n");
    for (auto *node : sorted_nodes) {
        node->print(out, Ground::ProfileIndent{1}, detail, type);
    }
}

void ProfileProgram::begin_step() {
    for (auto const &kv : nodes_) {
        kv.second->begin_step();
    }
}

void ProfileProgram::end_step() {
    for (auto const &kv : nodes_) {
        kv.second->end_step();
    }
}

} // namespace CppClingo::Control
