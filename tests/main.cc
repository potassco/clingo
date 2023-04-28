#include <algorithm>
#include <cstddef>
#include <iostream>
#include <istream>
#include <iterator>
#include <memory>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include <catch2/catch_test_macros.hpp>

#include <lexy/dsl.hpp>
#include <lexy/action/scan.hpp>
#include <lexy/action/parse.hpp>
#include <lexy/input/string_input.hpp>
#include <lexy_ext/report_error.hpp>
#include <lexy/callback.hpp>


namespace {

template <typename Encoding>
using default_location_counting = std::conditional_t<
    std::is_same_v<Encoding, lexy::byte_encoding>,
    lexy::byte_location_counting<>, lexy::code_unit_location_counting>;

/// Reader to read bytes from a buffer coupled with iterators that stay valid
/// even if the underlying buffer is reallocated.
template <typename StreamBuffer>
class StreamReader {
public:
    using encoding = typename StreamBuffer::encoding;
    using couning = typename StreamBuffer::counting;
    using char_type = typename encoding::char_type;

    class iterator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = char_type;
        using pointer = char_type const *;
        using reference = char_type const &;
        using iterator_category = std::forward_iterator_tag;

        iterator()
        : buffer_{nullptr}
        , offset_{0} { }

        iterator(StreamBuffer &buffer, size_t offset)
        : buffer_{&buffer}
        , offset_{offset} { }

        iterator& operator++() {
            ++offset_;
            return *this;
        }

        iterator operator++(int) {
            iterator retval = *this;
            ++(*this);
            return retval;
        }

        bool operator==(iterator other) const {
            return offset_ == other.offset_ && buffer_ == other.buffer_;
        }

        bool operator!=(iterator other) const {
            return !(*this == other);
        }

        char_type operator*() const {
            return buffer_->at(offset_);
        }

        size_t offset() const {
            return offset_;
        }

    private:
        StreamBuffer *buffer_;
        size_t offset_;
    };

    explicit StreamReader(StreamBuffer &buffer, size_t id = 0)
    : buffer_(&buffer)
    , id_{id} {
    }

    /// Obtain the next byte without changing the reader's position.
    auto peek() const {
        if (buffer_->is_eoi(id_)) {
            return encoding::eof();
        }
        else {
            return encoding::to_int_type(buffer_->at(id_));
        }
    }

    /// Advance position to the next byte.
    void bump() noexcept {
        ++id_;
    }

    /// Get an iterator to the current position of the reader.
    auto position() const noexcept {
        return iterator(*buffer_, id_);
    }

    /// Set the current position of the reader.
    void set_position(iterator new_pos) noexcept {
        id_ = new_pos.offset();
    }

    /// Discard all bytes before the current position.
    void discard_before() {
        buffer_->discard(id_);
    }
private:
    StreamBuffer *buffer_;
    std::size_t id_;
};

template <typename Encoding = lexy::default_encoding, typename Counting = default_location_counting<Encoding>>
class StreamBuffer {
public:
    using encoding  = Encoding;
    using counting = Counting;
    using char_type = typename Encoding::char_type;
    static_assert(sizeof(char_type) == sizeof(char), "only support single-byte encodings");

    StreamBuffer(std::istream &in)
    : in_{in} { }

    /// Check if the given offset no longer points to valid input.
    ///
    /// This function might read bytes from the input stream to determine the
    /// information.
    bool is_eoi(size_t id) {
        while (id >= start_ + buffer_.size()) {
            char c;
            // Note: better read a chunk
            if (in_.get(c)) {
                buffer_.emplace_back(c);
            }
            else {
                return true;
            }
        }
        return false;
    }

