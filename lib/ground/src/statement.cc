#include <clingo/ground/matcher.hh>
#include <clingo/ground/statement.hh>

#include <clingo/util/print.hh>
#include <clingo/util/unordered_map.hh>

namespace Clingo::Ground {

void Linearizer::start(Queue &queue) { iqueue_ = &queue; }

void Linearizer::prepare(InstanceCallback &cb, ULitVec const &body, VariableSet important) {
    rec_.clear();
    todos_.clear();
    todos_.emplace_back();
    auto i = size_t{0};
    // gather indices of recursive literals and extend important variables
    for (auto const &lit : body) {
        todos_.back().emplace_back(MatcherType::all_atoms);
        if (!lit->single_pass()) {
            rec_.emplace_back(i);
        }
        // TODO: the domain flag could also be forced to avoid the is_important part
        if (cb.is_important(i) && !lit->domain()) {
            lit->vars(important, VarSelectMode::all);
        }
        ++i;
    }
    // compute linearization
    todos_.reserve(rec_.size());
    for (auto i : rec_) {
        todos_.back()[i] = MatcherType::new_atoms;
        if (i != rec_.back()) {
            todos_.emplace_back(todos_.back());
            todos_.back()[i] = MatcherType::old_atoms;
        }
    }
    build_(body);
    for (auto const &todo : todos_) {
        auto [inst, index] = order_(cb, todo, important, body);
        iqueue_->insert(std::move(inst), index);
    }
}

void Linearizer::build_(ULitVec const &lits) {
    lit_map_.clear();
    var_map_.clear();
    auto i = size_t{0};
    auto vars = VariableSet{};
    auto num_vars = size_t{0};
    lit_map_.reserve(lits.size());
    for (auto const &lit : lits) {
        lit->vars(vars, VarSelectMode::depend);
        auto depend = std::vector<size_t>(vars.begin(), vars.end());
        vars.clear();
        lit->vars(vars, VarSelectMode::provide);
        auto provide = std::vector<size_t>(vars.begin(), vars.end());
        vars.clear();
        num_vars =
            std::accumulate(depend.begin(), depend.end(), num_vars, [](auto a, auto b) { return std::max(a, b + 1); });
        num_vars = std::accumulate(provide.begin(), provide.end(), num_vars,
                                   [](auto a, auto b) { return std::max(a, b + 1); });
        lit_map_.emplace_back(0, std::move(depend), std::move(provide));
        ++i;
    }
    i = 0;
    var_map_.resize(num_vars);
    for (auto const &ndp : lit_map_) {
        for (auto var : std::get<1>(ndp)) {
            var_map_[var].emplace_back(i);
        }
        ++i;
    }
}

auto Linearizer::order_(InstanceCallback &cb, std::vector<MatcherType> const &todo, VariableSet const &important,
                        ULitVec const &lits) -> std::pair<Instantiator, std::optional<size_t>> {
    auto inst = Instantiator{cb, var_map_.size(), lits.size()};
    size_t gen = 0;
    queue_.clear();
    auto i = size_t{0};
    // initialize the queue
    for (auto &[cur, dep, prv] : lit_map_) {
        if (cur = dep.size(); cur == 0) {
            queue_.emplace_back(i, ++gen);
        }
        ++i;
    }
    auto provided = std::vector<size_t>(var_map_.size(), 0);
    auto bound = std::vector<bool>(var_map_.size(), false);
    auto make_depend = [&provided, &bound](auto const &dep, auto const &prv) {
        auto res = std::vector<size_t>{};
        for (auto var : dep) {
            assert(var < provided.size());
            res.emplace_back(provided[var]);
        }
        for (auto var : prv) {
            if (bound[var]) {
                res.emplace_back(provided[var]);
            }
        }
        std::sort(res.begin(), res.end());
        res.erase(std::unique(res.begin(), res.end()), res.end());
        return res;
    };
    // process the queue
    auto done = Util::unordered_set<Lit const *>{};
    done.reserve(lits.size());
    auto res_index = std::optional<size_t>{};
    for (size_t k = 0; !queue_.empty();) {
        // get minimum element in queue (breaking ties using insertion order)
        auto pred = [&](auto const &ei, auto const &ej) -> bool {
            auto si(lits[ei.first]->score(bound));
            auto sj(lits[ej.first]->score(bound));
            auto ti = todo[ei.first];
            auto tj = todo[ej.first];
            if ((ti == MatcherType::new_atoms || tj == MatcherType::new_atoms) && (si >= 0 && sj >= 0)) {
                assert(ti != tj);
                return ti < tj;
            }
            return std::tie(si, ei.second) < std::tie(sj, ej.second);
        };
        std::iter_swap(queue_.rbegin(), std::min_element(queue_.rbegin(), queue_.rend(), pred));
        i = queue_.back().first;
        queue_.pop_back();
        // skip if an equivalent matcher has already been added (i.e., X=Y and Y=X)
        if (!done.emplace(lits[i].get()).second && todo[i] == MatcherType::all_atoms) {
            continue;
        }
        auto [matcher, index] = lits[i]->matcher(*mbr_, todo[i], bound);
        if (index) {
            res_index = index;
        }
        auto const &[cur, dep, prv] = lit_map_[i];
        inst.add(std::move(matcher), make_depend(dep, prv));
        for (auto var : prv) {
            if (bound[var]) {
                continue;
            }
            assert(var < var_map_.size());
            for (auto j : var_map_[var]) {
                if (--std::get<0>(lit_map_[j]) == 0) {
                    queue_.emplace_back(j, ++gen);
                }
            }
            provided[var] = k;
            bound[var] = true;
        }
        ++k;
    }
    inst.finalize(make_depend(important, std::vector<size_t>{}));
    return {std::move(inst), res_index};
}

void StmRule::init_() {
    if (head_) {
        body_.emplace_back(std::make_unique<LitFactCheck>(*base_, *head_, atom_));
    }
}

void StmRule::do_print_head(std::ostream &out) const {
    if (head_) {
        if (type_ == RuleType::choice) {
            out << "{ " << *head_ << " }";
        } else {
            out << *head_;
        }
    }
}

void StmRule::do_print(std::ostream &out) const {
    out << "max: ";
    print_head(out);
    if (!indices_.empty()) {
        out << "[" << Util::p_range(indices_, ",") << "]";
    }
    out << " :- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

auto StmRule::do_body() const -> ULitVec const & { return body_; }

auto StmRule::do_important() const -> VariableSet {
    VariableSet important;
    if (head_) {
        head_->vars(important);
    }
    return important;
}

void StmRule::do_init(size_t gen) {
    if (base_ != nullptr) {
        base_->update(gen);
    }
}

auto StmRule::do_report(InstantiationContext &ctx) -> bool {
    bool fact = type_ == RuleType::normal;
    auto &out = ctx.out().body();
    for (auto const &lit : body_) {
        if (lit->output(ctx, out)) {
            fact = false;
        }
    }
    if (head_ != nullptr) {
        base_->add(atom_, fact ? StateAtom::fact : StateAtom::derived);
        ctx.out().rule(std::make_pair(atom_, type_ == RuleType::choice));
    } else if (type_ == RuleType::normal) {
        ctx.out().rule(std::nullopt);
    }
    return head_ != nullptr || !fact;
}

void StmRule::do_propagate([[maybe_unused]] SymbolStore &store, Queue &queue) {
    // Consider adding the propagation to the instantiator...
    if (base_ != nullptr && base_->has_update()) {
        for (auto const &idx : indices_) {
            queue.propagate(idx);
        }
    }
}

// definition of StmExternal

void StmExternal::init_() {
    class LitExternalCheck : public LitCheck {
      public:
        LitExternalCheck(StmExternal &stm) : stm_{&stm} {}

      private:
        [[nodiscard]] auto do_check(InstantiationContext &ctx) -> bool override {
            if (auto atom = stm_->atom_->eval(ctx.store(), ctx.ass()); atom && !stm_->base_->is_fact(*atom)) {
                stm_->res_atom_ = *atom;
            } else {
                return false;
            }
            if (stm_->type_) {
                if (auto type = stm_->type_->eval(ctx.store(), ctx.ass());
                    type && type->type() == SymbolType::function && type->args().empty() &&
                    !type->has_classical_sign()) {
                    if (type->name() == "true") {
                        stm_->res_type_ = ExternalType::true_;
                    } else if (type->name() == "false") {
                        stm_->res_type_ = ExternalType::false_;
                    } else if (type->name() == "free") {
                        stm_->res_type_ = ExternalType::free;
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            } else {
                stm_->res_type_ = ExternalType::free;
            }
            return true;
        }
        void do_vars(VariableSet &vars, VarSelectMode mode) const override {
            if (mode != VarSelectMode::provide) {
                if (stm_->type_) {
                    stm_->type_->vars(vars);
                }
                stm_->atom_->vars(vars);
            }
        }
        void do_print(std::ostream &out) const override {
            out << "#check(" << *stm_->atom_;
            if (stm_->type_) {
                out << "," << *stm_->type_;
            }
            out << ")";
        }
        [[nodiscard]] auto do_copy() const -> ULit override { return std::make_unique<LitExternalCheck>(*stm_); }

        StmExternal *stm_;
    };
    body_.emplace_back(std::make_unique<LitExternalCheck>(*this));
}

void StmExternal::do_print_head(std::ostream &out) const { out << "#external " << *atom_; }

void StmExternal::do_print(std::ostream &out) const {
    out << "max: ";
    print_head(out);
    if (!indices_.empty()) {
        out << "[" << Util::p_range(indices_, ",") << "]";
    }
    out << " :- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
    if (type_) {
        out << " [" << *type_ << "]";
    }
}

auto StmExternal::do_body() const -> ULitVec const & { return body_; }

auto StmExternal::do_important() const -> VariableSet {
    VariableSet important;
    atom_->vars(important);
    if (type_) {
        type_->vars(important);
    }
    return important;
}

void StmExternal::do_init(size_t gen) { base_->update(gen); }

auto StmExternal::do_report(InstantiationContext &ctx) -> bool {
    ctx.out().external(res_atom_, res_type_);
    return true;
}

void StmExternal::do_propagate([[maybe_unused]] SymbolStore &store, Queue &queue) {
    // Consider adding the propagation to the instantiator...
    if (base_->has_update()) {
        for (auto const &idx : indices_) {
            queue.propagate(idx);
        }
    }
}

// definition of StmWeakConstraint

void StmWeakConstraint::init_() {
    UTermVec terms;
    terms.reserve(terms_.size() + (prio_ ? 2 : 1));
    terms.emplace_back(weight_->copy());
    if (prio_) {
        terms.emplace_back(prio_->copy());
    }
    for (auto const &term : terms_) {
        terms.emplace_back(term->copy());
    }
    body_.emplace_back(std::make_unique<LitFailCheck>(std::move(terms), 1, &syms_));
}

void StmWeakConstraint::do_print(std::ostream &out) const {
    out << "max: ";
    out << " :~ " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ". ";
    print_head(out);
}

auto StmWeakConstraint::do_body() const -> ULitVec const & { return body_; }

auto StmWeakConstraint::do_important() const -> VariableSet {
    VariableSet vars;
    weight_->vars(vars);
    if (prio_) {
        prio_->vars(vars);
    }
    for (auto const &term : terms_) {
        term->vars(vars);
    }
    return vars;
}

void StmWeakConstraint::do_print_head(std::ostream &out) const {
    out << "[" << *weight_;
    if (prio_) {
        out << "@" << *prio_;
    }
    for (auto const &term : terms_) {
        out << "," << *term;
    }
    out << "]";
}

void StmWeakConstraint::do_init([[maybe_unused]] size_t gen) {}

auto StmWeakConstraint::do_report(InstantiationContext &ctx) -> bool {
    assert(syms_.size() > (prio_ ? 1 : 0));
    auto &out = ctx.out().body();
    for (auto const &lit : body_) {
        std::ignore = lit->output(ctx, out);
    }
    auto it = syms_.begin();
    auto weight = *it++;
    auto prio = std::optional<Symbol>{};
    if (prio_) {
        prio.emplace(*it++);
    }
    auto terms = std::span(it, syms_.end());
    ctx.out().weak_constraint(weight.num(), prio, terms);
    return true;
}

void StmWeakConstraint::do_propagate([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Queue &queue) {}

// definitition of StmHeuristic

void StmHeuristic::init_() {
    class LitHeuristicCheck : public LitCheck {
      public:
        LitHeuristicCheck(StmHeuristic &stm) : stm_{&stm} {}

      private:
        [[nodiscard]] auto do_check(InstantiationContext &ctx) -> bool override {
            if (auto weight = stm_->weight_->eval(ctx.store(), ctx.ass());
                weight && weight->type() == SymbolType::number) {
                stm_->res_weight_ = *weight;
            } else {
                return false;
            }
            if (stm_->prio_) {
                if (auto prio = stm_->prio_->eval(ctx.store(), ctx.ass()); prio && prio->type() == SymbolType::number) {
                    stm_->res_prio_ = *prio;
                } else {
                    return false;
                }
            }
            if (auto type = stm_->type_->eval(ctx.store(), ctx.ass());
                type && type->type() == SymbolType::function && type->args().empty() && !type->has_classical_sign()) {
                if (type->name() == "factor") {
                    stm_->res_type_ = HeuristicType::factor;
                } else if (type->name() == "false") {
                    stm_->res_type_ = HeuristicType::false_;
                } else if (type->name() == "init") {
                    stm_->res_type_ = HeuristicType::init;
                } else if (type->name() == "level") {
                    stm_->res_type_ = HeuristicType::level;
                } else if (type->name() == "sign") {
                    stm_->res_type_ = HeuristicType::sign;
                } else if (type->name() == "true") {
                    stm_->res_type_ = HeuristicType::true_;
                } else {
                    return false;
                }
            } else {
                return false;
            }
            return true;
        }
        void do_vars(VariableSet &vars, VarSelectMode mode) const override {
            if (mode != VarSelectMode::provide) {
                stm_->weight_->vars(vars);
                if (stm_->prio_) {
                    stm_->prio_->vars(vars);
                }
                stm_->type_->vars(vars);
            }
        }
        void do_print(std::ostream &out) const override {
            out << "#check(" << *stm_->weight_;
            if (stm_->prio_) {
                out << "," << *stm_->prio_;
            }
            out << "," << *stm_->type_ << ")";
        }
        [[nodiscard]] auto do_copy() const -> ULit override { return std::make_unique<LitHeuristicCheck>(*stm_); }

        StmHeuristic *stm_;
    };

    class LitAtom : public Lit {
      public:
        LitAtom(StmHeuristic &stm) : stm_{&stm} {}

      private:
        void do_print(std::ostream &out) const override { out << *stm_->atom_; }

        [[nodiscard]] auto do_output([[maybe_unused]] InstantiationContext &ctx,
                                     [[maybe_unused]] OutputLit &out) const -> bool override {
            return false;
        }

        [[nodiscard]] auto do_copy() const -> ULit override { return std::make_unique<LitAtom>(*stm_); }

        [[nodiscard]] auto do_domain() const -> bool override { return true; }

        [[nodiscard]] auto do_single_pass() const -> bool override { return true; }

        void do_vars(VariableSet &vars, VarSelectMode mode) const override {
            if (mode != VarSelectMode::depend) {
                stm_->atom_->vars(vars);
            }
        }

        [[nodiscard]] auto
        do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                   std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> override {
            return {make_atom_matcher(mbr, bound, *stm_->base_, *stm_->atom_, type, stm_->offset_), std::nullopt};
        }

        [[nodiscard]] auto do_score(std::vector<bool> const &bound) const -> double override {
            return stm_->atom_->score(static_cast<double>(stm_->base_->size()), bound);
        }

        [[nodiscard]] auto do_hash() const -> size_t override { return std::hash<LitAtom const *>{}(this); }

        [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override { return this == &other; }

        [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override {
            return this <=> &other;
        }

        StmHeuristic *stm_;
    };
    body_.emplace_back(std::make_unique<LitAtom>(*this));
    body_.emplace_back(std::make_unique<LitHeuristicCheck>(*this));
}

void StmHeuristic::do_print(std::ostream &out) const {
    out << "max: ";
    print_head(out);
    out << " :- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

auto StmHeuristic::do_body() const -> ULitVec const & { return body_; }

auto StmHeuristic::do_important() const -> VariableSet {
    VariableSet vars;
    atom_->vars(vars);
    weight_->vars(vars);
    if (prio_) {
        prio_->vars(vars);
    }
    type_->vars(vars);
    return vars;
}

void StmHeuristic::do_print_head(std::ostream &out) const {
    out << "#heuristic " << *atom_ << " [" << *weight_;
    if (prio_) {
        out << "@" << *prio_;
    }
    out << "," << *type_ << "]";
}

void StmHeuristic::do_init([[maybe_unused]] size_t gen) {}

auto StmHeuristic::do_report(InstantiationContext &ctx) -> bool {
    auto &out = ctx.out().body();
    for (auto const &lit : body_) {
        std::ignore = lit->output(ctx, out);
    }
    auto atom = base_->nth(offset_).key();
    ctx.out().heuristic(atom, res_weight_.num(), prio_ ? &res_prio_.num() : nullptr, res_type_);
    return true;
}

void StmHeuristic::do_propagate([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Queue &queue) {}

// definitition of StmEdge

void StmEdge::init_() {
    class LitEdgeCheck : public LitCheck {
      public:
        LitEdgeCheck(StmEdge &stm) : stm_{&stm} {}

      private:
        [[nodiscard]] auto do_check(InstantiationContext &ctx) -> bool override {
            if (auto src = stm_->src_->eval(ctx.store(), ctx.ass())) {
                stm_->res_src_ = *src;
            } else {
                return false;
            }
            if (auto dst = stm_->dst_->eval(ctx.store(), ctx.ass())) {
                stm_->res_dst_ = *dst;
            } else {
                return false;
            }
            return true;
        }
        void do_vars(VariableSet &vars, VarSelectMode mode) const override {
            if (mode != VarSelectMode::provide) {
                stm_->src_->vars(vars);
                stm_->dst_->vars(vars);
            }
        }
        void do_print(std::ostream &out) const override {
            out << "#check(" << *stm_->src_;
            out << "," << *stm_->dst_ << ")";
        }
        [[nodiscard]] auto do_copy() const -> ULit override { return std::make_unique<LitEdgeCheck>(*stm_); }

        StmEdge *stm_;
    };
    body_.emplace_back(std::make_unique<LitEdgeCheck>(*this));
}

void StmEdge::do_print(std::ostream &out) const {
    out << "max: ";
    print_head(out);
    out << " :- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

auto StmEdge::do_body() const -> ULitVec const & { return body_; }

auto StmEdge::do_important() const -> VariableSet {
    VariableSet vars;
    src_->vars(vars);
    dst_->vars(vars);
    return vars;
}

void StmEdge::do_print_head(std::ostream &out) const { out << "#edge (" << *src_ << "," << *dst_ << ")"; }

void StmEdge::do_init([[maybe_unused]] size_t gen) {}

auto StmEdge::do_report(InstantiationContext &ctx) -> bool {
    auto &out = ctx.out().body();
    for (auto const &lit : body_) {
        std::ignore = lit->output(ctx, out);
    }
    ctx.out().edge(res_src_, res_dst_);
    return true;
}

void StmEdge::do_propagate([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Queue &queue) {}

// definitition of StmShow

void StmShow::init_() {
    class LitShowCheck : public LitCheck {
      public:
        LitShowCheck(StmShow &stm) : stm_{&stm} {}

      private:
        [[nodiscard]] auto do_check(InstantiationContext &ctx) -> bool override {
            if (auto term = stm_->term_->eval(ctx.store(), ctx.ass())) {
                stm_->res_term_ = *term;
            } else {
                return false;
            }
            return true;
        }
        void do_vars(VariableSet &vars, VarSelectMode mode) const override {
            if (mode != VarSelectMode::provide) {
                stm_->term_->vars(vars);
            }
        }
        void do_print(std::ostream &out) const override { out << "#check(" << *stm_->term_ << ")"; }
        [[nodiscard]] auto do_copy() const -> ULit override { return std::make_unique<LitShowCheck>(*stm_); }

        StmShow *stm_;
    };
    body_.emplace_back(std::make_unique<LitShowCheck>(*this));
}

void StmShow::do_print(std::ostream &out) const {
    out << "max: ";
    print_head(out);
    out << " :- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

auto StmShow::do_body() const -> ULitVec const & { return body_; }

auto StmShow::do_important() const -> VariableSet {
    VariableSet vars;
    term_->vars(vars);
    return vars;
}

void StmShow::do_print_head(std::ostream &out) const { out << "#show " << *term_; }

void StmShow::do_init([[maybe_unused]] size_t gen) {}

auto StmShow::do_report(InstantiationContext &ctx) -> bool {
    auto &out = ctx.out().body();
    for (auto const &lit : body_) {
        std::ignore = lit->output(ctx, out);
    }
    ctx.out().show_term(res_term_);
    return true;
}

void StmShow::do_propagate([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Queue &queue) {}

// definitition of StmProject

void StmProject::init_() {
    class LitProject : public Lit {
      public:
        LitProject(StmProject &stm) : stm_{&stm} {}

      private:
        void do_print(std::ostream &out) const override { out << *stm_->atom_; }

        [[nodiscard]] auto do_output([[maybe_unused]] InstantiationContext &ctx,
                                     [[maybe_unused]] OutputLit &out) const -> bool override {
            return false;
        }

        [[nodiscard]] auto do_copy() const -> ULit override { return std::make_unique<LitProject>(*stm_); }

        [[nodiscard]] auto do_domain() const -> bool override { return true; }

        [[nodiscard]] auto do_single_pass() const -> bool override { return true; }

        void do_vars(VariableSet &vars, VarSelectMode mode) const override {
            if (mode != VarSelectMode::depend) {
                stm_->atom_->vars(vars);
            }
        }

        [[nodiscard]] auto
        do_matcher(std::pmr::monotonic_buffer_resource &mbr, MatcherType type,
                   std::vector<bool> const &bound) -> std::pair<UMatcher, std::optional<size_t>> override {
            return {make_atom_matcher(mbr, bound, *stm_->base_, *stm_->atom_, type, stm_->offset_), std::nullopt};
        }

        [[nodiscard]] auto do_score(std::vector<bool> const &bound) const -> double override {
            return stm_->atom_->score(static_cast<double>(stm_->base_->size()), bound);
        }

        [[nodiscard]] auto do_hash() const -> size_t override { return std::hash<LitProject const *>{}(this); }

        [[nodiscard]] auto do_equal_to(Lit const &other) const -> bool override { return this == &other; }

        [[nodiscard]] auto do_compare_to(Lit const &other) const -> std::weak_ordering override {
            return this <=> &other;
        }

        StmProject *stm_;
    };
    body_.emplace_back(std::make_unique<LitProject>(*this));
}

void StmProject::do_print(std::ostream &out) const {
    out << "max: ";
    print_head(out);
    out << " :- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

auto StmProject::do_body() const -> ULitVec const & { return body_; }

auto StmProject::do_important() const -> VariableSet {
    VariableSet vars;
    atom_->vars(vars);
    return vars;
}

void StmProject::do_print_head(std::ostream &out) const { out << "#project " << *atom_; }

void StmProject::do_init([[maybe_unused]] size_t gen) {}

auto StmProject::do_report(InstantiationContext &ctx) -> bool {
    ctx.out().project(base_->nth(offset_).key());
    return true;
}

void StmProject::do_propagate([[maybe_unused]] SymbolStore &store, [[maybe_unused]] Queue &queue) {}

} // namespace Clingo::Ground
