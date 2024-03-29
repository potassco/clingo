#include <gringo/ground/statement.hh>

#include <gringo/util/print.hh>
#include <gringo/util/unordered_map.hh>

namespace Gringo::Ground {

void StmRule::print(std::ostream &out) const {
    out << *head_;
    if (!indices_.empty()) {
        out << "[" << Util::p_range(indices_, ",") << "]";
    }
    out << " :- " << Util::p_range(body_, ", ", [](auto const &lit) -> decltype(auto) { return *lit; }) << ".";
}

// TODO: the code here has to be adapted to linearize a set of literals

namespace {

class LitDep {
  public:
    LitDep(ULitVec const &lits) : lits_{lits} {}
    //! Build the dependency graph among literals and variables.
    void build() {
        auto i = size_t{0};
        auto vars = VariableSet{};
        auto num_vars = size_t{0};
        lit_map_.reserve(lits_.size());
        for (auto const &lit : lits_) {
            lit->vars(vars, VarSelectMode::depend);
            auto depend = std::vector<size_t>(vars.begin(), vars.end());
            vars.clear();
            lit->vars(vars, VarSelectMode::provide);
            auto provide = std::vector<size_t>(vars.begin(), vars.end());
            vars.clear();
            num_vars = std::accumulate(depend.begin(), depend.end(), num_vars,
                                       [](auto a, auto b) { return std::max(a, b + 1); });
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
    //! Create matchers for literals ordering them heuristically.
    auto order(InstanceCallback &cb, std::vector<MatcherType> const &todo, VariableSet important) -> Instantiator {
        auto inst = Instantiator{cb, var_map_.size(), lits_.size()};
        size_t gen = 0;
        auto queue = std::vector<std::pair<size_t, size_t>>{};
        auto i = size_t{0};
        // initialize the queue
        for (auto &[cur, dep, prv] : lit_map_) {
            if ((cur = dep.size()) == 0) {
                queue.emplace_back(i, ++gen);
            }
            ++i;
        }
        auto provided = std::vector<size_t>(var_map_.size(), 0);
        auto make_depend = [&provided](auto const &vars) {
            auto dep = std::vector<size_t>{};
            for (auto var : vars) {
                dep.emplace_back(provided[var]);
            }
            return dep;
        };
        // proceess the queue
        auto done = Util::unordered_set<Lit const *>{};
        auto bound = std::vector<bool>(var_map_.size(), false);
        done.reserve(lits_.size());
        while (!queue.empty()) {
            // get minimum element in queue (breaking ties using insertion order)
            auto pred = [&, this](auto const &ei, auto const &ej) -> bool {
                auto si(lits_[ei.first]->score(bound));
                auto sj(lits_[ej.first]->score(bound));
                auto ti = todo[ei.first];
                auto tj = todo[ej.first];
                if ((ti == MatcherType::new_atoms || tj == MatcherType::new_atoms) && (si >= 0 && sj >= 0)) {
                    assert(ti != tj);
                    return ti < tj;
                }
                return std::tie(si, ei.second) < std::tie(sj, ej.second);
            };
            std::iter_swap(queue.rbegin(), std::min_element(queue.rbegin(), queue.rend(), pred));
            i = queue.back().first;
            queue.pop_back();
            // skip if an equivalent matcher has already been added (i.e., X=Y and Y=X)
            if (!done.emplace(lits_[i].get()).second && todo[i] == MatcherType::all_atoms) {
                continue;
            }
            inst.add(lits_[i]->matcher(todo[i]), make_depend(std::get<1>(lit_map_[i])));
            for (auto var : std::get<2>(lit_map_[i])) {
                if (bound[var]) {
                    continue;
                }
                assert(var < var_map_.size());
                for (auto j : var_map_[var]) {
                    if (--std::get<0>(lit_map_[j]) == 0) {
                        queue.emplace_back(j, ++gen);
                    }
                }
                provided[var] = i;
                bound[var] = true;
            }
        }
        inst.finalize(make_depend(important));
        return inst;
    }

  private:
    ULitVec const &lits_;
    //! A map from literal indices to provided variables.
    std::vector<std::tuple<size_t, std::vector<size_t>, std::vector<size_t>>> lit_map_;
    //! A map from variables to provided literals.
    std::vector<std::vector<size_t>> var_map_;
};

void linearize_(InstantiatorVec &insts, bool domain, InstanceCallback &cb, VariableSet important, ULitVec const &lits) {
    auto rec = std::vector<size_t>{};
    auto todos = std::vector<std::vector<MatcherType>>{1};
    auto i = size_t{0};
    // gather indices of recursize literals and extend important variables
    for (auto const &lit : lits) {
        todos.back().emplace_back(MatcherType::all_atoms);
        if (lit->recursive()) {
            rec.emplace_back(i);
        }
        if (!lit->domain(domain)) {
            lit->vars(important, VarSelectMode::all);
        }
        ++i;
    }
    // compute linearization
    todos.reserve(rec.size());
    for (auto i : rec) {
        todos.back()[i] = MatcherType::new_atoms;
        if (i != rec.back()) {
            todos.emplace_back(todos.back());
            todos.back()[i] = MatcherType::old_atoms;
        }
    }
    insts.reserve(insts.size() + todos.size());
    auto done = std::vector<bool>{};
    done.reserve(lits.size());
    auto dep = LitDep{lits};
    dep.build();
    for (auto const &todo : todos) {
        insts.emplace_back(dep.order(cb, todo, important));
    }
}

} // namespace

void StmRule::linearize(InstantiatorVec &insts, bool domain) {
    VariableSet important;
    head_->vars(important);
    linearize_(insts, domain, *this, std::move(important), body_);
}

void StmRule::init() {
    // Consider adding the propagation to the instantiator...
    printf("implement me: StmRule::init\n");
}

void StmRule::report(Assignment const &ass) {
    static_cast<void>(ass);
    printf("implement me: StmRule::report\n");
}

void StmRule::propagate(Queue &queue) {
    static_cast<void>(queue);
    // Consider adding the propagation to the instantiator...
    printf("implement me: StmRule::propagate\n");
}

} // namespace Gringo::Ground
