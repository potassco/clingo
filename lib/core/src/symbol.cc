#include <algorithm>
#include <clingo/core/symbol.hh>

#include <clingo/util/macro.hh>
#include <clingo/util/print.hh>
#include <clingo/util/unordered_set.hh>

#include <atomic>
#include <condition_variable>
#include <cstring>
#include <mutex>

// #define DEBUG_GC

// NOLINTBEGIN(readability-magic-numbers,modernize-avoid-c-arrays,cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-bounds-constant-array-index,performance-no-int-to-ptr)

namespace CppClingo {

static_assert(sizeof(size_t) <= sizeof(uint64_t));

auto bigint_refcount(uint64_t repr) -> std::atomic_size_t &;

namespace {

constexpr auto TYPE_SIZE = static_cast<uint64_t>(3);
constexpr auto TYPE_MASK = static_cast<uint64_t>((1 << TYPE_SIZE) - 1);
constexpr auto EXT_TYPE_SIZE = static_cast<uint64_t>(TYPE_SIZE + 2);
constexpr auto EXT_TYPE_MASK = static_cast<uint64_t>((1 << EXT_TYPE_SIZE) - 1);

// NOLINTNEXTLINE(performance-enum-size)
enum RepType : uint64_t {
    REP_NUM_OR_CONSTANT = 0,
    REP_STR = 1,
    REP_TUP = 2,
    REP_SIGNED_ID = 3,
    REP_ID = 4,
    REP_SIGNED_FUN = 5,
    REP_FUN = 6,
    REP_BIGINT = 7,
};

// NOLINTNEXTLINE(performance-enum-size)
enum ExtRepType : uint64_t {
    EXT_REP_NUM = 0,
    EXT_REP_INF = 1 << TYPE_SIZE,
    EXT_REP_SUP = 2 << TYPE_SIZE,
};

struct Sized {
    uint64_t size : 63;
    uint64_t tag : 1;
};

auto alloc_size(void const *mem) -> size_t {
    return (reinterpret_cast<Sized const *>(mem) - 1)->size;
}

//! Simple thread-safe allocator prefixing pointers with a size.
//!
//! The size can be obtained using function alloc_size().
class SimpleAlloc {
  public:
    //! Allocated memory aligned to uint64_t.
    static auto alloc(size_t n) -> void * {
        auto k = n + sizeof(Sized);
        auto *data = reinterpret_cast<Sized *>(::operator new[](k));
        new (data) Sized{n, 1};
        return data + 1;
    }

    //! Deallocate the given memory.
    static void dealloc(void *mem) {
        if (mem != nullptr) {
            ::operator delete[](reinterpret_cast<Sized *>(mem) - 1);
        }
    }
};

//! A slotted allocator made for single-thread or locked multi-threaded use.
//!
//! This allocator should hopefully speed up allocation of symbols.
class SlottedAlloc {
  public:
    struct Node {
        uint64_t size : 63;
        uint64_t tag : 1;
        Node *next;
    };
    struct Head {
        Node *node = nullptr;
        size_t use_count = 0;
    };

    static constexpr size_t ptr_size = sizeof(Node *);
    static constexpr size_t max_slot = 256;
    static constexpr size_t max_alloc = 4096;

    SlottedAlloc() = default;
    SlottedAlloc(SlottedAlloc &&) = delete;
    auto operator=(SlottedAlloc &&) -> SlottedAlloc & = delete;
    ~SlottedAlloc() noexcept {
        for (auto &head : free_list_) {
            free_head_(head);
        }
    }

    //! Allocate memory aligned to uint64_t.
    auto alloc(size_t n) -> void * {
        void *mem = nullptr;
        if (n < max_slot) {
            auto k = block_size_(n);
            auto &head = free_list_[n];
            auto &node = head.node;
            if (node == nullptr) {
                // use limited exponential growth scheme for allocation
                size_t m = (head.use_count < max_alloc ? head.use_count + 1 : max_alloc) * k;
                static_assert(alignof(Node *) <= alignof(uint64_t));
                node = reinterpret_cast<Node *>(::operator new[](m));
                // tag the beginning of the memory block
                new (node) Node{m, 1, nullptr};
            }
            Node *old = node;
            if (node->size == k) {
                node = node->next;
            } else {
                node = reinterpret_cast<Node *>(reinterpret_cast<char *>(old) + k);
                new (node) Node{old->size - k, 0, old->next};
            }
            old->size = n;
            mem = reinterpret_cast<Sized *>(old) + 1;
            ++head.use_count;
        } else {
            mem = SimpleAlloc::alloc(n);
        }
        return mem;
    }

