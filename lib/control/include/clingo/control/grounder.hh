#pragma once

#include <clingo/control/context.hh>
#include <clingo/control/parse.hh>

#include <clingo/input/program.hh>

#include <clingo/core/output.hh>

namespace CppClingo::Control {

//! @addtogroup control
//! @{

//! A grounder for logic programs.
//!
//! Takes care of parsing, grounding, and output.
class Grounder {
  public:
    struct Impl;
    //! Create a grounder object.
    Grounder(Logger &log, SymbolStore &store, Input::RewriteOptions opts, OutputStm &out, bool has_output);
    //! Destroy grounder.
    ~Grounder() noexcept;
    //! Join with the given program.
    void join(Input::UnprocessedProgram const &prg);
    //! Parse a program from the given string.
    auto parse(std::string_view str, Ground::ScriptExec *code = nullptr) -> BuiltinIncludes;
    //! Parse the given files.
    auto parse(std::span<std::string_view const> const &files, Ground::ScriptExec *code = nullptr,
               ProgramBackend *prg = nullptr, TheoryBackend *thy = nullptr) -> BuiltinIncludes;
    //! Define a constant.
    void add_const(String name, Symbol value);
    //! Get the const map.
    auto const_map() -> Input::ConstMap const &;
    //! Ground the program.
    [[nodiscard]] auto ground(ProgramParamVec const &params, Ground::ScriptCallback *context = nullptr) -> bool;
    //! Print per step summaries.
    void print_summary(bool final);
    void accept(Ground::ProfileNode::Visitor const &visit) const;

    //! Output the current unprocessed program.
    void output_unprocessed_program(std::ostream &out);
    //! Output the current program.
    void output_program(std::ostream &out);
    //! Show the given signature.
    void show(Input::SharedSig const &sig);
    //! Mark the given signature as defined.
    void mark_sig(Input::Sig const &sig);

    //! Get the program parts to ground.
    [[nodiscard]] auto get_parts() -> std::optional<Input::StmParts> const &;
    //! Set the program parts to ground.
    void set_parts(std::optional<Input::StmParts> parts);
    //! Get the associated base.
    [[nodiscard]] auto base() -> Ground::Bases &;
    //! Get the associated base.
    [[nodiscard]] auto base() const -> Ground::Bases const &;
    //! Get the contained symbol store.
    [[nodiscard]] auto store() const -> SymbolStore &;
    //! Get the contained symbol store.
    [[nodiscard]] auto log() const -> Logger &;

  private:
    //! Prepare a program for grounding.
    void prepare_();

    std::unique_ptr<Impl> impl_;
};

//! @}

} // namespace CppClingo::Control
