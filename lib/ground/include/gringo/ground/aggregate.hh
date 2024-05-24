#pragma once

#include <gringo/ground/statement.hh>

#include <gringo/util/enumerate.hh>
#include <gringo/util/span_stack.hh>

#include <iostream>

namespace Gringo::Ground {

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

template <class Match>
concept IsMatch = requires(Match const &m) {
    { m.vars() } -> std::same_as<VariableSet>;
    m.match(std::declval<SymbolStore &>(), std::declval<typename Match::Key>(), std::declval<Assignment &>());
    m.eval(std::declval<SymbolStore &>(), std::declval<Assignment &>());
    m.signature(std::declval<VariableSet const &>(), std::declval<VariableSet const &>());
    std::declval<std::ostream &>() << m;
};

struct BaseCondLit {
  public:
    // NOLINTNEXTLINE(performance-enum-size)
    enum class TruthConclusion : size_t {
        true_ = 0,
        false_ = 1,
        derived = 2,
        unknown = 3,
    };
    struct ElemState {
      public:
        ElemState(bool premise_is_fact, bool has_conclusion)
            : conclusion_truth_{has_conclusion ? TruthConclusion::unknown : TruthConclusion::false_},
              premise_is_fact_{static_cast<uint8_t>(premise_is_fact)} {}
        void mark_conclusion(bool fact) {
            assert(conclusion_truth_ == TruthConclusion::unknown);
            conclusion_truth_ = fact ? TruthConclusion::true_ : TruthConclusion::derived;
        }
        [[nodiscard]] auto is_fact() const -> bool { return conclusion_truth_ == TruthConclusion::true_; }
        [[nodiscard]] auto is_blocked() const -> bool {
            return premise_is_fact_ != 0 &&
                   (conclusion_truth_ == TruthConclusion::false_ || conclusion_truth_ == TruthConclusion::unknown);
        }
        void set_offset(size_t offset) { offset_ = offset; }
        [[nodiscard]] auto offset() const -> size_t { return offset_; }

      private:
        size_t offset_ : 56 = 0;
        TruthConclusion conclusion_truth_ : 7;
        size_t premise_is_fact_ : 1;
    };
    // we can use here that the number of local variables is fixed
    using ElemMap = Util::ordered_map<Symbol const *, ElemState, Util::SpanHash, Util::SpanEqualTo>;

    struct AtomState {
      public:
        AtomState() = default;
        void add_elem(size_t index) { elems_.emplace_back(index); }
        [[nodiscard]] auto enqueue(ElemMap const &elems) -> bool {
            if (!enqueued_ && !propagated_ &&
                (elems_propagated_ == elems_.size() || !elems.nth(elems_[elems_propagated_]).value().is_blocked())) {
                enqueued_ = true;
                return true;
            }
            return false;
        }
        [[nodiscard]] auto propagate(ElemMap const &elems) -> bool {
            assert(!propagated_ && enqueued_);
            enqueued_ = false;
            for (auto n = elems_.size(); elems_propagated_ < n; ++elems_propagated_) {
                if (elems.nth(elems_[elems_propagated_]).value().is_blocked()) {
                    return false;
                }
            }
            propagated_ = true;
            return true;
        }
        [[nodiscard]] auto is_fact(ElemMap const &elems) const -> bool {
            return std::all_of(elems_.begin(), elems_.end(),
                               [&elems](auto idx) { return elems.nth(idx).value().is_fact(); });
        }
        [[nodiscard]] auto is_false() const -> bool { return false_; }
        void set_offset(size_t offset) { offset_ = offset; }
        [[nodiscard]] auto offset() const -> size_t { return offset_; }

      private:
        std::vector<size_t> elems_;
        size_t offset_ = 0;
        size_t elems_propagated_ = 0;
        bool propagated_ = false;
        bool enqueued_ = false;
        bool false_ = false;
    };
    // we can use here that the number of global variables is fixed
    using AtomMap = Util::ordered_map<Symbol const *, AtomState, Util::SpanHash, Util::SpanEqualTo>;

    class BaseEmpty {
      public:
        using Key = Symbol const *;

        BaseEmpty(AtomMap &atoms) : atoms_{&atoms} {}

        //! Get the index of the first atom in the given generation.
        auto begin(MatcherType type) const -> size_t { return counts_.begin(type); }