    //! Deallocate the given memory.
    void dealloc(void *mem) {
        if (mem != nullptr) {
            size_t n = alloc_size(mem);
            if (n < max_slot) {
                // put node in the free list
                auto &head = free_list_[n];
                auto *node = reinterpret_cast<Node *>(reinterpret_cast<Sized *>(mem) - 1);
                node->next = head.node;
                node->size = block_size_(n);
                head.node = node;
                --head.use_count;
                if (head.use_count == 0) {
                    free_head_(head);
                }
            } else {
                SimpleAlloc::dealloc(mem);
            }
        }
    }

  private:
    static auto block_size_(size_t n) -> size_t {
        // the usable block must at least be able to hold a pointer
        auto l = (n < ptr_size ? ptr_size : n);
        // align the same as Sized
        // (restricts for which types the allocator can be used)
        auto r = l % alignof(Sized);
        if (r > 0) {
            l += alignof(Sized) - r;
        }
        // we have to store the size
        return l + sizeof(Sized);
    }

    static void free_head_(Head &head) {
        Node *node = head.node;
        head.node = nullptr;
        // filter allocated nodes
        Node *begin = nullptr;
        while (node != nullptr) {
            auto *next = node->next;
            if (node->tag == 1) {
                node->next = begin;
                begin = node;
            }
            node = next;
        }
        // delete allocated nodes
        while (begin != nullptr) {
            auto *next = begin->next;
            ::operator delete[](begin);
            begin = next;
        }
    }

    std::array<Head, max_slot> free_list_;
};

constexpr auto dec_bit = static_cast<size_t>(1) << (8 * sizeof(size_t) - 1);
constexpr auto mark_bit = static_cast<size_t>(1) << (8 * sizeof(size_t) - 2);

void inc_ref(std::atomic_size_t &ref) noexcept {
    ref.fetch_add(1, std::memory_order::relaxed);
}
void dec_ref(std::atomic_size_t &ref) noexcept {
    assert((ref.load(std::memory_order::relaxed) & ~(dec_bit | mark_bit)) > 0);
    ref.fetch_or(dec_bit, std::memory_order::relaxed);
    ref.fetch_sub(1, std::memory_order::relaxed);
}
auto mark_ref(std::atomic_size_t &ref, bool only_if_referenced = false) noexcept -> bool {
    auto val = ref.load(std::memory_order::relaxed);
    if ((val & mark_bit) == 0 && (!only_if_referenced || val != 0)) {
        ref.fetch_or(mark_bit, std::memory_order::relaxed);
        return true;
    }
    return false;
}
auto unmark_ref(std::atomic_size_t &ref) noexcept -> bool {
    return (ref.fetch_and(~(dec_bit | mark_bit), std::memory_order::relaxed) & mark_bit) != 0;
}

//! Helper to manage the reference count of a symbol.
template <class T> class RefCounted {
  public:
    using value_type = T;
    static constexpr size_t adjust = std::is_same_v<T, char> ? 1 : 0;

    [[nodiscard]] static auto from_repr(uint64_t repr) -> RefCounted & { return *reinterpret_cast<RefCounted *>(repr); }
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    template <class Alloc> static auto alloc(Alloc &alloc, size_t n, bool referenced) -> RefCounted * {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,cppcoreguidelines-owning-memory)
        return new (alloc.alloc(sizeof(RefCounted) + (sizeof(value_type) * (n + adjust)))) RefCounted(referenced);
    }
    template <class Alloc> auto dealloc(Alloc &alloc) { alloc.dealloc(this); }
    auto data() -> T * {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        return data_;
    }
    auto hash() const -> size_t { return hash_; }
    void compute_hash(size_t n) {
        if constexpr (std::is_same_v<T, char>) {
            hash_ = std::hash<std::string_view>{}(std::string_view(data(), n));
        } else {
            hash_ = Util::value_hash(std::span(data(), n));
        }
    }
    [[nodiscard]] auto span() noexcept -> std::span<T> { return {data(), size()}; }
    [[nodiscard]] auto head() noexcept -> T & { return *data(); }
    [[nodiscard]] auto tail() noexcept -> std::span<T> { return {data() + 1, size() - 1}; }
    [[nodiscard]] auto view() noexcept -> std::string_view { return {data(), size()}; }
    auto size() const -> size_t { return ((alloc_size(this) - sizeof(RefCounted)) / sizeof(value_type)) - adjust; }
    auto ref_count() noexcept -> std::atomic_size_t & { return ref_count_; }

  private:
    // NOLINTNEXTLINE(modernize-use-equals-default)
    RefCounted(bool referenced) : ref_count_{referenced ? 1U : 0U} {}

    std::atomic_size_t mutable ref_count_;
    size_t hash_ = 0;
    CLINGO_IGNORE_ZERO_SIZED_ARRAY_B
    T data_[0];
    CLINGO_IGNORE_ZERO_SIZED_ARRAY_E
};

using SymbolArray = RefCounted<Symbol>;

class KeySymbolArray {
  public:
    KeySymbolArray() = default;

