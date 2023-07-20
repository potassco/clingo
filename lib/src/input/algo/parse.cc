#include <iostream>

#include <lexy/action/parse.hpp>
#include <lexy/action/scan.hpp>
#include <lexy/input/file.hpp>
#include <lexy/input/string_input.hpp>

#include <input/algo/parse.hh>
#include <input/algo/print.hh>
#include <input/algo/rewrite.hh>

#include "parse/statement.hh"

namespace Gringo::Input {

namespace {

template <class P> struct root : Grammar::control {
    static constexpr auto rule = lexy::dsl::p<P> + lexy::dsl::eof;
    static constexpr auto value = lexy::forward<typename decltype(P::value)::return_type>;
};

template <typename Control>
auto parse(std::string_view str) -> std::optional<typename decltype(Control::value)::return_type> {
    auto input = lexy::string_input<Grammar::encoding>{str};
    auto res = lexy::parse<root<Control>>(input, report_error);
    if (res.has_value()) {
        return std::move(res).value();
    }
    return std::nullopt;
}

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

template <typename Input, typename Scanner> void discard(Input &input, Scanner &scanner) {
    static_cast<void>(input);
    static_cast<void>(scanner);
}

template <typename Encoding, typename Counting, typename Scanner>
void discard(StreamInput<Encoding, Counting> &input, Scanner &scanner) {
    input.discard_before(scanner.position());
}

} // namespace

class ScannerImpl {
  public:
    virtual ~ScannerImpl() noexcept = default;
    virtual auto scan() -> std::optional<Statement> = 0;

    static auto scan_(auto &self) -> std::optional<Statement> {
        // skip leading whitespace
        if (self.init_) {
            self.scanner_.parse(lexy::dsl::whitespace(Grammar::control::whitespace));
            self.init_ = false;
        }
        // report comments
        if (!self.comments_.empty()) {
            return Comment{CommentType::line, self.comments_.pop()};
        }
        // report last statement
        if (self.res_.has_value()) {
            auto res = std::move(self.res_).value();
            self.res_ = std::nullopt;
            return res;
        }
        // scan the next statement
        while (self.scanner_ && !self.scanner_.is_at_eof()) {
            discard(self.input_, self.scanner_);
            auto res = self.scanner_.template parse<Grammar::statement>();
            if (!self.scanner_) {
                recover(self.scanner_);
            }
            if (res.has_value()) {
                // delay reporting statement reporting comments first
                if (!self.comments_.empty()) {
                    self.res_ = std::move(res).value();
                    return Comment{CommentType::line, self.comments_.pop()};
                }
                // report statement
                return std::move(res).value();
            }
        }
        // ensure all comments are reported
        self.comments_.mark();
        if (!self.comments_.empty()) {
            return Comment{CommentType::line, self.comments_.pop()};
        }
        return std::nullopt;
    }
};

Scanner::Scanner(std::unique_ptr<ScannerImpl> impl) : impl_{std::move(impl)} {}

Scanner::~Scanner() noexcept = default;

auto Scanner::scan() -> std::optional<Statement> { return impl_->scan(); }

class StreamScanner : public ScannerImpl {
  public:
    StreamScanner(std::istream &in)
        : base_input_{in}, input_{base_input_, comments_},
          scanner_{lexy::scan<Grammar::control>(input_, report_error)} {}
    auto scan() -> std::optional<Statement> override { return scan_(*this); }

  private:
    friend ScannerImpl;

    using BaseInput = StreamInput<Grammar::encoding>;
    using Input = StatefulInput<BaseInput, Comments>;
    using Scanner = decltype(lexy::scan<Grammar::control>(std::declval<Input>(), report_error));

    Comments comments_;
    std::optional<Statement> res_;
    BaseInput base_input_;
    Input input_;
    Scanner scanner_;
    bool init_ = true;
};

class FileScanner : public ScannerImpl {
  public:
    FileScanner(char const *path)
        : handle_{lexy::read_file<Grammar::encoding>(path)}, base_input_{handle_.buffer()},
          input_{base_input_, comments_}, scanner_{lexy::scan<Grammar::control>(input_, report_error)} {}
    auto scan() -> std::optional<Statement> override { return scan_(*this); }

  private:
    friend ScannerImpl;

    using FileHandle = std::remove_cvref_t<decltype(lexy::read_file<Grammar::encoding>(std::declval<char const *>()))>;
    using BaseInput = std::remove_cvref_t<decltype(std::declval<FileHandle>().buffer())>;
    using Input = StatefulInput<BaseInput, Comments>;
    using Scanner = std::remove_cvref_t<decltype(lexy::scan<Grammar::control>(std::declval<Input>(), report_error))>;

    Comments comments_;
    std::optional<Statement> res_;
    FileHandle handle_;
    BaseInput base_input_;
    Input input_;
    Scanner scanner_;
    bool init_ = true;
};

class StringScanner : public ScannerImpl {
  public:
    StringScanner(std::string_view content)
        : base_input_{content}, input_{base_input_, comments_},
          scanner_{lexy::scan<Grammar::control>(input_, report_error)} {}
    auto scan() -> std::optional<Statement> override { return scan_(*this); }

  private:
    friend ScannerImpl;

    using BaseInput = lexy::string_input<Grammar::encoding>;
    using Input = StatefulInput<BaseInput, Comments>;
    using Scanner = decltype(lexy::scan<Grammar::control>(std::declval<Input>(), report_error));

    Comments comments_;
    std::optional<Statement> res_;
    BaseInput base_input_;
    Input input_;
    Scanner scanner_;
    bool init_ = true;
};

auto scan_stream(std::istream &in) -> Scanner { return Scanner{std::make_unique<StreamScanner>(in)}; }

auto scan_file(char const *path) -> Scanner { return Scanner{std::make_unique<FileScanner>(path)}; }

auto scan_string(std::string_view content) -> Scanner { return Scanner{std::make_unique<StringScanner>(content)}; }

auto parse_term(std::string_view str) -> std::optional<Term> { return parse<Grammar::term>(str); }

auto parse_literal(std::string_view str) -> std::optional<Literal> { return parse<Grammar::literal>(str); }

auto parse_head_literal(std::string_view str) -> std::optional<HeadLiteral> {
    return parse<Grammar::head_literal>(str);
}

auto parse_body_literal(std::string_view str) -> std::optional<BodyLiteral> {
    return parse<Grammar::body_literal>(str);
}

auto parse_statement(std::string_view str) -> std::optional<Statement> { return parse<Grammar::statement>(str); }

} // namespace Gringo::Input
