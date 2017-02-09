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

#ifndef _GRINGO_GROUND_PROGRAM_HH
#define _GRINGO_GROUND_PROGRAM_HH

#include <gringo/ground/statement.hh>

namespace Gringo { namespace Ground {

// {{{ declaration of Program

using IdVec     = std::vector<std::pair<Location, String>>;
using SEdb      = std::shared_ptr<std::pair<UTerm, SymVec>>;
using SEdbVec   = std::vector<SEdb>;
using SymVecSet = std::set<SymVec>;
using ParamSet  = std::map<Sig, SymVecSet>;

struct Parameters {
    Parameters();
    void add(String name, SymVec &&args);
    bool find(Sig sig) const;
    ParamSet::const_iterator begin() const;
    ParamSet::const_iterator end() const;
    bool empty() const;
    void clear();
    ~Parameters();
    ParamSet params;
};

struct Program {
    using ClassicalNegationVec = std::vector<std::pair<PredicateDomain &, PredicateDomain &>>;

    Program(SEdbVec &&edb, Statement::Dep::ComponentVec &&stms, ClassicalNegationVec &&negate);
    void linearize(Context &context, Logger &log);
    void ground(Parameters const &params, Context &context, Output::OutputBase &out, bool finalize, Logger &log);
    void ground(Context &context, Output::OutputBase &out, Logger &log);

    SEdbVec                      edb;
    bool                         linearized = false;
    Statement::Dep::ComponentVec stms;
    ClassicalNegationVec         negate;
};

std::ostream &operator<<(std::ostream &out, Program const &x);

// }}}

} } // namespace Ground Gringo

#endif // _GRINGO_GROUND_PROGRAM_HH
