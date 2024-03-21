#include <gringo/grounder/grounder.hh>

#include <gringo/ground/term.hh>

#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/parse.hh>
#include <gringo/input/algo/print.hh>
#include <gringo/input/algo/visit_variables.hh>

#include <gringo/util/unordered_map.hh>

#include <filesystem>
#include <iostream>

namespace Gringo {

namespace {

//! Helper for parsing.
struct Parser {
    //! Scan statements.
    template <class Scanner> void process(std::filesystem::path const &dir, Scanner &&scanner) {
        prg.ensure_base = true;
        for (auto stm = scanner.scan(); stm.has_value(); stm = scanner.scan()) {
            if (auto *include = std::get_if<Input::StmInclude>(&*stm); include != nullptr) {
                includes.emplace_back(dir, *include);
            } else {
                prg.add(store, std::move(stm).value());
            }
        }
    }

    //! Parse a program from the given path.
    auto process_path(auto &&path, bool required) -> bool {
        if (std::filesystem::exists(path)) {
            path = std::filesystem::canonical(path);
            auto rel = path.lexically_relative(root);
            if (!std::filesystem::is_directory(path)) {
                if (seen.emplace(path).second) {
                    process(path.parent_path(), Input::scan_file(log, store, rel.c_str()));
                } else {
                    GRINGO_REPORT(log, info_file_included) << "file already included: " << rel;
                }
            } else {
                GRINGO_REPORT(log, error) << "cannot include directory: " << rel;
            }
            return true;
        }
        if (required) {
            GRINGO_REPORT(log, error) << "file not found: " << path;
        }
        return false;
    }

    //! Parse a program from stdin.
    void process_stdin() {
        if (!processed_stdin) {
            processed_stdin = true;
            process(root, Input::scan_stream(log, store, std::cin));
        } else {
            GRINGO_REPORT(log, info_file_included) << "file already included: -";
        }
    }

    //! Process includes encountered while parsing.
    void process_includes() {
        for (; !includes.empty(); includes.pop_front()) {
            auto const &[parent, include] = includes.front();
            if (include.type() == Input::IncludeType::system) {
                auto path = std::filesystem::path(include.value());
                if (path.is_relative() && parent != root) {
                    if (process_path(parent / path, false)) {
                        continue;
                    }
                }
                process_path(path, true);
            }
        }
    }

