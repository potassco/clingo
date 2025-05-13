#pragma once

#include <cstddef>
#include <vector>

namespace CppClingo::Util {

//! @addtogroup util_algorithm
//! @{

//! Graph class to compute strongly connected components.
class Graph {
  public:
    //! A vector of node ids.
    using IdVec = std::vector<size_t>;
    //! A vector of vector of nodes forming a strongly connected component.
    using SCCVec = std::vector<IdVec>;

    //! Compute the strongly connected components of the graph.
    //!
    //! The components are reported in topological order.
    template <class Callback> void tarjan(Callback cb);
    //! Ensure that the graph holds at least n nodes.
    void ensure_size(size_t n);
    //! Add an edge to the graph.
    //!
    //! Nodes should be labeled consecutively.
    void add_edge(size_t u, size_t v);
    //! Check if the given vertex has a loop.
    //!
    //! @param u the vertex to check
    //! @return whether the vertex has a loop
    [[nodiscard]] auto has_loop(size_t u) const -> bool;
    //! Clear the graph.
    void clear() {
        phase_ = 0;
        nodes_.clear();
    }

  private:
    //! Node class to capture edges and state information.
    struct Node {
        Node(size_t visited) : visited_(visited) {}
        //! The outgoing edges of the node.
        IdVec out;
        //! An iterator pointing to the last element not yet processed.
        IdVec::iterator finished_;
        //! A counter when this node has been visited.
        size_t visited_;
    };

    //! Return the value assigned previously to phase_.
    [[nodiscard]] auto prev_phase_() const -> size_t;

    //! The vector of nodes.
    std::vector<Node> nodes_;
    //! The current phase to identify not yet visited nodes.
    size_t phase_ = 0;
};

inline auto Graph::prev_phase_() const -> size_t {
    return phase_ == 0 ? 1 : 0;
}

inline void Graph::ensure_size(size_t n) {
    if (nodes_.size() < n) {
        while (nodes_.size() < n) {
            nodes_.emplace_back(prev_phase_());
        }
    }
}

inline void Graph::add_edge(size_t u, size_t v) {
    ensure_size(std::max(u, v) + 1);
    nodes_[u].out.emplace_back(v);
}

inline auto Graph::has_loop(size_t u) const -> bool {
    return std::ranges::find(nodes_[u].out, u) != nodes_[u].out.end();
}

template <class Callback> inline void Graph::tarjan(Callback cb) {
    IdVec scc;
    IdVec stack;
    IdVec trail;
    for (size_t id_x = 0, n = nodes_.size(); id_x != n; ++id_x) {
        auto &x = nodes_[id_x];
        if (x.visited_ == prev_phase_()) {
            unsigned index = 1;
            auto push = [&stack, &trail, &index, this](size_t id_x) {
                auto &x = nodes_[id_x];
                x.visited_ = ++index;
                x.finished_ = x.out.begin();
                stack.emplace_back(id_x);
                trail.emplace_back(id_x);
            };
            push(id_x);
            while (!stack.empty()) {
                auto id_y = stack.back();
                auto &y = nodes_[id_y];
                auto end = y.out.end();
                for (; y.finished_ != end && nodes_[*y.finished_].visited_ != prev_phase_(); ++y.finished_) {
                }
                if (y.finished_ != end) {
                    push(*y.finished_++);
                } else {
                    stack.pop_back();
                    bool root = true;
                    for (auto id_z : y.out) {
                        auto &z = nodes_[id_z];
                        if (z.visited_ != phase_ && z.visited_ < y.visited_) {
                            root = false;
                            y.visited_ = z.visited_;
                        }
                    }
                    if (root) {
                        do {
                            scc.emplace_back(trail.back());
                            nodes_[trail.back()].visited_ = phase_;
                            trail.pop_back();
                        } while (scc.back() != id_y);
                        cb(scc);
                        scc.clear();
                    }
                }
            }
        }
    }
    phase_ = prev_phase_();
}

//! @}

} // namespace CppClingo::Util
