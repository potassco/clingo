#include <cassert>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include <lexy/action/scan.hpp>
#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <lexy/grammar.hpp>
#include <lexy_ext/report_error.hpp>

template <class Input, class Void = void> struct get_counting {
    using type = lexy::_default_location_counting<Input>;
};

template <class Input> struct get_counting<Input, std::void_t<typename Input::counting>> {
    using type = Input::counting;
};

template <class Input> auto get_anchor(Input const &input, int /*unused*/) -> decltype(input.anchor()) {
    return input.anchor();
}

template <class Input> auto get_anchor(Input const &input, long /*unused*/) {
    return lexy::input_location_anchor{input};
}

template <typename OutputIt, typename Input, typename Reader, typename Tag>
auto write_error(OutputIt out, const lexy::error_context<Input> &context, const lexy::error<Reader, Tag> &error,
                 lexy::visualization_options opts, const char *path) -> OutputIt {
    lexy_ext::diagnostic_writer<Input> writer(context.input(), opts);

    using Counting = get_counting<Input>::type;

    // Convert the context location and error location into line/column
    // information.
    auto context_location =
        lexy::get_input_location<Counting>(context.input(), context.position(), get_anchor(context.input(), 0));
    auto location = lexy::get_input_location<Counting>(context.input(), error.position(), context_location.anchor());

    // Write the main error headline.
    out = writer.write_message(out, lexy_ext::diagnostic_kind::error, [&](OutputIt out, lexy::visualization_options) {
        out = lexy::_detail::write_str(out, "while parsing ");
        out = lexy::_detail::write_str(out, context.production());
        return out;
    });
    if (path != nullptr) {
        out = writer.write_path(out, path);
    }
    out = writer.write_empty_annotation(out);

    // Write an annotation for the context.
    if (location.line_nr() != context_location.line_nr()) {
        out = writer.write_annotation(
            out, lexy_ext::annotation_kind::secondary, context_location, lexy::_detail::next(context.position()),
            [&](OutputIt out, lexy::visualization_options) { return lexy::_detail::write_str(out, "beginning here"); });
        out = writer.write_empty_annotation(out);
    }

    // Write the main annotation.
    if constexpr (std::is_same_v<Tag, lexy::expected_literal>) {
        auto string = lexy::_detail::make_literal_lexeme<typename Reader::encoding>(error.string(), error.length());

        out = writer.write_annotation(out, lexy_ext::annotation_kind::primary, location, error.index() + 1,
                                      [&](OutputIt out, lexy::visualization_options opts) {
                                          out = lexy::_detail::write_str(out, "expected '");
                                          out = lexy::visualize_to(out, string, opts);
                                          out = lexy::_detail::write_str(out, "'");
                                          return out;
                                      });
    } else if constexpr (std::is_same_v<Tag, lexy::expected_keyword>) {
        auto string = lexy::_detail::make_literal_lexeme<typename Reader::encoding>(error.string(), error.length());

        out = writer.write_annotation(out, lexy_ext::annotation_kind::primary, location, error.end(),
                                      [&](OutputIt out, lexy::visualization_options opts) {
                                          out = lexy::_detail::write_str(out, "expected keyword '");
                                          out = lexy::visualize_to(out, string, opts);
                                          out = lexy::_detail::write_str(out, "'");
                                          return out;
                                      });
    } else if constexpr (std::is_same_v<Tag, lexy::expected_char_class>) {
        out = writer.write_annotation(out, lexy_ext::annotation_kind::primary, location, 1U,
                                      [&](OutputIt out, lexy::visualization_options) {
                                          out = lexy::_detail::write_str(out, "expected ");
                                          out = lexy::_detail::write_str(out, error.name());
                                          return out;
                                      });
    } else {
        out = writer.write_annotation(
            out, lexy_ext::annotation_kind::primary, location, error.end(),
            [&](OutputIt out, lexy::visualization_options) { return lexy::_detail::write_str(out, error.message()); });
    }

    return out;
}

