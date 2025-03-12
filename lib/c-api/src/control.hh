#pragma once

#include "lib.hh"

#include <clingo/control/solver.hh>

#include <clingo/util/algorithm.hh>

#include <clingo/control.h>

#include <forward_list>

struct clingo_control {
    clingo_lib_t *lib;
    Clingo::Control::Solver *slv;
    Clasp::ClaspConfig *cfg;
    Clasp::ClaspFacade *clasp;
};

inline auto convert(Clingo::Input::ProgramParamVec const &parts) {
    return Clingo::Util::transform(parts, [](auto const &part) {
        return clingo_part_t{part.first->c_str(), c_cast(part.second.data()), part.second.size()};
    });
}

inline auto convert(std::vector<Clingo::Input::ProgramParamVec> const &parts) {
    auto vecs = std::forward_list<std::vector<clingo_part_t>>{};
    auto res = Clingo::Util::transform(parts, [&vecs](auto const &part) {
        auto &val = vecs.emplace_front(convert(part));
        return clingo_parts_array_t{val.data(), val.size()};
    });
    return std::pair{std::move(res), std::move(vecs)};
}

inline auto convert(std::optional<std::vector<Clingo::Input::ProgramParamVec>> const &parts) {
    if (parts) {
        return convert(*parts);
    }
    static constexpr clingo_part_t cpart = {"base", nullptr, 0};
    return std::pair{Clingo::Util::make_vec<clingo_parts_array_t>(clingo_parts_array_t{&cpart, 1}),
                     std::forward_list<std::vector<clingo_part_t>>{}};
}

inline auto convert(clingo_control_t *control, clingo_part_t const *parts, size_t parts_size)
    -> std::vector<Clingo::Input::ProgramParam> {
    auto make_part = [&](auto const &sym) { return Clingo::SharedSymbol{Clingo::Symbol::from_rep(sym)}; };
    auto make_parts = [&](auto const &part) {
        return Clingo::Input::ProgramParam{control->lib->store->string(part.name),
                                           Clingo::Util::transform(part.params, part.params + part.size, make_part)};
    };
    return Clingo::Util::transform(parts, parts + parts_size, make_parts);
}

inline auto convert(clingo_control_t *control, clingo_parts_array_t const *part_array, size_t size) {
    return Clingo::Util::transform(part_array, part_array + size,
                                   [&](clingo_parts_array_t const &x) { return convert(control, x.parts, x.size); });
}
