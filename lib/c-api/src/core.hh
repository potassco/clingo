#pragma once

#include <clingo/core.h>

#include <clingo/util/print.hh>

inline static auto c_cast(CppClingo::Util::OutputBuffer *buf) -> clingo_string_builder_t * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_string_builder_t *>(buf);
}

inline static auto cpp_cast(CppClingo::Util::OutputBuffer *buf) -> std::ostringstream * {
    // NOLINTNEXTLINE
    return reinterpret_cast<std::ostringstream *>(buf);
}

inline static auto cpp_cast(clingo_string_builder_t const *buf) -> CppClingo::Util::OutputBuffer const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<CppClingo::Util::OutputBuffer const *>(buf);
}

inline static auto cpp_cast(clingo_string_builder_t *buf) -> CppClingo::Util::OutputBuffer * {
    // NOLINTNEXTLINE
    return reinterpret_cast<CppClingo::Util::OutputBuffer *>(buf);
}
