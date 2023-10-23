#include <bitset>
#include <imath.h>
#include <iostream>
#include <memory>

auto to_binary(mp_int z, int len, int limit) {
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

template <class F> void mp_int_apply(mp_int a, mp_int b, mp_int c, F f) {
    auto len_a = mp_int_binary_len(a);
    auto len_b = mp_int_binary_len(b);
    auto limit = std::max(len_a, len_b);
    auto buf_a = to_binary(a, len_a, limit);
    auto buf_b = to_binary(b, len_b, limit);
    for (int i = 0; i < limit; ++i) {
        buf_a[i] = f(buf_a[i], buf_b[i]);
    }
    mp_int_read_binary(c, buf_a.get(), limit);
}

void mp_int_xor(mp_int a, mp_int b, mp_int c) {
    mp_int_apply(a, b, c, [](auto a, auto b) { return a ^ b; });
}

void mp_int_and(mp_int a, mp_int b, mp_int c) {
    mp_int_apply(a, b, c, [](auto a, auto b) { return a & b; });
}

void mp_int_or(mp_int a, mp_int b, mp_int c) {
    mp_int_apply(a, b, c, [](auto a, auto b) { return a | b; });
}

struct binary_rep {
    mp_int z;
    int len;
};

auto operator<<(std::ostream &out, binary_rep z) -> std::ostream & {
    auto len = mp_int_binary_len(z.z);
    auto buf = to_binary(z.z, len, z.len);
    for (auto i = 0; i < z.len; ++i) {
        std::cerr << std::bitset<8>{buf[i]};
    }
    return out;
}

auto operator<<(std::ostream &out, mp_int z) -> std::ostream & {
    auto len = mp_int_string_len(z, 10);
    auto buf = std::make_unique<char[]>(len);
    mp_int_to_string(z, 10, buf.get(), len);
    out << buf.get();
    return out;
}

void debug(mp_int a, char op, mp_int b, mp_int c) {
    auto len_a = mp_int_binary_len(a);
    auto len_b = mp_int_binary_len(b);
    auto limit = std::max(len_a, len_b);
    auto sep = std::string(limit * 8 + 2, '=') + "\n";
    std::cout << sep << "  " << binary_rep{a, limit} << "\n"
              << op << " " << binary_rep{b, limit} << "\n"
              << "= " << binary_rep{c, limit} << "\n"
              << std::string(limit * 8 + 2, '-') + "\n"
              << a << " " << op << " " << b << " = " << c << "\n"
              << sep << std::endl;
}

auto main(int argc, char *argv[]) -> int {
    static_cast<void>(argc);

    mp_int x = mp_int_alloc();
    mp_int_read_string(x, 10, argv[1]);

    mp_int y = mp_int_alloc();
    mp_int_read_string(y, 10, argv[2]);

    mp_int z = mp_int_alloc();

    mp_int_and(x, y, z);
    debug(x, '&', y, z);

    mp_int_or(x, y, z);
    debug(x, '|', y, z);

    mp_int_xor(x, y, z);
    debug(x, '^', y, z);

    return 0;
}
