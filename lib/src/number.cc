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

// NOLINTEND(readability-magic-numbers)

} // namespace

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
    // int = int
    if (repr_is_int(repr_) && repr_is_int(other.repr_)) {
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

[[nodiscard]] auto Number::as_int() const -> int32_t {
    assert(repr_is_int(repr_));
    return repr_to_int(repr_);
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

auto operator==(Number const &a, Number const &b) -> bool {
    // int == int
    if (repr_is_int(a.repr_) && repr_is_int(b.repr_)) {
        return a.as_int() == b.as_int();
    }
    // int == big
    if (repr_is_int(a.repr_)) {
        return mp_int_compare_value(repr_to_bigint(b.repr_), repr_to_int(a.repr_)) == 0;
    }
    // big == int
    if (repr_is_int(b.repr_)) {
        return mp_int_compare_value(repr_to_bigint(a.repr_), repr_to_int(b.repr_)) == 0;
    }
    // big == big
    return mp_int_compare(repr_to_bigint(a.repr_), repr_to_bigint(b.repr_)) == 0;
}

auto operator==(int32_t a, Number const &b) -> bool {
    // int == int
    if (repr_is_int(b.repr_)) {
        return a == repr_to_int(b.repr_);
    }
    // int == big
    return mp_int_compare_value(repr_to_bigint(b.repr_), a) == 0;
}

auto operator==(Number const &a, int32_t b) -> bool { return b == a; }

auto operator+(Number const &a, Number const &b) -> Number {
    // int + int
    if (repr_is_int(a.repr_) && repr_is_int(b.repr_)) {
        if (auto c = check_add(a.as_int(), b.as_int()); c) {
            return {*c};
        }
        mp_int_ptr z;
        handle_error(mp_int_init_value(z, a.as_int()));
        handle_error(mp_int_add_value(z, a.as_int(), z));
        return {z.release_repr(true)};
    }
    mp_int_ptr z;
    // int + big
    if (repr_is_int(a.repr_)) {
        handle_error(mp_int_add_value(repr_to_bigint(b.repr_), repr_to_int(a.repr_), z));
    }
    // big + int
    else if (repr_is_int(b.repr_)) {
        handle_error(mp_int_add_value(repr_to_bigint(a.repr_), repr_to_int(b.repr_), z));
    }
    // big + big
    else {
        handle_error(mp_int_add(repr_to_bigint(a.repr_), repr_to_bigint(b.repr_), z));
    }
    return z.release_repr();
}

auto operator+(Number &&a, Number const &b) -> Number {
    if (!repr_is_int(a.repr_)) {
        // big + int
        if (repr_is_int(b.repr_)) {
            handle_error(mp_int_add_value(repr_to_bigint(a.repr_), repr_to_int(b.repr_), repr_to_bigint(a.repr_)));
        }
        // big + big
        else {
            handle_error(mp_int_add(repr_to_bigint(a.repr_), repr_to_bigint(b.repr_), repr_to_bigint(a.repr_)));
        }
        return std::move(a);
    }
    return a + b;
}

auto operator+(Number const &a, Number &&b) -> Number {
    if (!repr_is_int(b.repr_)) {
        return std::move(b) + a;
    }
    return a + b;
}

auto operator+(Number &&a, Number &&b) -> Number {
    if (!repr_is_int(a.repr_)) {
        return std::move(a) + b;
    }
    if (!repr_is_int(b.repr_)) {
        return std::move(b) + a;
    }
    return a + b;
}

auto operator+=(Number &a, Number const &b) -> Number & {
    // int + int
    if (repr_is_int(a.repr_) && repr_is_int(b.repr_)) {
        if (auto c = check_add(a.as_int(), b.as_int()); c) {
            a.repr_ = int_to_repr(c.value());
            return a;
        }
        mp_int_ptr z;
        handle_error(mp_int_init_value(z, a.as_int()));
        handle_error(mp_int_add_value(z, a.as_int(), z));
        a.repr_ = z.release_repr(true);
    }
    // int + big
    else if (repr_is_int(a.repr_)) {
        mp_int_ptr z;
        handle_error(mp_int_init_value(z, a.as_int()));
        handle_error(mp_int_add(z, repr_to_bigint(b.repr_), z));
        a.repr_ = z.release_repr();
    }
    // big + int
    else if (repr_is_int(b.repr_)) {
        handle_error(mp_int_add_value(repr_to_bigint(a.repr_), repr_to_int(b.repr_), repr_to_bigint(a.repr_)));
    }
    // big + big
    else {
        handle_error(mp_int_add(repr_to_bigint(a.repr_), repr_to_bigint(b.repr_), repr_to_bigint(a.repr_)));
    }
    return a;
}

auto operator+=(Number &a, Number &&b) -> Number & {
    // int + big
    if (repr_is_int(a.repr_) && !repr_is_int(b.repr_)) {
        std::swap(a.repr_, b.repr_);
    }
    return a += b;
}

} // namespace Gringo
