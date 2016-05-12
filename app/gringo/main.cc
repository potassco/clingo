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
    using Foobar = std::vector<Gringo::FWSignature>;
	ProgramOptions::StringSeq   defines;
    Gringo::Output::LparseDebug lparseDebug           = Gringo::Output::LparseDebug::NONE;
    bool                        verbose               = false;
    bool                        text                  = false;
    bool                        lpRewrite             = false;
    bool                        wNoOperationUndefined = false;
    bool                        wNoAtomUndef          = false;
    bool                        wNoFileIncluded       = false;
    bool                        wNoVariableUnbounded  = false;
    bool                        wNoGlobalVariable     = false;
    bool                        rewriteMinimize       = false;
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
        foobar.emplace_back(y[0], a);
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
        if (opts.wNoOperationUndefined) { message_printer()->disable(W_OPERATION_UNDEFINED); }
        if (opts.wNoAtomUndef)          { message_printer()->disable(W_ATOM_UNDEFINED); }
        if (opts.wNoFileIncluded)       { message_printer()->disable(W_FILE_INCLUDED); }
        if (opts.wNoVariableUnbounded)  { message_printer()->disable(W_VARIABLE_UNBOUNDED); }
        if (opts.wNoGlobalVariable)     { message_printer()->disable(W_GLOBAL_VARIABLE); }
        for (auto &x : opts.defines) { 
            LOG << "define: " << x << std::endl;
            parser.parseDefine(x);
        }
        for (auto &x : files) {
            LOG << "file: " << x << std::endl;
            parser.pushFile(std::string(x));
        }
        if (files.empty()) {
            LOG << "reading from stdin" << std::endl;
            parser.pushFile("-");
        }
        parse();
    }
    void parse() {
        if (!parser.empty()) {
            parser.parse();
            defs.init();
            parsed = true;
        }
    }
    virtual void ground(Gringo::Control::GroundVec const &parts, Gringo::Any &&context) { 
        // NOTE: it would be cool to have assumptions in the lparse output
        auto exit = Gringo::onExit([this]{ scripts.context = Gringo::Any(); });
        scripts.context = std::move(context);
        parse();
        if (parsed) {
            LOG << "************** parsed program **************" << std::endl << prg;
            prg.rewrite(defs);
            LOG << "************* rewritten program ************" << std::endl << prg;
            prg.check();
            if (Gringo::message_printer()->hasError()) {
                throw std::runtime_error("grounding stopped because of errors");
            }
            parsed = false;
        }
        if (!grounded) {
            if (incremental) { out.incremental(); }
            grounded = true;
        }
        if (!parts.empty()) {
            Gringo::Ground::Parameters params;
            for (auto &x : parts) { params.add(x.first, x.second); }
            Gringo::Ground::Program gPrg(prg.toGround(out.domains));
            LOG << "************* intermediate program *************" << std::endl << gPrg << std::endl;
            LOG << "*************** grounded program ***************" << std::endl;
            gPrg.ground(params, scripts, out, false);
        }
    }
    virtual void add(std::string const &name, Gringo::FWStringVec const &params, std::string const &part) {
        Gringo::Location loc("<block>", 1, 1, "<block>", 1, 1);
        Gringo::Input::IdVec idVec;
        for (auto &x : params) { idVec.emplace_back(loc, x); }
        parser.pushBlock(name, std::move(idVec), part);
        parse();
    }
    virtual Gringo::Value getConst(std::string const &name) {
        parse();
        auto ret = defs.defs().find(name);
        if (ret != defs.defs().end()) {
            bool undefined = false;
            Gringo::Value val = std::get<2>(ret->second)->eval(undefined);
            if (!undefined) { return val; }
        }
        return Gringo::Value();
    }
    virtual void load(std::string const &filename) {
        parser.pushFile(std::string(filename));
        parse();
    }
    virtual void onModel(Gringo::Model const &) { }
    virtual bool blocked() { return false; }
    virtual void prepareSolve(Assumptions &&ass) {
        if (!ass.empty()) { std::cerr << "warning: the lparse format does not support assumptions" << std::endl; }
    }
    virtual Gringo::SolveResult solve(ModelHandler) {
        if (!grounded) {
            if (incremental) { out.incremental(); }
        }
        grounded = false;
        out.finish();
        return Gringo::SolveResult::UNKNOWN;
    }
    virtual Gringo::SolveIter *solveIter() {
        throw std::runtime_error("solving not supported in gringo");
    }
    virtual Gringo::SolveFuture *solveAsync(ModelHandler, FinishHandler) { throw std::runtime_error("solving not supported in gringo"); }
    virtual Gringo::Statistics *getStats() { throw std::runtime_error("statistics not supported in gringo (yet)"); }
    virtual void assignExternal(Gringo::Value ext, Gringo::TruthValue val) { 
        Gringo::PredicateDomain::element_type *atm = out.find2(ext);
        if (atm && atm->second.hasUid()) {
            out.assignExternal(*atm, val);
        }
    }
    virtual Gringo::DomainProxy &getDomain() { throw std::runtime_error("domain introspection not supported"); }
    virtual Gringo::ConfigProxy &getConf() { throw std::runtime_error("configuration not supported"); }
    virtual void useEnumAssumption(bool) { }
    virtual bool useEnumAssumption() { return false; }
    virtual ~IncrementalControl() { }
    virtual Gringo::Value parseValue(std::string const &str) { return termParser.parse(str); }
    virtual Control *newControl(int, char const **) { throw std::logic_error("creating new control instances not supported in gringo"); }
    virtual void freeControl(Control *) { }
    virtual void cleanupDomains() { }

    Gringo::Input::GroundTermParser        termParser;
    Gringo::Output::OutputBase            &out;
    Gringo::Scripts                        scripts;
    Gringo::Defines                        defs;
    Gringo::Input::Program                 prg;
    Gringo::Input::NongroundProgramBuilder pb;
    Gringo::Input::NonGroundParser         parser;
    GringoOptions const                   &opts;
    bool                                   parsed = false;
    bool                                   grounded = false;
    bool                                   incremental = false;
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

