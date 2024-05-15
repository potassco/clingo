#pragma once

#include <gringo/ground/statement.hh>

#include <gringo/util/enum.hh>

namespace Gringo::Ground {

class Component {
  public:
    void add(UStm stm) { stms_.emplace_back(std::move(stm)); }
    [[nodiscard]] auto stms() const -> UStmVec const & { return stms_; }

  private:
    UStmVec stms_;
};

} // namespace Gringo::Ground
