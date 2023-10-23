#include <cassert>
#include <memory>

#include <imath.h>

#include <util/checked_math.hh>

#include <number.hh>

namespace Gringo {

namespace {

constexpr int BASE = 10;
constexpr uint64_t BIGINT_MASK = 7;

// NOLINTBEGIN(readability-magic-numbers)

auto repr_is_int(uint64_t repr) -> bool { return (repr & BIGINT_MASK) == 0; }

auto repr_is_bigint(uint64_t repr) -> bool { return (repr & BIGINT_MASK) == BIGINT_MASK; }

auto repr_to_int(uint64_t repr) -> int32_t { return static_cast<int>(repr >> 32); }

auto repr_to_bigint(uint64_t repr) -> mp_int {
    return reinterpret_cast<mp_int>(static_cast<uintptr_t>(repr & ~BIGINT_MASK));
}

auto int_to_repr(int num) -> uint64_t { return static_cast<uint64_t>(num) << 32; }

auto bigint_to_repr(mp_int num, bool fast = false) -> uint64_t {
    if (mp_small inum = 0; !fast && mp_int_to_int(num, &inum) == MP_OK && check_cast<int32_t>(inum)) {
        return int_to_repr(static_cast<int32_t>(inum));
    }
    return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(num)) | BIGINT_MASK;
}

// NOLINTEND(readability-magic-numbers)

void handle_error(mp_result res) {
    if (res == MP_MEMORY) {
        throw std::bad_alloc();
    }
    if (res != MP_OK) {
        throw std::runtime_error(mp_error_string(res));
    }
}

void handle_error(mp_result res, mp_result skip) {
    if (res != skip) {
        handle_error(res);
    }
}

class mp_int_ptr {
  public:
    mp_int_ptr() : ptr_{mp_int_alloc()} {
        if (ptr_ == nullptr) {
            throw std::bad_alloc();
        }
    }
    ~mp_int_ptr() {
        if (ptr_ != nullptr) {
            mp_int_free(ptr_);
        }
    }
    auto release() -> mp_int {
        auto *ptr = ptr_;
        ptr_ = nullptr;
        return ptr;
    }
    auto release_repr(bool fast = false) -> uint64_t { return bigint_to_repr(release(), fast); }
    operator mp_int() const { return ptr_; }

  private:
    mp_int ptr_;
};

[[nodiscard]] auto mp_int_sub_value_inv(mp_int a, mp_small b, mp_int c) -> mp_result {
    if (auto res = mp_int_sub_value(a, b, c); res != MP_OK) {
        return res;
    }
    if (auto res = mp_int_neg(c, c); res != MP_OK) {
        return res;
    }
    return MP_OK;
}

[[nodiscard]] auto mp_int_floordiv(mp_int a, mp_int b, mp_int c) -> mp_result {
    auto zb = mp_int_compare_zero(b);
    if (zb == 0) {
        return MP_RANGE;
    }
    mp_int rem = mp_int_alloc();
    if (rem == nullptr) {
        return MP_MEMORY;
    }
    if (auto res = mp_int_div(a, b, c, rem); res != MP_OK) {
        mp_int_free(rem);
        return res;
    }
    auto zr = mp_int_compare_zero(rem);
    mp_int_free(rem);
    if ((zr > 0 && zb < 0) || (zr < 0 && zb > 0)) {
        if (auto res = mp_int_sub_value(c, 1, c); res != MP_OK) {
            return res;
        }
    }
    return MP_OK;
}

[[nodiscard]] auto mp_int_floordiv_value(mp_int a, mp_small b, mp_int c) -> mp_result {
    if (b == 0) {
        return MP_RANGE;
    }
    mp_small rem = 0;
    if (auto res = mp_int_div_value(a, b, c, &rem); res != MP_OK) {
        return res;
    }
    if ((rem > 0 && b < 0) || (rem < 0 && b > 0)) {
        if (auto res = mp_int_sub_value(c, 1, c); res != MP_OK) {
            return res;
        }
    }
    return MP_OK;
}

[[nodiscard]] auto mp_int_floordiv_value_inv(mp_int a, mp_small b, mp_int c) -> mp_result {
    if (a != c) {
        if (auto res = mp_int_set_value(c, b)) {
            return res;
        }
        return mp_int_floordiv(c, a, c);
    }
    mp_int z = mp_int_alloc();
    if (z == nullptr) {
        return MP_MEMORY;
    }
    if (auto res = mp_int_init_value(z, b); res != MP_OK) {
        mp_int_free(z);
        return res;
    }
    if (auto res = mp_int_floordiv(z, a, c); res != MP_OK) {
        mp_int_free(z);
        return res;
    }
    mp_int_free(z);
    return MP_OK;
}

[[nodiscard]] auto mp_expt_int_value_inv(mp_int a, mp_small b, mp_int c) {
    if (a != c) {
        if (auto res = mp_int_set_value(c, b)) {
            return res;
        }
        if (auto res = mp_int_expt_full(c, a, c)) {
            return res;
        }
    } else {
        mp_int z = mp_int_alloc();
        if (z == nullptr) {
            return MP_MEMORY;
        }
        if (auto res = mp_int_init_value(z, b); res != MP_OK) {
            mp_int_free(z);
            return res;
        }
        if (auto res = mp_int_expt_full(z, a, c); res != MP_OK) {
            mp_int_free(z);
            return res;
        }
        mp_int_free(z);
    }
    return MP_OK;
}

} // namespace

