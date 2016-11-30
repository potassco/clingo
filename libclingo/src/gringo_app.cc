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

#include <clingo/script.h>
#include <clingo/incmode.hh>
#include <clingo/control.hh>
#include <clingo/ast.hh>
#include <gringo/input/nongroundparser.hh>
#include <gringo/input/groundtermparser.hh>
#include <gringo/input/programbuilder.hh>
#include <gringo/input/program.hh>
#include <gringo/ground/program.hh>
#include <gringo/output/output.hh>
#include <gringo/output/statements.hh>
#include <gringo/logger.hh>
#include <clingo/scripts.hh>
#include <potassco/application.h>
#include <potassco/program_opts/typed_value.h>
#include <climits>
#include <iostream>
#include <stdexcept>

namespace Gringo {

using StrVec = std::vector<std::string>;

struct GringoOptions {
    using Foobar = std::vector<Sig>;
    StrVec                     defines;
    Output::OutputOptions outputOptions;
    Output::OutputFormat  outputFormat          = Output::OutputFormat::INTERMEDIATE;
    bool                          verbose               = false;
    bool                          wNoOperationUndefined = false;
    bool                          wNoAtomUndef          = false;
    bool                          wNoFileIncluded       = false;
    bool                          wNoVariableUnbounded  = false;
    bool                          wNoGlobalVariable     = false;
    bool                          wNoOther              = false;
    bool                          rewriteMinimize       = false;
    bool                          keepFacts             = false;
    Foobar                        foobar;
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
        if (!Potassco::string_cast<unsigned>(y[1], a)) { return false; }
        bool sign = !y[0].empty() && y[0][0] == '-';
        if (sign) { y[0] = y[0].substr(1); }
        foobar.emplace_back(y[0].c_str(), a, sign);
    }
    return true;
}

