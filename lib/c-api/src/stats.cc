#include "clingo/stats.h"

#include "control.hh" // IWYU pragma: keep
#include "core.hh"
#include "lib.hh"

using namespace CppClingo::CAPI;

namespace CppClingo::CAPI {
namespace {

inline auto cpp_cast(clingo_stats_t const *config) -> Potassco::AbstractStatistics const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Potassco::AbstractStatistics const *>(config);
}

inline auto cpp_cast(clingo_stats_t *config) -> Potassco::AbstractStatistics * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Potassco::AbstractStatistics *>(config);
}

inline auto c_cast(Potassco::AbstractStatistics const *config) -> clingo_stats_t const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_stats_t const *>(config);
}

inline auto cpp_cast(clingo_stats_type_e type) -> Potassco::StatisticsType {
    switch (type) {
        case clingo_stats_type_value: {
            return Potassco::StatisticsType::value;
        }
        case clingo_stats_type_map: {
            return Potassco::StatisticsType::map;
        }
        case clingo_stats_type_array: {
            return Potassco::StatisticsType::array;
        }
    }
    throw std::invalid_argument("unexpected enum value");
}

inline auto c_cast(Potassco::StatisticsType type) -> clingo_stats_type_e {
    switch (type) {
        case Potassco::StatisticsType::value: {
            return clingo_stats_type_value;
        }
        case Potassco::StatisticsType::map: {
            return clingo_stats_type_map;
        }
        case Potassco::StatisticsType::array: {
            return clingo_stats_type_array;
        }
        default: {
            throw std::invalid_argument("unexpected enum value");
        }
    }
}

} // namespace
} // namespace CppClingo::CAPI

clingo_string_t const clingo_user_stats_step = {.data = Clasp::ClaspFacade::user_step_stats.data(),
                                                .size = Clasp::ClaspFacade::user_step_stats.size()};
clingo_string_t const clingo_user_stats_accu = {.data = Clasp::ClaspFacade::user_accu_stats.data(),
                                                .size = Clasp::ClaspFacade::user_accu_stats.size()};