class Number::Impl {
  public:
    using OpCheck = std::optional<int32_t>(int32_t, int32_t);
    using Op = mp_result(mp_int, mp_int, mp_int);
    using OpValue = mp_result(mp_int, mp_small, mp_int);

    static auto op_binary(Op op, OpValue op_value, OpValue op_value_inv, OpCheck op_check, Number const &a,
                          Number const &b) -> Number {
        // op(int, int)
        if (repr_is_int(a.repr_) && repr_is_int(b.repr_)) {
            if (auto c = op_check(repr_to_int(a.repr_), repr_to_int(b.repr_)); c) {
                return {*c};
            }
            mp_int_ptr z;
            handle_error(mp_int_init_value(z, repr_to_int(a.repr_)));
            handle_error(op_value(z, repr_to_int(b.repr_), z));
            return {z.release_repr(true)};
        }
        mp_int_ptr z;
        // op(int, big)
        if (repr_is_int(a.repr_)) {
            handle_error(op_value_inv(repr_to_bigint(b.repr_), repr_to_int(a.repr_), z));
        }
        // op(big, int)
        else if (repr_is_int(b.repr_)) {
            handle_error(op_value(repr_to_bigint(a.repr_), repr_to_int(b.repr_), z));
        }
        // op(big, big)
        else {
            handle_error(op(repr_to_bigint(a.repr_), repr_to_bigint(b.repr_), z));
        }
        return z.release_repr();
    }

    static auto op_binary(Op op, OpValue op_value, OpValue op_value_inv, OpCheck op_check, Number &&a, Number const &b)
        -> Number {
        if (!repr_is_int(a.repr_)) {
            // op(big, int)
            if (repr_is_int(b.repr_)) {
                handle_error(op_value(repr_to_bigint(a.repr_), repr_to_int(b.repr_), repr_to_bigint(a.repr_)));
            }
            // op(big, big)
            else {
                handle_error(op(repr_to_bigint(a.repr_), repr_to_bigint(b.repr_), repr_to_bigint(a.repr_)));
            }
            return std::move(a);
        }
        return op_binary(op, op_value, op_value_inv, op_check, a, b);
    }

