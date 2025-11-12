#include <clingo/control.hh>
#include <clingo/theory.hh>

#include <catch2/catch_test_macros.hpp>

#include <array>

namespace Clingo::Test {

namespace {

struct Data {
    Library lib;
    bool prepared = false;
    bool registered = false;
    size_t models = 0;

    [[nodiscard]] auto sym(int number) const -> Symbol { return Function(lib, "a", {Number(number)}); }
};

struct TestTheory {
    static auto info([[maybe_unused]] void *self, clingo_string_t *name, int *major, int *minor, int *revision)
        -> bool {
        name->data = "TestTheory";
        name->size = std::strlen(name->data);
        *major = 1;
        *minor = 0;
        *revision = 0;
        return true;
    }
    static void destroy(void *self) { std::ignore = std::unique_ptr<Data>{static_cast<Data *>(self)}; }
    static auto register_theory([[maybe_unused]] void *self, [[maybe_unused]] clingo_control_t *control) -> bool {
        auto *data = static_cast<Data *>(self);
        data->registered = true;
        return true;
    }
    static auto rewrite_ast([[maybe_unused]] void *self, clingo_ast_t *statement, clingo_theory_ast_callback_t callback,
                            void *data) -> bool {
        return callback(statement, data);
    }
    static auto prepare(void *self, [[maybe_unused]] clingo_control_t *control) -> bool {
        auto *data = static_cast<Data *>(self);
        data->prepared = true;
        return true;
    }
    static auto register_options([[maybe_unused]] void *self, [[maybe_unused]] clingo_options_t *options) -> bool {
        // NOTE: somewhat hard to test but also not very important
        return true;
    }
    static auto validate_options([[maybe_unused]] void *self) -> bool {
        // NOTE: somewhat hard to test but also not very important
        return true;
    }
    static auto on_model([[maybe_unused]] void *self, clingo_model_t *model) -> bool {
        CLINGO_TRY {
            auto *data = static_cast<Data *>(self);
            ++data->models;
            auto mdl = Model{model};
            mdl.extend(std::to_array({Number(42)}));
        }
        CLINGO_CATCH;
    }
    static auto on_stats([[maybe_unused]] void *self, clingo_stats_t *stats) -> bool {
        CLINGO_TRY {
            uint64_t root = 0;
            Detail::handle_error(clingo_stats_root(stats, &root));
            auto st = Stats{stats, root};
            auto step = st.map().insert("user_step", StatsType::map).map();
            step.insert("test_key", StatsType::value).value(3.14);
        }
        CLINGO_CATCH;
    }
    static auto lookup_symbol(void *self, clingo_symbol_t symbol, size_t *index, bool *found) -> bool {
        CLINGO_TRY {
            auto *data = static_cast<Data *>(self);
            if (data->sym(1) == Symbol{symbol, true}) {
                *index = 0;
                *found = true;
            } else if (data->sym(2) == Symbol{symbol, true}) {
                *index = 1;
                *found = true;
            } else if (data->sym(3) == Symbol{symbol, true}) {
                *index = 2;
                *found = true;
            } else {
                *found = false;
            }
        }
        CLINGO_CATCH;
    }
    static auto assignment_next([[maybe_unused]] void *self, [[maybe_unused]] uint32_t thread_id, bool *init,
                                size_t *index, bool *has_value) -> bool {
        CLINGO_TRY {
            if (std::exchange(*init, false)) {
                *index = 0;
            } else {
                ++*index;
            }
            *has_value = *index < 3;
        }
        CLINGO_CATCH;
    }
    static auto assignment_get_value(void *self, [[maybe_unused]] uint32_t thread_id, size_t index,
                                     clingo_symbol_t *symbol, clingo_theory_value_t *value, bool *has_value) -> bool {
        CLINGO_TRY {
            auto *data = static_cast<Data *>(self);
            auto sym = Symbol();
            // NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
            if (index == 0) {
                sym = data->sym(1);
                value->int_number = 1;
                value->type = clingo_theory_value_type_int;
                *has_value = true;
            } else if (index == 1) {
                sym = data->sym(2);
                value->double_number = 3.14;
                value->type = clingo_theory_value_type_double;
                *has_value = true;
            } else if (index == 2) {
                sym = data->sym(3);
                auto val = Function(data->lib, "f", {Number(1)});
                value->symbol = c_cast(val);
                clingo_symbol_acquire(value->symbol);
                value->type = clingo_theory_value_type_symbol;
                *has_value = true;
            }
            // NOLINTEND(cppcoreguidelines-pro-type-union-access)
            *symbol = c_cast(sym);
            clingo_symbol_acquire(*symbol);
        }
        CLINGO_CATCH;
    }