    template <class Alloc> KeySymbolArray(Alloc &alloc, SymbolSpan symbols, bool referenced) {
        static_assert(alignof(Symbol) <= alignof(uint64_t));
        size_t n = symbols.size();
        auto *data = SymbolArray::alloc(alloc, n, referenced);
        std::ranges::copy(symbols, data->data());
        data->compute_hash(n);
        repr_ = reinterpret_cast<uintptr_t>(data);
    }

    template <class Alloc> KeySymbolArray(Alloc &alloc, Symbol name, SymbolSpan symbols, bool referenced) {
        static_assert(alignof(Symbol) <= alignof(uint64_t));
        size_t n = symbols.size() + 1;
        auto *data = SymbolArray::alloc(alloc, n, referenced);
        *data->data() = name;
        std::ranges::copy(symbols, data->data() + 1);
        data->compute_hash(n);
        repr_ = reinterpret_cast<uintptr_t>(data);
    }

    template <class Alloc> void destroy(Alloc &alloc) const noexcept { repr().dealloc(alloc); }

    void unmark() const noexcept { repr_ |= mask_; }

    static auto to_repr(KeySymbolArray const &arr) -> uint64_t { return arr.repr_ & ~mask_; }

    [[nodiscard]] auto hash() const noexcept -> size_t { return repr().hash(); }
    [[maybe_unused]] friend auto operator==(KeySymbolArray const &a, KeySymbolArray const &b) -> bool {
        if (a.marked() || b.marked()) {
            auto sa = a.repr().span();
            auto sb = b.repr().span();
            return std::ranges::equal(sa, sb);
        }
        return KeySymbolArray::to_repr(a) == KeySymbolArray::to_repr(b);
    }

  private:
    KeySymbolArray(uint64_t repr) : repr_{repr} {}
    [[nodiscard]] auto repr() const noexcept -> SymbolArray & {
        return *reinterpret_cast<SymbolArray *>(to_repr(*this));
    }
    [[nodiscard]] auto marked() const noexcept -> bool { return (repr_ & mask_) == 0; }

    static constexpr uint64_t mask_ = 1U;
    uint64_t mutable repr_ = 0U;
};

using CharArray = RefCounted<char>;

class KeyCharArray {
  public:
    KeyCharArray() = default;

    template <class Alloc> KeyCharArray(Alloc &alloc, std::string_view str, bool referenced) {
        static_assert(alignof(char) <= alignof(uint64_t));
        size_t n = str.size();
        auto *repr = CharArray::alloc(alloc, n, referenced);
        std::copy(str.begin(), str.end(), repr->data());
        std::fill_n(repr->data() + n, 1, '\0');
        // TODO: store in SymbolArray
        repr->compute_hash(n);
        repr_ = reinterpret_cast<uintptr_t>(repr);
    }

    template <class Alloc> void destroy(Alloc &alloc) const noexcept { repr().dealloc(alloc); }

    void unmark() const noexcept { repr_ |= mask_; }
    static auto to_repr(KeyCharArray const &arr) -> uint64_t { return arr.repr_ & ~mask_; }

    [[nodiscard]] auto hash() const noexcept -> size_t { return repr().hash(); }
    [[maybe_unused]] friend auto operator==(KeyCharArray const &a, KeyCharArray const &b) -> bool {
        if (a.marked() || b.marked()) {
            return a.repr().view() == b.repr().view();
        }
        return KeyCharArray::to_repr(a) == KeyCharArray::to_repr(b);
    }

  private:
    KeyCharArray(uint64_t repr) : repr_{repr} {}
    [[nodiscard]] auto repr() const noexcept -> CharArray & { return *reinterpret_cast<CharArray *>(to_repr(*this)); }
    [[nodiscard]] auto marked() const noexcept -> bool { return (repr_ & mask_) == 0; }