        //! Get the index plus one of the last atom in the given generation.
        auto end(MatcherType type) const -> size_t { return counts_.end(type); }

        //! Check if the base contains the given atom with in the given generation.
        [[nodiscard]] auto contains(Symbol const *sym, MatcherType type) const -> bool {
            auto index = static_cast<size_t>(std::distance(atoms_->begin(), atoms_->find(sym)));
            return counts_.contains(index, type);
        }

        //! Get the n-th atom in the base.
        auto nth(size_t i) const -> AtomMap::const_iterator { return atoms_->nth(i); }

        //! Get the n-th atom in the base.
        auto nth(size_t i) -> AtomMap::iterator { return atoms_->nth(i); }

        //! Update the generation counts.
        void update(size_t generation) const { counts_.update(generation, atoms_->size()); }

        template <class T> auto context() -> T & {
            if (context_ != nullptr) {
                if (auto res = dynamic_cast<T *>(context_.get()); res != nullptr) {
                    return *res;
                }
                throw std::bad_cast();
            }
            context_ = std::make_unique<T>();
            return static_cast<T &>(*context_);
        }

        // other

        [[nodiscard]] auto has_update() const -> bool { return counts_.has_update(atoms_->size()); }

      private:
        AtomMap *atoms_;
        std::unique_ptr<BaseContext> context_;
        GenerationCounts mutable counts_;
    };

    class MatchEmpty {
      public:
        using Key = Symbol const *;

        MatchEmpty(BaseCondLit &base) : base_{&base} { eval_.reserve(base_->vars_global().size()); }

        [[nodiscard]] auto vars() const -> VariableSet { return base_->vars(false); }

        [[nodiscard]] auto match([[maybe_unused]] SymbolStore &store, Symbol const *sym,
                                 Assignment &ass) const -> bool {
            for (auto const &var : base_->vars_global()) {
                if (auto &opt = ass[var]; opt) {
                    if (*opt != *sym) {
                        return false;
                    }
                } else {
                    ass[var] = *sym;
                }
                ++sym; // NOLINT
            }
            return true;
        };

        [[nodiscard]] auto eval([[maybe_unused]] SymbolStore &store,
                                Assignment &ass) const -> std::optional<Symbol const *> {
            eval_.clear();
            for (auto var : base_->vars_global()) {
                // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
                eval_.emplace_back(ass[var].value());
            }
            return eval_.data();
        };

        [[nodiscard]] auto signature(VariableSet const &bound,
                                     [[maybe_unused]] VariableSet const &bind) const -> VariableVec {
            static_cast<void>(this);
            return {bound.begin(), bound.end()};
        };

        friend auto operator<<(std::ostream &out, MatchEmpty const &m) -> std::ostream & {
            out << "#cond_lit(empty";
            for (auto var : m.base_->vars_global()) {
                out << ",X_" << var;
            }
            out << ")";
            return out;
        }

      private:
        std::vector<Symbol> mutable eval_;
        BaseCondLit *base_;
    };

    class BasePremise {
      public:
        using Key = Symbol const *;

        BasePremise(ElemMap &elems) : elems_{&elems} {}

        //! Add a blocked element to the base.
        void add(ElemMap::iterator it) {
            assert(it.value().is_blocked());
            it.value().set_offset(base_.size());
            base_.emplace_back(std::distance(elems_->begin(), it));
        }

        //! Get the index of the first atom in the given generation.
        auto begin(MatcherType type) const -> size_t { return counts_.begin(type); }

        //! Get the index plus one of the last atom in the given generation.
        auto end(MatcherType type) const -> size_t { return counts_.end(type); }

        //! Check if the base contains the given atom with in the given generation.
        [[nodiscard]] auto contains(Symbol const *sym, MatcherType type) const -> bool {
            auto index = elems_->find(sym)->second.offset();
            return counts_.contains(index, type);
        }

        //! Get the n-th atom in the base.
        auto nth(size_t i) const -> ElemMap::const_iterator { return elems_->nth(base_[i]); }

        //! Get the n-th atom in the base.
        auto nth(size_t i) -> ElemMap::iterator { return elems_->nth(base_[i]); }

        //! Update the generation counts.
        void update(size_t generation) const { counts_.update(generation, elems_->size()); }

