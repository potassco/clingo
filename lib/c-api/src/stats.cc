#include "clingo/stats.h"

#include "control.hh" // IWYU pragma: keep
#include "lib.hh"

inline auto cpp_cast(clingo_stats_t const *config) -> Potassco::AbstractStatistics const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Potassco::AbstractStatistics const *>(config);
}

inline auto cpp_cast(clingo_stats_t *config) -> Potassco::AbstractStatistics * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Potassco::AbstractStatistics *>(config);
}

inline auto c_cast(Potassco::AbstractStatistics *config) -> clingo_stats_t * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_stats_t *>(config);
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
                                            bool *result) -> bool {
    CLINGO_TRY {
        if (stats == nullptr || (name == nullptr && size > 0) || result == nullptr) {
            return fail_arguments();
        }
        *result = cpp_cast(stats)->find(key, std::string_view{name, size}, nullptr);
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

extern "C" auto clingo_control_stats(clingo_control_t *control, clingo_stats_t const **stats) -> bool {
    CLINGO_TRY {
        if (control == nullptr || stats == nullptr) {
            return fail_arguments();
        }
        *stats = c_cast(&control->slv->clasp_stats());
    }
    CLINGO_CATCH;
}
