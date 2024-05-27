#include <gringo/ground/statement.hh>

#include <gringo/util/print.hh>
#include <gringo/util/unordered_map.hh>

// TODO:
#include <iostream>
#include <sstream>

namespace Gringo::Ground {

void Linearizer::start(Queue &queue, bool domain) {
    iqueue_ = &queue;
    domain_ = domain;
}

void Linearizer::prepare(InstanceCallback &cb, ULitVec const &body, VariableSet important) {
    rec_.clear();
    todos_.clear();
    todos_.emplace_back();
    auto i = size_t{0};
    // gather indices of recursize literals and extend important variables
    for (auto const &lit : body) {
        todos_.back().emplace_back(MatcherType::all_atoms);
        if (lit->recursive()) {
            rec_.emplace_back(i);
        }
        if (!lit->domain() || (!domain_ && lit->recursive())) {
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
    // proceess the queue
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
        auto [matcher, index] = lits[i]->matcher(todo[i], bound);
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

void StmRule::print_head(std::ostream &out) const {
    if (head_) {
        out << *head_;
        if (!indices_.empty()) {
            out << "[" << Util::p_range(indices_, ",") << "]";
        }
    }
}

void StmRule::print(std::ostream &out) const {
    out << priority() << ": ";
    print_head(out);
    out << " :- " << Util::p_range(body_, ", ", [](std::ostream &out, auto const &lit) { out << *lit; }) << ".";
}

auto StmRule::body() const -> ULitVec const & { return body_; }

auto StmRule::important() const -> VariableSet {
    VariableSet important;
    if (head_) {
        head_->vars(important);
    }
    return important;
}

void StmRule::init(size_t gen) {
    if (base_ != nullptr) {
        base_->update(gen);
    }
}

void StmRule::report(SymbolStore &store, Assignment const &ass) {
    std::ostream &out = std::cerr;
    bool fact = true;
    std::ostringstream tmp_bd;
    bool comma = false;
    for (auto const &lit : body_) {
        std::ostringstream tmp_lit;
        if (lit->output(store, ass, tmp_lit)) {
            fact = false;
            if (comma) {
                tmp_bd << "; ";
            } else {
                comma = true;
            }
            tmp_bd << tmp_lit.view();
        }
    }
    if (head_ != nullptr) {
        if (auto atom = head_->eval(store, ass); atom) {
            base_->add(*atom, fact ? AtomState::fact : AtomState::derived);
            out << *atom;
            if (!fact) {
                out << " :- " << tmp_bd.view();
            }
            out << ".\n";
        }
    } else {
        out << " :- " << tmp_bd.view() << ".\n";
    }
}

void StmRule::propagate(Queue &queue) {
    // Consider adding the propagation to the instantiator...
    if (base_ != nullptr && base_->has_update()) {
        for (auto const &idx : indices_) {
            queue.propagate(idx);
        }
    }
}

} // namespace Gringo::Ground
