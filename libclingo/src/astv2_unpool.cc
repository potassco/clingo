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

template <class T>
T &get(AST &ast, clingo_ast_attribute name) {
    return mpark::get<T>(ast.value(name));
}


} // namespace

void unpool_term(SAST &ast, std::vector<SAST> &pool) {
    switch (ast->type()) {
        case clingo_ast_type_variable:
        case clingo_ast_type_symbolic_term: {
            pool.emplace_back(std::move(ast));
            break;
        }
        case clingo_ast_type_unary_operation: {
            std::vector<SAST> arg;
            unpool_term(get<SAST>(*ast, clingo_ast_attribute_argument), arg);
            AST *cpy = nullptr;
            for (auto &term : arg) {
                if (cpy == nullptr) {
                    cpy = ast.get();
                    pool.emplace_back(std::move(ast));
                }
                else {
                    pool.emplace_back(cpy->copy());
                }
                pool.back()->value(clingo_ast_attribute_argument) = std::move(term);
            }
            break;
        }
        case clingo_ast_type_binary_operation:
        case clingo_ast_type_interval:
        case clingo_ast_type_function: {
            throw std::logic_error("implement me!!!");
        }
        case clingo_ast_type_pool: {
            for (auto &term : mpark::get<AST::ASTVec>(ast->value(clingo_ast_attribute_terms))) {
                unpool_term(term, pool);
            }
            break;
        }
        case clingo_ast_type_csp_product:
        case clingo_ast_type_csp_sum:
        case clingo_ast_type_csp_guard:
        default:
            throw std::logic_error("implement me!!!");
    }
}

} } //namespace Input Gringo
