#pragma once

#include <clingo/core.h>

#include <clingo/util/print.hh>

inline static auto c_cast(Clingo::Util::OutputBuffer *buf) -> clingo_string_builder_t * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_string_builder_t *>(buf);
}

inline static auto cpp_cast(Clingo::Util::OutputBuffer *buf) -> std::ostringstream * {
    // NOLINTNEXTLINE
    return reinterpret_cast<std::ostringstream *>(buf);
}

inline static auto cpp_cast(clingo_string_builder_t const *buf) -> Clingo::Util::OutputBuffer const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Util::OutputBuffer const *>(buf);
}

inline static auto cpp_cast(clingo_string_builder_t *buf) -> Clingo::Util::OutputBuffer * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Util::OutputBuffer *>(buf);
}
