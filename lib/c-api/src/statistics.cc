#include "clingo/statistics.h"

#include "control.hh" // IWYU pragma: keep
#include "lib.hh"

extern "C" auto clingo_statistics_root(clingo_statistics_t const *statistics, uint64_t *key) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(statistics);
        *key = 0;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_type(clingo_statistics_t const *statistics, uint64_t key,
                                       clingo_statistics_type_t *type) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(statistics);
        static_cast<void>(key);
        *type = 0;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_array_size(clingo_statistics_t const *statistics, uint64_t key, size_t *size)
    -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(statistics);
        static_cast<void>(key);
        *size = 0;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_array_at(clingo_statistics_t const *statistics, uint64_t key, size_t offset,
                                           uint64_t *subkey) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(statistics);
        static_cast<void>(key);
        static_cast<void>(offset);
        *subkey = 0;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_array_push(clingo_statistics_t *statistics, uint64_t key,
                                             clingo_statistics_type_t type, uint64_t *subkey) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(statistics);
        static_cast<void>(key);
        static_cast<void>(type);
        *subkey = 0;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_map_size(clingo_statistics_t const *statistics, uint64_t key, size_t *size)
    -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(statistics);
        static_cast<void>(key);
        *size = 0;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_map_has_subkey(clingo_statistics_t const *statistics, uint64_t key, char const *name,
                                                 bool *result) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(statistics);
        static_cast<void>(key);
        static_cast<void>(name);
        *result = false;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_map_subkey_name(clingo_statistics_t const *statistics, uint64_t key, size_t offset,
                                                  char const **name) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(statistics);
        static_cast<void>(key);
        static_cast<void>(offset);
        *name = "";
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_map_at(clingo_statistics_t const *statistics, uint64_t key, char const *name,
                                         uint64_t *subkey) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(statistics);
        static_cast<void>(key);
        static_cast<void>(name);
        *subkey = 0;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_map_add_subkey(clingo_statistics_t *statistics, uint64_t key, char const *name,
                                                 clingo_statistics_type_t type, uint64_t *subkey) -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(statistics);
        static_cast<void>(key);
        static_cast<void>(name);
        static_cast<void>(type);
        *subkey = 0;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_value_get(clingo_statistics_t const *statistics, uint64_t key, double *value)
    -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(statistics);
        static_cast<void>(key);
        *value = 0;
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_statistics_value_set(clingo_statistics_t *statistics, uint64_t key, double value)
    -> clingo_result_t {
    CLINGO_TRY {
        static_cast<void>(statistics);
        static_cast<void>(key);
        static_cast<void>(value);
    }
    CLINGO_CATCH;
}
