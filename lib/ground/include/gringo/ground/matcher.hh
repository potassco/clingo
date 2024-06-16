#pragma once

#include <gringo/ground/base.hh>
#include <gringo/ground/instantiator.hh>
#include <gringo/ground/term.hh>

#include <gringo/util/ordered_set.hh>
#include <gringo/util/span_stack.hh>
#include <gringo/util/unordered_map.hh>

#include <gringo/core/core.hh>

namespace Gringo::Ground {

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
    { b.nth(std::declval<size_t>())->first } -> std::same_as<typename Base::Key const &>;
    b.update(size_t{0});
    { b.template context<int>() } -> std::same_as<int &>;
} && requires(Base const &b) {
    { b.nth(std::declval<size_t>())->first } -> std::same_as<typename Base::Key const &>;
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
    m.match(std::declval<SymbolStore &>(), std::declval<typename Match::Key>(), std::declval<Assignment &>());
    m.eval(std::declval<SymbolStore &>(), std::declval<Assignment &>());
    m.signature(std::declval<VariableSet const &>(), std::declval<VariableSet const &>());
    std::declval<std::ostream &>() << m;
};

//! A matcher that matches only provides one match.
//!
//! By default it provides exactly one match. Its do_once method can be
//! overriden to restrict matches further.
class OnceMatcher : public Matcher {
  public:
    //! Construct the matcher.
    OnceMatcher() = default;

  private:
    //! Return true if the matcher matches.
    //!
    //! Only called once.
    virtual auto do_once([[maybe_unused]] InstantiationContext &ctx) -> bool { return true; }

