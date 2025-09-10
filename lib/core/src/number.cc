#include <clingo/core/number.hh>

#include <clingo/util/checked_math.hh>
#include <clingo/util/hash.hh>

#include <imath.h>

#include <atomic>
#include <memory>

namespace CppClingo {

namespace {

constexpr int BASE = 10;
constexpr uint64_t BIGINT_MASK = 7;

// NOLINTBEGIN(readability-magic-numbers,cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr,cppcoreguidelines-avoid-c-arrays,cppcoreguidelines-pro-bounds-pointer-arithmetic)

struct mpz_ref_t {
    mpz_t num;
    std::atomic_size_t ref_count = 0;
};
using mp_int_ref = mpz_ref_t *;

auto repr_is_int(uint64_t repr) -> bool {
    return (repr & BIGINT_MASK) == 0;
}

auto repr_is_bigint(uint64_t repr) -> bool {
    return (repr & BIGINT_MASK) == BIGINT_MASK;
}

auto repr_to_int(uint64_t repr) -> int32_t {
    return static_cast<int>(repr >> 32);
}

auto repr_to_bigint(uint64_t repr) -> mp_int_ref {
    return reinterpret_cast<mp_int_ref>(static_cast<uintptr_t>(repr & ~BIGINT_MASK));
}

auto int_to_repr(int32_t num) -> uint64_t {
    return static_cast<uint64_t>(num) << 32;
}

auto mp_int_ref_alloc() -> mp_int_ref {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
    auto *out = static_cast<mp_int_ref>(malloc(sizeof(mpz_ref_t)));

    if (out != nullptr) {
        mp_int_init(&out->num);
        new (&out->ref_count) std::atomic_size_t{0};
    }

    return out;
}

void mp_int_ref_free(mp_int_ref a) {
    mp_int_clear(&a->num);
    std::destroy_at(&a->ref_count);
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory,cppcoreguidelines-no-malloc)
    free(a);
}

auto bigint_to_repr(mp_int_ref a, bool fast = false) -> uint64_t {
    if (mp_small inum = 0; !fast && mp_int_to_int(&a->num, &inum) == MP_OK && Util::check_cast<int32_t>(inum)) {
        mp_int_ref_free(a);
        return int_to_repr(static_cast<int32_t>(inum));
    }
    return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(a)) | BIGINT_MASK;
}

void handle_error(mp_result res) {
    if (res == MP_MEMORY) {
        throw std::bad_alloc();
    }
    if (res != MP_OK) {
        throw std::runtime_error(mp_error_string(res));
    }
}

class mp_int_ptr {
  public:
    mp_int_ptr() : ptr_{mp_int_ref_alloc()} {
        if (ptr_ == nullptr) {
            throw std::bad_alloc();
        }
    }
    mp_int_ptr(mp_int_ptr &&) noexcept = delete;
    ~mp_int_ptr() {
        if (ptr_ != nullptr) {
            mp_int_ref_free(ptr_);
        }
    }
    auto release() -> mp_int_ref {
        auto *ptr = ptr_;
        ptr_ = nullptr;
        return ptr;
    }
    auto release_repr(bool fast = false) -> uint64_t { return bigint_to_repr(release(), fast); }
    operator mp_int() const { return &ptr_->num; }

