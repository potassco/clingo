// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright (C) 2013  Roland Kaminski

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// }}}

#ifdef WITH_PYTHON
#  include <Python.h>
#endif
#ifdef WITH_LUA
#  include <lua.h>
#endif
#include <gringo/input/nongroundparser.hh>
#include <gringo/input/groundtermparser.hh>
#include <gringo/input/programbuilder.hh>
#include <gringo/input/program.hh>
#include <gringo/ground/program.hh>
#include <gringo/output/output.hh>
#include <gringo/output/statements.hh>
#include <gringo/logger.hh>
#include <gringo/scripts.hh>
#include <gringo/version.hh>
#include <gringo/control.hh>
#include <climits>
#include <iostream>
#include <stdexcept>
#include <program_opts/application.h>
#include <program_opts/typed_value.h>

struct GringoOptions {
    using Foobar = std::vector<Gringo::Sig>;
    ProgramOptions::StringSeq    defines;
    Gringo::Output::OutputDebug  outputDebug           = Gringo::Output::OutputDebug::NONE;
    Gringo::Output::OutputFormat outputFormat          = Gringo::Output::OutputFormat::INTERMEDIATE;
    bool                         verbose               = false;
    bool                         wNoOperationUndefined = false;
    bool                         wNoAtomUndef          = false;
    bool                         wNoFileIncluded       = false;
    bool                         wNoVariableUnbounded  = false;
    bool                         wNoGlobalVariable     = false;
    bool                         rewriteMinimize       = false;
    bool                        keepFacts             = false;
    Foobar foobar;
};

static inline std::vector<std::string> split(std::string const &source, char const *delimiter = " ", bool keepEmpty = false) {
    std::vector<std::string> results;
    size_t prev = 0;
    size_t next = 0;
    while ((next = source.find_first_of(delimiter, prev)) != std::string::npos) {
        if (keepEmpty || (next - prev != 0)) { results.push_back(source.substr(prev, next - prev)); }
        prev = next + 1;
    }
    if (prev < source.size()) { results.push_back(source.substr(prev)); }
    return results;
}

static inline bool parseFoobar(const std::string& str, GringoOptions::Foobar& foobar) {
    for (auto &x : split(str, ",")) {
        auto y = split(x, "/");
        if (y.size() != 2) { return false; }
        unsigned a;
        if (!bk_lib::string_cast<unsigned>(y[1], a)) { return false; }
        bool sign = !y[0].empty() && y[0][0] == '-';
        if (sign) { y[0] = y[0].substr(1); }
        foobar.emplace_back(y[0].c_str(), a, sign);
    }
    return true;
}

