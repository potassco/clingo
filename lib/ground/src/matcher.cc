#include <gringo/ground/matcher.hh>

#include <gringo/util/span_stack.hh>

namespace Gringo::Ground {

namespace {
class OnceMatcher : public Matcher {
  public:
    OnceMatcher() = default;
    virtual auto do_match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) -> bool {
        return true;
    }
    void init([[maybe_unused]] SymbolStore &store, [[maybe_unused]] size_t gen) override {}
    void match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) override { match_ = true; }
    auto next(SymbolStore &store, Assignment &ass) -> bool override {
        if (match_) {
            match_ = false;
            return do_match(store, ass);
        }
        return false;
    }
    void print(std::ostream &out) const override { out << "#once"; }

  private:
    bool match_ = false;
};

class CmpMatcher : public OnceMatcher {
  public:
    CmpMatcher(Term const &lhs, Relation cmp, Term const &rhs) : lhs_{&lhs}, rhs_{&rhs}, cmp_{cmp} {}
    auto do_match(SymbolStore &store, Assignment &ass) -> bool override {
        // std::cerr << "doing a cmp match: " << *lhs_ << " " << cmp_ << " " << *rhs_ << "\n";
        auto lhs = lhs_->eval(store, ass);
        if (!lhs) {
            return false;
        }
        auto rhs = rhs_->eval(store, ass);
        if (!rhs) {
            return false;
        }
        switch (cmp_) {
            case Relation::equal: {
                return *lhs == *rhs;
            }
            case Relation::greater: {
                return *lhs > *rhs;
            }
            case Relation::greater_equal: {
                return *lhs >= *rhs;
            }
            case Relation::less: {
                return *lhs < *rhs;
            }
            case Relation::less_equal: {
                return *lhs <= *rhs;
            }
            case Relation::not_equal: {
                return *lhs != *rhs;
            }
        }
        return false;
    }
    void print(std::ostream &out) const override { out << *lhs_ << cmp_ << *rhs_; }

  private:
    Term const *lhs_;
    Term const *rhs_;
    Relation cmp_;
};

class AssignMatcher : public OnceMatcher {
  public:
    AssignMatcher(Term const &lhs, Term const &rhs, VariableVec free)
        : lhs_{&lhs}, rhs_{&rhs}, free_{std::move(free)} {}
    auto do_match(SymbolStore &store, Assignment &ass) -> bool override {
        // unbind variables
        for (auto const &var : free_) {
            ass[var] = std::nullopt;
        }
        auto rhs = rhs_->eval(store, ass);
        // if (rhs) {
        //     std::cerr << "matching: " << *lhs_ << " and " << *rhs << "\n";
        // }
        return rhs && lhs_->match(store, *rhs, ass);
    }
    void print(std::ostream &out) const override { out << *lhs_ << ":=" << *rhs_; }

  private:
    Term const *lhs_;
    Term const *rhs_;
    VariableVec free_;
};

class NonFactMatcher : public OnceMatcher {
  public:
    NonFactMatcher(Base &base, Term const &term) : base_{&base}, term_{&term} {}
    void init([[maybe_unused]] SymbolStore &store, size_t gen) override { base_->update(gen); }
    auto do_match(SymbolStore &store, Assignment &ass) -> bool override {
        auto sym = term_->eval(store, ass);
        return !sym || !base_->is_fact(*sym);
    }
    void print(std::ostream &out) const override { out << "#not fact " << *term_; }

  private:
    Base *base_;
    Term const *term_;
};

class FullIndex {
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
    auto next(SymbolStore &store, Assignment &ass, Term const &term, VariableVec &free, MatcherType type, size_t &pos,
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
            if (term.match(store, base_->nth(imported_)->first, ass)) {
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
                return term.match(store, base_->nth(cur++)->first, ass);
            }
        }
        return false;
    }

  private:
    Base *base_;
    std::vector<std::pair<size_t, size_t>> index_;
    size_t imported_ = 0;
};

class HashIndex {
  public:
    HashIndex(Base &base, size_t bound, size_t bind)
        : base_{&base}, bound_values_{bound}, bind_values_{bind},
          index_{0, Util::SpanHash{bound}, Util::SpanEqualTo{bound}} {
        assert(bound > 0 && bind > 0);
        temp_values_.reserve(bound);
    }
    void init(size_t gen) { base_->update(gen); }

    static auto match() -> std::pair<size_t, size_t> {
        return {std::numeric_limits<size_t>::max(), std::numeric_limits<size_t>::max()};
    }