template <typename OutputIterator = int> struct _report_error {
    OutputIterator _iter;
    lexy::visualization_options _opts;
    const char *_path;

    struct _sink {
        OutputIterator _iter;
        lexy::visualization_options _opts;
        const char *_path;
        std::size_t _count;

        using return_type = std::size_t;

        template <typename Input, typename Reader, typename Tag>
        void operator()(const lexy::error_context<Input> &context, const lexy::error<Reader, Tag> &error) {
            if constexpr (std::is_same_v<OutputIterator, int>) {
                write_error(lexy::cfile_output_iterator{stderr}, context, error, _opts, _path);
            } else {
                _iter = write_error(_iter, context, error, _opts, _path);
            }
            ++_count;
        }

        auto finish() && -> std::size_t {
            if (_count != 0) {
                std::fputs("\n", stderr);
            }
            return _count;
        }
    };
    [[nodiscard]] constexpr auto sink() const { return _sink{_iter, _opts, _path, 0}; }

    /// Specifies a path that will be printed alongside the diagnostic.
    constexpr auto path(const char *path) const -> _report_error { return {_iter, _opts, path}; }

    /// Specifies an output iterator where the errors are written to.
    template <typename OI> constexpr auto to(OI out) const -> _report_error<OI> { return {out, _opts, _path}; }

    /// Overrides visualization options.
    [[nodiscard]] constexpr auto opts(lexy::visualization_options opts) const -> _report_error {
        return {_iter, opts, _path};
    }
};

/// An error callback that uses diagnostic_writer to print to stderr (by default).
constexpr auto report_error = _report_error{};

/// Location counting based on encoding.
template <typename Encoding>
using default_location_counting = std::conditional_t<std::is_same_v<Encoding, lexy::byte_encoding>,
                                                     lexy::byte_location_counting<>, lexy::code_unit_location_counting>;

/// An input to read from a stream.
template <typename Encoding = lexy::default_encoding, typename Counting = default_location_counting<Encoding>>
class StreamInput {
  public:
    using encoding = Encoding;
    using counting = Counting;
    using char_type = typename encoding::char_type;
    static_assert(sizeof(char_type) == sizeof(char), "only support single-byte encodings");

    class StreamBuffer {
      public:
        using encoding = StreamInput::encoding;
        using counting = StreamInput::counting;
        using char_type = StreamInput::char_type;

        StreamBuffer(std::istream &in) : in_{in} {}

        /// Check if the given offset no longer points to valid input.
        ///
        /// This function might read bytes from the input stream to determine the
        /// information.
        auto is_eoi(size_t id) -> bool {
            while (id >= start_ + buffer_.size()) {
                char c;
                // Note: better read a chunk
                if (in_.get(c)) {
                    buffer_.emplace_back(c);
                } else {
                    return true;
                }
            }
            return false;
        }

        /// Discard bytes before the given offset.
        void discard(size_t id) {
            buffer_.erase(buffer_.begin(), buffer_.begin() + (id - start_));
            start_ = id;
        }

        /// Get the byte at the given offset.
        ///
        /// The offset must either point to a byte in the buffer. Or, if the offset
        /// points to a previously discarded byte after the last discarded newline
        /// character, this function returns a space character.
        [[nodiscard]] auto at(size_t id) const -> char_type {
            if (id >= start_) {
                return static_cast<char_type>(buffer_[id - start_]);
            }
            return ' ';
        }

        /// Offsets before this value have been discarded.
        [[nodiscard]] auto offset() const { return start_; }

      private:
        std::istream &in_;
        std::vector<char_type> buffer_;
        size_t start_{0};
    };

    /// A forward iterator that stays valid even if the underlying buffer is relocated.
    class iterator {
      public:
        using difference_type = std::ptrdiff_t;
        using value_type = char_type;
        using pointer = char_type const *;
        using reference = char_type const &;
        using iterator_category = std::forward_iterator_tag;

        iterator() : buffer_{nullptr}, offset_{0} {}

        iterator(StreamBuffer &buffer, size_t offset) : buffer_{&buffer}, offset_{offset} {}

        auto operator++() -> iterator & {
            ++offset_;
            return *this;
        }

