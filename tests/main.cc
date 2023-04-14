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
        : pegtl::seq<pre_name, pegtl::if_must<lower, post_name>>
    {};

    struct variable
        : pegtl::seq<pre_name, pegtl::if_must<upper, post_name>>
    {};

    struct number
        : pegtl::sor<
            pegtl::one<'0'>,
            pegtl::seq<pegtl::range<'1', '9'>, pegtl::star<pegtl::digit>>
          >
    {};

    struct term
        : pegtl::sor<
            identifier,
            variable,
            number
          >
    {};

   struct grammar
      : pegtl::must<term, pegtl::eof>
   {};

   template< typename Rule >
   struct action
   {};

   template<>
   struct action<identifier> {
      template<typename ParseInput>
      static void apply(const ParseInput& in) {
          std::cerr << "got identifier: " << in.string() << std::endl;
      }
   };

   template<>
   struct action<variable> {
      template<typename ParseInput>
      static void apply(const ParseInput& in) {
          std::cerr << "got variable: " << in.string() << std::endl;
      }
   };

   template<>
   struct action<number> {
      template<typename ParseInput>
      static void apply(const ParseInput& in) {
          std::cerr << "got number: " << in.string() << std::endl;
      }
   };
}

TEST_CASE("test") {
    pegtl::parse<grammar, action>(pegtl::string_input{"__xX123", "from"});
    pegtl::parse<grammar, action>(pegtl::string_input{"123", "from"});
    pegtl::parse<grammar, action>(pegtl::string_input{"'X_7", "from"});
    try {
        pegtl::parse<grammar, action>(pegtl::string_input{"fäil", "from"});
    }
    catch (std::exception &e) {
        static_cast<void>(e);
    }
};
