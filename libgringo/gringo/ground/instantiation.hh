// {{{ MIT License

// Copyright 2017 Roland Kaminski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// }}}

#ifndef _GRINGO_GROUND_INSTANTIATION_HH
#define _GRINGO_GROUND_INSTANTIATION_HH

#include <gringo/output/types.hh>

namespace Gringo { namespace Ground {

// {{{ declaration of Queue

struct Instantiator;
struct Queue {
    void process(Output::OutputBase &out, Logger &log);
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
    virtual void match(Logger &log) = 0;
    virtual bool next() = 0;
    virtual ~Binder() { }
};
using UIdx = std::unique_ptr<Binder>;

// }}}
// {{{ declaration if SolutionCallback

class SolutionCallback {
public:
    virtual void report(Output::OutputBase &out, Logger &log) = 0;
    virtual void propagate(Queue &queue) = 0;
    virtual void printHead(std::ostream &out) const = 0;
    virtual unsigned priority() const { return 0; }
    virtual ~SolutionCallback() noexcept = default;
};

// }}}
// {{{ declaration of SolutionBinder

struct SolutionBinder : public Binder {
    IndexUpdater *getUpdater() override;
    void match(Logger &log) override;
    bool next() override;
    void print(std::ostream &out) const override;
    virtual ~SolutionBinder();
};

// }}}
// {{{ declaration of BackjumpBinder

struct BackjumpBinder {
    typedef std::vector<unsigned> DependVec;

    BackjumpBinder(UIdx &&index, DependVec &&depends);
    BackjumpBinder(BackjumpBinder &&x) noexcept;
    void match(Logger &log);
    bool next();
    bool first(Logger &log);
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

    Instantiator(SolutionCallback &callback);
    Instantiator(Instantiator &&x) = default;
    Instantiator &operator=(Instantiator &&x) = default;
    void add(UIdx &&index, DependVec &&depends);
    void finalize(DependVec &&depends);
    void enqueue(Queue &queue);
    void instantiate(Output::OutputBase &out, Logger &log);
    void print(std::ostream &out) const;
    unsigned priority() const;
    ~Instantiator() noexcept;

    SolutionCallback *callback;
    std::vector<BackjumpBinder> binders;
    bool enqueued = false;
};
using InstVec = std::vector<Instantiator>;
inline std::ostream &operator<<(std::ostream &out, Instantiator &x) { x.print(out); return out; }

// }}}

} } // namespace Ground Gringo

#endif // _GRINGO_GROUND_INSTANTIATION_HH
