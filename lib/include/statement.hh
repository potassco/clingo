#pragma once

#include <iostream>
#include <memory>
#include <optional>

#include <body_literal.hh>
#include <head_literal.hh>

struct Statement {
    virtual ~Statement() = default;
    virtual void print(std::ostream &out) const = 0;
    [[nodiscard]] auto to_string() const -> std::string {
        std::ostringstream out;
        out << *this;
        return out.str();
    }
    friend auto operator<<(std::ostream &out, Statement const &stm) -> std::ostream & {
        stm.print(out);
        return out;
    }
};

using UStatement = std::unique_ptr<Statement>;

struct Rule : Statement {
    explicit Rule(UHeadLiteral head, UBodyLiteralVec body) : head{std::move(head)}, body{std::move(body)} {}
    void print(std::ostream &out) const override {
        out << *head;
        if (head->print_empty() || !body.empty()) {
            out << " :- " << p_range(body, "; ");
        }
        out << ".";
    }
    UHeadLiteral head;
    UBodyLiteralVec body;
};

enum class TheoryOpType { unary, binary_left, binary_right };

struct TheoryOpDefinition {
    explicit TheoryOpDefinition(std::string op, int priority, TheoryOpType type)
        : op{std::move(op)}, priority{priority}, type{type} {}
    friend auto operator<<(std::ostream &out, TheoryOpDefinition const &def) -> std::ostream & {
        out << def.op << " : " << def.priority << ", ";
        switch (def.type) {
            case TheoryOpType::unary: {
                out << "unary";
                break;
            }
            case TheoryOpType::binary_left: {
                out << "binary, left";
                break;
            }
            case TheoryOpType::binary_right: {
                out << "binary, right";
                break;
            }
        }
        return out;
    }
    std::string op;
    int priority;
    TheoryOpType type;
};
using TheoryOpDefinitionVec = std::vector<TheoryOpDefinition>;

struct TheoryTermDefinition {
    explicit TheoryTermDefinition(std::string name, TheoryOpDefinitionVec op_defs)
        : name{std::move(name)}, op_defs{std::move(op_defs)} {}
    friend auto operator<<(std::ostream &out, TheoryTermDefinition const &def) -> std::ostream & {
        out << "  " << def.name << " {";
        if (def.op_defs.empty()) {
            out << " }";
        } else if (def.op_defs.size() == 1) {
            out << " " << def.op_defs.front() << " }";
        } else {
            out << "\n"
                << p_range_with(def.op_defs, ";\n", [](std::ostream &out, auto &op_def) { out << "    " << op_def; })
                << "\n  }";
        }
        return out;
    }
    std::string name;
    std::vector<TheoryOpDefinition> op_defs;
};
using TheoryTermDefinitionVec = std::vector<TheoryTermDefinition>;

enum class TheoryAtomType { head, body, any, directive };

inline auto operator<<(std::ostream &out, TheoryAtomType type) -> std::ostream & {
    switch (type) {
        case TheoryAtomType::head: {
            out << "head";
            break;
        }
        case TheoryAtomType::body: {
            out << "body";
            break;
        }
        case TheoryAtomType::any: {
            out << "any";
            break;
        }
        case TheoryAtomType::directive: {
            out << "directive";
            break;
        }
    }
    return out;
}

struct TheoryAtomDefinition {
    using Guard = std::optional<std::pair<std::vector<std::string>, std::string>>;
    explicit TheoryAtomDefinition(std::string name, int arity, std::string term, Guard guard, TheoryAtomType type)
        : name(std::move(name)), arity(arity), term(std::move(term)), guard(std::move(guard)), type(type) {}
    friend auto operator<<(std::ostream &out, TheoryAtomDefinition const &def) -> std::ostream & {
        out << "  &" << def.name << "/" << def.arity << ": " << def.term << ", ";
        if (def.guard) {
            out << "{" << p_range(def.guard->first, ",") << "}, " << def.guard->second << ", ";
        }
        out << def.type;
        return out;
    }
    std::string name;
    int arity;
    std::string term;
    std::optional<std::pair<std::vector<std::string>, std::string>> guard;
    TheoryAtomType type;
};
using TheoryAtomDefinitionVec = std::vector<TheoryAtomDefinition>;

