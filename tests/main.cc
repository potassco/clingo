#include <algorithm>
#include <cstddef>
#include <iostream>
#include <istream>
#include <iterator>
#include <sstream>
#include <stdexcept>

#include <catch2/catch_test_macros.hpp>

#include <lexy/dsl.hpp>
#include <lexy/action/scan.hpp>
#include <lexy_ext/report_error.hpp>


namespace {

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

namespace grammar {

namespace dsl = lexy::dsl;

struct channel {
    static constexpr auto rule = dsl::integer<std::uint8_t>(dsl::n_digits<2, dsl::hex>);
    static constexpr auto value = lexy::forward<std::uint8_t>;
};

struct color {
    static constexpr auto rule = dsl::hash_sign + dsl::times<3>(dsl::p<channel>);
    static constexpr auto value = lexy::construct<Color>;
};

} // namespace grammar

class StreamBuffer {
public:
    StreamBuffer(std::istream &in)
    : in_{in} { }

    bool is_eof(size_t id) {
        while (id >= start_ + buffer_.size()) {
            char c;
            // TODO: read a chunk
            if (in_.get(c)) {
                buffer_.emplace_back(c);
            }
            else {
                return true;
            }
        }
        return false;
    }

    void discard(size_t n) {
        for (size_t i = 0; i < n; ++i) {
            if (buffer_[i] == '\n') {
                last_nl_ = start_ + i;
                ++nl_;
            }
        }
        start_ += n;
        buffer_.erase(buffer_.begin(), buffer_.begin() + n);
    }

    char at(size_t id) const {
        if (id >= start_) {
            return buffer_[id - start_];
        }
        if (id > last_nl_) {
            // Note that this will mess up the column count with multibyte charaters
            // this should be fixable using more clever counting in the discard function
            // by adjusting the last_nl_ offset a bit.
            //
            // Maybe a counting strategy could be used here...
            return ' ';
        }
        assert(id == last_nl_);
        return '\n';
    }

    std::pair<size_t, unsigned> last_newline() const {
        return {last_nl_, nl_};
    }

private:
    std::istream &in_;
    std::vector<char> buffer_;
    size_t start_{0};
    size_t last_nl_{0};
    unsigned nl_{0};
};

class StreamReader {
public:
    using encoding = lexy::utf8_char_encoding;
    class iterator {
    public:
        using difference_type = ptrdiff_t;
        using value_type = char;
        using pointer = char const *;
        using reference = char const &;
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
            return offset_ == other.offset_;
        }

        bool operator!=(iterator other) const {
            return !(*this == other);
        }

        char operator*() const {
            return buffer_->at(offset_);
        }

    private:
        friend class StreamReader;
        StreamBuffer *buffer_;
        size_t offset_;
    };

    explicit StreamReader(StreamBuffer &buffer)
    : buffer_(&buffer) {
    }

    char peek() const {
        if (buffer_->is_eof(id_)) {
            return -1;
        }
        else {
            return buffer_->at(id_);
        }
    }

    void bump() noexcept {
        ++id_;
    }

    iterator position() const noexcept {
        return iterator(*buffer_, id_);
    }

    void set_position(iterator new_pos) noexcept {
        id_ = new_pos.offset_;
    }

private:
    StreamBuffer *buffer_;
    std::size_t id_{0};
};

class StreamInput {
public:
    using encoding = lexy::default_encoding;
    using buffer_type = std::vector<encoding::char_type>;

    explicit StreamInput(StreamBuffer &buffer)
    : buffer_(&buffer) {
    }

    StreamReader reader() const & {
        return StreamReader{*buffer_};
    }

    auto anchor() const {
        auto last_nl = buffer_->last_newline();
        return lexy::input_location_anchor<StreamInput>{StreamReader::iterator{*buffer_, last_nl.first}, last_nl.second};
    }

private:
    StreamBuffer *buffer_;
};

template <typename OutputIt, typename Input, typename Reader, typename Tag>
OutputIt write_error(OutputIt out, const lexy::error_context<Input>& context,
                     const lexy::error<Reader, Tag>& error, lexy::visualization_options opts,
                     const char* path)
{
    lexy_ext::diagnostic_writer<Input> writer(context.input(), opts);

    // Convert the context location and error location into line/column information.
    auto context_location
        = lexy::get_input_location(context.input(), context.position(), context.input().anchor());
    auto location
        = lexy::get_input_location(context.input(), error.position(), context_location.anchor());

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

} // namespace

TEST_CASE("test") {
    std::istringstream in;
    in.str("#FF00FF\n#AA00EE\n#AA00XE");
    StreamBuffer buf{in};
    auto input = StreamInput{buf};
    // an input comes with a reader that maintains a current position
    // my use case requires implementing an input together with a reader
    // having a discard functionality
    auto scanner = lexy::scan(input, report_error);
    auto c1 = scanner.parse<grammar::color>();
    REQUIRE(scanner);
    REQUIRE(c1.has_value());
    REQUIRE(c1.value() == Color{255, 0, 255});
    scanner.parse(lexy::dsl::newline);
    buf.discard(8);
    REQUIRE(scanner);
    auto c2 = scanner.parse<grammar::color>();
    REQUIRE(scanner);
    REQUIRE(c2.has_value());
    REQUIRE(c2.value() == Color{170, 0, 238});
    scanner.parse(lexy::dsl::newline);
    buf.discard(8);
    REQUIRE(!scanner.parse<grammar::color>().has_value());
};
