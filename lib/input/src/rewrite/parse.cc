#include "parse/params.hh"
#include "parse/statement.hh"

#include <gringo/input/parser.hh>
#include <gringo/input/rewrite.hh>

#include <gringo/input/rewrite/check_syntax.hh>
#include <gringo/input/rewrite/parse.hh>

#include <lexy/action/parse.hpp>
#include <lexy/action/scan.hpp>
#include <lexy/input/file.hpp>
#include <lexy/input/string_input.hpp>

#include <algorithm>
#include <utility>

namespace Gringo::Input {

namespace {

//! Helper to track comments and locations.
template <class It> class State {
  public:
    //! The underlying iterator type.
    using iterator = It;

    //! Construct state with beginning of input.
    State(SymbolStore &store, SharedString filename, It begin)
        : store_{&store}, filename_{std::move(filename)}, cur_{begin}, ws_begin_{begin}, ws_end_{begin} {}

    //! Compute line and column offsets for the given position.
    auto pos(It pos) -> Position {
        if (positions_.empty()) {
            positions_.emplace_back(0, 1, 1);
        }

        if (pos <= cur_) {
            size_t n_pos = n_cur_ - std::distance(pos, cur_);
            auto it = std::upper_bound(positions_.begin(), positions_.end(), n_pos,
                                       [](auto a, auto b) { return a < std::get<0>(b); });
            assert(it != positions_.begin());
            --it;
            return Position{*filename_, std::get<1>(*it), n_pos - std::get<0>(*it) + std::get<2>(*it)};
        }

        auto input = lexy::range_input(cur_, pos);
        auto reader = input.reader();

        auto line = std::get<1>(positions_.back());
        auto col = n_cur_ - std::get<0>(positions_.back()) + std::get<2>(positions_.back());

        auto counting = lexy::code_point_location_counting{};
        while (reader.position() != pos) {
            assert(reader.peek() != Grammar::encoding::eof() && "invalid position");
            if (counting.try_match_newline(reader)) {
                // [cur_, end) covers the newline.
                auto end = reader.position();
                if (lexy::_detail::min_range_end(cur_, end, pos) != end) {
                    break;
                }

                // Advance to the next line.
                size_t dist = std::distance(cur_, end);
                ++line;
                col = 1;
                cur_ = end;
                n_cur_ += dist;
                positions_.emplace_back(n_cur_, line, col);
            } else {
                counting.match_column(reader);

                // [cur_, end) covers the column.
                auto end = reader.position();
                if (lexy::_detail::min_range_end(cur_, end, pos) != end) {
                    break;
                }

                // Advance to the next column.
                size_t dist = std::distance(cur_, end);
                ++col;
                cur_ = end;
                n_cur_ += dist;
                if (dist != 1) {
                    positions_.emplace_back(n_cur_, line, col);
                }
            }
        }

        return Position{*filename_, line, col};
    }

    //! Get the last position ignoring consumed white space.
    auto post_pos(It it) -> Position { return pos(it == ws_end_ ? ws_begin_ : it); }

    [[nodiscard]] auto pos(Position pos) const -> Position { return pos; }

    auto loc(auto begin, auto end) -> Location {
        auto b = pos(end);
        auto a = pos(begin);
        return Location{std::move(a), std::move(b)};
    }

    auto loc(auto rng) -> Location { return loc(rng.begin(), rng.end()); }

    //! Discard computed positions before the given position.
    void discard(It cur) {
        // For our grammar, we always have cur_ <= cur and no positions in the middle of code points are discarded.
        auto position = pos(cur);
        positions_.clear();
        positions_.emplace_back(n_cur_ + std::distance(cur_, cur), position.line(), position.column());
    }

    //! Add a comment.
    void push(Location loc, CommentType type, std::string_view comment) {
        comments_.emplace(std::move(loc), type, store_->string_ref(comment));
    }

    //! Mark all currently available comments for popping.
    void mark() { mark_ = comments_.size(); }

    //! Check if a comment is available.
    [[nodiscard]] auto empty() const -> bool { return mark_ == 0; }

    //! Pop the last comment.
    auto pop() -> StmComment {
        assert(mark_ > 0);
        auto ret = std::move(comments_.front());
        comments_.pop();
        --mark_;
        return ret;
    }

    //! Mark consecutive white space.
    auto end_token(It begin, It end) {
        if (ws_end_ < begin) {
            ws_begin_ = begin;
        }
        if (ws_end_ < end) {
            ws_end_ = end;
        }
    }

    auto num(Number const &num) -> Symbol { return store_->num_ref(num); }

    auto num(Number &&num) -> Symbol { return store_->num_ref(std::move(num)); }

    auto string(std::string_view str) -> String { return store_->string_ref(str); }

    auto string(char const *str) -> String { return store_->string_ref(str); }

  private:
    //! The store to construct symbols.
    SymbolStore *store_;
    //! The name of the file at hand.
    SharedString filename_;
    //! Positions have been computed up to and including this iterator.
    It cur_;
    //! Start of white space.
    It ws_begin_;
    //! End of white space.
    It ws_end_;
    //! The offset of the cur_ iterator.
    size_t n_cur_ = 0;
    //! Positions for which positions have been computed.
    //!
    //! The first index is an offset and the next two indices the corresponding line and column numbers.
    //! Elements are sorted by offsets and intermediate ones omitted
    //! if columns can be computed by simple subtraction from the offset of the previous element
    //! (that is, all code points in between are represented by a single byte).
    std::vector<std::tuple<size_t, size_t, size_t>> positions_;
    //! A queue of comments.
    std::queue<StmComment> comments_;
    //! A marker for comments that can be popped.
    size_t mark_ = 0;
};

template <class P> struct root : Grammar::control {
    static constexpr auto name = P::name;
    static constexpr auto rule = lexy::dsl::p<P> + lexy::dsl::eof;
    static constexpr auto value = lexy::forward<typename decltype(P::value)::return_type>;
};

template <typename Control>
auto parse(Logger &log, SymbolStore &store,
           std::string_view str) -> std::optional<typename decltype(Control::value)::return_type> {
    auto error = false;
    auto input = lexy::string_input<Grammar::encoding>{str};
    auto state = State{store, store.string("<string>"), input.reader().position()};
    auto res = lexy::parse<root<Control>>(input, state, report_error{log, error});
    if (!error && res.has_value()) {
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

template <typename Input, typename Scanner>
void discard([[maybe_unused]] Input &input, [[maybe_unused]] Scanner &scanner) {}

template <typename Encoding, typename Counting, typename Scanner>
void discard(StreamInput<Encoding, Counting> &input, Scanner &scanner) {
    input.discard_before(scanner.position());
}

template <class T, class F> auto check(Gringo::Logger &log, std::optional<T> expr, F &&fun) -> std::optional<T> {
    if (expr && std::invoke(std::forward<F>(fun), log, *expr)) {
        return expr;
    }
    return std::nullopt;
}

} // namespace

class ScannerImpl {
  public:
    virtual ~ScannerImpl() noexcept = default;
    virtual auto scan() -> std::optional<Stm> = 0;
    [[nodiscard]] virtual auto has_error() const -> bool = 0;
    virtual auto logger() -> Logger & = 0;

    static auto scan_(auto &self) -> std::optional<Stm> {
        // skip leading whitespace
        if (self.init_) {
            self.scanner_.parse(lexy::dsl::whitespace(Grammar::control::whitespace));
            self.init_ = false;
        }
        // report comments
        if (!self.state_.empty()) {
            return self.state_.pop();
        }
        // report last statement
        if (self.res_.has_value()) {
            auto res = std::move(self.res_).value();
            self.res_ = std::nullopt;
            return res;
        }
        // scan the next statement
        while (self.scanner_ && !self.scanner_.is_at_eof()) {
            self.state_.discard(self.scanner_.position());
            discard(self.base_input_, self.scanner_);
            auto res = self.scanner_.template parse<Grammar::statement>();
            if (!self.scanner_) {
                recover(self.scanner_);
            }
            if (res.has_value()) {
                // report comments before statement
                if (!self.state_.empty()) {
                    self.res_ = std::move(res).value();
                    return self.state_.pop();
                }
                // report statement
                return std::move(res).value();
            }
        }
        // ensure all comments are reported
        self.state_.mark();
        if (!self.state_.empty()) {
            return self.state_.pop();
        }
        return std::nullopt;
    }
};

Scanner::Scanner(std::unique_ptr<ScannerImpl> impl) : impl_{std::move(impl)} {}

Scanner::~Scanner() noexcept = default;
Scanner::Scanner(Scanner &&other) noexcept = default;
auto Scanner::operator=(Scanner &&other) noexcept -> Scanner & = default;

auto Scanner::scan() -> std::optional<Stm> { return check(impl_->logger(), impl_->scan(), check_statement); }

[[nodiscard]] auto Scanner::has_error() const -> bool { return impl_->has_error(); }

class StreamScanner : public ScannerImpl {
  public:
    StreamScanner(Logger &log, SymbolStore &store, std::istream &in)
        : log_{log}, base_input_{in}, state_{store, store.string("<stream>"), base_input_.reader().position()},
          input_{base_input_, state_},
          scanner_{lexy::scan<Grammar::control>(input_, state_, report_error{log, error_})} {}
    auto scan() -> std::optional<Stm> override { return scan_(*this); }
    [[nodiscard]] auto has_error() const -> bool override { return error_; }
    auto logger() -> Logger & override { return log_; }

  private:
    friend ScannerImpl;

    using BaseInput = StreamInput<Grammar::encoding>;
    using State = decltype(Gringo::Input::State{std::declval<SymbolStore &>(), std::declval<SharedString>(),
                                                std::declval<BaseInput &>().reader().position()});
    using Input = StatefulInput<BaseInput, State>;
    using Scanner = decltype(lexy::scan<Grammar::control>(std::declval<Input &>(), std::declval<State &>(),
                                                          std::declval<report_error>()));

    Logger &log_;
    std::optional<Stm> res_;
    BaseInput base_input_;
    State state_;
    Input input_;
    Scanner scanner_;
    bool error_ = false;
    bool init_ = true;
};

class FileScanner : public ScannerImpl {
  public:
    FileScanner(Logger &log, SymbolStore &store, char const *path)
        : log_{log}, handle_{lexy::read_file<Grammar::encoding>(path)}, base_input_{handle_.buffer()},
          state_{store, store.string(path), base_input_.reader().position()}, input_{base_input_, state_},
          scanner_{lexy::scan<Grammar::control>(input_, state_, report_error{log, error_})} {}
    auto scan() -> std::optional<Stm> override { return scan_(*this); }
    [[nodiscard]] auto has_error() const -> bool override { return error_; }
    auto logger() -> Logger & override { return log_; }

  private:
    friend ScannerImpl;

    using FileHandle = std::remove_cvref_t<decltype(lexy::read_file<Grammar::encoding>(std::declval<char const *>()))>;
    using BaseInput = std::remove_cvref_t<decltype(std::declval<FileHandle>().buffer())>;
    using State = decltype(Gringo::Input::State{std::declval<SymbolStore &>(), std::declval<SharedString>(),
                                                std::declval<BaseInput &>().reader().position()});
    using Input = StatefulInput<BaseInput, State>;
    using Scanner = std::remove_cvref_t<decltype(lexy::scan<Grammar::control>(
        std::declval<Input &>(), std::declval<State &>(), std::declval<report_error>()))>;

    Logger &log_;
    std::optional<Stm> res_;
    FileHandle handle_;
    BaseInput base_input_;
    State state_;
    Input input_;
    Scanner scanner_;
    bool error_ = false;
    bool init_ = true;
};

class StringScanner : public ScannerImpl {
  public:
    StringScanner(Logger &log, SymbolStore &store, std::string_view content)
        : log_{log}, base_input_{content}, state_{store, store.string("<string>"), base_input_.reader().position()},
          input_{base_input_, state_},
          scanner_{lexy::scan<Grammar::control>(input_, state_, report_error{log, error_})} {}
    auto scan() -> std::optional<Stm> override { return scan_(*this); }
    [[nodiscard]] auto has_error() const -> bool override { return error_; }
    auto logger() -> Logger & override { return log_; }

  private:
    friend ScannerImpl;

    using BaseInput = lexy::string_input<Grammar::encoding>;
    using State = decltype(Gringo::Input::State{std::declval<SymbolStore &>(), std::declval<SharedString>(),
                                                std::declval<BaseInput &>().reader().position()});
    using Input = StatefulInput<BaseInput, State>;
    using Scanner = decltype(lexy::scan<Grammar::control>(std::declval<Input &>(), std::declval<State &>(),
                                                          std::declval<report_error>()));

    Logger &log_;
    std::optional<Stm> res_;
    BaseInput base_input_;
    State state_;
    Input input_;
    Scanner scanner_;
    bool error_ = false;
    bool init_ = true;
};

auto scan_stream(Logger &log, SymbolStore &store, std::istream &in) -> Scanner {
    return Scanner{std::make_unique<StreamScanner>(log, store, in)};
}

auto scan_file(Logger &log, SymbolStore &store, char const *path) -> Scanner {
    return Scanner{std::make_unique<FileScanner>(log, store, path)};
}

auto scan_string(Logger &log, SymbolStore &store, std::string_view content) -> Scanner {
    return Scanner{std::make_unique<StringScanner>(log, store, content)};
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

template <class P> struct symbol_root : SymbolGrammar::control {
    static constexpr auto name = P::name;
    static constexpr auto rule = lexy::dsl::p<P> + lexy::dsl::eof;
    static constexpr auto value = lexy::forward<typename decltype(P::value)::return_type>;
};

auto parse_parts(Logger &log, SymbolStore &store, std::string_view str) -> std::vector<Input::ProgramParamVec> {
    auto lock = GCLock{store};
    auto input = lexy::string_input<Grammar::encoding>{str};
    auto error = false;
    if (auto res = lexy::parse<root<SymbolGrammar::program_param_vec_vec>>(input, store, report_error{log, error});
        res && !error) {
        return std::move(res).value();
    }
    throw parse_error{};
}

auto parse_const(Logger &log, SymbolStore &store, std::string_view str) -> std::pair<SharedString, SharedSymbol> {
    auto lock = GCLock{store};
    auto input = lexy::string_input<Grammar::encoding>{str};
    auto error = false;
    if (auto res = lexy::parse<root<SymbolGrammar::const_def>>(input, store, report_error{log, error}); res && !error) {
        return {SharedString{res.value().first}, SharedSymbol{res.value().second}};
    }
    throw parse_error{};
}

} // namespace Gringo::Input
