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

// ========================================================================
// | 64 bit layout of symbol                                              |
// ========================================================================
// |                | 32b for number    | 29b for subtype   | 3b for type |
// |----------------+-------------------+-------------------+-------------|
// | Number         | used              | 0 for number      | 0           |
// | Inf            | unused            | 1 for inf         | 0           |
// | Sup            | unused            | 2 for sup         | 0           |
// |----------------+-------------------+-------------------+-------------|
// |                | 16b for size      | 45b for pointer   |             |
// |----------------+-------------------+-------------------+-------------|
// | String         | unused            | used              | 1           |
// | Tuple          | unused            | unused            | 2           |
// |                | < max             | pointer to small  | 3           |
// |                | = max             | pointer to large  | 3           |
// | Function       | unused            | pointer to string | 4           |
// |                | unused            | pointer to unary  | 5           |
// |                | < max             | pointer to small  | 6           |
// |                | = max             | pointer to large  | 6           |
// ========================================================================
// | 32 bit layout of symbol                                              |
// ========================================================================
// |                | 32b for number    | 29b for subtype   | 3b for type |
// |----------------+-------------------+-------------------+-------------|
// | Number         | used              | 0 for number      | 0           |
// | Inf            | unused            | 1 for inf         | 0           |
// | Sup            | unused            | 2 for sup         | 0           |
// |----------------+-------------------+-------------------+-------------|
// |                | 32b for pointer   | 29b for size      |             |
// |----------------+-------------------+-------------------+-------------|
// | String         | used              | unused            | 1           |
// | Tuple          | unused            | unused            | 2           |
// |                | pointer to small  | < max             | 3           |
// |                | pointer to large  | = max             | 3           |
// | Function       | pointer to string | unused            | 4           |
// |                | pointer to unary  | unuased           | 5           |
// |                | pointer to small  | < max             | 6           |
// |                | pointer to large  | = max             | 6           |
// ------------------------------------------------------------------------

namespace {

enum RepType : uint64_t {
    rep_number_or_constant = 0,
    rep_string = 1,
    rep_empty_tuple = 2,
    rep_tuple = 3,
    rep_empty_function = 4,
    rep_unary_function = 5,
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

    static constexpr uint64_t lower_mask = (1ULL >> 32) - 1;