    static constexpr uint64_t mask_ = 1U;
    uint64_t mutable repr_ = 0;
};

template <class Allocator> class DefaultSymbolStore : public SymbolStore {
  public:
    DefaultSymbolStore() = default;
    DefaultSymbolStore(DefaultSymbolStore &&) noexcept = delete;
    ~DefaultSymbolStore() noexcept override { clear(); }

    [[nodiscard]] auto do_num(Number num) noexcept -> Symbol override {
        auto jt = numbers_.insert(std::move(num)).first;
        return Symbol::from_rep(Number::to_repr(*jt));
    }

    [[nodiscard]] auto do_fun(String name, SymbolSpan args, bool sign, bool referenced) -> Symbol override {
        auto [jt, ins] = insert_(tuples_, SymbolStore::str_ref(name), args, referenced);
        auto rep = KeySymbolArray::to_repr(*jt) | (sign ? REP_SIGNED_FUN : REP_FUN);
        auto ret = Symbol::from_rep(rep);
        if (!ins && referenced) {
            ret.acquire();
        }
        return Symbol::from_rep(rep);
    }

    [[nodiscard]] auto do_tup(SymbolSpan args, bool referenced) -> Symbol override {
        // Almost the same as for function except that the name does not have to be stored separately.
        auto [jt, ins] = insert_(tuples_, args, referenced);
        auto rep = KeySymbolArray::to_repr(*jt) | REP_TUP;
        auto ret = Symbol::from_rep(rep);
        if (!ins && referenced) {
            ret.acquire();
        }
        return Symbol::from_rep(rep);
    }

    [[nodiscard]] auto do_string(std::string_view str, bool referenced) -> String override {
        assert(!str.empty());
        auto [it, ins] = insert_(strings_, str, referenced);
        auto ret = String::from_rep(KeyCharArray::to_repr(*it));
        if (!ins && referenced) {
            ret.acquire();
        }
        return ret;
    }

    void clear() {
        numbers_ = NumberSet{};
        clear_(strings_);
        clear_(tuples_);
    }

    void do_gc_block([[maybe_unused]] bool block) noexcept override {}

    void do_gc_add_owner(SymbolOwner const &owner) override { owners_.emplace(&owner); }

    void do_gc_del_owner(SymbolOwner const &owner) noexcept override { owners_.erase(&owner); }

    auto do_gc() -> std::tuple<size_t, size_t, size_t> override {
#ifdef DEBUG_GC
        printf("performing gc\n");
#endif
        SymbolCollector collector;
        // mark referenced tuples
        for (auto const &key : tuples_) {
            if (auto &arr = SymbolArray::from_repr(KeySymbolArray::to_repr(key)); mark_ref(arr.ref_count(), true)) {
                // recursively mark children
                for (auto const &sym : arr.span()) {
                    collector.mark(sym);
                }
            }
        }
        // mark referenced strings
        for (auto const &key : strings_) {
            mark_ref(CharArray::from_repr(KeyCharArray::to_repr(key)).ref_count(), true);
        }
        // mark numbers
        for (auto const &key : numbers_) {
            mark_ref(bigint_refcount(Number::to_repr(key)), true);
        }
        // mark symbols held by owners
        for (auto const *owner : owners_) {
            owner->mark(collector);
        }
        auto kept = size_t{0};
        auto collected = size_t{0};
        // destroy tuples
        for (auto it = tuples_.begin(); it != tuples_.end();) {
            if (unmark_ref(SymbolArray::from_repr(KeySymbolArray::to_repr(*it)).ref_count())) {
#ifdef DEBUG_GC
                printf("  keep tuple\n");
#endif
                ++kept;
                ++it;
            } else {
                ++collected;
                auto const &x = it.key();
                // NOTE: erase won't throw
                it = tuples_.erase(it);
                x.destroy(alloc_);
            }
        }
        // destroy strings
        for (auto it = strings_.begin(); it != strings_.end();) {
            if (unmark_ref(CharArray::from_repr(KeyCharArray::to_repr(*it)).ref_count())) {
#ifdef DEBUG_GC
                printf("  keep string: %s\n", CharArray::from_repr(KeyCharArray::to_repr(*it)).data());
#endif
                ++kept;
                ++it;
            } else {
#ifdef DEBUG_GC
                printf("  delete string: %s\n", CharArray::from_repr(KeyCharArray::to_repr(*it)).data());
#endif
                ++collected;
                auto const &x = it.key();
                // NOTE: erase won't throw
                it = strings_.erase(it);
                x.destroy(alloc_);
            }
        }
        // destroy numbers
        for (auto it = numbers_.begin(); it != numbers_.end();) {
            if (unmark_ref(bigint_refcount(Number::to_repr(*it)))) {
#ifdef DEBUG_GC
                printf("  keep number\n");
#endif
                ++kept;
                ++it;
            } else {
                ++collected;
                it = numbers_.erase(it);
            }
        }
#ifdef DEBUG_GC
        printf("  owners:    %zu\n", owners_.size());
        printf("  kept:      %zu\n", kept);
        printf("  collected: %zu\n", collected);
#endif
        return {owners_.size(), kept, collected};
    }

  private:
    using NumberSet = Util::unordered_set<Number>;
    using StringSet = Util::unordered_set<KeyCharArray>;
    using TupleSet = Util::unordered_set<KeySymbolArray>;
    using OwnerSet =
        Util::unordered_set<SymbolOwner const *, std::hash<SymbolOwner const *>, std::equal_to<SymbolOwner const *>>;

    template <class T, class... Args> auto insert_(T &table, Args &&...args) -> std::pair<typename T::iterator, bool> {
        typename T::value_type arr(alloc_, std::forward<Args>(args)...);
        try {
            auto [jt, ins] = table.emplace(arr);
            if (ins) {
                jt.key().unmark();
            } else {
                arr.destroy(alloc_);
            }
            return {jt, ins};
        } catch (...) {
            arr.destroy(alloc_);
            throw;
        }
    }

    template <class T> void clear_(T &table) noexcept {
        for (auto const &arr : table) {
            arr.destroy(alloc_);
        }
        table.clear();
    }

    Allocator alloc_;
    NumberSet numbers_;
    StringSet strings_;
    TupleSet tuples_;
    OwnerSet owners_;
};

//! Simple thread-safe symbol store.
//!
//! More fine-grained locking is possible and also a shared lock is interesting.
template <class Alloc> class SharedSymbolStore : public SymbolStore {
  public:
    [[nodiscard]] auto do_num(Number num) noexcept -> Symbol override {
        std::unique_lock ulock{mutex_};
        return store_.do_num(std::move(num));
    }

    [[nodiscard]] auto do_fun(String name, SymbolSpan args, bool sign, bool referenced) -> Symbol override {
        std::unique_lock ulock{mutex_};
        return store_.do_fun(name, args, sign, referenced);
    }

    [[nodiscard]] auto do_tup(SymbolSpan args, bool referenced) -> Symbol override {
        std::unique_lock ulock{mutex_};
        return store_.do_tup(args, referenced);
    }

    [[nodiscard]] auto do_string(std::string_view str, bool referenced) -> String override {
        assert(!str.empty());
        std::unique_lock ulock{mutex_};
        return store_.do_string(str, referenced);
    }

    void do_gc_block(bool block) noexcept override {
        std::unique_lock ulock{mutex_};
        if (block) {
            ++blocked_;
        } else {
            assert(blocked_ > 0);
            if (--blocked_ == 0) {
                ulock.unlock();
                cv_.notify_one();
            }
        }
    }

    void do_gc_add_owner(SymbolOwner const &owner) override {
        std::unique_lock ulock{mutex_};
        store_.gc_add_owner(owner);
    }

    void do_gc_del_owner(SymbolOwner const &owner) noexcept override {
        std::unique_lock ulock{mutex_};
        store_.gc_del_owner(owner);
    }

    auto do_gc() -> std::tuple<size_t, size_t, size_t> override {
        std::unique_lock ulock{mutex_};
        cv_.wait(ulock, [this] { return blocked_ == 0; });
        std::atomic_thread_fence(std::memory_order::seq_cst);
        return store_.gc();
    }

    ~SharedSymbolStore() noexcept override {
        std::unique_lock ulock{mutex_};
        store_.clear();
    }

  private:
    std::mutex mutex_;
    std::condition_variable cv_;
    DefaultSymbolStore<Alloc> store_;
    size_t blocked_ = 0;
};

auto default_symbol_store_() -> USymbolStore & {
    static USymbolStore store;
    return store;
}

} // namespace

