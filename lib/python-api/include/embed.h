#ifndef CLINGO_EMBED_H
#define CLINGO_EMBED_H

#include <clingo/core.h>

#ifdef __cplusplus
extern "C" {
#endif

// NOLINTBEGIN(modernize-*,cppcoreguidelines-macro-usage,performance-enum-size)

bool clingo_register_python(clingo_lib_t *lib);

// NOLINTEND(modernize-*,cppcoreguidelines-macro-usage,performance-enum-size)

#ifdef __cplusplus
}
#endif

#endif