    static constexpr size_t dynamic_size = 1ULL >> (64 - ptr_upper_shift);
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

struct SymbolArrDelete {
    void operator()(Symbol *ptr) const { ::operator delete[](ptr); }
};

using USymbolArr = std::unique_ptr<Symbol[], SymbolArrDelete>;

struct SymbolArrayEqual {
    SymbolArrayEqual(size_t size) : size{size} {}
    using is_transparent = void;
    auto operator()(USymbolArr const &a, Symbol const *b) const -> bool {
        return std::equal(a.get(), a.get() + size, b);
    }
    auto operator()(Symbol const *a, USymbolArr const &b) const -> bool { return operator()(b, a); }
    auto operator()(USymbolArr const &a, USymbolArr const &b) const -> bool { return a.get() == b.get(); }
    size_t size;
};

struct SymbolArrayHash {
    SymbolArrayHash(size_t size) : size{size} {}
    auto operator()(USymbolArr const &a) const -> size_t { return operator()(a.get()); }
    auto operator()(Symbol const *a) const -> size_t {
        std::string_view rep(reinterpret_cast<char const *>(a), sizeof(Symbol) * size);
        return std::hash<std::string_view>{}(rep);
    }
    size_t size;
};

struct LargeTuple {
    LargeTuple(size_t size, Symbol const *args) : size{size} { std::copy(args, args + size, this->args); }
    size_t size;
#ifdef __GNUC__
#pragma GCC diagnostic push
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wzero-length-array"
#else
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4200)
#endif
    Symbol args[0];
#ifdef __GNUC__
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
};

struct LargeTupleDelete {
    void operator()(LargeTuple *ptr) const { ::operator delete(ptr); }
};

using ULargeTuple = std::unique_ptr<LargeTuple, LargeTupleDelete>;

struct LargeTupleEqual {
    using is_transparent = void;
    auto operator()(ULargeTuple const &a, SymbolSpan const &b) const -> bool {
        return a->size == b.size() && std::equal(a->args, a->args + a->size, b.data());
    }
    auto operator()(SymbolSpan const &a, ULargeTuple const &b) const -> bool { return operator()(b, a); }
    auto operator()(ULargeTuple const &a, ULargeTuple const &b) const -> bool { return a.get() == b.get(); }
};

struct LargeTupleHash {
    auto operator()(ULargeTuple const &a) const -> size_t { return operator()(SymbolSpan{a->args, a->size}); }
    auto operator()(SymbolSpan const &a) const -> size_t {
        std::string_view rep(reinterpret_cast<char const *>(a.data()), sizeof(Symbol) * a.size());
        return std::hash<std::string_view>{}(rep);
    }
};

struct UnaryFunction {
    String name;
    Symbol arg;
};

using UUnaryFunction = std::unique_ptr<UnaryFunction>;

struct UnaryFunctionEqual {
    using is_transparent = void;
    auto operator()(UUnaryFunction const &a, UnaryFunction const &b) const -> bool {
        return a->name == b.name && a->arg == b.arg;
    }
    auto operator()(UnaryFunction const &a, UUnaryFunction const &b) const -> bool { return operator()(b, a); }
    auto operator()(UUnaryFunction const &a, UUnaryFunction const &b) const -> bool { return a.get() == b.get(); }
};

struct UnaryFunctionHash {
    auto operator()(UUnaryFunction const &a) const -> size_t { return operator()(*a); }
    auto operator()(UnaryFunction const &a) const -> size_t { return Util::value_hash(a.name, a.arg); }
};

struct SmallFunction {
    String name;
    Symbol const *args;
};

using USmallFunction = std::unique_ptr<SmallFunction>;

struct SmallFunctionEqual {
    using is_transparent = void;
    auto operator()(USmallFunction const &a, SmallFunction const &b) const -> bool {
        return a->name == b.name && a->args == b.args;
    }
    auto operator()(SmallFunction const &a, USmallFunction const &b) const -> bool { return operator()(b, a); }
    auto operator()(USmallFunction const &a, USmallFunction const &b) const -> bool { return a.get() == b.get(); }
};

struct SmallFunctionHash {
    auto operator()(USmallFunction const &a) const -> size_t { return operator()(*a); }
    auto operator()(SmallFunction const &a) const -> size_t {
        return Util::value_hash(a.name, reinterpret_cast<uintptr_t>(a.args));
    }
};

struct LargeFunction {
    String name;
    LargeTuple const *args;
};

using ULargeFunction = std::unique_ptr<LargeFunction>;

struct LargeFunctionEqual {
    using is_transparent = void;
    auto operator()(ULargeFunction const &a, LargeFunction const &b) const -> bool {
        return a->name == b.name && a->args == b.args;
    }
    auto operator()(LargeFunction const &a, ULargeFunction const &b) const -> bool { return operator()(b, a); }
    auto operator()(ULargeFunction const &a, ULargeFunction const &b) const -> bool { return a.get() == b.get(); }
};

struct LargeFunctionHash {
    auto operator()(ULargeFunction const &a) const -> size_t { return operator()(*a); }
    auto operator()(LargeFunction const &a) const -> size_t {
        return Util::value_hash(a.name, reinterpret_cast<uintptr_t>(a.args));
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

class DefaultSymbolStore : public SymbolStore {
  public:
    [[nodiscard]] auto function(String name, SymbolSpan args) -> Symbol override {
        auto size = args.size();
        if (size == 0) {
            auto rep = rep_empty_function | (static_cast<uint64_t>(String::to_rep(name)) << MS::ptr_shift);
            return Symbol::from_rep(rep);
        }
        if (size == 1) {
            // store a unary function
            UnaryFunction fun{name, args.front()};
            auto it = unary_funs_.find(fun);
            if (it != unary_funs_.end()) {
                it = unary_funs_.insert(std::make_unique<UnaryFunction>(fun)).first;
            }
            auto ptr = reinterpret_cast<uint64_t>(it->get());
            auto rep = rep_unary_function | (static_cast<uint64_t>(ptr) << MS::ptr_shift);
            return Symbol::from_rep(rep);
        }
        if (size + 1 < MS::dynamic_size) {
            // store a small size function
            auto &[tuples, funs] = size <= MS::small_size
                                       ? very_small_funs_[size]
                                       : small_funs_.try_emplace(args.size(), args.size()).first->second;
            // get unique arguments
            auto jt = tuples.find(args.data());
            if (jt == tuples.end()) {
                auto data = USymbolArr(reinterpret_cast<Symbol *>(::operator new[](size)));
                std::copy(args.begin(), args.end(), data.get());
                jt = tuples.insert(std::move(data)).first;
            }
            SmallFunction fun{name, jt->get()};
            auto it = funs.find(fun);
            if (it != funs.end()) {
                it = funs.insert(std::make_unique<SmallFunction>(fun)).first;
            }
            auto ptr = reinterpret_cast<uint64_t>(it->get());
            auto rep = rep_function | (static_cast<uint64_t>(size - 2) << MS::ptr_upper_shift) |
                       (static_cast<uint64_t>(ptr) << MS::ptr_shift);
            return Symbol::from_rep(rep);
        }
        // store a large size function
        auto &[tuples, funs] = large_funs_.try_emplace(args.size()).first->second;
        auto jt = tuples.find(args);
        if (jt == tuples.end()) {
            auto *mem = ::operator new(sizeof(LargeTuple) + size * sizeof(Symbol));
            jt = tuples.insert(ULargeTuple{new (mem) LargeTuple{size, args.data()}}).first;
        }
        LargeFunction fun{name, jt->get()};
        auto it = funs.find(fun);
        if (it != funs.end()) {
            it = funs.insert(std::make_unique<LargeFunction>(fun)).first;
        }
        auto ptr = reinterpret_cast<uint64_t>(it->get());
        auto rep = rep_function | (static_cast<uint64_t>(MS::dynamic_size - 1) << MS::ptr_upper_shift) |
                   (static_cast<uint64_t>(ptr) << MS::ptr_shift);
        return Symbol::from_rep(rep);
    }

    [[nodiscard]] auto tuple(SymbolSpan args) -> Symbol override {
        auto size = args.size();
        if (size == 0) {
            return Symbol::from_rep(rep_empty_tuple);
        }
        // store small tuples
        if (size < MS::dynamic_size) {
            // store a small size function
            auto &[tuples, funs] = size <= MS::small_size
                                       ? very_small_funs_[size]
                                       : small_funs_.try_emplace(args.size(), args.size()).first->second;
            // get unique arguments
            auto it = tuples.find(args.data());
            if (it == tuples.end()) {
                auto data = USymbolArr(reinterpret_cast<Symbol *>(::operator new[](size)));
                std::copy(args.begin(), args.end(), data.get());
                it = tuples.insert(std::move(data)).first;
            }
            auto ptr = reinterpret_cast<uint64_t>(it->get());
            auto rep = rep_tuple | (static_cast<uint64_t>(size - 1) << MS::ptr_upper_shift) |
                       (static_cast<uint64_t>(ptr) << MS::ptr_shift);
            return Symbol::from_rep(rep);
        }
        // store large tuples
        auto &[tuples, funs] = large_funs_.try_emplace(args.size()).first->second;
        auto it = tuples.find(args);
        if (it == tuples.end()) {
            auto *mem = ::operator new(sizeof(LargeTuple) + size * sizeof(Symbol));
            it = tuples.insert(ULargeTuple(new (mem) LargeTuple{size, args.data()})).first;
        }
        auto ptr = reinterpret_cast<uint64_t>(it->get());
        auto rep = rep_tuple | (static_cast<uint64_t>(MS::dynamic_size - 1) << MS::ptr_upper_shift) |
                   (static_cast<uint64_t>(ptr) << MS::ptr_shift);
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

  private:
    struct SmallFunStore {
        SmallFunStore(size_t size) : tuples{0, SymbolArrayHash{size}, SymbolArrayEqual{size}} {}
        hash_set<USymbolArr, SymbolArrayHash, SymbolArrayEqual> tuples;
        hash_set<USmallFunction, SmallFunctionHash, SmallFunctionEqual> funs;
    };
    struct LargeFunStore {
        hash_set<ULargeTuple, LargeTupleHash, LargeTupleEqual> tuples;
        hash_set<ULargeFunction, LargeFunctionHash, LargeFunctionEqual> funs;
    };
    hash_set<UString, UStringHash, UStringEqual> strings_;
    hash_set<UUnaryFunction, UnaryFunctionHash, UnaryFunctionEqual> unary_funs_;
    std::array<SmallFunStore, MS::small_size> very_small_funs_ = {1, 2, 3, 4, 5, 6, 7, 8};
    std::map<size_t, SmallFunStore> small_funs_;
    std::map<size_t, LargeFunStore> large_funs_;
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

[[nodiscard]] auto Symbol::args() const noexcept -> SymbolSpan {
    assert(type() == SymbolType::function || type() == SymbolType::tuple);
    auto type = rep_ & MS::type_mask;
    // case: empty tuple or function
    if (type == rep_empty_tuple || type == rep_empty_function) {
        return {};
    }
    uintptr_t ptr = (rep_ & MS::ptr_mask) >> MS::ptr_shift;
    // case: unary function
    if (type == rep_unary_function) {
        auto const *fun = reinterpret_cast<UnaryFunction *>(ptr);
        return {&fun->arg, 1};
    }
    // case: function with at least two arguments
    size_t size = ((rep_ & MS::ptr_upper_mask) >> MS::ptr_upper_shift);
    // case: non empty function
    if (type == rep_function) {
        // case: size could be stored separately
        if (size + 1 != MS::dynamic_size) {
            auto const *fun = reinterpret_cast<SmallFunction *>(ptr);
            return {fun->args, size};
        }
        // case: size is stored along with the function
        auto const *fun = reinterpret_cast<LargeFunction *>(ptr);
        return {fun->args->args, fun->args->size + 2};
    }
    // case: non empty tuple
    assert(type == rep_tuple);
    // case: size could be stored separately
    if (size + 1 != MS::dynamic_size) {
        auto const *args = reinterpret_cast<Symbol const *>(ptr);
        return {args, size + 1};
    }
    // case: size is stored along with the tuple
    auto *tuple = reinterpret_cast<LargeTuple *>(ptr);
    return {tuple->args, tuple->size};
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
        case rep_empty_tuple:
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
