#include <gringo/ground/literal.hh>

#include <typeindex>

#include <iostream>

namespace Gringo::Ground {

namespace {

class OnceMatcher : public Matcher {
  public:
    OnceMatcher() = default;
    virtual auto do_match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) -> bool {
        return true;
    }
    void init([[maybe_unused]] size_t gen) override {}
    void match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) override { match_ = true; }
    auto next(SymbolStore &store, Assignment &ass) -> bool override {
        if (match_) {
            match_ = false;
            return do_match(store, ass);
        }
        return false;
    }

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

  private:
    Term const *lhs_;
    Term const *rhs_;
    VariableVec free_;
};

class NonFactMatcher : public OnceMatcher {
  public:
    NonFactMatcher(Base const &base, Term const &term) : base_{&base}, term_{&term} {}
    void init(size_t gen) override { base_->update(gen); }
    auto do_match(SymbolStore &store, Assignment &ass) -> bool override {
        auto sym = term_->eval(store, ass);
        return !sym || !base_->is_fact(*sym);
    }

  private:
    Base const *base_;
    Term const *term_;
};

class FullIndex {
  public:
    FullIndex(Base const &base) : base_{&base} {}
    void init(size_t gen) { base_->update(gen); }
    auto match(MatcherType type) -> std::pair<size_t, size_t> {
        // select the index of the first atom of the matcher's generation
        auto current = base_->begin(type);
        // select the first interval that contains an atom of the matcher's generation
        return {current, std::distance(index_.begin(),
                                       std::upper_bound(index_.begin(), index_.end(), current,
                                                        [](auto const &a, auto const &b) { return a < b.second; }))};
    }
    auto next(SymbolStore &store, Assignment &ass, Term const &term, VariableVec &free, MatcherType type,
              size_t &current, size_t interval) -> bool {
        auto n = base_->end(type);
        // populate the index if it does not yet hold enough elements
        for (; imported_ <= current; ++imported_) {
            // the current index can no longer provide a match
            if (current >= n) {
                return false;
            }
            for (auto const &var : free) {
                ass[var] = std::nullopt;
            }
            if (term.match(store, base_->nth(imported_)->first, ass)) {
                if (index_.empty() || index_.back().second != imported_) {
                    interval = index_.size();
                    index_.emplace_back(imported_, imported_ + 1);
                } else {
                    ++index_.back().second;
                }
                if (imported_ == current) {
                    // the current index matches
                    ++current;
                    ++imported_;
                    return true;
                }
            } else if (current == imported_) {
                // the current index does not match
                ++current;
            }
        }
        // obtain a (guaranteed) match from the index
        for (; interval < index_.size(); ++interval) {
            // all atoms in the interval have been matched
            if (current < index_[interval].first) {
                current = index_[interval].first;
            }
            // the current index can no longer provide a match
            if (current >= n) {
                return false;
            }
            // match the next atom in the interval
            if (current < index_[interval].second) {
                for (auto const &var : free) {
                    ass[var] = std::nullopt;
                }
                return term.match(store, base_->nth(current++)->first, ass);
            }
        }
        return false;
    }

  private:
    Base const *base_;
    std::vector<std::pair<size_t, size_t>> index_;
    size_t imported_ = 0;
};

constexpr auto page_size = size_t{4096};

// NOLINTBEGIN(cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-bounds-pointer-arithmetic)

template <class T> class SpanStack {
  public:
    SpanStack(size_t size) : size_{size} {}
    auto push(std::span<T const> arr) -> std::span<T> {
        return root_->push_map(arr, [](auto const &val) { return val; });
    }
    template <typename Source, typename Map> auto push_map(Source const &c, Map map) {
        if (root_ == nullptr || root_->size() == chunk_size()) {
            auto *prev = root_;
            root_ = static_cast<Chunk *>(::operator new(sizeof(Chunk) + sizeof(T) * chunk_size(),
                                                        static_cast<std::align_val_t>(alignof(Chunk))));
            new (root_) Chunk{prev};
        }
        return root_->push_map(c, map);
    }
    void pop() {
        if (root_->empty()) {
            root_ = root_->free();
        }
        root_->pop(size_);
    }
    ~SpanStack() noexcept {
        for (; root_ != nullptr; root_ = root_->free()) {
        }
    }

  private:
    class Chunk {
      public:
        Chunk(Chunk *next = nullptr) : next_{next} {}
        ~Chunk() noexcept {
            std::for_each_n(data_, size_, [](auto &x) { x.~T(); });
        }
        [[nodiscard]] auto empty() const -> bool { return size_ == 0; }
        [[nodiscard]] auto size() const -> size_t { return size_; }
        template <typename Source, typename Map> auto push_map(Source const &arr, Map map) -> std::span<T> {
            // Note: does not provide strong exception guarantee
            auto *beg = data_ + size_;
            auto *ins = beg;
            for (auto const &val : arr) {
                new (ins) T(map(val));
                ++ins;
                ++size_;
            }
            return {beg, arr.size()};
        }
        void pop(size_t size) {
            auto end = data_ + size_;
            std::for_each(end - size, end, [](auto &x) { x.~T(); });
            size_ -= size;
        }
        auto free() noexcept -> Chunk * {
            auto ret = next_;
            this->~Chunk();
            ::operator delete(static_cast<void *>(this), static_cast<std::align_val_t>(alignof(Chunk)));
            return ret;
        }

      private:
        Chunk *next_;
        size_t size_ = 0;
        // NOLINTNEXTLINE
        T data_[0];
    };
    auto chunk_size() {
        auto n = page_size / sizeof(T);
        return size_ * (size_ >= n ? 1 : n / size_);
    }
    Chunk *root_ = nullptr;
    size_t size_;
};

