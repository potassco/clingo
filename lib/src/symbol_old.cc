#include <cstring>
#include <map>
#include <mutex>
#include <shared_mutex>

#include <tsl/hopscotch_set.h>

#include <symbol_old.hh>

// NOLINTBEGIN(readability-magic-numbers,modernize-avoid-c-arrays)

namespace Gringo {

template <class Key, class Hash = Util::value_hasher<Key>, class KeyEqual = std::equal_to<>,
          class Allocator = std::allocator<Key>, unsigned int NeighborhoodSize = 62, bool StoreHash = false> // NOLINT
using hash_set = tsl::hopscotch_set<Key, Hash, KeyEqual, Allocator, NeighborhoodSize, StoreHash>;

// Note that the implementation switches to the large representation if the
// upper 16 bits of the pointer are not zero. However, byte aligned pointers
// are a hard requirement. Furthermore, the design is targeted toward 64bit
// architectures. There are probably better ways to store symbols on 32bit
// architectures.
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
// | - small        | used              | pointer to small  | 2           |
// | - empty        | unused            | 0                 | 3           |
// | - large        | unused            | pointer to large  | 3           |
// | Function       |                   |                   |             |
// | - id           | unused            | pointer to string | 4           |
// | - small        | used              | pointer to small  | 5           |
// | - large        | unused            | pointer to large  | 6           |
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
// | - small        | pointer to small  | the size          | 2           |
// | - empty        | 0                 | unused            | 3           |
// | - large        | pointer to large  | unused            | 3           |
// | Function       |                   |                   |             |
// | - id           | pointer to string | unused            | 4           |
// | - small        | pointer to small  | the size          | 5           |
// | - large        | pointer to large  | unused            | 6           |
// ------------------------------------------------------------------------

namespace {

enum RepType : uint64_t {
    rep_number_or_constant = 0,
    rep_string = 1,
    rep_small_tuple = 2,
    rep_tuple = 3,
    rep_id = 4,
    rep_small_function = 5,
    rep_function = 6,
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

    static constexpr int ptr_upper_shift = 48;
    static constexpr uint64_t ptr_upper_mask = ~((1ULL << ptr_upper_shift) - 1);

    static constexpr uint64_t ptr_mask = ~(type_mask | ptr_upper_mask);
    static constexpr int ptr_shift = 0;

    static constexpr uint64_t lower_mask = (1ULL << 32) - 1;

    static constexpr size_t dynamic_size = 1ULL << (64 - ptr_upper_shift);
    static constexpr size_t small_size = 8ULL;
};

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-const-variable"
#endif

template <> struct MS_<4> {
    static constexpr int type_size = 3;
    static constexpr uint64_t type_mask = (1ULL << type_size) - 1;

    static constexpr int ptr_upper_shift = 3;
    static constexpr uint64_t ptr_upper_mask = ((1ULL << (32 - ptr_upper_shift)) - 1) << ptr_upper_shift;

    static constexpr uint64_t ptr_mask = ~0ULL;
    static constexpr int ptr_shift = 32;

    static constexpr uint64_t lower_mask = (1ULL >> 32) - 1;

    static constexpr size_t dynamic_size = 1ULL >> (32 - type_size);
    static constexpr size_t small_size = 8ULL;
};

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

using MS = MS_<sizeof(uint64_t)>;

template <class T> auto set_flags(T *ptr, uintptr_t flags) -> uintptr_t {
    return reinterpret_cast<uintptr_t>(ptr) | flags;
}

auto get_flags(uintptr_t rep) -> uintptr_t { return rep & static_cast<uintptr_t>(7); }

template <class T> auto get_ptr(uintptr_t ptr) -> T * {
    return reinterpret_cast<T *>(ptr & ~static_cast<uintptr_t>(7));
}

class SymbolArray {
  public:
    SymbolArray(SymbolSpan symbols, bool tagged) : repr_{init_(symbols, tagged)} {}
    SymbolArray(Symbol name, SymbolSpan symbols, bool tagged) : repr_{init_(name, symbols, tagged)} {}
    SymbolArray(SymbolArray const &other) = delete;
    SymbolArray(SymbolArray &&other) noexcept : repr_{other.repr_} { other.repr_ = 0; }

    auto operator=(SymbolArray const &other) -> SymbolArray & = delete;
    auto operator=(SymbolArray &&other) noexcept -> SymbolArray & {
        std::swap(repr_, other.repr_);
        return *this;
    }

    [[nodiscard]] auto get() const noexcept -> Symbol * { return get_ptr<Symbol>(repr_); }

    [[nodiscard]] auto has_size() const noexcept -> bool { return get_flags(repr_) != 0; }

    [[nodiscard]] auto size() const noexcept -> size_t {
        assert(has_size());
        return Symbol::to_rep(*(get() - 1));
    }

    ~SymbolArray() noexcept {
        auto *start = get();
        if (has_size()) {
            --start;
        }
        ::operator delete[](start);
    }

  private:
    static auto init_(SymbolSpan symbols, bool tagged) -> uintptr_t {
        uintptr_t flag = tagged ? 1 : 0;
        // It would be interesting to see the performance of a slotted allocator here.
        auto *data = reinterpret_cast<Symbol *>(::operator new[]((symbols.size() + flag) * sizeof(Symbol)));
        if (tagged) {
            *data++ = Symbol::from_rep(symbols.size());
        }
        std::copy(symbols.begin(), symbols.end(), data);
        return set_flags(data, flag);
    }