        template <class T> auto context() -> T & {
            if (context_ != nullptr) {
                if (auto res = dynamic_cast<T *>(context_.get()); res != nullptr) {
                    return *res;
                }
                throw std::bad_cast();
            }
            context_ = std::make_unique<T>();
            return static_cast<T &>(*context_);
        }

        // other

        [[nodiscard]] auto has_update() const -> bool { return counts_.has_update(base_.size()); }

      private:
        ElemMap *elems_;
        std::vector<size_t> base_;
        std::unique_ptr<BaseContext> context_;
        GenerationCounts mutable counts_;
    };

    class MatchPremise {
      public:
        using Key = Symbol const *;

        MatchPremise(BaseCondLit &base) : base_{&base} { eval_.reserve(base_->global_.size()); }

        [[nodiscard]] auto vars() const -> VariableSet { return base_->vars(true); }

        [[nodiscard]] auto match([[maybe_unused]] SymbolStore &store, Symbol const *sym,
                                 Assignment &ass) const -> bool {
            auto atom = base_->atoms_.nth(Symbol::to_rep(*sym++)); // NOLINT
            auto const *gsym = atom->first;
            for (auto var : base_->global_) {
                if (auto &opt = ass[var]; opt) {
                    if (*opt != *gsym) {
                        return false;
                    }
                } else {
                    ass[var] = *gsym;
                }
                ++gsym; // NOLINT
            }
            for (auto var : base_->local_) {
                if (auto &opt = ass[var]; opt) {
                    if (*opt != *sym) {
                        return false;
                    }
                } else {
                    ass[var] = *sym;
                }
                ++sym; // NOLINT
            }
            return true;
        };

        [[nodiscard]] auto eval([[maybe_unused]] SymbolStore &store,
                                Assignment &ass) const -> std::optional<Symbol const *> {
            auto atom = base_->find_atom(ass);
            if (atom == base_->atoms_.end()) {
                return std::nullopt;
            }
            eval_.emplace_back(Symbol::from_rep(std::distance(base_->atoms_.begin(), atom)));
            for (auto var : base_->local_) {
                eval_.emplace_back(ass[var].value()); // NOLINT
            }
            return eval_.data();
        };

        [[nodiscard]] auto signature(VariableSet const &bound,
                                     [[maybe_unused]] VariableSet const &bind) const -> VariableVec {
            static_cast<void>(this);
            return {bound.begin(), bound.end()};
        };

        friend auto operator<<(std::ostream &out, MatchPremise const &m) -> std::ostream & {
            out << "#cond_lit(premise";
            for (auto var : m.base_->global_) {
                out << ",X_" << var;
            }
            for (auto var : m.base_->local_) {
                out << ",X_" << var;
            }
            out << ")";
            return out;
        }

      private:
        std::vector<Symbol> mutable eval_;
        BaseCondLit *base_;
    };

    class BaseLit {
      public:
        using Key = Symbol const *;

        BaseLit(AtomMap &atoms) : atoms_{&atoms} {}

        //! Add a propagated atom to the base.
        void add(AtomMap::iterator it) {
            it.value().set_offset(base_.size());
            base_.emplace_back(std::distance(atoms_->begin(), it));
        }

        //! Get the index of the first atom in the given generation.
        auto begin(MatcherType type) const -> size_t { return counts_.begin(type); }

        //! Get the index plus one of the last atom in the given generation.
        auto end(MatcherType type) const -> size_t { return counts_.end(type); }

        //! Check if the base contains the given atom with in the given generation.
        [[nodiscard]] auto contains(Symbol const *sym, MatcherType type) const -> bool {
            auto index = atoms_->find(sym)->second.offset();
            return counts_.contains(index, type);
        }

        //! Get the n-th atom in the base.
        auto nth(size_t i) const -> AtomMap::const_iterator { return atoms_->nth(base_[i]); }

        //! Get the n-th atom in the base.
        auto nth(size_t i) -> AtomMap::iterator { return atoms_->nth(base_[i]); }

        //! Update the generation counts.
        void update(size_t generation) const { counts_.update(generation, base_.size()); }

        template <class T> auto context() -> T & {
            if (context_ != nullptr) {
                if (auto res = dynamic_cast<T *>(context_.get()); res != nullptr) {
                    return *res;
                }
                throw std::bad_cast();
            }
            context_ = std::make_unique<T>();
            return static_cast<T &>(*context_);
        }

        // other

        [[nodiscard]] auto has_update() const -> bool { return counts_.has_update(base_.size()); }