    void do_init([[maybe_unused]] SymbolStore &store, [[maybe_unused]] size_t gen) override {}
    void do_match([[maybe_unused]] InstantiationContext &ctx) override { match_ = true; }
    auto do_next(InstantiationContext &ctx) -> bool override {
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
auto make_interval_matcher(std::vector<bool> const &bound, Term const &lhs, Term const &lower,
                           Term const &upper) -> UMatcher;

//! Construct a matcher for comparisons.
//!
//! It matches if `lhs rel rhs` holds. All variables in lhs and rhs must be
//! bound.
auto make_comp_matcher(std::vector<bool> const &bound, Term const &lhs, Relation rel, Term const &rhs) -> UMatcher;

//! Construct a matcher for facts.
//!
//! Matches if the term represents a fact. If target is given, the evaluated
//! term is stored in it.
//!
//! @note: candidate for generalization
auto make_non_fact_matcher(Base &base, Term const &term, Symbol *target) -> UMatcher;

//! Construct a matcher for an atom.
//!
//! A base and an atom implementing the IsBase and IsMatch concepts must be given.
template <IsBase Base, IsMatch Match>
auto make_atom_matcher(std::vector<bool> const &bound, Base &base, Match const &atom, MatcherType type) -> UMatcher;

namespace Detail {

template <IsBase Base> class FullIndex {
  public:
    FullIndex(Base &base) : base_{&base} {}
    void init(size_t gen) { base_->update(gen); }
    auto match(MatcherType type) -> std::pair<size_t, size_t> {
        // select the index of the first atom of the matcher's generation
        auto cur = base_->begin(type);
        // select the first interval that contains an atom of the matcher's generation
        return {
            std::distance(index_.begin(), std::upper_bound(index_.begin(), index_.end(), cur,
                                                           [](auto const &a, auto const &b) { return a < b.second; })),
            cur};
    }
    template <IsMatch Match>
    auto next(SymbolStore &store, Assignment &ass, Match const &m, VariableVec &free, MatcherType type, size_t &pos,
              size_t &cur) -> bool {
        auto n = base_->end(type);
        // populate the index if it does not yet hold enough elements
        for (; imported_ <= cur; ++imported_) {
            // the current index can no longer provide a match
            if (cur >= n) {
                return false;
            }
            for (auto const &var : free) {
                ass[var] = std::nullopt;
            }
            if (m.match(store, base_->nth(imported_)->first, ass)) {
                if (index_.empty() || index_.back().second != imported_) {
                    pos = index_.size();
                    index_.emplace_back(imported_, imported_ + 1);
                } else {
                    ++index_.back().second;
                }
                if (imported_ == cur) {
                    // the current index matches
                    ++cur;
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
            if (cur < index_[pos].first) {
                cur = index_[pos].first;
            }
            // the current index can no longer provide a match
            if (cur >= n) {
                return false;
            }
            // match the next atom in the interval
            if (cur < index_[pos].second) {
                for (auto const &var : free) {
                    ass[var] = std::nullopt;
                }
                return m.match(store, base_->nth(cur++)->first, ass);
            }
        }
        return false;
    }

  private:
    Base *base_;
    std::vector<std::pair<size_t, size_t>> index_;
    size_t imported_ = 0;
};

template <IsBase Base> class HashIndex {
  public:
    struct Hash {
        Hash(size_t size) : size{size} {}
        template <class T> auto operator()(T const *sym) const -> size_t {
            return Util::value_hash_range(std::span{sym, size});
        }
        size_t size;
    };
    HashIndex(Base &base, size_t bound, size_t bind)
        : base_{&base}, bound_values_{bound}, bind_values_{bind}, index_{0, Hash{bound}, Util::SpanEqualTo{bound}} {
        assert(bound > 0 && bind > 0);
        temp_values_.reserve(bound);
    }
    void init(size_t gen) { base_->update(gen); }

    static auto match() -> std::pair<size_t, size_t> {
        return {std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max()};
    }

    template <IsMatch Match>
    auto next(SymbolStore &store, Assignment &ass, VariableVec &bound_vars, VariableVec &bind_vars, Match const &m,
              MatcherType type, size_t &pos, size_t &cur) -> bool {
        if (pos == std::numeric_limits<size_t>::max()) {
            temp_values_.clear();
            for (auto const &var : bound_vars) {
                assert(ass[var]);
                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                temp_values_.emplace_back(*ass[var]);
            }
            if (auto it = index_.find(temp_values_.data()); it != index_.end()) {
                pos = std::distance(index_.begin(), it);
                cur = static_cast<size_t>(std::distance(
                    it->second.begin(), std::lower_bound(it->second.begin(), it->second.end(), base_->begin(type),
                                                         [](auto const &a, auto const &b) { return a.first < b; })));
                if (cur < it->second.size()) {
                    return bind_next(ass, bind_vars, type, it, cur);
                }
            }
            return import_next(store, ass, bound_vars, bind_vars, m, type, std::span(temp_values_), pos, cur);
        }
        auto it = index_.nth(pos);
        if (cur < it->second.size()) {
            return bind_next(ass, bind_vars, type, it, cur);
        }
        return import_next(store, ass, bound_vars, bind_vars, m, type, std::span(it->first, bound_vars.size()), pos,
                           cur);
    }

  private:
    // TODO: adjust
    // - should be changed to: pair<new_offset, vector<tuple<hash, index, symbols[0]>>>
    // - the new_offset could be used to quickly jump to the new generation
    //   (even though going backwards or a binary search would also be an option)
    // - the hash should be stored for faster lookup
    // - the index to check the current generation
    // - the index could also be reported to the literal to avoid later evals/lookups
    using BindVec = std::vector<std::pair<size_t, Symbol *>>;
    // TODO:
    // - get rid of the ordered map by importing all symbols
    using IndexMap = Util::ordered_map<Symbol *, BindVec, Hash, Util::SpanEqualTo>;

    auto bind_next(Assignment &ass, VariableVec &bind_vars, MatcherType type, IndexMap::iterator &it,
                   size_t &cur) -> bool {
        if (auto [i, bind_vals] = it->second[cur]; i < base_->end(type)) {
            for (auto const &var : bind_vars) {
                ass[var] = *bind_vals;
                ++bind_vals;
            }
            ++cur;
            return true;
        }
        return false;
    }
    template <IsMatch Match>
    auto import_next(SymbolStore &store, Assignment &ass, VariableVec &bound_vars, VariableVec &bind_vars,
                     Match const &m, MatcherType type, std::span<Symbol> bound_vals, size_t &pos, size_t &cur) -> bool {
        auto n = base_->end(type);
        // there can be no more matches
        if (imported_ >= n) {
            return false;
        }
        for (auto i = base_->begin(type); imported_ < n; ++imported_) {
            // unbind all vars for matching
            for (auto const &var : bound_vars) {
                ass[var] = std::nullopt;
            }
            for (auto const &var : bind_vars) {
                ass[var] = std::nullopt;
            }
            // try to match
            if (m.match(store, base_->nth(imported_)->first, ass)) {
                auto bound_match = bound_values_.push_map(bound_vars, [&ass](auto const &var) { return *ass[var]; });
                auto [jt, ins] = index_.try_emplace(bound_match.data());
                if (!ins) {
                    bound_match = {jt->first, bound_vars.size()};
                    bound_values_.pop();
                }
                // TODO: cache-wise this is not the best layout
                // it would be better to store the matches in contiguous memory
                // something like this: [size,var,...,var,size,var,...,var,...]
                auto bind_match = bind_values_.push_map(bind_vars, [&ass](auto const &var) { return *ass[var]; });
                jt.value().emplace_back(imported_, bind_match.data());
                // check if the imported symbol is a match
                // note that the current assignment captures the match
                if (i <= imported_ && Util::value_equal_to{}(bound_match, bound_vals)) {
                    ++imported_;
                    pos = std::distance(index_.begin(), jt);
                    cur = jt->second.size();
                    return true;
                }
            }
        }
        // restore the assignment if there was no match
        auto jt = bound_vals.begin();
        for (auto const &var : bound_vars) {
            ass[var] = *jt++;
        }
        return false;
    }

    Base *base_;
    std::vector<Symbol> temp_values_;
    Util::SpanStack<Symbol> bound_values_;
    Util::SpanStack<Symbol> bind_values_;
    IndexMap index_;
    size_t imported_ = 0;
};

template <IsBase Base, IsMatch Match> class LookupMatcher : public OnceMatcher {
  public:
    LookupMatcher(Base &base, Match const &m, MatcherType type) : base_{&base}, match_{&m}, type_{type} {}

  private:
    void do_init([[maybe_unused]] SymbolStore &store, size_t gen) override { base_->update(gen); }
    auto do_once(InstantiationContext &ctx) -> bool override {
        auto sym = match_->eval(ctx.store(), ctx.ass());
        return sym && base_->contains(*sym, type_);
    }
    void do_print(std::ostream &out) const override { out << *match_; }
    [[nodiscard]] auto do_type() const -> MatcherType override { return type_; }

    Base *base_;
    Match const *match_;
    MatcherType type_;
};

template <IsBase Base, IsMatch Match> class FullMatcher : public Matcher {
  public:
    using Index = FullIndex<Base>;

    FullMatcher(Index &index, Match const &m, VariableVec free, MatcherType type)
        : index_{&index}, match_{&m}, free_{std::move(free)}, type_{type} {}

  private:
    void do_init([[maybe_unused]] SymbolStore &store, size_t gen) override { index_->init(gen); }
    void do_match([[maybe_unused]] InstantiationContext &ctx) override { std::tie(pos_, cur_) = index_->match(type_); }
    auto do_next(InstantiationContext &ctx) -> bool override {
        return index_->next(ctx.store(), ctx.ass(), *match_, free_, type_, pos_, cur_);
    }
    void do_print(std::ostream &out) const override { out << *match_; }
    [[nodiscard]] auto do_type() const -> MatcherType override { return type_; }

    Index *index_;
    Match const *match_;
    VariableVec free_;
    MatcherType type_;
    size_t pos_ = 0;
    size_t cur_ = 0;
};

template <IsBase Base, IsMatch Match> class HashMatcher : public Matcher {
  public:
    using Index = HashIndex<Base>;

    HashMatcher(Index &index, Match const &m, VariableVec bound, VariableVec bind, MatcherType type)
        : index_{&index}, match_{&m}, bound_{std::move(bound)}, bind_{std::move(bind)}, type_{type} {}

  private:
    void do_init([[maybe_unused]] SymbolStore &store, size_t gen) override { index_->init(gen); }
    void do_match([[maybe_unused]] InstantiationContext &ctx) override { std::tie(pos_, cur_) = Index::match(); }
    auto do_next(InstantiationContext &ctx) -> bool override {
        return index_->next(ctx.store(), ctx.ass(), bound_, bind_, *match_, type_, pos_, cur_);
    }
    void do_print(std::ostream &out) const override { out << *match_; }
    [[nodiscard]] auto do_type() const -> MatcherType override { return type_; }

    Index *index_;
    Match const *match_;
    VariableVec bound_;
    VariableVec bind_;
    MatcherType type_;
    size_t pos_ = 0;
    size_t cur_ = 0;
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

    auto add_hash(Base &base, Sig sig, size_t bound, size_t bind) -> HashIndex<Base> & {
        auto it = hash_.try_emplace(std::move(sig), nullptr).first;
        if (it->second == nullptr) {
            it.value() = std::make_unique<HashIndex<Base>>(base, bound, bind);
        }
        return *it->second;
    }

  private:
    // Note: the full index does not need to capture the bound variables
    Util::unordered_map<Sig, std::unique_ptr<FullIndex<Base>>> full_;
    Util::unordered_map<Sig, std::unique_ptr<HashIndex<Base>>> hash_;
};

} // namespace Detail

template <IsBase Base, IsMatch Match>
auto make_atom_matcher(std::vector<bool> const &bound, Base &base, Match const &atom, MatcherType type) -> UMatcher {
    VariableSet bind = atom.vars();
    VariableSet lookup;
    erase_if(bind, [&bound, &lookup](auto const &var) {
        if (bound[var]) {
            lookup.insert(var);
        }
        return bound[var];
    });
    if (bind.empty()) {
        return std::make_unique<Detail::LookupMatcher<Base, Match>>(base, atom, type);
    }
    auto &ctx = base.template context<Detail::IndexSet<Base, decltype(atom.signature(lookup, bind))>>();
    if (lookup.empty()) {
        auto &full = ctx.add_full(base, atom.signature(lookup, bind));
        return std::make_unique<Detail::FullMatcher<Base, Match>>(full, atom, bind.release(), type);
    }
    // Note: this could be optimized for small lookup/bind sizes
    // especially the small bind sizes seem interesting
    auto &hash = ctx.add_hash(base, atom.signature(lookup, bind), lookup.size(), bind.size());
    return std::make_unique<Detail::HashMatcher<Base, Match>>(hash, atom, lookup.release(), bind.release(), type);
}

//! @}

} // namespace Gringo::Ground
