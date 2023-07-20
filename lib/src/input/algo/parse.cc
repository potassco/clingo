#include <iostream>

#include <lexy/action/scan.hpp>
#include <lexy/input/file.hpp>
#include <lexy/input/string_input.hpp>

#include <input/algo/parse.hh>
#include <input/algo/print.hh>
#include <input/algo/rewrite.hh>

#include <input/parser/statement.hh>

namespace Gringo::Input {

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
void discard(Gringo::Util::StreamInput<Encoding, Counting> &input, Scanner &scanner) {
    input.discard_before(scanner.position());
}

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
          scanner_{lexy::scan<Grammar::control>(input_, Gringo::Util::report_error)} {}
    auto scan() -> std::optional<Statement> override { return scan_(*this); }

  private:
    friend ScannerImpl;

    using BaseInput = Util::StreamInput<Grammar::encoding>;
    using Input = StatefulInput<BaseInput, Comments>;
    using Scanner = decltype(lexy::scan<Grammar::control>(std::declval<Input>(), Gringo::Util::report_error));

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
          input_{base_input_, comments_}, scanner_{lexy::scan<Grammar::control>(input_, Gringo::Util::report_error)} {}
    auto scan() -> std::optional<Statement> override { return scan_(*this); }

  private:
    friend ScannerImpl;

    using FileHandle = std::remove_cvref_t<decltype(lexy::read_file<Grammar::encoding>(std::declval<char const *>()))>;
    using BaseInput = std::remove_cvref_t<decltype(std::declval<FileHandle>().buffer())>;
    using Input = StatefulInput<BaseInput, Comments>;
    using Scanner =
        std::remove_cvref_t<decltype(lexy::scan<Grammar::control>(std::declval<Input>(), Gringo::Util::report_error))>;

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
          scanner_{lexy::scan<Grammar::control>(input_, Gringo::Util::report_error)} {}
    auto scan() -> std::optional<Statement> override { return scan_(*this); }

  private:
    friend ScannerImpl;

    using BaseInput = decltype(lexy::string_input<Grammar::encoding>(std::declval<std::string>()));
    using Input = StatefulInput<BaseInput, Comments>;
    using Scanner = decltype(lexy::scan<Grammar::control>(std::declval<Input>(), Gringo::Util::report_error));

    Comments comments_;
    std::optional<Statement> res_;
    BaseInput base_input_;
    Input input_;
    Scanner scanner_;
    bool init_ = true;
};

auto parse_stream(std::istream &in) -> Scanner { return Scanner{std::make_unique<StreamScanner>(in)}; }

auto parse_file(char const *path) -> Scanner { return Scanner{std::make_unique<FileScanner>(path)}; }

auto parse_string(std::string_view content) -> Scanner { return Scanner{std::make_unique<StringScanner>(content)}; }

} // namespace Gringo::Input
