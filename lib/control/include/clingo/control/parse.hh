#pragma once

#include <clingo/input/parser.hh>
#include <clingo/input/print.hh>
#include <clingo/input/program.hh>

#include <clingo/ground/script.hh>

#include <clingo/util/enum.hh>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace CppClingo::Control {

//! @addtogroup control
//! @{

//! Bitset of enabled builtin includes.
enum class BuiltinIncludes : uint8_t {
    empty = 0,  //!< The empty set.
    incmode = 1 //!< Enable the incremental mode.
};
//! Indicate that the builtin includes type is a bitset.
CLINGO_ENABLE_BITSET_ENUM(BuiltinIncludes);

//! A sequences of program parameter vectors to ground and solve incrementally.
using CppClingo::Input::ProgramParamVec;

//! A pair capturing a program `#parts` directive.
using ProgramParams = std::pair<CppClingo::Input::Precedence, std::optional<ProgramParamVec>>;

//! A helper for parsing.
//!
//! This class manages include directives.
class ParseHelper {
  public:
    //! Construct the helper.
    ParseHelper(Logger &log, SymbolStore &store, Input::UnprocessedProgram &prg, ProgramParams &parts,
                Ground::ScriptExec *exec = nullptr, ProgramBackend *prg_backend = nullptr,
                TheoryBackend *thy_backend = nullptr)
        : log_{&log}, store_{&store}, parts_{&parts}, exec_{exec}, parser_{log, store, prg_backend, thy_backend},
          prg_{&prg} {}

    //! Parse a program from the given string.
    void process_string(std::string_view str) {
        parser_.init(str, *store_->string("<string>"));
        process_();
    }

    //! Parse a program from stdin.
    void process_stdin() {
        if (!processed_stdin_) {
            processed_stdin_ = true;
            parser_.init(std::cin, *store_->string("-"));
            process_(root_);
        } else {
            CLINGO_REPORT(*log_, info_file_included) << "file already included: -";
        }
    }

    //! Parse a program from the given path.
    //!
    //! Returns false if the file was not found or raises an error if it was
    //! required.
    auto process_path(std::string_view path) -> bool { return process_path_(path, true); }

    //! Process includes encountered while parsing.
    [[nodiscard]] auto process_includes() -> BuiltinIncludes {
        auto includes = BuiltinIncludes::empty;
        for (; !includes_.empty(); includes_.pop_front()) {
            auto const &[parent, include] = includes_.front();
            if (include.type() == Input::IncludeType::system) {
                auto path = std::filesystem::path(include.value().c_str());
                if (path.is_relative() && parent != root_) {
                    if (process_path_(parent / path, false)) {
                        continue;
                    }
                }
                process_path_(path, true);
            } else {
                if (include.value().view() == "incmode") {
                    includes |= BuiltinIncludes::incmode;
                } else {
                    parse_error_ = true;
                    CLINGO_REPORT_LOC(*log_, error, include.loc()) << "unknown include: " << include.value();
                }
            }
        }
        return includes;
    }

    //! Throws if there was an error during parsing.
    void check() const {
        if (parse_error_) {
            throw parse_error();
        }
    }

  private:
    //! Scan statements.
    void process_() { process_(root_); }

    //! Parse a program from the given path.
    //!
    //! Returns false if the file was not found or raises an error if it was
    //! required.
    auto process_path_(std::filesystem::path path, bool required) -> bool {
        if (std::filesystem::exists(path)) {
            path = std::filesystem::canonical(path);
            auto rel = path.root_name() == root_.root_name() ? path.lexically_relative(root_) : path;
            if (!std::filesystem::is_directory(path)) {
                if (seen_.emplace(path).second) {
                    fin_.open(rel);
                    parser_.init(fin_, *store_->string(rel.string()));
                    process_(path.parent_path());
                } else {
                    CLINGO_REPORT(*log_, info_file_included) << "file already included: " << rel;
                }
                return true;
            }
            if (required) {
                CLINGO_REPORT(*log_, error) << "cannot include directory: " << rel;
                parse_error_ = true;
            }
            return false;
        }
        if (required) {
            CLINGO_REPORT(*log_, error) << "file not found: " << path;
            parse_error_ = true;
        }
        return false;
    }

    // NOLINTBEGIN(cppcoreguidelines-missing-std-forward,bugprone-unchecked-optional-access)
    //! Scan statements.
    void process_(std::filesystem::path const &dir) {
        prg_->ensure_base();
        while (true) {
            auto [stm, res] = parser_.scan();
            parse_error_ = parse_error_ || !res;
            if (!stm) {
                fin_.close();
                break;
            }
            if (auto *parts = std::get_if<Input::StmParts>(&*stm); parts != nullptr) {
                if (!parts_->second || parts_->first < parts->type()) {
                    parts_->second.emplace(parts->elems());
                    parts_->first = parts->type();
                } else if (parts_->first == parts->type()) {
                    CLINGO_REPORT_LOC(*log_, error, parts->loc())
                        << "multiple parts directives with the same precedence: " << *stm;
                    parse_error_ = true;
                }
            } else if (auto *include = std::get_if<Input::StmInclude>(&*stm); include != nullptr) {
                includes_.emplace_back(dir, std::move(*include));
            } else {
                if (auto *script = std::get_if<Input::StmScript>(&*stm); exec_ != nullptr && script != nullptr) {
                    exec_->exec(script->loc(), *log_, script->type().view(), script->value().view());
                }
                prg_->add(*store_, *std::move(stm));
            }
        }
    }
    // NOLINTEND(cppcoreguidelines-missing-std-forward,bugprone-unchecked-optional-access)

    Logger *log_;
    SymbolStore *store_;
    ProgramParams *parts_;
    Ground::ScriptExec *exec_;
    std::ifstream fin_;
    Input::Parser parser_;
    Input::UnprocessedProgram *prg_;
    std::filesystem::path root_ = std::filesystem::current_path();
    std::deque<std::pair<std::filesystem::path, Input::StmInclude>> includes_;
    Util::unordered_set<std::filesystem::path> seen_;
    bool processed_stdin_ = false;
    bool parse_error_ = false;
};

//! @}

} // namespace CppClingo::Control
