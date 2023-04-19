#include <algorithm>
#include <cstddef>
#include <iostream>
#include <istream>
#include <list>
#include <iterator>

#include <catch2/catch_test_macros.hpp>

#include <sstream>
#include <tao/pegtl.hpp>

#include <lexy/dsl.hpp>
#include <lexy/input/range_input.hpp>

// lexy

namespace {

struct Color {
    std::uint8_t r, g, b;
};

namespace Grammar {

namespace dsl = lexy::dsl;

struct Channel {
    static constexpr auto rule = dsl::n_digits<2, dsl::hex>;
};

struct Color {
    //static constexpr auto rule = dsl::hash_sign + dsl::p<Channel> + dsl::p<Channel> + dsl::p<Channel>;
    static constexpr auto rule = dsl::hash_sign + dsl::times<3>(dsl::p<Channel>) + dsl::eof;
};



} // namespace Grammar

} // namespace




namespace pegtl = tao::pegtl;

namespace {

    struct lower
        : pegtl::range<'a', 'z'>
    {};

    struct upper
        : pegtl::range<'A', 'Z'>
    {};

    struct pre_name
        : pegtl::star<pegtl::sor<pegtl::one<'_'>, pegtl::one<'\''>>>
    {};

    struct post_name
        : pegtl::star<pegtl::sor<lower, upper, pegtl::digit, pegtl::one<'_'>, pegtl::one<'\''>>>
    {};

    struct identifier
        : pegtl::if_must<lower, post_name>
    {};

    struct variable
        : pegtl::if_must<upper, post_name>
    {};

    struct number
        : pegtl::sor<
              pegtl::one<'0'>,
              pegtl::seq<pegtl::range<'1', '9'>, pegtl::star<pegtl::digit>>
          >
    {};

    struct sum_term;

    struct atomic_term
        : pegtl::sor<
              pegtl::seq<pegtl::one<'('>, sum_term, pegtl::one<')'>>,
              pegtl::seq<pre_name, pegtl::sor<identifier, variable>>,
              number
          >
    {};

    struct mul_term
        : pegtl::list_must<atomic_term, pegtl::one<'*'>>
    {};

    struct sum_term
        : pegtl::list_must<mul_term, pegtl::one<'+'>>
    {};

    struct term : sum_term {};

    struct grammar
        : pegtl::must<term, pegtl::eof>
    {};

    struct Builder {
    };

    struct TermBuilder {
        std::string prefix;
    };

    template< typename Rule >
    struct action : pegtl::nothing< Rule > { };

    template<>
    struct action<term>
    : tao::pegtl::change_states<TermBuilder> {
        template< typename ParseInput >
        static void success(ParseInput const &, TermBuilder &tbld, Builder &bld) { }
    };

    template<>
    struct action<sum_term> {
        template<typename ParseInput>
        static void apply(const ParseInput& in, TermBuilder &bld) {
            std::cerr << "got sum b: " << in.string() << std::endl;
        }
    };

    template<>
    struct action<pre_name> {
        template<typename ParseInput>
        static void apply(const ParseInput& in, TermBuilder &bld) {
            bld.prefix = in.string();
        }
    };

    template<>
    struct action<identifier> {
        template<typename ParseInput>
        static void apply(const ParseInput& in, TermBuilder &bld) {
            std::cerr << "idr: " << bld.prefix << in.string() << std::endl;
        }
    };

    template<>
    struct action<variable> {
        template<typename ParseInput>
        static void apply(const ParseInput& in, TermBuilder &bld) {
            std::cerr << "var: " << bld.prefix << in.string() << std::endl;
        }
    };

    template<>
    struct action<number> {
        template<typename ParseInput>
        static void apply(const ParseInput& in, TermBuilder &bld) {
            std::cerr << "num: " << in.string() << std::endl;
        }
    };

    template<>
    struct action<atomic_term> {
        template<typename ParseInput>
        static void apply(const ParseInput& in, TermBuilder &bld) {
            std::cerr << "atm: " << in.string() << std::endl;
        }
    };

    template<>
    struct action<mul_term> {
        template<typename ParseInput>
        static void apply(const ParseInput& in, TermBuilder &bld) {
            std::cerr << "mul: " << in.string() << std::endl;
        }
    };
}

class StreamBuffer {
public:
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
            if (buffer_ == other.buffer_) {
                return offset_ == other.offset_;
            }
            if (buffer_ == nullptr) {
                return !other.buffer_->read_(other.offset_);
            }
            assert (other.buffer_ == nullptr);
            return !buffer_->read_(offset_);
        }

        bool operator!=(iterator other) const {
            return !(*this == other);
        }

        char operator*() const {
            return buffer_->get_(offset_);
        }

    private:
        friend StreamBuffer;
        StreamBuffer *buffer_;
        size_t offset_;
    };

    StreamBuffer(std::istream &stream)
    : stream_{stream} {
    }

    //! An iterator representing the beginning of the stream.
    //!
    //! Iterators pointing to discarded values must not be dereferenced.
    iterator begin() {
        return iterator{*this, 0};
    }

    //! An iterator representing the end of input.
    iterator end() {
        return iterator{};
    }

    //! Discard all values before the given iterator.
    void discard(iterator it) {
        if (start_ < it.offset_) {
            size_t n = it.offset_ - start_;
            start_ = it.offset_;
            buffer_.erase(buffer_.begin(), buffer_.begin() + n);
        }
    }

private:
    //! Ensure that at least `n` bytes have been read.
    //!
    //! The function returns false if the end of file has been reached before
    //! reading the required number of bytes.
    bool read_(size_t n) {
        // An efficient implementation should read a large chunk of bytes.
        if (n >= start_ + buffer_.size()) {
            buffer_.reserve(n - start_);
            while (buffer_.size() <= n - start_) {
                char c;
                if (stream_.get(c)) {
                    buffer_.emplace_back(c);
                }
                else {
                    return false;
                }
            }
        }
        return true;
    }

    //! Returns the `i`-th byte read from the stream.
    //!
    //! The byte must not have already been discarded.
    char get_(size_t i) {
        assert(i >= start_);
        read_(i);
        return buffer_[i - start_];
    }

    std::istream &stream_;
    std::vector<char> buffer_;
    size_t start_{0};
};

TEST_CASE("test") {
    SECTION("pegtl") {
        Builder bld;
        pegtl::parse<grammar, action>(pegtl::string_input{"(__xX123+123+2)*'X7", "from"}, bld);
        try {
            pegtl::parse<grammar, action>(pegtl::string_input{"fäil", "from"}, bld);
        }
        catch (std::exception &e) {
            static_cast<void>(e);
        }
    }
    SECTION("lexy") {
        std::istringstream stream{"#FF00FF"};
        StreamBuffer sbuf{stream};
        auto good = lexy::range_input(sbuf.begin(), sbuf.end());
        REQUIRE(lexy::match<Grammar::Color>(good));
    }
};