// StringRef

auto String::c_str() const -> const char * {
    return rep_ != 0 ? CharArray::from_repr(rep_).data() : "";
}

auto String::view() const -> std::string_view {
    return rep_ != 0 ? CharArray::from_repr(rep_).view() : std::string_view{};
}

auto String::data() const -> char const * {
    return c_str();
}

auto String::empty() const -> bool {
    return *c_str() == '\0';
}

auto String::size() const -> size_t {
    return rep_ != 0 ? CharArray::from_repr(rep_).size() : 0;
}

auto String::starts_with(std::string_view prefix) const -> bool {
    return view().starts_with(prefix);
}

auto String::hash() const -> size_t {
    return rep_ != 0 ? CharArray::from_repr(rep_).hash() : 0;
}

auto operator<<(std::ostream &out, String const &str) -> std::ostream & {
    out << str.view();
    return out;
}

auto operator<<(Util::OutputBuffer &out, String const &str) -> Util::OutputBuffer & {
    out.append(str.view());
    return out;
}

void String::acquire() const noexcept {
    if (rep_ != 0) {
        inc_ref(CharArray::from_repr(rep_).ref_count());
    }
}

void String::release() const noexcept {
    if (rep_ != 0) {
        dec_ref(CharArray::from_repr(rep_).ref_count());
    }
}

// SymbolRef

void Symbol::acquire() const noexcept {
    auto val = rep_ & ~TYPE_MASK;
    switch (rep_ & TYPE_MASK) {
        case REP_ID:
        case REP_SIGNED_ID:
        case REP_STR: {
            if (val != 0) {
                inc_ref(CharArray::from_repr(val).ref_count());
            }
            break;
        }
        case REP_SIGNED_FUN:
        case REP_FUN:
        case REP_TUP: {
            if (val != 0) {
                inc_ref(SymbolArray::from_repr(val).ref_count());
            }
            break;
        }
        case REP_BIGINT: {
            inc_ref(bigint_refcount(rep_));
            break;
        }
        default: {
            break;
        }
    }
}

