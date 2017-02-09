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

#ifndef PYCLINGO_PYCLINGO_H
#define PYCLINGO_PYCLINGO_H

#include <clingo/script.h>

#if defined _WIN32 || defined __CYGWIN__
#   define PYCLINGO_WIN
#endif
#ifdef PYCLINGO_NO_VISIBILITY
#   define PYCLINGO_VISIBILITY_DEFAULT
#   define PYCLINGO_VISIBILITY_PRIVATE
#else
#   ifdef PYCLINGO_WIN
#       ifdef PYCLINGO_BUILD_LIBRARY
#           define PYCLINGO_VISIBILITY_DEFAULT __declspec (dllexport)
#       else
#           define PYCLINGO_VISIBILITY_DEFAULT __declspec (dllimport)
#       endif
#       define PYCLINGO_VISIBILITY_PRIVATE
#   else
#       if __GNUC__ >= 4
#           define PYCLINGO_VISIBILITY_DEFAULT  __attribute__ ((visibility ("default")))
#           define PYCLINGO_VISIBILITY_PRIVATE __attribute__ ((visibility ("hidden")))
#       else
#           define PYCLINGO_VISIBILITY_DEFAULT
#           define PYCLINGO_VISIBILITY_PRIVATE
#       endif
#   endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

PYCLINGO_VISIBILITY_DEFAULT void *clingo_init_python_();
PYCLINGO_VISIBILITY_DEFAULT bool clingo_register_python_();

#ifdef __cplusplus
}
#endif

#endif // PYCLINGO_PYCLINGO_H