  private:
    mp_int_ref ptr_;
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

[[nodiscard]] auto mp_int_floormod(mp_int a, mp_int b, mp_int c) -> mp_result {
    auto zb = mp_int_compare_zero(b);
    if (zb == 0) {
        return MP_RANGE;
    }
    mp_int d = b;
    // we have to make a copy of b for later
    if (c == b) {
        d = mp_int_alloc();
        if (d == nullptr) {
            return MP_MEMORY;
        }
        if (auto res = mp_int_init_copy(d, b); res != MP_OK) {
            mp_int_free(d);
            return res;
        }
    }
    if (auto res = mp_int_div(a, d, nullptr, c); res != MP_OK) {
        if (c == b) {
            mp_int_free(d);
        }
        return res;
    }
    auto zr = mp_int_compare_zero(c);
    if ((zr > 0 && zb < 0) || (zr < 0 && zb > 0)) {
        if (auto res = mp_int_add(c, d, c); res != MP_OK) {
            if (c == b) {
                mp_int_free(d);
            }
            return res;
        }
    }
    if (c == b) {
        mp_int_free(d);
    }
    return MP_OK;
}

[[nodiscard]] auto mp_int_floormod_value(mp_int a, mp_small b, mp_int c) -> mp_result {
    if (b == 0) {
        return MP_RANGE;
    }
    mp_small rem = 0;
    if (auto res = mp_int_div_value(a, b, nullptr, &rem); res != MP_OK) {
        return res;
    }
    if ((rem > 0 && b < 0) || (rem < 0 && b > 0)) {
        rem = b + rem;
    }
    return mp_int_set_value(c, rem);
}

[[nodiscard]] auto mp_int_floormod_value_inv(mp_int a, mp_small b, mp_int c) -> mp_result {
    if (a != c) {
        if (auto res = mp_int_set_value(c, b)) {
            return res;
        }
        return mp_int_floormod(c, a, c);
    }
    mp_int z = mp_int_alloc();
    if (z == nullptr) {
        return MP_MEMORY;
    }
    if (auto res = mp_int_init_value(z, b); res != MP_OK) {
        mp_int_free(z);
        return res;
    }
    if (auto res = mp_int_floormod(z, a, c); res != MP_OK) {
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

// NOLINTBEGIN(modernize-avoid-c-arrays,readability-magic-numbers)

// TODO: make a pull request and implement this directly on the representation of the number

auto to_binary(mp_int z, int len, int limit) -> std::unique_ptr<unsigned char[]> {
    auto msb = limit - len;
    auto buf = std::make_unique<unsigned char[]>(limit);
    mp_int_to_binary(z, buf.get() + msb, len);
    if (len < limit && (buf[msb] & 128) == 128) {
        for (int i = 0; i < msb; ++i) {
            buf[i] = 255;
        }
    }
    return buf;
}

// NOLINTEND(modernize-avoid-c-arrays,readability-magic-numbers)

template <char op> auto mp_int_binop(mp_int a, mp_int b, mp_int c) -> mp_result {
    try {
        auto len_a = mp_int_binary_len(a);
        auto len_b = mp_int_binary_len(b);
        auto limit = std::max(len_a, len_b);
        auto buf_a = to_binary(a, len_a, limit);
        auto buf_b = to_binary(b, len_b, limit);
        for (int i = 0; i < limit; ++i) {
            if constexpr (op == '&') {
                buf_a[i] = buf_a[i] & buf_b[i];
            }
            if constexpr (op == '|') {
                buf_a[i] = buf_a[i] | buf_b[i];
            }
            if constexpr (op == '^') {
                buf_a[i] = buf_a[i] ^ buf_b[i];
            }
        }
        return mp_int_read_binary(c, buf_a.get(), limit);
    } catch (std::bad_alloc const &) {
        return MP_MEMORY;
    }
}

template <char op> auto mp_int_binop_value(mp_int a, mp_small b, mp_int c) -> mp_result {
    try {
        auto len_a = mp_int_binary_len(a);
        auto len_b = static_cast<mp_result>(sizeof(mp_small));
        auto limit = std::max(len_a, len_b);
        auto buf_a = to_binary(a, len_a, limit);
        auto *buf_b = reinterpret_cast<unsigned char *>(&b);
        for (int i = 0; i < limit; ++i) {
            auto j = limit - i - 1;
            unsigned char char_b = 0;
            if (j < len_b) {
                char_b = buf_b[j];
            } else if (b < 0) {
                char_b = 255; // NOLINT(readability-magic-numbers)
            }
            if constexpr (op == '&') {
                buf_a[i] = buf_a[i] & char_b;
            }
            if constexpr (op == '|') {
                buf_a[i] = buf_a[i] | char_b;
            }
            if constexpr (op == '^') {
                buf_a[i] = buf_a[i] ^ char_b;
            }
        }
        return mp_int_read_binary(c, buf_a.get(), limit);
    } catch (std::bad_alloc const &) {
        return MP_MEMORY;
    }
}

template <char op> auto check_binop(int32_t a, int32_t b) -> std::optional<int32_t> {
    if constexpr (op == '&') {
        return a & b;
    }
    if constexpr (op == '|') {
        return a | b;
    }
    if constexpr (op == '^') {
        return a ^ b;
    }
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
            handle_error(op_value_inv(&repr_to_bigint(b.repr_)->num, repr_to_int(a.repr_), z));
        }
        // op(big, int)
        else if (repr_is_int(b.repr_)) {
            handle_error(op_value(&repr_to_bigint(a.repr_)->num, repr_to_int(b.repr_), z));
        }
        // op(big, big)
        else {
            handle_error(op(&repr_to_bigint(a.repr_)->num, &repr_to_bigint(b.repr_)->num, z));
        }
        return z.release_repr();
    }

    static auto op_binary(Op op, OpValue op_value, OpValue op_value_inv, OpCheck op_check, Number &&a, Number const &b)
        -> Number {
        if (!repr_is_int(a.repr_)) {
            auto *int_a = repr_to_bigint(a.repr_);
            // op(big, int)
            if (repr_is_int(b.repr_)) {
                handle_error(op_value(&int_a->num, repr_to_int(b.repr_), &int_a->num));
            }
            // op(big, big)
            else {
                handle_error(op(&int_a->num, &repr_to_bigint(b.repr_)->num, &int_a->num));
            }
            a.repr_ = bigint_to_repr(int_a);
            return std::move(a);
        }
        return op_binary(op, op_value, op_value_inv, op_check, a, b);
    }

    static auto op_binary(Op op, OpValue op_value, OpValue op_value_inv, OpCheck op_check, Number const &a, Number &&b)
        -> Number {
        if (!repr_is_int(b.repr_)) {
            auto *int_b = repr_to_bigint(b.repr_);
            // op(int, big)
            if (repr_is_int(a.repr_)) {
                handle_error(op_value_inv(&int_b->num, repr_to_int(a.repr_), &int_b->num));
            }
            // op(big, big)
            else {
                handle_error(op(&repr_to_bigint(a.repr_)->num, &int_b->num, &int_b->num));
            }
            b.repr_ = bigint_to_repr(int_b);
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
        if (repr_is_int(a.repr_)) {
            // op(int, int)
            if (repr_is_int(b.repr_)) {
                if (auto c = op_check(repr_to_int(a.repr_), repr_to_int(b.repr_)); c) {
                    a.repr_ = int_to_repr(c.value());
                    return a;
                }
                mp_int_ptr z;
                handle_error(mp_int_init_value(z, repr_to_int(a.repr_)));
                handle_error(op_value(z, repr_to_int(b.repr_), z));
                a.repr_ = z.release_repr(true);
            }
            // op(int, big)
            else if (repr_is_int(a.repr_)) {
                mp_int_ptr z;
                handle_error(op_value_inv(&repr_to_bigint(b.repr_)->num, repr_to_int(a.repr_), z));
                a.repr_ = z.release_repr();
            }
        } else {
            auto *int_a = repr_to_bigint(a.repr_);
            // op(big, int)
            if (repr_is_int(b.repr_)) {
                handle_error(op_value(&int_a->num, repr_to_int(b.repr_), &int_a->num));
            }
            // op(big, big)
            else {
                handle_error(op(&int_a->num, &repr_to_bigint(b.repr_)->num, &int_a->num));
            }
            a.repr_ = bigint_to_repr(int_a);
        }
        return a;
    }

    // NOLINTBEGIN(cppcoreguidelines-rvalue-reference-param-not-moved)
    static auto op_assign(Op op, OpValue op_value, OpValue op_value_inv, OpCheck op_check, Number &a, Number &&b)
        -> Number & {
        // op(int, big)
        if (repr_is_int(a.repr_) && !repr_is_int(b.repr_)) {
            std::swap(a.repr_, b.repr_);
            auto *int_a = repr_to_bigint(a.repr_);
            handle_error(op_value_inv(&int_a->num, repr_to_int(b.repr_), &int_a->num));
            a.repr_ = bigint_to_repr(int_a);
            return a;
        }
        return op_assign(op, op_value, op_value_inv, op_check, a, b);
    }
    // NOLINTEND(cppcoreguidelines-rvalue-reference-param-not-moved)

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
        return mp_int_compare_value(&repr_to_bigint(a.repr_)->num, b);
    }

    static auto cmp(int32_t a, Number const &b) -> int {
        // int == int
        if (repr_is_int(b.repr_)) {
            return cmp(a, repr_to_int(b.repr_));
        }
        // int == big
        return -mp_int_compare_value(&repr_to_bigint(b.repr_)->num, a);
    }

    static auto cmp(Number const &a, Number const &b) -> int {
        // int == int
        if (repr_is_int(a.repr_) && repr_is_int(b.repr_)) {
            return cmp(repr_to_int(a.repr_), repr_to_int(b.repr_));
        }
        // int == big
        if (repr_is_int(a.repr_)) {
            return mp_int_compare_value(&repr_to_bigint(b.repr_)->num, repr_to_int(a.repr_));
        }
        // big == int
        if (repr_is_int(b.repr_)) {
            return mp_int_compare_value(&repr_to_bigint(a.repr_)->num, repr_to_int(b.repr_));
        }
        // big == big
        return mp_int_compare(&repr_to_bigint(a.repr_)->num, &repr_to_bigint(b.repr_)->num);
    }
};

Number::Number(uint64_t repr) : repr_{repr} {
}

Number::Number(int32_t value) noexcept : repr_{int_to_repr(value)} {
}

namespace {

auto parse_big(char const *str, Base base) {
    mp_int_ptr z;
    auto res = mp_int_read_string(z, static_cast<mp_size>(base), str);
    if (res != MP_OK) {
        throw std::runtime_error(mp_error_string(res));
    }
    return z.release_repr();
}

auto parse_small(char const *str, char const *end, int32_t &num, Base base) {
    auto [ptr, ec] = std::from_chars(str, end, num, static_cast<int>(base));
    return ec == std::errc() && ptr == end;
}

auto parse_num(char const *str, Base base) {
    int32_t num = 0;
    // NOLINTBEGIN
    auto const *end = str + strlen(str);
    if (parse_small(str, end, num, base)) {
        return int_to_repr(num);
    }
    // NOLINTEND
    return parse_big(std::string{str}.c_str(), base);
}

auto parse_num(std::string_view str, Base base) {
    int32_t num = 0;
    // NOLINTBEGIN
    auto const *end = str.data() + str.size();
    if (parse_small(str.data(), end, num, base)) {
        return int_to_repr(num);
    }
    // NOLINTEND
    return parse_big(std::string{str}.c_str(), base);
}

} // namespace

Number::Number(char const *str, Base base) : repr_{parse_num(str, base)} {
}

Number::Number(std::string_view str, Base base) : repr_{parse_num(str, base)} {
}

Number::Number(Number const &other) : repr_{0} {
    *this = other;
}

Number::Number(Number &&other) noexcept : repr_{0} {
    *this = std::move(other);
}

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
        handle_error(mp_int_init_copy(z, &repr_to_bigint(other.repr_)->num));
        repr_ = z.release_repr();
    }
    // big = int
    else if (repr_is_int(other.repr_)) {
        handle_error(mp_int_set_value(&repr_to_bigint(repr_)->num, repr_to_int(other.repr_)));
    }
    // big = big
    else if (repr_is_bigint(other.repr_)) {
        handle_error(mp_int_copy(&repr_to_bigint(repr_)->num, &repr_to_bigint(other.repr_)->num));
    }
    return *this;
}

