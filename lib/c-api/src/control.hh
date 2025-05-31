#pragma once

#include "lib.hh"

#include <clingo/control/solver.hh>

#include <clingo/util/algorithm.hh>

#include <clingo/control.h>

struct clingo_control {
    //! Construct a control object that is bound later.
    //!
    //! See bind().
    clingo_control(clingo_lib_t *lib) : lib{lib} {
        assert(lib != nullptr);
        clingo_lib_acquire(lib);
    }
    //! Construct a control that owns the given clasp and clingo objects.
    clingo_control(clingo_lib_t *lib, std::unique_ptr<CppClingo::Control::Solver> slv,
                   std::unique_ptr<Clasp::ClaspConfig> cfg, std::unique_ptr<Clasp::ClaspFacade> clasp)
        : lib{lib}, slv{slv.release()}, cfg{cfg.release()}, clasp{clasp.release()}, own{true} {
        assert(lib != nullptr);
        clingo_lib_acquire(lib);
        // NOTE: we set the user data of the solver to this to allow for
        // obtaining this control object when mapping callbacks where the
        // solver is passed as an argument.
        this->slv->user_data() = this;
    }
    clingo_control(clingo_control const &other) = delete;
    clingo_control(clingo_control &&other) noexcept = delete;
    auto operator=(clingo_control const &other) -> clingo_control & = delete;
    auto operator=(clingo_control &&other) noexcept -> clingo_control & = delete;
    //! Destroy the held clasp and clingo objects if the control is owning.
    ~clingo_control() {
        if (own) {
            delete slv;
            delete clasp;
            delete cfg;
        }
        clingo_lib_release(lib);
    }
    //! Bind the control object to the clasp and clingo objects.
    //!
    //! The control will not own these objects and not delete them.
    void bind(CppClingo::Control::Solver *slv, Clasp::ClaspConfig *cfg, Clasp::ClaspFacade *clasp) {
        assert(!own);
        this->slv = slv;
        this->cfg = cfg;
        this->clasp = clasp;
        // NOTE: see the note in the owning constructor.
        this->slv->user_data() = this;
        own = false;
    }
    clingo_lib_t *lib;
    CppClingo::Control::Solver *slv = nullptr;
    Clasp::ClaspConfig *cfg = nullptr;
    Clasp::ClaspFacade *clasp = nullptr;
    std::atomic<size_t> ref_count = 1;
    bool own = false;
};

namespace CppClingo::CAPI {

inline auto convert(CppClingo::Input::ProgramParamVec const &parts) -> std::vector<clingo_part_t> {
    return CppClingo::Util::to_vec(parts, [](auto const &part) {
        return clingo_part_t{part.first->data(), part.first->size(), c_cast(part.second.data()), part.second.size()};
    });
}

inline auto convert(std::optional<CppClingo::Input::ProgramParamVec> const &parts) {
    if (parts) {
        return convert(*parts);
    }
    static constexpr clingo_part_t cpart = {"base", 4, nullptr, 0};
    return std::vector{cpart};
}

inline auto convert(clingo_control_t *control, clingo_part_t const *parts, size_t parts_size)
    -> CppClingo::Input::ProgramParamVec {
    auto make_part = [&](auto const &sym) { return CppClingo::SharedSymbol{CppClingo::Symbol::from_rep(sym)}; };
    auto make_parts = [&](auto const &part) {
        return CppClingo::Input::ProgramParam{
            control->lib->store->string({part.name, part.name_size}),
            CppClingo::Util::to_vec(part.params, part.params + part.params_size, make_part)};
    };
    return CppClingo::Util::to_vec(parts, parts + parts_size, make_parts);
}

} // namespace CppClingo::CAPI
