#include "test.hh" // IWYU pragma: keep

#include <clingo/core/backend.hh>
#include <clingo/input/parser.hh>
#include <clingo/util/print.hh>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace CppClingo::Input::Test {

namespace {

void p_weighted(std::ostream &out, std::pair<prg_lit_t, prg_weight_t> lit) {
    out << "(" << lit.first << "," << lit.second << ")";
}

class LogCall {
  public:
    explicit LogCall(std::vector<std::string> &calls) : calls_{&calls} {}

    template <typename... Args> void log_call_(std::string_view name, Args &&...args) {
        oss_.str("");
        oss_ << name;
        if constexpr (sizeof...(args) > 0) {
            oss_ << "(";
            // NOTE: we can very well cast string literals into pointers
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay)
            (oss_ << ... << std::forward<Args>(args));
            oss_ << ")";
        }
        calls_->emplace_back(oss_.view());
    }

  private:
    std::vector<std::string> *calls_;
    std::ostringstream oss_;
};

// Test backend that records all calls
class TestBackend : public ProgramBackend, private LogCall {
  public:
    explicit TestBackend(std::vector<std::string> &calls) : LogCall{calls} {}

  private:
    void do_preamble(unsigned major, unsigned minor, unsigned revision, bool incremental) override {
        log_call_("preamble", major, ",", minor, ",", revision, ",", (incremental ? "incremental" : "non-incremental"));
    }

    void do_begin_step() override { log_call_("begin_step"); }

    void do_end_ground() override { log_call_("end_ground"); }

    void do_end_step() override { log_call_("end_step"); }

    auto do_next_lit() -> prg_lit_t override { return ++next_lit_; }

    auto do_fact_lit() -> std::optional<prg_lit_t> override { return std::nullopt; }

    void do_rule(PrgLitSpan head, PrgLitSpan body, bool choice) override {
        log_call_("rule", "head:[", Util::p_range(head), "], body:[", Util::p_range(body),
                  "], choice:", (choice ? "true" : "false"));
    }

    void do_bd_aggr(PrgLitSpan head, WeightedPrgLitSpan body, int32_t bound, bool choice) override {
        log_call_("bd_aggr", "head:[", Util::p_range(head), "], body:[", Util::p_range(body, p_weighted),
                  "], bound:", bound, ", choice:", (choice ? "true" : "false"));
    }

    void do_show_term(Symbol sym, PrgLitSpan body) override {
        log_call_("show_term", "sym:", sym, ", body:[", Util::p_range(body), "]");
    }

    void do_show_term(Symbol sym, prg_id_t id) override { log_call_("show_term", "sym:", sym, ", id:", id); }

    void do_show_term(prg_id_t id, PrgLitSpan body) override {
        log_call_("show_term", "id:", id, ", body:[", Util::p_range(body), "]");
    }

    void do_show_atom(Symbol sym, prg_lit_t lit) override { log_call_("show_atom", "sym:", sym, ", lit:", lit); }

    void do_edge(prg_id_t u, prg_id_t v, PrgLitSpan body) override {
        log_call_("edge", "u:", u, ", v:", v, ", body:[", Util::p_range(body), "]");
    }

    void do_heuristic(prg_lit_t atom, prg_weight_t weight, prg_weight_t prio, HeuristicType type,
                      PrgLitSpan body) override {
        log_call_("heuristic", "atom:", atom, ", weight:", weight, ", prio:", prio, ", type:", static_cast<int>(type),
                  ", body:[", Util::p_range(body), "]");
    }

    void do_external(prg_lit_t atom, ExternalType type) override {
        log_call_("external", "atom:", atom, ", type:", static_cast<int>(type));
    }

    void do_project(PrgLitSpan atoms) override { log_call_("project", "atoms:[", Util::p_range(atoms), "]"); }

    void do_assume(PrgLitSpan literals) override { log_call_("assume", "literals:[", Util::p_range(literals), "]"); }

    void do_minimize(prg_weight_t priority, WeightedPrgLitSpan body) override {
        log_call_("minimize", "priority:", priority, ", body:[", Util::p_range(body, p_weighted), "]");
    }

    prg_lit_t next_lit_ = 0;
};

// Test theory backend that records all calls
class TestTheoryBackend : public TheoryBackend, private LogCall {
  public:
    explicit TestTheoryBackend(std::vector<std::string> &calls) : LogCall{calls} {}

  private:
    void do_num(prg_id_t id, prg_weight_t num) override { log_call_("num", id, ",", num); }

    void do_str(prg_id_t id, std::string_view str) override { log_call_("str", id, ",", str); }

    void do_fun(prg_id_t id, prg_id_t name, PrgIdSpan args) override {
        log_call_("fun", "id:", id, ", name:", name, ", args:[", Util::p_range(args), "]");
    }

