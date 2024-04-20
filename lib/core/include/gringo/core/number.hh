#include <cstdint>
#include <optional>
#include <ostream>
#include <string>

namespace Gringo {

//! @addtogroup core_number
//! @{

//! The base of a number.
enum class Base : uint8_t {
    dec = 10, //!< Decimal base.
    hex = 16, //!< Hexadecimal base.
    oct = 8,  //!< Octal base.
    bin = 2,  //!< Binary base.
};

//! An arbitrary precision integer.
//!
//! Directly stores a 32bit integer if possible and otherwise a pointer to an arbitrary size integer.
class Number {
  public:
    //! Construct a number from an integer.
    Number(int32_t value) noexcept;
    //! Construct a number from a string.
    Number(char const *str, Base base = Base::dec);

    //! Copy construct a number.
    Number(Number const &other);
    //! Move construct a number.
    Number(Number &&other) noexcept;
    //! Copy assign a number.
    auto operator=(Number const &other) -> Number &;
    //! Move assign a number.
    auto operator=(Number &&other) noexcept -> Number &;

    //! Destroy the number freeing associated memory.
    ~Number() noexcept;

    //! Convert the number to an integer if possible.
    [[nodiscard]] auto as_int() const -> std::optional<int32_t>;

    //! Convert the number into a string.
    [[nodiscard]] auto as_string() const -> std::string;

    //! Swap two numbers.
    void swap(Number &other) noexcept;

    // comparison

    //! Compare to numbers returning a comparator.
    friend auto compare(Number const &a, Number const &b) -> int;
    //! Compare to numbers returning a comparator.
    friend auto compare(Number const &a, int32_t b) -> int;
    //! Compare to numbers returning a comparator.
    friend auto compare(int32_t a, Number const &b) -> int;

    //! Equality compare two numbers.
    friend auto operator==(Number const &a, Number const &b) { return compare(a, b) == 0; }
    //! Equality compare two numbers.
    friend auto operator==(int32_t a, Number const &b) -> bool { return compare(a, b) == 0; }
    //! Equality compare two numbers.
    friend auto operator==(Number const &a, int32_t b) -> bool { return compare(a, b) == 0; }

    //! Compare two numbers.
    friend auto operator<=>(Number const &a, Number const &b) { return compare(a, b) <=> 0; }
    //! Compare two numbers.
    friend auto operator<=>(int32_t a, Number const &b) { return compare(a, b) <=> 0; }
    //! Compare two numbers.
    friend auto operator<=>(Number const &a, int32_t b) { return compare(a, b) <=> 0; }

    // addition

    //! Sum up two numbers.
    friend auto operator+(Number const &a, Number const &b) -> Number;
    //! Sum up two numbers.
    friend auto operator+(Number &&a, Number const &b) -> Number;
    //! Sum up two numbers.
    friend auto operator+(Number const &a, Number &&b) -> Number;
    //! Sum up two numbers.
    friend auto operator+(Number &&a, Number &&b) -> Number;

    //! In place add the given number.
    friend auto operator+=(Number &a, Number const &b) -> Number &;
    //! In place add the given number.
    friend auto operator+=(Number &a, Number &&b) -> Number &;

    // subtraction

    //! Subtract two numbers.
    friend auto operator-(Number const &a, Number const &b) -> Number;
    //! Subtract two numbers.
    friend auto operator-(Number &&a, Number const &b) -> Number;
    //! Subtract two numbers.
    friend auto operator-(Number const &a, Number &&b) -> Number;
    //! Subtract two numbers.
    friend auto operator-(Number &&a, Number &&b) -> Number;

    //! In place subtract the given number.
    friend auto operator-=(Number &a, Number const &b) -> Number &;
    //! In place subtract the given number.
    friend auto operator-=(Number &a, Number &&b) -> Number &;

    // multiplication

    //! Multiply two numbers.
    friend auto operator*(Number const &a, Number const &b) -> Number;
    //! Multiply two numbers.
    friend auto operator*(Number &&a, Number const &b) -> Number;
    //! Multiply two numbers.
    friend auto operator*(Number const &a, Number &&b) -> Number;
    //! Multiply two numbers.
    friend auto operator*(Number &&a, Number &&b) -> Number;

    //! In place multiply the given number.
    friend auto operator*=(Number &a, Number const &b) -> Number &;
    //! In place multiply the given number.
    friend auto operator*=(Number &a, Number &&b) -> Number &;

    // division

    //! Divide two numbers.
    friend auto operator/(Number const &a, Number const &b) -> Number;
    //! Divide two numbers.
    friend auto operator/(Number &&a, Number const &b) -> Number;
    //! Divide two numbers.
    friend auto operator/(Number const &a, Number &&b) -> Number;
    //! Divide two numbers.
    friend auto operator/(Number &&a, Number &&b) -> Number;

    //! In place divide the given number.
    friend auto operator/=(Number &a, Number const &b) -> Number &;
    //! In place divide the given number.
    friend auto operator/=(Number &a, Number &&b) -> Number &;

    // modulus

    //! Modulo of two numbers.
    friend auto operator%(Number const &a, Number const &b) -> Number;
    //! Modulo of two numbers.
    friend auto operator%(Number &&a, Number const &b) -> Number;
    //! Modulo of two numbers.
    friend auto operator%(Number const &a, Number &&b) -> Number;
    //! Modulo of two numbers.
    friend auto operator%(Number &&a, Number &&b) -> Number;

