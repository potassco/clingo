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

#ifndef CLINGO_SCRIPTS_HH
#define CLINGO_SCRIPTS_HH

#include <clingo/control.hh>
#include <gringo/base.hh>

namespace Gringo {

class Script : public Context {
public:
    virtual void main(Control &ctl) = 0;
    virtual char const *version() = 0;
    ~Script() override = default;
};
using UScript = std::shared_ptr<Script>;
using UScriptVec = std::vector<std::tuple<String, bool, UScript>>;

class Scripts : public Context {
public:
    Scripts() = default;
    bool callable(String name) override;
    SymVec call(Location const &loc, String name, SymSpan args, Logger &log) override;
    void main(Control &ctl);
    void registerScript(String type, UScript script);
    void setContext(Context &ctx) { context_ = &ctx; }
    void resetContext() { context_ = nullptr; }
    void exec(String type, Location loc, String code) override;
    char const *version(String type);

    ~Scripts() override;
private:
    Context *context_ = nullptr;
    UScriptVec scripts_;
};

Scripts &g_scripts();

} // namespace Gringo

#endif // CLINGO_SCRIPTS_HH