struct SpanHash {
    SpanHash(size_t size) : size{size} {}
    template <class T> auto operator()(T const *sym) const -> size_t { return Util::value_hash(std::span(sym, size)); }
    size_t size;
};

struct SpanEqualTo {
    SpanEqualTo(size_t size) : size{size} {}
    template <class T> auto operator()(T const *a, T const *b) const -> bool {
        return Util::value_equal_to{}(std::span(a, size), std::span(b, size));
    }
    size_t size;
};

class HashIndex {
  public:
    using BindVec = std::vector<std::pair<size_t, Symbol *>>;
    using IndexMap = Util::unordered_map<Symbol *, BindVec, SpanHash, SpanEqualTo>;

    HashIndex(Base const &base, size_t bound, size_t bind)
        : base_{&base}, bound_values_{bound}, bind_values_{bind}, index_{0, SpanHash{bound}, SpanEqualTo{bound}} {
        assert(bound > 0 && bind > 0);
        temp_values_.reserve(bound);
    }
    void init(size_t gen) { base_->update(gen); }

    auto next(SymbolStore &store, Assignment &ass, VariableVec &bound_vars, VariableVec &bind_vars, Term const &term,
              MatcherType type, IndexMap::iterator &it, size_t &cur) -> bool {
        if (cur == std::numeric_limits<size_t>::max()) {
            temp_values_.clear();
            for (auto const &var : bound_vars) {
                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                temp_values_.emplace_back(*ass[var]);
            }
            if (it = index_.find(temp_values_.data()); it != index_.end()) {
                cur = static_cast<size_t>(std::distance(
                    it->second.begin(), std::lower_bound(it->second.begin(), it->second.end(), base_->begin(type),
                                                         [](auto const &a, auto const &b) { return a.first < b; })));
                if (cur < it->second.size()) {
                    return bind_next(ass, bind_vars, type, it, cur);
                }
            }
            return import_next(store, ass, bound_vars, bind_vars, term, type, std::span(temp_values_), it, cur);
        }
        if (cur < it->second.size()) {
            return bind_next(ass, bind_vars, type, it, cur);
        }
        return import_next(store, ass, bound_vars, bind_vars, term, type, std::span(it->first, bound_vars.size()), it,
                           cur);
    }

  private:
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
                     Term const &term, MatcherType type, std::span<Symbol> bound_vals, IndexMap::iterator &it,
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
                auto bind_match = bind_values_.push_map(bind_vars, [&ass](auto const &var) { return *ass[var]; });
                jt.value().emplace_back(imported_, bind_match.data());
                // check if the imported symbol is a match
                // note that the current assignment captures the match
                if (i <= imported_ && Util::value_equal_to{}(bound_match, bound_vals)) {
                    ++imported_;
                    cur = jt->second.size();
                    it = jt;
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
    SpanStack<Symbol> bound_values_;
    SpanStack<Symbol> bind_values_;
    Util::unordered_map<Symbol *, std::vector<std::pair<size_t, Symbol *>>, SpanHash, SpanEqualTo> index_;
    size_t imported_ = 0;
};

// NOLINTEND(cppcoreguidelines-pro-bounds-array-to-pointer-decay,cppcoreguidelines-pro-bounds-pointer-arithmetic)

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
    LookupMatcher(Base const &base, Term const &term, MatcherType type) : base_{&base}, term_{&term}, type_{type} {}
    void init(size_t gen) override { base_->update(gen); }
    auto do_match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) -> bool override {
        auto sym = term_->eval(store, ass);
        return sym && base_->contains(*sym, type_);
    }

  private:
    Base const *base_;
    Term const *term_;
    MatcherType type_;
};

