#include <cstring>
#include <forward_list>
#include <map>
#include <mutex>

#include <tsl/hopscotch_set.h>

#include <util/print.hh>

#include <symbol.hh>

#include <iostream>

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
    rep_signed_id = 3,
    rep_id = 4,
    rep_signed_function = 5,
    rep_function = 6,
    rep_bigint = 7,
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

struct Sized {
    uint64_t size : 63;
    uint64_t tag : 1;
};

auto alloc_size(void *mem) -> size_t { return (reinterpret_cast<Sized *>(mem) - 1)->size; }

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

    ~SlottedAlloc() noexcept {
        for (auto &head : free_list_) {
            free_head_(head);
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

class SymbolArray {
  public:
    SymbolArray() = default;

    template <class Alloc> void init(Alloc &alloc, SymbolSpan symbols) {
        static_assert(alignof(Symbol) <= alignof(uint64_t));
        repr_ = reinterpret_cast<Symbol *>(alloc.alloc(symbols.size() * sizeof(Symbol)));
        std::copy(symbols.begin(), symbols.end(), repr_);
    }

    template <class Alloc> void init(Alloc &alloc, Symbol name, SymbolSpan symbols) {
        static_assert(alignof(Symbol) <= alignof(uint64_t));
        repr_ = reinterpret_cast<Symbol *>(alloc.alloc((symbols.size() + 1) * sizeof(Symbol)));
        *repr_ = name;
        std::copy(symbols.begin(), symbols.end(), repr_ + 1);
    }

    template <class Alloc> void destroy(Alloc &alloc) noexcept {
        alloc.dealloc(repr_);
        repr_ = nullptr;
    }

    [[nodiscard]] auto span() const noexcept -> SymbolSpan { return {repr_, size()}; }
    [[nodiscard]] auto head() const noexcept -> Symbol { return *repr_; }
    [[nodiscard]] auto tail() const noexcept -> SymbolSpan { return {repr_ + 1, size() - 1}; }
    [[nodiscard]] auto data() const noexcept -> Symbol * { return repr_; }
    [[nodiscard]] auto size() const noexcept -> size_t { return alloc_size(repr_) / sizeof(Symbol); }

  private:
    Symbol *repr_ = nullptr;
};

struct SymbolArrayHash {
    auto operator()(SymbolSpan fun) const -> size_t { return operator()(std::make_pair(fun.front(), fun.subspan(1))); }
    auto operator()(std::pair<Symbol, SymbolSpan> fun) const -> size_t {
        return Util::value_hash(fun.first, std::string_view{reinterpret_cast<char const *>(fun.second.data()),
                                                            fun.second.size() * sizeof(Symbol)});
    }

    auto operator()(SymbolArray const &fun) const -> size_t {
        return operator()(std::make_pair(fun.head(), fun.tail()));
    }
};

struct SymbolArrayEqual {
    using is_transparent = void;
    auto operator()(SymbolSpan a, SymbolSpan b) const -> bool {
        return std::equal(a.begin(), a.end(), b.begin(), b.end());
    }

    auto operator()(SymbolArray a, SymbolSpan b) const -> bool { return operator()(a.span(), b); }
    auto operator()(SymbolSpan a, SymbolArray b) const -> bool { return operator()(a, b.span()); }

    auto operator()(SymbolArray a, std::pair<Symbol, SymbolSpan> b) const -> bool {
        return a.head() == b.first && operator()(a.tail(), b.second);
    }
    auto operator()(std::pair<Symbol, SymbolSpan> a, SymbolArray b) const -> bool { return operator()(b, a); }
    auto operator()(SymbolArray a, SymbolArray b) const -> bool { return a.data() == b.data(); }
};

class CharArray {
  public:
    CharArray() = default;

    template <class Alloc> void init(Alloc &alloc, std::string_view str) {
        static_assert(alignof(char) <= alignof(uint64_t));
        repr_ = reinterpret_cast<char *>(alloc.alloc((str.size() + 1) * sizeof(char)));
        std::copy(str.begin(), str.end(), repr_);
        repr_[str.size()] = '\0';
    }

    template <class Alloc> void destroy(Alloc &alloc) noexcept {
        alloc.dealloc(repr_);
        repr_ = nullptr;
    }

    [[nodiscard]] auto view() const noexcept -> std::string_view { return {repr_, size()}; }
    [[nodiscard]] auto data() const noexcept -> char const * { return repr_; }
    [[nodiscard]] auto size() const noexcept -> size_t { return alloc_size(repr_) / sizeof(char) - 1; }

  private:
    char *repr_ = nullptr;
};

struct CharArrayEqual {
    using is_transparent = void;
    auto operator()(CharArray a, std::string_view b) const -> bool { return a.view() == b; }
    auto operator()(std::string_view a, CharArray b) const -> bool { return a == b.view(); }
    auto operator()(CharArray a, CharArray b) const -> bool { return a.view() == b.view(); }
};

struct CharArrayHash {
    auto operator()(CharArray a) const -> size_t { return operator()(a.view()); }
    auto operator()(std::string_view a) const -> size_t { return std::hash<std::string_view>{}(a); }
};

template <class Allocator> class DefaultSymbolStore : public SymbolStore {
  public:
    [[nodiscard]] auto store_num(Number const &num) noexcept -> Symbol override {
        auto jt = numbers_.find(num);
        if (jt == numbers_.end()) {
            jt = numbers_.insert(num).first;
        }
        return Symbol::from_rep(Number::to_repr(*jt));
    }

    [[nodiscard]] auto store_num(Number &&num) noexcept -> Symbol override {
        auto jt = numbers_.insert(std::move(num)).first;
        return Symbol::from_rep(Number::to_repr(*jt));
    }

    [[nodiscard]] auto fun(String name, SymbolSpan args, bool sign) -> Symbol override {
        auto size = args.size();
        if (size == 0) {
            auto rep = (sign ? rep_signed_id : rep_id) | (static_cast<uint64_t>(String::to_rep(name)) << MS::ptr_shift);
            return Symbol::from_rep(rep);
        }
        auto fun = std::make_pair(SymbolStore::str(name), args);
        auto jt = tuples_.find(fun);
        if (jt == tuples_.end()) {
            jt = insert_(tuples_, fun.first, fun.second);
        }
        auto rep = reinterpret_cast<uint64_t>(jt->data()) | (sign ? rep_signed_function : rep_function);
        return Symbol::from_rep(rep);
    }

    [[nodiscard]] auto tup(SymbolSpan args) -> Symbol override {
        // Almost the same as for function except that the name does not have to be stored separately.
        auto size = args.size();
        if (size == 0) {
            return Symbol::from_rep(rep_tuple);
        }
        auto jt = tuples_.find(args);
        if (jt == tuples_.end()) {
            jt = insert_(tuples_, args);
        }
        auto rep = reinterpret_cast<uint64_t>(jt->data()) | rep_tuple;
        return Symbol::from_rep(rep);
    }

    [[nodiscard]] auto string(std::string_view str) -> String override {
        auto it = strings_.find(str);
        if (it == strings_.end()) {
            it = insert_(strings_, str);
        }
        return String::from_rep(reinterpret_cast<uintptr_t>(it->data()));
    }

    void clear() {
        numbers_ = NumberSet{};
        clear_(strings_);
        clear_(tuples_);
    }

    ~DefaultSymbolStore() noexcept override { clear(); }

  private:
    using NumberSet = hash_set<Number>;
    using StringSet = hash_set<CharArray, CharArrayHash, CharArrayEqual>;
    using TupleSet = hash_set<SymbolArray, SymbolArrayHash, SymbolArrayEqual>;

    template <class T, class... Args> auto insert_(T &table, Args &&...args) -> typename T::iterator {
        typename T::value_type arr;
        try {
            arr.init(alloc_, std::forward<Args>(args)...);
            return table.emplace(arr).first;
        } catch (...) {
            arr.destroy(alloc_);
            throw;
        }
    }

    template <class T> void clear_(T &table) noexcept {
        for (auto arr : table) {
            arr.destroy(alloc_);
        }
        table.clear();
    }

    Allocator alloc_;
    NumberSet numbers_;
    StringSet strings_;
    TupleSet tuples_;
};

//! Simple thread-safe symbol store.
//!
//! More fine-grained locking is possible and also a shared lock is interesting.
template <class Alloc> class SharedSymbolStore : public SymbolStore {
  public:
    [[nodiscard]] auto store_num(Number const &num) noexcept -> Symbol override {
        std::unique_lock ulock{mutex_};
        return store_.store_num(num);
    }

    [[nodiscard]] auto store_num(Number &&num) noexcept -> Symbol override {
        std::unique_lock ulock{mutex_};
        return store_.store_num(std::move(num));
    }

    [[nodiscard]] auto fun(String name, SymbolSpan args, bool sign) -> Symbol override {
        std::unique_lock ulock{mutex_};
        return store_.fun(name, args, sign);
    }

    [[nodiscard]] auto tup(SymbolSpan args) -> Symbol override {
        std::unique_lock ulock{mutex_};
        return store_.tup(args);
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
    DefaultSymbolStore<Alloc> store_;
};

auto default_symbol_store_() -> USymbolStore & {
    static USymbolStore store;
    return store;
}

} // namespace

auto String::c_str() const -> const char * { return reinterpret_cast<char const *>(rep_); }

auto String::view() const -> std::string_view { return {c_str(), size()}; }

auto String::empty() const -> bool { return *c_str() == '\0'; }

auto String::size() const -> size_t { return alloc_size(reinterpret_cast<void *>(rep_)) - 1; }

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
    return String::from_rep((rep_ & ~MS::type_mask) >> MS::ptr_shift);
}

[[nodiscard]] auto Symbol::name() const noexcept -> String {
    assert(type() == SymbolType::function);
    switch (rep_ & MS::type_mask) {
        case rep_id:
        case rep_signed_id: {
            return String::from_rep((rep_ & ~MS::type_mask) >> MS::ptr_shift);
        }
        default: {
            return reinterpret_cast<Symbol *>(rep_ & ~MS::type_mask)->str();
        }
    }
}

[[nodiscard]] auto Symbol::args() const noexcept -> SymbolSpan {
    switch (rep_ & MS::type_mask) {
        case rep_signed_id:
        case rep_id: {
            return SymbolSpan{};
        }
        case rep_signed_function:
        case rep_function: {
            auto *ptr = reinterpret_cast<Symbol *>(rep_ & ~MS::type_mask);
            auto size = alloc_size(ptr) / sizeof(Symbol);
            return SymbolSpan{ptr + 1, size - 1};
        }
        default: {
            assert((rep_ & MS::type_mask) == rep_tuple);
            auto *ptr = reinterpret_cast<Symbol *>(rep_ & ~MS::type_mask);
            auto size = ptr != nullptr ? alloc_size(ptr) / sizeof(Symbol) : 0;
            return SymbolSpan{ptr, size};
        }
    }
}

[[nodiscard]] auto Symbol::has_classical_sign() const -> bool {
    switch (rep_ & MS::type_mask) {
        case rep_signed_id:
        case rep_signed_function: {
            return true;
        }
        default: {
            return false;
        }
    }
}

[[nodiscard]] auto Symbol::has_sign() const -> bool {
    switch (rep_ & MS::type_mask) {
        case rep_number_or_constant: {
            if ((rep_ & MS::lower_mask) >> MS::type_size == sub_rep_number) {
                return static_cast<int>(rep_ >> 32) < 0;
            }
            return false;
        }
        case rep_signed_id:
        case rep_signed_function: {
            return true;
        }
        case rep_bigint: {
            return *num() < 0;
        }
        default: {
            return false;
        }
    }
}

[[nodiscard]] auto Symbol::flip_classical_sign() const -> std::optional<Symbol> {
    switch (rep_ & MS::type_mask) {
        case rep_signed_id: {
            return Symbol{(rep_ & ~MS::type_mask) | rep_id};
        }
        case rep_signed_function: {
            return Symbol{(rep_ & ~MS::type_mask) | rep_function};
        }
        case rep_id: {
            return Symbol{(rep_ & ~MS::type_mask) | rep_signed_id};
        }
        case rep_function: {
            return Symbol{(rep_ & ~MS::type_mask) | rep_signed_function};
        }
        default: {
            return std::nullopt;
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
        case rep_bigint: {
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

auto SymbolStore::sup() noexcept -> Symbol {
    uint64_t rep = (sub_rep_sup << MS::type_size) | rep_number_or_constant;
    return Symbol::from_rep(rep);
}

auto SymbolStore::inf() noexcept -> Symbol {
    uint64_t rep = (sub_rep_inf << MS::type_size) | rep_number_or_constant;
    return Symbol::from_rep(rep);
}

auto SymbolStore::str(String str) noexcept -> Symbol {
    uint64_t rep = reinterpret_cast<uint64_t>(String::to_rep(str)) | rep_string;
    return Symbol::from_rep(rep);
}

auto SymbolStore::num(Number const &num) noexcept -> Symbol {
    if (auto res = num.as_int(); res) {
        return Symbol::from_rep(Number::to_repr(num));
    }
    return store_num(num);
}

auto SymbolStore::num(Number &&num) noexcept -> Symbol {
    if (auto res = num.as_int(); res) {
        return Symbol::from_rep(Number::to_repr(num));
    }
    return store_num(std::move(num));
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

} // namespace Gringo

// NOLINTEND(readability-magic-numbers,modernize-avoid-c-arrays)
