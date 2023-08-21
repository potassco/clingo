#include <cstring>
#include <forward_list>
#include <map>
#include <mutex>

#include <tsl/hopscotch_set.h>

#include <symbol_old.hh>

// NOLINTBEGIN(readability-magic-numbers,modernize-avoid-c-arrays)

namespace Gringo {

template <class Key, class Hash = Util::value_hasher<Key>, class KeyEqual = std::equal_to<>,
          class Allocator = std::allocator<Key>, unsigned int NeighborhoodSize = 62, bool StoreHash = false> // NOLINT
using hash_set = tsl::hopscotch_set<Key, Hash, KeyEqual, Allocator, NeighborhoodSize, StoreHash>;

// Note: The old system implemented symbol storage using 32bit indices. In
// principle, we can do something like this here, too, by adding further
// representation types and caveats regarding usage.

// The design is targeted toward 64bit architectures. There are probably better
// ways to store symbols on 32bit architectures.
// ========================================================================
// | 64 bit layout of symbol                                              |
// ========================================================================
// |                | 32b for number    | 29b for subtype   | 3b for type |
// |----------------+-------------------+-------------------+-------------|
// | Number         | the number        | 0 for number      | 0           |
// | Inf            | unused            | 1 for inf         | 0           |
// | Sup            | unused            | 2 for sup         | 0           |
// |----------------+-------------------+-------------------+-------------|
// |                | 16b for size      | 45b for pointer   |             |
// |----------------+-------------------+-------------------+-------------|
// | String         | unused            | pointer to string | 1           |
// | Tuple          |                   |                   |             |
// | - empty        | unused            | 0                 | 2           |
// | - large        | unused            | pointer to large  | 2           |
// | Function       |                   |                   |             |
// | - id           | unused            | pointer to string | 3           |
// | - large        | unused            | pointer to large  | 4           |
// ========================================================================
// | 32 bit layout of symbol                                              |
// ========================================================================
// |                | 32b for number    | 29b for subtype   | 3b for type |
// |----------------+-------------------+-------------------+-------------|
// | Number         | the number        | 0 for number      | 0           |
// | Inf            | unused            | 1 for inf         | 0           |
// | Sup            | unused            | 2 for sup         | 0           |
// |----------------+-------------------+-------------------+-------------|
// |                | 32b for pointer   | 29b for size      |             |
// |----------------+-------------------+-------------------+-------------|
// | String         | pointer to string | unused            | 1           |
// | Tuple          |                   |                   |             |
// | - empty        | 0                 | unused            | 2           |
// | - large        | pointer to large  | unused            | 2           |
// | Function       |                   |                   |             |
// | - id           | pointer to string | unused            | 3           |
// | - large        | pointer to large  | unused            | 4           |
// ------------------------------------------------------------------------

namespace {

enum RepType : uint64_t {
    rep_number_or_constant = 0,
    rep_string = 1,
    rep_tuple = 2,
    rep_id = 3,
    rep_function = 4,
};

enum SubRepType : uint64_t {
    sub_rep_number = 0,
    sub_rep_inf = 1,
    sub_rep_sup = 2,
};

template <size_t N> struct MS_;

template <> struct MS_<8> {
    static constexpr int type_size = 3;
    static constexpr uint64_t type_mask = (1ULL << type_size) - 1;

    static constexpr int ptr_shift = 0;

    static constexpr uint64_t lower_mask = (1ULL << 32) - 1;
};

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-const-variable"
#endif

template <> struct MS_<4> {
    static constexpr int type_size = 3;
    static constexpr uint64_t type_mask = (1ULL << type_size) - 1;

    static constexpr int ptr_shift = 32;

    static constexpr uint64_t lower_mask = (1ULL >> 32) - 1;
};

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

using MS = MS_<sizeof(uint64_t)>;

//! Simple thread-safe allocator prefixing pointers with a size.
//!
//! Note that GNU malloc actually prefixes a size already.
class SimpleAlloc {
  public:
    static auto alloc(size_t n) -> void * {
        auto k = n + sizeof(size_t);
        auto *data = reinterpret_cast<size_t *>(::operator new[](k));
        *data = n;
        return (data + 1);
    }

    static void dealloc(void *mem) {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfree-nonheap-object"
#endif
        // false positive
        ::operator delete[](reinterpret_cast<size_t *>(mem) - 1);
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
    }

