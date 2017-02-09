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

#include "gringo/input/literal.hh"

namespace Gringo { namespace Input {

// {{{ definition of Projection

Projection::Projection(UTerm &&projected, UTerm &&project)
: projected(std::move(projected))
, project(std::move(project)) { }

Projection::Projection(Projection &&) = default;
Projection &Projection::operator=(Projection &&) = default;

Projection::~Projection() = default;

Projection::operator Term const &() const { return *projected; }

// }}}
// {{{ definition of Projections

Projections::Projections() = default;

Projections::Projections(Projections &&) = default;

Projections::~Projections() = default;

Projections::ProjectionMap::Iterator Projections::begin() {
    return proj.begin();
}
Projections::ProjectionMap::Iterator Projections::end() {
    return proj.end();
}

UTerm Projections::add(Term &term) {
    AuxGen gen;
    auto ret(term.project(true, gen));
    proj.push(std::move(std::get<1>(ret)), std::move(std::get<2>(ret)));
    return std::move(std::get<0>(ret));
}

// }}}

} } // namespace Input Gringo