struct TheoryDefinition : Statement {
    explicit TheoryDefinition(std::string name, TheoryTermDefinitionVec term_defs, TheoryAtomDefinitionVec atom_defs)
        : name{std::move(name)}, term_defs{std::move(term_defs)}, atom_defs{std::move(atom_defs)} {}
    void print(std::ostream &out) const override {
        out << "#theory " << name << (term_defs.empty() && atom_defs.empty() ? " { " : " {\n");
        out << p_range(term_defs, ";\n");
        if (!term_defs.empty()) {
            if (!atom_defs.empty()) {
                out << ";";
            }
            out << "\n";
        }
        out << p_range(atom_defs, ";\n");
        if (!atom_defs.empty()) {
            out << "\n";
        }
        out << "}.";
    }
    std::string name;
    TheoryTermDefinitionVec term_defs;
    TheoryAtomDefinitionVec atom_defs;
};

enum class OptimizeType { minimize, maximize };

inline auto operator<<(std::ostream &out, OptimizeType type) -> std::ostream & {
    switch (type) {
        case OptimizeType::maximize: {
            out << "#maximize";
            break;
        }
        case OptimizeType::minimize: {
            out << "#minimize";
            break;
        }
    }
    return out;
}

struct StatementOptimize : Statement {
    using Tuple = std::tuple<UTerm, std::optional<UTerm>, UTermVec>;
    using Element = std::tuple<Tuple, ULiteralVec>;
    using ElementVec = std::vector<Element>;
    explicit StatementOptimize(OptimizeType type, ElementVec elems) : type{type}, elems{std::move(elems)} {}
    void print(std::ostream &out) const override {
        out << type << " { "
            << p_range_with(elems, "; ",
                            [](std::ostream &out, auto const &elem) {
                                auto const &[tuple, cond] = elem;
                                auto const &[weight, prio, terms] = tuple;
                                out << *weight;
                                if (prio) {
                                    out << "@" << *prio.value();
                                }
                                if (!terms.empty()) {
                                    out << "," << p_range(terms);
                                }
                                if (!cond.empty()) {
                                    out << ": " << p_range(cond, ", ");
                                }
                            })
            << (elems.empty() ? "}" : " }") << ".";
    }
    OptimizeType type;
    ElementVec elems;
};

struct StatementWeakConstraint : Statement {
    using Tuple = StatementOptimize::Tuple;
    explicit StatementWeakConstraint(UBodyLiteralVec body, Tuple tuple)
        : body{std::move(body)}, tuple{std::move(tuple)} {}
    void print(std::ostream &out) const override {
        auto const &[weight, prio, terms] = tuple;
        out << " :~ " << p_range(body, "; ") << ". [" << *weight;
        if (prio) {
            out << "@" << *prio.value();
        }
        if (!terms.empty()) {
            out << "," << p_range(terms);
        }
        out << "]";
    }
    UBodyLiteralVec body;
    Tuple tuple;
};

struct StatementShow : Statement {
    StatementShow(UTerm term, UBodyLiteralVec body) : term(std::move(term)), body(std::move(body)) {}
    void print(std::ostream &out) const override { out << "#show " << *term << ": " << p_range(body, "; ") << "."; }
    UTerm term;
    UBodyLiteralVec body;
};

struct StatementShowSig : Statement {
    StatementShowSig(bool has_sign, std::string name, int arity)
        : has_sign{has_sign}, name{std::move(name)}, arity{arity} {}
    void print(std::ostream &out) const override {
        out << "#show " << (has_sign ? "-" : "") << name << "/" << arity << ".";
    }
    bool has_sign;
    std::string name;
    int arity;
};

