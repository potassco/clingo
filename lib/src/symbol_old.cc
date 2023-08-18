#include <map>
#include <mutex>
#include <shared_mutex>

#include <tsl/hopscotch_set.h>

#include <symbol_old.hh>

// NOLINTBEGIN(readability-magic-numbers,clang-diagnostic-zero-length-array,modernize-avoid-c-arrays)

namespace Gringo {

template <class Key, class Hash = Util::value_hasher<Key>, class KeyEqual = std::equal_to<>,
          class Allocator = std::allocator<Key>, unsigned int NeighborhoodSize = 62, bool StoreHash = false> // NOLINT
using hash_set = tsl::hopscotch_set<Key, Hash, KeyEqual, Allocator, NeighborhoodSize, StoreHash>;

// Layout of symbol.
// - 64 bit:
// ------------------------------------------------------------------------
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
// |                | = 0               | pointer to unary  | 5           |
// |                | < max             | pointer to small  | 5           |
// |                | = max             | pointer to large  | 5           |
// ------------------------------------------------------------------------
// - 32 bit:
// ------------------------------------------------------------------------
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
// |                | pointer to unary  | = 0               | 5           |
// |                | pointer to small  | < max             | 5           |
// |                | pointer to large  | = max             | 5           |
// ------------------------------------------------------------------------

enum RepType : uint64_t {
    rep_number_or_constant = 0,
    rep_string = 1,
    rep_empty_tuple = 2,
    rep_tuple = 3,
    rep_empty_function = 4,
    rep_function = 5,
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

struct SymbolArrayEqual {
    SymbolArrayEqual(size_t size) : size{size} {}
    using is_transparent = void;
    auto operator()(std::unique_ptr<Symbol const[]> const &a, Symbol const *b) const -> bool {
        return std::equal(a.get(), a.get() + size, b);
    }
    auto operator()(Symbol const *a, std::unique_ptr<Symbol const[]> const &b) const -> bool {
        return operator()(b, a);
    }
    auto operator()(std::unique_ptr<Symbol const[]> const &a, std::unique_ptr<Symbol const[]> const &b) const -> bool {
        return a.get() == b.get();
    }
    size_t size;
};

struct SymbolArrayHash {
    SymbolArrayHash(size_t size) : size{size} {}
    auto operator()(std::unique_ptr<Symbol const[]> const &a) const -> size_t { return operator()(a.get()); }
    auto operator()(Symbol const *a) const -> size_t {
        std::string_view rep(reinterpret_cast<char const *>(a), sizeof(Symbol) * size);
        return Util::value_hash(rep);
    }
    size_t size;
};

struct LargeTuple {
    LargeTuple(size_t size, Symbol const *args) : size{size} { std::copy(args, args + size, this->args); }
    size_t size;
    Symbol args[0];
};

struct LargeTupleEqual {
    using is_transparent = void;
    auto operator()(std::unique_ptr<LargeTuple> const &a, SymbolSpan const &b) const -> bool {
        return a->size == b.size() && std::equal(a->args, a->args + a->size, b.data());
    }
    auto operator()(SymbolSpan const &a, std::unique_ptr<LargeTuple> const &b) const -> bool {
        return operator()(b, a);
    }
    auto operator()(std::unique_ptr<LargeTuple> const &a, std::unique_ptr<LargeTuple> const &b) const -> bool {
        return a.get() == b.get();
    }
};

struct LargeTupleHash {
    auto operator()(std::unique_ptr<LargeTuple> const &a) const -> size_t {
        return operator()(std::span{a->args, a->size});
    }
    auto operator()(SymbolSpan const &a) const -> size_t {
        std::string_view rep(reinterpret_cast<char const *>(a.data()), sizeof(Symbol) * a.size());
        return Util::value_hash(rep);
    }
};

struct UnaryFunction {
    String name;
    Symbol arg;
};

struct UnaryFunctionEqual {
    using is_transparent = void;
    auto operator()(std::unique_ptr<UnaryFunction> const &a, UnaryFunction const &b) const -> bool {
        return a->name == b.name && a->arg == b.arg;
    }
    auto operator()(UnaryFunction const &a, std::unique_ptr<UnaryFunction> const &b) const -> bool {
        return operator()(b, a);
    }
    auto operator()(std::unique_ptr<UnaryFunction> const &a, std::unique_ptr<UnaryFunction> const &b) const -> bool {
        return a.get() == b.get();
    }
};

struct UnaryFunctionHash {
    auto operator()(std::unique_ptr<UnaryFunction> const &a) const -> size_t {
        return Util::value_hash(a->name, a->arg);
    }
    auto operator()(UnaryFunction const &a) const -> size_t { return Util::value_hash(a.name, a.arg); }
};

struct SmallFunction {
    String name;
    Symbol const *args;
};

struct SmallFunctionEqual {
    using is_transparent = void;
    auto operator()(std::unique_ptr<SmallFunction> const &a, SmallFunction const &b) const -> bool {
        return a->name == b.name && a->args == b.args;
    }
    auto operator()(SmallFunction const &a, std::unique_ptr<SmallFunction> const &b) const -> bool {
        return operator()(b, a);
    }
    auto operator()(std::unique_ptr<SmallFunction> const &a, std::unique_ptr<SmallFunction> const &b) const -> bool {
        return a.get() == b.get();
    }
};

struct SmallFunctionHash {
    auto operator()(std::unique_ptr<SmallFunction> const &a) const -> size_t { return operator()(*a); }
    auto operator()(SmallFunction const &a) const -> size_t { return std::hash<Symbol const *>{}(a.args); }
};

struct LargeFunction {
    String name;
    LargeTuple const *args;
};

struct LargeFunctionEqual {
    using is_transparent = void;
    auto operator()(std::unique_ptr<LargeFunction> const &a, LargeFunction const &b) const -> bool {
        return a->name == b.name && a->args == b.args;
    }
    auto operator()(LargeFunction const &a, std::unique_ptr<LargeFunction> const &b) const -> bool {
        return operator()(b, a);
    }
    auto operator()(std::unique_ptr<LargeFunction> const &a, std::unique_ptr<LargeFunction> const &b) const -> bool {
        return a.get() == b.get();
    }
    size_t size;
};

struct LargeFunctionHash {
    auto operator()(std::unique_ptr<LargeFunction> const &a) const -> size_t { return operator()(*a); }
    auto operator()(LargeFunction const &a) const -> size_t {
        return Util::value_hash(a.name, std::hash<LargeTuple const *>{}(a.args));
    }
};

struct String::Impl {
    Impl(size_t hash, std::string_view str) : hash{hash} {
        std::copy(str.begin(), str.end(), data);
        data[str.size()] = '\0';
    }
    size_t hash;
    char data[0];
};

using MS = MS_<sizeof(uint64_t)>;

[[nodiscard]] auto Symbol::num() const noexcept -> int32_t {
    assert(type() == SymbolType::number);
    return static_cast<uint32_t>(repr_ >> 32);
}

[[nodiscard]] auto Symbol::str() const noexcept -> String {
    assert(type() == SymbolType::string);
    uintptr_t ptr = (repr_ & MS::ptr_mask) >> MS::ptr_shift;
    return reinterpret_cast<String::Impl *>(ptr);
}

[[nodiscard]] auto Symbol::name() const noexcept -> String {
    assert(type() == SymbolType::function);
    uintptr_t ptr = (repr_ & MS::ptr_mask) >> MS::ptr_shift;
    return reinterpret_cast<String::Impl *>(ptr);
}

[[nodiscard]] auto Symbol::args() const noexcept -> SymbolSpan {
    assert(type() == SymbolType::function || type() == SymbolType::tuple);
    auto type = repr_ & MS::type_mask;
    // case: empty tuple or function
    if (type == rep_empty_tuple || type == rep_empty_function) {
        return {};
    }
    // case: non empty function or tuple
    uintptr_t ptr = (repr_ & MS::ptr_mask) >> MS::ptr_shift;
    size_t size = ((repr_ & MS::ptr_upper_mask) >> MS::ptr_upper_shift) + 1;
    // case: non empty function
    if (type == rep_function) {
        // case: a single argument that is stored along with the function
        if (size == 1) {
            auto const *fun = reinterpret_cast<UnaryFunction *>(ptr);
            return {&fun->arg, 1};
        }
        // case: size could be stored separately
        if (size < MS::dynamic_size) {
            auto const *fun = reinterpret_cast<SmallFunction *>(ptr);
            return {fun->args, size};
        }
        // case: size is stored along with the function
        auto const *fun = reinterpret_cast<LargeFunction *>(ptr);
        return {fun->args->args, fun->args->size};
    }
    // case: non empty tuple
    assert(type == rep_tuple);
    // case: size could be stored separately
    if (size < MS::dynamic_size) {
        auto const *args = reinterpret_cast<Symbol const *>(ptr);
        return {args, size};
    }
    // case: size is stored along with the tuple
    auto *tuple = reinterpret_cast<LargeTuple *>(ptr);
    return {tuple->args, tuple->size};
}

[[nodiscard]] auto Symbol::type() const noexcept -> SymbolType {
    switch (repr_ & MS::type_mask) {
        case rep_number_or_constant: {
            auto sub_type = (repr_ & MS::lower_mask) >> MS::type_size;
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

class UnlockedSymbolStore : public SymbolStore {
  public:
    [[nodiscard]] auto function(String name, SymbolSpan args) -> Symbol override {
        auto size = args.size();
        if (size == 0) {
            auto ptr = reinterpret_cast<uint64_t>(name.impl_);
            auto rep = rep_empty_function | (static_cast<uint64_t>(ptr) << MS::ptr_shift);
            return Symbol{rep};
        }
        if (size == 1) {
            // store a unary function
            UnaryFunction fun{name, args.front()};
            auto it = unary_funs_.find(fun);
            if (it != unary_funs_.end()) {
                it = unary_funs_.insert(std::make_unique<UnaryFunction>(fun)).first;
            }
            auto ptr = reinterpret_cast<uint64_t>(it->get());
            auto rep = rep_function | (static_cast<uint64_t>(ptr) << MS::ptr_shift);
            return Symbol{rep};
        }
        if (size < MS::dynamic_size) {
            // store a small size function
            auto &[tuples, funs] =
                size <= 8 ? very_small_funs_[size] : small_funs_.try_emplace(args.size(), args.size()).first->second;
            // get unique arguments
            auto jt = tuples.find(args.data());
            if (jt == tuples.end()) {
                auto data = std::unique_ptr<Symbol[]>(new Symbol[size]);
                std::copy(args.begin(), args.end(), data.get());
                jt = tuples.insert(std::move(data)).first;
            }
            SmallFunction fun{name, jt->get()};
            auto it = funs.find(fun);
            if (it != funs.end()) {
                it = funs.insert(std::make_unique<SmallFunction>(fun)).first;
            }
            auto ptr = reinterpret_cast<uint64_t>(it->get());
            auto rep = rep_function | (static_cast<uint64_t>(size - 1) << MS::ptr_upper_shift) |
                       (static_cast<uint64_t>(ptr) << MS::ptr_shift);
            return Symbol{rep};
        }
        // store a large size function
        auto &[tuples, funs] = large_funs_.try_emplace(args.size()).first->second;
        auto jt = tuples.find(args);
        if (jt == tuples.end()) {
            auto *mem = ::operator new(sizeof(LargeTuple) + size * sizeof(Symbol));
            jt = tuples.insert(std::unique_ptr<LargeTuple>(new (mem) LargeTuple{size, args.data()})).first;
        }
        LargeFunction fun{name, jt->get()};
        auto it = funs.find(fun);
        if (it != funs.end()) {
            it = funs.insert(std::make_unique<LargeFunction>(fun)).first;
        }
        auto ptr = reinterpret_cast<uint64_t>(it->get());
        auto rep = rep_function | (static_cast<uint64_t>(MS::dynamic_size - 1) << MS::ptr_upper_shift) |
                   (static_cast<uint64_t>(ptr) << MS::ptr_shift);
        return Symbol{rep};
    }

    [[nodiscard]] auto tuple(SymbolSpan args) -> Symbol override {
        auto size = args.size();
        if (size == 0) {
            return Symbol{rep_empty_tuple};
        }
        // store small tuples
        if (size < MS::dynamic_size) {
            // store a small size function
            auto &[tuples, funs] =
                size <= 8 ? very_small_funs_[size] : small_funs_.try_emplace(args.size(), args.size()).first->second;
            // get unique arguments
            auto it = tuples.find(args.data());
            if (it == tuples.end()) {
                auto data = std::unique_ptr<Symbol[]>(new Symbol[size]);
                std::copy(args.begin(), args.end(), data.get());
                it = tuples.insert(std::move(data)).first;
            }
            auto ptr = reinterpret_cast<uint64_t>(it->get());
            auto rep = rep_tuple | (static_cast<uint64_t>(size - 1) << MS::ptr_upper_shift) |
                       (static_cast<uint64_t>(ptr) << MS::ptr_shift);
            return Symbol{rep};
        }
        // store large tuples
        auto &[tuples, funs] = large_funs_.try_emplace(args.size()).first->second;
        auto it = tuples.find(args);
        if (it == tuples.end()) {
            auto *mem = ::operator new(sizeof(LargeTuple) + size * sizeof(Symbol));
            it = tuples.insert(std::unique_ptr<LargeTuple>(new (mem) LargeTuple{size, args.data()})).first;
        }
        auto ptr = reinterpret_cast<uint64_t>(it->get());
        auto rep = rep_tuple | (static_cast<uint64_t>(MS::dynamic_size - 1) << MS::ptr_upper_shift) |
                   (static_cast<uint64_t>(ptr) << MS::ptr_shift);
        return Symbol{rep};
    }

    [[nodiscard]] auto string(std::string_view str) -> String override {
        size_t hash = std::hash<std::string_view>{}(str);
        auto it = strings_.find(str, hash);
        if (it != strings_.end()) {
            return *it;
        }
        auto *mem = ::operator new(sizeof(String::Impl) + (str.size() + 1) * sizeof(char));
        auto res = String{new (mem) String::Impl{hash, str}};
        return *strings_.insert(res).first;
    }

    void destroy(String str) noexcept override {
        str.impl_->~Impl();
        delete str.impl_;
    }

    ~UnlockedSymbolStore() noexcept override {
        for (auto str : strings_) {
            delete str.impl_;
        }
    }

  private:
    struct SmallFunStore {
        SmallFunStore(size_t size) : tuples{0, SymbolArrayHash{size}, SymbolArrayEqual{size}} {}
        hash_set<std::unique_ptr<Symbol const[]>, SymbolArrayHash, SymbolArrayEqual> tuples;
        hash_set<std::unique_ptr<SmallFunction>, SmallFunctionHash, SmallFunctionEqual> funs;
    };
    struct LargeFunStore {
        hash_set<std::unique_ptr<LargeTuple>, LargeTupleHash, LargeTupleEqual> tuples;
        hash_set<std::unique_ptr<LargeFunction>, LargeFunctionHash, LargeFunctionEqual> funs;
    };
    // TODO: we can proceed as with the other cases and store a unique ptr here that takes care of cleaning the strings
    hash_set<String> strings_;
    hash_set<std::unique_ptr<UnaryFunction>, UnaryFunctionHash, UnaryFunctionEqual> unary_funs_;
    std::array<SmallFunStore, 8> very_small_funs_ = {1, 2, 3, 4, 5, 6, 7, 8};
    std::map<size_t, SmallFunStore> small_funs_;
    std::map<size_t, LargeFunStore> large_funs_;
};

/*
class LockedSymbolStore : public SymbolStore {
  public:
    [[nodiscard]] auto string(std::string_view str) -> String override {
        size_t hash = std::hash<std::string_view>{}(str);
        {
            std::shared_lock rlock{mut_strings_};
            auto it = strings_.find(str, hash);
            if (it != strings_.end()) {
                return *it;
            }
        }
        {
            auto res = String{new (::operator new(sizeof(String::Impl) + (str.size() + 1) * sizeof(char)))
                                  String::Impl{hash, str}};
            std::unique_lock ulock{mut_strings_};
            // Note: in multi-threaded mode there is the chance that the string has
            // been inserted in the mean time.
            auto it = strings_.find(str, hash);
            if (it != strings_.end()) {
                return *it;
            }
            return *strings_.insert(res).first;
        }
    }

    void destroy(String str) noexcept override {
        str.impl_->~Impl();
        delete str.impl_;
    }

    ~LockedSymbolStore() noexcept override {
        for (auto str : strings_) {
            delete str.impl_;
        }
    }

  private:
    mutable std::shared_mutex mut_strings_;
    hash_set<String> strings_;
};
*/

} // namespace Gringo

namespace std {

auto hash<Gringo::String>::operator()(Gringo::String str) const -> size_t { return str.impl_->hash; }

} // namespace std

// NOLINTEND(readability-magic-numbers,clang-diagnostic-zero-length-array,modernize-avoid-c-arrays)

/*
#include <algorithm>
#include <cstring>
#include <gringo/hash_set.hh>
#include <gringo/symbol.hh>
#include <iterator>
#include <mutex>

#ifdef _MSC_VER
#pragma warning(disable : 4200) // nonstandard extension used: zero-sized array in struct/union
#endif

namespace Gringo {

namespace {

// {{{1 auxiliary functions

constexpr const uint16_t upperMax = std::numeric_limits<uint16_t>::max();
constexpr const uint16_t lowerMax = 3;

uint16_t upper(uint64_t rep) { return rep >> 48; }

uint8_t lower(uint64_t rep) { return rep & 3; }

uintptr_t ptr(uint64_t rep) { return static_cast<uintptr_t>(rep & 0xFFFFFFFFFFFC); }

uint64_t combine(uint16_t u, uintptr_t ptr, uint8_t l) {
    static_cast<void>(lowerMax);
    assert(l <= lowerMax);
    return static_cast<uint64_t>(u) << 48 | ptr | l;
}

uint64_t combine(uint16_t u, int32_t num) {
    return static_cast<uint64_t>(u) << 48 | static_cast<uint64_t>(static_cast<uint32_t>(num));
}

uint64_t setUpper(uint16_t u, uint64_t rep) { return combine(u, 0, 0) | (rep & 0xFFFFFFFFFFFF); }

// uint64_t setLower(uint8_t l, uint8_t rep) {
//     assert(l <= lowerMax);
//     return combine(0, 0, l) | (rep & 0xFFFFFFFFFFFFFFFC);
// }

enum class SymbolType_ : uint8_t { Inf = 0, Num = 1, IdP = 2, IdN = 3, Str = 4, Fun = 5, Special = 6, Sup = 7 };

SymbolType_ symbolType_(uint64_t rep) { return static_cast<SymbolType_>(upper(rep)); }

template <class T> T const *cast(uint64_t rep) {
    return reinterpret_cast<T const *>(ptr(rep)); // NOLINT
}

String toString(uint64_t rep) { return String::fromRep(ptr(rep)); }

// {{{1 definition of Unique

template <class T> struct UniqueConstruct {
  public:
    using Set = hash_set<T, typename T::Hash, typename T::EqualTo>;

    template <class U> static T const &construct(U &&x) {
        // TODO: in C++17 this can use a read/write lock to not block reading threads
        size_t hash = typename T::Hash{}(x);
        std::lock_guard<std::mutex> g(mutex_);
        auto it = set_.find(x, hash);
        if (it != set_.end()) {
            return *it;
        }
        return *set_.insert(T{std::forward<U>(x), hash}).first;
    }

  private:
    static Set set_;          // NOLINT
    static std::mutex mutex_; // NOLINT
};

template <class T> typename UniqueConstruct<T>::Set UniqueConstruct<T>::set_; // NOLINT

template <class T> typename std::mutex UniqueConstruct<T>::mutex_; // NOLINT

template <class T, class U> T const &construct_unique(U &&x) {
    return UniqueConstruct<T>::construct(std::forward<U>(x));
}

// {{{1 definition of USig

class MSig {
  public:
    using Cons = std::pair<String, uint32_t>;

    struct Hash {
        size_t operator()(MSig const &sig) const { return sig.hash_; }
        size_t operator()(Cons const &sig) const { return hash_mix(get_value_hash(sig)); }
    };
    struct EqualTo {
        using is_transparent = void;
        template <class A, class B> size_t operator()(A const &a, B const &b) const {
            return name(a) == name(b) && arity(a) == arity(b);
        }
    };

    explicit MSig(Cons const &cons, size_t hash) : sig_{cons.first, cons.second}, hash_{hash} {}

    Cons const &as_sig() const { return sig_; }

  private:
    static String name(MSig const &a) { return a.sig_.first; }

    static String name(Cons const &a) { return a.first; }

    static uint32_t arity(MSig const &a) { return a.sig_.second; }

    static uint32_t arity(Cons const &a) { return a.second; }

    std::pair<String, uint32_t> sig_;
    size_t hash_;
};

uint64_t encodeSig(String name, uint32_t arity, bool sign) {
    uint8_t isign = sign ? 1 : 0;
    return arity < upperMax ? combine(arity, String::toRep(name), isign)
                            : combine(upperMax,
                                      reinterpret_cast<uintptr_t>(
                                          &construct_unique<MSig>(std::make_pair(name, arity)).as_sig()), // NOLINT
                                      isign);
}

// {{{1 definition of Fun

class Fun {
  public:
    Fun(Fun const &other) = delete;
    Fun(Fun &&other) noexcept = delete;
    Fun &operator=(Fun const &other) = delete;
    Fun &operator=(Fun &&other) noexcept = delete;

    static Fun *make(Sig sig, SymSpan args, size_t hash) {
        auto *mem = ::operator new(sizeof(Fun) + args.size * sizeof(Symbol));
        return new (mem) Fun(sig, args, hash); // NOLINT
    }

    void destroy() noexcept {
        this->~Fun();
        ::operator delete(this);
    }

    Sig sig() const { return sig_; }

    SymSpan args() const { return {args_, sig().arity()}; }

    size_t hash() const { return hash_; }

  private:
    ~Fun() noexcept = default;

    Fun(Sig sig, SymSpan args, size_t hash) noexcept : sig_(sig), hash_{hash} {
        std::memcpy(static_cast<void *>(args_), args.first, args.size * sizeof(Symbol));
    }

    Sig const sig_;
    size_t hash_;
    Symbol args_[0]; // NOLINT
};

class MFun {
  public:
    using Cons = std::pair<Sig, SymSpan>;

    struct Hash {
        size_t operator()(MFun const &a) const { return a.fun_->hash(); }
        size_t operator()(Cons const &a) const {
            return hash_mix(get_value_hash(a.first, hash_range(begin(a.second), end(a.second))));
        }
    };
    struct EqualTo {
        using is_transparent = void;
        template <class A, class B> size_t operator()(A const &a, B const &b) const {
            auto args_a = args(a);
            auto args_b = args(b);
            return sig(a) == sig(b) && std::equal(begin(args_a), end(args_a), begin(args_b));
        }
    };

    explicit MFun(Cons fun, size_t hash) : fun_{Fun::make(fun.first, fun.second, hash)} {}
    MFun() = delete;
    MFun(MFun const &other) = delete;
    MFun(MFun &&other) noexcept { std::swap(fun_, other.fun_); }
    MFun &operator=(MFun const &other) = delete;
    MFun &operator=(MFun &&other) noexcept {
        std::swap(fun_, other.fun_);
        return *this;
    }
    ~MFun() noexcept {
        if (fun_ != nullptr) {
            fun_->destroy();
        }
    }

    Fun const &as_fun() const { return *fun_; }

  private:
    static Sig sig(MFun const &a) { return a.fun_->sig(); }

    static Sig sig(Cons const &a) { return a.first; }

    static SymSpan args(MFun const &a) { return a.fun_->args(); }

    static SymSpan args(Cons const &a) { return a.second; }

    Fun *fun_ = nullptr;
};

// }}}1

} // namespace

// {{{1 definition of String

// {{{1 definition of MString

class String::Impl {
  public:
    class MString;

    Impl(Impl const &other) = delete;
    Impl(Impl &&other) noexcept = delete;
    Impl &operator=(Impl const &other) = delete;
    Impl &operator=(Impl &&other) noexcept = delete;

    static Impl *make(StringSpan const &span, size_t hash) {
        size_t n = span.size;
        auto *mem = ::operator new(sizeof(Impl) + (n + 1) * sizeof(char));
        return new (mem) Impl(span.first, n, hash); // NOLINT
    }

    static Impl *make(char const *str, size_t hash) {
        size_t n = strlen(str);
        auto *mem = ::operator new(sizeof(Impl) + (n + 1) * sizeof(char));
        return new (mem) Impl(str, n, hash); // NOLINT
    }

    void destroy() noexcept {
        this->~Impl();
        ::operator delete(this);
    }

    char const *str() const { return str_; }

    size_t hash() const { return hash_; }

  private:
    Impl(char const *str, size_t n, size_t hash) noexcept : hash_{hash} {
        std::memcpy(str_, str, n * sizeof(char));
        str_[n] = '\0'; // NOLINT
    }
    ~Impl() noexcept = default;

    size_t hash_;
    char str_[0]; // NOLINT
};

class String::Impl::MString {
  public:
    struct Hash {
        size_t operator()(MString const &str) const { return str.str_->hash(); }
        size_t operator()(char const *str) const { return hash_mix(strhash(str)); }
        size_t operator()(StringSpan const &str) const { return hash_mix(strhash(str)); }
    };
    struct EqualTo {
        using is_transparent = void;
        bool operator()(MString const &a, char const *b) const { return std::strcmp(a.as_impl()->str(), b) == 0; }
        bool operator()(MString const &a, StringSpan const &span_b) const {
            auto const *str_a = a.as_impl()->str();
            StringSpan span_a = {str_a, std::strlen(str_a)};
            return span_a.size == span_b.size && std::equal(begin(span_a), end(span_a), begin(span_b));
        }
        bool operator()(MString const &a, MString const &b) const { return operator()(a, b.as_impl()->str()); }
        bool operator()(char const *a, MString const &b) const { return operator()(b, a); }
        bool operator()(StringSpan const &a, MString const &b) const { return operator()(b, a); }
    };

    explicit MString(char const *str, size_t hash) : str_{String::Impl::make(str, hash)} {}
    explicit MString(StringSpan str, size_t hash) : str_{String::Impl::make(str, hash)} {}
    MString() = delete;
    MString(MString const &other) = delete;
    MString(MString &&other) noexcept { std::swap(str_, other.str_); }
    MString &operator=(MString const &other) = delete;
    MString &operator=(MString &&other) noexcept {
        std::swap(str_, other.str_);
        return *this;
    }
    ~MString() noexcept {
        if (str_ != nullptr) {
            str_->destroy();
        }
    }

    Impl *as_impl() const { return str_; }

  private:
    String::Impl *str_ = nullptr;
};

String::String(char const *str) : str_(construct_unique<Impl::MString>(str).as_impl()) {}

String::String(StringSpan str) : str_(construct_unique<Impl::MString>(str).as_impl()) {}

String::String(uintptr_t r) noexcept : str_(reinterpret_cast<Impl *>(r)) {} // NOLINT

const char *String::c_str() const { return str_->str(); }

bool String::empty() const {
    return *c_str() == '\0'; // NOLINT
}

size_t String::length() const { return std::strlen(c_str()); }

bool String::startsWith(char const *prefix) const { return std::strncmp(prefix, c_str(), strlen(prefix)) == 0; }

size_t String::hash() const {
    return reinterpret_cast<uintptr_t>(str_); // NOLINT
}

uintptr_t String::toRep(String s) noexcept {
    return reinterpret_cast<uintptr_t>(s.str_); // NOLINT
}

String String::fromRep(uintptr_t t) noexcept { return {t}; }

// {{{1 definition of Signature

Sig::Sig(String name, uint32_t arity, bool sign) : Sig{encodeSig(name, arity, sign)} {}

String Sig::name() const {
    uint16_t u = upper(rep());
    return u < upperMax ? toString(rep()) : cast<std::pair<String, uint32_t>>(rep())->first;
}

Sig Sig::flipSign() const { return Sig(rep() ^ 1); }

uint32_t Sig::arity() const {
    uint16_t u = upper(rep());
    return u < upperMax ? u : cast<std::pair<String, uint32_t>>(rep())->second;
}

bool Sig::sign() const { return lower(rep()) != 0; }

size_t Sig::hash() const { return get_value_hash(rep()); }

namespace {

bool less(Sig const &a, Sig const &b) {
    if (a.sign() != b.sign()) {
        return !a.sign() && b.sign();
    }
    if (a.arity() != b.arity()) {
        return a.arity() < b.arity();
    }
    return a.name() < b.name();
}

} // namespace

bool Sig::operator==(Sig s) const { return rep() == s.rep(); }

bool Sig::operator!=(Sig s) const { return rep() != s.rep(); }

bool Sig::operator<(Sig s) const { return *this != s && less(*this, s); }

bool Sig::operator>(Sig s) const { return *this != s && less(s, *this); }

bool Sig::operator<=(Sig s) const { return *this == s || less(*this, s); }

bool Sig::operator>=(Sig s) const { return *this == s || less(s, *this); }

// {{{1 definition of Symbol

// {{{2 construction

Symbol::Symbol() : Symbol(combine(static_cast<uint16_t>(SymbolType_::Special), 0, 0)) {}

Symbol Symbol::createInf() { return Symbol(combine(static_cast<uint16_t>(SymbolType_::Inf), 0, 0)); }

Symbol Symbol::createSup() { return Symbol(combine(static_cast<uint16_t>(SymbolType_::Sup), 0, 0)); }

Symbol Symbol::createNum(int num) { return Symbol(combine(static_cast<uint16_t>(SymbolType_::Num), num)); }

Symbol Symbol::createId(String val, bool sign) {
    return Symbol(combine(static_cast<uint16_t>(sign ? SymbolType_::IdN : SymbolType_::IdP), String::toRep(val), 0));
}

Symbol Symbol::createStr(String val) {
    return Symbol(combine(static_cast<uint16_t>(SymbolType_::Str), String::toRep(val), 0));
}

Symbol Symbol::createTuple(SymSpan args) { return createFun("", args, false); }

Symbol Symbol::createFun(String name, SymSpan args, bool sign) {
    return args.size != 0
               ? Symbol(combine(static_cast<uint16_t>(SymbolType_::Fun),
                                reinterpret_cast<uintptr_t>(
                                    &construct_unique<MFun>(
                                         std::make_pair(Sig(name, numeric_cast<uint32_t>(args.size), sign), args))
                                         .as_fun()), // NOLINT
                                0))
               : createId(name, sign);
}

// {{{2 inspection

SymbolType Symbol::type() const {
    auto t = symbolType_(rep_);
    switch (t) {
        case SymbolType_::IdP:
        case SymbolType_::IdN: {
            return SymbolType::Fun;
        }
        default: {
            return static_cast<SymbolType>(t);
        }
    }
}

int32_t Symbol::num() const {
    assert(type() == SymbolType::Num);
    return static_cast<int32_t>(static_cast<uint32_t>(rep_));
}

String Symbol::string() const {
    assert(type() == SymbolType::Str);
    return toString(rep_);
}

Sig Symbol::sig() const {
    assert(type() == SymbolType::Fun);
    switch (symbolType_(rep_)) {
        case SymbolType_::IdP: {
            return {toString(rep_), 0, false};
        }
        case SymbolType_::IdN: {
            return {toString(rep_), 0, true};
        }
        default: {
            return cast<Fun>(rep_)->sig();
        }
    }
}

bool Symbol::hasSig() const { return type() == SymbolType::Fun; }

String Symbol::name() const {
    assert(type() == SymbolType::Fun);
    switch (symbolType_(rep_)) {
        case SymbolType_::IdP:
        case SymbolType_::IdN: {
            return toString(rep_);
        }
        default: {
            return cast<Fun>(rep_)->sig().name();
        }
    }
}

SymSpan Symbol::args() const {
    assert(type() == SymbolType::Fun);
    switch (symbolType_(rep_)) {
        case SymbolType_::IdP:
        case SymbolType_::IdN: {
            return SymSpan{nullptr, 0};
        }
        default: {
            return cast<Fun>(rep_)->args();
        }
    }
}

bool Symbol::sign() const {
    assert(type() == SymbolType::Fun || type() == SymbolType::Num);
    switch (symbolType_(rep_)) {
        case SymbolType_::Num: {
            return num() < 0;
        }
        case SymbolType_::IdP: {
            return false;
        }
        case SymbolType_::IdN: {
            return true;
        }
        default: {
            return cast<Fun>(rep_)->sig().sign();
        }
    }
}

// {{{2 modification

Symbol Symbol::flipSign() const {
    assert(type() == SymbolType::Fun || type() == SymbolType::Num);
    switch (symbolType_(rep_)) {
        case SymbolType_::Num: {
            return Symbol::createNum(-num());
        }
        case SymbolType_::IdP: {
            return Symbol(setUpper(static_cast<uint16_t>(SymbolType_::IdN), rep_));
        }
        case SymbolType_::IdN: {
            return Symbol(setUpper(static_cast<uint16_t>(SymbolType_::IdP), rep_));
        }
        default: {
            auto const *f = cast<Fun>(rep_);
            auto s = f->sig();
            return createFun(s.name(), f->args(), !s.sign());
        }
    }
}

Symbol Symbol::replace(IdSymMap const &map) const {
    assert(symbolType_(rep_) != SymbolType_::IdN);
    switch (symbolType_(rep_)) {
        case SymbolType_::Fun: {
            SymVec vals;
            for (auto const &x : args()) {
                vals.emplace_back(x.replace(map));
            }
            return createFun(name(), Potassco::toSpan(vals));
        }
        case SymbolType_::IdP: {
            auto it = map.find(name());
            if (it != map.end()) {
                return it->second;
            }
        }
        default: {
            return *this;
        }
    }
}

// {{{2 comparison

size_t Symbol::hash() const { return get_value_hash(rep_); }

namespace {

bool less(Symbol const &a, Symbol const &b) {
    auto ta = symbolType_(a.rep());
    auto tb = symbolType_(b.rep());
    if (ta != tb) {
        return ta < tb;
    }
    switch (ta) {
        case SymbolType_::Num: {
            return a.num() < b.num();
        }
        case SymbolType_::IdN:
        case SymbolType_::IdP: {
            return a.name() < b.name();
        }
        case SymbolType_::Str: {
            return a.string() < b.string();
        }
        case SymbolType_::Inf:
        case SymbolType_::Sup: {
            return false;
        }
        case SymbolType_::Fun: {
            auto sa = a.sig();
            auto sb = b.sig();
            if (sa != sb) {
                return sa < sb;
            }
            auto aa = a.args();
            auto ab = b.args();
            return std::lexicographical_compare(begin(aa), end(aa), begin(ab), end(ab));
        }
        case SymbolType_::Special: {
            return false;
        }
    }
    assert(false);
    return false;
}

} // namespace

bool Symbol::operator==(Symbol const &other) const { return rep_ == other.rep_; }

bool Symbol::operator!=(Symbol const &other) const { return rep_ != other.rep_; }

bool Symbol::operator<(Symbol const &other) const { return (*this != other) && less(*this, other); }

bool Symbol::operator>(Symbol const &other) const { return (*this != other) && less(other, *this); }

bool Symbol::operator<=(Symbol const &other) const { return (*this == other) || less(*this, other); }

bool Symbol::operator>=(Symbol const &other) const { return (*this == other) || less(other, *this); }

// {{{2 output

void Symbol::print(std::ostream &out) const {
    switch (symbolType_(rep_)) {
        case SymbolType_::Num: {
            out << num();
            break;
        }
        case SymbolType_::IdN: {
            out << "-";
        }
        case SymbolType_::IdP: {
            char const *n = name().c_str();
            out << (n[0] != '\0' ? n : "()"); // NOLINT
            break;
        }
        case SymbolType_::Str: {
            out << '"' << quote(string().c_str()) << '"';
            break;
        }
        case SymbolType_::Inf: {
            out << "#inf";
            break;
        }
        case SymbolType_::Sup: {
            out << "#sup";
            break;
        }
        case SymbolType_::Fun: {
            auto s = sig();
            if (s.sign()) {
                out << "-";
            }
            out << s.name();
            auto a = args();
            out << "(";
            if (a.size > 0) {
                std::copy(begin(a), end(a) - 1, std::ostream_iterator<Symbol>(out, ",")); // NOLINT
                out << *(end(a) - 1);                                                     // NOLINT
            }
            if (a.size == 1 && s.name() == "") {
                out << ",";
            }
            out << ")";
            break;
        }
        case SymbolType_::Special: {
            out << "#special";
            break;
        }
    }
}

// }}}2

// }}}1

} // namespace Gringo
*/