auto Number::operator=(Number &&other) noexcept -> Number & {
    std::swap(repr_, other.repr_);
    return *this;
}

Number::~Number() noexcept {
    if (repr_is_bigint(repr_)) {
        mp_int_ref_free(repr_to_bigint(repr_));
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
    auto len = mp_int_string_len(&z->num, BASE);
    ret.resize(len, '\0');
    handle_error(mp_int_to_string(&z->num, BASE, ret.data(), len));
    // NOTE: The mp_int library sometimes reports too large length for numbers,
    // which requires the while loop below.
    while (!ret.empty() && ret.back() == '\0') {
        ret.pop_back();
    }
    return ret;
}

void Number::swap(Number &other) noexcept {
    std::swap(repr_, other.repr_);
}

auto compare(Number const &a, Number const &b) -> int {
    return Number::Impl::cmp(a, b);
}

auto compare(int32_t a, Number const &b) -> int {
    return Number::Impl::cmp(a, b);
}

auto compare(Number const &a, int32_t b) -> int {
    return Number::Impl::cmp(a, b);
}

// addition

auto operator+(Number const &a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_add, mp_int_add_value, mp_int_add_value, Util::check_add, a, b);
}

auto operator+(Number &&a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_add, mp_int_add_value, mp_int_add_value, Util::check_add, std::move(a), b);
}