#define LOG if (opts.verbose) std::cerr
struct IncrementalControl : Gringo::Control, Gringo::GringoModule {
    using StringVec = std::vector<std::string>;
    IncrementalControl(Gringo::Output::OutputBase &out, StringVec const &files, GringoOptions const &opts)
    : out(out)
    , scripts(*this)
    , pb(scripts, prg, out, defs, opts.rewriteMinimize)
    , parser(pb)
    , opts(opts) {
        using namespace Gringo;
        out.keepFacts = opts.keepFacts;
        if (opts.wNoOperationUndefined) { logger_.disable(clingo_warning_operation_undefined); }
        if (opts.wNoAtomUndef)          { logger_.disable(clingo_warning_atom_undefined); }
        if (opts.wNoFileIncluded)       { logger_.disable(clingo_warning_file_included); }
        if (opts.wNoVariableUnbounded)  { logger_.disable(clingo_warning_variable_unbounded); }
        if (opts.wNoGlobalVariable)     { logger_.disable(clingo_warning_global_variable); }
        for (auto &x : opts.defines) {
            LOG << "define: " << x << std::endl;
            parser.parseDefine(x, logger_);
        }
        for (auto &x : files) {
            LOG << "file: " << x << std::endl;
            parser.pushFile(std::string(x), logger_);
        }
        if (files.empty()) {
            LOG << "reading from stdin" << std::endl;
            parser.pushFile("-", logger_);
        }
        parse();
    }
    Gringo::Logger &logger() override {
        return logger_;
    }
    void parse() {
        if (!parser.empty()) {
            parser.parse(logger_);
            defs.init(logger_);
            parsed = true;
        }
    }
    void ground(Gringo::Control::GroundVec const &parts, Gringo::Context *context) override {
        // NOTE: it would be cool to have assumptions in the lparse output
        auto exit = Gringo::onExit([this]{ scripts.context = nullptr; });
        scripts.context = context;
        parse();
        if (parsed) {
            LOG << "************** parsed program **************" << std::endl << prg;
            prg.rewrite(defs, logger_);
            LOG << "************* rewritten program ************" << std::endl << prg;
            prg.check(logger_);
            if (logger_.hasError()) {
                throw std::runtime_error("grounding stopped because of errors");
            }
            parsed = false;
        }
        if (!grounded) {
            out.beginStep();
            grounded = true;
        }
        if (!parts.empty()) {
            Gringo::Ground::Parameters params;
            for (auto &x : parts) { params.add(x.first, Gringo::SymVec(x.second)); }
            Gringo::Ground::Program gPrg(prg.toGround(out.data, logger_));
            LOG << "************* intermediate program *************" << std::endl << gPrg << std::endl;
            LOG << "*************** grounded program ***************" << std::endl;
            gPrg.ground(params, scripts, out, false, logger_);
        }
    }
    void add(std::string const &name, Gringo::FWStringVec const &params, std::string const &part) override {
        Gringo::Location loc("<block>", 1, 1, "<block>", 1, 1);
        Gringo::Input::IdVec idVec;
        for (auto &x : params) { idVec.emplace_back(loc, x); }
        parser.pushBlock(name, std::move(idVec), part, logger_);
        parse();
    }
    Gringo::Symbol getConst(std::string const &name) override {
        parse();
        auto ret = defs.defs().find(name.c_str());
        if (ret != defs.defs().end()) {
            bool undefined = false;
            Gringo::Symbol val = std::get<2>(ret->second)->eval(undefined, logger_);
            if (!undefined) { return val; }
        }
        return Gringo::Symbol();
    }
    void load(std::string const &filename) override {
        parser.pushFile(std::string(filename), logger_);
        parse();
    }
    bool blocked() override { return false; }
    Gringo::SolveResult solve(ModelHandler, Assumptions &&ass) override {
        if (!ass.empty()) { std::cerr << "warning: the lparse format does not support assumptions" << std::endl; }
        grounded = false;
        out.endStep(true, logger_);
        out.reset();
        const_cast<Potassco::TheoryData&>(out.data.theory().data()).reset();
        return {Gringo::SolveResult::Unknown, false, false};
    }
    Gringo::SolveIter *solveIter(Assumptions &&) override {
        throw std::runtime_error("solving not supported in gringo");
    }
    void interrupt() override {
        throw std::runtime_error("interrupting not supported in gringo");
    }
    Gringo::SolveFuture *solveAsync(ModelHandler, FinishHandler, Assumptions &&) override { throw std::runtime_error("asynchronous solving not supported"); }
    Gringo::Statistics *getStats() override { throw std::runtime_error("statistics not supported (yet)"); }
    void assignExternal(Gringo::Symbol ext, Potassco::Value_t val) override {
        auto atm = out.find(ext);
        if (atm.second && atm.first->hasUid()) {
            Gringo::Id_t offset = atm.first - atm.second->begin();
            Gringo::Output::External external(Gringo::Output::LiteralId{Gringo::NAF::POS, Gringo::Output::AtomType::Predicate, offset, atm.second->domainOffset()}, val);
            out.output(external);
        }
    }
    Gringo::DomainProxy &getDomain() override { throw std::runtime_error("domain introspection not supported"); }
    Gringo::ConfigProxy &getConf() override { throw std::runtime_error("configuration not supported"); }
    void registerPropagator(Gringo::Propagator &, bool) override { throw std::runtime_error("theory propagators not supported"); }
    void useEnumAssumption(bool) override { }
    bool useEnumAssumption() override { return false; }
    virtual ~IncrementalControl() { }
    Gringo::Symbol parseValue(std::string const &str, Gringo::Logger::Printer, unsigned) override { return termParser.parse(str, logger_); }
    Gringo::Control *newControl(int, char const **, Gringo::Logger::Printer, unsigned) override { throw std::logic_error("new control instances not supported"); }
    Gringo::TheoryData const &theory() const override { return out.data.theoryInterface(); }
    void cleanupDomains() override { }
    void parse(char const *, std::function<void (clingo_ast_t const &)>) override { throw std::logic_error("AST parsing not supported"); }
    void add(std::function<void (std::function<void (clingo_ast_t const &)>)>) override { throw std::logic_error("AST parsing not supported"); }
    Gringo::Backend *backend() override { return out.backend(); }
    Potassco::Atom_t addProgramAtom() override { return out.data.newAtom(); }
    Gringo::Input::GroundTermParser        termParser;
    Gringo::Output::OutputBase            &out;
    Gringo::Scripts                        scripts;
    Gringo::Defines                        defs;
    Gringo::Input::Program                 prg;
    Gringo::Input::NongroundProgramBuilder pb;
    Gringo::Input::NonGroundParser         parser;
    GringoOptions const                   &opts;
    Gringo::Logger                         logger_;
    bool                                   parsed = false;
    bool                                   grounded = false;
};
#undef LOG

