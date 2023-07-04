#ifndef __clang_analyzer__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#include <iostream>
#include <sstream>

#include <lexy/action/scan.hpp>
#include <lexy/input/file.hpp>
#include <lexy/input/string_input.hpp>

#include <parser/statement.hh>

template <typename Scanner> auto recover(Scanner &scanner) {
    auto recovery = scanner.error_recovery();
    while (!scanner.branch(lexy::dsl::period)) {
        if (!scanner.discard(lexy::dsl::code_point)) {
            LEXY_MOV(recovery).cancel();
            return;
        }
    }
    if (scanner.branch(LEXY_LIT("["))) {
        while (!scanner.branch(LEXY_LIT("]"))) {
            if (!scanner.discard(lexy::dsl::code_point)) {
                LEXY_MOV(recovery).cancel();
                return;
            }
        }
    }
    std::move(recovery).finish();
}

template <typename Input, typename Scanner> void discard(Input &input, Scanner &scanner) {}

template <typename Encoding, typename Counting, typename Scanner>
void discard(StreamInput<Encoding, Counting> &input, Scanner &scanner) {
    input.discard_before(scanner.position());
}

void parse(auto &&input, auto &&out) {
    // TODO: add options for fine grained control
    std::vector<std::string> comments;
    auto stateful_input = StatefulInput{input, comments};
    auto scanner = lexy::scan<grammar::control>(stateful_input, report_error);
    // Note: skip leading whitespace
    scanner.parse(lexy::dsl::whitespace(grammar::control::whitespace));
    while (scanner && !scanner.is_at_eof()) {
        discard(input, scanner);
        lexy::scan_result<SStatement> res_stm = scanner.template parse<grammar::statement>();
        if (res_stm.has_value()) {
            auto stm = res_stm.value()->rewrite_anonymous().value_or(res_stm.value());
            auto unpooled_stms = stm->unpool();
            if (!unpooled_stms.has_value()) {
                unpooled_stms = make_vec<SStatement>(stm);
            }
            for (auto &unpooled : unpooled_stms.value()) {
                auto projected = unpooled->project().value_or(unpooled);
                out << *projected << "\n";
            }
            // TODO: ensure proper order of comments
            for (auto &comment : comments) {
                out << comment << "\n";
            }
        }
        comments.clear();
        if (!scanner) {
            recover(scanner);
        }
    }
};

EMSCRIPTEN_KEEPALIVE
extern "C" auto run(char const *program) -> char const * {
    std::ostringstream out;
    parse(lexy::string_input<grammar::encoding>(program, strlen(program)), out);
    static std::string result = out.str();
    return result.c_str();
}
