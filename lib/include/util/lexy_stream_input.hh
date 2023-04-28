#pragma once

#include <istream>
#include <vector>

#include <lexy/input_location.hpp>

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

