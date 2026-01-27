#include "test.hh"

#include <clingo/input/parser.hh>
#include <clingo/core/backend.hh>

#include <sstream>
#include <string>
#include <vector>

namespace CppClingo::Input::Test {

namespace {

// Helper to format comma-separated values
template<typename T>
auto comma_separated(std::span<T const> values) -> std::string {
    std::ostringstream oss;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) oss << ",";
        oss << values[i];
    }
    return oss.str();
}

// Helper to format comma-separated weighted literals
auto comma_separated_weighted(WeightedPrgLitSpan values) -> std::string {
    std::ostringstream oss;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "(" << values[i].first << "," << values[i].second << ")";
    }
    return oss.str();
}

// Test backend that records all calls
class TestBackend : public ProgramBackend {
public:
    explicit TestBackend(std::vector<std::string> &calls) : calls_{calls} {}

private:
    void do_preamble(unsigned major, unsigned minor, unsigned revision, bool incremental) override {
        std::ostringstream oss;
        oss << "preamble(" << major << "," << minor << "," << revision << "," 
            << (incremental ? "incremental" : "non-incremental") << ")";
        calls_.push_back(oss.str());
    }

    void do_begin_step() override {
        calls_.push_back("begin_step");
    }

    void do_end_ground() override {
        calls_.push_back("end_ground");
    }

    void do_end_step() override {
        calls_.push_back("end_step");
    }

    auto do_next_lit() -> prg_lit_t override {
        return ++next_lit_;
    }

    auto do_fact_lit() -> std::optional<prg_lit_t> override {
        return std::nullopt;
    }

    void do_rule(PrgLitSpan head, PrgLitSpan body, bool choice) override {
        std::ostringstream oss;
        oss << "rule(head:[" << comma_separated(head)
            << "], body:[" << comma_separated(body)
            << "], choice:" << (choice ? "true" : "false") << ")";
        calls_.push_back(oss.str());
    }

    void do_bd_aggr(PrgLitSpan head, WeightedPrgLitSpan body, int32_t bound, bool choice) override {
        std::ostringstream oss;
        oss << "bd_aggr(head:[" << comma_separated(head)
            << "], body:[" << comma_separated_weighted(body)
            << "], bound:" << bound << ", choice:" << (choice ? "true" : "false") << ")";
        calls_.push_back(oss.str());
    }

    void do_show_term(Symbol sym, PrgLitSpan body) override {
        std::ostringstream oss;
        oss << "show_term(sym:" << sym << ", body:[" << comma_separated(body) << "])";
        calls_.push_back(oss.str());
    }

    void do_show_term(Symbol sym, prg_id_t id) override {
        std::ostringstream oss;
        oss << "show_term(sym:" << sym << ", id:" << id << ")";
        calls_.push_back(oss.str());
    }

    void do_show_term(prg_id_t id, PrgLitSpan body) override {
        std::ostringstream oss;
        oss << "show_term(id:" << id << ", body:[" << comma_separated(body) << "])";
        calls_.push_back(oss.str());
    }

    void do_show_atom(Symbol sym, prg_lit_t lit) override {
        std::ostringstream oss;
        oss << "show_atom(sym:" << sym << ", lit:" << lit << ")";
        calls_.push_back(oss.str());
    }

    void do_edge(prg_id_t u, prg_id_t v, PrgLitSpan body) override {
        std::ostringstream oss;
        oss << "edge(u:" << u << ", v:" << v << ", body:[" << comma_separated(body) << "])";
        calls_.push_back(oss.str());
    }

    void do_heuristic(prg_lit_t atom, prg_weight_t weight, prg_weight_t prio, HeuristicType type,
                     PrgLitSpan body) override {
        std::ostringstream oss;
        oss << "heuristic(atom:" << atom << ", weight:" << weight << ", prio:" << prio
            << ", type:" << static_cast<int>(type) << ", body:[" << comma_separated(body) << "])";
        calls_.push_back(oss.str());
    }

    void do_external(prg_lit_t atom, ExternalType type) override {
        std::ostringstream oss;
        oss << "external(atom:" << atom << ", type:" << static_cast<int>(type) << ")";
        calls_.push_back(oss.str());
    }

    void do_project(PrgLitSpan atoms) override {
        std::ostringstream oss;
        oss << "project(atoms:[" << comma_separated(atoms) << "])";
        calls_.push_back(oss.str());
    }

    void do_assume(PrgLitSpan literals) override {
        std::ostringstream oss;
        oss << "assume(literals:[" << comma_separated(literals) << "])";
        calls_.push_back(oss.str());
    }

    void do_minimize(prg_weight_t priority, WeightedPrgLitSpan body) override {
        std::ostringstream oss;
        oss << "minimize(priority:" << priority << ", body:[" << comma_separated_weighted(body) << "])";
        calls_.push_back(oss.str());
    }

    std::vector<std::string> &calls_;
    prg_lit_t next_lit_ = 0;
};

// Test theory backend that records all calls
class TestTheoryBackend : public TheoryBackend {
public:
    explicit TestTheoryBackend(std::vector<std::string> &calls) : calls_{calls} {}

private:
    void do_num(prg_id_t id, prg_weight_t num) override {
        std::ostringstream oss;
        oss << "num(" << id << "," << num << ")";
        calls_.push_back(oss.str());
    }