        auto operator++(int) -> iterator {
            iterator retval = *this;
            ++(*this);
            return retval;
        }

        auto operator==(iterator other) const -> bool { return offset_ == other.offset_ && buffer_ == other.buffer_; }

        auto operator!=(iterator other) const -> bool { return !(*this == other); }

        auto operator*() const -> char_type { return buffer_->at(offset_); }

        /// The offset from the beginning of the underlying buffer.
        [[nodiscard]] auto offset() const -> size_t { return offset_; }

      private:
        StreamBuffer const *buffer_;
        size_t offset_;
    };

    /// Reader to read bytes from a buffer coupled with iterators that stay valid
    /// even if the underlying buffer is reallocated.
    class StreamReader {
      public:
        using encoding = StreamInput::encoding;
        using couning = StreamInput::counting;
        using char_type = StreamInput::char_type;
        using iterator = StreamInput::iterator;

        explicit StreamReader(StreamBuffer &buffer) : buffer_(&buffer), offset_{buffer.offset()} {}

        /// Obtain the next byte without changing the reader's position.
        [[nodiscard]] auto peek() const {
            if (buffer_->is_eoi(offset_)) {
                return encoding::eof();
            }
            return encoding::to_int_type(buffer_->at(offset_));
        }

        /// Advance position to the next byte.
        void bump() noexcept { ++offset_; }

        /// Get an iterator to the current position of the reader.
        [[nodiscard]] auto position() const noexcept { return iterator(*buffer_, offset_); }

        /// Set the current position of the reader.
        void set_position(iterator new_pos) noexcept { offset_ = new_pos.offset(); }

      private:
        StreamBuffer *buffer_;
        std::size_t offset_;
    };

    explicit StreamInput(std::istream &in) : buffer_{in} {}

    /// Get the reader for the input.
    ///
    /// The reader starts at the latest discarded position.
    auto reader() const & { return StreamReader{buffer_}; }

    /// Discard all bytes before the given iterator.
    ///
    /// Additionally, place an anchor at the last newline before this position.
    auto discard_before(iterator it) {
        auto id = it.offset();
        if (id > buffer_.offset()) {
            unsigned col = 0;
            Counting counting;
            StreamReader reader{buffer_};
            while (reader.position() != it) {
                assert(reader.peek() != encoding::eof());
                if (counting.try_match_newline(reader)) {
                    ++nl_;
                    col = 1;
                } else {
                    counting.match_column(reader);
                    ++col;
                }
            }
            buffer_.discard(id);
            if (col > 0) {
                last_nl_ = buffer_.offset() - col + 1;
            }
        }
    }

    /// Get the beginning of the line w.r.t. the characters at the beginning of
    /// the underlying buffer.
    auto anchor() const { return lexy::input_location_anchor<StreamInput>{iterator{buffer_, last_nl_}, nl_}; }

  private:
    mutable StreamBuffer buffer_;
    size_t last_nl_{0};
    unsigned nl_{1};
};

namespace grammar {

namespace dsl = lexy::dsl;

using encoding = lexy::utf8_encoding;
using input = StreamInput<encoding>;
using iterator = input::iterator;
using lexeme = lexy::lexeme_for<input>;

struct identifier : lexy::token_production {
    static constexpr auto rule = []() {
        auto head = dsl::ascii::alpha;
        auto tail = dsl::ascii::alpha_underscore;
        auto id = dsl::identifier(head, tail);

        return id;
    }();
    static constexpr auto value = lexy::as_string<std::string>;
};

struct statement {
    static constexpr auto rule = dsl::p<identifier> + dsl::lit_c<';'>;
    static constexpr auto value = lexy::forward<std::string>;
};

struct control {
    static constexpr auto whitespace = dsl::ascii::space | dsl::newline;
};

} // namespace grammar

TEST_CASE("stream parsing") {
    std::istringstream in;
    in.str("a;\nb");
    auto input = grammar::input{in};
    auto scanner = lexy::scan<grammar::control>(input, report_error);
    scanner.parse<grammar::statement>();
    input.discard_before(scanner.position());
    scanner.parse<grammar::statement>();
}
