// This example is the C++ implementation of ./examples/clingo/heuristic/. To
// run this example, use the instances provided with the other example.

#include <clingo.hh>
#include <unordered_map>

struct State {
    std::unordered_map<Clingo::Symbol, int> degree;
};

class ColoringHeuristic : public Clingo::Heuristic {
public:
    void init(Clingo::PropagateInit &init) override {
        states_.resize(init.number_of_threads());
        for (auto &&a : init.symbolic_atoms()) {
            if (a.match("edge", 2)) {
                auto u = a.symbol().arguments()[0];
                auto v = a.symbol().arguments()[1];
                graph_[u].emplace_back(v);
                graph_[v].emplace_back(u);
                for (auto &&state : states_) {
                    ++state.degree[u];
                    ++state.degree[v];
                }
            }
            else if (a.match("assign", 2)) {
                auto u = a.symbol().arguments()[0];
                auto l = init.solver_literal(a.literal());
                init.add_watch(l);
                assign_[l].emplace_back(u);
            }
        }
    }

    void propagate(Clingo::PropagateControl &ctl, Clingo::LiteralSpan changes) override {
        auto &&state = states_[ctl.thread_id()];
        for (auto &&l : changes) {
            for (auto &&u : assign_[l]) {
                for (auto &&v : graph_[u]) {
                    state.degree[v] -= 1;
                }
            }
        }
    }

    void undo(Clingo::PropagateControl const &ctl, Clingo::LiteralSpan changes) override {
        auto &&state = states_[ctl.thread_id()];
        for (auto &&l : changes) {
            for (auto &&u : assign_[l]) {
                for (auto &&v : graph_[u]) {
                    state.degree[v] += 1;
                }
            }
        }
    }

    Clingo::literal_t decide(Clingo::id_t thread_id, Clingo::Assignment const &assign, Clingo::literal_t) override {
        // in practice this implementation is unusable because it is too inefficient
        // a heap should be used to extract the vertices with maximum degree
        auto &&state = states_[thread_id];
        clingo_literal_t decision = 0;
        int degree = 0;
        for (auto &&group : assign_) {
            for (auto &&u : group.second) {
                if (assign.truth_value(group.first) == Clingo::TruthValue::Free && degree < state.degree[u]) {
                    decision = group.first;
                    degree   = state.degree[u];
                }
            }
        }
        return decision;
    }

private:
    std::unordered_map<Clingo::Symbol, Clingo::SymbolVector> graph_;
    std::unordered_map<Clingo::literal_t, Clingo::SymbolVector> assign_;
    std::vector<State> states_;
};

class GraphColorizer : Clingo::Application {
public:
    static int run(int argc, char const **argv) {
        GraphColorizer gc;
        return Clingo::clingo_main(gc, {argv+1, argv+argc});
    }
private:
    char const *program_name() const noexcept override {
        return "colorizer";
    };
    char const *version() const noexcept override { return "1.0"; }
    void main(Clingo::Control &ctl, Clingo::StringSpan files) override {
        for (auto &&file : files) {
            ctl.load(file);
        }
        if (files.empty()) { ctl.load("-"); }
        ctl.add("base", {},
            "1 { assign(U,C) : color(C) } 1 :- vertex(U).\n"
            " :- edge(U,V), assign(U,C), assign(V,C).\n");
        ColoringHeuristic heu;
        ctl.register_propagator(heu);
        ctl.ground({{"base", {}}});
        ctl.solve(Clingo::LiteralSpan{}, nullptr, false, false).get();
    }
};

int main(int argc, char const **argv) {
    return GraphColorizer::run(argc, argv);
}