    static auto size(void *mem) -> size_t { return *(reinterpret_cast<size_t *>(mem) - 1); }
};

//! A slotted allocator made for single-thread or locked multi-threaded use.
//!
//! This allocator should hopefully speed up allocation of symbols.
class SlottedAlloc {
  public:
    struct Sized {
        size_t size : sizeof(size_t) - 1;
        size_t tag : 1;
    };
    struct Node {
        size_t size : sizeof(size_t) - 1;
        size_t tag : 1;
        Node *next;
    };
    struct Head {
        Node *node = nullptr;
        size_t use_count = 0;
    };

    // TODO: maybe this can be done more cleverly decreasing the number of
    // allocations for large slots.
    static constexpr size_t ptr_size = sizeof(Node *);
    static constexpr size_t max_slot = 256;
    static constexpr size_t max_alloc = 1024;

    auto alloc(size_t n) -> void * {
        auto l = (n < ptr_size ? ptr_size : n);
        void *mem = nullptr;
        auto k = l + sizeof(size_t);
        if ((l - ptr_size) < max_slot) {
            auto &head = free_list_[l - ptr_size];
            auto &node = head.node;
            if (node == nullptr) {
                size_t m = max_alloc * k;
                node = reinterpret_cast<Node *>(::operator new[](m));
                // tag the beginning of the memory block
                new (node) Node(m, 1, nullptr);
            }
            Node *old = node;
            if (node->size == k) {
                node->size = n;
                node = node->next;
            } else {
                node = old + k;
                new (node) Node(old->size - k, 0, old->next);
            }
            old->size = n;
            mem = reinterpret_cast<Sized *>(old) + 1;
            ++head.use_count;
        } else {
            auto *data = reinterpret_cast<Sized *>(::operator new[](k));
            new (data) Sized(n, 1);
            mem = data + 1;
        }
        return mem;
    }

    void dealloc(void *mem) {
        if (mem != nullptr) {
            size_t n = size(mem);
            auto l = (n < ptr_size ? ptr_size : n);
            auto k = l + sizeof(size_t);
            if (l - ptr_size < max_slot) {
                // put node in the free list
                auto &head = free_list_[l - ptr_size];
                auto *node = reinterpret_cast<Node *>(reinterpret_cast<Sized *>(mem) - 1);
                node->next = head.node;
                node->size = k;
                head.node = node;
                --head.use_count;
                if (head.use_count == 0) {
                    free_head_(head);
                }
            } else {
                ::operator delete[](reinterpret_cast<Sized *>(mem) - 1);
            }
        }
    }

    static auto size(void *mem) -> size_t { return (reinterpret_cast<Sized *>(mem) - 1)->size; }

    ~SlottedAlloc() noexcept {
        for (auto &head : free_list_) {
            free_head_(head);
        }
    }

  private:
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

template <bool slotted> auto get_alloc() {
    if constexpr (slotted) {
        static SlottedAlloc alloc;
        return alloc;
    } else {
        static SimpleAlloc alloc;
        return alloc;
    }
}

template <bool slotted> class SymbolArray {
  public:
    SymbolArray(SymbolSpan symbols) : repr_{init_(symbols)} {}
    SymbolArray(Symbol name, SymbolSpan symbols) : repr_{init_(name, symbols)} {}
    SymbolArray(SymbolArray const &other) = delete;
    SymbolArray(SymbolArray &&other) noexcept : repr_{other.repr_} { other.repr_ = nullptr; }

    auto operator=(SymbolArray const &other) -> SymbolArray & = delete;
    auto operator=(SymbolArray &&other) noexcept -> SymbolArray & {
        std::swap(repr_, other.repr_);
        return *this;
    }

    [[nodiscard]] auto span() const noexcept -> SymbolSpan { return {repr_, size()}; }
    [[nodiscard]] auto head() const noexcept -> Symbol { return *repr_; }
    [[nodiscard]] auto tail() const noexcept -> SymbolSpan { return {repr_ + 1, size() - 1}; }
    [[nodiscard]] auto data() const noexcept -> Symbol * { return repr_; }
    [[nodiscard]] auto size() const noexcept -> size_t { return SlottedAlloc::size(repr_) / sizeof(Symbol); }

    ~SymbolArray() noexcept { get_alloc<slotted>().dealloc(repr_); }

  private:
    static auto init_(SymbolSpan symbols) -> Symbol * {
        auto *data = reinterpret_cast<Symbol *>(get_alloc<slotted>().alloc(symbols.size() * sizeof(Symbol)));
        std::copy(symbols.begin(), symbols.end(), data);
        return data;
    }

    static auto init_(Symbol name, SymbolSpan symbols) -> Symbol * {
        auto *data = reinterpret_cast<Symbol *>(get_alloc<slotted>().alloc((symbols.size() + 1) * sizeof(Symbol)));
        *data = name;
        std::copy(symbols.begin(), symbols.end(), data + 1);
        return data;
    }

    Symbol *repr_;
};

struct SymbolArrayHash {
    auto operator()(SymbolSpan fun) const -> size_t { return operator()(std::make_pair(fun.front(), fun.subspan(1))); }
    auto operator()(std::pair<Symbol, SymbolSpan> fun) const -> size_t {
        return Util::value_hash(fun.first, std::string_view{reinterpret_cast<char const *>(fun.second.data()),
                                                            fun.second.size() * sizeof(Symbol)});
    }

