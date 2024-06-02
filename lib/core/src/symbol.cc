#include <gringo/core/symbol.hh>

#include <gringo/util/print.hh>
#include <gringo/util/unordered_set.hh>

#include <atomic>
#include <cstring>
#include <iostream>
#include <mutex>

// NOLINTBEGIN(readability-magic-numbers,modernize-avoid-c-arrays,cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-bounds-constant-array-index,performance-no-int-to-ptr)

namespace Gringo {

static_assert(sizeof(size_t) <= sizeof(uint64_t));

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

auto alloc_size(void const *mem) -> size_t { return (reinterpret_cast<Sized const *>(mem) - 1)->size; }

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

//! Helper to manage the reference count of a symbol.
//!
//! Only the inc and dec function should be called in parallel. Before garbage
//! collection, all threads modifying reference counts should release memory
//! with std::atomic_thread_fence. The thread performing the actual garbage
//! collection should acquire memory using the same function.
template <class T> class RefCounted {
  public:
    using value_type = T;
    static constexpr size_t adjust = std::is_same_v<T, char> ? 1 : 0;

    [[nodiscard]] static auto from_repr(uint64_t repr) -> RefCounted & { return *reinterpret_cast<RefCounted *>(repr); }
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    template <class Alloc> static auto alloc(Alloc &alloc, size_t n) -> RefCounted * {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init,cppcoreguidelines-owning-memory)
        return new (alloc.alloc(sizeof(RefCounted) + sizeof(value_type) * (n + adjust))) RefCounted();
    }
    template <class Alloc> auto dealloc(Alloc &alloc) { alloc.dealloc(this); }
    auto data() -> T * {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
        return data_;
    }
    void inc() const noexcept { ref_count_.fetch_add(1, std::memory_order::relaxed); }
    void dec() const noexcept { ref_count_.fetch_sub(1, std::memory_order::relaxed); }
    //! Mark symbols.
    //!
    //! If the init flag is set, the count of referenced symbols is
    //! increased by one. The idea is to be able to reset reference counts of
    //! such symbols by subtracting one later. Such symbols have at least a
    //! count of 2.
    //! If the init flag is not set, set the reference count to one. This
    //! values serves as a marker and is never incremented beyond that value
    //! during garbage collection.
    auto mark(bool init = false) -> bool {
        if (init ? ref_count() > 0 : ref_count() == 0) {
            ref_count_.fetch_add(1, std::memory_order::relaxed);
            return true;
        }
        return false;
    }
    //! Unmark previously marked symbols.
    //!
    //! Returns false if the symbol has not previously been marked. Such
    //! symbols should be deleted.
    auto unmark() -> bool {
        if (ref_count() > 0) {
            ref_count_.fetch_sub(1, std::memory_order::relaxed);
            return true;
        }
        return false;
    }
    auto ref_count() const noexcept -> size_t { return ref_count_.load(std::memory_order::relaxed); }
    auto hash(size_t n) -> size_t {
        if constexpr (std::is_same_v<T, char>) {
            return std::hash<std::string_view>{}(std::string_view(data(), n));
        } else {
            return Util::value_hash(std::span(data(), n));
        }
    }
    [[nodiscard]] auto span() noexcept -> std::span<T> { return {data(), size()}; }
    [[nodiscard]] auto head() noexcept -> T & { return *data(); }
    [[nodiscard]] auto tail() noexcept -> std::span<T> { return {data() + 1, size() - 1}; }
    [[nodiscard]] auto view() noexcept -> std::string_view { return {data(), size()}; }
    auto size() const -> size_t { return ((alloc_size(this) - sizeof(RefCounted)) / sizeof(value_type)) - adjust; }

  private:
    // NOLINTNEXTLINE(modernize-use-equals-default)
    RefCounted() {}

    std::atomic_size_t mutable ref_count_ = 0;
    T data_[0];
};

using SymbolArray = RefCounted<Symbol>;

class KeySymbolArray {
  public:
    KeySymbolArray() = default;

    template <class Alloc> KeySymbolArray(Alloc &alloc, SymbolSpan symbols) {
        static_assert(alignof(Symbol) <= alignof(uint64_t));
        size_t n = symbols.size();
        auto *data = SymbolArray::alloc(alloc, n);
        std::copy(symbols.begin(), symbols.end(), data->data());
        hash_ = data->hash(n);
        repr_ = reinterpret_cast<uintptr_t>(data);
    }

    template <class Alloc> KeySymbolArray(Alloc &alloc, Symbol name, SymbolSpan symbols) {
        static_assert(alignof(Symbol) <= alignof(uint64_t));
        size_t n = symbols.size() + 1;
        auto *data = SymbolArray::alloc(alloc, n);
        *data->data() = name;
        std::copy(symbols.begin(), symbols.end(), data->data() + 1);
        hash_ = data->hash(n);
        repr_ = reinterpret_cast<uintptr_t>(data);
    }

