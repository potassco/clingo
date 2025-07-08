#pragma once

#include <clingo/control/term.hh>

#include <clingo/input/literal.hh>
#include <clingo/input/program.hh>
#include <clingo/input/term.hh>

#include <clingo/ground/base.hh>
#include <clingo/ground/program.hh>
#include <clingo/ground/script.hh>

#include <clingo/input/rewrite/analyze.hh>

#include <clingo/util/small_vector.hh>
#include <clingo/util/type_traits.hh>

#include <ostream>

namespace CppClingo::Control {

//! @addtogroup control
//! @{

//! A map from a terms with projections associated state used during grounding.
//!
//! The terms represent a class of similar terms that can reuse the same projection state.
using ProjectMap = CppClingo::Ground::ProjectMap;

//! A map from signatures to atom bases.
using BaseMap = CppClingo::Ground::BaseMap;

//! A map from variable names (input) to their indices (ground).
using VarMap = Util::unordered_map<String, size_t>;

//! A map from terms to the indices defining them.
using DefMap = Util::unordered_map<Input::Term const *, std::vector<size_t>>;

class ProfileProgram {
  public:
    auto add(Input::Stm const &stm) -> Ground::ProfileNodeInternal &;
    void print(std::ostream &out);

  private:
    using NodeMap = Util::unordered_map<Input::Stm const *, std::unique_ptr<Ground::ProfileNodeInternal>,
                                        std::hash<Input::Stm const *>, std::equal_to<void>>;

    NodeMap nodes_;
};

//! Context object holding necessary data for translating from input to ground
//! representation.
class BuildContext {
  public:
    //! Construct the build context.
    BuildContext(std::pmr::monotonic_buffer_resource &mbr, Logger &log, SymbolStore &store,
                 TheorySigVec const &theory_directives, Ground::Bases &base, Input::Component const &comp,
                 DefMap &def_map, Ground::Component &gcomp, VarMap &var_map, Ground::ULitVec &body,
                 Ground::UStateVec &states, Ground::ScriptCallback *context, ProfileProgram &profile)
        : mbr_{&mbr}, log_{&log}, store_{&store}, theory_directives_{&theory_directives}, base_{&base}, comp_{&comp},
          def_map_{&def_map}, gcomp_{&gcomp}, var_map_{&var_map}, body_{&body}, states_{&states}, context_{context},
          profile_{&profile} {}

    //! Get the index of the given symbolic literal.
    [[nodiscard]] auto index(Input::LitSymbolic const &lit) const -> size_t {
        auto it = comp_->incomplete.find(&lit.term());
        if (it != comp_->incomplete.end()) {
            return static_cast<size_t>(it - comp_->incomplete.begin());
        }
        return Ground::stratified_index;
    }
    //! Check if the given input literal is single pass.
    [[nodiscard]] auto single_pass(Input::Lit const &lit) const -> bool {
        if (intersects(comp_->type, Input::ComponentType::single_pass)) {
            return true;
        }
        if (auto const *slit = std::get_if<Input::LitSymbolic>(&lit); slit != nullptr) {
            return slit->sign() != Sign::none || index(*slit) == Ground::stratified_index;
        }
        return true;
    }

    //! Check if the (current) body can be grounded in a single pass.
    [[nodiscard]] auto single_pass_body() const -> bool {
        return intersects(comp_->type, Input::ComponentType::single_pass) ||
               std::ranges::all_of(*body_, [](auto const &lit) { return lit->single_pass(); });
    }

    //! Return a fresh atom index.
    [[nodiscard]] auto next_index() -> size_t { return comp_->incomplete.size() + index_++; }

    //! Get the logger.
    [[nodiscard]] auto logger() const -> Logger & { return *log_; }

    //! Get the symbol store.
    [[nodiscard]] auto store() const -> SymbolStore & { return *store_; }

    //! Get the associated context to call scripts.
    [[nodiscard]] auto context() const -> Ground::ScriptCallback * { return context_; }

    //! Get the symbol store.
    [[nodiscard]] auto profile() const -> ProfileProgram & { return *profile_; }

    //! Add a base for a projection.
    auto add_project(Ground::UTerm const &term, Ground::AtomBase &base)
        -> std::pair<Ground::UTerm, Ground::ProjectState *> {
        return base_->add_project(*store_, term, base);
    }