    /// Discard bytes before the given offset.
    void discard(size_t id) {
        if (id > start_) {
            unsigned col = 0;
            Counting counting;
            typename StreamReader<StreamBuffer>::iterator position{*this, id};
            StreamReader<StreamBuffer> reader{*this, start_};
            while (reader.position() != position) {
                assert (reader.peek() != encoding::eof());
                if (counting.try_match_newline(reader)) {
                    ++nl_;
                    col = 0;
                }
                else {
                    counting.match_column(reader);
                    ++col;
                }
            }
            buffer_.erase(buffer_.begin(), buffer_.begin() + (id - start_));
            start_ = id;
            last_nl_ = start_ - col;
        }
    }

    /// Get the byte at the given offset.
    ///
    /// The offset must either point to a byte in the buffer. Or, if the offset
    /// points to a previously discarded byte after the last discarded newline
    /// character, this function returns a space character.
    char_type at(size_t id) const {
        if (id >= start_) {
            return buffer_[id - start_];
        }
        //std::cerr << "id: " << id << ", nl: " << last_nl_ << std::endl;
        // TODO: why not???
        //assert (id > last_nl_);
        return ' ';
    }

    /// Get the offset where the first line still in the buffer starts together
    /// with number of discarded lines.
    std::pair<size_t, unsigned> last_newline() const {
        return {last_nl_, nl_};
    }

private:
    std::istream &in_;
    std::vector<char_type> buffer_;
    size_t start_{0};
    size_t last_nl_{0};
    unsigned nl_{1};
};

/// An input to read from a stream.
template <typename StreamBuffer>
class StreamInput {
public:
    using encoding = typename StreamBuffer::encoding;
    using counting = typename StreamBuffer::counting;
    using reader_type = StreamReader<StreamBuffer>;
    using iterator = typename reader_type::iterator;

    explicit StreamInput(StreamBuffer &buffer)
    : buffer_(&buffer) {
    }

    auto reader() const & {
        return reader_type{*buffer_, start_};
    }

    auto discard_before(iterator it) {
        start_ = it.offset();
        buffer_->discard(it.offset());
    }

    /// Get the beginning of the line w.r.t. the characters at the beginning of
    /// the underlying buffer.
    auto anchor() const {
        auto last_nl = buffer_->last_newline();
        return lexy::input_location_anchor<StreamInput>{iterator{*buffer_, last_nl.first}, last_nl.second};
    }

private:
    StreamBuffer *buffer_;
    size_t start_{0};
};

template <typename OutputIt, typename Input, typename Reader, typename Tag>
OutputIt write_error(OutputIt out, const lexy::error_context<Input>& context,
                     const lexy::error<Reader, Tag>& error, lexy::visualization_options opts,
                     const char* path)
{
    lexy_ext::diagnostic_writer<Input> writer(context.input(), opts);

    // Convert the context location and error location into line/column information.
    auto context_location
        = lexy::get_input_location<typename Input::counting>(context.input(), context.position(), context.input().anchor());
    auto location
        = lexy::get_input_location<typename Input::counting>(context.input(), error.position(), context_location.anchor());

    // Write the main error headline.
    out = writer.write_message(out, lexy_ext::diagnostic_kind::error,
                               [&](OutputIt out, lexy::visualization_options) {
                                   out = lexy::_detail::write_str(out, "while parsing ");
                                   out = lexy::_detail::write_str(out, context.production());
                                   return out;
                               });
    if (path != nullptr)
        out = writer.write_path(out, path);
    out = writer.write_empty_annotation(out);

    // Write an annotation for the context.
    if (location.line_nr() != context_location.line_nr())
    {
        out = writer.write_annotation(out, lexy_ext::annotation_kind::secondary, context_location,
                                      lexy::_detail::next(context.position()),
                                      [&](OutputIt out, lexy::visualization_options) {
                                          return lexy::_detail::write_str(out, "beginning here");
                                      });
        out = writer.write_empty_annotation(out);
    }

    // Write the main annotation.
    if constexpr (std::is_same_v<Tag, lexy::expected_literal>)
    {
        auto string = lexy::_detail::make_literal_lexeme<typename Reader::encoding>(error.string(),
                                                                                    error.length());

        out = writer.write_annotation(out, lexy_ext::annotation_kind::primary, location, error.index() + 1,
                                      [&](OutputIt out, lexy::visualization_options opts) {
                                          out = lexy::_detail::write_str(out, "expected '");
                                          out = lexy::visualize_to(out, string, opts);
                                          out = lexy::_detail::write_str(out, "'");
                                          return out;
                                      });
    }
    else if constexpr (std::is_same_v<Tag, lexy::expected_keyword>)
    {
        auto string = lexy::_detail::make_literal_lexeme<typename Reader::encoding>(error.string(),
                                                                                    error.length());

        out = writer.write_annotation(out, lexy_ext::annotation_kind::primary, location, error.end(),
                                      [&](OutputIt out, lexy::visualization_options opts) {
                                          out = lexy::_detail::write_str(out, "expected keyword '");
                                          out = lexy::visualize_to(out, string, opts);
                                          out = lexy::_detail::write_str(out, "'");
                                          return out;
                                      });
    }
    else if constexpr (std::is_same_v<Tag, lexy::expected_char_class>)
    {
        out = writer.write_annotation(out, lexy_ext::annotation_kind::primary, location, 1u,
                                      [&](OutputIt out, lexy::visualization_options) {
                                          out = lexy::_detail::write_str(out, "expected ");
                                          out = lexy::_detail::write_str(out, error.name());
                                          return out;
                                      });
    }
    else
    {
        out = writer.write_annotation(out, lexy_ext::annotation_kind::primary, location, error.end(),
                                      [&](OutputIt out, lexy::visualization_options) {
                                          return lexy::_detail::write_str(out, error.message());
                                      });
    }

    return out;
}