auto operator+(Number const &a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_add, mp_int_add_value, mp_int_add_value, Util::check_add, a, std::move(b));
}

auto operator+(Number &&a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_add, mp_int_add_value, mp_int_add_value, Util::check_add, std::move(a),
                                   std::move(b));
}

auto operator+=(Number &a, Number const &b) -> Number & {
    return Number::Impl::op_assign(mp_int_add, mp_int_add_value, mp_int_add_value, Util::check_add, a, b);
}

auto operator+=(Number &a, Number &&b) -> Number & {
    return Number::Impl::op_assign(mp_int_add, mp_int_add_value, mp_int_add_value, Util::check_add, a, std::move(b));
}

// subtraction

auto operator-(Number const &a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_sub, mp_int_sub_value, mp_int_sub_value_inv, Util::check_sub, a, b);
}

auto operator-(Number &&a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_sub, mp_int_sub_value, mp_int_sub_value_inv, Util::check_sub, std::move(a),
                                   b);
}

auto operator-(Number const &a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_sub, mp_int_sub_value, mp_int_sub_value_inv, Util::check_sub, a,
                                   std::move(b));
}

auto operator-(Number &&a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_sub, mp_int_sub_value, mp_int_sub_value_inv, Util::check_sub, std::move(a),
                                   std::move(b));
}