void Symbol::release() const noexcept {
    auto val = rep_ & ~TYPE_MASK;
    switch (rep_ & TYPE_MASK) {
        case REP_ID:
        case REP_SIGNED_ID:
        case REP_STR: {
            if (val != 0) {
                dec_ref(CharArray::from_repr(val).ref_count());
            }
            break;
        }
        case REP_SIGNED_FUN:
        case REP_FUN:
        case REP_TUP: {
            if (val != 0) {
                dec_ref(SymbolArray::from_repr(val).ref_count());
            }
            break;
        }
        case REP_BIGINT: {
            dec_ref(bigint_refcount(rep_));
            break;
        }
        default: {
            break;
        }
    }
}

auto Symbol::num() const noexcept -> Number const & {
    assert(type() == SymbolType::number);
    return *reinterpret_cast<Number const *>(this);
}

auto Symbol::str() const noexcept -> String {
    assert(type() == SymbolType::string);
    return String::from_rep(rep_ & ~TYPE_MASK);
}

auto Symbol::name() const noexcept -> String {
    assert(type() == SymbolType::function);
    switch (rep_ & TYPE_MASK) {
        case REP_ID:
        case REP_SIGNED_ID: {
            return String::from_rep(rep_ & ~TYPE_MASK);
        }
        default: {
            return SymbolArray::from_repr(rep_ & ~TYPE_MASK).head().str();
        }
    }
}

auto Symbol::args() const noexcept -> SymbolSpan {
    switch (rep_ & TYPE_MASK) {
        case REP_SIGNED_ID:
        case REP_ID: {
            return SymbolSpan{};
        }
        case REP_SIGNED_FUN:
        case REP_FUN: {
            return SymbolArray::from_repr(rep_ & ~TYPE_MASK).tail();
        }
        default: {
            assert((rep_ & TYPE_MASK) == REP_TUP);
            auto ptr = rep_ & ~TYPE_MASK;
            return ptr != 0 ? SymbolArray::from_repr(ptr).span() : SymbolSpan{};
        }
    }
}

auto Symbol::has_classical_sign() const -> bool {
    switch (rep_ & TYPE_MASK) {
        case REP_SIGNED_ID:
        case REP_SIGNED_FUN: {
            return true;
        }
        default: {
            return false;
        }
    }
}

auto Symbol::has_sign() const -> bool {
    switch (rep_ & TYPE_MASK) {
        case REP_NUM_OR_CONSTANT: {
            if ((rep_ & EXT_TYPE_MASK) == EXT_REP_NUM) {
                return static_cast<int>(rep_ >> 32) < 0;
            }
            return false;
        }
        case REP_SIGNED_ID:
        case REP_SIGNED_FUN: {
            return true;
        }
        case REP_BIGINT: {
            return num() < 0;
        }
        default: {
            return false;
        }
    }
}

auto Symbol::signature() const -> std::optional<std::tuple<String, size_t, bool>> {
    auto s = false;
    switch (rep_ & TYPE_MASK) {
        case REP_SIGNED_ID: {
            s = true;
            [[fallthrough]];
        }
        case REP_ID: {
            auto n = String::from_rep(rep_ & ~TYPE_MASK);
            return std::tuple{n, 0, s};
        }
        case REP_SIGNED_FUN: {
            s = true;
            [[fallthrough]];
        }
        case REP_FUN: {
            auto &a = SymbolArray::from_repr(rep_ & ~TYPE_MASK);
            return std::tuple{a.head().str(), a.size() - 1, s};
        }
        default: {
            return std::nullopt;
        }
    }
}

auto Symbol::flip_classical_sign() const -> std::optional<Symbol> {
    switch (rep_ & TYPE_MASK) {
        case REP_SIGNED_ID: {
            return Symbol{(rep_ & ~TYPE_MASK) | REP_ID};
        }
        case REP_SIGNED_FUN: {
            return Symbol{(rep_ & ~TYPE_MASK) | REP_FUN};
        }
        case REP_ID: {
            return Symbol{(rep_ & ~TYPE_MASK) | REP_SIGNED_ID};
        }
        case REP_FUN: {
            return Symbol{(rep_ & ~TYPE_MASK) | REP_SIGNED_FUN};
        }
        default: {
            return std::nullopt;
        }
    }
}

auto Symbol::type() const noexcept -> SymbolType {
    switch (rep_ & TYPE_MASK) {
        case REP_NUM_OR_CONSTANT: {
            switch (rep_ & EXT_TYPE_MASK) {
                case EXT_REP_NUM: {
                    return SymbolType::number;
                }
                case EXT_REP_INF: {
                    return SymbolType::inf;
                }
                default: {
                    return SymbolType::sup;
                }
            }
        }
        case REP_STR: {
            return SymbolType::string;
        }
        case REP_TUP: {
            return SymbolType::tuple;
        }
        case REP_BIGINT: {
            return SymbolType::number;
        }
        default: {
            return SymbolType::function;
        }
    }
}

