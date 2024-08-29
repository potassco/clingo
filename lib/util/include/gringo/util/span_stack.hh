#pragma once

#include <gringo/util/hash.hh>
#include <gringo/util/macro.hh>

#include <cassert>
#include <memory_resource>
#include <span>

namespace Gringo::Util {

//! @addtogroup util_container
//! @{

//! Allocation size in bytes for chunks.
constexpr auto page_size = size_t{4096};

//! Compactly store fixed size arrays avoiding reallocations.
//!
//! The size is fixed upon construction.
template <class T> class SpanStack {
  public:
    //! Create a stack with the given size.
    SpanStack(size_t size) : size_{size} {}

    //! Deconstructor.
    ~SpanStack() {
        for (; root_ != nullptr; root_ = root_->free()) {
        }
    }

    //! Push an element.
    auto push(std::span<T const> arr) -> std::span<T> {
        if (size_ == 0) {
            return std::span<T>{static_cast<T *>(nullptr), 0};
        }
        return push_map(arr, [](auto const &val) { return val; });
    }

    //! Push an element in-place constructing it from the given range.
    template <typename Rng, typename Map> auto push_map(Rng const &rng, Map map) {
        if (size_ == 0) {
            return std::span<T>{static_cast<T *>(nullptr), 0};
        }
        if (root_ == nullptr || root_->size() == chunck_size_()) {
            auto *prev = root_;
            root_ = static_cast<Chunk *>(::operator new(sizeof(Chunk) + sizeof(T) * chunck_size_(),
                                                        static_cast<std::align_val_t>(alignof(Chunk))));
            new (root_) Chunk{prev};
        }
        return root_->push_map(rng, map);
    }

    //! Pop the last element.
    void pop() {
        if (size_ > 0) {
            if (root_->empty()) {
                root_ = root_->free();
            }
            root_->pop(size_);
        }
    }

  private:
    // NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-bounds-pointer-arithmetic)
    class Chunk {
      public:
        Chunk(Chunk *next = nullptr) : next_{next} {}
        ~Chunk() noexcept {
            std::for_each_n(data_, size_, [](auto &x) { x.~T(); });
        }
        [[nodiscard]] auto empty() const -> bool { return size_ == 0; }
        [[nodiscard]] auto size() const -> size_t { return size_; }
        template <typename Rng, typename Map> auto push_map(Rng const &rng, Map map) -> std::span<T> {
            // Note: does not provide strong exception guarantee (but could be implemented)
            auto *beg = data_ + size_;
            auto *ins = beg;
            for (auto const &val : rng) {
                new (ins) T(map(val));
                ++ins;
                ++size_;
            }
            return {beg, ins};
        }
        void pop(size_t size) {
            auto end = data_ + size_;
            std::for_each(end - size, end, [](auto &x) { x.~T(); });
            size_ -= size;
        }
        auto free() noexcept -> Chunk * {
            auto ret = next_;
            this->~Chunk();
            ::operator delete(static_cast<void *>(this), static_cast<std::align_val_t>(alignof(Chunk)));
            return ret;
        }

      private:
        Chunk *next_;
        size_t size_ = 0;
        GRINGO_IGNORE_ZERO_SIZED_ARRAY_B
        T data_[0]; // NOLINT
        GRINGO_IGNORE_ZERO_SIZED_ARRAY_E
    };
    // NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-bounds-pointer-arithmetic)

    auto chunck_size_() {
        auto n = page_size / sizeof(T);
        return size_ * (size_ >= n ? 1 : n / size_);
    }

    Chunk *root_ = nullptr;
    size_t size_;
};

//! Hasher for spans of fixed size.
//!
//! The size must be given upon construction.
class SpanHash {
  public:
    //! Initialize with the given size.
    SpanHash(size_t size) : size_{size} {}
    //! Get the hash of the symbol array.
    template <class T> auto operator()(T const *sym) const -> size_t { return value_hash(std::span(sym, size_)); }

  private:
    size_t size_;
};

//! Comparison operator for spans of fixed size.
//!
//! The size must be given upon construction.
struct SpanEqualTo {
  public:
    //! Initialize with the given size.
    SpanEqualTo(size_t size) : size_{size} {}
    //! Compare two symbols arrays.
    template <class T> auto operator()(T const *a, T const *b) const -> bool {
        return value_equal_to{}(std::span(a, size_), std::span(b, size_));
    }

  private:
    size_t size_;
};

//! @}

} // namespace Gringo::Util