    Logger &log;
    SymbolStore &store;
    Input::UnprocessedProgram &prg;
    std::filesystem::path root = std::filesystem::current_path();
    std::deque<std::pair<std::filesystem::path, Input::StmInclude>> includes = {};
    Util::unordered_set<std::filesystem::path> seen = {};
    bool processed_stdin = false;
};

struct BuilderTerm {
    auto operator()(Input::TermVariable const &term) const -> Ground::UTerm {
        assert(var_map.find(term.name()) != var_map.end());
        return std::make_unique<Ground::TermVariable>(var_map.find(term.name())->second);
    }
    auto operator()(Input::TermSymbol const &term) const -> Ground::UTerm {
        return std::make_unique<Ground::TermSymbol>(term.value());
    }
    auto operator()(Input::TermTuple const &term) const -> Ground::UTerm {
        assert(term.pool().size() == 1 && std::holds_alternative<Input::ArgumentTuple>(term.pool().front()));
        auto const &args = std::get<Input::ArgumentTuple>(term.pool().front()).elems();
        Ground::UTermVec g_args;
        g_args.reserve(args.size());
        for (auto const &arg : args) {
            if (!std::holds_alternative<Input::Term>(arg)) {
                // This should set a flag.
                // The term can be constructed by inserting a special projection term,
                // which are handled in a second pass:
                // - create a copy of renaming variables in order of occurrence
                //   - this term can serve as a unique representation to identify the projection
                //   - we can genearate a unique name for a projection domain
                // - create a copy removing projected places setting this unique name
                //   - we need two copies one with the original and the renamed variables
                //   - the version with the original variables can be used in rule bodies
                //   - the version with the renamed variables can be used to create a projection rule
                throw std::logic_error("implement me: handle projection!!!");
            }
            g_args.emplace_back(std::visit(*this, std::get<Input::Term>(arg)));
        }
        return std::make_unique<Ground::TermTuple>(std::move(g_args));
    }
    auto operator()(Input::TermFunction const &term) const -> Ground::UTerm {
        assert(!term.external() && term.pool().size() == 1);
        auto const &args = term.pool().front().elems();
        Ground::UTermVec g_args;
        g_args.reserve(args.size());
        for (auto const &arg : args) {
            if (!std::holds_alternative<Input::Term>(arg)) {
                // see above...
                throw std::logic_error("implement me: handle projection!!!");
            }
            g_args.emplace_back(std::visit(*this, std::get<Input::Term>(arg)));
        }
        return std::make_unique<Ground::TermFunction>(term.name(), std::move(g_args));
    }
    auto operator()(Input::TermAbs const &term) const -> Ground::UTerm {
        assert(term.pool().size() == 1);
        return std::make_unique<Ground::TermUnary>(Ground::UnaryOperator::abs, std::visit(*this, term.pool().front()));
    }
    auto operator()(Input::TermUnary const &term) const -> Ground::UTerm {
        Ground::UnaryOperator op =
            term.op() == Input::UnaryOperator::negate ? Ground::UnaryOperator::minus : Ground::UnaryOperator::invert;
        return std::make_unique<Ground::TermUnary>(op, std::visit(*this, *term.rhs()));
    }
    auto operator()(Input::TermBinary const &term) const -> Ground::UTerm {
        assert(term.op() != Input::BinaryOperator::dots);
        if (auto lin = Input::check_linear(term); lin) {
            assert(var_map.find(lin->x()) != var_map.end());
            return std::make_unique<Ground::TermLinear>(*lin->m(), var_map.find(lin->x())->second, lin->n());
        }
        Ground::BinaryOperator op = Ground::BinaryOperator::plus;
        switch (term.op()) {
            case Input::BinaryOperator::and_: {
                op = Ground::BinaryOperator::and_;
                break;
            }
            case Input::BinaryOperator::div: {
                op = Ground::BinaryOperator::div;
                break;
            }
            case Input::BinaryOperator::dots: {
                break;
            }
            case Input::BinaryOperator::minus: {
                op = Ground::BinaryOperator::minus;
                break;
            }
            case Input::BinaryOperator::mod: {
                op = Ground::BinaryOperator::mod;
                break;
            }
            case Input::BinaryOperator::or_: {
                op = Ground::BinaryOperator::or_;
                break;
            }
            case Input::BinaryOperator::plus: {
                op = Ground::BinaryOperator::plus;
                break;
            }
            case Input::BinaryOperator::pow: {
                op = Ground::BinaryOperator::pow;
                break;
            }
            case Input::BinaryOperator::times: {
                op = Ground::BinaryOperator::times;
                break;
            }
            case Input::BinaryOperator::xor_: {
                op = Ground::BinaryOperator::xor_;
                break;
            }
        }
        return std::make_unique<Ground::TermBinary>(std::visit(*this, *term.lhs()), op, std::visit(*this, *term.rhs()));
    }
    Util::unordered_map<String, size_t> &var_map;
};

struct BuilderLit {
    template <class T> void operator()([[maybe_unused]] T const &lit) const {
        throw std::logic_error("implement me!!!");
    }
    void operator()(Input::LitSymbolic const &lit) const {
        BuilderTerm bld_term{var_map};
        auto term = std::visit(bld_term, lit.term());
    }
    Util::unordered_map<String, size_t> &var_map;
};

struct BuilderHdLit {
    template <class T> void operator()([[maybe_unused]] T const &lit) const {
        throw std::logic_error("implement me!!!");
    }
    void operator()(Input::HdLitSimple const &lit) const {
        auto bld_lit = BuilderLit{var_map};
        std::visit(bld_lit, lit.lit());
    }
    Util::unordered_map<String, size_t> &var_map;
};

struct BuilderBdLit {
    template <class T> void operator()([[maybe_unused]] T const &lit) const {
        throw std::logic_error("implement me!!!");
    }
    void operator()(Input::BdLitSimple const &lit) const {
        auto bld_lit = BuilderLit{var_map};
        std::visit(bld_lit, lit.lit());
    }
    Util::unordered_map<String, size_t> &var_map;
};

struct BuilderStm {
    template <class T> void operator()([[maybe_unused]] T const &stm) const {
        throw std::logic_error("implement me!!!");
    }
    void operator()(Input::StmRule const &stm) const {
        auto bld_bd = BuilderBdLit{var_map};
        auto bld_hd = BuilderHdLit{var_map};
        // TODO: first step handle simple literals
        std::cerr << stm << "\n";
        std::visit(bld_hd, stm.head());
        for (auto const &lit : stm.body()) {
            std::visit(bld_bd, lit);
        }
    }
    Util::unordered_map<String, size_t> &var_map;
};

// TODO: here transform statements into grounding directives
struct Builder : Input::DependencyBuilder {
    void param(Input::ProgramParam const &param) override {
        std::cerr << "#program_" << param.first << "(";
        bool comma = false;
        for (auto const &sym : param.second) {
            if (comma) {
                std::cerr << ", ";
            } else {
                comma = true;
            }
            std::cerr << sym;
        }
        std::cerr << ").\n";
    }
    void meta(std::vector<Input::Stm> const &stms) override {
        for (auto const &stm : stms) {
            std::cerr << stm << "\n";
        }
    }
    void fact(std::vector<Symbol> const &facts) override {
        for (auto const &fact : facts) {
            std::cerr << fact << ".\n";
        }
    }
    void components(Input::Components const &comps) override {
        for (auto const &ref_comps : comps) {
            std::cerr << "% component\n";
            for (auto const &ref_comp : ref_comps) {
                std::cerr << "% refined component\n";
                for (auto const &stm : ref_comp.stms) {
                    Util::unordered_map<String, size_t> var_map;
                    Input::visit_variables(
                        *stm,
                        [&var_map]([[maybe_unused]] Input::Location const &loc, String var) {
                            var_map.try_emplace(var, var_map.size());
                        },
                        Input::VariableContext::all);
                    auto bld_stm = BuilderStm{var_map};
                    std::visit(bld_stm, *stm);
                }
            }
        }
    }
};

} // namespace

void Grounder::parse(std::string_view prg) {
    GRINGO_REPORT(log_, debug) << "parsing...";
    auto prs = Parser{log_, store_, unprocessed_prg_};
    auto scanner = Input::scan_string(log_, store_, prg);
    prs.process(prs.root, scanner);
    prs.process_includes();
}

void Grounder::parse(std::vector<std::string> const &files) {
    GRINGO_REPORT(log_, debug) << "parsing...";
    auto prs = Parser{log_, store_, unprocessed_prg_};
    if (files.empty()) {
        prs.process_stdin();
        prs.process_includes();
    }
    for (auto const &file : files) {
        if (file == "-") {
            prs.process_stdin();
        } else {
            prs.process_path(std::filesystem::path(file), true);
        }
        prs.process_includes();
    }
}

void Grounder::prepare() {
    GRINGO_REPORT(log_, debug) << "preparing...";
    prg_.join(log_, store_, std::move(unprocessed_prg_));
    unprocessed_prg_.clear();
}

void Grounder::ground(Input::ProgramParamVec const &params) {
    GRINGO_REPORT(log_, debug) << "grounding...";
    Builder bld;
    prg_.analyze(store_, params, bld);
    std::cerr.flush();
}

void Grounder::output_unprocessed_program(std::ostream &out) {
    for (auto const &stm : unprocessed_prg_.const_stms) {
        out << stm << "\n";
    }
    for (auto const &stm : unprocessed_prg_.thy_stms) {
        out << stm << "\n";
    }
    for (auto const &stm : unprocessed_prg_.meta_stms) {
        out << stm << "\n";
    }
    for (auto const &[prg_stm, stms, facts] : unprocessed_prg_.parts) {
        out << prg_stm << "\n";
        for (auto fact : facts) {
            out << fact << ".\n";
        }
        for (auto stm : stms) {
            out << stm << "\n";
        }
    }
    out.flush();
}

void Grounder::output_program(std::ostream &out) {
    prg_.visit_stms(store_, [&out](auto const &stm) { out << stm << "\n"; });
    out.flush();
}

} // namespace Gringo