#define LOG if (opts.verbose) std::cerr
struct IncrementalControl : Control {
    IncrementalControl(Output::OutputBase &out, StrVec const &files, GringoOptions const &opts)
    : out(out)
    , pb(scripts, prg, out, defs, opts.rewriteMinimize)
    , parser(pb, incmode)
    , opts(opts) {
        using namespace Gringo;
        // TODO: should go where python script is once refactored
        out.keepFacts = opts.keepFacts;
        logger_.enable(Warnings::OperationUndefined, !opts.wNoOperationUndefined);
        logger_.enable(Warnings::AtomUndefined, !opts.wNoAtomUndef);
        logger_.enable(Warnings::VariableUnbounded, !opts.wNoVariableUnbounded);
        logger_.enable(Warnings::FileIncluded, !opts.wNoFileIncluded);
        logger_.enable(Warnings::GlobalVariable, !opts.wNoGlobalVariable);
        logger_.enable(Warnings::Other, !opts.wNoOther);
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
    Logger &logger() override {
        return logger_;
    }
    void parse() {
        if (!parser.empty()) {
            parser.parse(logger_);
            defs.init(logger_);
            parsed = true;
        }
    }
    void ground(Control::GroundVec const &parts, Context *context) override {
        // NOTE: it would be cool to have assumptions in the lparse output
        auto exit = onExit([this]{ scripts.resetContext(); });
        if (context) { scripts.setContext(*context); }
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
            if (!initialized_) {
                initialized_ = true;
                out.init(incremental_);
            }
            out.beginStep();
            grounded = true;
        }
        if (!parts.empty()) {
            Ground::Parameters params;
            for (auto &x : parts) { params.add(x.first, SymVec(x.second)); }
            Ground::Program gPrg(prg.toGround(out.data, logger_));
            LOG << "************* intermediate program *************" << std::endl << gPrg << std::endl;
            LOG << "*************** grounded program ***************" << std::endl;
            gPrg.ground(params, scripts, out, false, logger_);
        }
    }
    void add(std::string const &name, StringVec const &params, std::string const &part) override {
        Location loc("<block>", 1, 1, "<block>", 1, 1);
        Input::IdVec idVec;
        for (auto &x : params) { idVec.emplace_back(loc, x); }
        parser.pushBlock(name, std::move(idVec), part, logger_);
        parse();
    }
    Symbol getConst(std::string const &name) override {
        parse();
        auto ret = defs.defs().find(name.c_str());
        if (ret != defs.defs().end()) {
            bool undefined = false;
            Symbol val = std::get<2>(ret->second)->eval(undefined, logger_);
            if (!undefined) { return val; }
        }
        return Symbol();
    }
    void load(std::string const &filename) override {
        parser.pushFile(std::string(filename), logger_);
        parse();
    }
    bool blocked() override { return false; }
    SolveResult solve(ModelHandler, Assumptions &&ass) override {
        if (!ass.empty()) { std::cerr << "warning: the lparse format does not support assumptions" << std::endl; }
        grounded = false;
        out.endStep(true, logger_);
        out.reset(true);
        return {SolveResult::Unknown, false, false};
    }
    SolveFuture *solveIter(Assumptions &&) override { throw std::runtime_error("error: iterative solving not supported"); }
    SolveFuture *solveRefactored(Assumptions &&ass, clingo_solve_mode_bitset_t) override {
        static DefaultSolveFuture future_;
        out.assume(std::move(ass));
        grounded = false;
        out.endStep(true, logger_);
        out.reset(true);
        return &future_;
    }
    SolveFuture *solveAsync(ModelHandler, FinishHandler, Assumptions &&) override { throw std::runtime_error("error: iterative solving not supported"); }
    void interrupt() override { }
    void *claspFacade() override {
        return nullptr;
    }
    void beginAdd() override {
        parse();
    }
    void add(clingo_ast_statement_t const &stm) override {
        Input::parseStatement(pb, logger_, stm);
    }
    void endAdd() override {
        defs.init(logger_);
    }
    void registerObserver(UBackend prg, bool replace) override {
        out.registerObserver(std::move(prg), replace);
    }
    Potassco::AbstractStatistics *statistics() override { throw std::runtime_error("statistics not supported (yet)"); }
    void assignExternal(Symbol ext, Potassco::Value_t val) override {
        auto atm = out.find(ext);
        if (atm.second && atm.first->hasUid()) {
            Id_t offset = numeric_cast<Id_t>(atm.first - atm.second->begin());
            Output::External external(Output::LiteralId{NAF::POS, Output::AtomType::Predicate, offset, atm.second->domainOffset()}, val);
            out.output(external);
        }
    }
    SymbolicAtoms &getDomain() override { throw std::runtime_error("domain introspection not supported"); }
    ConfigProxy &getConf() override { throw std::runtime_error("configuration not supported"); }
    void registerPropagator(UProp, bool) override { throw std::runtime_error("theory propagators not supported"); }
    void useEnumAssumption(bool) override { }
    bool useEnumAssumption() override { return false; }
    virtual ~IncrementalControl() { }
    Output::DomainData const &theory() const override { return out.data; }
    void cleanupDomains() override { }
    Backend *backend() override { return out.backend(); }
    Potassco::Atom_t addProgramAtom() override { return out.data.newAtom(); }
    Input::GroundTermParser        termParser;
    Output::OutputBase            &out;
    Scripts                        scripts;
    Defines                        defs;
    Input::Program                 prg;
    Input::NongroundProgramBuilder pb;
    Input::NonGroundParser         parser;
    GringoOptions const           &opts;
    Logger                         logger_;
    std::unique_ptr<Input::NongroundProgramBuilder> builder;
    bool                                   incmode = false;
    bool                                   parsed = false;
    bool                                   grounded = false;
    bool initialized_ = false;
    bool incremental_ = true;
};
#undef LOG

static bool parseConst(const std::string& str, std::vector<std::string>& out) {
    out.push_back(str);
    return true;
}

