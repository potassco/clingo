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

enum class MatcherType { new_atoms, old_atoms, all_atoms };

class LitDep {
  public:
    LitDep(ULitVec const &lits) : lits_{lits} {}
    void build() {
        // handling X=Y in both directions would be super annoying.
        // it is probably easier to add it in both direction during building
        // however, it should be pruned here
        // we can then use a set of literals and mark them as done:
        // done: {literal}
        // for assignment literals:
        //   eq: a and b if a.op == b.op && (a.lhs == b.lhs && a.rhs == b.rhs || a.lhs == b.rhs && a.rhs == b.lhs)
        //   hash: compute the hash as (op, lhs, rhs) ^ (op, rhs, lhs)
        //   we cannot prune recursive equivalent literals
        auto i = size_t{0};
        auto vars = VariableSet{};
        for (auto const &lit : lits_) {
            vars.clear();
            lit->vars(vars, VarSelectMode::depend);
            lit_map_.emplace_back(vars.size(), std::vector<size_t>{vars.begin(), vars.end()});
            vars.clear();
            lit->vars(vars, VarSelectMode::provide);
            for (auto var : vars) {
                if (var_map_.size() <= var) {
                    var_map_.resize(var);
                }
                var_map_[var].emplace_back(i);
            }
            ++i;
        }
    }
    void order() {
        auto queue = std::deque<size_t>{};
        auto i = size_t{0};
        for (auto &[num, prv] : lit_map_) {
            num = prv.size();
            if (num == 0) {
                queue.emplace_back(i);
            }
            ++i;
        }
        auto provided = std::vector<bool>(var_map_.size(), false);
        while (!queue.empty()) {
            // TODO: sort queue by priority
            i = queue.front();
            // TODO: add matcher to instantiator
            //       if an equivalent non-recursive literal has not yet been added
            queue.pop_front();
            for (auto var : lit_map_[i].second) {
                if (!provided[var]) {
                    assert(var < var_map_.size());
                    for (auto j : var_map_[var]) {
                        if (--lit_map_[j].first; lit_map_[j].first == 0) {
                            queue.emplace_back(j);
                        }
                    }
                    provided[var] = true;
                }
            }
        }
    }

  private:
    ULitVec const &lits_;
    //! A map from literal indices to provided variables.
    std::vector<std::pair<size_t, std::vector<size_t>>> lit_map_;
    //! A map from variables to provided literals.
    std::vector<std::vector<size_t>> var_map_;
};

void linearize_(InstantiatorVec &insts, bool domain, InstanceCallback &cb, VariableSet important, ULitVec const &lits,
                VariableSet const &bound_initially = {}) {
    static_cast<void>(domain);
    static_cast<void>(cb);
    static_cast<void>(important);
    static_cast<void>(bound_initially);
    auto rec = std::vector<size_t>{};
    auto todos = std::vector<std::vector<std::pair<MatcherType, Lit *>>>{1};
    auto i = size_t{0};
    for (auto const &lit : lits) {
        todos.back().emplace_back(MatcherType::all_atoms, lit.get());
        if (lit->recursive()) {
            rec.emplace_back(i);
        }
        ++i;
    }
    todos.reserve(rec.size());
    for (auto i : rec) {
        todos.back()[i].first = MatcherType::new_atoms;
        if (i != rec.back()) {
            todos.emplace_back(todos.back());
            todos.back()[i].first = MatcherType::old_atoms;
        }
    }
    auto max_var = size_t{0};
    insts.reserve(insts.size() + todos.size()); // Note: preserve references
    auto done = std::vector<bool>{};
    done.reserve(lits.size());
    for (auto const &todo : todos) {
        // TODO: why do we need initially bound variables???
        auto dep = LitDep{lits};
        dep.build();
        dep.order();
        insts.emplace_back(cb, max_var, lits.size());
        static_cast<void>(todo);
    }
    /*
    if (!domain) {
        for (auto const &lit : lits) {
            lit->vars(important, true);
        }
    }
    */
}