namespace {

template <class T> void output_symbol(T &out, Symbol const &sym) {
    switch (sym.type()) {
        case SymbolType::inf: {
            out << "#inf";
            break;
        }
        case SymbolType::sup: {
            out << "#sup";
            break;
        }
        case SymbolType::number: {
            out << sym.num();
            break;
        }
        case SymbolType::string: {
            out << Util::p_quoted(sym.str().view());
            break;
        }
        case SymbolType::tuple: {
            auto args = sym.args();
            out << "(" << Util::p_range(args, ",") << (args.size() == 1 ? ",)" : ")");
            break;
        }
        case SymbolType::function: {
            auto args = sym.args();
            if (sym.has_classical_sign()) {
                out << "-";
            }
            out << sym.name();
            if (args.empty()) {
                out << Util::p_range(args, ",");
            } else {
                out << "(" << Util::p_range(args, ",") << ")";
            }
            break;
        }
    }
}

} // namespace

auto operator<<(std::ostream &out, Symbol const &sym) -> std::ostream & {
    output_symbol(out, sym);
    return out;
}

auto operator<<(Util::OutputBuffer &out, Symbol const &sym) -> Util::OutputBuffer & {
    output_symbol(out, sym);
    return out;
}

// Symbol

void SymbolCollector::mark(Symbol const &sym) {
    stack_.emplace_back(sym);
    while (!stack_.empty()) {
        auto rep = Symbol::to_rep(stack_.back());
        stack_.pop_back();
        auto typ = rep & TYPE_MASK;
        auto val = rep & ~TYPE_MASK;
        switch (typ) {
            case REP_TUP:
            case REP_SIGNED_FUN:
            case REP_FUN: {
                if (val != 0) {
                    if (auto &arr = SymbolArray::from_repr(val); mark_ref(arr.ref_count())) {
                        for (auto const &arg : arr.span()) {
                            stack_.emplace_back(arg);
                        }
                    }
                }
                break;
            }
            case REP_ID:
            case REP_SIGNED_ID:
            case REP_STR: {
                if (val != 0) {
                    mark_ref(CharArray::from_repr(val).ref_count());
                }
                break;
            }
            case REP_BIGINT: {
                mark_ref(bigint_refcount(rep));
                break;
            }
            default: {
                break;
            }
        }
    }
}

void SymbolCollector::mark(String const &str) {
    static_cast<void>(this);
    if (auto rep = String::to_rep(str); rep != 0) {
        mark_ref(CharArray::from_repr(rep).ref_count());
    }
}

// SymbolStore

auto SymbolStore::string(std::string_view str) -> SharedString {
    if (str.empty()) {
        return {};
    }
    return SharedString{do_string(str, true), false};
}

auto SymbolStore::sup() noexcept -> Symbol {
    return Symbol::from_rep(EXT_REP_SUP);
}

auto SymbolStore::inf() noexcept -> Symbol {
    return Symbol::from_rep(EXT_REP_INF);
}

auto SymbolStore::str(String str) noexcept -> SharedSymbol {
    return SymbolStore::str(SharedString{str});
}

auto SymbolStore::str(SharedString str) noexcept -> SharedSymbol {
    return SharedSymbol::from_rep(SharedString::to_rep(std::move(str)) | REP_STR);
}

auto SymbolStore::num(Number num) noexcept -> SharedSymbol {
    if (auto res = num.as_int(); res) {
        return SharedSymbol{Symbol::from_rep(Number::to_repr(num)), false};
    }
    return SharedSymbol{do_num(std::move(num)), true};
}

auto SymbolStore::num(int32_t num) noexcept -> SharedSymbol {
    return SharedSymbol{Symbol::from_rep(Number::to_repr(Number(num))), false};
}

auto SymbolStore::tup(SharedSymbolSpan args) -> SharedSymbol {
    return tup(SymbolSpan{reinterpret_cast<Symbol const *>(args.data()), args.size()});
}

auto SymbolStore::tup(SymbolSpan args) -> SharedSymbol {
    if (args.empty()) {
        return SharedSymbol{Symbol::from_rep(REP_TUP), false};
    }
    return SharedSymbol{do_tup(args, true), false};
}

auto SymbolStore::fun(SharedString const &name, SharedSymbolSpan args, bool sign) -> SharedSymbol {
    return fun(*name, SymbolSpan{reinterpret_cast<Symbol const *>(args.data()), args.size()}, sign);
}

auto SymbolStore::fun(String name, SymbolSpan args, bool sign) -> SharedSymbol {
    // The string is passed by const ref here. In principle, an acquire could
    // be avoided providing an overload by value. I don't think it's worth to
    // clutter the interface, though.
    if (args.empty()) {
        auto rep = (sign ? REP_SIGNED_ID : REP_ID) | String::to_rep(name);
        return SharedSymbol{Symbol::from_rep(rep), true};
    }
    return SharedSymbol{do_fun(name, args, sign, true), false};
}

