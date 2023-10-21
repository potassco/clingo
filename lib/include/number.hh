#include <cstdint>
#include <optional>
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

    // unary minus

    friend auto operator-(Number const &a) -> Number;
    friend auto operator-(Number &&a) -> Number;

    // exponentiation

    friend auto pow(Number const &a, Number const &b) -> Number;
    friend auto pow(Number &&a, Number const &b) -> Number;
    friend auto pow(Number const &a, Number &&b) -> Number;
    friend auto pow(Number &&a, Number &&b) -> Number;

    static auto from_repr(uint64_t repr) -> Number { return {repr}; }

    static auto to_repr(Number const &num) -> uint64_t { return num.repr_; }

  private:
    class Impl;
    friend class Impl;
    Number(uint64_t repr);

    uint64_t repr_;
};

} // namespace Gringo