    template <bool slotted> auto operator()(SymbolArray<slotted> const &fun) const -> size_t {
        return operator()(std::make_pair(fun.head(), fun.tail()));
    }
};

struct SymbolArrayEqual {
    using is_transparent = void;
    auto operator()(SymbolSpan a, SymbolSpan b) const -> bool {
        return std::equal(a.begin(), a.end(), b.begin(), b.end());
    }

    template <bool slotted> auto operator()(SymbolArray<slotted> const &a, SymbolSpan b) const -> bool {
        return operator()(a.span(), b);
    }
    template <bool slotted> auto operator()(SymbolSpan a, SymbolArray<slotted> const &b) const -> bool {
        return operator()(a, b.span());
    }

    template <bool slotted>
    auto operator()(SymbolArray<slotted> const &a, std::pair<Symbol, SymbolSpan> b) const -> bool {
        return a.head() == b.first && operator()(a.tail(), b.second);
    }
    template <bool slotted>
    auto operator()(std::pair<Symbol, SymbolSpan> a, SymbolArray<slotted> const &b) const -> bool {
        return operator()(b, a);
    }
    template <bool slotted>
    auto operator()(SymbolArray<slotted> const &a, SymbolArray<slotted> const &b) const -> bool {
        return a.data() == b.data();
    }
};

using UString = std::unique_ptr<char[]>;

struct UStringEqual {
    using is_transparent = void;
    auto operator()(UString const &a, std::string_view b) const -> bool { return std::string_view{a.get()} == b; }
    auto operator()(std::string_view a, UString const &b) const -> bool { return operator()(b, a); }
    auto operator()(UString const &a, UString const &b) const -> bool { return a.get() == b.get(); }
};

struct UStringHash {
    auto operator()(UString const &a) const -> size_t { return operator()(a.get()); }
    auto operator()(std::string_view a) const -> size_t { return std::hash<std::string_view>{}(a); }
};

template <bool slotted> class DefaultSymbolStore : public SymbolStore {
  public:
    [[nodiscard]] auto function(String name, SymbolSpan args) -> Symbol override {
        auto size = args.size();
        if (size == 0) {
            auto rep = rep_id | (static_cast<uint64_t>(String::to_rep(name)) << MS::ptr_shift);
            return Symbol::from_rep(rep);
        }
        auto fun = std::make_pair(SymbolStore::string(name), args);
        auto jt = tuples_.find(fun);
        if (jt == tuples_.end()) {
            jt = tuples_.emplace(SymbolArray<slotted>{fun.first, fun.second}).first;
        }
        auto rep = reinterpret_cast<uint64_t>(jt->data()) | rep_function;
        return Symbol::from_rep(rep);
    }

    [[nodiscard]] auto tuple(SymbolSpan args) -> Symbol override {
        // Almost the same as for function except that the name does not have to be stored separately.
        auto size = args.size();
        if (size == 0) {
            return Symbol::from_rep(rep_tuple);
        }
        auto jt = tuples_.find(args);
        if (jt == tuples_.end()) {
            jt = tuples_.emplace(SymbolArray<slotted>{args}).first;
        }
        auto rep = reinterpret_cast<uint64_t>(jt->data()) | rep_tuple;
        return Symbol::from_rep(rep);
    }

    [[nodiscard]] auto string(std::string_view str) -> String override {
        auto it = strings_.find(str);
        if (it == strings_.end()) {
            auto res = std::make_unique<char[]>(str.size() + 1);
            std::copy(str.begin(), str.end(), res.get());
            it = strings_.emplace(std::move(res)).first;
        }
        return String::from_rep(reinterpret_cast<uintptr_t>(it->get()));
    }

    void clear() {
        strings_.clear();
        tuples_.clear();
    }

  private:
    using StringSet = hash_set<UString, UStringHash, UStringEqual>;
    using TupleSet = hash_set<SymbolArray<slotted>, SymbolArrayHash, SymbolArrayEqual>;

    StringSet strings_;
    TupleSet tuples_;
};

//! Simple thread-safe symbol store.
//!
//! More fine-grained locking is possible and also a shared lock is interesting.
template <bool slotted> class SharedSymbolStore : public SymbolStore {
  public:
    [[nodiscard]] auto function(String name, SymbolSpan args) -> Symbol override {
        std::unique_lock ulock{mutex_};
        return store_.function(name, args);
    }

    [[nodiscard]] auto tuple(SymbolSpan args) -> Symbol override {
        std::unique_lock ulock{mutex_};
        return store_.tuple(args);
    }

    [[nodiscard]] auto string(std::string_view str) -> String override {
        std::unique_lock ulock{mutex_};
        return store_.string(str);
    }

    ~SharedSymbolStore() noexcept override {
        std::unique_lock ulock{mutex_};
        store_.clear();
    }

  private:
    std::mutex mutex_;
    DefaultSymbolStore<slotted> store_;
};

auto default_symbol_store_() -> USymbolStore & {
    static USymbolStore store;
    return store;
}

} // namespace

[[nodiscard]] auto Symbol::num() const noexcept -> int32_t {
    assert(type() == SymbolType::number);
    return static_cast<uint32_t>(rep_ >> 32);
}

[[nodiscard]] auto Symbol::str() const noexcept -> String {
    assert(type() == SymbolType::string);
    return String::from_rep((rep_ & ~MS::type_mask) >> MS::ptr_shift);
}

[[nodiscard]] auto Symbol::name() const noexcept -> String {
    assert(type() == SymbolType::function);
    if ((rep_ & MS::type_mask) == rep_function) {
        return Symbol::from_rep(rep_ & ~MS::type_mask).str();
    }
    return Symbol::from_rep(rep_ & ~MS::type_mask).str();
}

[[nodiscard]] auto Symbol::args() const noexcept -> SymbolSpan {
    switch (rep_ & MS::type_mask) {
        case rep_id: {
            return SymbolSpan{};
        }
        case rep_function: {
            auto *ptr = reinterpret_cast<Symbol *>(rep_ & ~MS::type_mask);
            auto size = SlottedAlloc::size(ptr) / sizeof(Symbol);
            return SymbolSpan{ptr + 1, size - 1};
        }
        default: {
            assert((rep_ & MS::type_mask) == rep_tuple);
            auto *ptr = reinterpret_cast<Symbol *>(rep_ & ~MS::type_mask);
            auto size = ptr != nullptr ? SlottedAlloc::size(ptr) / sizeof(Symbol) : 0;
            return SymbolSpan{ptr, size};
        }
    }
}

[[nodiscard]] auto Symbol::type() const noexcept -> SymbolType {
    switch (rep_ & MS::type_mask) {
        case rep_number_or_constant: {
            auto sub_type = (rep_ & MS::lower_mask) >> MS::type_size;
            switch (sub_type) {
                case sub_rep_number:
                    return SymbolType::number;
                case sub_rep_inf:
                    return SymbolType::inf;
                default:
                    return SymbolType::sup;
            }
        }
        case rep_string: {
            return SymbolType::string;
        }
        case rep_tuple: {
            return SymbolType::tuple;
        }
        default: {
            return SymbolType::function;
        }
    }
}

auto SymbolStore::number(int32_t num) noexcept -> Symbol {
    uint64_t rep = (static_cast<uint64_t>(num) << 32) | (sub_rep_number << MS::type_size) | rep_number_or_constant;
    return Symbol::from_rep(rep);
}

[[nodiscard]] auto SymbolStore::sup() noexcept -> Symbol {
    uint64_t rep = (sub_rep_sup << MS::type_size) | rep_number_or_constant;
    return Symbol::from_rep(rep);
}

[[nodiscard]] auto SymbolStore::inf() noexcept -> Symbol {
    uint64_t rep = (sub_rep_inf << MS::type_size) | rep_number_or_constant;
    return Symbol::from_rep(rep);
}

auto SymbolStore::string(String str) noexcept -> Symbol {
    uint64_t rep = reinterpret_cast<uint64_t>(String::to_rep(str)) | rep_string;
    return Symbol::from_rep(rep);
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
        default_store = std::make_unique<DefaultSymbolStore<true>>();
    }
    return *default_store;
}

auto make_symbol_store(bool local, bool shared) -> USymbolStore {
    if (shared) {
        if (local) {
            return std::make_unique<SharedSymbolStore<false>>();
        }
        return std::make_unique<SharedSymbolStore<true>>();
    }
    if (local) {
        return std::make_unique<DefaultSymbolStore<false>>();
    }
    return std::make_unique<DefaultSymbolStore<true>>();
}

} // namespace Gringo

// NOLINTEND(readability-magic-numbers,modernize-avoid-c-arrays)
