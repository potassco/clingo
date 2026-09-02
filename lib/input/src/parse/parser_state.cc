#include "parser_state.hh"

// The re2c-generated lexer reads a `char const*` buffer as `unsigned char`
// (YYCTYPE), which trips the strict conversion gate. Exempt only the generated
// include; hand-written code in this TU stays under the gate.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include "parse/lexer_impl.hh"
#pragma GCC diagnostic pop
