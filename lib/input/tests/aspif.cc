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
    std::vector<std::string> calls;

private:
    void do_preamble(unsigned major, unsigned minor, unsigned revision, bool incremental) override {
        std::ostringstream oss;
        oss << "preamble(" << major << "," << minor << "," << revision << "," 
            << (incremental ? "incremental" : "non-incremental") << ")";
        calls.push_back(oss.str());
    }

    void do_begin_step() override {
        calls.push_back("begin_step");
    }

    void do_end_ground() override {
        calls.push_back("end_ground");
    }

    void do_end_step() override {
        calls.push_back("end_step");
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
        calls.push_back(oss.str());
    }

    void do_bd_aggr(PrgLitSpan head, WeightedPrgLitSpan body, int32_t bound, bool choice) override {
        std::ostringstream oss;
        oss << "bd_aggr(head:[" << comma_separated(head)
            << "], body:[" << comma_separated_weighted(body)
            << "], bound:" << bound << ", choice:" << (choice ? "true" : "false") << ")";
        calls.push_back(oss.str());
    }

    void do_show_term(Symbol sym, PrgLitSpan body) override {
        std::ostringstream oss;
        oss << "show_term(sym:" << sym << ", body:[" << comma_separated(body) << "])";
        calls.push_back(oss.str());
    }

    void do_show_term(Symbol sym, prg_id_t id) override {
        std::ostringstream oss;
        oss << "show_term(sym:" << sym << ", id:" << id << ")";
        calls.push_back(oss.str());
    }

    void do_show_term(prg_id_t id, PrgLitSpan body) override {
        std::ostringstream oss;
        oss << "show_term(id:" << id << ", body:[" << comma_separated(body) << "])";
        calls.push_back(oss.str());
    }

    void do_show_atom(Symbol sym, prg_lit_t lit) override {
        std::ostringstream oss;
        oss << "show_atom(sym:" << sym << ", lit:" << lit << ")";
        calls.push_back(oss.str());
    }

    void do_edge(prg_id_t u, prg_id_t v, PrgLitSpan body) override {
        std::ostringstream oss;
        oss << "edge(u:" << u << ", v:" << v << ", body:[" << comma_separated(body) << "])";
        calls.push_back(oss.str());
    }

    void do_heuristic(prg_lit_t atom, prg_weight_t weight, prg_weight_t prio, HeuristicType type,
                     PrgLitSpan body) override {
        std::ostringstream oss;
        oss << "heuristic(atom:" << atom << ", weight:" << weight << ", prio:" << prio
            << ", type:" << static_cast<int>(type) << ", body:[" << comma_separated(body) << "])";
        calls.push_back(oss.str());
    }

    void do_external(prg_lit_t atom, ExternalType type) override {
        std::ostringstream oss;
        oss << "external(atom:" << atom << ", type:" << static_cast<int>(type) << ")";
        calls.push_back(oss.str());
    }

    void do_project(PrgLitSpan atoms) override {
        std::ostringstream oss;
        oss << "project(atoms:[" << comma_separated(atoms) << "])";
        calls.push_back(oss.str());
    }

    void do_assume(PrgLitSpan literals) override {
        std::ostringstream oss;
        oss << "assume(literals:[" << comma_separated(literals) << "])";
        calls.push_back(oss.str());
    }

    void do_minimize(prg_weight_t priority, WeightedPrgLitSpan body) override {
        std::ostringstream oss;
        oss << "minimize(priority:" << priority << ", body:[" << comma_separated_weighted(body) << "])";
        calls.push_back(oss.str());
    }

    prg_lit_t next_lit_ = 0;
};

// Test theory backend that records all calls
class TestTheoryBackend : public TheoryBackend {
public:
    std::vector<std::string> calls;

private:
    void do_num(prg_id_t id, prg_weight_t num) override {
        std::ostringstream oss;
        oss << "num(" << id << "," << num << ")";
        calls.push_back(oss.str());
    }