template <typename OutputIterator = int>
struct _report_error
{
    OutputIterator              _iter;
    lexy::visualization_options _opts;
    const char*                 _path;

    struct _sink
    {
        OutputIterator              _iter;
        lexy::visualization_options _opts;
        const char*                 _path;
        std::size_t                 _count;

        using return_type = std::size_t;

        template <typename Input, typename Reader, typename Tag>
        void operator()(const lexy::error_context<Input>& context,
                        const lexy::error<Reader, Tag>&   error)
        {
            if constexpr (std::is_same_v<OutputIterator, int>)
                write_error(lexy::cfile_output_iterator{stderr}, context, error, _opts, _path);
            else
                _iter = write_error(_iter, context, error, _opts, _path);
            ++_count;
        }

        std::size_t finish() &&
        {
            if (_count != 0)
                std::fputs("\n", stderr);
            return _count;
        }
    };
    constexpr auto sink() const
    {
        return _sink{_iter, _opts, _path, 0};
    }

    /// Specifies a path that will be printed alongside the diagnostic.
    constexpr _report_error path(const char* path) const
    {
        return {_iter, _opts, path};
    }

    /// Specifies an output iterator where the errors are written to.
    template <typename OI>
    constexpr _report_error<OI> to(OI out) const
    {
        return {out, _opts, _path};
    }

    /// Overrides visualization options.
    constexpr _report_error opts(lexy::visualization_options opts) const
    {
        return {_iter, opts, _path};
    }
};

/// An error callback that uses diagnostic_writer to print to stderr (by default).
constexpr auto report_error = _report_error<>{};

struct Color {
    friend bool operator==(Color const &a, Color const &b) {
        return a.r == b.r && a.g == b.g && a.b == b.b;
    }
    friend std::ostream &operator<<(std::ostream &out, Color const &c) {
        out << "RGB(" << static_cast<int>(c.r) << "," << static_cast<int>(c.g) << "," << static_cast<int>(c.b) << ")";
        return out;
    }
    std::uint8_t r, g, b;
};

struct Term {
    virtual ~Term() = default;
    virtual void print(std::ostream &out) const = 0;
    std::string to_string() {
        std::ostringstream out;
        out << *this;
        return out.str();
    }
    friend std::ostream &operator<<(std::ostream &out, Term const &term) {
        term.print(out);
        return out;
    }
};

