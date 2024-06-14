#pragma once

#include <gringo/ground/statement.hh>

#include <gringo/util/enum.hh>

namespace Gringo::Ground {

//! @addtogroup ground_program
//! @{

//! Captures statements dependening cyclically on each other.
class Component {
  public:
    //! Add a statement to a component.
    void add(UStm stm) { stms_.emplace_back(std::move(stm)); }
    //! Get the statements in the component.
    [[nodiscard]] auto stms() const -> UStmVec const & { return stms_; }

  private:
    UStmVec stms_;
};

//! @}

} // namespace Gringo::Ground
