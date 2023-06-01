#pragma once

#include <cassert>
#include <istream>
#include <vector>

#include <lexy/input_location.hpp>

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

        static constexpr size_t chunk_size = 4096;

        StreamBuffer(std::istream &in) : in_{in} {}

        /// Check if the given offset no longer points to valid input.
        ///
        /// This function might read bytes from the input stream to determine the
        /// information.
        auto is_eoi(size_t id) -> bool {
            while (id - start_ + discard_ >= buffer_.size()) {
                if (eoi_) {
                    return true;
                }
                if (discard_ > 0) {
                    buffer_.erase(buffer_.begin(), buffer_.begin() + discard_);
                    discard_ = 0;
                }
                size_t old_size = buffer_.size();
                buffer_.resize(old_size + chunk_size);
                in_.read(buffer_.data() + old_size, chunk_size);
                size_t num = in_.gcount();
                if (num < chunk_size) {
                    eoi_ = true;
                    buffer_.resize(old_size + num);
                }
            }
            return false;
        }

        /// Mark bytes before the given offset for disposal.
        void discard(size_t id) {
            discard_ += id - start_;
            start_ = id;
        }

        /// Get the byte at the given offset.
        ///
        /// The offset must either point to a byte in the buffer. Or, if the
        /// offset points to a previously discarded byte, a space character is
        /// returned.
        [[nodiscard]] auto at(size_t id) const -> char_type {
            if (id >= start_) {
                return static_cast<char_type>(buffer_[id - start_ + discard_]);
            }
            return ' ';
        }

        [[nodiscard]] auto data(size_t id) const -> char const * {
            assert(id >= start_);
            return buffer_.data() + id - start_ + discard_;
        }

        /// Offsets before this value have been discarded.
        [[nodiscard]] auto offset() const { return start_; }

      private:
        std::istream &in_;
        std::vector<char> buffer_;
        size_t start_{0};
        size_t discard_{0};
        bool eoi_{false};
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
    class StreamReader : lexy::_detail::_swar_base {
      public:
        using encoding = StreamInput::encoding;
        using couning = StreamInput::counting;
        using char_type = StreamInput::char_type;
        using iterator = StreamInput::iterator;
        using swar_int = lexy::_detail::swar_int;

        static constexpr auto swar_length = sizeof(swar_int) / sizeof(unsigned char);

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

        /// Peek bytes fitting into the largest available unsigned integer.
        [[nodiscard]] auto peek_swar() const -> swar_int {
            swar_int result = 0;
            if (!buffer_->is_eoi(offset_ + swar_length - 1)) {
                auto ptr = buffer_->data(offset_);
#if LEXY_IS_LITTLE_ENDIAN
                std::memcpy(&result, ptr, sizeof(swar_int));
#else
                auto dst = reinterpret_cast<unsigned char *>(&result);
                for (auto i = 0U; i != swar_length; ++i) {
                    std::memcpy(dst + i, ptr + swar_length - i - 1, sizeof(unsigned char));
                }
#endif
            } else {
                auto *dst = reinterpret_cast<unsigned char *>(&result);
                for (auto it = position(); !buffer_->is_eoi(it.offset()); ++it) {
#if LEXY_IS_LITTLE_ENDIAN
                    std::memcpy(dst + it.offset() - offset_, buffer_->data(it.offset()), sizeof(unsigned char));
#else
                    std::memcpy(dst + swar_length - 1 - it.offset() + offset_, buffer_->data(it.offset()),
                                sizeof(unsigned char));
#endif
                }
            }
            return result;
        }

        /// Advance according to size of largest available unsigned integer.
        void bump_swar() { offset_ += swar_length; }

        /// Advance by the given number of bytes.
        void bump_swar(std::size_t char_count) { offset_ += char_count; }

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
