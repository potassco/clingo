#include <algorithm>
#include <cstddef>
#include <iostream>
#include <istream>
#include <lexy/encoding.hpp>
#include <list>
#include <iterator>

#include <catch2/catch_test_macros.hpp>

#include <ostream>
#include <sstream>
#include <tao/pegtl.hpp>

#include <lexy/dsl.hpp>
#include <lexy/input/range_input.hpp>
#include <lexy/input/string_input.hpp>
#include <lexy/action/scan.hpp>
#include <lexy/action/parse.hpp>
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

    struct term_grammar
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

template <typename Encoding = lexy::default_encoding>
class StreamBuffer {
public:
    using encoding = Encoding;
    class sentinel {
    };
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

        bool operator==(sentinel other) const {
            static_cast<void>(other);
            return false;
        }

        bool operator!=(sentinel other) const {
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
    sentinel end() {
        return sentinel{};
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
    //! Returns the `i`-th byte read from the stream.
    //!
    //! The byte must not have already been discarded.
    char get_(size_t i) {
        assert(i >= start_);
        if (i >= start_ + buffer_.size()) {
            buffer_.reserve(i - start_);
            while (buffer_.size() <= i - start_) {
                char c;
                if (stream_.get(c)) {
                    std::cerr << "c: " << c << std::endl;
                    buffer_.emplace_back(c);
                }
                else {
                    std::cerr << "c: eof" << std::endl;
                    buffer_.emplace_back(-1);
                    break;
                }
            }
        }
        return buffer_[i - start_];
    }

    std::istream &stream_;
    std::vector<char> buffer_;
    size_t start_{0};
};

TEST_CASE("test") {
    SECTION("pegtl") {
        Builder bld;
        pegtl::parse<term_grammar, action>(pegtl::string_input{"(__xX123+123+2)*'X7", "from"}, bld);
        try {
            pegtl::parse<term_grammar, action>(pegtl::string_input{"fäil", "from"}, bld);
        }
        catch (std::exception &e) {
            static_cast<void>(e);
        }
    }
    SECTION("lexy") {
        std::istringstream in;
        in.str("#FF00FF\n#AA00EE");
        StreamBuffer buf{in};
        auto rng = lexy::range_input{buf.begin(), buf.end()};
        // an input comes with a reader that maintains a current position
        // my use case requires implementing an input together with a reader
        // having a discard functionality
        auto scanner = lexy::scan(rng, lexy_ext::report_error);
        auto c1 = scanner.parse<grammar::color>();
        REQUIRE(scanner);
        REQUIRE(c1.has_value());
        REQUIRE(c1.value() == Color{255, 0, 255});
        auto ri = scanner.remaining_input();
        REQUIRE(*ri.reader().position() == '\n');
        scanner.parse(lexy::dsl::newline);
        REQUIRE(scanner);
        auto c2 = scanner.parse<grammar::color>();
        REQUIRE(scanner);
        REQUIRE(c2.has_value());
        REQUIRE(c2.value() == Color{170, 0, 238});
        ri = scanner.remaining_input();
        REQUIRE(*ri.reader().position() == -1);
    }
};
