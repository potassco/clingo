#include <gringo/grounder/grounder.hh>

#include <gringo/input/algo/parse.hh>
#include <gringo/input/algo/print.hh>

#include <gringo/util/ordered_set.hh>

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

struct BuilderStm {
    template <class T> void operator()([[maybe_unused]] T const &stm) const {
        throw std::logic_error("implement me!!!");
    }
    void operator()(Input::StmRule const &stm) const {
        // TODO: first step handle simple literals
        std::cerr << stm << "\n";
    }
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
        BuilderStm bld_stm;
        for (auto const &ref_comps : comps) {
            std::cerr << "% component\n";
            for (auto const &ref_comp : ref_comps) {
                std::cerr << "% refined component\n";
                for (auto const &stm : ref_comp.stms) {
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