class FullMatcher : public Matcher {
  public:
    FullMatcher(FullIndex &index, Term const &term, VariableVec free, MatcherType type)
        : index_{&index}, term_{&term}, free_{std::move(free)}, type_{type} {}
    void init(size_t gen) override { index_->init(gen); }
    void match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) override {
        std::tie(current_, interval_) = index_->match(type_);
    }
    auto next(SymbolStore &store, Assignment &ass) -> bool override {
        return index_->next(store, ass, *term_, free_, type_, current_, interval_);
    }

  private:
    FullIndex *index_;
    Term const *term_;
    VariableVec free_;
    MatcherType type_;
    size_t interval_ = 0;
    size_t current_ = 0;
};

class HashMatcher : public Matcher {
  public:
    HashMatcher(HashIndex &index, Term const &term, VariableVec bound, VariableVec bind, MatcherType type)
        : index_{&index}, term_{&term}, bound_{std::move(bound)}, bind_{std::move(bind)}, type_{type} {}
    void init(size_t gen) override {
        // std::cerr << "set generation of " << *term_ << " to " << gen << "\n";
        index_->init(gen);
    }
    void match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) override {
        current_ = std::numeric_limits<size_t>::max();
        // std::cerr << "matching: " << *term_ << " in range " << current_ << "-" << base_->end(type_) << "\n";
    }
    auto next(SymbolStore &store, Assignment &ass) -> bool override {
        return index_->next(store, ass, bound_, bind_, *term_, type_, it_, current_);
    }

  private:
    HashIndex *index_;
    Term const *term_;
    VariableVec bound_;
    VariableVec bind_;
    MatcherType type_;
    HashIndex::IndexMap::iterator it_;
    size_t current_ = std::numeric_limits<size_t>::max();
};

class IntervalMatcher : public Matcher {
  public:
    IntervalMatcher(Term const &lhs, Term const &lower, Term const &upper, VariableVec free)
        : lhs_{&lhs}, lower_{&lower}, upper_{&upper}, free_{std::move(free)} {}
    void init([[maybe_unused]] size_t gen) override {}
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

  private:
    Term const *lhs_;
    Term const *lower_;
    Term const *upper_;
    VariableVec free_;
    Number val_current_ = 0;
    Number val_upper_ = 0;
};

} // namespace

auto operator<<(std::ostream &out, Sign sign) -> std::ostream & {
    switch (sign) {
        case Sign::none: {
            break;
        }
        case Sign::once: {
            out << "not ";
            break;
        }
        case Sign::twice: {
            out << "not not ";
            break;
        }
    }
    return out;
}

auto operator<<(std::ostream &out, Relation rel) -> std::ostream & {
    switch (rel) {
        case Relation::equal: {
            out << "=";
            break;
        }
        case Relation::greater: {
            out << ">";
            break;
        }
        case Relation::greater_equal: {
            out << ">=";
            break;
        }
        case Relation::less: {
            out << "<";
            break;
        }
        case Relation::less_equal: {
            out << "<=";
            break;
        }
        case Relation::not_equal: {
            out << "!=";
            break;
        }
    }
    return out;
}

void LitInterval::print(std::ostream &out) const { out << *lhs_ << "=" << *lower_ << ".." << *upper_; }

auto LitInterval::output(SymbolStore &store, Assignment const &ass, std::ostream &out) const -> bool {
    if (auto lhs = lhs_->eval(store, ass), lower = lower_->eval(store, ass), upper = upper_->eval(store, ass);
        lhs && lower && upper) {
        out << *lower << "<=" << *lhs << "<=" << *upper;
    } else {
        out << "#false";
    }
    return false;
}

auto LitInterval::domain([[maybe_unused]] bool domain) const -> bool { return true; }

auto LitInterval::recursive() const -> bool { return false; }

void LitInterval::vars(VariableSet &vars, VarSelectMode mode) const {
    switch (mode) {
        case VarSelectMode::all: {
            lhs_->vars(vars);
            lower_->vars(vars);
            upper_->vars(vars);
            break;
        }
        case VarSelectMode::provide: {
            lhs_->vars(vars);
            break;
        }
        case VarSelectMode::depend: {
            lower_->vars(vars);
            upper_->vars(vars);
            break;
        }
    }
}