    //! Get the component type.
    [[nodiscard]] auto type() const -> Input::ComponentType { return comp_->type; };

    //! Add an atom base for the given signature.
    auto add_base(std::tuple<String, size_t, bool> sig) -> Ground::AtomBase & { return base_->add_base(sig); }

    //! Get the monotonic allocator for the component.
    [[nodiscard]] auto mbr() const -> std::pmr::monotonic_buffer_resource & { return *mbr_; }

    //! Get the definition map.
    [[nodiscard]] auto def_map() const -> DefMap & { return *def_map_; }
    //! Get the variable map.
    [[nodiscard]] auto var_map() const -> VarMap & { return *var_map_; }

    //! Get the current component.
    [[nodiscard]] auto gcomp() const -> Ground::Component & { return *gcomp_; }
    //! Get the current statement body.
    [[nodiscard]] auto body() const -> Ground::ULitVec & { return *body_; }

    //! Add a new state object for a body aggregate literal.
    template <class T, class... Args> [[nodiscard]] auto state(Args &&...args) -> T & {
        states_->emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
        return static_cast<T &>(*states_->back());
    }

    //! Increment the priority and return its previous value.
    auto inc_priority() -> size_t { return priority++; }

    //! Translate the given simple input literal into a simple ground atom.
    [[nodiscard]] auto simple_lit(Input::Lit const &lit) -> Ground::AtomSimple {
        auto res = Ground::AtomSimple{};
        with_simple_lit(lit, [&res]([[maybe_unused]] auto sig, auto term, auto &base, auto provides) {
            res.emplace(std::make_tuple(std::move(term), std::ref(base), std::move(provides)));
        });
        return res;
    }

    //! Translate the given input term (for an atom) into a simple ground atom.
    [[nodiscard]] auto simple_lit(Input::Term const &term) -> Ground::AtomSimple::value_type {
        auto res = Ground::AtomSimple{};
        with_simple_lit(term, [&res]([[maybe_unused]] auto sig, auto term, auto &base, auto provides) {
            res.emplace(std::make_tuple(std::move(term), std::ref(base), std::move(provides)));
        });
        assert(res);
        return *std::move(res);
    }

    //! Translate the given input term (for an atom) with a callback.
    template <class F> void with_simple_lit(Input::Term const &term, F &&fun) {
        auto provides = std::vector<size_t>{};
        auto sig = signature(term);
        assert(sig);
        auto &base = add_base(*sig);
        if (auto it = def_map_->find(&term); it != def_map_->end()) {
            provides = it->second;
        }
        std::invoke(std::forward<F>(fun), *sig, build_term(*var_map_, term), base, std::move(provides));
    }

    //! Translate the given simple input literal with a callback.
    template <class F> void with_simple_lit(Input::Lit const &lit, F &&fun, bool expect_truth = false) {
        std::visit(
            [&]<class T>(T const &lit) {
                if constexpr (Util::matches<T, Input::LitSymbolic>) {
                    assert(lit.sign() == Sign::none);
                    with_simple_lit(lit.term(), std::forward<F>(fun));
                    return;
                } else if constexpr (Util::matches<T, Input::LitBool>) {
                    if (lit.value() == expect_truth) {
                        return;
                    }
                }
                throw std::runtime_error("unexpected literal in rule head");
            },
            lit);
    }

    //! Check whether the given signature corresponds to a directive.
    [[nodiscard]] auto is_theory_directive(TheorySig sig) const -> bool {
        return std::ranges::binary_search(*theory_directives_, sig);
    }

  private:
    std::pmr::monotonic_buffer_resource *mbr_;
    Logger *log_;
    SymbolStore *store_;
    TheorySigVec const *theory_directives_;
    Ground::Bases *base_;
    Input::Component const *comp_;
    DefMap *def_map_;
    Ground::Component *gcomp_;
    Util::unordered_map<String, size_t> *var_map_;
    Ground::ULitVec *body_;
    Ground::UStateVec *states_;
    Ground::ScriptCallback *context_;
    ProfileProgram *profile_;
    size_t priority = 0;
    size_t index_ = 0;
};

//! @}

} // namespace CppClingo::Control
