#pragma once

#if __clang__
#define CLINGO_IGNORE_UNUSED_FUNCTION_B
#define CLINGO_IGNORE_UNUSED_FUNCTION_E
#define CLINGO_IGNORE_ZERO_SIZED_ARRAY_B                                                                               \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wzero-length-array\"")
#define CLINGO_IGNORE_ZERO_SIZED_ARRAY_E _Pragma("clang diagnostic pop")
#define CLINGO_IGNORE_PAR_EQ_B                                                                                         \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wparentheses-equality\"")
#define CLINGO_IGNORE_PAR_EQ_E _Pragma("clang diagnostic pop")
#define CLINGO_IGNORE_NON_TEMPLATE_FRIEND_B
#define CLINGO_IGNORE_NON_TEMPLATE_FRIEND_E
#define CLINGO_IGNORE_UNION_B                                                                                          \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wgnu-anonymous-struct\"")                    \
        _Pragma("clang diagnostic ignored \"-Wnested-anon-types\"")
#define CLINGO_IGNORE_UNION_E _Pragma("clang diagnostic pop")
#elif __GNUC__
#define CLINGO_IGNORE_UNUSED_FUNCTION_B                                                                                \
    _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wunused-function\"")
#define CLINGO_IGNORE_UNUSED_FUNCTION_E _Pragma("GCC diagnostic pop")
#define CLINGO_IGNORE_ZERO_SIZED_ARRAY_B _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wpedantic\"")
#define CLINGO_IGNORE_ZERO_SIZED_ARRAY_E _Pragma("GCC diagnostic pop")
#define CLINGO_IGNORE_PAR_EQ_B
#define CLINGO_IGNORE_PAR_EQ_E
#define CLINGO_IGNORE_NON_TEMPLATE_FRIEND_B                                                                            \
    _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wnon-template-friend\"")
#define CLINGO_IGNORE_NON_TEMPLATE_FRIEND_E _Pragma("GCC diagnostic pop")
#define CLINGO_IGNORE_UNION_B _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wpedantic\"")
#define CLINGO_IGNORE_UNION_E _Pragma("GCC diagnostic pop")
#else
#define CLINGO_IGNORE_UNUSED_FUNCTION_B
#define CLINGO_IGNORE_UNUSED_FUNCTION_E
#define CLINGO_IGNORE_ZERO_SIZED_ARRAY_B
#define CLINGO_IGNORE_ZERO_SIZED_ARRAY_E
#define CLINGO_IGNORE_PAR_EQ_B
#define CLINGO_IGNORE_PAR_EQ_E
#define CLINGO_IGNORE_NON_TEMPLATE_FRIEND_B
#define CLINGO_IGNORE_NON_TEMPLATE_FRIEND_E
#define CLINGO_IGNORE_UNION_B
#define CLINGO_IGNORE_UNION_E
#endif