using UTerm = std::unique_ptr<Term>;

struct TermInteger : Term {
    explicit TermInteger(int v)
    : value(v) { }

    void print(std::ostream &out) const override {
        out << value;
    }

    int value;
};

enum class UnaryOperator {
    negate,
};

std::ostream &operator<<(std::ostream &out, UnaryOperator op) {
    assert(op == UnaryOperator::negate);
    out << "-";
    return out;
}

struct TermUnary : Term {
    explicit TermUnary(UnaryOperator op, UTerm e)
    : op(op)
    , rhs(std::move(e)) { }

    void print(std::ostream &out) const override {
        out << "(" << op << *rhs << ")";
    }

    UnaryOperator op;
    UTerm rhs;
};

enum BinaryOperator {
    plus,
    minus,
    times,
    div,
    pow,
};

std::ostream &operator<<(std::ostream &out, BinaryOperator op) {
    switch(op) {
        case plus: {
            out << "+";
            break;
        }
        case minus: {
            out << "-";
            break;
        }
        case times: {
            out << "*";
            break;
        }
        case div: {
            out << "/";
            break;
        }
        case pow: {
            out << "**";
            break;
        }
    }
    return out;
}

struct TermBinary : Term {
    explicit TermBinary(UTerm lhs, BinaryOperator op, UTerm rhs)
    : op(op), lhs(std::move(lhs)), rhs(std::move(rhs))
    {}

    void print(std::ostream &out) const override {
        out << "(" << *lhs << op << *rhs << ")";
    }

    BinaryOperator op;
    UTerm lhs;
    UTerm rhs;
};

namespace grammar {

namespace dsl = lexy::dsl;
using iterator = StreamInput<StreamBuffer<>>::iterator;


struct channel {
    static constexpr auto rule = dsl::integer<std::uint8_t>(dsl::n_digits<2, dsl::hex>);
    static constexpr auto value = lexy::forward<std::uint8_t>;
};

struct color {
    static constexpr auto rule = dsl::hash_sign + dsl::times<3>(dsl::p<channel>);
    static constexpr auto value = lexy::construct<Color>;
};

constexpr auto escaped_newline = dsl::backslash >> dsl::newline;

struct integer : lexy::token_production {
    static constexpr auto rule = LEXY_LIT("0x") >> dsl::integer<int, dsl::hex> | dsl::integer<int>;
    static constexpr auto value = lexy::forward<int>;
};
struct nested_expr : lexy::transparent_production {
    static constexpr auto whitespace = dsl::ascii::space | dsl::newline;
    static constexpr auto rule = dsl::recurse<struct expr>;
    static constexpr auto value = lexy::forward<UTerm>;
};

struct expr : lexy::expression_production {
    struct expected_operand {
        static constexpr auto name = "expected operand";
    };

    // We need to specify the atomic part of an expression.
    static constexpr auto atom = [] {
        auto paren_expr = dsl::parenthesized(dsl::p<nested_expr>);
        auto literal    = dsl::p<integer>;
        return paren_expr | literal | dsl::error<expected_operand>;
    }();

    struct math_power : dsl::infix_op_right {
        static constexpr auto op = dsl::op<BinaryOperator::pow>(LEXY_LIT("**"));
        using operand = dsl::atom;
    };

    struct math_prefix : dsl::prefix_op {
        static constexpr auto op = dsl::op<UnaryOperator::negate>(LEXY_LIT("-"));
        using operand = math_power;
    };

    struct math_product : dsl::infix_op_left {
        static constexpr auto op = [] {
            auto star = dsl::not_followed_by(LEXY_LIT("*"), dsl::lit_c<'*'>);
            return dsl::op<BinaryOperator::times>(star) /
                   dsl::op<BinaryOperator::div>(LEXY_LIT("/"));
        }();
        using operand = math_prefix;
    };