    void do_str(prg_id_t id, std::string_view str) override {
        std::ostringstream oss;
        oss << "str(" << id << "," << str << ")";
        calls.push_back(oss.str());
    }

    void do_fun(prg_id_t id, prg_id_t name, PrgIdSpan args) override {
        std::ostringstream oss;
        oss << "fun(id:" << id << ", name:" << name << ", args:[" << comma_separated(args) << "])";
        calls.push_back(oss.str());
    }

    void do_tup(prg_id_t id, TheoryTermTupleType type, PrgIdSpan args) override {
        std::ostringstream oss;
        oss << "tup(id:" << id << ", type:" << static_cast<int>(type) << ", args:[" << comma_separated(args) << "])";
        calls.push_back(oss.str());
    }

    void do_elem(prg_id_t id, PrgIdSpan terms, PrgLitSpan cond) override {
        std::ostringstream oss;
        oss << "elem(id:" << id << ", terms:[" << comma_separated(terms)
            << "], cond:[" << comma_separated(cond) << "])";
        calls.push_back(oss.str());
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
        calls.push_back(oss.str());
    }
};

} // namespace

TEST_CASE("aspif single step", "[input][aspif][single-step]") {
    auto log = Logger{[](MessageCode, std::string_view) {}};
    auto store = make_symbol_store(true, false);
    auto backend = TestBackend{};
    auto theory_backend = TestTheoryBackend{};
    auto parser = Parser{log, *store, &backend, &theory_backend};

    // Simple aspif with single step
    // asp 1 0 0: aspif version 1.0.0
    // 1 0 1 1 0 0: normal rule, head=[1], body=[]
    // 4 1 a 1 1: show atom "a" when literal 1 is true
    // 0: step terminator
    std::string input = R"(asp 1 0 0
1 0 1 1 0 0
4 1 a 1 1
0
)";

    std::istringstream iss(input);
    parser.init(iss, store->string_ref("<test>"));
    auto [stm, res] = parser.scan();
    
    REQUIRE(res);
    REQUIRE(!stm);

    // Check backend calls
    REQUIRE(backend.calls.size() >= 3);
    REQUIRE(backend.calls[0] == "preamble(1,0,0,non-incremental)");

    // Check for begin_step, end_ground, end_step sequence
    bool found_begin = false;
    bool found_end_ground = false;
    bool found_end_step = false;
    for (const auto& call : backend.calls) {
        if (call == "begin_step") {
            found_begin = true;
            REQUIRE_FALSE(found_end_ground);  // begin_step before end_ground
            REQUIRE_FALSE(found_end_step);    // begin_step before end_step
        } else if (call == "end_ground") {
            found_end_ground = true;
            REQUIRE(found_begin);             // end_ground after begin_step
            REQUIRE_FALSE(found_end_step);    // end_ground before end_step
        } else if (call == "end_step") {
            found_end_step = true;
            REQUIRE(found_begin);             // end_step after begin_step
            REQUIRE(found_end_ground);        // end_step after end_ground
        }
    }
    REQUIRE(found_begin);
    REQUIRE(found_end_ground);
    REQUIRE(found_end_step);
}

