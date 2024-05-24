#include "gringo/util/print.hh"
#include <gringo/ground/aggregate.hh>

#include <iostream>
#include <typeindex>

namespace Gringo::Ground {

namespace {

template <IsBase Base> class FullIndex {
  public:
    FullIndex(Base const &base) : base_{&base} {}
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
    Base const *base_;
    std::vector<std::pair<size_t, size_t>> index_;
    size_t imported_ = 0;
};

template <IsBase Base> class HashIndex {
  public:
    HashIndex(Base const &base, size_t bound, size_t bind)
        : base_{&base}, bound_values_{bound}, bind_values_{bind},
          index_{0, Util::SpanHash{bound}, Util::SpanEqualTo{bound}} {
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

    Base const *base_;
    std::vector<Symbol> temp_values_;
    Util::SpanStack<Symbol> bound_values_;
    Util::SpanStack<Symbol> bind_values_;
    IndexMap index_;
    size_t imported_ = 0;
};

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

template <IsBase Base, IsMatch Match> class LookupMatcher : public OnceMatcher {
  public:
    LookupMatcher(Base const &base, Match const &m, MatcherType type) : base_{&base}, match_{&m}, type_{type} {}
    void init([[maybe_unused]] SymbolStore &store, size_t gen) override { base_->update(gen); }
    auto do_match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) -> bool override {
        auto sym = match_->eval(store, ass);
        return sym && base_->contains(*sym, type_);
    }
    void print(std::ostream &out) const override { out << *match_; }

  private:
    Base const *base_;
    Match const *match_;
    MatcherType type_;
};

template <IsBase Base, IsMatch Match> class FullMatcher : public Matcher {
  public:
    using Index = FullIndex<Base>;

    FullMatcher(Index &index, Match const &m, VariableVec free, MatcherType type)
        : index_{&index}, match_{&m}, free_{std::move(free)}, type_{type} {}
    void init([[maybe_unused]] SymbolStore &store, size_t gen) override { index_->init(gen); }
    void match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) override {
        std::tie(pos_, cur_) = index_->match(type_);
    }
    auto next(SymbolStore &store, Assignment &ass) -> bool override {
        return index_->next(store, ass, *match_, free_, type_, pos_, cur_);
    }
    void print(std::ostream &out) const override { out << *match_; }

  private:
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
    void init([[maybe_unused]] SymbolStore &store, size_t gen) override { index_->init(gen); }
    void match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) override {
        std::tie(pos_, cur_) = Index::match();
    }
    auto next(SymbolStore &store, Assignment &ass) -> bool override {
        return index_->next(store, ass, bound_, bind_, *match_, type_, pos_, cur_);
    }
    void print(std::ostream &out) const override { out << *match_; }

  private:
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

// TODO: should become public

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
        return std::make_unique<LookupMatcher<Base, Match>>(base, atom, type);
    }
    auto &ctx = base.template context<IndexSet<Base, decltype(atom.signature(lookup, bind))>>();
    if (lookup.empty()) {
        auto &full = ctx.add_full(base, atom.signature(lookup, bind));
        return std::make_unique<FullMatcher<Base, Match>>(full, atom, bind.release(), type);
    }
    auto &hash = ctx.add_hash(base, atom.signature(lookup, bind), lookup.size(), bind.size());
    return std::make_unique<HashMatcher<Base, Match>>(hash, atom, lookup.release(), bind.release(), type);
}

} // namespace

// LitCondLit

auto operator<<(std::ostream &out, LitCondLitType type) -> std::ostream & {
    switch (type) {
        case LitCondLitType::empty: {
            out << "empty";
            break;
        }
        case LitCondLitType::premise: {
            out << "premise";
            break;
        }
        case LitCondLitType::lit: {
            out << "condlit";
            break;
        }
    }
    return out;
}

void LitCondLit::vars(VariableSet &vars, VarSelectMode mode) const {
    if (mode != VarSelectMode::depend) {
        base_->vars(vars, type_ == LitCondLitType::premise);
    }
}

auto LitCondLit::domain() const -> bool {
    // We can return true here because a cond lit domain is empty upon an incremental step.
    return true;
}

