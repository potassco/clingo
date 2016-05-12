// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Roland Kaminski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// }}}

#ifndef _GRINGO_GRAPH_HH
#define _GRINGO_GRAPH_HH

#include <vector>
#include <forward_list>

namespace Gringo {

// {{{ declaration of Graph<T>

template <class T>
class Graph {
public:
    struct Node;
    typedef std::vector<Node*> NodeVec;
    typedef std::vector<NodeVec> SCCVec;
    struct Node {
        friend class Graph;
        template <class... U>
        Node(bool phase, U &&...x);
        Node(Node const &) = delete;
        void insertEdge(Node &n);
        typename NodeVec::const_iterator begin() const;
        typename NodeVec::const_iterator end() const;

        T  data;
    private:
        NodeVec edges_;
        unsigned visited_;
        typename NodeVec::iterator finished_;
    };

    Graph();
    Graph(Graph &&g);
    Graph(Graph const &) = delete;
    SCCVec tarjan();
    template <class... U>
    Node &insertNode(U &&...x);
private:
    typedef std::forward_list<Node> NodeList;
    NodeList nodes_;
    bool phase_ = false;
};

// }}}

// {{{ definition of Graph<T>::Node

template <class T>
template <class... U>
Graph<T>::Node::Node(bool phase, U &&...data)
    : data(std::forward<U>(data)...)
    , visited_(!phase) { }

template <class T>
void Graph<T>::Node::insertEdge(Node &n) {
    edges_.emplace_back(&n);
}

template <class T>
typename Graph<T>::NodeVec::const_iterator Graph<T>::Node::begin() const {
    return edges_.begin();
}

template <class T>
typename Graph<T>::NodeVec::const_iterator Graph<T>::Node::end() const {
    return edges_.end();
}

// }}}
// {{{ definition of Graph<T>

template <class T>
Graph<T>::Graph() = default;

template <class T>
Graph<T>::Graph(Graph &&) = default;

template <class T>
template <class... U>
typename Graph<T>::Node &Graph<T>::insertNode(U &&... x) {
    nodes_.emplace_front(phase_, std::forward<U>(x)...);
    return nodes_.front();
}

template <class T>
typename Graph<T>::SCCVec Graph<T>::tarjan() {
    SCCVec sccs;
    NodeVec stack;
    NodeVec trail;
    for (auto &x : nodes_) {
        if (x.visited_ == !phase_) {
            unsigned index = 1;
            auto push = [&stack, &trail, &index](Node &x) {
                x.visited_  = ++index;
                x.finished_ = x.edges_.begin();
                stack.emplace_back(&x);
                trail.emplace_back(&x);
            };
            push(x);
            while (!stack.empty()) {
                auto &y = stack.back();
                auto end = y->edges_.end();
                for (; y->finished_ != end && (*y->finished_)->visited_ != !phase_; ++y->finished_) { }
                if (y->finished_ != end) { push(**y->finished_++); }
                else {
                    stack.pop_back();
                    bool root = true;
                    for (auto &z : y->edges_) {
                        if (z->visited_ != phase_ && z->visited_ < y->visited_) {
                            root = false;
                            y->visited_ = z->visited_;
                        }
                    }
                    if (root) {
                        sccs.emplace_back();
                        do {
                            sccs.back().emplace_back(trail.back());
                            trail.back()->visited_ = phase_;
                            trail.pop_back();
                        }
                        while (sccs.back().back() != y);
                    }
                }
            }
        }
    }
    phase_ = !phase_;
    return std::move(sccs);
}

// }}}

} // namespace Gringo

#endif // _GRINGO_GRAPH_HH
