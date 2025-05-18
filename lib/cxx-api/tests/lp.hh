#pragma once

#include <cctype>
#include <stdexcept>
#include <string>
#include <vector>

struct LPData {
    std::vector<std::string> options;
    std::vector<std::vector<std::string>> solutions;
};

class LPParser {
  public:
    LPParser(std::string_view in) : input_(in) {}

    auto parse() -> LPData {
        auto data = LPData{};
        skip_outer();
        skip_inner();
        expect('{');
        while (true) {
            auto key = parse_string();
            skip_inner();
            expect(':');
            if (key == "options") {
                data.options = parse_options();
            } else if (key == "solutions") {
                data.solutions = parse_solutions();
            } else {
                throw std::runtime_error("Unknown key: " + key);
            }
            skip_inner();
            if (branch('}')) {
                break;
            }
            expect(',');
        }
        skip_inner();
        if (auto c = peek(); c != '\0') {
            report(std::string{"Unexpected character '"} + c + "'");
        }
        return data;
    }

  private:
    void report(std::string msg) const {
        throw std::runtime_error(std::move(msg) + " at position " + std::to_string(pos_));
    }

    [[nodiscard]] auto peek() const -> char { return pos_ < input_.size() ? input_[pos_] : '\0'; }

    auto consume() -> char {
        auto c = peek();
        if (c == '\0') {
            report(std::string{"Unexpected end of input"});
        }
        ++pos_;
        return c;
    }

    template <class... Cs> auto branch(Cs... cs) -> bool {
        auto c = peek();
        if ((std::equal_to{}(c, cs) || ...)) {
            consume();
            return true;
        }
        return false;
    }

    void expect(char c) {
        if (consume() != c) {
            report(std::string{"Expected '"} + c + "'");
        }
    }

    void skip_outer() {
        while (true) {
            if (branch('%') && branch('%')) {
                return;
            }
            if (peek() == '\0') {
                return;
            }
            consume();
        }
    }

    void skip_inner() {
        while (true) {
            while (branch(' ', '\t', '\r')) {
            }
            if (peek() != '\n') {
                break;
            }
            skip_outer();
        }
    }

    auto parse_string() -> std::string {
        skip_inner();
        expect('\"');
        auto result = std::string{};
        while (true) {
            auto c = consume();
            if (c == '\\') {
                c = consume();
                if (c == '\"') {
                    result += '\"';
                } else if (c == '\\') {
                    result += '\\';
                } else {
                    report(std::string{"Unexpected escape '\\"} + c + "'");
                }
            } else if (c == '\"') {
                break;
            } else {
                result += c;
            }
        }
        return result;
    }

    auto parse_options() -> std::vector<std::string> {
        auto arr = std::vector<std::string>{};
        skip_inner();
        expect('[');
        skip_inner();
        if (branch(']')) {
            return arr;
        }
        while (true) {
            arr.emplace_back(parse_string());
            skip_inner();
            if (branch(']')) {
                break;
            }
            expect(',');
        }
        return arr;
    }

    auto parse_solutions() -> std::vector<std::vector<std::string>> {
        std::vector<std::vector<std::string>> arr;
        skip_inner();
        expect('[');
        skip_inner();
        if (branch(']')) {
            return arr;
        }
        while (true) {
            arr.push_back(parse_options());
            skip_inner();
            if (branch(']')) {
                break;
            }
            expect(',');
        }
        return arr;
    }

    std::string_view input_;
    size_t pos_ = 0;
};