static bool parseConst(const std::string& str, std::vector<std::string>& out) {
    out.push_back(str);
    return true;
}

static bool parseWarning(const std::string& str, GringoOptions& out) {
    if (str == "no-atom-undefined")      { out.wNoAtomUndef          = true;  return true; }
    if (str ==    "atom-undefined")      { out.wNoAtomUndef          = false; return true; }
    if (str == "no-file-included")       { out.wNoFileIncluded       = true;  return true; }
    if (str ==    "file-included")       { out.wNoFileIncluded       = false; return true; }
    if (str == "no-operation-undefined") { out.wNoOperationUndefined = true;  return true; }
    if (str ==    "operation-undefined") { out.wNoOperationUndefined = false; return true; }
    if (str == "no-variable-unbounded")  { out.wNoVariableUnbounded  = true;  return true; }
    if (str ==    "variable-unbounded")  { out.wNoVariableUnbounded  = false; return true; }
    if (str == "no-global-variable")     { out.wNoGlobalVariable     = true;  return true; }
    if (str ==    "global-variable")     { out.wNoGlobalVariable     = false; return true; }
    return false;
}

static bool parseText(const std::string&, GringoOptions& out) {
    out.outputFormat = Gringo::Output::OutputFormat::TEXT;
    return true;
}

