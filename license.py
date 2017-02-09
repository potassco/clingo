#!/usr/bin/env python
import re
import os
import sys

gpl      = re.compile(r"// {{{ GPL License.*?// }}}\n", re.MULTILINE | re.DOTALL)
filetype = re.compile(r"^.*\.(cc|c|h|hh|yy|xh|xch)$")
mit      = """\
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
"""
for path, directories, files in os.walk('.'):
    if path == ".":
        directories.remove("clasp")
        directories.remove("build")
    for x in files:
        if filetype.match(x):
            filepath = os.path.join(path, x)
            content = open(filepath).read()
            open(filepath, "w").write(gpl.sub(mit, content))

