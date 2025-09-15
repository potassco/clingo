#pragma once

#include <clingo/core/symbol.hh>

#include <cstdint>
#include <vector>

namespace CppClingo {

//! @addtogroup core
//! @{

//! Format specification for a field.
struct FormatSpec {
    //! Enumeration of conversion options.
    //!
    //! If the string representation of a symbol is used, clingo strings are
    //! printed without quotes and with special characters.
    enum class Conversion : uint8_t {
        str = 0,  //!< Use the string representation of a symbol.
        repr = 1, //!< Use the clingo representation of a symbol.
    };

    //! The alignment options.
    enum class Align : uint8_t {
        none,   //!< No alignment specified (see defaults below).
        left,   //!< Use left alignment (default for non-numbers).
        right,  //!< Use right alignment (default for numbers).
        number, //!< Use number alignment (sign to the left/number to the right).
        center, //!< Use center alignment.
    };

    //! The sign options.
    enum class Sign : uint8_t {
        plus,  //!< Use an explicit plus sign for positive numbers.
        minus, //!< Use a minus sign for negative numbers only (default).
        space, //!< Use a space for positive numbers.
    };

    //! Enumeration of grouping options.
    enum class Grouping : uint8_t {
        none,       //! Do not use thousands separator (default).
        comma,      //! Use comma as thousands separator.
        underscore, //! Use underscore as thousands separator.
    };

    //! Enumeration of type options.
    enum class Type : uint8_t {
        character, //!< Output the character with the given integer code.
        binary,    //!< OUtput an integer in binary format.
        octal,     //!< Output an integer in octal format.
        decimal,   //!< Output an integer in decimal format.
        hex_lower, //!< Output an integer in hexadecimal format with lower-case letters.
        hex_upper, //!< Output an integer in hexadecimal format with upper-case letters.
        locale,    //!< Output an integer using the locale's conventions.
        string,    //!< Output symbol using their default string representation (default).
    };

    //! Construct a default format specification.
    FormatSpec() = default;
    //! Parse a format specification from a string.
    //!
    //! A format specification has the following syntax:
    //!
    //! @code
    //! spec     ::= variable(accessor*)[[fill]align][sign]["#"]"][width][grouping][type]
    //! variable ::= <clingo variable>
    //! accessor ::= "." <clingo identifier> | "[" <unsigned number> "]"
    //! fill     ::= <any character>
    //! align    ::= "<" | ">" | "=" | "^"
    //! sign     ::= "+" | "-" | " "
    //! width    ::= <unsigned number>
    //! grouping ::= "," | "_"
    //! type     ::= "b" | "c" | "d" | "o" | "x" | "X" | "n" | "s"
    //! @endcode
    [[nodiscard]] static auto build(SymbolStore &store, std::string_view str) -> std::optional<FormatSpec>;

    //! Compare two format specifications for equality.
    friend auto operator==(FormatSpec const &a, FormatSpec const &b) -> bool = default;
    //! Compare two format specifications.
    friend auto operator<=>(FormatSpec const &a, FormatSpec const &b) -> std::strong_ordering = default;
    //! Output the format specification to a stream.
    friend auto operator<<(std::ostream &out, FormatSpec const &spec) -> std::ostream &;
    //! Output the format specification to a stream.
    friend auto operator<<(Util::OutputBuffer &out, FormatSpec const &spec) -> Util::OutputBuffer &;
    //! Compute the hash of the format specification.
    [[nodiscard]] auto hash() const -> size_t {
        return Util::value_hash_record<FormatSpec>(accessors, width, fill, type, grouping, conversion, align, sign,
                                                   alternate_form);
    }

    //! The vector of accessors.
    //!
    //! One can for exmaples use `[0].name` to refer to `g` in the term
    //! `f(g(1),2)`.
    std::vector<std::variant<SharedString, size_t>> accessors;
    //! The width of the field.
    uint32_t width = 0;
    //! The fill character for padding if the field is wider than the content.
    std::optional<char> fill;
    //! The type of the field.
    Type type = Type::string;
    //! The grouping option.
    Grouping grouping = Grouping::none;
    //! The conversion option.
    Conversion conversion = Conversion::str;
    //! The alignment of the field.
    Align align = Align::none;
    //! The sign option.
    Sign sign = Sign::minus;
    //! Whether to use the alternate form.
    bool alternate_form = false;
};

//! @}

} // namespace CppClingo