auto operator-=(Number &a, Number const &b) -> Number & {
    return Number::Impl::op_assign(mp_int_sub, mp_int_sub_value, mp_int_sub_value_inv, Util::check_sub, a, b);
}

auto operator-=(Number &a, Number &&b) -> Number & {
    return Number::Impl::op_assign(mp_int_sub, mp_int_sub_value, mp_int_sub_value_inv, Util::check_sub, a,
                                   std::move(b));
}

// multiplication

auto operator*(Number const &a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_mul, mp_int_mul_value, mp_int_mul_value, Util::check_mul, a, b);
}

auto operator*(Number &&a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_mul, mp_int_mul_value, mp_int_mul_value, Util::check_mul, std::move(a), b);
}

auto operator*(Number const &a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_mul, mp_int_mul_value, mp_int_mul_value, Util::check_mul, a, std::move(b));
}

auto operator*(Number &&a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_mul, mp_int_mul_value, mp_int_mul_value, Util::check_mul, std::move(a),
                                   std::move(b));
}

auto operator*=(Number &a, Number const &b) -> Number & {
    return Number::Impl::op_assign(mp_int_mul, mp_int_mul_value, mp_int_mul_value, Util::check_mul, a, b);
}

auto operator*=(Number &a, Number &&b) -> Number & {
    return Number::Impl::op_assign(mp_int_mul, mp_int_mul_value, mp_int_mul_value, Util::check_mul, a, std::move(b));
}

// division

auto operator/(Number const &a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_floordiv, mp_int_floordiv_value, mp_int_floordiv_value_inv, Util::check_div,
                                   a, b);
}

