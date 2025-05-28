#pragma once

#include <clingo/ground/base.hh>
#include <clingo/ground/instantiator.hh>
#include <clingo/ground/term.hh>

#include <clingo/util/unordered_map.hh>

#include <clingo/core/core.hh>

#include <limits>
#include <memory_resource>

namespace CppClingo::Ground {

//! @addtogroup ground_matcher
//! @{

//! Concept for atom bases.
//!
//! An atom base must support the following:
//! - begin and end functions returning offsets for the given generation,
//! - a contains function to check if it contains a (ground) atom identified by its key,
//! - an n-th function that returns a pair where the first value is the key,
//! - and update function to update the current generation,
//! - a context function to add arbitrary contexts.
template <class Base>
concept IsBase = requires(Base &b) {
    b.begin(std::declval<MatcherType>());
    b.end(std::declval<MatcherType>());
    b.contains(std::declval<typename Base::Key>(), std::declval<MatcherType>());
    { b.nth(std::declval<size_t>()).key() } -> std::same_as<typename Base::Key const &>;
    b.update(size_t{0});
    { b.template context<int>() } -> std::same_as<int &>;
} && requires(Base const &b) {
    { b.nth(std::declval<size_t>()).key() } -> std::same_as<typename Base::Key const &>;
};

//! Concept for matchable expressions.
//!
//! A match must support the following:
//! - a vars function to get all variables in the expression,
//! - a match function to match a ground expression and an assignment,
//! - an eval function returning an optional ground expression given an assignment,
//! - a signature function to obtain a signature grouping compatible expressions,
//! - the match must be printable.
template <class Match>
concept IsMatch = requires(Match const &m) {
    { m.vars() } -> std::same_as<VariableSet>;
    m.match(std::declval<EvalContext const &>(), std::declval<typename Match::Key>());
    m.eval(std::declval<EvalContext const &>());
    m.signature(std::declval<VariableSet const &>(), std::declval<VariableSet const &>());
    std::declval<std::ostream &>() << m;
};

//! Concept for evaluable expressions.
template <class Eval>
concept IsEval = requires(Eval const &m) { m.eval(std::declval<EvalContext const &>()); };

static constexpr auto invalid_offset = std::numeric_limits<size_t>::max();

//! A matcher that matches only provides one match.
//!
//! By default, it provides exactly one match. Its do_once method can be
//! overridden to restrict matches further.
class OnceMatcher : public Matcher {
  public:
    //! Construct the matcher.
    OnceMatcher() = default;

  private:
    //! Return true if the matcher matches.
    //!
    //! Only called once.
    virtual auto do_once([[maybe_unused]] EvalContext const &ctx) -> bool { return true; }

    void do_init([[maybe_unused]] InstantiationContext const &ctx, [[maybe_unused]] size_t gen) override {}
    void do_match([[maybe_unused]] EvalContext const &ctx) override { match_ = true; }
    auto do_next(EvalContext const &ctx) -> bool override {
        if (match_) {
            match_ = false;
            return do_once(ctx);
        }
        return false;
    }
    void do_print(std::ostream &out) const override { out << "#once"; }