      private:
        AtomMap *atoms_;
        std::vector<size_t> base_;
        std::unique_ptr<BaseContext> context_;
        GenerationCounts mutable counts_;
    };

    class MatchLit {
      public:
        using Key = Symbol const *;

        MatchLit(BaseCondLit &base) : base_{&base} { eval_.reserve(base_->global_.size()); }
        [[nodiscard]] auto match([[maybe_unused]] SymbolStore &store, Symbol const *sym,
                                 Assignment &ass) const -> bool {
            for (auto var : base_->global_) {
                if (auto &opt = ass[var]; opt) {
                    if (*opt != *sym) {
                        return false;
                    }
                } else {
                    ass[var] = *sym;
                }
                ++sym; // NOLINT
            }
            return true;
        };

        [[nodiscard]] auto eval([[maybe_unused]] SymbolStore &store,
                                Assignment &ass) const -> std::optional<Symbol const *> {
            auto atom = base_->find_atom(ass);
            if (atom == base_->atoms_.end()) {
                return std::nullopt;
            }
            eval_.emplace_back(Symbol::from_rep(std::distance(base_->atoms_.begin(), atom)));
            return eval_.data();
        };

        [[nodiscard]] auto vars() const -> VariableSet { return base_->vars(false); }

        [[nodiscard]] auto signature(VariableSet const &bound,
                                     [[maybe_unused]] VariableSet const &bind) const -> VariableVec {
            static_cast<void>(this);
            return {bound.begin(), bound.end()};
        };

        friend auto operator<<(std::ostream &out, MatchLit const &m) -> std::ostream & {
            out << "#cond_lit(lit";
            for (auto var : m.base_->global_) {
                out << ",X_" << var;
            }
            out << ")";
            return out;
        }

      private:
        std::vector<Symbol> mutable eval_;
        BaseCondLit *base_;
    };

    BaseCondLit(VariableVec local, VariableVec global, size_t index, bool has_conclusion, bool rec_premise)
        : local_{std::move(local)}, global_{std::move(global)}, syms_elems_{local_.size() + 1},
          syms_atoms_{global_.size()}, atoms_{0, Util::SpanHash{global_.size()}, Util::SpanEqualTo{global_.size()}},
          elems_{0, Util::SpanHash{local_.size() + 1}, Util::SpanEqualTo{local_.size() + 1}}, base_empty_{atoms_},
          base_premise_{elems_}, base_lit_{atoms_}, index_{index}, has_conclusion_{has_conclusion},
          rec_premise_{rec_premise} {
        temp_syms_.reserve(std::max(global_.size(), local_.size() + 1));
    }

    //! Get the variables occuring in the conditional literal.
    void vars(VariableSet &res, bool all) const {
        if (all) {
            res.insert(local_.begin(), local_.end());
        }
        res.insert(global_.begin(), global_.end());
    }

    //! Get the variables occuring in the conditional literal.
    [[nodiscard]] auto vars(bool all) const -> VariableSet {
        VariableSet res;
        res.reserve(all ? global_.size() + local_.size() : global_.size());
        vars(res, all);
        return res;
    }

    auto vars_global() const -> VariableVec const & { return global_; }

    auto vars_local() const -> VariableVec const & { return local_; }

    //! Get the update index of the conditional literal.
    [[nodiscard]] auto index() const -> size_t { return index_; }

    //! Add a new cond lit atom.
    void add_empty(Assignment const &ass) {
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

    //! Add a new cond lit element.
    void add_premise(Assignment const &ass, bool fact) {
        auto it = find_atom(ass);
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
            propagate_.emplace_back(std::distance(atoms_.begin(), it));
        }
    }

    //! Add a conclusion to an element.
    void add_conclusion(Assignment const &ass, bool fact) {

        auto it = find_atom(ass);
        assert(it != atoms_.end());
        auto jt = find_elem(ass, it);
        assert(jt != elems_.end());
        auto &atom = it.value();
        auto &elem = jt.value();
        elem.mark_conclusion(fact);
        if (atom.enqueue(elems_)) {
            propagate_.emplace_back(std::distance(atoms_.begin(), it));
        }
    }

