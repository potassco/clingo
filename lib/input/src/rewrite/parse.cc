#include <gringo/input/parser.hh>
#include <gringo/input/rewrite.hh>

#include <gringo/input/rewrite/check_syntax.hh>
#include <gringo/input/rewrite/parse.hh>

#include <fstream>
#include <utility>

namespace Gringo::Input {

class ScannerImpl {
  public:
    virtual ~ScannerImpl() noexcept = default;
    virtual auto scan() -> std::optional<Stm> = 0;
    [[nodiscard]] virtual auto has_error() const -> bool = 0;
    virtual auto logger() -> Logger & = 0;
};

Scanner::Scanner(std::unique_ptr<ScannerImpl> impl) : impl_{std::move(impl)} {}

Scanner::~Scanner() noexcept = default;
Scanner::Scanner(Scanner &&other) noexcept = default;
auto Scanner::operator=(Scanner &&other) noexcept -> Scanner & = default;

auto Scanner::scan() -> std::optional<Stm> { return impl_->scan(); }

[[nodiscard]] auto Scanner::has_error() const -> bool { return impl_->has_error(); }

class FileParser : public ScannerImpl {
  public:
    FileParser(Logger &log, SymbolStore &store, char const *path) : file_{path}, log_{&log}, parser_{log, store} {
        parser_.init(file_, store.string_ref(path));
    }

    auto scan() -> std::optional<Stm> override {
        auto [stm, res] = parser_.scan();
        if (!res) {
            has_error_ = true;
        }
        return stm;
    }

    [[nodiscard]] auto has_error() const -> bool override { return has_error_; }
    auto logger() -> Logger & override { return *log_; }

  private:
    std::ifstream file_;
    Logger *log_;
    Parser parser_;
    bool has_error_ = false;
};

class StreamParser : public ScannerImpl {
  public:
    StreamParser(Logger &log, SymbolStore &store, std::istream &in) : in_{&in}, log_{&log}, parser_{log, store} {
        parser_.init(*in_, store.string_ref("<stream>"));
    }

    auto scan() -> std::optional<Stm> override {
        auto [stm, res] = parser_.scan();
        if (!res) {
            has_error_ = true;
        }
        return stm;
    }

    [[nodiscard]] auto has_error() const -> bool override { return has_error_; }
    auto logger() -> Logger & override { return *log_; }

  private:
    std::istream *in_;
    Logger *log_;
    Parser parser_;
    bool has_error_ = false;
};

class StringParser : public ScannerImpl {
  public:
    StringParser(Logger &log, SymbolStore &store, std::string_view str) : log_{&log}, parser_{log, store} {
        parser_.init(str, store.string_ref("<string>"));
    }

    auto scan() -> std::optional<Stm> override {
        auto [stm, res] = parser_.scan();
        if (!res) {
            has_error_ = true;
        }
        return stm;
    }

    [[nodiscard]] auto has_error() const -> bool override { return has_error_; }
    auto logger() -> Logger & override { return *log_; }

  private:
    Logger *log_;
    Parser parser_;
    bool has_error_ = false;
};

auto scan_stream(Logger &log, SymbolStore &store, std::istream &in) -> Scanner {
    return Scanner{std::make_unique<StreamParser>(log, store, in)};
}

auto scan_file(Logger &log, SymbolStore &store, char const *path) -> Scanner {
    return Scanner{std::make_unique<FileParser>(log, store, path)};
}

auto scan_string(Logger &log, SymbolStore &store, std::string_view content) -> Scanner {
    return Scanner{std::make_unique<StringParser>(log, store, content)};
}

auto parse_term(Logger &log, SymbolStore &store, std::string_view str) -> std::optional<Term> {
    auto p = Parser{log, store};
    p.init(str, *store.string(str));
    return p.parse_term();
}

auto parse_theory_term(Logger &log, SymbolStore &store, std::string_view str) -> std::optional<TheoryTerm> {
    auto p = Parser{log, store};
    p.init(str, *store.string(str));
    return p.parse_theory_term();
}

auto parse_literal(Logger &log, SymbolStore &store, std::string_view str) -> std::optional<Lit> {
    auto p = Parser{log, store};
    p.init(str, *store.string(str));
    return p.parse_literal();
}

auto parse_head_literal(Logger &log, SymbolStore &store, std::string_view str) -> std::optional<HdLit> {
    auto p = Parser{log, store};
    p.init(str, *store.string(str));
    return p.parse_head_literal();
}

auto parse_body_literal(Logger &log, SymbolStore &store, std::string_view str) -> std::optional<BdLit> {
    auto p = Parser{log, store};
    p.init(str, *store.string(str));
    return p.parse_body_literal();
}

auto parse_statement(Logger &log, SymbolStore &store, std::string_view str) -> std::optional<Stm> {
    auto p = Parser{log, store};
    p.init(str, *store.string(str));
    return p.parse_statement();
}

auto parse_parts(Logger &log, SymbolStore &store, std::string_view str) -> std::vector<Input::ProgramParamVec> {
    auto p = Parser{log, store};
    p.init(str, *store.string(str));
    if (auto def = p.parse_program_parts()) {
        return *std::move(def);
    }
    throw parse_error{};
}

auto parse_const(Logger &log, SymbolStore &store, std::string_view str) -> std::pair<SharedString, SharedSymbol> {
    auto p = Parser{log, store};
    p.init(str, *store.string(str));
    if (auto def = p.parse_const_def()) {
        return *std::move(def);
    }
    throw parse_error{};
}

} // namespace Gringo::Input