    static auto create(clingo_lib_t *lib, clingo_theory_t *theory) -> bool {
        CLINGO_TRY {
            auto data = std::make_unique<Data>(Library{lib, true});
            *theory = clingo_theory_t{
                info,           destroy,          register_theory,  rewrite_ast,
                prepare,        register_options, validate_options, on_model,
                on_stats,       lookup_symbol,    assignment_next,  assignment_get_value,
                data.release(),
            };
        }
        CLINGO_CATCH;
    }
};

struct Fixture : SolveEventHandler {
    Library lib;
    Control ctl{lib, {"0"}};
    Theory thy{lib, TestTheory::create};

    [[nodiscard]] auto sym(int number) const -> Symbol { return Function(lib, "a", {Number(number)}); }

    auto do_model(Model model) -> bool override {
        thy.model(model);
        auto m_syms = model.symbols(ShowFlags::atoms);
        auto t_syms = model.symbols(ShowFlags::theory);
        REQUIRE(m_syms.size() == 1);
        REQUIRE(t_syms.size() == 1);
        REQUIRE(m_syms.front() == Clingo::Function(lib, "a"));
        REQUIRE(t_syms.front() == Number(42));
        auto vals = std::vector<TheoryAssignment::iterator::value_type>{};
        auto tass = thy.assignment(model.thread_id());
        for (auto const &val : tass) {
            vals.emplace_back(val);
        }
        REQUIRE(vals.size() == 3);
        REQUIRE(tass.lookup(sym(1)) == 0);
        REQUIRE(tass.lookup(sym(2)) == 1);
        REQUIRE(tass.lookup(sym(3)) == 2);
        // index 0
        REQUIRE(vals[0] == thy.assignment(model.thread_id()).at(0));
        REQUIRE(vals[0].first == sym(1));
        REQUIRE(std::get<int>(vals[0].second) == 1);
        // index 1
        REQUIRE(vals[1] == thy.assignment(model.thread_id()).at(1));
        REQUIRE(vals[1].first == sym(2));
        REQUIRE(std::get<double>(vals[1].second) == 3.14);
        // index 2
        REQUIRE(vals[2] == thy.assignment(model.thread_id()).at(2));
        REQUIRE(vals[2].first == sym(3));
        REQUIRE(std::get<Symbol>(vals[2].second) == Function(lib, "f", {Number(1)}));
        return true;
    }
    void do_stats(Stats step, Stats accu) override { thy.stats(step, accu); }
};

} // namespace

TEST_CASE_METHOD(Fixture, "theory", "[cxx][theory]") {
    auto *dta = static_cast<Data *>(c_cast(thy)->self);
    thy.register_theory(ctl);
    REQUIRE(dta->registered);
    thy.rewrite(lib, ctl, "a.");
    ctl.ground();
    thy.prepare(ctl);
    REQUIRE(dta->prepared);
    REQUIRE(ctl.solve(std::ref(*this)).get().satisfiable());
    REQUIRE(dta->models == 1);
    REQUIRE(ctl.stats()["user_step"]["test_key"].value() == 3.14);
}

} // namespace Clingo::Test