    void do_tup(prg_id_t id, TheoryTermTupleType type, PrgIdSpan args) override {
        log_call_("tup", "id:", id, ", type:", static_cast<int>(type), ", args:[", Util::p_range(args), "]");
    }

    void do_elem(prg_id_t id, PrgIdSpan terms, PrgLitSpan cond) override {
        log_call_("elem", "id:", id, ", terms:[", Util::p_range(terms), "], cond:[", Util::p_range(cond), "]");
    }

    void do_atom(prg_lit_t atom_or_zero, prg_id_t name, PrgIdSpan elems,
                 std::optional<std::pair<prg_id_t, prg_id_t>> guard) override {
        log_call_("atom", "atom:", atom_or_zero, ", name:", name, ", elems:[", Util::p_range(elems), "], guard:",
                  guard ? "(" + std::to_string(guard->first) + "," + std::to_string(guard->second) + ")" : "none");
    }
};

// Helper function to parse aspif input
auto parse(std::string_view input) -> std::vector<std::string> {
    auto log = Logger{[](MessageCode code, std::string_view msg) {
        std::ignore = code;
        std::ignore = msg;
    }};
    auto store = make_symbol_store(true, false);
    std::vector<std::string> calls;
    auto backend = TestBackend{calls};
    auto theory_backend = TestTheoryBackend{calls};
    auto parser = Parser{log, *store, &backend, &theory_backend};

    std::string aspif_input;
    if (input.starts_with("asp")) {
        aspif_input = std::string(input);
    } else {
        aspif_input = "asp 2 0 0\n" + std::string(input);
    }
    std::istringstream iss(aspif_input);
    parser.init(iss, store->string_ref("<test>"));
    auto [stm, res] = parser.scan();
    REQUIRE(!stm.has_value());
    REQUIRE(res);
    return calls;
}

} // namespace

using SV = std::vector<std::string>;

TEST_CASE("aspif v1 single step", "[input][aspif][single-step]") {
    REQUIRE(parse(R"(asp 1 0 0
1 0 1 1 0 0
4 1 a 1 1
0
)") == SV{
           "preamble(1,0,0,non-incremental)",
           "begin_step",
           "rule(head:[1], body:[], choice:false)",
           "show_atom(sym:a, lit:1)",
           "end_ground",
           "end_step",
       });
}

TEST_CASE("aspif v2 single step", "[input][aspif][single-step]") {
    REQUIRE(parse(R"(1 0 1 1 0 0
4 0 1 1 a
4 1 2 1 b
4 2 2 2 1 3
0
)") == SV{
           "preamble(2,0,0,non-incremental)",
           "begin_step",
           "rule(head:[1], body:[], choice:false)",
           "show_atom(sym:a, lit:1)",
           "show_term(sym:b, id:2)",
           "show_term(id:2, body:[1,3])",
           "end_ground",
           "end_step",
       });
}

TEST_CASE("aspif multi step", "[input][aspif][multi-step]") {
    REQUIRE(parse(R"(asp 2 0 0 incremental
1 0 1 1 0 0
4 0 1 1 a
0
1 0 1 2 0 0
4 0 2 1 b
0
)") == SV{
           "preamble(2,0,0,incremental)",
           "begin_step",
           "rule(head:[1], body:[], choice:false)",
           "show_atom(sym:a, lit:1)",
           "end_ground",
           "end_step",
           "begin_step",
           "rule(head:[2], body:[], choice:false)",
           "show_atom(sym:b, lit:2)",
           "end_ground",
           "end_step",
       });
}

TEST_CASE("aspif rule", "[input][aspif][rule]") {
    REQUIRE(parse(R"(1 0 1 1 0 0
1 0 1 2 0 1 -1
0
)") == SV{
           "preamble(2,0,0,non-incremental)",
           "begin_step",
           "rule(head:[1], body:[], choice:false)",
           "rule(head:[2], body:[-1], choice:false)",
           "end_ground",
           "end_step",
       });
}

TEST_CASE("aspif choice rule", "[input][aspif][choice]") {
    REQUIRE(parse(R"(1 1 2 1 2 0 0
0
)") == SV{
           "preamble(2,0,0,non-incremental)",
           "begin_step",
           "rule(head:[1,2], body:[], choice:true)",
           "end_ground",
           "end_step",
       });
}

TEST_CASE("aspif minimize", "[input][aspif][minimize]") {
    REQUIRE(parse(R"(2 0 2 1 1 2 2
0
)") == SV{
           "preamble(2,0,0,non-incremental)",
           "begin_step",
           "minimize(priority:0, body:[(1,1),(2,2)])",
           "end_ground",
           "end_step",
       });
}

TEST_CASE("aspif project", "[input][aspif][project]") {
    REQUIRE(parse(R"(3 2 1 2
0
)") == SV{
           "preamble(2,0,0,non-incremental)",
           "begin_step",
           "project(atoms:[1,2])",
           "end_ground",
           "end_step",
       });
}

