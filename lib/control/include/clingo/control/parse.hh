#pragma once

#include <clingo/input/parser.hh>
#include <clingo/input/print.hh>
#include <clingo/input/program.hh>

#include <clingo/ground/script.hh>

#include <clingo/util/enum.hh>
#include <clingo/util/type_traits.hh>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <utility>

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
    ParseHelper(Logger &log, SymbolStore &store, std::function<void(Input::Stm)> cb, Ground::ScriptExec *exec = nullptr,
                ProgramBackend *prg_backend = nullptr, TheoryBackend *thy_backend = nullptr)
        : log_{&log}, store_{&store}, exec_{exec}, parser_{log, store, prg_backend, thy_backend}, cb_{std::move(cb)} {}

    //! Parse a program from the given string.
    auto process_string(std::string_view str) -> BuiltinIncludes {
        parser_.init(str, *store_->string("<string>"));
        process_();
        auto ret = process_includes();
        check();
        return ret;
    }

    //! Parse a program from the given files.
    auto process_files(std::span<std::string_view const> const &files) -> BuiltinIncludes {
        auto ret = BuiltinIncludes::empty;
        if (files.empty()) {
            process_stdin();
            ret |= process_includes();
        }
        for (auto const &file : files) {
            if (file == "-") {
                process_stdin();
            } else {
                process_path(file);
            }
            ret |= process_includes();
        }
        check();
        return ret;
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
        bool ensure_base = true;
        while (true) {
            auto [stm, res] = parser_.scan();
            parse_error_ = parse_error_ || !res;
            if (!stm) {
                fin_.close();
                break;
            }
            std::visit(
                [&]<class T>(T const &val) {
                    if constexpr (Util::matches<T, Input::StmInclude>) {
                        // enqueue include
                        includes_.emplace_back(dir, std::move(val));
                    } else if constexpr (Util::is_among_v<T, Input::StmScript>) {
                        // execute script statements
                        if (exec_ != nullptr) {
                            exec_->exec(val.loc(), *log_, val.type().view(), val.value().view());
                        }
                    } else if constexpr (Util::matches<T, Input::StmProgram>) {
                        // disable base injection
                        ensure_base = false;
                        is_base = val.name() == "base" && val.args().empty();
                    } else if constexpr (!Util::is_among_v<T, Input::StmShowNothing, Input::StmShowSig,
                                                           Input::StmProjectSig, Input::StmDefined, Input::StmConst,
                                                           Input::StmTheory, Input::StmParts, Input::StmComment>) {
                        // inject base part before non-meta statements
                        if (!is_base && ensure_base) {
                            cb_(Input::StmProgram{location(val), store_->string_ref("base"), StringSpan{}});
                            ensure_base = false;
                            is_base = true;
                        }
                    }
                    cb_(*std::move(stm));
                },
                *stm);
        }
    }
    // NOLINTEND(cppcoreguidelines-missing-std-forward,bugprone-unchecked-optional-access)

    Logger *log_;
    SymbolStore *store_;
    Ground::ScriptExec *exec_;
    std::ifstream fin_;
    Input::Parser parser_;
    std::function<void(Input::Stm)> cb_;
    std::filesystem::path root_ = std::filesystem::current_path();
    std::deque<std::pair<std::filesystem::path, Input::StmInclude>> includes_;
    Util::unordered_set<std::filesystem::path> seen_;
    bool processed_stdin_ = false;
    bool parse_error_ = false;
    bool is_base = false;
};

//! @}

} // namespace CppClingo::Control