    struct math_sum : dsl::infix_op_left {
        static constexpr auto op = dsl::op<BinaryOperator::plus>(LEXY_LIT("+")) /
                                   dsl::op<BinaryOperator::minus>(LEXY_LIT("-"));
        using operand = math_product;
    };

    using operation = math_sum;
    static constexpr auto value =
        lexy::callback(
            lexy::forward<UTerm>,
            lexy::new_<TermInteger, UTerm>,
            lexy::new_<TermUnary, UTerm>,
            lexy::new_<TermBinary, UTerm>);
};

struct separator {
    static constexpr auto rule = dsl::lit_c<';'>;
    static constexpr auto value = lexy::construct<void>;
};

struct eoi {
    static constexpr auto rule = dsl::eof;
    static constexpr auto value = lexy::construct<void>;
};

struct A {
    static constexpr auto rule = dsl::lit_c<'a'>;
    static constexpr auto value = lexy::construct<void>;
};

struct statement {
    static constexpr char const *name = "statement";
    static constexpr auto rule = dsl::p<nested_expr> + dsl::lit_c<';'> + dsl::position;
    static constexpr auto value = lexy::construct<std::pair<UTerm, iterator>>;
};

} // namespace grammar

} // namespace

TEST_CASE("scanner-test") {
    auto input = lexy::zstring_input("b");
    auto scanner = lexy::scan(input, lexy_ext::report_error);
    REQUIRE(!scanner.parse<grammar::A>().has_value());
    //REQUIRE(!scanner.parse<grammar::eoi>().has_value());
    //scanner.parse(LEXY_LIT("a"));
}

TEST_CASE("term-test") {
    std::istringstream in;
    in.str("42  *-\n2-32**3;\n43a");
    StreamBuffer buf{in};
    auto input = StreamInput{buf};
    auto scanner = lexy::scan(input, report_error);
    auto c1 = scanner.parse<grammar::nested_expr>();
    REQUIRE(c1.has_value());
    REQUIRE(c1.value()->to_string() == "((42*(-2))-(32**3))");
    REQUIRE(scanner.parse<grammar::separator>().has_value());
    scanner.remaining_input().reader().discard_before();
    auto c2 = scanner.parse<grammar::nested_expr>();
    REQUIRE(c2.has_value());
    REQUIRE(c2.value()->to_string() == "43");
    scanner.remaining_input().reader().discard_before();
    auto res = scanner.parse<grammar::eoi>();
    //REQUIRE(!res.has_value());
}

TEST_CASE("term-test-working") {
    // NOTE: this works and is very close to what I want!!!
    // just accessing the last position is a bit annoying
    std::istringstream in;
    in.str("42  *-\n2-32**3;\n43a;");
    StreamBuffer buf{in};
    auto input = StreamInput{buf};
    auto stm = lexy::parse<grammar::statement>(input, report_error);
    REQUIRE(stm.has_value());
    input.discard_before(stm.value().second);
    stm = lexy::parse<grammar::statement>(input, report_error);
    REQUIRE(!stm.has_value());
}

TEST_CASE("test") {
    std::istringstream in;
    in.str("#FF00FF\n#AA00EE\n#AA00XE");
    StreamBuffer buf{in};
    auto input = StreamInput{buf};
    auto scanner = lexy::scan(input, report_error);
    auto c1 = scanner.parse<grammar::color>();
    REQUIRE(scanner);
    REQUIRE(c1.has_value());
    REQUIRE(c1.value() == Color{255, 0, 255});
    scanner.parse(lexy::dsl::newline);
    scanner.remaining_input().reader().discard_before();
    REQUIRE(scanner);
    auto c2 = scanner.parse<grammar::color>();
    REQUIRE(scanner);
    REQUIRE(c2.has_value());
    REQUIRE(c2.value() == Color{170, 0, 238});
    scanner.parse(lexy::dsl::newline);
    scanner.remaining_input().reader().discard_before();
    REQUIRE(!scanner.parse<grammar::color>().has_value());
};