auto LitCondLit::recursive() const -> bool { return index_ != stratified_index; }

auto LitCondLit::matcher(MatcherType type,
                         std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    auto index = std::optional<size_t>{};
    if (index_ != std::numeric_limits<size_t>::max() && type == MatcherType::new_atoms) {
        index = index_;
    }
    if (type_ == LitCondLitType::empty) {
        return {make_atom_matcher(bound, base_->base_empty(), base_->match_empty(), type), index};
    }
    if (type_ == LitCondLitType::premise) {
        return {make_atom_matcher(bound, base_->base_premise(), base_->match_premise(), type), index};
    }
    return {make_atom_matcher(bound, base_->base_lit(), base_->match_lit(), type), index};
}

auto LitCondLit::score(std::vector<bool> const &bound) const -> double {
    static_cast<void>(bound);
    std::cerr << "TODO: cond lit " << type_ << " compute proper score or return something very small\n";
    return 0;
}

void LitCondLit::print(std::ostream &out) const {
    out << "#cond_lit(" << type_;
    for (auto var : base_->vars_global()) {
        out << ","
            << "X_" << var;
    }
    if (type_ != LitCondLitType::empty && type_ != LitCondLitType::lit) {
        for (auto var : base_->vars_local()) {
            out << ","
                << "X_" << var;
        }
    }
    out << ")";
}

auto LitCondLit::output(SymbolStore &store, Assignment const &ass, std::ostream &out) const -> bool {
    static_cast<void>(store);
    static_cast<void>(ass);
    static_cast<void>(out);
    if (type_ == LitCondLitType::lit) {
        std::cerr << "TODO: cond lit " << type_ << " output something\n";
    }
    return false;
}

auto LitCondLit::copy() const -> ULit { return std::make_unique<LitCondLit>(type_, *base_, index_); }

auto LitCondLit::hash() const -> size_t {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return Util::value_hash_record<LitCondLit>(type_, reinterpret_cast<uintptr_t>(base_));
}

auto LitCondLit::equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitCondLit const *>(&other);
    return x != nullptr && std::tie(type_, base_) == std::tie(x->type_, x->base_);
}

auto LitCondLit::compare_to(Lit const &other) const -> std::weak_ordering {
    if (auto const *x = dynamic_cast<LitCondLit const *>(&other); x != nullptr) {
        return std::tie(type_, base_) <=> std::tie(x->type_, x->base_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

// StmCondLit

auto operator<<(std::ostream &out, StmCondLitType type) -> std::ostream & {
    switch (type) {
        case StmCondLitType::empty: {
            out << "empty";
            break;
        }
        case StmCondLitType::premise: {
            out << "premise";
            break;
        }
        case StmCondLitType::conclusion: {
            out << "conclusion";
            break;
        }
    }
    return out;
}

void StmCondLit::print_head(std::ostream &out) const {
    out << "#cond_lit(" << type_;
    for (auto var : base_->vars_global()) {
        out << ","
            << "X_" << var;
    }
    if (type_ != StmCondLitType::empty) {
        for (auto var : base_->vars_local()) {
            out << ","
                << "X_" << var;
        }
    }
    out << ")";
    // if (!indices_.empty()) {
    //     out << "[" << Util::p_range(indices_, ",") << "]";
    // }
}
void StmCondLit::print(std::ostream &out) const {
    out << prio_ << ": ";
    print_head(out);
    out << " <- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

auto StmCondLit::body() const -> ULitVec const & { return body_; }

auto StmCondLit::important() const -> VariableSet { return base_->vars(type_ != StmCondLitType::empty); }

void StmCondLit::init([[maybe_unused]] size_t gen) {
    // by construction, this statement does not increment the generation
}

void StmCondLit::report([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment const &ass) {}

void StmCondLit::propagate([[maybe_unused]] Queue &queue) {
    if (type_ == StmCondLitType::empty) {
    }
    std::cerr << "propagate cond lit " << type_ << "\n";
}

auto StmCondLit::priority() const -> size_t { return prio_; }

} // namespace Gringo::Ground
