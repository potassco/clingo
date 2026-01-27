//
// Copyright (c) 2006-present Benjamin Kaufmann
//
// This file is part of Clasp. See https://potassco.org/clasp/
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//
//! \file
//! \brief Active configuration.
#pragma once

//! Library version.
#define CLASP_VERSION       "4.0.0"
#define CLASP_VERSION_MAJOR 4
#define CLASP_VERSION_MINOR 0
#define CLASP_VERSION_PATCH 0
#define CLASP_LEGAL         "Copyright (C) Benjamin Kaufmann"

//! Single or multi-threaded version of clasp.
#define CLASP_HAS_THREADS 1

//! Use std::vector instead of pod_vector.
#define CLASP_USE_STD_VECTOR 0

#include <potassco/platform.h>

#if CLASP_HAS_THREADS
#include <atomic>
#endif
#include <utility>

namespace Clasp {
constexpr auto cache_line_size = 64;

namespace mt {
consteval bool hasThreads() { return CLASP_HAS_THREADS; }
#if CLASP_HAS_THREADS
using std::memory_order_acq_rel;
using std::memory_order_acquire;
using std::memory_order_relaxed;
using std::memory_order_release;
using std::memory_order_seq_cst;
using MemoryOrder = std::memory_order;
template <typename T>
using AtomicType = std::atomic<T>;
#else
enum class MemoryOrder {};
constexpr inline auto memory_order_acq_rel = MemoryOrder{};
constexpr inline auto memory_order_acquire = MemoryOrder{};
constexpr inline auto memory_order_relaxed = MemoryOrder{};
constexpr inline auto memory_order_release = MemoryOrder{};
constexpr inline auto memory_order_seq_cst = MemoryOrder{};
template <typename T>
using AtomicType = void;
#endif

template <typename T, bool Mt = hasThreads()>
class ThreadSafe {
    struct SingleThread {
        [[nodiscard]] T load(MemoryOrder = {}) const noexcept { return val; }
        void            store(T xVal, MemoryOrder = {}) noexcept { val = xVal; }
        T               exchange(T nVal, MemoryOrder = {}) noexcept { return std::exchange(val, nVal); }
        T               fetch_add(T xVal, MemoryOrder = {}) noexcept { return std::exchange(val, val + xVal); }
        T               fetch_sub(T xVal, MemoryOrder = {}) noexcept { return std::exchange(val, val - xVal); }
        T               fetch_or(T xVal, MemoryOrder = {}) noexcept { return std::exchange(val, val | xVal); }
        T               fetch_and(T xVal, MemoryOrder = {}) noexcept { return std::exchange(val, val & xVal); }
        bool            compare_exchange_weak(T& oVal, T nVal, MemoryOrder m = {}) noexcept {
            return compare_exchange_strong(oVal, nVal, m);
        }
        bool compare_exchange_strong(T& oVal, T nVal, MemoryOrder = {}) noexcept {
            return val == oVal ? (store(nVal), true) : (oVal = val, false);
        }
        T val{};
    };
    static_assert(not Mt || hasThreads(), "Threads are disabled!");
    using ImplType = std::conditional_t<Mt, AtomicType<T>, SingleThread>;

public:
    explicit ThreadSafe(T x) : ref_{x} {}
    ThreadSafe() : ref_{} {}

    [[nodiscard]] T load(MemoryOrder m = memory_order_seq_cst) const noexcept { return ref_.load(m); }
    operator T() const noexcept { return load(); }
    void store(T xVal, MemoryOrder m = memory_order_seq_cst) noexcept { ref_.store(xVal, m); }
    T    exchange(T nVal, MemoryOrder m = memory_order_seq_cst) noexcept { return ref_.exchange(nVal, m); }
    T    add(T xVal, MemoryOrder m = memory_order_seq_cst) noexcept { return ref_.fetch_add(xVal, m) + xVal; }
    T    sub(T xVal, MemoryOrder m = memory_order_seq_cst) noexcept { return ref_.fetch_sub(xVal, m) - xVal; }

    bool compare_exchange_weak(T& oVal, T nVal, MemoryOrder m = memory_order_seq_cst) noexcept {
        return ref_.compare_exchange_weak(oVal, nVal, m);
    }
    bool compare_exchange_strong(T& oVal, T nVal, MemoryOrder m = memory_order_seq_cst) noexcept {
        return ref_.compare_exchange_strong(oVal, nVal, m);
    }
    ImplType& ref() { return ref_; }

private:
    ImplType ref_;
};
} // namespace mt

} // namespace Clasp
