#include <cstdint>
#include <optional>
#include <ostream>
#include <string>

namespace Gringo {

class Number {
  public:
    Number(int32_t value) noexcept;
    Number(char const *str);

    Number(Number const &other);
    Number(Number &&other) noexcept;
    auto operator=(Number const &other) -> Number &;
    auto operator=(Number &&other) noexcept -> Number &;

    ~Number() noexcept;

    [[nodiscard]] auto as_int() const -> std::optional<int32_t>;

    [[nodiscard]] auto as_string() const -> std::string;

    // comparison

    friend auto operator==(Number const &a, Number const &b) -> bool;
    friend auto operator==(int32_t a, Number const &b) -> bool;
    friend auto operator==(Number const &a, int32_t b) -> bool;

    friend auto operator!=(Number const &a, Number const &b) -> bool;
    friend auto operator!=(int32_t a, Number const &b) -> bool;
    friend auto operator!=(Number const &a, int32_t b) -> bool;

    friend auto operator<(Number const &a, Number const &b) -> bool;
    friend auto operator<(int32_t a, Number const &b) -> bool;
    friend auto operator<(Number const &a, int32_t b) -> bool;

    friend auto operator<=(Number const &a, Number const &b) -> bool;
    friend auto operator<=(int32_t a, Number const &b) -> bool;
    friend auto operator<=(Number const &a, int32_t b) -> bool;

    friend auto operator>(Number const &a, Number const &b) -> bool;
    friend auto operator>(int32_t a, Number const &b) -> bool;
    friend auto operator>(Number const &a, int32_t b) -> bool;

    friend auto operator>=(Number const &a, Number const &b) -> bool;
    friend auto operator>=(int32_t a, Number const &b) -> bool;
    friend auto operator>=(Number const &a, int32_t b) -> bool;

    // addition

    friend auto operator+(Number const &a, Number const &b) -> Number;
    friend auto operator+(Number &&a, Number const &b) -> Number;
    friend auto operator+(Number const &a, Number &&b) -> Number;
    friend auto operator+(Number &&a, Number &&b) -> Number;

    friend auto operator+=(Number &a, Number const &b) -> Number &;
    friend auto operator+=(Number &a, Number &&b) -> Number &;

    // subtraction

    friend auto operator-(Number const &a, Number const &b) -> Number;
    friend auto operator-(Number &&a, Number const &b) -> Number;
    friend auto operator-(Number const &a, Number &&b) -> Number;
    friend auto operator-(Number &&a, Number &&b) -> Number;

    friend auto operator-=(Number &a, Number const &b) -> Number &;
    friend auto operator-=(Number &a, Number &&b) -> Number &;

    // multiplication

    friend auto operator*(Number const &a, Number const &b) -> Number;
    friend auto operator*(Number &&a, Number const &b) -> Number;
    friend auto operator*(Number const &a, Number &&b) -> Number;
    friend auto operator*(Number &&a, Number &&b) -> Number;

    friend auto operator*=(Number &a, Number const &b) -> Number &;
    friend auto operator*=(Number &a, Number &&b) -> Number &;

    // division

    friend auto operator/(Number const &a, Number const &b) -> Number;
    friend auto operator/(Number &&a, Number const &b) -> Number;
    friend auto operator/(Number const &a, Number &&b) -> Number;
    friend auto operator/(Number &&a, Number &&b) -> Number;

    friend auto operator/=(Number &a, Number const &b) -> Number &;
    friend auto operator/=(Number &a, Number &&b) -> Number &;

    // division

    friend auto operator%(Number const &a, Number const &b) -> Number;
    friend auto operator%(Number &&a, Number const &b) -> Number;
    friend auto operator%(Number const &a, Number &&b) -> Number;
    friend auto operator%(Number &&a, Number &&b) -> Number;

    friend auto operator%=(Number &a, Number const &b) -> Number &;
    friend auto operator%=(Number &a, Number &&b) -> Number &;

    // unary minus

    friend auto operator-(Number const &a) -> Number;
    friend auto operator-(Number &&a) -> Number;

    // complement

    friend auto operator~(Number const &a) -> Number;
    friend auto operator~(Number &&a) -> Number;

    // binary and

    friend auto operator&(Number const &a, Number const &b) -> Number;
    friend auto operator&(Number &&a, Number const &b) -> Number;
    friend auto operator&(Number const &a, Number &&b) -> Number;
    friend auto operator&(Number &&a, Number &&b) -> Number;

    friend auto operator&=(Number &a, Number const &b) -> Number &;
    friend auto operator&=(Number &a, Number &&b) -> Number &;

    // binary or

    friend auto operator|(Number const &a, Number const &b) -> Number;
    friend auto operator|(Number &&a, Number const &b) -> Number;
    friend auto operator|(Number const &a, Number &&b) -> Number;
    friend auto operator|(Number &&a, Number &&b) -> Number;

    friend auto operator|=(Number &a, Number const &b) -> Number &;
    friend auto operator|=(Number &a, Number &&b) -> Number &;

    // binary xor

    friend auto operator^(Number const &a, Number const &b) -> Number;
    friend auto operator^(Number &&a, Number const &b) -> Number;
    friend auto operator^(Number const &a, Number &&b) -> Number;
    friend auto operator^(Number &&a, Number &&b) -> Number;

    friend auto operator^=(Number &a, Number const &b) -> Number &;
    friend auto operator^=(Number &a, Number &&b) -> Number &;

    // exponentiation

    friend auto pow(Number const &a, Number const &b) -> Number;
    friend auto pow(Number &&a, Number const &b) -> Number;
    friend auto pow(Number const &a, Number &&b) -> Number;
    friend auto pow(Number &&a, Number &&b) -> Number;

    // absolute

    friend auto abs(Number const &a) -> Number;
    friend auto abs(Number &&a) -> Number;

    // get the sign of the number

    friend auto get_sign(Number const &a) -> int;

    // get a hash value for the number

    friend auto hash_code(Number const &a) -> size_t;

    // output

    friend auto operator<<(std::ostream &out, Number const &num) -> std::ostream &;

    // conversion between numbers and their representations

    static auto from_repr(uint64_t repr) -> Number { return {repr}; }

    static auto to_repr(Number const &num) -> uint64_t { return num.repr_; }

    static auto release(Number &num) -> uint64_t {
        auto repr = num.repr_;
        num.repr_ = 0;
        return repr;
    }

  private:
    class Impl;
    friend class Impl;
    Number(uint64_t repr);

    uint64_t repr_;
};

///! A const reference to a number.
class NumberRef {
  public:
    NumberRef() : repr_{0} {}
    explicit NumberRef(uint64_t repr) : repr_{repr} {}
    explicit NumberRef(Number const &num) : repr_{Number::to_repr(num)} {}
    auto operator->() const -> Number const * { return reinterpret_cast<Number const *>(&repr_); }
    operator Number const &() const { return reinterpret_cast<Number const &>(repr_); }
    auto operator*() const -> Number const & { return reinterpret_cast<Number const &>(repr_); }

  private:
    uint64_t repr_;
};

} // namespace Gringo

namespace std {

//! Hasher for numbers.
template <> struct hash<Gringo::Number> {
    //! Compute hash of string.
    auto operator()(Gringo::Number a) const -> size_t { return hash_code(a); }
};

} // namespace std