    bool match_ = false;
};

//! Construct a matcher matching only once.
auto make_once_matcher() -> UMatcher;

//! Construct an interval matcher.
//!
//! It matches [lhs] with all values from the interval [lower, upper]. All
//! variables in lower and upper must be bound.
auto make_interval_matcher(std::vector<bool> const &bound, Term const &lhs, Term const &lower, Term const &upper)
    -> UMatcher;

//! Construct a matcher for comparisons.
//!
//! It matches if `lhs rel rhs` holds. All variables in lhs and rhs must be
//! bound.
auto make_comp_matcher(std::vector<bool> const &bound, Term const &lhs, Relation rel, Term const &rhs) -> UMatcher;

//! Construct a once matcher that also evaluates an expression.
//!
//! The evaluated match is stored in the given reference.
template <IsEval Expr> auto make_once_matcher(Expr const &expr, typename Expr::Key &target) -> UMatcher;

//! Construct a matcher for facts.
//!
//! A matcher for anything that is not a fact. The evaluated match is stored in
//! the given reference.
template <IsBase Base, IsMatch Match>
auto make_non_fact_matcher(Base &base, Match const &match, typename Match::Key &target) -> UMatcher;

//! Construct a matcher for an atom.
//!
//! A base and an atom implementing the IsBase and IsMatch concepts must be given.
template <IsBase Base, IsMatch Match>
auto make_atom_matcher(std::pmr::monotonic_buffer_resource &mbr, std::vector<bool> const &bound, Base &base,
                       Match const &atom, MatcherType type, size_t &offset) -> UMatcher;

namespace Detail {

template <IsBase Base> class FullIndex {
  public:
    FullIndex(Base &base) : base_{&base} {}
    void init(size_t gen) { base_->update(gen); }
    auto match(MatcherType type) -> std::pair<size_t, size_t> {
        // select the index of the first atom of the matcher's generation
        auto cur = base_->begin(type);
        // select the first interval that contains an atom of the matcher's generation
        return {std::distance(index_.begin(),
                              std::ranges::upper_bound(index_, cur, std::less<>{},
                                                       [](auto const &a) -> decltype(auto) { return a.second; })),
                cur};
    }
    template <IsMatch Match>
    auto next(EvalContext const &ctx, Match const &m, VariableVec &free, MatcherType type, size_t &pos, size_t &cur,
              size_t &prev) -> bool {
        auto n = base_->end(type);
        // populate the index if it does not yet hold enough elements
        // (this also adds elements from previous generations that cannot match)
        for (; imported_ <= cur; ++imported_) {
            // the current index can no longer provide a match
            if (cur >= n) {
                return false;
            }
            for (auto const &var : free) {
                ctx.ass()[var] = std::nullopt;
            }
            // note that matches with `imported_ < cur` populate the index
            // (but do not match because they are from a previous generation)
            if (m.match(ctx, base_->nth(imported_).key())) {
                if (index_.empty() || index_.back().second != imported_) {
                    pos = index_.size();
                    index_.emplace_back(imported_, imported_ + 1);
                } else {
                    ++index_.back().second;
                }
                if (imported_ == cur) {
                    // the current index matches
                    prev = cur++;
                    ++imported_;
                    return true;
                }
            } else if (cur == imported_) {
                // the current index does not match
                ++cur;
            }
        }
        // obtain a (guaranteed) match from the index
        for (; pos < index_.size(); ++pos) {
            // all atoms in the interval have been matched
            cur = std::max(cur, index_[pos].first);
            // the current index can no longer provide a match
            if (cur >= n) {
                return false;
            }
            // match the next atom in the interval
            if (cur < index_[pos].second) {
                prev = cur++;
                for (auto const &var : free) {
                    ctx.ass()[var] = std::nullopt;
                }
                return m.match(ctx, base_->nth(prev).key());
            }
        }
        return false;
    }

  private:
    Base *base_;
    std::vector<std::pair<size_t, size_t>> index_;
    size_t imported_ = 0;
};

struct BindVals {
  public:
    BindVals() = default;
    void add(Assignment const &ass, VariableVec &bind_vars, size_t index) {
        symbols_.emplace_back(Symbol::from_rep(index));
        for (auto const &var : bind_vars) {
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            symbols_.emplace_back(*ass[var]);
        }
    }
    void match(size_t vars, size_t begin, SymbolVec::iterator &it) {
        it = symbols_.begin();
        if (begin > 0) {
            auto n = static_cast<std::ptrdiff_t>(vars + 1);
            std::advance(it, offset_);
            for (; it != symbols_.end() && Symbol::to_rep(*it) < begin; it += n, offset_ += n) {
            }
        }
    }
    auto next(Assignment &ass, VariableVec &bind_vars, size_t end, SymbolVec::iterator &it, size_t &offset) -> bool {
        if (it != symbols_.end() && Symbol::to_rep(*it) < end) {
            offset = Symbol::to_rep(*it++);
            for (auto const &var : bind_vars) {
                ass[var] = *it++;
            }
            return true;
        }
        return false;
    }