TEST_CASE("aspif multi step", "[input][aspif][multi-step]") {
    auto log = Logger{[](MessageCode, std::string_view) {}};
    auto store = make_symbol_store(true, false);
    auto backend = TestBackend{};
    auto theory_backend = TestTheoryBackend{};
    auto parser = Parser{log, *store, &backend, &theory_backend};

    // Aspif with multiple steps (multiple 0 separators)
    // asp 1 0 0 incremental: aspif version 1.0.0 in incremental mode
    // First step: rule with head=[1] and output a
    // 0: first step terminator
    // Second step: rule with head=[2] and output b
    // 0: second step terminator
    std::string input = R"(asp 1 0 0 incremental
1 0 1 1 0 0
4 1 a 1 1
0
1 0 1 2 0 0
4 1 b 1 2
0
)";

    std::istringstream iss(input);
    parser.init(iss, store->string_ref("<test>"));
    auto [stm, res] = parser.scan();
    
    REQUIRE(res);
    REQUIRE(!stm);

    // Check that we have the right sequence
    REQUIRE(backend.calls.size() >= 7);
    REQUIRE(backend.calls[0] == "preamble(1,0,0,incremental)");

    // Check for proper step transitions
    int step_count = 0;
    bool in_step = false;
    bool grounded = false;
    
    for (const auto& call : backend.calls) {
        if (call == "begin_step") {
            REQUIRE_FALSE(in_step);  // Should not be in a step already
            in_step = true;
            grounded = false;
            step_count++;
        } else if (call == "end_ground") {
            REQUIRE(in_step);        // Must be in a step
            REQUIRE_FALSE(grounded); // Should not have grounded already
            grounded = true;
        } else if (call == "end_step") {
            REQUIRE(in_step);        // Must be in a step
            REQUIRE(grounded);       // Must have grounded
            in_step = false;
        }
    }
    
    REQUIRE(step_count == 2);  // Two steps for two 0 separators
    REQUIRE_FALSE(in_step);    // Should not be in a step at the end
}

TEST_CASE("aspif rule", "[input][aspif][rule]") {
    auto log = Logger{[](MessageCode, std::string_view) {}};
    auto store = make_symbol_store(true, false);
    auto backend = TestBackend{};
    auto theory_backend = TestTheoryBackend{};
    auto parser = Parser{log, *store, &backend, &theory_backend};

    // Test rules with and without body
    // 1 0 1 1 0 0: fact (rule with head=[1], empty body)
    // 1 0 1 2 0 1 -1: rule with head=[2], body=[-1]
    std::string input = R"(asp 1 0 0
1 0 1 1 0 0
1 0 1 2 0 1 -1
0
)";

    std::istringstream iss(input);
    parser.init(iss, store->string_ref("<test>"));
    auto [stm, res] = parser.scan();
    
    REQUIRE(res);

    // Find rule calls
    bool found_rule1 = false;
    bool found_rule2 = false;
    for (const auto& call : backend.calls) {
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
    auto log = Logger{[](MessageCode, std::string_view) {}};
    auto store = make_symbol_store(true, false);
    auto backend = TestBackend{};
    auto theory_backend = TestTheoryBackend{};
    auto parser = Parser{log, *store, &backend, &theory_backend};

    std::string input = R"(asp 1 0 0
5 1 1
0
)";

    std::istringstream iss(input);
    parser.init(iss, store->string_ref("<test>"));
    auto [stm, res] = parser.scan();
    
    REQUIRE(res);

    // Find external call
    bool found_external = false;
    for (const auto& call : backend.calls) {
        if (call.find("external(atom:1") != std::string::npos) {
            found_external = true;
        }
    }
    REQUIRE(found_external);
}

TEST_CASE("aspif theory", "[input][aspif][theory]") {
    auto log = Logger{[](MessageCode, std::string_view) {}};
    auto store = make_symbol_store(true, false);
    auto backend = TestBackend{};
    auto theory_backend = TestTheoryBackend{};
    auto parser = Parser{log, *store, &backend, &theory_backend};

    // Test theory statements
    // 9 1 1 1 p: theory string term with id=1, value="p"
    // 9 0 2 5: theory number term with id=2, value=5
    std::string input = R"(asp 1 0 0
9 1 1 1 p
9 0 2 5
0
)";

    std::istringstream iss(input);
    parser.init(iss, store->string_ref("<test>"));
    auto [stm, res] = parser.scan();
    
    REQUIRE(res);

    // Check theory backend was called
    REQUIRE(theory_backend.calls.size() >= 2);
    
    bool found_str = false;
    bool found_num = false;
    for (const auto& call : theory_backend.calls) {
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
