#include <iostream>

#include <catch2/catch_test_macros.hpp>

#include <tao/pegtl.hpp>

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

} // namespace

TEST_CASE("pegtl") {
    Builder bld;
    pegtl::parse<term_grammar, action>(pegtl::string_input{"(__xX123+123+2)*'X7", "from"}, bld);
    try {
        pegtl::parse<term_grammar, action>(pegtl::string_input{"fäil", "from"}, bld);
    }
    catch (std::exception &e) {
        static_cast<void>(e);
    }
}
