#pragma once

#include <gringo/ground/term.hh>
#include <gringo/ground/theory.hh>

#include <memory_resource>

namespace Gringo::Ground {

using TheoryRGuard = std::optional<std::pair<String, UTheoryTerm>>;

class StateBdTheory {
  public:
    StateBdTheory(std::pmr::monotonic_buffer_resource &mbr, VariableVec global, UTerm name, TheoryRGuard guard)
        : mbr_{&mbr}, global_{std::move(global)}, name_{std::move(name)}, guard_{std::move(guard)} {}

    //! Output all previously output theory atoms.
    void output(OutputStm &out);

    [[nodiscard]] auto name() const -> UTerm const & { return name_; }

    [[nodiscard]] auto guard() const -> TheoryRGuard const & { return guard_; }

  private:
    std::pmr::monotonic_buffer_resource *mbr_;
    VariableVec global_;
    UTerm name_;
    TheoryRGuard guard_;
};

} // namespace Gringo::Ground