struct GringoApp : public ProgramOptions::Application {
	using StringSeq = std::vector<std::string>;
	virtual const char* getName() const    { return "gringo"; }
	virtual const char* getVersion() const { return GRINGO_VERSION; }
protected:
	virtual void initOptions(ProgramOptions::OptionContext& root) {
        using namespace ProgramOptions;
        grOpts_.defines.clear();
        grOpts_.verbose = false;
        OptionGroup gringo("Gringo Options");
        gringo.addOptions()
            ("text,t"                   , flag(grOpts_.text = false)     , "Print plain text format")
            ("const,c"                  , storeTo(grOpts_.defines, parseConst)->composing()->arg("<id>=<term>"), "Replace term occurrences of <id> with <term>")
            ("lparse-rewrite"           , flag(grOpts_.lpRewrite = false), "Use together with --text to inspect lparse rewriting")
            ("lparse-debug"             , storeTo(grOpts_.lparseDebug = Gringo::Output::LparseDebug::NONE, values<Gringo::Output::LparseDebug>()
              ("none"  , Gringo::Output::LparseDebug::NONE)
              ("plain" , Gringo::Output::LparseDebug::PLAIN)
              ("lparse", Gringo::Output::LparseDebug::LPARSE)
              ("all"   , Gringo::Output::LparseDebug::ALL)), "Debug information during lparse rewriting:\n"
             "      none  : no additional info\n"
             "      plain : print rules as in plain output (prefix %%)\n"
             "      lparse: print rules as in lparse output (prefix %%%%)\n"
             "      all   : combines plain and lparse\n")
            ("warn,W"                   , storeTo(grOpts_, parseWarning)->arg("<warn>")->composing(), "Enable/disable warnings:\n"
             "      [no-]atom-undefined:        a :- b.\n"
             "      [no-]file-included:         #include \"a.lp\". #include \"a.lp\".\n"
             "      [no-]operation-undefined:   p(1/0).\n"
             "      [no-]variable-unbounded:    $x > 10.\n"
             "      [no-]global-variable:       :- #count { X } = 1, X = 1.\n")
            ("rewrite-minimize"         , flag(grOpts_.rewriteMinimize = false), "Rewrite minimize constraints into rules")
            ("keep-facts"               , flag(grOpts_.keepFacts = false), "Do not remove facts from normal rules")
            ("foobar,@4"                , storeTo(grOpts_.foobar, parseFoobar), "Foobar")
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
            inc.incremental = true;
            inc.scripts.main(inc);
        }
        else { 
            Gringo::Control::GroundVec parts;
            parts.emplace_back("base", FWValVec{});
            inc.ground(parts, Gringo::Any());
            inc.prepareSolve({});
            inc.solve(nullptr);
        }
    }

	virtual void run() {
        using namespace Gringo;
        grOpts_.verbose = verbose() == UINT_MAX;
        Output::OutputPredicates outPreds;
        for (auto &x : grOpts_.foobar) {
            outPreds.emplace_back(Location("<cmd>",1,1,"<cmd>", 1,1), x, false);
        }
        if (grOpts_.text) {
            Output::OutputBase out(std::move(outPreds), std::cout, grOpts_.lpRewrite);
            ground(out);
        }
        else {
            Output::PlainLparseOutputter plo(std::cout);
            Output::OutputBase out(std::move(outPreds), plo);
            ground(out);
        }
    }
private:
	StringSeq     input_;
    GringoOptions grOpts_;
};

int main(int argc, char **argv) {
	GringoApp app;
	return app.main(argc, argv);
}