extern "C" auto clingo_stats_root(clingo_stats_t const *stats, uint64_t *key) -> bool {
    CLINGO_TRY {
        if (stats == nullptr || key == nullptr) {
            return fail_arguments();
        }
        *key = cpp_cast(stats)->root();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_stats_type(clingo_stats_t const *stats, uint64_t key, clingo_stats_type_t *type) -> bool {
    CLINGO_TRY {
        if (stats == nullptr || type == nullptr) {
            return fail_arguments();
        }
        *type = c_cast(cpp_cast(stats)->type(key));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_stats_array_size(clingo_stats_t const *stats, uint64_t key, size_t *size) -> bool {
    CLINGO_TRY {
        if (stats == nullptr || size == nullptr) {
            return fail_arguments();
        }
        *size = cpp_cast(stats)->size(key);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_stats_array_at(clingo_stats_t const *stats, uint64_t key, size_t offset, uint64_t *subkey)
    -> bool {
    CLINGO_TRY {
        if (stats == nullptr || subkey == nullptr) {
            return fail_arguments();
        }
        *subkey = cpp_cast(stats)->at(key, offset);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_stats_array_push(clingo_stats_t *stats, uint64_t key, clingo_stats_type_t type, uint64_t *subkey)
    -> bool {
    CLINGO_TRY {
        if (stats == nullptr || subkey == nullptr) {
            return fail_arguments();
        }
        *subkey = cpp_cast(stats)->push(key, cpp_cast(static_cast<clingo_stats_type_e>(type)));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_stats_map_size(clingo_stats_t const *stats, uint64_t key, size_t *size) -> bool {
    CLINGO_TRY {
        if (stats == nullptr || size == nullptr) {
            return fail_arguments();
        }
        *size = cpp_cast(stats)->size(key);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_stats_map_has_subkey(clingo_stats_t const *stats, uint64_t key, char const *name, size_t size,
                                            uint64_t *subkey, bool *result) -> bool {
    CLINGO_TRY {
        if (stats == nullptr || (name == nullptr && size > 0) || result == nullptr) {
            return fail_arguments();
        }
        *result = cpp_cast(stats)->find(key, std::string_view{name, size}, subkey);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_stats_map_subkey_name(clingo_stats_t const *stats, uint64_t key, size_t offset,
                                             clingo_string_t *name) -> bool {
    CLINGO_TRY {
        if (stats == nullptr || name == nullptr) {
            return fail_arguments();
        }
        auto str = cpp_cast(stats)->key(key, offset);
        name->data = str.data();
        name->size = str.size();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_stats_map_at(clingo_stats_t const *stats, uint64_t key, char const *name, size_t size,
                                    uint64_t *subkey) -> bool {
    CLINGO_TRY {
        if (stats == nullptr || (name == nullptr && size > 0) || subkey == nullptr) {
            return fail_arguments();
        }
        *subkey = cpp_cast(stats)->get(key, std::string_view{name, size});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_stats_map_add_subkey(clingo_stats_t *stats, uint64_t key, char const *name, size_t size,
                                            clingo_stats_type_t type, uint64_t *subkey) -> bool {
    CLINGO_TRY {
        if (stats == nullptr || (name == nullptr && size > 0) || subkey == nullptr) {
            return fail_arguments();
        }
        *subkey =
            cpp_cast(stats)->add(key, std::string_view{name, size}, cpp_cast(static_cast<clingo_stats_type_e>(type)));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_stats_value_get(clingo_stats_t const *stats, uint64_t key, double *value) -> bool {
    CLINGO_TRY {
        if (stats == nullptr || value == nullptr) {
            return fail_arguments();
        }
        *value = cpp_cast(stats)->value(key);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_stats_value_set(clingo_stats_t *stats, uint64_t key, double value) -> bool {
    CLINGO_TRY {
        if (stats == nullptr) {
            return fail_arguments();
        }
        cpp_cast(stats)->set(key, value);
    }
    CLINGO_CATCH;
}

namespace CppClingo::CAPI {
namespace {

class ToString {
  public:
    using Key = Potassco::AbstractStatistics::Key_t;
    using Type = Potassco::AbstractStatistics::Type;

    ToString(Potassco::AbstractStatistics const *stats, Util::OutputBuffer *buf) : stats_{stats}, buf_{buf} {}

    void str(Key key) const {
        buf_->reset();
        str_(key, 0, 0);
    }

  private:
    void str_(Key key, size_t first_indent, size_t indent) const {
        auto fi = [&, first = true]() mutable {
            return Util::fill(std::exchange(first, false) ? first_indent : indent);
        };
        switch (stats_->type(key)) {
            case Potassco::AbstractStatistics::Type::value: {
                *buf_ << fi() << stats_->value(key) << "\n";
                break;
            }
            case Potassco::AbstractStatistics::Type::map: {
                auto size = stats_->size(key);
                for (size_t i = 0; i < size; ++i) {
                    auto name = std::string_view{stats_->key(key, i)};
                    *buf_ << fi() << name << ":";
                    auto sub_key = stats_->get(key, name);
                    if (stats_->type(sub_key) == Potassco::AbstractStatistics::Type::value) {
                        *buf_ << " ";
                        str_(sub_key, 0, indent + name.size() + 2);
                    } else {
                        *buf_ << "\n";
                        str_(sub_key, indent + 2, indent + 2);
                    }
                }
                break;
            }
            case Potassco::AbstractStatistics::Type::array: {
                size_t size = stats_->size(key);
                if (size > 0) {
                    for (size_t i = 0; i != size; ++i) {
                        *buf_ << fi() << "- ";
                        auto sub_key = stats_->at(key, i);
                        str_(sub_key, 0, indent + 2);
                    }
                } else {
                    *buf_ << fi() << "[]\n";
                }
                break;
            }
        }
    }

    Potassco::AbstractStatistics const *stats_;
    Util::OutputBuffer *buf_;
};

} // namespace
} // namespace CppClingo::CAPI

extern "C" auto clingo_stats_to_string(clingo_stats_t const *stats, uint64_t key, clingo_string_builder_t *builder)
    -> bool {
    CLINGO_TRY {
        ToString{cpp_cast(stats), cpp_cast(builder)}.str(key);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_stats(clingo_control_t *control, clingo_stats_t const **stats) -> bool {
    CLINGO_TRY {
        if (control == nullptr || stats == nullptr) {
            return fail_arguments();
        }
        *stats = c_cast(&control->slv->clasp_stats());
    }
    CLINGO_CATCH;
}
