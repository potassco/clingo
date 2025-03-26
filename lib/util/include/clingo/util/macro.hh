#pragma once

#if __clang__
#define GRINGO_IGNORE_UNUSED_FUNCTION_B
#define GRINGO_IGNORE_UNUSED_FUNCTION_E
#define GRINGO_IGNORE_ZERO_SIZED_ARRAY_B                                                                               \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wzero-length-array\"")
#define GRINGO_IGNORE_ZERO_SIZED_ARRAY_E _Pragma("clang diagnostic pop")
#define GRINGO_IGNORE_PAR_EQ_B                                                                                         \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wparentheses-equality\"")
#define GRINGO_IGNORE_PAR_EQ_E _Pragma("clang diagnostic pop")
#define GRINGO_IGNORE_NON_TEMPLATE_FRIEND_B
#define GRINGO_IGNORE_NON_TEMPLATE_FRIEND_E
#define GRINGO_IGNORE_UNION_B                                                                                          \
    _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wgnu-anonymous-struct\"")                    \
        _Pragma("clang diagnostic ignored \"-Wnested-anon-types\"")
#define GRINGO_IGNORE_UNION_E _Pragma("clang diagnostic pop")
#elif __GNUC__
#define GRINGO_IGNORE_UNUSED_FUNCTION_B                                                                                \
    _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wunused-function\"")
#define GRINGO_IGNORE_UNUSED_FUNCTION_E _Pragma("GCC diagnostic pop")
#define GRINGO_IGNORE_ZERO_SIZED_ARRAY_B _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wpedantic\"")
#define GRINGO_IGNORE_ZERO_SIZED_ARRAY_E _Pragma("GCC diagnostic pop")
#define GRINGO_IGNORE_PAR_EQ_B
#define GRINGO_IGNORE_PAR_EQ_E
#define GRINGO_IGNORE_NON_TEMPLATE_FRIEND_B                                                                            \
    _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wnon-template-friend\"")
#define GRINGO_IGNORE_NON_TEMPLATE_FRIEND_E _Pragma("GCC diagnostic pop")
#define GRINGO_IGNORE_UNION_B _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wpedantic\"")
#define GRINGO_IGNORE_UNION_E _Pragma("GCC diagnostic pop")
#else
#define GRINGO_IGNORE_UNUSED_FUNCTION_B
#define GRINGO_IGNORE_UNUSED_FUNCTION_E
#define GRINGO_IGNORE_ZERO_SIZED_ARRAY_B
#define GRINGO_IGNORE_ZERO_SIZED_ARRAY_E
#define GRINGO_IGNORE_PAR_EQ_B
#define GRINGO_IGNORE_PAR_EQ_E
#define GRINGO_IGNORE_NON_TEMPLATE_FRIEND_B
#define GRINGO_IGNORE_NON_TEMPLATE_FRIEND_E
#define GRINGO_IGNORE_UNION_B
#define GRINGO_IGNORE_UNION_E
#endif
