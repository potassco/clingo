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

// lexy

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
        if (id == last_nl_ || id < nl_) {
            return '\n';
        }
        return ' ';
    }

private:
    std::istream &in_;
    std::vector<char> buffer_;
    size_t start_{0};
    size_t last_nl_{0};
    size_t nl_{0};
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

private:
    StreamBuffer *buffer_;
    std::size_t id_{0};
};

} // namespace

TEST_CASE("test") {
    std::istringstream in;
    in.str("#FF00FF\n#AA00EE\n#AA00XE");
    StreamBuffer buf{in};
    auto input = StreamInput{buf};
    // an input comes with a reader that maintains a current position
    // my use case requires implementing an input together with a reader
    // having a discard functionality
    auto scanner = lexy::scan(input, lexy_ext::report_error);
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