TEST_CASE("aspif external", "[input][aspif][external]") {
    REQUIRE(parse(R"(5 1 1
0
)") == SV{
           "preamble(2,0,0,non-incremental)",
           "begin_step",
           "external(atom:1, type:1)",
           "end_ground",
           "end_step",
       });
}

TEST_CASE("aspif assume", "[input][aspif][assume]") {
    REQUIRE(parse(R"(6 2 1 123
0
)") == SV{
           "preamble(2,0,0,non-incremental)",
           "begin_step",
           "assume(literals:[1,123])",
           "end_ground",
           "end_step",
       });
}

TEST_CASE("aspif edge", "[input][aspif][edge]") {
    REQUIRE(parse(R"(8 1 2 0
0
)") == SV{
           "preamble(2,0,0,non-incremental)",
           "begin_step",
           "edge(u:1, v:2, body:[])",
           "end_ground",
           "end_step",
       });
}

TEST_CASE("aspif theory num", "[input][aspif][theory][num]") {
    REQUIRE(parse(R"(9 0 1 42
0
)") == SV{
           "preamble(2,0,0,non-incremental)",
           "begin_step",
           "num(1,42)",
           "end_ground",
           "end_step",
       });
}

TEST_CASE("aspif theory str", "[input][aspif][theory][str]") {
    REQUIRE(parse(R"(9 1 1 1 p
0
)") == SV{
           "preamble(2,0,0,non-incremental)",
           "begin_step",
           "str(1,p)",
           "end_ground",
           "end_step",
       });
}

TEST_CASE("aspif theory fun", "[input][aspif][theory][fun]") {
    REQUIRE(parse(R"(9 1 2 1 f
9 0 3 10
9 0 4 20
9 2 5 2 2 3 4
0
)") == SV{
           "preamble(2,0,0,non-incremental)",
           "begin_step",
           "str(2,f)",
           "num(3,10)",
           "num(4,20)",
           "fun(id:5, name:2, args:[3,4])",
           "end_ground",
           "end_step",
       });
}

TEST_CASE("aspif theory tup", "[input][aspif][theory][tup]") {
    REQUIRE(parse(R"(9 0 3 10
9 0 4 20
9 2 5 -1 2 3 4
0
)") == SV{
           "preamble(2,0,0,non-incremental)",
           "begin_step",
           "num(3,10)",
           "num(4,20)",
           "tup(id:5, type:0, args:[3,4])",
           "end_ground",
           "end_step",
       });
}

TEST_CASE("aspif theory elem", "[input][aspif][theory][elem]") {
    REQUIRE(parse(R"(9 0 1 5
9 4 2 1 1 0
0
)") == SV{
           "preamble(2,0,0,non-incremental)",
           "begin_step",
           "num(1,5)",
           "elem(id:2, terms:[1], cond:[])",
           "end_ground",
           "end_step",
       });
}

TEST_CASE("aspif theory atom", "[input][aspif][theory][atom]") {
    REQUIRE(parse(R"(9 1 1 1 p
9 4 2 1 1 0
9 5 0 1 1 2
0
)") == SV{
           "preamble(2,0,0,non-incremental)",
           "begin_step",
           "str(1,p)",
           "elem(id:2, terms:[1], cond:[])",
           "atom(atom:0, name:1, elems:[2], guard:none)",
           "end_ground",
           "end_step",
       });
}

TEST_CASE("aspif weight constraint", "[input][aspif][weight-constraint]") {
    REQUIRE(parse(R"(1 0 1 3 1 2 1 5 2
0
)") == SV{
           "preamble(2,0,0,non-incremental)",
           "begin_step",
           "bd_aggr(head:[3], body:[(5,2)], bound:2, choice:false)",
           "end_ground",
           "end_step",
       });
}

TEST_CASE("aspif heuristic", "[input][aspif][heuristic]") {
    REQUIRE(parse(R"(7 1 3 10 0 1 2
0
)") == SV{
           "preamble(2,0,0,non-incremental)",
           "begin_step",
           "heuristic(atom:3, weight:10, prio:0, type:1, body:[2])",
           "end_ground",
           "end_step",
       });
}

TEST_CASE("aspif theory atom with guard", "[input][aspif][theory][atom-with-guard]") {
    REQUIRE(parse(R"(9 1 1 2 op
9 0 2 5
9 1 3 1 p
9 4 4 1 3 0
9 6 0 1 1 4 1 2
0
)") == SV{
           "preamble(2,0,0,non-incremental)",
           "begin_step",
           "str(1,op)",
           "num(2,5)",
           "str(3,p)",
           "elem(id:4, terms:[3], cond:[])",
           "atom(atom:0, name:1, elems:[4], guard:(1,2))",
           "end_ground",
           "end_step",
       });
}

} // namespace CppClingo::Input::Test
