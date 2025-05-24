#pragma once

#include <clingo/solve.hh>

namespace Clingo::Test {

using MV = std::vector<std::vector<std::string>>;

class MCB : public SolveEventHandler {
  public:
    MCB(MV &models) : models_{&models} { models_->clear(); }
    ~MCB() override { std::ranges::sort(*models_); }

    using SolveEventHandler::model;

    auto model(ConstModel model) -> bool {
        if (!proven && model.optimality_proven()) {
            models_->clear();
            proven = true;
        }
        models_->emplace_back();
        for (auto &sym : model.symbols(Clingo::ShowFlags::shown)) {
            models_->back().push_back(sym.to_string());
        }
        std::ranges::sort(models_->back());
        return true;
    }

  private:
    auto do_model(Model model) -> bool override { return MCB::model(static_cast<ConstModel>(model)); }
    MV *models_;
    bool proven = false;
};

} // namespace Clingo::Test