inline void enableAll(GringoOptions& out, bool enable) {
    out.wNoAtomUndef          = !enable;
    out.wNoFileIncluded       = !enable;
    out.wNoOperationUndefined = !enable;
    out.wNoVariableUnbounded  = !enable;
    out.wNoGlobalVariable     = !enable;
    out.wNoOther              = !enable;
}

inline bool parseWarning(const std::string& str, GringoOptions& out) {
    if (str == "none")                     { enableAll(out, false);             return true; }
    if (str == "all")                      { enableAll(out, true);              return true; }
    if (str == "no-atom-undefined")        { out.wNoAtomUndef          = true;  return true; }
    if (str ==    "atom-undefined")        { out.wNoAtomUndef          = false; return true; }
    if (str == "no-file-included")         { out.wNoFileIncluded       = true;  return true; }
    if (str ==    "file-included")         { out.wNoFileIncluded       = false; return true; }
    if (str == "no-operation-undefined")   { out.wNoOperationUndefined = true;  return true; }
    if (str ==    "operation-undefined")   { out.wNoOperationUndefined = false; return true; }
    if (str == "no-variable-unbounded")    { out.wNoVariableUnbounded  = true;  return true; }
    if (str ==    "variable-unbounded")    { out.wNoVariableUnbounded  = false; return true; }
    if (str == "no-global-variable")       { out.wNoGlobalVariable     = true;  return true; }
    if (str ==    "global-variable")       { out.wNoGlobalVariable     = false; return true; }
    if (str == "no-other")                 { out.wNoOther              = true;  return true; }
    if (str ==    "other")                 { out.wNoOther              = false; return true; }
    return false;
}

static bool parseText(const std::string&, GringoOptions& out) {
    out.outputFormat = Output::OutputFormat::TEXT;
    return true;
}

struct GringoApp : public Potassco::Application {
    using StringSeq = std::vector<std::string>;
    virtual const char* getName() const    { return "gringo"; }
    virtual const char* getVersion() const { return CLINGO_VERSION; }
    virtual void initOptions(Potassco::ProgramOptions::OptionContext& root) {
        using namespace Potassco::ProgramOptions;
        grOpts_.defines.clear();
        grOpts_.verbose = false;
        OptionGroup gringo("Gringo Options");
        gringo.addOptions()
            ("text,t", storeTo(grOpts_, parseText)->flag(), "Print plain text format")
            ("const,c", storeTo(grOpts_.defines, parseConst)->composing()->arg("<id>=<term>"), "Replace term occurrences of <id> with <term>")
            ("output,o", storeTo(grOpts_.outputFormat = Output::OutputFormat::INTERMEDIATE, values<Output::OutputFormat>()
              ("intermediate", Output::OutputFormat::INTERMEDIATE)
              ("text", Output::OutputFormat::TEXT)
              ("reify", Output::OutputFormat::REIFY)
              ("smodels", Output::OutputFormat::SMODELS)), "Choose output format:\n"
             "      intermediate: print intermediate format\n"
             "      text        : print plain text format\n"
             "      reify       : print program as reified facts\n"
             "      smodels     : print smodels format\n"
             "                    (only supports basic features)")
            ("output-debug", storeTo(grOpts_.outputOptions.debug = Output::OutputDebug::NONE, values<Output::OutputDebug>()
              ("none", Output::OutputDebug::NONE)
              ("text", Output::OutputDebug::TEXT)
              ("translate", Output::OutputDebug::TRANSLATE)
              ("all", Output::OutputDebug::ALL)), "Print debug information during output:\n"
             "      none     : no additional info\n"
             "      text     : print rules as plain text (prefix %%)\n"
             "      translate: print translated rules as plain text (prefix %%%%)\n"
             "      all      : combines text and translate")
            ("warn,W", storeTo(grOpts_, parseWarning)->arg("<warn>")->composing(), "Enable/disable warnings:\n"
             "      none:                     disable all warnings\n"
             "      all:                      enable all warnings\n"
             "      [no-]atom-undefined:      a :- b.\n"
             "      [no-]file-included:       #include \"a.lp\". #include \"a.lp\".\n"
             "      [no-]operation-undefined: p(1/0).\n"
             "      [no-]variable-unbounded:  $x > 10.\n"
             "      [no-]global-variable:     :- #count { X } = 1, X = 1.\n"
             "      [no-]other:               uncategorized warnings")
            ("rewrite-minimize", flag(grOpts_.rewriteMinimize = false), "Rewrite minimize constraints into rules")
            ("keep-facts", flag(grOpts_.keepFacts = false), "Do not remove facts from normal rules")
            ("reify-sccs", flag(grOpts_.outputOptions.reifySCCs = false), "Calculate SCCs for reified output")
            ("reify-steps", flag(grOpts_.outputOptions.reifySteps = false), "Add step numbers to reified output")
            ("foobar,@4", storeTo(grOpts_.foobar, parseFoobar), "Foobar")
            ;
        root.add(gringo);
        OptionGroup basic("Basic Options");
        basic.addOptions()
            ("file,f,@2", storeTo(input_)->composing(), "Input files")
            ;
        root.add(basic);
    }
    virtual void validateOptions(const Potassco::ProgramOptions::OptionContext&, const Potassco::ProgramOptions::ParsedOptions&, const Potassco::ProgramOptions::ParsedValues&) { }
    virtual void setup() { }
    static bool parsePositional(std::string const &, std::string& out) {
        out = "file";
        return true;
    }
    virtual Potassco::ProgramOptions::PosOption getPositional() const { return parsePositional; }

