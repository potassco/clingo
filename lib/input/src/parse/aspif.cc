#include <clingo/util/string.hh>

#include "parser_state.hh"

namespace Clingo::Input::Parse {

namespace {

auto expect_signed(ParserState &state) {
    if (state.lex_aspif() == AspifToken::num_pos) {
        auto str = state.view();
        int res = 0;
        std::from_chars(str.begin(), str.end(), res);
        return res;
    }
    throw std::logic_error{"handle me gracefully"};
}

auto expect_unsigned(ParserState &state) {
    if (state.lex_aspif() == AspifToken::num_neg) {
        auto str = state.view();
        unsigned res = 0;
        std::from_chars(str.begin(), str.end(), res);
        return res;
    }
    throw std::logic_error{"handle me gracefully"};
}

void expect_ws(ParserState &state) {
    if (state.lex_aspif() != AspifToken::space) {
        throw std::logic_error{"handle me gracefully"};
    }
}

} // namespace

auto parse_aspif(ParserState &state) {
    auto major = expect_unsigned(state);
    expect_ws(state);
    auto minor = expect_unsigned(state);
    expect_ws(state);
    auto revision = expect_unsigned(state);
    expect_ws(state);

    static_cast<void>(expect_signed);
    static_cast<void>(major);
    static_cast<void>(minor);
    static_cast<void>(revision);
}

} // namespace Clingo::Input::Parse