    void do_str(prg_id_t id, std::string_view str) override {
        std::ostringstream oss;
        oss << "str(" << id << "," << str << ")";
        calls_.push_back(oss.str());
    }

    void do_fun(prg_id_t id, prg_id_t name, PrgIdSpan args) override {
        std::ostringstream oss;
        oss << "fun(id:" << id << ", name:" << name << ", args:[" << comma_separated(args) << "])";
        calls_.push_back(oss.str());
    }

    void do_tup(prg_id_t id, TheoryTermTupleType type, PrgIdSpan args) override {
        std::ostringstream oss;
        oss << "tup(id:" << id << ", type:" << static_cast<int>(type) << ", args:[" << comma_separated(args) << "])";
        calls_.push_back(oss.str());
    }

    void do_elem(prg_id_t id, PrgIdSpan terms, PrgLitSpan cond) override {
        std::ostringstream oss;
        oss << "elem(id:" << id << ", terms:[" << comma_separated(terms)
            << "], cond:[" << comma_separated(cond) << "])";
        calls_.push_back(oss.str());
    }

    void do_atom(prg_lit_t atom_or_zero, prg_id_t name, PrgIdSpan elems,
                 std::optional<std::pair<prg_id_t, prg_id_t>> guard) override {
        std::ostringstream oss;
        oss << "atom(atom:" << atom_or_zero << ", name:" << name << ", elems:[" << comma_separated(elems) << "], guard:";
        if (guard) {
            oss << "(" << guard->first << "," << guard->second << ")";
        } else {
            oss << "none";
        }
        oss << ")";
        calls_.push_back(oss.str());
    }

    std::vector<std::string> &calls_;
};

// Helper function to parse aspif input
auto parse(std::string_view input) -> std::vector<std::string> {
    auto log = Logger{[](MessageCode, std::string_view) {}};
    auto store = make_symbol_store(true, false);
    std::vector<std::string> calls;
    auto backend = TestBackend{calls};
    auto theory_backend = TestTheoryBackend{calls};
    auto parser = Parser{log, *store, &backend, &theory_backend};
    
    std::string aspif_input;
    // Check if input already starts with "asp", otherwise prepend it
    if (input.substr(0, 3) == "asp") {
        aspif_input = std::string(input);
    } else {
        aspif_input = "asp 1 0 0\n" + std::string(input);
    }
    std::istringstream iss(aspif_input);
    parser.init(iss, store->string_ref("<test>"));
    parser.scan();
    
    return calls;
}

} // namespace

using SV = std::vector<std::string>;

TEST_CASE("aspif single step", "[input][aspif][single-step]") {
    REQUIRE(parse(R"(1 0 1 1 0 0
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

TEST_CASE("aspif multi step", "[input][aspif][multi-step]") {
    auto calls = parse(R"(asp 1 0 0 incremental
1 0 1 1 0 0
4 1 a 1 1
0
1 0 1 2 0 0
4 1 b 1 2
0
)");
    
    // Check for proper step transitions
    int step_count = 0;
    bool in_step = false;
    bool grounded = false;
    
    for (const auto& call : calls) {
        if (call == "begin_step") {
            REQUIRE_FALSE(in_step);
            in_step = true;
            grounded = false;
            step_count++;
        } else if (call == "end_ground") {
            REQUIRE(in_step);
            REQUIRE_FALSE(grounded);
            grounded = true;
        } else if (call == "end_step") {
            REQUIRE(in_step);
            REQUIRE(grounded);
            in_step = false;
        }
    }
    
    REQUIRE(step_count == 2);
    REQUIRE_FALSE(in_step);
}

TEST_CASE("aspif rule", "[input][aspif][rule]") {
    auto calls = parse(R"(1 0 1 1 0 0
1 0 1 2 0 1 -1
0
)");
    
    // Find rule calls
    bool found_rule1 = false;
    bool found_rule2 = false;
    for (const auto& call : calls) {
        if (call.find("rule(head:[1]") != std::string::npos &&
            call.find("body:[]") != std::string::npos) {
            found_rule1 = true;
        }
        if (call.find("rule(head:[2]") != std::string::npos &&
            call.find("body:[-1]") != std::string::npos) {
            found_rule2 = true;
        }
    }
    REQUIRE(found_rule1);
    REQUIRE(found_rule2);
}

TEST_CASE("aspif external", "[input][aspif][external]") {
    auto calls = parse(R"(5 1 1
0
)");
    
    bool found_external = false;
    for (const auto& call : calls) {
        if (call.find("external(atom:1") != std::string::npos) {
            found_external = true;
        }
    }
    REQUIRE(found_external);
}

TEST_CASE("aspif theory", "[input][aspif][theory]") {
    auto calls = parse(R"(9 1 1 1 p
9 0 2 5
0
)");
    
    bool found_str = false;
    bool found_num = false;
    for (const auto& call : calls) {
        if (call == "str(1,p)") {
            found_str = true;
        }
        if (call == "num(2,5)") {
            found_num = true;
        }
    }
    REQUIRE(found_str);
    REQUIRE(found_num);
}

} // namespace CppClingo::Input::Test