auto operator/(Number &&a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_floordiv, mp_int_floordiv_value, mp_int_floordiv_value_inv, Util::check_div,
                                   std::move(a), b);
}

auto operator/(Number const &a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_floordiv, mp_int_floordiv_value, mp_int_floordiv_value_inv, Util::check_div,
                                   a, std::move(b));
}

auto operator/(Number &&a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_floordiv, mp_int_floordiv_value, mp_int_floordiv_value_inv, Util::check_div,
                                   std::move(a), std::move(b));
}

auto operator/=(Number &a, Number const &b) -> Number & {
    return Number::Impl::op_assign(mp_int_floordiv, mp_int_floordiv_value, mp_int_floordiv_value_inv, Util::check_div,
                                   a, b);
}

auto operator/=(Number &a, Number &&b) -> Number & {
    return Number::Impl::op_assign(mp_int_floordiv, mp_int_floordiv_value, mp_int_floordiv_value_inv, Util::check_div,
                                   a, std::move(b));
}

// modulus

auto operator%(Number const &a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_floormod, mp_int_floormod_value, mp_int_floormod_value_inv, Util::check_mod,
                                   a, b);
}

auto operator%(Number &&a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_floormod, mp_int_floormod_value, mp_int_floormod_value_inv, Util::check_mod,
                                   std::move(a), b);
}

auto operator%(Number const &a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_floormod, mp_int_floormod_value, mp_int_floormod_value_inv, Util::check_mod,
                                   a, std::move(b));
}

auto operator%(Number &&a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_floormod, mp_int_floormod_value, mp_int_floormod_value_inv, Util::check_mod,
                                   std::move(a), std::move(b));
}

auto operator%=(Number &a, Number const &b) -> Number & {
    return Number::Impl::op_assign(mp_int_floormod, mp_int_floormod_value, mp_int_floormod_value_inv, Util::check_mod,
                                   a, b);
}

auto operator%=(Number &a, Number &&b) -> Number & {
    return Number::Impl::op_assign(mp_int_floormod, mp_int_floormod_value, mp_int_floormod_value_inv, Util::check_mod,
                                   a, std::move(b));
}

// unary minus

auto operator-(Number const &a) -> Number {
    bool is_int = repr_is_int(a.repr_);
    if (is_int) {
        if (auto res = Util::check_neg(repr_to_int(a.repr_)); res.has_value()) {
            return {res.value()};
        }
    }
    mp_int_ptr z;
    if (is_int) {
        handle_error(mp_int_init_value(z, repr_to_int(a.repr_)));
    } else {
        handle_error(mp_int_init_copy(z, &repr_to_bigint(a.repr_)->num));
    }
    handle_error(mp_int_neg(z, z));
    return {z.release_repr(is_int)};
}

auto operator-(Number &&a) -> Number {
    if (repr_is_int(a.repr_)) {
        return -a;
    }
    auto *int_a = repr_to_bigint(a.repr_);
    handle_error(mp_int_neg(&int_a->num, &int_a->num));
    a.repr_ = bigint_to_repr(int_a);
    return std::move(a);
}

// complement

auto operator~(Number const &a) -> Number {
    if (repr_is_int(a.repr_)) {
        return {~repr_to_int(a.repr_)};
    }
    mp_int_ptr z;
    handle_error(mp_int_neg(&repr_to_bigint(a.repr_)->num, z));
    handle_error(mp_int_sub_value(z, 1, z));
    // Note: cannot become int32_t
    return {z.release_repr(true)};
}

auto operator~(Number &&a) -> Number {
    if (repr_is_int(a.repr_)) {
        return {~repr_to_int(a.repr_)};
    }
    auto *z = repr_to_bigint(a.repr_);
    handle_error(mp_int_neg(&z->num, &z->num));
    handle_error(mp_int_sub_value(&z->num, 1, &z->num));
    return std::move(a);
}

// binary and

auto operator&(Number const &a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_binop<'&'>, mp_int_binop_value<'&'>, mp_int_binop_value<'&'>,
                                   check_binop<'&'>, a, b);
}

