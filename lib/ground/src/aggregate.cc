#include <gringo/ground/aggregate.hh>
#include <gringo/util/print.hh>

#include <typeindex>

namespace Gringo::Ground {

namespace {

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

    Base *base_;
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
    LookupMatcher(Base &base, Match const &m, MatcherType type) : base_{&base}, match_{&m}, type_{type} {}
    void init([[maybe_unused]] SymbolStore &store, size_t gen) override { base_->update(gen); }
    auto do_match([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment &ass) -> bool override {
        auto sym = match_->eval(store, ass);
        return sym && base_->contains(*sym, type_);
    }
    void print(std::ostream &out) const override { out << *match_; }

  private:
    Base *base_;
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

// StateCondLitBase

void StateCondLit::vars(VariableSet &res, bool all) const {
    if (all) {
        res.insert(local_.begin(), local_.end());
    }
    res.insert(global_.begin(), global_.end());
}

auto StateCondLit::vars(bool all) const -> VariableSet {
    VariableSet res;
    res.reserve(all ? global_.size() + local_.size() : global_.size());
    vars(res, all);
    return res;
}

auto StateCondLit::vars_global() const -> VariableVec const & { return global_; }

auto StateCondLit::vars_local() const -> VariableVec const & { return local_; }

auto StateCondLit::index() const -> size_t { return index_; }

void StateCondLit::add_empty(Assignment const &ass) {
    auto const syms = syms_atoms_.push_map(global_, [&ass](auto var) {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        return ass[var].value();
    });
    if (auto [it, ins] = atoms_.try_emplace(syms.data()); ins) {
        if (it.value().enqueue(elems_)) {
            propagate_.emplace_back(std::distance(atoms_.begin(), it));
        }
    } else {
        syms_atoms_.pop();
    }
}

void StateCondLit::add_premise(Assignment const &ass, bool fact) {
    auto it = atom_find(ass);
    // no further elements have to be accumulated if the literal is false
    if (it.value().is_false()) {
        return;
    }
    auto syms_elem = syms_elems_.push_map(Util::enumerate{local_.size() + 1}, [this, it, &ass](size_t i) {
        if (i == 0) {
            return Symbol::from_rep(std::distance(atoms_.begin(), it));
        }
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        return ass[local_[i - 1]].value();
    });

    auto [jt, ins] = elems_.try_emplace(syms_elem.data(), fact, has_conclusion_);
    // an element can only be added once
    assert(ins);

    auto &atom = it.value();
    auto &elem = jt.value();

    atom.add_elem(std::distance(elems_.begin(), jt));
    if (elem.is_blocked()) {
        if (!fact || has_conclusion_) {
            base_premise_.add(jt);
        }
    } else if (atom.enqueue(elems_)) {
        propagate_.emplace_back(atom_index(it));
    }
}

void StateCondLit::add_conclusion(Assignment const &ass, bool fact) {
    auto it = atom_find(ass);
    assert(it != atoms_.end());
    auto jt = elem_find(ass, it);
    assert(jt != elems_.end());
    auto &atom = it.value();
    auto &elem = jt.value();
    elem.mark_conclusion(fact);
    if (atom.enqueue(elems_)) {
        propagate_.emplace_back(atom_index(it));
    }
}

auto StateCondLit::propagate() -> bool {
    bool res = false;
    for (auto atom_index : propagate_) {
        auto it = atoms_.nth(atom_index);
        auto &atom = it.value();
        if (atom.propagate(elems_)) {
            base_lit_.add(it);
            res = true;
        }
    }
    propagate_.clear();
    return res;
}

auto StateCondLit::base_empty() -> BaseCondLitEmpty & { return base_empty_; }

auto StateCondLit::base_premise() -> BaseCondLitPremise & { return base_premise_; }

auto StateCondLit::base_lit() -> BaseCondLit & { return base_lit_; }

auto StateCondLit::lit_is_fact(Assignment const &ass) {
    if (rec_premise_) {
        return false;
    }
    auto it = atom_find(ass);
    assert(it != atoms_.end());
    return it->second.is_fact(elems_);
}

auto StateCondLit::atom_index(Assignment &ass) const -> std::optional<size_t> {
    auto it = atom_find(ass);
    if (it != atoms_.end()) {
        return atom_index(it);
    }
    return std::nullopt;
}

auto StateCondLit::atom_nth(size_t index) -> MapAtomCondLit::iterator { return atoms_.nth(index); }

auto StateCondLit::atom_index(MapAtomCondLit::const_iterator it) const -> size_t {
    return std::distance(atoms_.begin(), it);
}

auto StateCondLit::atom_find(Assignment const &ass) const -> MapAtomCondLit::const_iterator {
    temp_syms_.clear();
    for (auto var : global_) {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        temp_syms_.emplace_back(ass[var].value());
    }
    return atoms_.find(temp_syms_.data());
}

auto StateCondLit::atom_find(Assignment const &ass) -> MapAtomCondLit::iterator {
    temp_syms_.clear();
    for (auto var : global_) {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        temp_syms_.emplace_back(ass[var].value());
    }
    return atoms_.find(temp_syms_.data());
}

auto StateCondLit::elem_find(Assignment const &ass, MapAtomCondLit::iterator it) -> MapElemCondLit::iterator {
    temp_syms_.clear();
    temp_syms_.emplace_back(Symbol::from_rep(atom_index(it)));
    for (auto var : local_) {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        temp_syms_.emplace_back(ass[var].value());
    }
    return elems_.find(temp_syms_.data());
}

// MatchCondLit

auto MatchCondLit::vars() const -> VariableSet { return state_->vars(type_ == LitCondLitType::premise); }

auto MatchCondLit::signature(VariableSet const &bound, [[maybe_unused]] VariableSet const &bind) const -> VariableVec {
    static_cast<void>(this);
    return {bound.begin(), bound.end()};
};

auto MatchCondLit::match([[maybe_unused]] SymbolStore &store, Symbol const *sym, Assignment &ass) const -> bool {
    if (type_ == LitCondLitType::premise) {
        auto atom = state_->atom_nth(Symbol::to_rep(*sym));
        return match_(ass, atom->first, state_->vars_global()) && match_(ass, std::next(sym), state_->vars_local());
    }
    return match_(ass, sym, state_->vars_global());
};

auto MatchCondLit::eval([[maybe_unused]] SymbolStore &store, Assignment &ass) const -> std::optional<Symbol const *> {
    eval_.clear();
    bool is_premise = type_ == LitCondLitType::premise;
    if (is_premise) {
        if (auto index = state_->atom_index(ass); index) {
            eval_.emplace_back(Symbol::from_rep(*index));
        } else {
            return std::nullopt;
        }
    }
    for (auto var : is_premise ? state_->vars_local() : state_->vars_global()) {
        // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
        eval_.emplace_back(ass[var].value());
    }
    return eval_.data();
};

auto operator<<(std::ostream &out, MatchCondLit const &m) -> std::ostream & {
    out << "#cond_lit(" << m.type_;
    for (auto var : m.state_->vars_global()) {
        out << ",X_" << var;
    }
    if (m.type_ == LitCondLitType::premise) {
        for (auto var : m.state_->vars_local()) {
            out << ",X_" << var;
        }
    }
    out << ")";
    return out;
}

auto MatchCondLit::state() const -> StateCondLit & { return *state_; }

auto MatchCondLit::type() const -> LitCondLitType { return type_; }

auto MatchCondLit::match_(Assignment &ass, Symbol const *sym, VariableVec const &vars) -> bool {
    for (auto var : vars) {
        if (auto &opt = ass[var]; opt) {
            if (*opt != *sym) {
                return false;
            }
        } else {
            ass[var] = *sym;
        }
        sym = std::next(sym);
    }
    return true;
}

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
        state().vars(vars, type() == LitCondLitType::premise);
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
    auto &match = static_cast<MatchCondLit &>(*this);
    if (this->type() == LitCondLitType::empty) {
        return {make_atom_matcher(bound, state().base_empty(), match, type), index};
    }
    if (this->type() == LitCondLitType::premise) {
        return {make_atom_matcher(bound, state().base_premise(), match, type), index};
    }
    return {make_atom_matcher(bound, state().base_lit(), match, type), index};
}

auto LitCondLit::score([[maybe_unused]] std::vector<bool> const &bound) const -> double { return 1; }

void LitCondLit::print(std::ostream &out) const {
    out << "#cond_lit(" << type();
    for (auto var : state().vars_global()) {
        out << ","
            << "X_" << var;
    }
    if (type() == LitCondLitType::premise) {
        for (auto var : state().vars_local()) {
            out << ","
                << "X_" << var;
        }
    }
    out << ")";
    if (index_ != stratified_index) {
        out << "[" << index_ << "]";
    }
}

auto LitCondLit::output([[maybe_unused]] SymbolStore &store, Assignment const &ass, std::ostream &out) const -> bool {
    if (type() == LitCondLitType::lit) {
        // TODO: fix once there is a proper output
        out << "#cond_lit(TODO)";
        return !state().lit_is_fact(ass);
    }
    return false;
}

auto LitCondLit::copy() const -> ULit { return std::make_unique<LitCondLit>(type(), state(), index_); }

auto LitCondLit::hash() const -> size_t {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return Util::value_hash_record<LitCondLit>(type(), reinterpret_cast<uintptr_t>(&state()));
}

auto LitCondLit::equal_to(Lit const &other) const -> bool {
    auto const *x = dynamic_cast<LitCondLit const *>(&other);
    return x != nullptr && std::make_tuple(type(), &state()) == std::make_tuple(x->type(), &x->state());
}

auto LitCondLit::compare_to(Lit const &other) const -> std::weak_ordering {
    if (auto const *x = dynamic_cast<LitCondLit const *>(&other); x != nullptr) {
        return std::make_tuple(type(), &state()) <=> std::make_tuple(x->type(), &x->state());
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
    if (index_ != stratified_index) {
        out << "[" << index_ << "]";
    }
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

void StmCondLit::report([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Assignment const &ass) {
    switch (type_) {
        case StmCondLitType::empty: {
            base_->add_empty(ass);
            break;
        }
        case StmCondLitType::premise: {
            // TODO: fix once there is a proper output
            bool fact = true;
            for (auto const &lit : body_) {
                std::stringstream out;
                if (lit->output(store, ass, out)) {
                    fact = false;
                }
            }
            base_->add_premise(ass, fact);
            break;
        }
        case StmCondLitType::conclusion: {
            // TODO: fix once there is a proper output
            bool fact = true;
            for (auto const &lit : body_) {
                std::stringstream out;
                if (lit->output(store, ass, out)) {
                    fact = false;
                }
            }
            base_->add_conclusion(ass, fact);
            break;
        }
    }
}

void StmCondLit::propagate([[maybe_unused]] Queue &queue) {
    switch (type_) {
        case StmCondLitType::empty: {
            if (base_->base_empty().has_update()) {
                if (index_ != stratified_index) {
                    queue.propagate(index_);
                }
            }
            break;
        }
        case StmCondLitType::premise: {
            if (base_->base_premise().has_update()) {
                if (index_ != stratified_index) {
                    queue.propagate(index_);
                }
            }
            // note that atoms not blocked at this point are not added to the premise base
            // thus, we have to propagate here already
            if (base_->propagate() && base_->index() != stratified_index) {
                queue.propagate(base_->index());
            }
            break;
        }
        case StmCondLitType::conclusion: {
            if (base_->base_lit().has_update()) {
                if (index_ != stratified_index) {
                    queue.propagate(index_);
                }
            }
            // propagate further conditional literals
            if (base_->propagate() && base_->index() != stratified_index) {
                queue.propagate(base_->index());
            }
            break;
        }
    }
}

auto StmCondLit::priority() const -> size_t { return prio_; }

} // namespace Gringo::Ground