    static auto op_binary(Op op, OpValue op_value, OpValue op_value_inv, OpCheck op_check, Number const &a, Number &&b)
        -> Number {
        if (!repr_is_int(b.repr_)) {
            // op(int, big)
            if (repr_is_int(a.repr_)) {
                handle_error(op_value_inv(repr_to_bigint(b.repr_), repr_to_int(a.repr_), repr_to_bigint(b.repr_)));
            }
            // op(big, big)
            else {
                handle_error(op(repr_to_bigint(a.repr_), repr_to_bigint(b.repr_), repr_to_bigint(b.repr_)));
            }
            return std::move(b);
        }
        return op_binary(op, op_value, op_value_inv, op_check, a, b);
    }

    static auto op_binary(Op op, OpValue op_value, OpValue op_value_inv, OpCheck op_check, Number &&a, Number &&b)
        -> Number {
        // op(big, *)
        if (!repr_is_int(a.repr_)) {
            return op_binary(op, op_value, op_value_inv, op_check, std::move(a), b);
        }
        // op(*, big)
        if (!repr_is_int(b.repr_)) {
            return op_binary(op, op_value, op_value_inv, op_check, a, std::move(b));
        }
        return op_binary(op, op_value, op_value_inv, op_check, a, b);
    }

    static auto op_assign(Op op, OpValue op_value, OpValue op_value_inv, OpCheck op_check, Number &a, Number const &b)
        -> Number & {
        // op(int, int)
        if (repr_is_int(a.repr_) && repr_is_int(b.repr_)) {
            if (auto c = op_check(repr_to_int(a.repr_), repr_to_int(b.repr_)); c) {
                a.repr_ = int_to_repr(c.value());
                return a;
            }
            mp_int_ptr z;
            handle_error(mp_int_init_value(z, repr_to_int(a.repr_)));
            handle_error(op_value(z, repr_to_int(a.repr_), z));
            a.repr_ = z.release_repr(true);
        }
        // op(int, big)
        else if (repr_is_int(a.repr_)) {
            mp_int_ptr z;
            handle_error(op_value_inv(repr_to_bigint(b.repr_), repr_to_int(a.repr_), z));
            a.repr_ = z.release_repr();
        }
        // op(big, int)
        else if (repr_is_int(b.repr_)) {
            handle_error(op_value(repr_to_bigint(a.repr_), repr_to_int(b.repr_), repr_to_bigint(a.repr_)));
        }
        // op(big, big)
        else {
            handle_error(op(repr_to_bigint(a.repr_), repr_to_bigint(b.repr_), repr_to_bigint(a.repr_)));
        }
        return a;
    }

    static auto op_assign(Op op, OpValue op_value, OpValue op_value_inv, OpCheck op_check, Number &a, Number &&b)
        -> Number & {
        // op(int, big)
        if (repr_is_int(a.repr_) && !repr_is_int(b.repr_)) {
            std::swap(a.repr_, b.repr_);
            handle_error(op_value_inv(repr_to_bigint(a.repr_), repr_to_int(b.repr_), repr_to_bigint(a.repr_)));
            return a;
        }
        return op_assign(op, op_value, op_value_inv, op_check, a, b);
    }

    static auto cmp(int32_t a, int32_t b) -> int {
        if (a < b) {
            return -1;
        }
        if (a > b) {
            return 1;
        }
        return 0;
    }

    static auto cmp(Number const &a, int32_t b) -> int {
        // int == int
        if (repr_is_int(a.repr_)) {
            return cmp(repr_to_int(a.repr_), b);
        }
        // int == big
        return mp_int_compare_value(repr_to_bigint(a.repr_), b);
    }

    static auto cmp(int32_t a, Number const &b) -> int {
        // int == int
        if (repr_is_int(b.repr_)) {
            return cmp(a, repr_to_int(b.repr_));
        }
        // int == big
        return -mp_int_compare_value(repr_to_bigint(b.repr_), a);
    }

