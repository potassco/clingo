#pragma once

#include <istream>
#include <vector>

#include <lexy/input_location.hpp>

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
        auto at(size_t id) const -> char_type {
            if (id >= start_) {
                return static_cast<char_type>(buffer_[id - start_]);
            }
            return ' ';
        }

        /// Offsets before this value have been discarded.
        auto offset() const {
            return start_;
        }

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
        auto peek() const {
            if (buffer_->is_eoi(offset_)) {
                return encoding::eof();
            }
            return encoding::to_int_type(buffer_->at(offset_));
        }

        /// Advance position to the next byte.
        void bump() noexcept { ++offset_; }

        /// Get an iterator to the current position of the reader.
        auto position() const noexcept { return iterator(*buffer_, offset_); }

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
    auto anchor() const {
        return lexy::input_location_anchor<StreamInput>{iterator{buffer_, last_nl_}, nl_};
    }

  private:
    mutable StreamBuffer buffer_;
    size_t last_nl_{0};
    unsigned nl_{1};
};