    template <class Alloc> void destroy(Alloc &alloc) const noexcept { repr().dealloc(alloc); }

    void unmark() const noexcept { repr_ |= mask_; }

    static auto to_repr(KeySymbolArray const &arr) -> uint64_t { return arr.repr_ & ~mask_; }

    [[nodiscard]] auto hash() const noexcept -> size_t { return hash_; }
    [[maybe_unused]] friend auto operator==(KeySymbolArray const &a, KeySymbolArray const &b) -> bool {
        if (a.marked() || b.marked()) {
            auto sa = a.repr().span();
            auto sb = b.repr().span();
            return std::equal(sa.begin(), sa.end(), sb.begin(), sb.end());
        }
        return KeySymbolArray::to_repr(a) == KeySymbolArray::to_repr(b);
    }

  private:
    KeySymbolArray(uint64_t repr) : repr_{repr} {}
    [[nodiscard]] auto repr() const noexcept -> SymbolArray & {
        return *reinterpret_cast<SymbolArray *>(to_repr(*this));
    }
    [[nodiscard]] auto marked() const noexcept -> bool { return (repr_ & mask_) == 0; }

    static constexpr uintptr_t mask_ = 1U;
    size_t hash_ = 0;
    uintptr_t mutable repr_ = 0U;
};

using CharArray = RefCounted<char>;

class KeyCharArray {
  public:
    KeyCharArray() = default;

    template <class Alloc> KeyCharArray(Alloc &alloc, std::string_view str) {
        static_assert(alignof(char) <= alignof(uint64_t));
        size_t n = str.size();
        auto *repr = CharArray::alloc(alloc, n);
        std::copy(str.begin(), str.end(), repr->data());
        std::fill_n(repr->data() + n, 1, '\0');
        hash_ = repr->hash(n);
        repr_ = reinterpret_cast<uintptr_t>(repr);
    }

    template <class Alloc> void destroy(Alloc &alloc) const noexcept { repr().dealloc(alloc); }

    void unmark() const noexcept { repr_ |= mask_; }
    static auto to_repr(KeyCharArray const &arr) -> uint64_t { return arr.repr_ & ~mask_; }

    [[nodiscard]] auto hash() const noexcept -> size_t { return hash_; }
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

    static constexpr uintptr_t mask_ = 1U;
    size_t hash_ = 0;
    uintptr_t mutable repr_ = 0;
};