    static auto cmp(Number const &a, Number const &b) -> int {
        // int == int
        if (repr_is_int(a.repr_) && repr_is_int(b.repr_)) {
            return cmp(repr_to_int(a.repr_), repr_to_int(b.repr_));
        }
        // int == big
        if (repr_is_int(a.repr_)) {
            return mp_int_compare_value(repr_to_bigint(b.repr_), repr_to_int(a.repr_));
        }
        // big == int
        if (repr_is_int(b.repr_)) {
            return mp_int_compare_value(repr_to_bigint(a.repr_), repr_to_int(b.repr_));
        }
        // big == big
        return mp_int_compare(repr_to_bigint(a.repr_), repr_to_bigint(b.repr_));
    }
};

Number::Number(uint64_t repr) : repr_{repr} {}

Number::Number(int32_t value) noexcept : repr_{int_to_repr(value)} {}

Number::Number(char const *str) : repr_{0} {
    mp_int_ptr z;
    auto res = mp_int_read_string(z, BASE, str);
    if (res != MP_OK) {
        throw std::runtime_error(mp_error_string(res));
    }
    repr_ = bigint_to_repr(z.release());
}

Number::Number(Number const &other) : repr_{0} { *this = other; }

Number::Number(Number &&other) noexcept : repr_{0} { *this = std::move(other); }

auto Number::operator=(Number const &other) -> Number & {
    // noop
    if (this == &other) {
    }
    // int = int
    else if (repr_is_int(repr_) && repr_is_int(other.repr_)) {
        repr_ = other.repr_;
    }
    // int = big
    else if (repr_is_int(repr_)) {
        mp_int_ptr z;
        handle_error(mp_int_init_copy(z, repr_to_bigint(other.repr_)));
        repr_ = z.release_repr();
    }
    // big = int
    else if (repr_is_int(other.repr_)) {
        handle_error(mp_int_set_value(repr_to_bigint(repr_), repr_to_int(other.repr_)));
    }
    // big = big
    else if (repr_is_bigint(other.repr_)) {
        handle_error(mp_int_copy(repr_to_bigint(repr_), repr_to_bigint(other.repr_)));
    }
    return *this;
}

auto Number::operator=(Number &&other) noexcept -> Number & {
    std::swap(repr_, other.repr_);
    return *this;
}

Number::~Number() noexcept {
    if (repr_is_bigint(repr_)) {
        mp_int_free(repr_to_bigint(repr_));
    }
}

[[nodiscard]] auto Number::as_int() const -> std::optional<int32_t> {
    if (repr_is_int(repr_)) {
        return repr_to_int(repr_);
    }
    return std::nullopt;
}

[[nodiscard]] auto Number::as_string() const -> std::string {
    if (repr_is_int(repr_)) {
        return std::to_string(repr_to_int(repr_));
    }
    std::string ret;
    auto *z = repr_to_bigint(repr_);
    auto len = mp_int_string_len(z, BASE);
    ret.resize(len - 1, '0');
    handle_error(mp_int_to_string(z, BASE, ret.data(), len - 1), MP_TRUNC);
    return ret;
}

auto operator==(Number const &a, int32_t b) -> bool { return Number::Impl::cmp(a, b) == 0; }

auto operator==(Number const &a, Number const &b) -> bool { return Number::Impl::cmp(a, b) == 0; }

auto operator==(int32_t a, Number const &b) -> bool { return Number::Impl::cmp(a, b) == 0; }

auto operator!=(Number const &a, int32_t b) -> bool { return Number::Impl::cmp(a, b) != 0; }

auto operator!=(Number const &a, Number const &b) -> bool { return Number::Impl::cmp(a, b) != 0; }

auto operator!=(int32_t a, Number const &b) -> bool { return Number::Impl::cmp(a, b) != 0; }

auto operator<(Number const &a, int32_t b) -> bool { return Number::Impl::cmp(a, b) < 0; }

auto operator<(Number const &a, Number const &b) -> bool { return Number::Impl::cmp(a, b) < 0; }

auto operator<(int32_t a, Number const &b) -> bool { return Number::Impl::cmp(a, b) < 0; }

auto operator<=(Number const &a, int32_t b) -> bool { return Number::Impl::cmp(a, b) <= 0; }