struct StatementProject : Statement {
    StatementProject(UTerm term, UBodyLiteralVec body) : term(std::move(term)), body(std::move(body)) {}
    void print(std::ostream &out) const override {
        out << "#project " << *term << (body.empty() ? "" : ": ") << p_range(body, "; ") << ".";
    }
    UTerm term;
    UBodyLiteralVec body;
};

struct StatementProjectSig : Statement {
    StatementProjectSig(bool has_sign, std::string name, int arity)
        : has_sign{has_sign}, name{std::move(name)}, arity{arity} {}
    void print(std::ostream &out) const override {
        out << "#project " << (has_sign ? "-" : "") << name << "/" << arity << ".";
    }
    bool has_sign;
    std::string name;
    int arity;
};

struct StatementDefined : Statement {
    StatementDefined(bool has_sign, std::string name, int arity)
        : has_sign{has_sign}, name{std::move(name)}, arity{arity} {}
    void print(std::ostream &out) const override {
        out << "#defined " << (has_sign ? "-" : "") << name << "/" << arity << ".";
    }
    bool has_sign;
    std::string name;
    int arity;
};

struct StatementExternal : Statement {
    StatementExternal(UTerm term, UBodyLiteralVec body) : term(std::move(term)), body(std::move(body)) {}
    StatementExternal(UTerm term, UBodyLiteralVec body, UTerm type)
        : term(std::move(term)), body(std::move(body)), type{std::move(type)} {}
    void print(std::ostream &out) const override {
        out << "#external " << *term << (body.empty() ? "" : ": ") << p_range(body, "; ") << ".";
        if (type.has_value()) {
            out << " [" << *type.value() << "]";
        }
    }
    UTerm term;
    UBodyLiteralVec body;
    std::optional<UTerm> type;
};

struct StatementEdge : Statement {
    using Edge = std::pair<UTerm, UTerm>;
    using EdgeVec = std::vector<Edge>;
    explicit StatementEdge(EdgeVec edges, UBodyLiteralVec body = {}) : edges{std::move(edges)}, body{std::move(body)} {}
    void print(std::ostream &out) const override {
        out << "#edge ("
            << p_range_with(edges, ";",
                            [](std::ostream &out, auto &edge) { out << *edge.first << "," << *edge.second; })
            << ")" << (body.empty() ? "" : ": ") << p_range(body, "; ") << ".";
    }
    EdgeVec edges;
    UBodyLiteralVec body;
};

struct StatementHeuristic : Statement {
    explicit StatementHeuristic(bool has_sign, UTerm atom, UBodyLiteralVec body, UTerm type, UTerm priority,
                                UTerm modifier)
        : atom{std::move(atom)}, body{std::move(body)}, type(std::move(type)), priority(std::move(priority)),
          modifier(std::move(modifier)), has_sign{has_sign} {}
    explicit StatementHeuristic(bool has_sign, UTerm atom, UBodyLiteralVec body, UTerm type, UTerm modifier)
        : atom{std::move(atom)}, body{std::move(body)}, type(std::move(type)), modifier(std::move(modifier)),
          has_sign{has_sign} {}
    void print(std::ostream &out) const override {
        out << "#heuristic " << (has_sign ? "-" : "") << *atom << (body.empty() ? "" : ": ") << p_range(body, "; ")
            << ". [" << *type;
        if (priority) {
            out << "@" << *priority.value();
        }
        out << "," << *modifier << "]";
    }
    UTerm atom;
    UBodyLiteralVec body;
    UTerm type;
    std::optional<UTerm> priority;
    UTerm modifier;
    bool has_sign;
};

enum class ScriptType {
    lua,
    python,
};

inline auto operator<<(std::ostream &out, ScriptType type) -> std::ostream & {
    switch (type) {
        case ScriptType::lua: {
            out << "lua";
            break;
        }
        case ScriptType::python: {
            out << "python";
            break;
        }
    }
    return out;
}

struct StatementScript : Statement {
    explicit StatementScript(ScriptType type, std::string content) : type(type), content(std::move(content)) {}
    void print(std::ostream &out) const override { out << "#script (" << type << ")" << content << "#end."; }
    ScriptType type;
    std::string content;
};