    static auto init_(Symbol name, SymbolSpan symbols, bool tagged) -> uintptr_t {
        uintptr_t flag = tagged ? 1 : 0;
        auto *data = reinterpret_cast<Symbol *>(::operator new[]((symbols.size() + 1 + flag) * sizeof(Symbol)));
        if (tagged) {
            *data++ = Symbol::from_rep(symbols.size());
        }
        *data = name;
        std::copy(symbols.begin(), symbols.end(), data + 1);
        return set_flags(data, flag);
    }

    uintptr_t repr_;
};

struct SymbolArrayHash {
    auto operator()(SymbolSpan fun) const -> size_t { return operator()({fun.front(), {fun.data() + 1, size - 1}}); }
    auto operator()(std::pair<Symbol, SymbolSpan> fun) const -> size_t {
        return Util::value_hash(fun.first, std::string_view{reinterpret_cast<char const *>(fun.second.data()),
                                                            fun.second.size() * sizeof(Symbol)});
    }
    auto operator()(SymbolArray const &fun) const -> size_t {
        return operator()({*fun.get(), {fun.get() + 1, size - 1}});
    }
    size_t size;
};

struct SymbolArrayEqual {
    using is_transparent = void;
    auto operator()(SymbolArray const &a, SymbolSpan b) const -> bool {
        return std::equal(b.begin(), b.end(), a.get());
    }
    auto operator()(SymbolSpan a, SymbolArray const &b) const -> bool { return operator()(b, a); }
    auto operator()(SymbolArray const &a, std::pair<Symbol, SymbolSpan> b) const -> bool {
        return *a.get() == b.first && std::equal(b.second.begin(), b.second.end(), a.get() + 1);
    }
    auto operator()(std::pair<Symbol, SymbolSpan> a, SymbolArray const &b) const -> bool { return operator()(b, a); }
    auto operator()(SymbolArray const &a, SymbolArray const &b) const -> bool { return a.get() == b.get(); }
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

class DefaultSymbolStore : public SymbolStore {
  public:
    [[nodiscard]] auto function(String name, SymbolSpan args) -> Symbol override {
        auto size = args.size();
        if (size == 0) {
            auto rep = rep_id | (static_cast<uint64_t>(String::to_rep(name)) << MS::ptr_shift);
            return Symbol::from_rep(rep);
        }
        // We need a tuple set for size + 1 to store the name and arguments of
        // the function. Since we handle the empty argument case above, we only
        // ever access tuples of size 2 and above.
        auto &tuples =
            size < MS::small_size ? small_tuples_[size] : tuples_.try_emplace(size + 1, size + 1).first->second;
        auto fun = std::make_pair(SymbolStore::string(name), args);
        auto jt = tuples.find(fun);
        if (jt == tuples.end()) {
            // Example: dynamic size = 1 and size = 1, then the size does not
            // have to be stored explicetly and size zero is stored in the
            // upper part of the pointer.
            jt = tuples.emplace(SymbolArray{fun.first, fun.second, size > MS::dynamic_size}).first;
            // Even though there are extensions that permit more than 48 bit
            // addresses, this has to be explicetly requested by an
            // application. Also kernel addresses where the upper bits are sign
            // extended should never be allocated here. Hence, the branch below
            // should never be taken.
            if (jt->has_size() && (reinterpret_cast<uintptr_t>(jt->get()) & MS::ptr_upper_mask) != 0) {
                tuples.erase(jt);
                jt = tuples.emplace(SymbolArray{fun.first, fun.second, true}).first;
            }
        }
        auto rep = reinterpret_cast<uint64_t>(jt->get()) |
                   (jt->has_size() ? rep_function
                                   : ((static_cast<uint64_t>(size - 2) << MS::ptr_upper_shift) | rep_small_function));
        return Symbol::from_rep(rep);
    }

    [[nodiscard]] auto tuple(SymbolSpan args) -> Symbol override {
        static_cast<void>(args);
        throw std::logic_error("TODO: reimplement me!!!");
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

  private:
    using StringSet = hash_set<UString, UStringHash, UStringEqual>;
    using TupleSet = hash_set<SymbolArray, SymbolArrayHash, SymbolArrayEqual>;

    static auto ts(size_t size) -> TupleSet { return TupleSet{0, SymbolArrayHash{size}}; }

    StringSet strings_;
    std::array<TupleSet, MS::small_size> small_tuples_ = {ts(1), ts(2), ts(3), ts(4), ts(5), ts(6), ts(7), ts(8)};
    std::map<size_t, TupleSet> tuples_;
};

//! Simple thread-safe symbol store.
//!
//! More fine-grained locking is possible and also a shared lock is interesting.
class SharedSymbolStore : public SymbolStore {
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

  private:
    std::mutex mutex_;
    DefaultSymbolStore store_;
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
    return String::from_rep((rep_ & MS::ptr_mask) >> MS::ptr_shift);
}

[[nodiscard]] auto Symbol::name() const noexcept -> String {
    assert(type() == SymbolType::function);
    return String::from_rep((rep_ & MS::ptr_mask) >> MS::ptr_shift);
}

[[nodiscard]] auto Symbol::args() noexcept -> SymbolSpan { throw std::logic_error("TODO: reimplement me!!!"); }

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
        case rep_small_tuple:
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
        default_store = std::make_unique<DefaultSymbolStore>();
    }
    return *default_store;
}

auto make_symbol_store(bool shared) -> USymbolStore {
    if (shared) {
        return std::make_unique<SharedSymbolStore>();
    }
    return std::make_unique<DefaultSymbolStore>();
}

} // namespace Gringo

// NOLINTEND(readability-magic-numbers,modernize-avoid-c-arrays)