auto LitInterval::matcher([[maybe_unused]] MatcherType type, [[maybe_unused]] std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    VariableSet vars;
    lhs_->vars(vars);
    erase_if(vars, [&bound](auto const &var) { return bound[var]; });
    return {std::make_unique<IntervalMatcher>(*lhs_, *lower_, *upper_, vars.release()), std::nullopt};
}

auto LitInterval::score([[maybe_unused]] std::vector<bool> const &bound) const -> double {
    // TODO: compute proper score
    // NOLINTNEXTLINE(readability-magic-numbers)
    return 100;
}

auto LitInterval::hash() const -> size_t { return Util::value_hash_record<LitInterval>(lhs_, lower_, upper_); }

auto LitInterval::equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitInterval const *>(&other);
    if (x != nullptr) {
        return std::tie(*lhs_, *lower_, *upper_) == std::tie(*x->lhs_, *x->lower_, *x->upper_);
    }
    return false;
}

auto LitInterval::compare_to(Lit const &other) const -> std::weak_ordering {
    auto const *x = dynamic_cast<LitInterval const *>(&other);
    if (x != nullptr) {
        return std::tie(*lhs_, *lower_, *upper_) <=> std::tie(*x->lhs_, *x->lower_, *x->upper_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

void LitComparison::print(std::ostream &out) const { out << *lhs_ << cmp_ << *rhs_; }

auto LitComparison::output(SymbolStore &store, Assignment const &ass, std::ostream &out) const -> bool {
    if (auto lhs = lhs_->eval(store, ass), rhs = rhs_->eval(store, ass); lhs && rhs) {
        out << *lhs << cmp_ << *rhs;
    } else {
        out << "#false";
    }
    return false;
}

auto LitComparison::domain([[maybe_unused]] bool domain) const -> bool { return true; }

auto LitComparison::recursive() const -> bool { return false; }

void LitComparison::vars(VariableSet &vars, VarSelectMode mode) const {
    if (cmp_ != Relation::equal) {
        mode = VarSelectMode::all;
    }
    switch (mode) {
        case VarSelectMode::all: {
            lhs_->vars(vars);
            rhs_->vars(vars);
            break;
        }
        case VarSelectMode::provide: {
            lhs_->vars(vars, true);
            break;
        }
        case VarSelectMode::depend: {
            // Note: the rewriting ensures that if variables can be provided,
            //       then all of them can be provided.
            VariableSet provide;
            lhs_->vars(provide, true);
            if (provide.empty()) {
                lhs_->vars(vars);
            }
            rhs_->vars(vars);
            break;
        }
    }
}

auto LitComparison::matcher([[maybe_unused]] MatcherType type, [[maybe_unused]] std::vector<bool> const &bound)
    -> std::pair<UMatcher, std::optional<size_t>> {
    if (cmp_ == Relation::equal) {
        VariableSet vars;
        lhs_->vars(vars, true);
        erase_if(vars, [&bound](auto const &var) { return bound[var]; });
        if (!vars.empty()) {
            return {std::make_unique<AssignMatcher>(*lhs_, *rhs_, vars.release()), std::nullopt};
        }
    }
    return {std::make_unique<CmpMatcher>(*lhs_, cmp_, *rhs_), std::nullopt};
}

auto LitComparison::score([[maybe_unused]] std::vector<bool> const &bound) const -> double { return 0; }

auto LitComparison::hash() const -> size_t {
    if (cmp_ == Relation::equal && *rhs_ < *lhs_) {
        return Util::value_hash_record<LitComparison>(rhs_, cmp_, lhs_);
    }
    return Util::value_hash_record<LitComparison>(lhs_, cmp_, rhs_);
}

auto LitComparison::equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitComparison const *>(&other);
    if (x != nullptr) {
        if (cmp_ == Relation::equal && x->cmp_ == Relation::equal) {
            return std::tie(*lhs_, *rhs_) == std::tie(*x->lhs_, *x->rhs_) ||
                   std::tie(*lhs_, *rhs_) == std::tie(*x->rhs_, *x->lhs_);
        }
        return std::tie(*lhs_, cmp_, *rhs_) == std::tie(*x->lhs_, x->cmp_, *x->rhs_);
    }
    return false;
}

auto LitComparison::compare_to(Lit const &other) const -> std::weak_ordering {
    auto const *x = dynamic_cast<LitComparison const *>(&other);
    if (x != nullptr) {
        if (cmp_ == Relation::equal && x->cmp_ == Relation::equal && (*rhs_ < *lhs_ != *x->rhs_ < *x->lhs_)) {
            return std::tie(*lhs_, *rhs_) <=> std::tie(*x->rhs_, *x->lhs_);
        }
        return std::tie(*lhs_, cmp_, *rhs_) <=> std::tie(*x->lhs_, x->cmp_, *x->rhs_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

void LitSymbolic::print(std::ostream &out) const {
    out << sign_ << *atom_;
    if (index_ != std::numeric_limits<size_t>::max()) {
        out << "[" << index_ << "]";
    }
}

auto LitSymbolic::output(SymbolStore &store, Assignment const &ass, std::ostream &out) const -> bool {
    if (auto sym = atom_->eval(store, ass)) {
        out << sign_ << *sym;
        if (sign_ == Sign::once) {
            return index_ != std::numeric_limits<size_t>::max() || base_->contains(*sym);
        }
        return !base_->is_fact(*sym);
    }
    out << "#false";
    return true;
}

auto LitSymbolic::domain(bool domain) const -> bool {
    // check if the base of the literal is domain
    if (!base_->domain()) {
        return false;
    }
    // stratifed literals with a domain base can be completely evaluated
    if (index_ == std::numeric_limits<size_t>::max()) {
        return true;
    }
    // return true if the literal is in a domain component
    // noting that a domain component cannot contain negative literals
    return domain;
}

auto LitSymbolic::recursive() const -> bool {
    return sign_ == Sign::none && index_ != std::numeric_limits<size_t>::max();
}

void LitSymbolic::vars(VariableSet &vars, VarSelectMode mode) const {
    switch (mode) {
        case VarSelectMode::all: {
            atom_->vars(vars);
            break;
        }
        case VarSelectMode::provide: {
            if (sign_ == Sign::none || (sign_ == Sign::twice && index_ == std::numeric_limits<size_t>::max())) {
                atom_->vars(vars);
            }
            break;
        }
        case VarSelectMode::depend: {
            if (sign_ == Sign::once || (sign_ == Sign::twice && index_ != std::numeric_limits<size_t>::max())) {
                atom_->vars(vars);
            }
            break;
        }
    }
}

auto LitSymbolic::matcher(MatcherType type,
                          std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> {
    if (sign_ == Sign::once) {
        return {std::make_unique<NonFactMatcher>(*base_, *atom_), std::nullopt};
    }
    if (sign_ == Sign::twice && index_ != std::numeric_limits<size_t>::max()) {
        return {std::make_unique<OnceMatcher>(), std::nullopt};
    }
    // TODO: proper matcher creation
    VariableSet bind;
    VariableSet lookup;
    atom_->vars(bind);
    erase_if(bind, [&bound, &lookup](auto const &var) {
        if (bound[var]) {
            lookup.insert(var);
        }
        return bound[var];
    });
    auto index = std::optional<size_t>{};
    if (index_ != std::numeric_limits<size_t>::max() && type == MatcherType::new_atoms) {
        index = index_;
    }
    if (bind.empty()) {
        return {std::make_unique<LookupMatcher>(*base_, *atom_, type), index};
    }
    auto names = Util::unordered_map<size_t, size_t>{};
    names.reserve(bind.size() + lookup.size());
    auto sig_term = atom_->rename(names);
    auto sig_lookup = std::vector<size_t>{};
    sig_lookup.reserve(lookup.size());
    for (auto const &var : lookup) {
        sig_lookup.emplace_back(names[var]);
    }

    if (lookup.empty()) {
        auto &full = base_->context<IndexSet>().add_full(*base_, std::move(sig_lookup), std::move(sig_term));
        return {std::make_unique<FullMatcher>(full, *atom_, bind.release(), type), index};
    }
    auto &hash = base_->context<IndexSet>().add_hash(*base_, std::move(sig_lookup), bind.size(), std::move(sig_term));
    return {std::make_unique<HashMatcher>(hash, *atom_, lookup.release(), bind.release(), type), index};
}

auto LitSymbolic::score(std::vector<bool> const &bound) const -> double {
    static_cast<void>(bound);
    // TODO: proper score computation
    return 2;
}

auto LitSymbolic::hash() const -> size_t { return Util::value_hash_record<LitSymbolic>(sign_, atom_); }

auto LitSymbolic::equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitSymbolic const *>(&other);
    return x != nullptr && std::tie(sign_, *atom_) == std::tie(x->sign_, *x->atom_);
}

auto LitSymbolic::compare_to(Lit const &other) const -> std::weak_ordering {
    if (auto const *x = dynamic_cast<LitSymbolic const *>(&other); x != nullptr) {
        return std::tie(sign_, *atom_) <=> std::tie(x->sign_, *x->atom_);
    }
    return std::type_index(typeid(*this)) <=> std::type_index(typeid(other));
}

} // namespace Gringo::Ground
