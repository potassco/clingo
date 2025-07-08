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
    for (auto const &node : nodes_) {
        node.second->print(out, Ground::ProfileIndent{0});
    }
}

} // namespace CppClingo::Control
