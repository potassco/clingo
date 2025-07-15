#include "clingo/profile.h"

#include "control.hh" // IWYU pragma: keep
#include "lib.hh"

using namespace CppClingo::CAPI;

static_assert(static_cast<int>(CppClingo::Ground::ProfileType::accu) == static_cast<int>(clingo_profile_type_accu));
static_assert(static_cast<int>(CppClingo::Ground::ProfileType::step) == static_cast<int>(clingo_profile_type_step));

extern "C" auto clingo_control_profile(clingo_control_t const *control, clingo_profile_visitor_t const *visit,
                                       void *data) -> bool {
    CLINGO_TRY {
        if (control == nullptr || visit == nullptr) {
            return fail_arguments();
        }
        control->slv->accept([data, visit](auto const &x, size_t depth) {
            handle_error(std::visit(
                [data, visit, depth]<typename T>(T const &x) {
                    if constexpr (std::same_as<T, std::pair<std::string_view, bool>>) {
                        return visit->internal(depth, x.first.data(), x.first.size(), x.second, data);
                    } else if constexpr (std::same_as<T, std::pair<CppClingo::Ground::ProfileStats const *,
                                                                   CppClingo::Ground::ProfileType>>) {
                        auto vals = clingo_profile_data_t{x.first->matches, x.first->instances,
                                                          x.first->time_instantiate, x.first->time_propagate};
                        return visit->leaf(depth, &vals, static_cast<clingo_profile_type_t>(x.second), data);
                    }
                },
                x));
        });
    }
    CLINGO_CATCH;
}