auto operator&(Number &&a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_binop<'&'>, mp_int_binop_value<'&'>, mp_int_binop_value<'&'>,
                                   check_binop<'&'>, std::move(a), b);
}

auto operator&(Number const &a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_binop<'&'>, mp_int_binop_value<'&'>, mp_int_binop_value<'&'>,
                                   check_binop<'&'>, a, std::move(b));
}

auto operator&(Number &&a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_binop<'&'>, mp_int_binop_value<'&'>, mp_int_binop_value<'&'>,
                                   check_binop<'&'>, std::move(a), std::move(b));
}

auto operator&=(Number &a, Number const &b) -> Number & {
    return Number::Impl::op_assign(mp_int_binop<'&'>, mp_int_binop_value<'&'>, mp_int_binop_value<'&'>,
                                   check_binop<'&'>, a, b);
}

auto operator&=(Number &a, Number &&b) -> Number & {
    return Number::Impl::op_assign(mp_int_binop<'&'>, mp_int_binop_value<'&'>, mp_int_binop_value<'&'>,
                                   check_binop<'&'>, a, std::move(b));
}

// binary and

auto operator|(Number const &a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_binop<'|'>, mp_int_binop_value<'|'>, mp_int_binop_value<'|'>,
                                   check_binop<'|'>, a, b);
}

auto operator|(Number &&a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_binop<'|'>, mp_int_binop_value<'|'>, mp_int_binop_value<'|'>,
                                   check_binop<'|'>, std::move(a), b);
}

auto operator|(Number const &a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_binop<'|'>, mp_int_binop_value<'|'>, mp_int_binop_value<'|'>,
                                   check_binop<'|'>, a, std::move(b));
}

auto operator|(Number &&a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_binop<'|'>, mp_int_binop_value<'|'>, mp_int_binop_value<'|'>,
                                   check_binop<'|'>, std::move(a), std::move(b));
}

auto operator|=(Number &a, Number const &b) -> Number & {
    return Number::Impl::op_assign(mp_int_binop<'|'>, mp_int_binop_value<'|'>, mp_int_binop_value<'|'>,
                                   check_binop<'|'>, a, b);
}

auto operator|=(Number &a, Number &&b) -> Number & {
    return Number::Impl::op_assign(mp_int_binop<'|'>, mp_int_binop_value<'|'>, mp_int_binop_value<'|'>,
                                   check_binop<'|'>, a, std::move(b));
}

// binary and

auto operator^(Number const &a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_binop<'^'>, mp_int_binop_value<'^'>, mp_int_binop_value<'^'>,
                                   check_binop<'^'>, a, b);
}

auto operator^(Number &&a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_binop<'^'>, mp_int_binop_value<'^'>, mp_int_binop_value<'^'>,
                                   check_binop<'^'>, std::move(a), b);
}

auto operator^(Number const &a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_binop<'^'>, mp_int_binop_value<'^'>, mp_int_binop_value<'^'>,
                                   check_binop<'^'>, a, std::move(b));
}

auto operator^(Number &&a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_binop<'^'>, mp_int_binop_value<'^'>, mp_int_binop_value<'^'>,
                                   check_binop<'^'>, std::move(a), std::move(b));
}

auto operator^=(Number &a, Number const &b) -> Number & {
    return Number::Impl::op_assign(mp_int_binop<'^'>, mp_int_binop_value<'^'>, mp_int_binop_value<'^'>,
                                   check_binop<'^'>, a, b);
}

auto operator^=(Number &a, Number &&b) -> Number & {
    return Number::Impl::op_assign(mp_int_binop<'^'>, mp_int_binop_value<'^'>, mp_int_binop_value<'^'>,
                                   check_binop<'^'>, a, std::move(b));
}

// exponentiation

auto pow(Number const &a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_expt_full, mp_int_expt, mp_expt_int_value_inv, Util::check_pow, a, b);
}

