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

void parse(RewriteOptions opts, auto &&input, auto &&output) {
    Comments comments;
    auto stateful_input = StatefulInput{input, comments};
    auto scanner = lexy::scan<Grammar::control>(stateful_input, Gringo::Util::report_error);
    // skip leading whitespace
    scanner.parse(lexy::dsl::whitespace(Grammar::control::whitespace));
    while (scanner && !scanner.is_at_eof()) {
        discard(input, scanner);
        lexy::scan_result<Statement> res_stm = scanner.template parse<Grammar::statement>();
        if (res_stm.has_value()) {
            // output comments before end of statement
            for (auto &comment : comments) {
                output << comment << "\n";
            }
            comments.clear();
            // rewrite statements
            StatementVec stms;
            rewrite(std::move(res_stm.value()), opts, stms);
            for (auto const &stm : stms) {
                output << stm << "\n";
            }
        }
        if (!scanner) {
            recover(scanner);
        }
    }
    // print remaining comments
    comments.mark();
    for (auto &comment : comments) {
        output << comment << "\n";
    }
    comments.clear();
};

class ScannerImpl {
  public:
    virtual ~ScannerImpl() noexcept = default;
    virtual auto scan() -> std::optional<Statement> = 0;
};

Scanner::Scanner(std::unique_ptr<ScannerImpl> impl) : impl_{std::move(impl)} {}

Scanner::~Scanner() noexcept = default;

auto Scanner::scan() -> std::optional<Statement> { return impl_->scan(); }

class StreamScanner : public ScannerImpl {
  public:
    StreamScanner(std::istream &in)
        : base_input_{in}, input_{base_input_, comments_},
          scanner_{lexy::scan<Grammar::control>(input_, Gringo::Util::report_error)} {}
    auto scan() -> std::optional<Statement> override {
        // TODO: put the comments into the statement
        // and report them here
        return std::nullopt;
    }

  private:
    using BaseInput = Util::StreamInput<Grammar::encoding>;
    using Input = StatefulInput<Util::StreamInput<Grammar::encoding>, Comments>;
    using Scanner = decltype(lexy::scan<Grammar::control>(std::declval<Input>(), Gringo::Util::report_error));

    Comments comments_;
    BaseInput base_input_;
    Input input_;
    Scanner scanner_;
};

auto parse_stream(std::istream &in) -> Scanner { return Scanner{std::make_unique<StreamScanner>(in)}; }

auto parse_file(char const *path) -> Scanner {
    // auto file = lexy::read_file<Grammar::encoding>(path);
    // parse(opts, file.buffer(), std::cout);
    static_cast<void>(path);
    throw std::runtime_error("implement me!!!");
}

auto parse_string(std::string content) -> Scanner {
    // auto input = lexy::string_input<Grammar::encoding>(content);
    // parse(opts, input, std::cout);
    static_cast<void>(content);
    throw std::runtime_error("implement me!!!");
}

} // namespace Gringo::Input