auto operator<=(Number const &a, Number const &b) -> bool { return Number::Impl::cmp(a, b) <= 0; }

auto operator<=(int32_t a, Number const &b) -> bool { return Number::Impl::cmp(a, b) <= 0; }

auto operator>(Number const &a, int32_t b) -> bool { return Number::Impl::cmp(a, b) > 0; }

auto operator>(Number const &a, Number const &b) -> bool { return Number::Impl::cmp(a, b) > 0; }

auto operator>(int32_t a, Number const &b) -> bool { return Number::Impl::cmp(a, b) > 0; }

auto operator>=(Number const &a, int32_t b) -> bool { return Number::Impl::cmp(a, b) >= 0; }

auto operator>=(Number const &a, Number const &b) -> bool { return Number::Impl::cmp(a, b) >= 0; }

auto operator>=(int32_t a, Number const &b) -> bool { return Number::Impl::cmp(a, b) >= 0; }

// addition

auto operator+(Number const &a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_add, mp_int_add_value, mp_int_add_value, check_add, a, b);
}

auto operator+(Number &&a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_add, mp_int_add_value, mp_int_add_value, check_add, std::move(a), b);
}

auto operator+(Number const &a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_add, mp_int_add_value, mp_int_add_value, check_add, a, std::move(b));
}

auto operator+(Number &&a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_add, mp_int_add_value, mp_int_add_value, check_add, std::move(a),
                                   std::move(b));
}

auto operator+=(Number &a, Number const &b) -> Number & {
    return Number::Impl::op_assign(mp_int_add, mp_int_add_value, mp_int_add_value, check_add, a, b);
}

auto operator+=(Number &a, Number &&b) -> Number & {
    return Number::Impl::op_assign(mp_int_add, mp_int_add_value, mp_int_add_value, check_add, a, std::move(b));
}

// subtraction

auto operator-(Number const &a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_sub, mp_int_sub_value, mp_int_sub_value_inv, check_sub, std::move(a),
                                   std::move(b));
}

auto operator-(Number &&a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_sub, mp_int_sub_value, mp_int_sub_value_inv, check_sub, std::move(a), b);
}

auto operator-(Number const &a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_sub, mp_int_sub_value, mp_int_sub_value_inv, check_sub, a, std::move(b));
}

auto operator-(Number &&a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_sub, mp_int_sub_value, mp_int_sub_value_inv, check_sub, std::move(a),
                                   std::move(b));
}

auto operator-=(Number &a, Number const &b) -> Number & {
    return Number::Impl::op_assign(mp_int_add, mp_int_sub_value, mp_int_sub_value_inv, check_add, a, b);
}

auto operator-=(Number &a, Number &&b) -> Number & {
    return Number::Impl::op_assign(mp_int_add, mp_int_sub_value, mp_int_sub_value_inv, check_add, a, std::move(b));
}

// multiplication

auto operator*(Number const &a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_mul, mp_int_mul_value, mp_int_mul_value, check_mul, a, b);
}

auto operator*(Number &&a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_mul, mp_int_mul_value, mp_int_mul_value, check_mul, std::move(a), b);
}

auto operator*(Number const &a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_mul, mp_int_mul_value, mp_int_mul_value, check_mul, a, std::move(b));
}

auto operator*(Number &&a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_mul, mp_int_mul_value, mp_int_mul_value, check_mul, std::move(a),
                                   std::move(b));
}

auto operator*=(Number &a, Number const &b) -> Number & {
    return Number::Impl::op_assign(mp_int_mul, mp_int_mul_value, mp_int_mul_value, check_mul, a, b);
}

auto operator*=(Number &a, Number &&b) -> Number & {
    return Number::Impl::op_assign(mp_int_mul, mp_int_mul_value, mp_int_mul_value, check_mul, a, std::move(b));
}

// division

auto operator/(Number const &a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_floordiv, mp_int_floordiv_value, mp_int_floordiv_value_inv, check_div, a, b);
}

