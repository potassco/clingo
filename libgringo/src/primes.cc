// {{{ MIT License

// Copyright 2017 Roland Kaminski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// }}}

#include "gringo/primes.hh"
#include <stdexcept>
#include <algorithm>
#include <initializer_list>
#include <cassert>

namespace Gringo {

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)

// Note: all of this is really unnecessary.
// Probe devision is completly sufficient for realistic container sizes. :)
namespace {

uint32_t mulmod(uint32_t a, uint32_t b, uint32_t m) {
    return (static_cast<uint64_t>(a) * b) % m;
}

#ifdef GRINGO_NEXTPRIME64
#   if defined(__SIZEOF_INT128__)
inline uint64_t mulmod(uint64_t a, uint64_t b, uint64_t m) {
    return (static_cast<__uint128_t>(a) * b) % m;
}
#   else
// https://en.wikipedia.org/wiki/Ancient_Egyptian_multiplication#Russian_peasant_multiplication
uint64_t mulmod(uint64_t a, uint64_t b, uint64_t m) {
    uint64_t res = 0;
    if (b < a) { std::swap(a, b); }
    if (b >= m) { b = (m > std::numeric_limits<uint64_t>::max() >> 1) ? b - m : b % m; }
    while (a != 0) {
        if (a & 1) {
            if (b >= m - res) { res -= m; }
            res += b;
        }
        a >>= 1;
        uint64_t t = b;
        if (b >= m - b) { t -= m; }
        b += t;
    }
    return res;
}
#   endif
#endif

// https://en.wikipedia.org/wiki/Modular_exponentiation
template <class T>
T modpow(T b, T e, T m) {
    if (m == 1) { return 0; }
    T r = 1;
    if (b >= m) { b = b % m; }
    while (e > 0) {
        if ((e & 1) == 1) { r = mulmod(r, b, m); }
        e = e >> 1;
        b = mulmod(b, b, m);
    }
    return r;
}

// https://stackoverflow.com/questions/4475996/given-prime-number-n-compute-the-next-prime
template <class T>
bool isPrimeProbe(T x) { // Note: this one makes assumptions about the numbers generated by next prime
    for (T i : { 0x7, 0xb, 0xd, 0x11, 0x13, 0x17, 0x1d }) {
        T q = x / i;
        if (q < i) { return true; }
        if (x == q * i) { return false; }
    }
    for (T i = 31;;) {
        for (T a : {0x6, 0x4, 0x2, 0x4, 0x2, 0x4, 0x6, 0x2}) {
            T q = x / i;
            if (q < i) { return true; }
            if (x == q * i) { return false; }
            i+= a;
        }
    }
    return true;
}

// https://miller-rabin.appspot.com/
std::initializer_list<uint32_t> const a32s1 = { 0x36e8934f };
std::initializer_list<uint32_t> const a32s2 = { 0x11724a, 0x8c16d95c };
std::initializer_list<uint32_t> const a32s3 = { 0x2, 0x7, 0x3d };
std::initializer_list<uint32_t> test(uint32_t n) {
    if (n < 0xbff5)     { return a32s1; }
    if (n < 0x157571b9) { return a32s2; }
    return a32s3;
}
#ifdef GRINGO_NEXTPRIME64
std::initializer_list<uint32_t> const a64s1 = { 0x81b33f22efdceaa9 };
std::initializer_list<uint32_t> const a64s2 = { 0x4e69b6552d, 0x223f5bb83fc553 };
std::initializer_list<uint32_t> const a64s3 = { 0x2, 0x7, 0x3d };
std::initializer_list<uint32_t> const a64s4 = { 0x3ab4f88ff0cc7c80, 0xcbee4cdf120c10aa, 0xe6f1343b0edca8e7 };
std::initializer_list<uint32_t> const a64s5 = { 0x2, 0x810c207b08bf, 0x10a42595b01d3765, 0x99fd2b545eab5322 };
std::initializer_list<uint32_t> const a64s6 = { 0x2, 0x3c1c7396f6d, 0x2142e2e3f22de5c, 0x297105b6b7b29dd, 0x370eb221a5f176dd };
std::initializer_list<uint32_t> const a64s7 = { 0x2, 0x70722e8f5cd0, 0x20cd6bd5ace2d1, 0x9bbc940c751630, 0xa90404784bfcb4d, 0x1189b3f265c2b0c7 };
std::initializer_list<uint32_t> const a64s8 = { 0x2, 0x3, 0x5, 0x7, 0xb, 0xd, 0x11, 0x13, 0x17, 0x1d, 0x1f, 0x25 };
std::initializer_list<uint64_t> test(uint64_t n) {
    if (n < 0x5361b)           { return a64s1; }
    if (n < 0x3e9de64d)        { return a64s2; }
    if (n < 0x11baa74c5)       { return a64s3; }
    if (n < 0x518dafbfd1)      { return a64s4; }
    if (n < 0x50649b08971)     { return a64s5; }
    if (n < 0x1c6b470864f683)  { return a64s6; }
    if (n < 0x81f23f390affe89) { return a64s7; }
    return a64s8;
}
#endif

// https://en.wikipedia.org/wiki/Miller%E2%80%93Rabin_primality_test#Deterministic_variants_of_the_test
template <class T>
bool isPrime(T n) {
    T r = 0;
    T d = n - 1;
    for (; (d & 1) == 0; d >>= 1, ++r) { }
    for (T a : test(n)) {
        assert(n != 0);
        a = a % n;
        if (a != 0) {
            T x = modpow(a, d, n);
            if (x != 1) {
                for (T i = 1, s = d; i < r && x != n-1; ++i) {
                    s <<= 1;
                    x = modpow(a, s, n);
                }
                if (x != n-1) { return false; }
            }
        }
    }
    //assert(isPrimeProbe(n));
    return true;
}

// https://stackoverflow.com/questions/4475996/given-prime-number-n-compute-the-next-prime
template <class T>
T nextPrime(T n) {
    T l = 30;
    std::initializer_list<T> smallPrimes = { 0x2, 0x3, 0x5, 0x7, 0xb, 0xd, 0x11, 0x13, 0x17, 0x1d };
    std::initializer_list<T> indices = { 0x1, 0x7, 0xb, 0xd, 0x11, 0x13, 0x17, 0x1d };
    if (n <= *(smallPrimes.end() - 1)) {
        return *std::lower_bound(smallPrimes.begin(), smallPrimes.end(), n);
    }
    T k = n / l;
    T i = static_cast<T>(std::lower_bound(indices.begin(), indices.end(), n - k * l) - indices.begin());
    n = l * k + *(indices.begin() + i);
    while (!isPrime(n)) {
        if (++i == indices.size()) {
            ++k;
            i = 0;
        }
        n = l * k + *(indices.begin() + i);
    }
    return n;
}

} // namespace

// should this be used???
//if ((x + 1) % 4 != 0) { return false; }

uint32_t nextPrime(uint32_t n) {
    if (n > 0xfffffffb) {
        throw std::overflow_error("maximum prime number exceeded");
    }
    return nextPrime<uint32_t>(n);
}

#ifdef GRINGO_NEXTPRIME64

uint64_t nextPrime(uint64_t n) {
    if (n > 0xffffffffffffffc5) {
        throw std::overflow_error("maximum prime number exceeded");
    }
    return nextPrime<uint64_t>(n);
}

#else

uint64_t nextPrime(uint64_t n) {
    static_cast<void>(n);
    throw std::runtime_error("support for 64bit primes not enabled");
}

#endif

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

} // namspace Gringo