    virtual void printHelp(const Potassco::ProgramOptions::OptionContext& root) {
        printf("%s version %s\n", getName(), getVersion());
        printUsage();
        Potassco::ProgramOptions::FileOut out(stdout);
        root.description(out);
        printf("\n");
        printUsage();
    }

    virtual void printVersion() {
        char const *py_version = clingo_script_version_(clingo_ast_script_type_python);
        char const *lua_version = clingo_script_version_(clingo_ast_script_type_lua);
        Potassco::Application::printVersion();
        printf("\n");
        printf("libgringo version " CLINGO_VERSION "\n");
        printf("Configuration: %s%s, %s%s\n",
             py_version ? "with Python " : "without Python", py_version ?  py_version : "",
            lua_version ? "with Lua "    : "without Lua",   lua_version ? lua_version : "");
        printf("Copyright (C) Roland Kaminski\n");
        printf("License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n");
        printf("Gringo is free software: you are free to change and redistribute it.\n");
        printf("There is NO WARRANTY, to the extent permitted by law.\n");
        fflush(stdout);
    }

    void ground(Output::OutputBase &out) {
        using namespace Gringo;
        IncrementalControl inc(out, input_, grOpts_);
        if (inc.scripts.callable("main")) {
            inc.incremental_ = true;
            inc.scripts.main(inc);
        }
        else if (inc.incmode) {
            inc.incremental_ = true;
            incmode(inc);
        }
        else {
            Control::GroundVec parts;
            parts.emplace_back("base", SymVec{});
            inc.incremental_ = false;
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
            Output::OutputBase out(data, std::move(outPreds), std::cout, grOpts_.outputFormat, grOpts_.outputOptions);
            ground(out);
        }
        catch (GringoError const &e) {
            std::cerr << e.what() << std::endl;
            throw std::runtime_error("fatal error");
        }
        catch (...) { throw; }
    }
private:
    StringSeq     input_;
    GringoOptions grOpts_;
};

} // namespace Gringo

extern "C" CLINGO_VISIBILITY_DEFAULT int gringo_main_(int argc, char *argv[]) {
    Gringo::GringoApp app;
    return app.main(argc, argv);
}