    auto next(SymbolStore &store, Assignment &ass, VariableVec &bound_vars, VariableVec &bind_vars, Term const &term,
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
            return import_next(store, ass, bound_vars, bind_vars, term, type, std::span(temp_values_), pos, cur);
        }
        auto it = index_.nth(pos);
        if (cur < it->second.size()) {
            return bind_next(ass, bind_vars, type, it, cur);
        }
        return import_next(store, ass, bound_vars, bind_vars, term, type, std::span(it->first, bound_vars.size()), pos,
                           cur);
    }

  private:
    using BindVec = std::vector<std::pair<size_t, Symbol *>>;
    // Note: we need an ordered map to be able to update indices while
    // matching. The same index might be updated from different matchers.
    using IndexMap = Util::ordered_map<Symbol *, BindVec, Util::SpanHash, Util::SpanEqualTo>;

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
    auto import_next(SymbolStore &store, Assignment &ass, VariableVec &bound_vars, VariableVec &bind_vars,
                     Term const &term, MatcherType type, std::span<Symbol> bound_vals, size_t &pos,
                     size_t &cur) -> bool {
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
            if (term.match(store, base_->nth(imported_)->first, ass)) {
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

using IndexSig = std::pair<std::vector<size_t>, UTerm>;

class IndexSet : public BaseContext {
  public:
    auto add_full(Base &base, std::vector<size_t> bound, UTerm term) -> FullIndex & {
        auto it = full_.try_emplace(IndexSig{std::move(bound), std::move(term)}, nullptr).first;
        if (it->second == nullptr) {
            it.value() = std::make_unique<FullIndex>(base);
        }
        return *it->second;
    }
    auto add_hash(Base &base, std::vector<size_t> bound, size_t bind, UTerm term) -> HashIndex & {
        auto it = hash_.try_emplace(IndexSig{std::move(bound), std::move(term)}, nullptr).first;
        if (it->second == nullptr) {
            it.value() = std::make_unique<HashIndex>(base, it->first.first.size(), bind);
        }
        return *it->second;
    }

  private:
    // Note: the full index does not need to capture the bound variables
    Util::unordered_map<IndexSig, std::unique_ptr<FullIndex>, Util::value_hasher, Util::value_equal_to> full_;
    Util::unordered_map<IndexSig, std::unique_ptr<HashIndex>, Util::value_hasher, Util::value_equal_to> hash_;
};

class LookupMatcher : public OnceMatcher {
  public:
    LookupMatcher(Base &base, Term const &term, MatcherType type) : base_{&base}, term_{&term}, type_{type} {}
    void init([[maybe_unused]] SymbolStore &store, size_t gen) override { base_->update(gen); }
    auto do_match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) -> bool override {
        auto sym = term_->eval(store, ass);
        return sym && base_->contains(*sym, type_);
    }
    void print(std::ostream &out) const override { out << *term_; }

  private:
    Base *base_;
    Term const *term_;
    MatcherType type_;
};

class FullMatcher : public Matcher {
  public:
    FullMatcher(FullIndex &index, Term const &term, VariableVec free, MatcherType type)
        : index_{&index}, term_{&term}, free_{std::move(free)}, type_{type} {}
    void init([[maybe_unused]] SymbolStore &store, size_t gen) override { index_->init(gen); }
    void match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) override {
        std::tie(pos_, cur_) = index_->match(type_);
    }
    auto next(SymbolStore &store, Assignment &ass) -> bool override {
        return index_->next(store, ass, *term_, free_, type_, pos_, cur_);
    }
    void print(std::ostream &out) const override { out << *term_; }

  private:
    FullIndex *index_;
    Term const *term_;
    VariableVec free_;
    MatcherType type_;
    size_t pos_ = 0;
    size_t cur_ = 0;
};

class HashMatcher : public Matcher {
  public:
    HashMatcher(HashIndex &index, Term const &term, VariableVec bound, VariableVec bind, MatcherType type)
        : index_{&index}, term_{&term}, bound_{std::move(bound)}, bind_{std::move(bind)}, type_{type} {}
    void init([[maybe_unused]] SymbolStore &store, size_t gen) override { index_->init(gen); }
    void match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) override {
        std::tie(pos_, cur_) = HashIndex::match();
    }
    auto next(SymbolStore &store, Assignment &ass) -> bool override {
        return index_->next(store, ass, bound_, bind_, *term_, type_, pos_, cur_);
    }
    void print(std::ostream &out) const override { out << *term_; }

  private:
    HashIndex *index_;
    Term const *term_;
    VariableVec bound_;
    VariableVec bind_;
    MatcherType type_;
    size_t pos_ = 0;
    size_t cur_ = 0;
};