  private:
    size_t offset_ = 0;
    SymbolVec symbols_;
};

template <IsBase Base> class SingleIndex {
  private:
    using IndexMap = Util::unordered_map<Symbol, BindVals>;

  public:
    using KeyIterator = IndexMap::iterator;
    using ValIterator = SymbolVec::iterator;

    SingleIndex(Base &base) : base_{&base} {}
    void init(size_t gen) { base_->update(gen); }

    template <IsMatch Match>
    void match(EvalContext const &ctx, size_t bound_var, VariableVec &bind_vars, Match const &m, MatcherType type,
               KeyIterator &it, ValIterator &jt) {
        auto &ass = ctx.ass();
        // store the bound values
        auto bound_val = *ass[bound_var];
        // import
        if (auto n = base_->end(MatcherType::all_atoms); imported_ < n) {
            for (; imported_ < n; ++imported_) {
                // unbind all vars for matching
                ass[bound_var] = std::nullopt;
                for (auto const &var : bind_vars) {
                    ass[var] = std::nullopt;
                }
                // try to match
                if (m.match(ctx, base_->nth(imported_).key())) {
                    index_.try_emplace(*ass[bound_var]).first.value().add(ass, bind_vars, imported_);
                }
            }
            // restore the assignment
            ass[bound_var] = bound_val;
        }
        // find match
        if (it = index_.find(bound_val); it != index_.end()) {
            it.value().match(bind_vars.size(), base_->begin(type), jt);
        }
    }

    auto next(EvalContext const &ctx, VariableVec &bind_vars, MatcherType type, KeyIterator it, ValIterator &jt,
              size_t &offset) -> bool {
        return it != index_.end() && it.value().next(ctx.ass(), bind_vars, base_->end(type), jt, offset);
    }

  private:
    Base *base_;
    IndexMap index_;
    size_t imported_ = 0;
};

template <IsBase Base> class HashIndex {
  private:
    class Key {
      public:
        Key(Symbol const *syms, size_t hash)
            : hash_{hash},
              // NOLINTNEXTLINE
              syms_{reinterpret_cast<uintptr_t>(syms) | mask_} {}
        [[nodiscard]] auto hash() const -> size_t { return hash_; }
        [[nodiscard]] auto marked() const -> bool { return (syms_ & mask_) != 0; }
        void unmark() const { syms_ = syms_ & ~mask_; }
        [[nodiscard]] auto symbols() const -> Symbol const * {
            // NOLINTNEXTLINE
            return reinterpret_cast<Symbol const *>(syms_ & ~mask_);
        }

      private:
        static constexpr uintptr_t mask_ = 1;
        size_t hash_;
        mutable uintptr_t syms_;
    };

    struct KeyEqual {
        auto operator()(Key const &a, Key const &b) const {
            if (a.marked() || b.marked()) {
                return std::equal(a.symbols(), std::next(a.symbols(), size), b.symbols(), std::next(b.symbols(), size));
            }
            return a.symbols() == b.symbols();
        }
        size_t size;
    };

    using IndexMap = Util::unordered_map<Key, BindVals, Util::value_hasher, KeyEqual>;

  public:
    using KeyIterator = IndexMap::iterator;
    using ValIterator = SymbolVec::iterator;

    HashIndex(std::pmr::monotonic_buffer_resource &mbr, Base &base, size_t bound)
        : mbr_{&mbr}, base_{&base}, index_{0, Util::value_hasher{}, KeyEqual{bound}} {
        assert(bound > 0);
        temp_values_.reserve(bound);
    }
    void init(size_t gen) { base_->update(gen); }

    template <IsMatch Match>
    void match(EvalContext const &ctx, VariableVec &bound_vars, VariableVec &bind_vars, Match const &m,
               MatcherType type, KeyIterator &it, ValIterator &jt) {
        auto &ass = ctx.ass();
        // store the bound values
        temp_values_.clear();
        for (auto const &var : bound_vars) {
            temp_values_.emplace_back(*ass[var]);
        }
        // import
        if (auto n = base_->end(MatcherType::all_atoms); imported_ < n) {
            for (; imported_ < n; ++imported_) {
                // unbind all vars for matching
                for (auto const &var : bound_vars) {
                    ass[var] = std::nullopt;
                }
                for (auto const &var : bind_vars) {
                    ass[var] = std::nullopt;
                }
                // try to match
                if (m.match(ctx, base_->nth(imported_).key())) {
                    if (key_ == nullptr) {
                        key_ =
                            static_cast<Symbol *>(mbr_->allocate(bound_vars.size() * sizeof(Symbol), alignof(Symbol)));
                    }
                    auto *it = key_;
                    for (auto &var : bound_vars) {
                        std::construct_at(it, *ass[var]);
                        it = std::next(it);
                    }
                    auto key_hash = Util::value_hash(std::span{key_, it});
                    auto [kt, ins] = index_.try_emplace(Key{key_, key_hash});
                    if (ins) {
                        key_ = nullptr;
                        kt.key().unmark();
                    }
                    kt.value().add(ass, bind_vars, imported_);
                }
            }
            // restore the assignment
            auto kt = temp_values_.begin();
            for (auto const &var : bound_vars) {
                ass[var] = *kt++;
            }
        }
        // find match
        auto bound_hash = Util::value_hash(std::span(temp_values_.data(), bound_vars.size()));
        if (it = index_.find(Key{temp_values_.data(), bound_hash}); it != index_.end()) {
            it.value().match(bind_vars.size(), base_->begin(type), jt);
        }
    }

    auto next(EvalContext const &ctx, VariableVec &bind_vars, MatcherType type, KeyIterator it, ValIterator &jt,
              size_t &offset) -> bool {
        return it != index_.end() && it.value().next(ctx.ass(), bind_vars, base_->end(type), jt, offset);
    }

  private:
    std::pmr::monotonic_buffer_resource *mbr_;
    Base *base_;
    SymbolVec temp_values_;
    Symbol *key_ = nullptr;
    IndexMap index_;
    size_t imported_ = 0;
};

template <IsBase Base, IsMatch Match> class LookupMatcher : public OnceMatcher {
  public:
    LookupMatcher(Base &base, Match const &m, MatcherType type, size_t &offset)
        : base_{&base}, match_{&m}, type_{type}, offset_{&offset} {}

  private:
    void do_init([[maybe_unused]] InstantiationContext const &ctx, size_t gen) override { base_->update(gen); }
    auto do_once(EvalContext const &ctx) -> bool override {
        if (auto sym = match_->eval(ctx); sym) {
            if (auto idx = base_->contains(*sym, type_); idx) {
                *offset_ = *idx;
                return true;
            }
        }
        return false;
    }
    void do_print(std::ostream &out) const override { out << *match_; }
    [[nodiscard]] auto do_type() const -> MatcherType override { return type_; }

    Base *base_;
    Match const *match_;
    MatcherType type_;
    size_t *offset_;
};

template <IsBase Base, IsMatch Match> class FullMatcher : public Matcher {
  public:
    using Index = FullIndex<Base>;

    FullMatcher(Index &index, Match const &m, VariableVec free, MatcherType type, size_t &offset)
        : index_{&index}, offset_{&offset}, match_{&m}, free_{std::move(free)}, type_{type} {}

  private:
    void do_init([[maybe_unused]] InstantiationContext const &ctx, size_t gen) override { index_->init(gen); }
    void do_match([[maybe_unused]] EvalContext const &ctx) override { std::tie(pos_, cur_) = index_->match(type_); }
    auto do_next(EvalContext const &ctx) -> bool override {
        return index_->next(ctx, *match_, free_, type_, pos_, cur_, *offset_);
    }
    void do_print(std::ostream &out) const override { out << *match_; }
    [[nodiscard]] auto do_type() const -> MatcherType override { return type_; }

    Index *index_;
    size_t *offset_;
    Match const *match_;
    VariableVec free_;
    size_t pos_ = 0;
    size_t cur_ = 0;
    MatcherType type_;
};

template <IsBase Base, IsMatch Match> class SingleMatcher : public Matcher {
  public:
    using Index = SingleIndex<Base>;
    using KeyIterator = Index::KeyIterator;
    using ValIterator = Index::ValIterator;

    SingleMatcher(Index &index, Match const &m, size_t bound, VariableVec bind, MatcherType type, size_t &offset)
        : index_{&index}, match_{&m}, offset_{&offset}, bound_{bound}, bind_{std::move(bind)}, type_{type} {}

  private:
    void do_init([[maybe_unused]] InstantiationContext const &ctx, size_t gen) override { index_->init(gen); }
    void do_match([[maybe_unused]] EvalContext const &ctx) override {
        return index_->match(ctx, bound_, bind_, *match_, type_, pos_, cur_);
    }
    auto do_next(EvalContext const &ctx) -> bool override {
        return index_->next(ctx, bind_, type_, pos_, cur_, *offset_);
    }
    void do_print(std::ostream &out) const override { out << *match_; }
    [[nodiscard]] auto do_type() const -> MatcherType override { return type_; }

    Index *index_;
    Match const *match_;
    size_t *offset_;
    size_t bound_;
    VariableVec bind_;
    MatcherType type_;
    KeyIterator pos_;
    ValIterator cur_;
};

template <IsBase Base, IsMatch Match> class HashMatcher : public Matcher {
  public:
    using Index = HashIndex<Base>;
    using KeyIterator = Index::KeyIterator;
    using ValIterator = Index::ValIterator;

    HashMatcher(Index &index, Match const &m, VariableVec bound, VariableVec bind, MatcherType type, size_t &offset)
        : index_{&index}, match_{&m}, offset_{&offset}, bound_{std::move(bound)}, bind_{std::move(bind)}, type_{type} {}

  private:
    void do_init([[maybe_unused]] InstantiationContext const &ctx, size_t gen) override { index_->init(gen); }
    void do_match([[maybe_unused]] EvalContext const &ctx) override {
        return index_->match(ctx, bound_, bind_, *match_, type_, pos_, cur_);
    }
    auto do_next(EvalContext const &ctx) -> bool override {
        return index_->next(ctx, bind_, type_, pos_, cur_, *offset_);
    }
    void do_print(std::ostream &out) const override { out << *match_; }
    [[nodiscard]] auto do_type() const -> MatcherType override { return type_; }

    Index *index_;
    Match const *match_;
    size_t *offset_;
    VariableVec bound_;
    VariableVec bind_;
    MatcherType type_;
    KeyIterator pos_;
    ValIterator cur_;
};

template <IsBase Base, class Sig> class IndexSet : public BaseContext {
  public:
    auto add_full(Base &base, Sig sig) -> FullIndex<Base> & {
        auto it = full_.try_emplace(std::move(sig), nullptr).first;
        if (it->second == nullptr) {
            it.value() = std::make_unique<FullIndex<Base>>(base);
        }
        return *it->second;
    }

    auto add_single(Base &base, Sig sig) -> SingleIndex<Base> & {
        auto it = single_.try_emplace(std::move(sig), nullptr).first;
        if (it->second == nullptr) {
            it.value() = std::make_unique<SingleIndex<Base>>(base);
        }
        return *it->second;
    }

    auto add_hash(std::pmr::monotonic_buffer_resource &mbr, Base &base, Sig sig, size_t bound) -> HashIndex<Base> & {
        auto it = hash_.try_emplace(std::move(sig), nullptr).first;
        if (it->second == nullptr) {
            it.value() = std::make_unique<HashIndex<Base>>(mbr, base, bound);
        }
        return *it->second;
    }

  private:
    // Note: the full index does not need to capture the bound variables
    Util::unordered_map<Sig, std::unique_ptr<FullIndex<Base>>> full_;
    Util::unordered_map<Sig, std::unique_ptr<HashIndex<Base>>> hash_;
    Util::unordered_map<Sig, std::unique_ptr<SingleIndex<Base>>> single_;
};

template <IsBase Base, IsMatch Match> class NonFactMatcher : public OnceMatcher {
  public:
    NonFactMatcher(Base &base, Match const &match, typename Match::Key &target)
        : base_{&base}, match_{&match}, target_{&target} {}

  private:
    void do_init([[maybe_unused]] InstantiationContext const &ctx, size_t gen) override { base_->update(gen); }
    auto do_once(EvalContext const &ctx) -> bool override {
        if (auto sym = match_->eval(ctx)) {
            *target_ = *sym;
            return !base_->is_fact(*sym);
        }
        return false;
    }
    void do_print(std::ostream &out) const override { out << "#not_fact " << *match_; }

    Base *base_;
    Match const *match_;
    typename Match::Key *target_;
};

//! A matcher that provides at most one match.
template <IsMatch Match> class EvalMatcher : public OnceMatcher {
  public:
    //! Construct the matcher.
    EvalMatcher(Match const &match, typename Match::Key &target) : match_{&match}, target_{&target} {}

  private:
    //! Evaluate the matcher in the current context.
    auto do_once(EvalContext const &ctx) -> bool override {
        if (auto sym = match_->eval(ctx)) {
            *target_ = *sym;
            return true;
        }
        return false;
    }

    Match const *match_;
    typename Match::Key *target_;
};

} // namespace Detail

template <IsBase Base, IsMatch Match>
auto make_atom_matcher(std::pmr::monotonic_buffer_resource &mbr, std::vector<bool> const &bound, Base &base,
                       Match const &atom, MatcherType type, size_t &offset) -> UMatcher {
    VariableSet bind = atom.vars();
    VariableSet lookup;
    erase_if(bind, [&bound, &lookup](auto const &var) {
        if (bound[var]) {
            lookup.insert(var);
        }
        return bound[var];
    });
    // all variables are bound: evaluate + lookup in domain
    if (bind.empty()) {
        return std::make_unique<Detail::LookupMatcher<Base, Match>>(base, atom, type, offset);
    }
    auto &ctx = base.template context<Detail::IndexSet<Base, decltype(atom.signature(lookup, bind))>>();
    // no variables are bound: build an index optimized for sequences
    if (lookup.empty()) {
        auto &full = ctx.add_full(base, atom.signature(lookup, bind));
        return std::make_unique<Detail::FullMatcher<Base, Match>>(full, atom, bind.release(), type, offset);
    }
    // exactly one variable is bound and some are unbound: optimized special case
    if (lookup.size() == 1) {
        auto &hash = ctx.add_single(base, atom.signature(lookup, bind));
        return std::make_unique<Detail::SingleMatcher<Base, Match>>(hash, atom, lookup.front(), bind.release(), type,
                                                                    offset);
    }
    // at least two variables are bound and some are unbound: general case
    auto &hash = ctx.add_hash(mbr, base, atom.signature(lookup, bind), lookup.size());
    return std::make_unique<Detail::HashMatcher<Base, Match>>(hash, atom, lookup.release(), bind.release(), type,
                                                              offset);
}

template <IsBase Base, IsMatch Match>
auto make_non_fact_matcher(Base &base, Match const &match, typename Match::Key &target) -> UMatcher {
    return std::make_unique<Detail::NonFactMatcher<Base, Match>>(base, match, target);
}

template <IsEval Expr> auto make_once_matcher(Expr const &expr, typename Expr::Key &target) -> UMatcher {
    return std::make_unique<Detail::EvalMatcher<Expr>>(expr, target);
}

//! @}

} // namespace CppClingo::Ground
