#include "clingo/statistics.h"

#include "control.hh" // IWYU pragma: keep
#include "lib.hh"

inline auto cpp_cast(clingo_statistics_t const *config) -> Potassco::AbstractStatistics const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Potassco::AbstractStatistics const *>(config);
}

inline auto cpp_cast(clingo_statistics_t *config) -> Potassco::AbstractStatistics * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Potassco::AbstractStatistics *>(config);
}

inline auto c_cast(Potassco::AbstractStatistics *config) -> clingo_statistics_t * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_statistics_t *>(config);
}

inline auto c_cast(Potassco::AbstractStatistics const *config) -> clingo_statistics_t const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_statistics_t const *>(config);
}

inline auto cpp_cast(clingo_statistics_type_e type) -> Potassco::StatisticsType {
    switch (type) {
        case clingo_statistics_type_empty: {
            return Potassco::StatisticsType::empty;
        }
        case clingo_statistics_type_value: {
            return Potassco::StatisticsType::value;
        }
        case clingo_statistics_type_map: {
            return Potassco::StatisticsType::map;
        }
        case clingo_statistics_type_array: {
            return Potassco::StatisticsType::array;
        }
    }
    throw std::invalid_argument("unexpected enum value");
}

inline auto c_cast(Potassco::StatisticsType type) -> clingo_statistics_type_e {
    switch (type) {
        case Potassco::StatisticsType::empty: {
            return clingo_statistics_type_empty;
        }
        case Potassco::StatisticsType::value: {
            return clingo_statistics_type_value;
        }
        case Potassco::StatisticsType::map: {
            return clingo_statistics_type_map;
        }
        case Potassco::StatisticsType::array: {
            return clingo_statistics_type_array;
        }
    }
    throw std::invalid_argument("unexpected enum value");
}

extern "C" auto clingo_statistics_root(clingo_statistics_t const *statistics, uint64_t *key) -> clingo_result_t {
    CLINGO_TRY {
        if (statistics == nullptr || key == nullptr) {
            return clingo_result_invalid;
        }
        *key = cpp_cast(statistics)->root();
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_type(clingo_statistics_t const *statistics, uint64_t key,
                                       clingo_statistics_type_t *type) -> clingo_result_t {
    CLINGO_TRY {
        if (statistics == nullptr || type == nullptr) {
            return clingo_result_invalid;
        }
        switch (cpp_cast(statistics)->type(key)) {
            case Potassco::StatisticsType::empty: {
                *type = clingo_statistics_type_empty;
                break;
            }
            case Potassco::StatisticsType::value: {
                *type = clingo_statistics_type_value;
                break;
            }
            case Potassco::StatisticsType::map: {
                *type = clingo_statistics_type_map;
                break;
            }
            case Potassco::StatisticsType::array: {
                *type = clingo_statistics_type_array;
                break;
            }
        }
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_array_size(clingo_statistics_t const *statistics, uint64_t key, size_t *size)
    -> clingo_result_t {
    CLINGO_TRY {
        if (statistics == nullptr || size == nullptr) {
            return clingo_result_invalid;
        }
        *size = cpp_cast(statistics)->size(key);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_array_at(clingo_statistics_t const *statistics, uint64_t key, size_t offset,
                                           uint64_t *subkey) -> clingo_result_t {
    CLINGO_TRY {
        if (statistics == nullptr || subkey == nullptr) {
            return clingo_result_invalid;
        }
        *subkey = cpp_cast(statistics)->at(key, offset);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_array_push(clingo_statistics_t *statistics, uint64_t key,
                                             clingo_statistics_type_t type, uint64_t *subkey) -> clingo_result_t {
    CLINGO_TRY {
        if (statistics == nullptr || subkey == nullptr) {
            return clingo_result_invalid;
        }
        *subkey = cpp_cast(statistics)->push(key, cpp_cast(static_cast<clingo_statistics_type_e>(type)));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_map_size(clingo_statistics_t const *statistics, uint64_t key, size_t *size)
    -> clingo_result_t {
    CLINGO_TRY {
        if (statistics == nullptr || size == nullptr) {
            return clingo_result_invalid;
        }
        *size = cpp_cast(statistics)->size(key);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_map_has_subkey(clingo_statistics_t const *statistics, uint64_t key, char const *name,
                                                 bool *result) -> clingo_result_t {
    CLINGO_TRY {
        if (statistics == nullptr || name == nullptr || result == nullptr) {
            return clingo_result_invalid;
        }
        *result = cpp_cast(statistics)->find(key, name, nullptr);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_map_subkey_name(clingo_statistics_t const *statistics, uint64_t key, size_t offset,
                                                  char const **name) -> clingo_result_t {
    CLINGO_TRY {
        if (statistics == nullptr || name == nullptr) {
            return clingo_result_invalid;
        }
        *name = cpp_cast(statistics)->key(key, offset);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_map_at(clingo_statistics_t const *statistics, uint64_t key, char const *name,
                                         uint64_t *subkey) -> clingo_result_t {
    CLINGO_TRY {
        if (statistics == nullptr || name == nullptr || subkey == nullptr) {
            return clingo_result_invalid;
        }
        *subkey = cpp_cast(statistics)->get(key, name);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_map_add_subkey(clingo_statistics_t *statistics, uint64_t key, char const *name,
                                                 clingo_statistics_type_t type, uint64_t *subkey) -> clingo_result_t {
    CLINGO_TRY {
        if (statistics == nullptr || name == nullptr || subkey == nullptr) {
            return clingo_result_invalid;
        }
        *subkey = cpp_cast(statistics)->add(key, name, cpp_cast(static_cast<clingo_statistics_type_e>(type)));
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_value_get(clingo_statistics_t const *statistics, uint64_t key, double *value)
    -> clingo_result_t {
    CLINGO_TRY {
        if (statistics == nullptr || value == nullptr) {
            return clingo_result_invalid;
        }
        *value = cpp_cast(statistics)->value(key);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_value_set(clingo_statistics_t *statistics, uint64_t key, double value)
    -> clingo_result_t {
    CLINGO_TRY {
        if (statistics == nullptr) {
            return clingo_result_invalid;
        }
        cpp_cast(statistics)->set(key, value);
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_control_statistics(clingo_control_t *control, clingo_statistics_t const **statistics)
    -> clingo_result_t {
    CLINGO_TRY {
        if (control == nullptr || statistics == nullptr) {
            return clingo_result_invalid;
        }
        *statistics = c_cast(&control->slv->clasp_statistics());
    }
    CLINGO_CATCH;
}