struct GringoApp : public ProgramOptions::Application {
    using StringSeq = std::vector<std::string>;
    virtual const char* getName() const    { return "gringo"; }
    virtual const char* getVersion() const { return GRINGO_VERSION; }
    virtual void initOptions(ProgramOptions::OptionContext& root) {
        using namespace ProgramOptions;
        grOpts_.defines.clear();
        grOpts_.verbose = false;
        OptionGroup gringo("Gringo Options");
        gringo.addOptions()
            ("text,t", storeTo(grOpts_, parseText)->flag(), "Print plain text format")
            ("const,c", storeTo(grOpts_.defines, parseConst)->composing()->arg("<id>=<term>"), "Replace term occurrences of <id> with <term>")
            ("output,o", storeTo(grOpts_.outputFormat = Gringo::Output::OutputFormat::INTERMEDIATE, values<Gringo::Output::OutputFormat>()
              ("intermediate", Gringo::Output::OutputFormat::INTERMEDIATE)
              ("text", Gringo::Output::OutputFormat::TEXT)
              ("reify", Gringo::Output::OutputFormat::REIFY)
              ("smodels", Gringo::Output::OutputFormat::SMODELS)), "Choose output format:\n"
             "      intermediate: print intermediate format\n"
             "      text        : print plain text format\n"
             "      reify       : print program as reified facts\n"
             "      smodels     : print smodels format\n"
             "                    (only supports basic features)")
            ("output-debug", storeTo(grOpts_.outputDebug = Gringo::Output::OutputDebug::NONE, values<Gringo::Output::OutputDebug>()
              ("none", Gringo::Output::OutputDebug::NONE)
              ("text", Gringo::Output::OutputDebug::TEXT)
              ("translate", Gringo::Output::OutputDebug::TRANSLATE)
              ("all", Gringo::Output::OutputDebug::ALL)), "Print debug information during output:\n"
             "      none     : no additional info\n"
             "      text     : print rules as plain text (prefix %%)\n"
             "      translate: print translated rules as plain text (prefix %%%%)\n"
             "      all      : combines text and translate")
            ("warn,W", storeTo(grOpts_, parseWarning)->arg("<warn>")->composing(), "Enable/disable warnings:\n"
             "      [no-]atom-undefined:        a :- b.\n"
             "      [no-]file-included:         #include \"a.lp\". #include \"a.lp\".\n"
             "      [no-]operation-undefined:   p(1/0).\n"
             "      [no-]variable-unbounded:    $x > 10.\n"
             "      [no-]global-variable:       :- #count { X } = 1, X = 1.")
            ("rewrite-minimize", flag(grOpts_.rewriteMinimize = false), "Rewrite minimize constraints into rules")
            ("keep-facts", flag(grOpts_.keepFacts = false), "Do not remove facts from normal rules")
            ("foobar,@4", storeTo(grOpts_.foobar, parseFoobar), "Foobar")
            ;
        root.add(gringo);
        OptionGroup basic("Basic Options");
        basic.addOptions()
            ("file,f,@2", storeTo(input_)->composing(), "Input files")
            ;
        root.add(basic);
    }
    virtual void validateOptions(const ProgramOptions::OptionContext&, const ProgramOptions::ParsedOptions&, const ProgramOptions::ParsedValues&) { }
    virtual void setup() { }
    static bool parsePositional(std::string const &, std::string& out) {
        out = "file";
        return true;
    }
    virtual ProgramOptions::PosOption getPositional() const { return parsePositional; }

    virtual void printHelp(const ProgramOptions::OptionContext& root) {
        printf("%s version %s\n", getName(), getVersion());
        printUsage();
        ProgramOptions::FileOut out(stdout);
        root.description(out);
        printf("\n");
        printUsage();
    }

    virtual void printVersion() {
        Application::printVersion();
        printf(
            "Configuration: "
#ifdef WITH_PYTHON
            "with Python " PY_VERSION
#else
            "without Python"
#endif
            ", "
#ifdef WITH_LUA
            "with " LUA_RELEASE
#else
            "without Lua"
#endif
            "\n"
            "Copyright (C) Roland Kaminski\n"
            "License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n"
            "Gringo is free software: you are free to change and redistribute it.\n"
            "There is NO WARRANTY, to the extent permitted by law.\n"
            );
        fflush(stdout);
    }

    void ground(Gringo::Output::OutputBase &out) {
        using namespace Gringo;
        IncrementalControl inc(out, input_, grOpts_);
        if (inc.scripts.callable("main")) {
            out.init(true);
            inc.scripts.main(inc);
        }
        else {
            out.init(false);
            Gringo::Control::GroundVec parts;
            parts.emplace_back("base", SymVec{});
            inc.ground(parts, nullptr);
            inc.solve(nullptr, {});
        }
    }

    virtual void run() {
        try {
            using namespace Gringo;
            grOpts_.verbose = verbose() == UINT_MAX;
            Output::OutputPredicates outPreds;
            for (auto &x : grOpts_.foobar) {
                outPreds.emplace_back(Location("<cmd>",1,1,"<cmd>", 1,1), x, false);
            }
            Potassco::TheoryData data;
            data.update();
            Output::OutputBase out(data, std::move(outPreds), std::cout, grOpts_.outputFormat, grOpts_.outputDebug);
            ground(out);
        }
        catch (Gringo::GringoError const &e) {
            std::cerr << e.what() << std::endl;
            throw std::runtime_error("fatal error");
        }
        catch (...) { throw; }
    }
private:
    StringSeq     input_;
    GringoOptions grOpts_;
};

int main(int argc, char **argv) {
    GringoApp app;
    return app.main(argc, argv);
}