auto SymbolStore::str_ref(String str) noexcept -> Symbol {
    return Symbol::from_rep(String::to_rep(str) | REP_STR);
}

auto SymbolStore::num_ref(Number num) noexcept -> Symbol {
    if (auto res = num.as_int(); res) {
        return Symbol::from_rep(Number::to_repr(num));
    }
    return do_num(std::move(num));
}

auto SymbolStore::num_ref(int32_t num) noexcept -> Symbol {
    return Symbol::from_rep(Number::to_repr(Number(num)));
}

auto SymbolStore::tup_ref(SymbolSpan args) -> Symbol {
    if (args.empty()) {
        return Symbol::from_rep(REP_TUP);
    }
    return do_tup(args, false);
}

auto SymbolStore::fun_ref(String name, SymbolSpan args, bool sign) -> Symbol {
    if (args.empty()) {
        auto rep = (sign ? REP_SIGNED_ID : REP_ID) | String::to_rep(name);
        return Symbol::from_rep(rep);
    }
    return do_fun(name, args, sign, false);
}

auto SymbolStore::string_ref(std::string_view str) -> String {
    if (str.empty()) {
        return {};
    }
    return do_string(str, false);
}

void init_default_symbol_store(USymbolStore store) {
    auto &default_store = default_symbol_store_();
    if (default_store.get() != nullptr) {
        throw std::runtime_error("the default symbol store can be set only once");
    }
    default_store = std::move(store);
}

auto default_symbol_store() -> SymbolStore & {
    auto &default_store = default_symbol_store_();
    if (default_store.get() == nullptr) {
        default_store = std::make_unique<DefaultSymbolStore<SlottedAlloc>>();
    }
    return *default_store;
}

auto make_symbol_store(bool slotted, bool shared) -> USymbolStore {
    if (shared) {
        if (slotted) {
            return std::make_unique<SharedSymbolStore<SlottedAlloc>>();
        }
        return std::make_unique<SharedSymbolStore<SimpleAlloc>>();
    }
    if (slotted) {
        return std::make_unique<DefaultSymbolStore<SlottedAlloc>>();
    }
    return std::make_unique<DefaultSymbolStore<SimpleAlloc>>();
}

auto NameGen::new_name() -> String {
    while (true) {
        auto [it, res] = names_.emplace(store_.string(prefix_ + std::to_string(num_)));
        ++num_;
        if (res) {
            return *it.key();
        }
    }
}

auto compare(Number const &a, Symbol const &b) -> int {
    return compare(reinterpret_cast<Symbol const &>(a), b);
}

auto compare(Symbol const &a, Number const &b) -> int {
    return compare(a, reinterpret_cast<Symbol const &>(b));
}

auto compare(Symbol const &a, Symbol const &b) -> int {
    if (a == b) {
        return 0;
    }
    auto type_prio = [](SymbolType type) -> int {
        switch (type) {
            case SymbolType::inf: {
                return 0;
            }
            case SymbolType::number: {
                return 1;
            }
            case SymbolType::string: {
                return 2;
            }
            case SymbolType::tuple: {
                return 3;
            }
            case SymbolType::function: {
                return 4;
            }
            case SymbolType::sup: {
                break;
            }
        }
        return 5;
    };
    auto type_a = a.type();
    auto type_b = b.type();
    if (type_a != type_b) {
        return type_prio(type_a) - type_prio(type_b);
    }
    switch (type_a) {
        case SymbolType::number: {
            return compare(a.num(), b.num());
        }
        case SymbolType::string: {
            return std::strcmp(a.str().c_str(), a.str().c_str());
        }
        case SymbolType::tuple:
        case SymbolType::function: {
            if (type_a == SymbolType::function) {
                auto name_a = a.name();
                auto name_b = b.name();
                if (name_a != name_b) {
                    return std::strcmp(name_a.c_str(), name_b.c_str());
                }
            }
            auto args_a = a.args();
            auto args_b = b.args();
            if (args_a.size() != args_b.size()) {
                return args_a.size() < args_b.size() ? -1 : 1;
            }
            for (auto it = args_a.begin(), jt = args_b.begin(), ie = args_a.end(); it != ie; ++it, ++jt) {
                auto cmp = compare(*it, *jt);
                if (cmp != 0) {
                    return cmp;
                }
            }
            break;
        }
        case SymbolType::inf:
        case SymbolType::sup: {
            break;
        }
    }
    return 0;
}

} // namespace CppClingo

// NOLINTEND(readability-magic-numbers,modernize-avoid-c-arrays,cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-bounds-constant-array-index,performance-no-int-to-ptr)
