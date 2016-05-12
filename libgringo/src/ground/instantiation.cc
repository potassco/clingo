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

#include <gringo/ground/instantiation.hh>
#include <gringo/output/output.hh>

#define DEBUG_INSTANTIATION 0

namespace Gringo { namespace Ground {

// {{{ definition of SolutionBinder

IndexUpdater *SolutionBinder::getUpdater()   { return nullptr; }
void SolutionBinder::match()                 { }
bool SolutionBinder::next()                  { return false; }
void SolutionBinder::print(std::ostream &out) const { out << "#end"; }
SolutionBinder::~SolutionBinder()             { }

// }}}
// {{{ definition of BackjumpBinder

BackjumpBinder::BackjumpBinder(UIdx &&index, DependVec &&depends)
    : index(std::move(index))
    , depends(std::move(depends)) { }
BackjumpBinder::BackjumpBinder(BackjumpBinder &&) noexcept = default;
void BackjumpBinder::match() { index->match(); }
bool BackjumpBinder::next()  { return index->next(); }
bool BackjumpBinder::first() {
    index->match();
    return next();
}
void BackjumpBinder::print(std::ostream &out) const {
    out << *index;
    out << ":[";
    print_comma(out, depends, ",");
    out << "]";
}
BackjumpBinder::~BackjumpBinder() { }

// }}}
// {{{ definition of Instantiator

Instantiator::Instantiator(SolutionCallback &callback)
    : callback(&callback) { }
void Instantiator::add(UIdx &&index, DependVec &&depends) {
    binders.emplace_back(std::move(index), std::move(depends));
}
void Instantiator::finalize(DependVec &&depends) {
    binders.emplace_back(gringo_make_unique<SolutionBinder>(), std::move(depends));
}
void Instantiator::enqueue(Queue &queue) { queue.enqueue(*this); }
void Instantiator::instantiate(Output::OutputBase &out) {
#if DEBUG_INSTANTIATION > 0
    std::cerr << "  instantiate: " << *this << std::endl;
#endif
    auto ie = binders.rend(), it = ie - 1, ib = binders.rbegin();
    it->match();
    do {
#if DEBUG_INSTANTIATION > 1
        std::cerr << "    start at: " << *it << std::endl;
#endif
        it->backjumpable = true;
        if (it->next()) {
            for (--it; it->first(); --it) { it->backjumpable = true; }
#if DEBUG_INSTANTIATION > 1
            std::cerr << "    advanced to: " << *it << std::endl;
#endif
        }
        if (it == ib) { callback->report(out); }
        for (auto &x : it->depends) { binders[x].backjumpable = false; }
        for (++it; it != ie && it->backjumpable; ++it) { }
#if DEBUG_INSTANTIATION > 1
        std::cerr << "    backfumped to: ";
        if (it != ie) { it->print(std::cerr); }
        else          { std::cerr << "the head :)"; }
        std::cerr << std::endl;
#endif
    }
    while (it != ie);
}
void Instantiator::print(std::ostream &out) const {
    using namespace std::placeholders;
    // Note: consider adding something to callback
    callback->printHead(out);
    out << " <~ ";
    print_comma(out, binders, " , ", std::bind(&BackjumpBinder::print, _2, _1));
    out << ".";
}
unsigned Instantiator::priority() const {
    return callback->priority();
}
Instantiator::~Instantiator() noexcept = default;

// }}}
// {{{ definition of Queue

void Queue::process(Output::OutputBase &out) {
    bool empty;
    do {
        empty = true;
        for (auto &queue : queues) {
            if (!queue.empty()) {
#if DEBUG_INSTANTIATION > 0
                std::cerr << "************start step" << std::endl;
#endif
                queue.swap(current);
                for (Instantiator &x : current) {
                    x.instantiate(out);
                    x.enqueued = false;
                }
                for (Instantiator &x : current) { x.callback->propagate(*this); }
                current.clear();
                // OPEN -> NEW, NEW -> OLD
                auto jt = std::remove_if(domains.begin(), domains.end(), [](Domain &x) -> bool {
                    x.nextGeneration();
                    return !x.dequeue();
                });
                domains.erase(jt, domains.end());
                empty = false;
                break;
            }
        }
    }
    while (!empty);
    // NEW -> OLD
    for (Domain &x : domains) {
        x.nextGeneration();
        if (x.dequeue()) { assert(false); }
    }
    domains.clear();
}
void Queue::enqueue(Instantiator &inst) {
    if (!inst.enqueued) {
        queues[inst.priority()].emplace_back(inst);
        inst.enqueued = true;
    }
}
void Queue::enqueue(Domain &x) {
    if (!x.isEnqueued()) {
        domains.emplace_back(x);
    }
    x.enqueue();
}
Queue::~Queue() { }

// }}}

} } // namespace Ground Gringo
