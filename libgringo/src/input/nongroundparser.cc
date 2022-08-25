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

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE
#include "potassco/basic_types.h"
#endif
#include <cstdlib>
#ifdef __USE_GNU
#  include <libgen.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#endif
#include "gringo/input/nongroundparser.hh"
#include "gringo/input/groundtermparser.hh"
#include "gringo/lexerstate.hh"
#include "gringo/symbol.hh"
#include "gringo/logger.hh"
#include "gringo/hash_set.hh"
#include "input/nongroundgrammar/grammar.hh"
#include <cstddef>
#include <climits>
#include <memory>
#include <fstream>
#include <vector>
#include <algorithm>


namespace Gringo { namespace Input {

namespace {

struct Free {
    void operator ()(char *ptr) const {
        free(ptr); // NOLINT(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
    }
};

template <typename T>
void report_included(T const &loc, char const *filename, Logger &log) {
    GRINGO_REPORT(log, Warnings::FileIncluded) << loc << ": warning: already included file:\n"
        << "  " << filename << "\n";
}

template <typename T>
void report_not_found(T const &loc, char const *filename, Logger &log) {
    GRINGO_REPORT(log, Warnings::RuntimeError) << loc << ": error: file could not be opened:\n"
        << "  " << filename << "\n";
}

// NOTE: is there a better way?
#ifdef __USE_GNU

bool is_relative(std::string const &filename) {
    return filename.compare(0, 1, "/", 1) != 0;
}

bool check_relative(std::string const &filename, std::string path, std::pair<std::string, std::string> &ret) {
    struct stat sb; // NOLINT(cppcoreguidelines-pro-type-member-init)
    if (!path.empty()) {
        path.push_back('/');
    }
    path.append(filename);
    if (stat(path.c_str(), &sb) != -1) {
        if ((sb.st_mode & S_IFMT) == S_IFIFO) {
            ret = {path, path};
            return true;
        }
        std::unique_ptr<char, Free> x{canonicalize_file_name(path.c_str())};
        if (x) {
            ret = {x.get(), path};
            return true;
        }
    }
    return false;
}

std::string get_dir(std::string const &filename) {
    std::unique_ptr<char, Free> x(strdup(filename.c_str()));
    std::string path = dirname(x.get());
    if (path == ".") { path.clear(); }
    return path;
}

#else

bool is_relative(std::string const &) {
    return true;
}

bool check_relative(std::string const &filename, std::string path, std::pair<std::string, std::string> &ret) {
#if defined _WIN32 || defined __WIN32__ || defined __EMX__ || defined __DJGPP__
    char slash = '\\';
#else
    char slash = '/';
#endif
    if (!path.empty()) { path.push_back(slash); }
    path.append(filename);
    if (std::ifstream(path).good()) {
        ret = {path, path};
        return true;
    }
    return false;
}

std::string get_dir(std::string const &filename) {
#if defined _WIN32 || defined __WIN32__ || defined __EMX__ || defined __DJGPP__
    const char *SLASH = "/\\";
#else
    const char *SLASH = "/";
#endif
    size_t slash = filename.find_last_of(SLASH);
    return slash != std::string::npos
        ? filename.substr(0, slash)
        : "";
}

#endif

template<typename Out>
void split(char const *s, char delim, Out result) {
    std::istringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = std::move(item);
    }
}

std::string check_file(std::string const &filename) {
    if (filename == "-") { return filename; }
    std::pair<std::string, std::string> ret = {"", ""};
    if (check_relative(filename, "", ret)) { return ret.first; }
    return "";
}

std::pair<std::string, std::string> check_file(std::string const &filename, std::string const &source) {
    std::pair<std::string, std::string> ret = {"", ""};
    if (check_relative(filename, "", ret)) {
        return ret;
    }
    if (is_relative(filename)) {
        auto path = get_dir(source);
        if (!path.empty() && check_relative(filename, path, ret)) {
            return ret;
        }
    }
#   include "input/clingopath.hh"
    std::vector<std::string> e_paths;
    std::vector<std::string> const *paths = &g_paths;
    if (char *env_paths = getenv("CLINGOPATH")) {
        split(env_paths, ':', std::back_inserter(e_paths));
        paths = &e_paths;
    }
    for (auto const &path : *paths) {
        if (check_relative(filename, path, ret)) {
            return ret;
        }
    }
    return ret;
}

void format_(std::ostringstream &out) {
}

template <class T>
void format_(std::ostringstream &out, T const &x) {
    out << x;
}

void format_(std::ostringstream &out, StringSpan const &x) {
    out << std::string(begin(x), end(x));
}

template <class T, class... Args>
void format_(std::ostringstream &out, T const &x, Args const &... args) {
    format_(out, x);
    format_(out, args...);
}

template <class... Args>
std::string format(Args const &... args) {
    std::ostringstream out;
    format_(out, args...);
    return out.str();
}

} // namespace

// {{{ defintion of NonGroundParser

NonGroundParser::NonGroundParser(INongroundProgramBuilder &pb, Backend &bck, bool &incmode)
: incmode_(incmode)
, not_("not")
, pb_(pb)
, bck_{bck}
, injectSymbol_(0)
, filename_("") { }

void NonGroundParser::parseError(Location const &loc, std::string const &msg) {
    GRINGO_REPORT(*log_, Warnings::RuntimeError) << loc << ": error: " << msg << "\n";
}

void NonGroundParser::lexerError(Location const &loc, StringSpan token) {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    GRINGO_REPORT(*log_, Warnings::RuntimeError) << loc << ": error: lexer error, unexpected " << std::string(token.first, token.first + token.size) << "\n";
}

bool NonGroundParser::push(std::string const &filename, bool include) {
    return (include && !empty()) ?
        LexerState::push(filename.c_str(), {filename.c_str(), LexerState::data().second}) :
        LexerState::push(filename.c_str(), {filename.c_str(), {"base", {}}});
}

bool NonGroundParser::push(std::string const &filename, std::unique_ptr<std::istream> in) {
    return LexerState::push(std::move(in), {filename.c_str(), {"base", {}}});
}

void NonGroundParser::pop() { LexerState::pop(); }

String NonGroundParser::filename() const { return LexerState::data().first; }

void NonGroundParser::pushFile(std::string &&file, Logger &log) {
    auto checked = check_file(file);
    if (!checked.empty() && !filenames_.insert(checked).second) {
        report_included("<cmd>", file.c_str(), log);
    }
    else if (checked.empty() || !push(file)) {
        report_not_found("<cmd>", file.c_str(), log);
    }
}

void NonGroundParser::pushStream(std::string &&file, std::unique_ptr<std::istream> in, Logger &log) {
    auto res = filenames_.insert(std::move(file));
    if (!res.second) {
        report_included("<cmd>", res.first->c_str(), log);
    }
    else if (!push(*res.first, std::move(in))) {
        report_not_found("<cmd>", res.first->c_str(), log);
    }
}

void NonGroundParser::pushBlock(std::string const &name, IdVec const &vec, std::string const &block, Logger &log) {
    static_cast<void>(log);
    LexerState::push(gringo_make_unique<std::istringstream>(block), {"<block>", {name.c_str(), vec}});
}

void NonGroundParser::init_() {
    if (!empty()) {
        Location loc(filename(), 1, 1, filename(), 1, 1);
        IdVecUid params = pb_.idvec();
        for (auto const &x : data().second.second) {
            params = pb_.idvec(params, x.first, x.second);
        }
        pb_.block(loc, data().second.first, params);
    }
}

int NonGroundParser::lex(void *pValue, Location &loc) {
    if (injectSymbol_ != 0) {
        auto ret = injectSymbol_;
        injectSymbol_ = 0;
        if (ret == NonGroundGrammar::parser::token::SYNC) {
            pop();
            init_();
        }
        else {
            return ret;
        }
    }
    while (!empty()) {
        int minor = lex_impl(pValue, loc);
        end(loc);
        if (minor != 0) {
            return minor;
        }
        injectSymbol_ = NonGroundGrammar::parser::token::SYNC;
        return injectSymbol_;
    }
    return 0;
}

void NonGroundParser::include(String file, Location const &loc, bool inbuilt, Logger &log) {
    if (inbuilt) {
        if (file == "incmode") {
            if (incmode_) {
                report_included(loc, "<incmode>", log);
            }
            else {
                incmode_ = true;
            }
        }
        else {
            report_not_found(loc, (std::string("<") + file.c_str() + ">").c_str(), log);
        }
    }
    else {
        auto paths = check_file(file.c_str(), loc.beginFilename.c_str());
        if (!paths.first.empty() && !filenames_.insert(paths.first).second) {
            report_included(loc, file.c_str(), log);
        }
        else if (paths.first.empty() || !push(paths.second, true)) {
            report_not_found(loc, file.c_str(), log);
        }
    }
}

bool NonGroundParser::parseDefine(std::string const &define, Logger &log) {
    log_ = &log;
    pushStream("<" + define + ">", gringo_make_unique<std::stringstream>(define), log);
    injectSymbol_ = NonGroundGrammar::parser::token::PARSE_DEF;
    NonGroundGrammar::parser parser(this);
    auto ret = parser.parse();
    filenames_.clear();
    return ret == 0;
}

void NonGroundParser::theoryLexing(TheoryLexing mode) {
   theoryLexing_ = mode;
}

void NonGroundParser::condition(Condition cond) {
    assert(condition_ != yyctheory);
    condition_ = cond;
}

NonGroundParser::Condition NonGroundParser::condition() const {
    if (condition_ == yycnormal) {
        switch (theoryLexing_) {
            case TheoryLexing::Disabled:   { return  yycnormal; }
            case TheoryLexing::Theory:     { return  yyctheory; }
            case TheoryLexing::Definition: { return  yycdefinition; }
        }
    }
    return condition_;
}

void NonGroundParser::start(Location &loc) {
    start();
    loc.beginFilename = filename();
    loc.beginLine     = line();
    loc.beginColumn   = column();
}

Location &NonGroundParser::end(Location &loc) {
    loc.endFilename = filename();
    loc.endLine     = line();
    loc.endColumn   = column();
    return loc;
}

Location &NonGroundParser::eof(Location &loc) {
    start(loc);
    end(loc);
    --loc.beginColumn;
    return loc;
}

ParseResult NonGroundParser::parse(Logger &log) {
    log_ = &log;
    theoryLexing_ = TheoryLexing::Disabled;
    injectSymbol_ = NonGroundGrammar::parser::token::PARSE_LP;
    if (empty()) {
        disable_aspif();
        return ParseResult::Gringo;
    }
    NonGroundGrammar::parser parser(this);
    init_();
    parser.parse();
    filenames_.clear();
    auto ret = condition() == yycaspif ? ParseResult::ASPIF : ParseResult::Gringo;
    disable_aspif();
    return ret;
}

INongroundProgramBuilder &NonGroundParser::builder() { return pb_; }

unsigned NonGroundParser::aggregate(AggregateFunction fun, bool choice, unsigned elems, BoundVecUid bounds) {
    return aggregates_.insert({fun, choice ? 1U : 0U, elems, bounds});
}

unsigned NonGroundParser::aggregate(TheoryAtomUid atom) {
    return aggregates_.insert({AggregateFunction::COUNT, 2, atom, static_cast<BoundVecUid>(0)});
}

HdLitUid NonGroundParser::headaggregate(Location const &loc, unsigned hdaggr) {
    auto aggr = aggregates_.erase(hdaggr);
    switch (aggr.choice) {
        case 1:  return builder().headaggr(loc, aggr.fun, aggr.bounds, CondLitVecUid(aggr.elems));
        case 2:  return builder().headaggr(loc, static_cast<TheoryAtomUid>(aggr.elems));
        default: return builder().headaggr(loc, aggr.fun, aggr.bounds, HdAggrElemVecUid(aggr.elems));
    }
}

BdLitVecUid NonGroundParser::bodyaggregate(BdLitVecUid body, Location const &loc, NAF naf, unsigned bdaggr) {
    auto aggr = aggregates_.erase(bdaggr);
    switch (aggr.choice) {
        case 1:  return builder().bodyaggr(body, loc, naf, aggr.fun, aggr.bounds, CondLitVecUid(aggr.elems));
        case 2:  return builder().bodyaggr(body, loc, naf, static_cast<TheoryAtomUid>(aggr.elems));
        default: return builder().bodyaggr(body, loc, naf, aggr.fun, aggr.bounds, BdAggrElemVecUid(aggr.elems));
    }
}

BoundVecUid NonGroundParser::boundvec(Relation ra, TermUid ta, Relation rb, TermUid tb) {
    auto bound(builder().boundvec());
    auto undef = TermUid(-1);
    if (ta != undef) { builder().boundvec(bound, inv(ra), ta); }
    if (tb != undef) { builder().boundvec(bound, rb, tb); }
    return bound;
}

// NOLINTNEXTLINE
void NonGroundParser::aspif_error_(Location const &loc, char const *msg) {
    std::ostringstream out;
    out << loc << ": error: aspif error, " << msg << "\n";
    throw std::runtime_error(out.str());
}

int NonGroundParser::aspif_(Location &loc) {
    aspif_preamble_(loc);
    bck_.beginStep();
    for (;;) {
        auto stm_type = aspif_unsigned_(loc);
        switch (stm_type) {
            case 0:  { aspif_solve_(loc); return 0; }
            case 1:  { aspif_rule_(loc); break; }
            case 2:  { aspif_minimize_(loc); break; }
            case 3:  { aspif_project_(loc); break; }
            case 4:  { aspif_output_(loc); break; }
            case 5:  { aspif_external_(loc); break; }
            case 6:  { aspif_assumption_(loc); break; }
            case 7:  { aspif_heuristic_(loc); break; }
            case 8:  { aspif_edge_(loc); break; }
            case 9:  { aspif_theory_(loc); break; }
            case 10: { aspif_comment_(loc); break; }
            default: { aspif_error_(loc, format("unsupported statement type: ", stm_type).c_str()); }
        }
    }
    bck_.endStep();
    return 0;
}

void NonGroundParser::aspif_solve_(Location &loc) {
    aspif_nl_(loc);
    aspif_eof_(loc);
}

void NonGroundParser::aspif_rule_(Location &loc) {
    aspif_ws_(loc);
    auto stm_type = Potassco::Head_t(aspif_unsigned_(loc));
    switch (stm_type) {
        case Potassco::Head_t::Choice:
        case Potassco::Head_t::Disjunctive: {
            break;
        }
        default:  {
            aspif_error_(loc, format("unsupported rule type: ", stm_type).c_str());
        }
    }
    aspif_ws_(loc);
    auto num_hd = aspif_unsigned_(loc);
    std::vector<Potassco::Atom_t> hd;
    hd.reserve(num_hd);
    for (uint32_t i = 0; i < num_hd; ++i) {
        aspif_ws_(loc);
        hd.emplace_back(aspif_signed_(loc));
    }
    aspif_ws_(loc);
    auto bd_type = aspif_unsigned_(loc);
    switch (bd_type) {
        case 0: {
            aspif_ws_(loc);
            auto num_bd = aspif_unsigned_(loc);
            std::vector<Potassco::Lit_t> bd;
            bd.reserve(num_bd);
            for (uint32_t i = 0; i < num_bd; ++i) {
                aspif_ws_(loc);
                bd.emplace_back(aspif_signed_(loc));
            }
            bck_.rule(stm_type, make_span(hd), make_span(bd));
            break;
        }
        case 1: {
            throw std::runtime_error("handle weight bodies");
        }
        default:  {
            aspif_error_(loc, format("unsupported body type: ", bd_type).c_str());
        }
    }
    aspif_nl_(loc);
}

void NonGroundParser::aspif_minimize_(Location &loc) {
    throw std::logic_error("implement me: minimize");
}

void NonGroundParser::aspif_project_(Location &loc) {
    throw std::logic_error("implement me: project");
}

Symbol NonGroundParser::aspif_symbol_(std::string const &str) {
    GroundTermParser parser;
    Symbol sym = parser.parse(str, logger());
    if (sym.type() == SymbolType::Special) {
        throw std::runtime_error("parsing failed");
    }
    return sym;
}

void NonGroundParser::aspif_output_(Location &loc) {
    aspif_ws_(loc);
    uint32_t num_str = aspif_unsigned_(loc);
    aspif_ws_(loc);
    auto str_span = aspif_string_(loc, num_str);
    using Potassco::end;
    auto str = std::string(begin(str_span), end(str_span));
    aspif_ws_(loc);
    uint32_t num_cond = aspif_unsigned_(loc);
    std::vector<Potassco::Lit_t> cond;
    cond.reserve(num_cond);
    for (uint32_t i = 0; i < num_cond; ++i) {
        aspif_ws_(loc);
        cond.emplace_back(aspif_signed_(loc));
    }
    bck_.output(aspif_symbol_({begin(str_span), end(str_span)}), make_span(cond));
    aspif_nl_(loc);
}

void NonGroundParser::aspif_external_(Location &loc) {
    throw std::logic_error("implement me: external");
}

void NonGroundParser::aspif_assumption_(Location &loc) {
    throw std::logic_error("implement me: assumption");
}

void NonGroundParser::aspif_heuristic_(Location &loc) {
    throw std::logic_error("implement me: heuristic");
}

void NonGroundParser::aspif_edge_(Location &loc) {
    throw std::logic_error("implement me: edge");
}

void NonGroundParser::aspif_theory_(Location &loc) {
    throw std::logic_error("implement me: theory");
}

void NonGroundParser::aspif_comment_(Location &loc) {
    throw std::logic_error("implement me: comment");
}

// }}}

} } // namespace Input Gringo

#include "input/nongroundlexer.hh"

