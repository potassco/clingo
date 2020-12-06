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

#include <clingo/astv2.hh>

namespace Gringo { namespace Input {

Value::Value(int num)
: type_{ValueType::Num}
, num_{num} { }

Value::Value(Symbol sym)
: type_{ValueType::Sym}
, sym_{sym} { }

Value::Value(String str)
: type_{ValueType::Str}
, str_{str} { }

Value::Value(SStrVec strs)
: type_{ValueType::StrVec} {
    new (&strs_) SStrVec{std::move(strs)};
}
Value::Value(SASTVec asts)
: type_{ValueType::ASTVec} {
    new (&asts_) SASTVec{std::move(asts)};
}

Value::Value(Value const &val)
: type_{val.type_} {
    switch (val.type_) {
        case ValueType::Num: {
            num_ = val.num_;
            break;
        }
        case ValueType::Str: {
            str_ = val.str_;
            break;
        }
        case ValueType::Sym: {
            sym_ = val.sym_;
            break;
        }
        case ValueType::AST: {
            new (&ast_) SAST{val.ast_};
            break;
        }
        case ValueType::StrVec: {
            new (&ast_) SStrVec{val.strs_};
            break;
        }
        case ValueType::ASTVec: {
            new (&ast_) SASTVec{val.asts_};
            break;
        }
    }
}

Value::Value(Value &&val) noexcept
: type_{val.type_} {
    switch (val.type_) {
        case ValueType::Num: {
            num_ = val.num_;
            break;
        }
        case ValueType::Str: {
            str_ = val.str_;
            break;
        }
        case ValueType::Sym: {
            sym_ = val.sym_;
            break;
        }
        case ValueType::AST: {
            new (&ast_) SAST{std::move(val.ast_)};
            break;
        }
        case ValueType::StrVec: {
            new (&strs_) SStrVec{std::move(val.strs_)};
            break;
        }
        case ValueType::ASTVec: {
            new (&asts_) SASTVec{std::move(val.asts_)};
            break;
        }
    }
}

Value &Value::operator=(Value const &val) {
    clear_();
    new (this) Value(val);
    return *this;
}

Value &Value::operator=(Value &&val) noexcept {
    clear_();
    new (this) Value(std::move(val));
    return *this;
}

Value::~Value() {
    clear_();
}

void Value::clear_() {
    switch (type_) {
        case ValueType::AST: {
            ast_ = nullptr;
            type_ = ValueType::Num;
            break;
        }
        case ValueType::StrVec: {
            strs_ = nullptr;
            type_ = ValueType::Num;
            break;
        }
        case ValueType::ASTVec: {
            asts_ = nullptr;
            type_ = ValueType::Num;
            break;
        }
        default: {
            break;
        }
    }
}

void Value::require_(ValueType type) const {
    if (type_ != type) {
        throw std::runtime_error("invalid attribute");
    }
}

ValueType Value::type() const {
    return type_;
}

int &Value::num() {
    require_(ValueType::Num);
    return num_;
}

int Value::num() const {
    require_(ValueType::Num);
    return num_;
}

String &Value::str() {
    require_(ValueType::Str);
    return str_;
}

String Value::str() const {
    require_(ValueType::Str);
    return str_;
}

SAST &Value::ast() {
    require_(ValueType::AST);
    return ast_;
}

SAST const &Value::ast() const {
    require_(ValueType::AST);
    return ast_;
}

Value::SASTVec &Value::asts() {
    require_(ValueType::ASTVec);
    return asts_;
}

Value::SASTVec const &Value::asts() const {
    require_(ValueType::ASTVec);
    return asts_;
}

Value::SStrVec &Value::strs() {
    require_(ValueType::StrVec);
    return strs_;
}

Value::SStrVec const &Value::strs() const {
    require_(ValueType::StrVec);
    return strs_;
}

} } // namespace Input Gringo
