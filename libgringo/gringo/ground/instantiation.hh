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

#ifndef _GRINGO_GROUND_INSTANTIATION_HH
#define _GRINGO_GROUND_INSTANTIATION_HH

#include <gringo/base.hh>
#include <queue>

// {{{ forward declarations

namespace Gringo { 
struct Domain;
struct IndexUpdater;
namespace Output { struct OutputBase; } 
}

// }}}

namespace Gringo { namespace Ground {

// {{{ declaration of Queue

struct Instantiator;
struct Queue {
    void process(Output::OutputBase &out);
    void enqueue(Instantiator &inst);
    void enqueue(Domain &inst);
    ~Queue();
    
    using QueueDec  = std::vector<std::reference_wrapper<Instantiator>>;
    using DomainVec = std::vector<std::reference_wrapper<Domain>>;
    QueueDec  current;
    std::array<QueueDec,2>  queues;
    DomainVec domains;
};

// }}}
// {{{ declaration of Binder

struct Binder : Printable {
    virtual IndexUpdater *getUpdater() = 0;
    virtual void match() = 0;
    virtual bool next() = 0;
    virtual ~Binder() { }
};
using UIdx = std::unique_ptr<Binder>;

// }}}
// {{{ declaration if SolutionCallback

struct SolutionCallback {
    virtual void report(Output::OutputBase &out) = 0;
    virtual void propagate(Queue &queue) = 0;
    virtual void printHead(std::ostream &out) const = 0;
    virtual unsigned priority() const { return 0; }
    virtual ~SolutionCallback() { }
};

// }}}
// {{{ declaration of SolutionBinder

struct SolutionBinder : public Binder {
    virtual IndexUpdater *getUpdater();
    virtual void match();
    virtual bool next();
    virtual void print(std::ostream &out) const;
    virtual ~SolutionBinder();
};

// }}}
// {{{ declaration of BackjumpBinder

struct BackjumpBinder {
    typedef std::vector<unsigned> DependVec;
    typedef std::vector<Term::SVal> SValVec;

    BackjumpBinder(UIdx &&index, DependVec &&depends);
    BackjumpBinder(BackjumpBinder &&x) noexcept;
    void match();
    bool next();
    bool first();
    void print(std::ostream &out) const;
    ~BackjumpBinder();

    UIdx index;
    DependVec depends;
    bool backjumpable = false;
};
inline std::ostream &operator<<(std::ostream &out, BackjumpBinder &x) { x.print(out); return out; }

// }}}
// {{{ declaration of Instantiator

struct Instantiator {
    using DependVec = BackjumpBinder::DependVec;
    using SValVec = BackjumpBinder::SValVec;
    
    Instantiator(SolutionCallback &callback);
    Instantiator(Instantiator &&x) noexcept;
    Instantiator &operator=(Instantiator &&x) noexcept;
    void add(UIdx &&index, DependVec &&depends);
    void finalize(DependVec &&depends);
    void enqueue(Queue &queue);
    void instantiate(Output::OutputBase &out);
    void print(std::ostream &out) const;
    unsigned priority() const;
    ~Instantiator();

    SolutionCallback &callback;
    std::vector<BackjumpBinder> binders;
    bool enqueued = false;
};
using InstVec = std::vector<Instantiator>;
inline std::ostream &operator<<(std::ostream &out, Instantiator &x) { x.print(out); return out; }

// }}}

} } // namespace Ground Gringo

#endif // _GRINGO_GROUND_INSTANTIATION_HH
