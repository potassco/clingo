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

namespace {

struct Deepcopy {
    template <typename T>
    AST::Value operator()(T const &val) {
        return val;
    }
    AST::Value operator()(SAST &ast) {
        return ast->deepcopy();
    }
    AST::Value operator()(AST::ASTVec &asts) {
        AST::ASTVec ret;
        ret.reserve(asts.size());
        for (auto &ast : asts) {
            ret.emplace_back(ast->deepcopy());
        }
        return AST::Value{std::move(ret)};
    }
};

} // namespace

AST::AST(clingo_ast_type type)
: type_{type} { }

bool AST::hasValue(clingo_ast_attribute name) const {
    return values_.find(name) != values_.end();
}

AST::Value const &AST::value(clingo_ast_attribute name) const {
    auto it = values_.find(name);
    if (it == values_.end()) {
        std::ostringstream oss;
        oss << "ast "
            << "'" << g_clingo_ast_constructors.constructors[type()].name << "'"
            << " does not have attribute "
            << "'" << g_clingo_ast_attribute_names.names[name] << "'";
        throw std::runtime_error(oss.str());
    }
    return it->second;
}

AST::Value &AST::value(clingo_ast_attribute name) {
    auto it = values_.find(name);
    if (it == values_.end()) {
        std::ostringstream oss;
        oss << "ast "
            << "'" << g_clingo_ast_constructors.constructors[type()].name << "'"
            << " does not have attribute "
            << "'" << g_clingo_ast_attribute_names.names[name] << "'";
        throw std::runtime_error(oss.str());
    }
    return it->second;
}

void AST::value(clingo_ast_attribute name, Value value) {
    values_[name] = std::move(value);
}

SAST AST::copy() {
    return SAST{*this};
}

SAST AST::deepcopy() {
    auto ast = SAST{type_};
    for (auto &val : values_) {
        ast->values_.emplace(val.first, mpark::visit(Deepcopy{}, val.second));
    }
    return ast;
}

void AST::incRef() {
    ++refCount_;
}

void AST::decRef() {
    --refCount_;
}

unsigned AST::refCount() const {
    return refCount_;
}

bool AST::unique() const {
    return refCount_ == 1;
}

clingo_ast_type AST::type() const {
    return type_;
}

SAST::SAST()
: ast_{nullptr} { }

SAST::SAST(AST *ast)
: ast_{ast} {
    if (ast_ != nullptr) {
        ast_->incRef();
    }
}

SAST::SAST(clingo_ast_type type)
: ast_{new AST{type}} {
    ast_->incRef();
}

SAST::SAST(AST const &ast)
: ast_{new AST{ast}} {
    ast_->incRef();
}

SAST::SAST(SAST const &ast)
: ast_{ast.ast_} {
    if (ast_ != nullptr) {
        ast_->incRef();
    }
}

SAST::SAST(SAST &&ast) noexcept
: ast_{nullptr} {
    std::swap(ast_, ast.ast_);
}

SAST &SAST::operator=(SAST const &ast) {
    if (this != &ast) {
        clear();
        if (ast.ast_ != nullptr) {
            ast_ = ast.ast_;
            ast_->incRef();
        }
    }
    return *this;
}

SAST &SAST::operator=(SAST &&ast) noexcept {
    std::swap(ast_, ast.ast_);
    return *this;
}

AST *SAST::operator->() const {
    return ast_;
}

unsigned SAST::use_count() const {
    return ast_->refCount();
}

AST *SAST::get() const {
    return ast_;
}

void SAST::clear() {
    if (ast_ != nullptr) {
        ast_->decRef();
        if (ast_->refCount() == 0) {
            delete ast_;
        }
    }
}

AST &SAST::operator*() const {
    return *ast_;
}

SAST::~SAST() {
    clear();
}

} } // namespace Input Gringo