/*

InstVec _linearize(Logger &log, Context &context, bool positive, SolutionCallback &cb, Term::VarSet &&important, ULitVec
const &lits, Term::VarSet const &boundInitially = Term::VarSet()) { InstVec insts; std::vector<unsigned> rec;
    std::vector<std::vector<std::pair<BinderType,Literal*>>> todo{1};
    unsigned i{0};
    for (auto const &x : lits) {
        todo.back().emplace_back(BinderType::ALL, x.get());
        if (x->isRecursive() && x->occurrence() != nullptr && !x->occurrence()->isNegative()) {
            rec.emplace_back(i);
        }
        ++i;
    }
    todo.reserve(std::max(std::vector<unsigned>::size_type(1), rec.size()));
    insts.reserve(todo.capacity()); // Note: preserve references
    for (auto i : rec) {
        todo.back()[i].first = BinderType::NEW;
        if (i != rec.back()) {
            todo.emplace_back(todo.back());
            todo.back()[i].first = BinderType::OLD;
        }
    }
    if (!positive) {
        for (auto const &lit : lits) {
            if (!lit->auxiliary()) {
                lit->collectImportant(important);
            }
        }
    }
    for (auto &x : todo) {
        Term::VarSet bound = boundInitially;
        insts.emplace_back(cb);
        SC s;
        std::unordered_map<String, SC::VarNode*> varMap;
        std::vector<std::pair<String, std::vector<unsigned>>> boundBy;
        for (auto &lit : x) {
            auto &entNode(s.insertEnt(lit.first, *lit.second));
            VarTermBoundVec vars;
            lit.second->collect(vars);
            for (auto &occ : vars) {
                if (bound.find(occ.first->name) == bound.end()) {
                    auto &varNode(varMap[occ.first->name]);
                    if (varNode == nullptr)   {
                        varNode = &s.insertVar(numeric_cast<unsigned>(boundBy.size()));
                        boundBy.emplace_back(occ.first->name, std::vector<unsigned>{});
                    }
                    if (occ.second) {
                        s.insertEdge(entNode, *varNode);
                    }
                    else {
                        s.insertEdge(*varNode, entNode);
                    }
                    entNode.data.vars.emplace_back(varNode->data);
                }
            }
        }
        Instantiator::DependVec depend;
        unsigned uid = 0;
        auto pred = [&bound, &log](Ent const &x, Ent const &y) -> bool {
            double sx(x.lit.score(bound, log));
            double sy(y.lit.score(bound, log));
            //std::cerr << "  " << x.lit << "@" << sx << " < " << y.lit << "@" << sy << " with " << bound.size() <<
std::endl; if (sx < 0 || sy < 0) { return sx < sy;
            }
            if ((x.type == BinderType::NEW || y.type == BinderType::NEW) && x.type != y.type) {
                return x.type < y.type;
            }
            return sx < sy;
        };

        SC::EntVec open;
        s.init(open);
        while (!open.empty()) {
            for (auto it = open.begin(), end = open.end() - 1; it != end; ++it) {
                if (pred((*it)->data, open.back()->data)) {
                    std::swap(open.back(), *it);
                }
            }
            auto *y = open.back();
            for (auto &var : y->data.vars) {
                auto &bb(boundBy[var]);
                if (bound.find(bb.first) == bound.end()) {
                    bb.second.emplace_back(uid);
                    if ((depend.empty() || depend.back() != uid) && important.find(bb.first) != important.end()) {
                        depend.emplace_back(uid);
                    }
                }
                else {
                    y->data.depends.insert(y->data.depends.end(), bb.second.begin(), bb.second.end());
                }
            }
            auto index(y->data.lit.index(context, y->data.type, bound));
            if (auto *update = index->getUpdater()) {
                if (BodyOcc *occ = y->data.lit.occurrence()) {
                    for (HeadOccurrence &x : occ->definedBy()) {
                        x.defines(*update, y->data.type == BinderType::NEW ? &insts.back() : nullptr);
                    }
                }
            }
            std::sort(y->data.depends.begin(), y->data.depends.end());
            y->data.depends.erase(std::unique(y->data.depends.begin(), y->data.depends.end()), y->data.depends.end());
            insts.back().add(std::move(index), std::move(y->data.depends));
            uid++;
            open.pop_back();
            s.propagate(y, open);
        }
        insts.back().finalize(std::move(depend));
    }
    return insts;
}
*/

} // namespace

void StmRule::linearize(InstantiatorVec &insts, bool domain) {
    InstanceCallback *cb = nullptr;
    VariableSet important;
    head_->vars(important);
    linearize_(insts, domain, *cb, std::move(important), body_);
}

} // namespace Gringo::Ground
