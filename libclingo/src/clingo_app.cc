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

#include <clingo/clingo_app.hh>
#include <clingo/script.h>
#include <clasp/parser.h>
#include <climits>

namespace Gringo {

// {{{ declaration of ClingoApp

ClingoApp::ClingoApp() { }

static bool parseConst(const std::string& str, std::vector<std::string>& out) {
    out.push_back(str);
    return true;
}

static bool parseText(const std::string&, ClingoOptions& out) {
    out.outputFormat = Gringo::Output::OutputFormat::TEXT;
    return true;
}

void ClingoApp::initOptions(Potassco::ProgramOptions::OptionContext& root) {
    using namespace Potassco::ProgramOptions;
    BaseType::initOptions(root);
    grOpts_.defines.clear();
    grOpts_.verbose = false;
    OptionGroup gringo("Gringo Options");
    gringo.addOptions()
        ("text", storeTo(grOpts_, parseText)->flag(), "Print plain text format")
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
        ("output-debug", storeTo(grOpts_.outputOptions.debug = Gringo::Output::OutputDebug::NONE, values<Gringo::Output::OutputDebug>()
          ("none", Gringo::Output::OutputDebug::NONE)
          ("text", Gringo::Output::OutputDebug::TEXT)
          ("translate", Gringo::Output::OutputDebug::TRANSLATE)
          ("all", Gringo::Output::OutputDebug::ALL)), "Print debug information during output:\n"
         "      none     : no additional info\n"
         "      text     : print rules as plain text (prefix %%)\n"
         "      translate: print translated rules as plain text (prefix %%%%)\n"
         "      all      : combines text and translate")
        ("warn,W"                   , storeTo(grOpts_, parseWarning)->arg("<warn>")->composing(), "Enable/disable warnings:\n"
         "      none:                     disable all warnings\n"
         "      all:                      enable all warnings\n"
         "      [no-]atom-undefined:      a :- b.\n"
         "      [no-]file-included:       #include \"a.lp\". #include \"a.lp\".\n"
         "      [no-]operation-undefined: p(1/0).\n"
         "      [no-]variable-unbounded:  $x > 10.\n"
         "      [no-]global-variable:     :- #count { X } = 1, X = 1.\n"
         "      [no-]other:               clasp related and uncategorized warnings")
        ("rewrite-minimize"         , flag(grOpts_.rewriteMinimize = false), "Rewrite minimize constraints into rules")
        ("keep-facts"               , flag(grOpts_.keepFacts = false), "Do not remove facts from normal rules")
        ("reify-sccs"               , flag(grOpts_.outputOptions.reifySCCs = false), "Calculate SCCs for reified output")
        ("reify-steps"              , flag(grOpts_.outputOptions.reifySteps = false), "Add step numbers to reified output")
        ("foobar,@4"                , storeTo(grOpts_.foobar, parseFoobar) , "Foobar")
        ;
    root.add(gringo);

    OptionGroup basic("Basic Options");
    basic.addOptions()
        ("mode", storeTo(mode_ = mode_clingo, values<Mode>()
            ("clingo", mode_clingo)
            ("clasp", mode_clasp)
            ("gringo", mode_gringo)),
         "Run in {clingo|clasp|gringo} mode\n")
        ;
    root.add(basic);
}

void ClingoApp::validateOptions(const Potassco::ProgramOptions::OptionContext& root, const Potassco::ProgramOptions::ParsedOptions& parsed, const Potassco::ProgramOptions::ParsedValues& vals) {
    BaseType::validateOptions(root, parsed, vals);
    if (parsed.count("text") > 0) {
        if (parsed.count("output") > 0) {
            error("'--text' and '--output' are mutually exclusive!");
            exit(Clasp::Cli::E_NO_RUN);
        }
        if (parsed.count("mode") > 0 && mode_ != mode_gringo) {
            error("'--text' can only be used with '--mode=gringo'!");
            exit(Clasp::Cli::E_NO_RUN);
        }
        mode_ = mode_gringo;
    }
    if (parsed.count("output") > 0) {
        if (parsed.count("mode") > 0 && mode_ != mode_gringo) {
            error("'--output' can only be used with '--mode=gringo'!");
            exit(Clasp::Cli::E_NO_RUN);
        }
        mode_ = mode_gringo;
    }
}

Clasp::ProblemType ClingoApp::getProblemType() {
    if (mode_ != mode_clasp) return Clasp::Problem_t::Asp;
    return Clasp::ClaspFacade::detectProblemType(getStream());
}
ClingoApp::ClaspOutput* ClingoApp::createOutput(ProblemType f) {
    if (mode_ == mode_gringo) return 0;
    return BaseType::createOutput(f);
}

void ClingoApp::printHelp(const Potassco::ProgramOptions::OptionContext& root) {
    BaseType::printHelp(root);
    printf("\nclingo is part of Potassco: %s\n", "https://potassco.org/clingo");
    printf("Get help/report bugs via : https://potassco.org/support\n");
    fflush(stdout);
}

void ClingoApp::printVersion() {
    char const *py_version = clingo_script_version_(clingo_ast_script_type_python);
    char const *lua_version = clingo_script_version_(clingo_ast_script_type_lua);
    Potassco::Application::printVersion();
    printf("\n");
    printf("libgringo version " CLINGO_VERSION "\n");
    printf("Configuration: %s%s, %s%s\n",
         py_version ? "with Python " : "without Python", py_version ?  py_version : "",
        lua_version ? "with Lua "    : "without Lua",   lua_version ? lua_version : "");
    printf("License: MIT\n");
    printf("\n");
    BaseType::printLibClaspVersion();
}
bool ClingoApp::onModel(Clasp::Solver const& s, Clasp::Model const& m) {
    bool ret = !grd || grd->onModel(m);
    return BaseType::onModel(s, m) && ret;
}
void ClingoApp::shutdown() {
    // TODO: can be removed in future...
    //       or could be bound differently given the new interface...
    if (grd) grd->solveFuture_ = nullptr;
    Clasp::Cli::ClaspAppBase::shutdown();
}
void ClingoApp::onEvent(Clasp::Event const& ev) {
#if CLASP_HAS_THREADS
    Clasp::ClaspFacade::StepReady const *r = Clasp::event_cast<Clasp::ClaspFacade::StepReady>(ev);
    if (r && grd) { grd->onFinish(r->summary->result); }
#endif
    BaseType::onEvent(ev);
}
void ClingoApp::run(Clasp::ClaspFacade& clasp) {
    try {
        using namespace std::placeholders;
        if (mode_ != mode_clasp) {
            ProblemType     pt  = getProblemType();
            Clasp::ProgramBuilder* prg = &clasp.start(claspConfig_, pt);
            grOpts_.verbose = verbose() == UINT_MAX;
            Clasp::Asp::LogicProgram* lp = mode_ != mode_gringo ? static_cast<Clasp::Asp::LogicProgram*>(prg) : 0;
            grd = Gringo::gringo_make_unique<ClingoControl>(g_scripts(), mode_ == mode_clingo, clasp_.get(), claspConfig_, std::bind(&ClingoApp::handlePostGroundOptions, this, _1), std::bind(&ClingoApp::handlePreSolveOptions, this, _1), nullptr, 20);
            grd->parse(claspAppOpts_.input, grOpts_, lp);
            grd->main();
        }
        else {
            ClaspAppBase::run(clasp);
        }
    }
    catch (Gringo::GringoError const &e) {
        std::cerr << e.what() << std::endl;
        throw std::runtime_error("fatal error");
    }
    catch (...) { throw; }
}

// }}}

} // namespace Gringo

extern "C" CLINGO_VISIBILITY_DEFAULT int clingo_main_(int argc, char *argv[]) {
    Gringo::ClingoApp app;
    return app.main(argc, argv);
}

