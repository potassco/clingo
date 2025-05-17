#pragma once

#include <clingo/solve.hh>

namespace Clingo::Test {

class MCB : public SolveEventHandler {
  public:
    MCB(std::vector<std::vector<std::string>> &models) : models_{&models} { models_->clear(); }
    ~MCB() override { std::ranges::sort(*models_); }

    using SolveEventHandler::model;

    auto model(ConstModel &model) -> bool {
        models_->emplace_back();
        for (auto &sym : model.symbols(Clingo::ShowFlags::shown)) {
            models_->back().push_back(sym.to_string());
        }
        std::ranges::sort(models_->back());
        return true;
    }

  private:
    auto do_model(Model &model) -> bool override { return MCB::model(static_cast<ConstModel &>(model)); }
    std::vector<std::vector<std::string>> *models_;
};

} // namespace Clingo::Test