    //! Propagate enqueued conditional literals whose elements are not blocked.
    auto propagate() -> bool {
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

    auto base_empty() -> BaseEmpty & { return base_empty_; }
    auto base_premise() -> BasePremise & { return base_premise_; }
    auto base_lit() -> BaseLit & { return base_lit_; }

    auto match_empty() const -> MatchEmpty const & { return match_empty_; }
    auto match_premise() const -> MatchPremise const & { return match_premise_; }
    auto match_lit() const -> MatchLit const & { return match_lit_; }

    auto is_fact_lit(Assignment const &ass) {
        if (rec_premise_) {
            return false;
        }
        auto it = find_atom(ass);
        assert(it != atoms_.end());
        return it->second.is_fact(elems_);
    }

  private:
    [[nodiscard]] auto find_atom(Assignment const &ass) -> AtomMap::iterator {
        temp_syms_.clear();
        for (auto var : global_) {
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            temp_syms_.emplace_back(ass[var].value());
        }
        return atoms_.find(temp_syms_.data());
    }

    [[nodiscard]] auto find_elem(Assignment const &ass, AtomMap::iterator it) -> ElemMap::iterator {
        temp_syms_.clear();
        temp_syms_.emplace_back(Symbol::from_rep(std::distance(atoms_.begin(), it)));
        for (auto var : local_) {
            // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
            temp_syms_.emplace_back(ass[var].value());
        }
        return elems_.find(temp_syms_.data());
    }

    VariableVec local_;
    VariableVec global_;
    MatchEmpty match_empty_ = MatchEmpty{*this};
    MatchPremise match_premise_ = MatchPremise{*this};
    MatchLit match_lit_ = MatchLit{*this};
    std::vector<Symbol> temp_syms_;
    Util::SpanStack<Symbol> syms_elems_;
    Util::SpanStack<Symbol> syms_atoms_;
    AtomMap atoms_;
    ElemMap elems_;
    std::vector<size_t> propagate_;
    BaseEmpty base_empty_;
    BasePremise base_premise_;
    BaseLit base_lit_;
    size_t index_;
    bool has_conclusion_;
    bool rec_premise_;
};

enum class LitCondLitType : uint8_t {
    empty = 0,
    premise = 1,
    lit = 2,
};
auto operator<<(std::ostream &out, LitCondLitType type) -> std::ostream &;

class LitCondLit : public Lit {
  public:
    LitCondLit(LitCondLitType type, BaseCondLit &base, size_t index) : base_{&base}, index_{index}, type_{type} {}
    void vars(VariableSet &vars, VarSelectMode mode) const override;
    [[nodiscard]] auto domain() const -> bool override;
    [[nodiscard]] auto recursive() const -> bool override;
    [[nodiscard]] auto matcher(MatcherType type,
                               std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> override;
    [[nodiscard]] auto score(std::vector<bool> const &bound) const -> double override;
    void print(std::ostream &out) const override;
    auto output(SymbolStore &store, Assignment const &ass, std::ostream &out) const -> bool override;
    [[nodiscard]] auto copy() const -> ULit override;
    [[nodiscard]] auto hash() const -> size_t override;
    [[nodiscard]] auto equal_to(Lit const &other) const -> bool override;
    [[nodiscard]] auto compare_to(Lit const &other) const -> std::weak_ordering override;

  private:
    BaseCondLit *base_;
    size_t index_;
    LitCondLitType type_;
};

enum class StmCondLitType : uint8_t {
    empty = 0,
    premise = 1,
    conclusion = 2,
};
auto operator<<(std::ostream &out, StmCondLitType type) -> std::ostream &;

class StmCondLit : public Stm {
  public:
    StmCondLit(StmCondLitType type, BaseCondLit &base, ULitVec body, size_t prio, size_t index)
        : base_{&base}, body_{std::move(body)}, prio_{prio}, index_{index}, type_{type} {}
    // statement interface
    void print(std::ostream &out) const override;
    [[nodiscard]] auto body() const -> ULitVec const & override;
    [[nodiscard]] auto important() const -> VariableSet override;
    // solution callback interface
    void print_head(std::ostream &out) const override;
    void init(size_t gen) override;
    void report(SymbolStore &store, Assignment const &ass) override;
    void propagate(Queue &queue) override;
    [[nodiscard]] auto priority() const -> size_t override;

  private:
    BaseCondLit *base_;
    ULitVec body_;
    size_t prio_;
    size_t index_;
    StmCondLitType type_;
};

} // namespace Gringo::Ground
