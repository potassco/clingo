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

#ifndef _GRINGO_INPUT_STATEMENT_HH
#define _GRINGO_INPUT_STATEMENT_HH

#include <gringo/terms.hh>
#include <gringo/input/types.hh>

namespace Gringo { namespace Input {

// {{{ declaration of Statement

struct Statement : Printable, Locatable {
    Statement(UHeadAggr &&head, UBodyAggrVec &&body);
    virtual UStmVec unpool(bool beforeRewrite);
    virtual void assignLevels(VarTermBoundVec &bound);
    virtual bool simplify(Projections &project, Logger &log);
    virtual void rewrite();
    virtual Symbol isEDB() const;
    virtual void print(std::ostream &out) const;
    virtual bool hasPool(bool beforeRewrite) const;
    virtual void check(Logger &log) const;
    virtual void replace(Defines &dx);
    virtual void toGround(ToGroundArg &x, Ground::UStmVec &stms) const;
    virtual void add(ULit &&lit);
    virtual void initTheory(TheoryDefs &def, Logger &log);
    virtual void getNeg(std::function<void (Sig)> f) const;
    virtual ~Statement();

    UHeadAggr     head;
    UBodyAggrVec  body;
};

// }}}

} } // namespace Input Gringo

#endif // _GRINGO_INPUT_STATEMENT_HH