auto operator/(Number &&a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_floordiv, mp_int_floordiv_value, mp_int_floordiv_value_inv, check_div,
                                   std::move(a), b);
}

auto operator/(Number const &a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_floordiv, mp_int_floordiv_value, mp_int_floordiv_value_inv, check_div, a,
                                   std::move(b));
}

auto operator/(Number &&a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_floordiv, mp_int_floordiv_value, mp_int_floordiv_value_inv, check_div,
                                   std::move(a), std::move(b));
}

auto operator/=(Number &a, Number const &b) -> Number & {
    return Number::Impl::op_assign(mp_int_floordiv, mp_int_floordiv_value, mp_int_floordiv_value_inv, check_div, a, b);
}

auto operator/=(Number &a, Number &&b) -> Number & {
    return Number::Impl::op_assign(mp_int_floordiv, mp_int_floordiv_value, mp_int_floordiv_value_inv, check_div, a,
                                   std::move(b));
}

auto operator-(Number const &a) -> Number {
    bool is_int = repr_is_int(a.repr_);
    if (is_int) {
        if (auto res = check_neg(repr_to_int(a.repr_)); res.has_value()) {
            return {res.value()};
        }
    }
    mp_int_ptr z;
    if (is_int) {
        handle_error(mp_int_init_value(z, repr_to_int(a.repr_)));
    } else {
        handle_error(mp_int_init_copy(z, repr_to_bigint(a.repr_)));
    }
    handle_error(mp_int_neg(z, z));
    return {z.release_repr(is_int)};
}

auto operator-(Number &&a) -> Number {
    if (repr_is_int(a.repr_)) {
        return -a;
    }
    handle_error(mp_int_neg(repr_to_bigint(a.repr_), repr_to_bigint(a.repr_)));
    return std::move(a);
}

auto operator~(Number const &a) -> Number {
    if (repr_is_int(a.repr_)) {
        return {~repr_to_int(a.repr_)};
    }
    mp_int_ptr z;
    handle_error(mp_int_neg(repr_to_bigint(a.repr_), z));
    handle_error(mp_int_sub_value(z, 1, z));
    return {z.release_repr()};
}

auto operator~(Number &&a) -> Number {
    if (repr_is_int(a.repr_)) {
        return {~repr_to_int(a.repr_)};
    }
    auto *z = repr_to_bigint(a.repr_);
    handle_error(mp_int_neg(z, z));
    handle_error(mp_int_sub_value(z, 1, z));
    return std::move(a);
}

// exponentiation

auto pow(Number const &a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_expt_full, mp_int_expt, mp_expt_int_value_inv, check_pow, a, b);
}

auto pow(Number &&a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_expt_full, mp_int_expt, mp_expt_int_value_inv, check_pow, std::move(a), b);
}

auto pow(Number const &a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_expt_full, mp_int_expt, mp_expt_int_value_inv, check_pow, a, std::move(b));
}

auto pow(Number &&a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_expt_full, mp_int_expt, mp_expt_int_value_inv, check_pow, std::move(a),
                                   std::move(b));
}

auto abs(Number const &a) -> Number {
    bool is_int = repr_is_int(a.repr_);
    if (is_int) {
        if (auto res = check_abs(repr_to_int(a.repr_))) {
            return {res.value()};
        }
    }
    mp_int_ptr z;
    if (is_int) {
        handle_error(mp_int_init_value(z, repr_to_int(a.repr_)));
        handle_error(mp_int_abs(z, z));
    } else {
        handle_error(mp_int_abs(repr_to_bigint(a.repr_), z));
    }
    return {z.release_repr(is_int)};
}

auto abs(Number &&a) -> Number {
    if (repr_is_int(a.repr_)) {
        return abs(a);
    }
    handle_error(mp_int_abs(repr_to_bigint(a.repr_), repr_to_bigint(a.repr_)));
    return std::move(a);
}

} // namespace Gringo