class IntervalMatcher : public Matcher {
  public:
    IntervalMatcher(Term const &lhs, Term const &lower, Term const &upper, VariableVec free)
        : lhs_{&lhs}, lower_{&lower}, upper_{&upper}, free_{std::move(free)} {}
    void init([[maybe_unused]] SymbolStore &store, [[maybe_unused]] size_t gen) override {}
    void match(SymbolStore &store, Assignment &ass) override {
        val_current_ = 1;
        val_upper_ = 0;
        if (auto lower = lower_->eval(store, ass), upper = upper_->eval(store, ass);
            lower && upper && lower->type() == SymbolType::number && upper->type() == SymbolType::number) {
            if (!free_.empty()) {
                val_current_ = lower->num();
                val_upper_ = upper->num();
            }
            // Note: that the case free is empty could be handled a little more
            // efficiently. I would not expect a big impact, though.
            else if (auto lhs = lhs_->eval(store, ass); lhs && lhs->type() == SymbolType::number &&
                                                        *lower->num() <= *lhs->num() && *lhs->num() <= *upper->num()) {
                val_current_ = lhs->num();
                val_upper_ = lhs->num();
            }
        }
    }
    auto next(SymbolStore &store, Assignment &ass) -> bool override {
        while (val_current_ <= val_upper_) {
            for (auto const &var : free_) {
                ass[var] = std::nullopt;
            }
            auto num = val_current_;
            val_current_ += 1;
            if (lhs_->match(store, store.num(std::move(num)), ass)) {
                return true;
            }
        }
        return false;
    }
    void print(std::ostream &out) const override { out << *lhs_ << ":=" << *lower_ << ".." << *upper_; }

  private:
    Term const *lhs_;
    Term const *lower_;
    Term const *upper_;
    VariableVec free_;
    Number val_current_ = 0;
    Number val_upper_ = 0;
};

} // namespace

auto make_once_matcher() -> UMatcher { return std::make_unique<OnceMatcher>(); }

auto make_interval_matcher(std::vector<bool> const &bound, Term const &lhs, Term const &lower,
                           Term const &upper) -> UMatcher {
    VariableSet vars;
    lhs.vars(vars);
    erase_if(vars, [&bound](auto const &var) { return bound[var]; });
    return std::make_unique<IntervalMatcher>(lhs, lower, upper, vars.release());
}

auto make_comp_matcher(std::vector<bool> const &bound, Term const &lhs, Relation rel, Term const &rhs) -> UMatcher {
    if (rel == Relation::equal) {
        VariableSet vars;
        lhs.vars(vars, true);
        erase_if(vars, [&bound](auto const &var) { return bound[var]; });
        if (!vars.empty()) {
            return std::make_unique<AssignMatcher>(lhs, rhs, vars.release());
        }
    }
    return std::make_unique<CmpMatcher>(lhs, rel, rhs);
}

auto make_non_fact_matcher(Base &base, Term const &term) -> UMatcher {
    return std::make_unique<NonFactMatcher>(base, term);
}

auto make_atom_matcher(std::vector<bool> const &bound, Base &base, Term const &atom, MatcherType type) -> UMatcher {
    VariableSet bind;
    VariableSet lookup;
    atom.vars(bind);
    erase_if(bind, [&bound, &lookup](auto const &var) {
        if (bound[var]) {
            lookup.insert(var);
        }
        return bound[var];
    });
    if (bind.empty()) {
        return std::make_unique<LookupMatcher>(base, atom, type);
    }
    auto names = Util::unordered_map<size_t, size_t>{};
    names.reserve(bind.size() + lookup.size());
    auto sig_term = atom.rename(names);
    auto sig_lookup = std::vector<size_t>{};
    sig_lookup.reserve(lookup.size());
    for (auto const &var : lookup) {
        sig_lookup.emplace_back(names[var]);
    }

    if (lookup.empty()) {
        auto &full = base.context<IndexSet>().add_full(base, std::move(sig_lookup), std::move(sig_term));
        return std::make_unique<FullMatcher>(full, atom, bind.release(), type);
    }
    auto &hash = base.context<IndexSet>().add_hash(base, std::move(sig_lookup), bind.size(), std::move(sig_term));
    return std::make_unique<HashMatcher>(hash, atom, lookup.release(), bind.release(), type);
}

} // namespace Gringo::Ground