    //! In place modulo the given number.
    friend auto operator%=(Number &a, Number const &b) -> Number &;
    //! In place modulo the given number.
    friend auto operator%=(Number &a, Number &&b) -> Number &;

    // unary minus

    //! Negate the number.
    friend auto operator-(Number const &a) -> Number;
    //! Negate the number.
    friend auto operator-(Number &&a) -> Number;

    // complement

    //! Binary complement of the number.
    friend auto operator~(Number const &a) -> Number;
    //! Binary complement of the number.
    friend auto operator~(Number &&a) -> Number;

    // binary and

    //! Binary and of the two numbers.
    friend auto operator&(Number const &a, Number const &b) -> Number;
    //! Binary and of the two numbers.
    friend auto operator&(Number &&a, Number const &b) -> Number;
    //! Binary and of the two numbers.
    friend auto operator&(Number const &a, Number &&b) -> Number;
    //! Binary and of the two numbers.
    friend auto operator&(Number &&a, Number &&b) -> Number;

    //! In place binary and the given number.
    friend auto operator&=(Number &a, Number const &b) -> Number &;
    //! In place binary and the given number.
    friend auto operator&=(Number &a, Number &&b) -> Number &;

    // binary or

    //! Binary or of two numbers.
    friend auto operator|(Number const &a, Number const &b) -> Number;
    //! Binary or of two numbers.
    friend auto operator|(Number &&a, Number const &b) -> Number;
    //! Binary or of two numbers.
    friend auto operator|(Number const &a, Number &&b) -> Number;
    //! Binary or of two numbers.
    friend auto operator|(Number &&a, Number &&b) -> Number;

    //! In place binary or the given number.
    friend auto operator|=(Number &a, Number const &b) -> Number &;
    //! In place binary or the given number.
    friend auto operator|=(Number &a, Number &&b) -> Number &;

    // binary xor

    //! Binary xor of two numbers.
    friend auto operator^(Number const &a, Number const &b) -> Number;
    //! Binary xor of two numbers.
    friend auto operator^(Number &&a, Number const &b) -> Number;
    //! Binary xor of two numbers.
    friend auto operator^(Number const &a, Number &&b) -> Number;
    //! Binary xor of two numbers.
    friend auto operator^(Number &&a, Number &&b) -> Number;

    //! In place binary xor the given number.
    friend auto operator^=(Number &a, Number const &b) -> Number &;
    //! In place binary xor the given number.
    friend auto operator^=(Number &a, Number &&b) -> Number &;

    // exponentiation

    //! Exponentiation of two numbers.
    friend auto pow(Number const &a, Number const &b) -> Number;
    //! Exponentiation of two numbers.
    friend auto pow(Number &&a, Number const &b) -> Number;
    //! Exponentiation of two numbers.
    friend auto pow(Number const &a, Number &&b) -> Number;
    //! Exponentiation of two numbers.
    friend auto pow(Number &&a, Number &&b) -> Number;

    // absolute

    //! Get absolute of the given number.
    friend auto abs(Number const &a) -> Number;
    //! Get absolute of the given number.
    friend auto abs(Number &&a) -> Number;

    // get the sign of the number

    //! Get the sign of the given number.
    friend auto get_sign(Number const &a) -> int;

    // get a hash value for the number

    //! Get a hash code of the number suitable for unordered containers.
    friend auto hash_code(Number const &a) -> size_t;

    // output

    //! Output the given number.
    friend auto operator<<(std::ostream &out, Number const &num) -> std::ostream &;

    // conversion between numbers and their representations

    //! Construct a number from the given representation.
    static auto from_repr(uint64_t repr) -> Number { return {repr}; }

    //! Get the internal representation of the number.
    //!
    //! Can be used with NumberRef below.
    static auto to_repr(Number const &num) -> uint64_t { return num.repr_; }

    //! Get the internal representation of the number and set it to zero.
    //!
    //! A new number can be constructed from the given representation.
    static auto release(Number &num) -> uint64_t {
        auto repr = num.repr_;
        num.repr_ = 0;
        return repr;
    }

  private:
    class Impl;
    friend class Impl;
    //! Construct a number from a representation.
    Number(uint64_t repr);

    //! The representation of the number.
    uint64_t repr_;
};

//! A const reference to a number.
class NumberRef {
  public:
    //! Construct a reference to number 0.
    NumberRef() : repr_{0} {}
    //! Construct a reference to the number given as its representation.
    explicit NumberRef(uint64_t repr) : repr_{repr} {}
    //! Construct a reference to the given number.
    explicit NumberRef(Number const &num) : repr_{Number::to_repr(num)} {}
    //! Access members of the underlying number.
    auto operator->() const -> Number const * { return &static_cast<Number const &>(*this); }
    //! Convert to const ref to underlying number.
    operator Number const &() const {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        return reinterpret_cast<Number const &>(repr_);
    }
    //! Convert to const ref to underlying number.
    auto operator*() const -> Number const & { return static_cast<Number const &>(*this); }

  private:
    //! The representation of the number.
    uint64_t repr_;
};

//! @}

} // namespace Gringo

//! Hasher for numbers.
template <> struct std::hash<Gringo::Number> {
    //! Compute hash of string.
    auto operator()(Gringo::Number const &a) const -> size_t { return hash_code(a); }
};