template <class Allocator> class DefaultSymbolStore : public SymbolStore {
  public:
    DefaultSymbolStore() = default;
    DefaultSymbolStore(DefaultSymbolStore &&) noexcept = delete;
    ~DefaultSymbolStore() noexcept override { clear(); }

    [[nodiscard]] auto do_num(Number const &num) noexcept -> Symbol override {
        auto jt = numbers_.find(num);
        if (jt == numbers_.end()) {
            jt = numbers_.insert(num).first;
        }
        return Symbol::from_rep(Number::to_repr(*jt));
    }

    [[nodiscard]] auto do_num(Number &&num) noexcept -> Symbol override {
        auto jt = numbers_.insert(std::move(num)).first;
        return Symbol::from_rep(Number::to_repr(*jt));
    }

    [[nodiscard]] auto do_fun(String name, SymbolSpan args, bool sign) -> Symbol override {
        auto jt = insert_(tuples_, SymbolStore::str(name), args);
        auto rep = KeySymbolArray::to_repr(*jt) | (sign ? REP_SIGNED_FUN : REP_FUN);
        return Symbol::from_rep(rep);
    }

    [[nodiscard]] auto do_tup(SymbolSpan args) -> Symbol override {
        // Almost the same as for function except that the name does not have to be stored separately.
        auto jt = insert_(tuples_, args);
        auto rep = KeySymbolArray::to_repr(*jt) | REP_TUP;
        return Symbol::from_rep(rep);
    }

    [[nodiscard]] auto do_string(std::string_view str) -> String override {
        assert(!str.empty());
        auto it = insert_(strings_, str);
        return String::from_rep(KeyCharArray::to_repr(*it));
    }

    void clear() {
        numbers_ = NumberSet{};
        clear_(strings_);
        clear_(tuples_);
    }

    void do_add_owner(SymbolOwner &owner) override { owners_.emplace_back(&owner); }

    void do_remove_owner(SymbolOwner &owner) noexcept override {
        owners_.erase(std::find(owners_.begin(), owners_.end(), &owner));
    }

    void do_gc() override {
        SymbolCollector collector;
        // mark referenced tuples
        for (auto const &key : tuples_) {
            auto &arr = SymbolArray::from_repr(KeySymbolArray::to_repr(key));
            arr.mark(true);
        }
        // mark referenced strings
        for (auto const &key : strings_) {
            auto &arr = CharArray::from_repr(KeyCharArray::to_repr(key));
            arr.mark(true);
        }
        // mark numbers
        // TODO!
        // mark all (unreferenced) children
        for (auto const &key : tuples_) {
            auto &arr = SymbolArray::from_repr(KeySymbolArray::to_repr(key));
            if (arr.ref_count() > 1) {
                // mark the children
                for (auto const &sym : arr.span()) {
                    collector.mark(sym);
                }
            }
        }
        // mark all (unreferenced) children held by owners
        for (auto *owner : owners_) {
            owner->mark(collector);
        }
        for (auto const &key : tuples_) {
            auto &arr = SymbolArray::from_repr(KeySymbolArray::to_repr(key));
            if (!arr.unmark()) {
                tuples_.erase(key);
                key.destroy(alloc_);
            }
        }
        // destroy strings
        for (auto const &key : strings_) {
            auto &arr = CharArray::from_repr(KeyCharArray::to_repr(key));
            if (!arr.unmark()) {
                strings_.erase(key);
                key.destroy(alloc_);
            }
        }
        // destroy numbers
        // TODO!
    }

  private:
    using NumberSet = Util::unordered_set<Number>;
    using StringSet = Util::unordered_set<KeyCharArray>;
    using TupleSet = Util::unordered_set<KeySymbolArray>;

    template <class T, class... Args> auto insert_(T &table, Args &&...args) -> typename T::iterator {
        typename T::value_type arr(alloc_, std::forward<Args>(args)...);
        try {
            auto [jt, ins] = table.emplace(arr);
            if (ins) {
                jt.key().unmark();
            } else {
                arr.destroy(alloc_);
            }
            return jt;
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
    std::vector<SymbolOwner *> owners_;
};

//! Simple thread-safe symbol store.
//!
//! More fine-grained locking is possible and also a shared lock is interesting.
template <class Alloc> class SharedSymbolStore : public SymbolStore {
  public:
    [[nodiscard]] auto do_num(Number const &num) noexcept -> Symbol override {
        std::unique_lock ulock{mutex_};
        return store_.do_num(num);
    }

    [[nodiscard]] auto do_num(Number &&num) noexcept -> Symbol override {
        std::unique_lock ulock{mutex_};
        return store_.do_num(std::move(num));
    }

    [[nodiscard]] auto do_fun(String name, SymbolSpan args, bool sign) -> Symbol override {
        std::unique_lock ulock{mutex_};
        return store_.fun(name, args, sign);
    }

    [[nodiscard]] auto do_tup(SymbolSpan args) -> Symbol override {
        std::unique_lock ulock{mutex_};
        return store_.tup(args);
    }

    [[nodiscard]] auto do_string(std::string_view str) -> String override {
        assert(!str.empty());
        std::unique_lock ulock{mutex_};
        return store_.string(str);
    }

    void do_add_owner(SymbolOwner &owner) override {
        std::unique_lock ulock{mutex_};
        store_.add_owner(owner);
    }

    void do_remove_owner(SymbolOwner &owner) noexcept override {
        std::unique_lock ulock{mutex_};
        store_.add_owner(owner);
    }

    void do_gc() override {
        // TODO: this function should be blocked
        store_.gc();
    }

    ~SharedSymbolStore() noexcept override {
        std::unique_lock ulock{mutex_};
        store_.clear();
    }

  private:
    std::mutex mutex_;
    DefaultSymbolStore<Alloc> store_;
};

auto default_symbol_store_() -> USymbolStore & {
    static USymbolStore store;
    return store;
}

} // namespace

auto String::c_str() const -> const char * { return rep_ != 0 ? CharArray::from_repr(rep_).data() : ""; }

auto String::view() const -> std::string_view {
    return rep_ != 0 ? CharArray::from_repr(rep_).view() : std::string_view{};
}

auto String::empty() const -> bool { return *c_str() == '\0'; }

auto String::size() const -> size_t { return rep_ != 0 ? CharArray::from_repr(rep_).size() : 0; }

auto String::starts_with(std::string_view prefix) const -> bool { return view().starts_with(prefix); }

auto operator<<(std::ostream &out, String const &str) -> std::ostream & {
    out << str.view();
    return out;
}

[[nodiscard]] auto Symbol::num() const noexcept -> NumberRef {
    assert(type() == SymbolType::number);
    return NumberRef{rep_};
}

[[nodiscard]] auto Symbol::str() const noexcept -> String {
    assert(type() == SymbolType::string);
    return String::from_rep(rep_ & ~TYPE_MASK);
}

[[nodiscard]] auto Symbol::name() const noexcept -> String {
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

[[nodiscard]] auto Symbol::args() const noexcept -> SymbolSpan {
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

[[nodiscard]] auto Symbol::has_classical_sign() const -> bool {
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

[[nodiscard]] auto Symbol::has_sign() const -> bool {
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
            return *num() < 0;
        }
        default: {
            return false;
        }
    }
}

[[nodiscard]] auto Symbol::flip_classical_sign() const -> std::optional<Symbol> {
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

[[nodiscard]] auto Symbol::type() const noexcept -> SymbolType {
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

auto operator<<(std::ostream &out, Symbol const &sym) -> std::ostream & {
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
            out << *sym.num();
            break;
        }
        case SymbolType::string: {
            Util::print_quoted(out, sym.str().view());
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
    return out;
}

// SymbolRef

SymbolRef::SymbolRef(Symbol sym) noexcept : sym_{sym} {
    auto rep = Symbol::to_rep(sym_);
    auto ptr = rep & ~TYPE_MASK;
    switch (rep & TYPE_MASK) {
        case REP_STR: {
            if (ptr != 0) {
                CharArray::from_repr(ptr).inc();
            }
            break;
        }
        case REP_SIGNED_FUN:
        case REP_FUN:
        case REP_TUP: {
            if (ptr != 0) {
                SymbolArray::from_repr(ptr).inc();
            }
            break;
        }
        // TODO: big ints
        default: {
            break;
        }
    }
}

SymbolRef::~SymbolRef() noexcept {
    auto rep = Symbol::to_rep(sym_);
    auto ptr = rep & ~TYPE_MASK;
    switch (rep & TYPE_MASK) {
        case REP_STR: {
            if (ptr != 0) {
                CharArray::from_repr(ptr).dec();
            }
            break;
        }
        case REP_SIGNED_FUN:
        case REP_FUN:
        case REP_TUP: {
            if (ptr != 0) {
                SymbolArray::from_repr(ptr).dec();
            }
            break;
        }
        // TODO: big ints
        default: {
            break;
        }
    }
}

void SymbolCollector::mark(Symbol const &sym) {
    // TODO:
    // - put unmarked symbols on a stack
    // - while stack not empty
    //   - pop and mark
    //   - add unmarked children to stack
    // - NOTE: referenced symbols are considered marked
    static_cast<void>(this);
    // NOLINTBEGIN(performance-avoid-endl)
    std::cerr << "todo: mark symbol: " << sym << std::endl;
    // NOLINTEND(performance-avoid-endl)
}

// SymbolStore

auto SymbolStore::sup() noexcept -> Symbol { return Symbol::from_rep(EXT_REP_SUP); }

auto SymbolStore::inf() noexcept -> Symbol { return Symbol::from_rep(EXT_REP_INF); }

auto SymbolStore::str(String str) noexcept -> Symbol { return Symbol::from_rep(String::to_rep(str) | REP_STR); }

auto SymbolStore::num(Number const &num) noexcept -> Symbol {
    if (auto res = num.as_int(); res) {
        return Symbol::from_rep(Number::to_repr(num));
    }
    return do_num(num);
}

auto SymbolStore::num(Number &&num) noexcept -> Symbol {
    if (auto res = num.as_int(); res) {
        return Symbol::from_rep(Number::to_repr(num));
    }
    return do_num(std::move(num));
}

auto SymbolStore::tup(SymbolSpan args) -> Symbol {
    if (args.empty()) {
        return Symbol::from_rep(REP_TUP);
    }
    return do_tup(args);
}

auto SymbolStore::fun(String name, SymbolSpan args, bool sign) -> Symbol {
    if (args.empty()) {
        auto rep = (sign ? REP_SIGNED_ID : REP_ID) | String::to_rep(name);
        return Symbol::from_rep(rep);
    }
    return do_fun(name, args, sign);
}

auto SymbolStore::string(std::string_view str) -> String {
    if (str.empty()) {
        return {};
    }
    return do_string(str);
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
        auto name = store_.string(prefix_ + std::to_string(num_));
        ++num_;
        if (!names_.contains(name)) {
            return name;
        }
    }
}

auto compare(Symbol a, Symbol b) -> int {
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
            return compare(*a.num(), *b.num());
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

} // namespace Gringo

// NOLINTEND(readability-magic-numbers,modernize-avoid-c-arrays,cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-type-reinterpret-cast,cppcoreguidelines-pro-bounds-constant-array-index,performance-no-int-to-ptr)