auto pow(Number &&a, Number const &b) -> Number {
    return Number::Impl::op_binary(mp_int_expt_full, mp_int_expt, mp_expt_int_value_inv, Util::check_pow, std::move(a),
                                   b);
}

auto pow(Number const &a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_expt_full, mp_int_expt, mp_expt_int_value_inv, Util::check_pow, a,
                                   std::move(b));
}

auto pow(Number &&a, Number &&b) -> Number {
    return Number::Impl::op_binary(mp_int_expt_full, mp_int_expt, mp_expt_int_value_inv, Util::check_pow, std::move(a),
                                   std::move(b));
}

// absolute

auto abs(Number const &a) -> Number {
    bool is_int = repr_is_int(a.repr_);
    if (is_int) {
        if (auto res = Util::check_abs(repr_to_int(a.repr_))) {
            return {res.value()};
        }
    }
    mp_int_ptr z;
    if (is_int) {
        handle_error(mp_int_init_value(z, repr_to_int(a.repr_)));
        handle_error(mp_int_abs(z, z));
    } else {
        handle_error(mp_int_abs(&repr_to_bigint(a.repr_)->num, z));
    }
    return {z.release_repr(is_int)};
}

auto abs(Number &&a) -> Number {
    if (repr_is_int(a.repr_)) {
        return abs(a);
    }
    auto *int_a = repr_to_bigint(a.repr_);
    handle_error(mp_int_abs(&int_a->num, &int_a->num));
    // Note: cannot become int32_t
    return std::move(a);
}

auto get_sign(Number const &a) -> int {
    if (repr_is_int(a.repr_)) {
        auto num = repr_to_int(a.repr_);
        if (num > 0) {
            return 1;
        }
        if (num < 0) {
            return -1;
        }
        return 0;
    }
    return mp_int_compare_zero(&repr_to_bigint(a.repr_)->num);
}

// hash code

auto Number::hash() const noexcept -> size_t {
    if (repr_is_int(repr_)) {
        return Util::value_hash(repr_to_int(repr_));
    }
    auto *int_a = repr_to_bigint(repr_);
    size_t hash = 0;
    if (int_a->num.used == 1) {
        hash = std::hash<mp_digit>{}(int_a->num.single);
    } else {
        hash = std::hash<std::string_view>{}(
            std::string_view(reinterpret_cast<char const *>(int_a->num.digits), sizeof(mp_digit) * int_a->num.used));
    }
    return Util::hash_combine(static_cast<size_t>(int_a->num.sign), hash);
}

// output

auto operator<<(std::ostream &out, Number const &num) -> std::ostream & {
    if (repr_is_int(num.repr_)) {
        out << repr_to_int(num.repr_);
    } else {
        out << num.as_string();
    }
    return out;
}

auto operator<<(Util::OutputBuffer &out, Number const &num) -> Util::OutputBuffer & {
    if (repr_is_int(num.repr_)) {
        out << repr_to_int(num.repr_);
    } else {
        auto *z = repr_to_bigint(num.repr_);
        auto len = mp_int_string_len(&z->num, BASE);
        auto target = out.reserve(len);
        handle_error(mp_int_to_string(&z->num, BASE, target.data(), len));
        out.trim_zero(len);
    }
    return out;
}

void append(Util::OutputBuffer &out, Number const &num, int base) {
    if (repr_is_int(num.repr_)) {
        out.append(repr_to_int(num.repr_), base);
    } else {
        auto *z = repr_to_bigint(num.repr_);
        auto len = mp_int_string_len(&z->num, base);
        auto target = out.reserve(len);
        handle_error(mp_int_to_string(&z->num, base, target.data(), len));
        out.trim_zero(len);
    }
}

auto bigint_refcount(uint64_t repr) -> std::atomic_size_t & {
    return repr_to_bigint(repr)->ref_count;
}

// NOLINTEND(readability-magic-numbers,cppcoreguidelines-pro-type-reinterpret-cast,performance-no-int-to-ptr,cppcoreguidelines-avoid-c-arrays,cppcoreguidelines-pro-bounds-pointer-arithmetic)

} // namespace CppClingo
