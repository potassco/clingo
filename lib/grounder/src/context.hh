#pragma once

#include <gringo/input/program.hh>
#include <gringo/input/term.hh>

#include <gringo/input/rewrite/analyze.hh>

#include <gringo/ground/assignment_aggregate.hh>
#include <gringo/ground/base.hh>
#include <gringo/ground/body_aggregate.hh>
#include <gringo/ground/condlit.hh>
#include <gringo/ground/disjunction.hh>
#include <gringo/ground/head_aggregate.hh>
#include <gringo/ground/literal.hh>
#include <gringo/ground/program.hh>
#include <gringo/ground/term.hh>
#include <gringo/ground/theory_atom.hh>

#include <forward_list>

namespace Gringo::Grounder {

//! A map from a terms with projections associated state used during grounding.
//!
//! The terms represensts a class of similar terms that can reuse the same projection state.
using ProjectMap = Util::ordered_map<Ground::UTerm, std::unique_ptr<Ground::LitProject::State>>;

//! A map from signatures to atom bases.
using BaseMap = Util::ordered_map<std::tuple<String, size_t, bool>, std::unique_ptr<Ground::Base>>;

//! A helper that manages atom bases and auxiliary bases.
class BaseHelper {
  public:
    BaseHelper(BaseMap &atom_base, BaseMap &aux_base, ProjectMap &project_base)
        : atom_base_{&atom_base}, aux_base_{&aux_base}, project_base_{&project_base} {}

    auto add_base(std::tuple<String, size_t, bool> sig) -> BaseMap::iterator {
        auto aux = std::get<0>(sig).starts_with("#");
        auto dom_it = (aux ? aux_base_ : atom_base_)->try_emplace(std::move(sig), nullptr).first;
        if (dom_it->second == nullptr) {
            dom_it.value() = std::make_unique<Ground::Base>();
        }
        return dom_it;
    }

    //! Add a base for the given projection.
    auto add_project(SymbolStore &store, Ground::UTerm const &term,
                     Ground::Base &base) -> std::pair<Ground::UTerm, Ground::LitProject::State *> {
        size_t vars = 0;
        auto [it, ins] =
            project_base_->try_emplace(term->rename(store, Ground::RenameMode::rename_vars, nullptr, &vars));
        auto const &p_key = *it->first;
        auto &state = it.value();
        if (ins) {
            auto p_name = store.string_ref("#p_" + std::to_string(project_base_->size()));
            auto p_head = p_key.rename(store, Ground::RenameMode::drop_projection, &p_name, nullptr);
            auto p_body = p_key.rename(store, Ground::RenameMode::rename_projection, nullptr, &vars);
            state =
                std::make_unique<Ground::LitProject::State>(p_name, vars, base, std::move(p_head), std::move(p_body));
        }
        return {term->rename(store, Ground::RenameMode::drop_projection, &state->name(), nullptr), state.get()};
    }

  private:
    BaseMap *atom_base_;
    BaseMap *aux_base_;
    ProjectMap *project_base_;
};

using StateList =
    std::forward_list<std::variant<Ground::StateCondLit, Ground::StateHdAggr, Ground::StateBdAggr,
                                   Ground::StateAssignAggr, Ground::StateDisjunction, Ground::StateTheory>>;

//! Context object holding necessary data for translating from input to ground
//! representation.
class BuildContext {
  public:
    using DefMap = Util::unordered_map<Input::Term const *, std::vector<size_t>>;
    BuildContext(std::pmr::monotonic_buffer_resource &mbr, Logger &log, SymbolStore &store, BaseHelper base,
                 Input::Component const &comp, Util::unordered_map<Input::Term const *, std::vector<size_t>> &def_map,
                 Ground::Component &gcomp, Util::unordered_map<String, size_t> &var_map, Ground::ULitVec &body,
                 StateList &state)
        : mbr_{&mbr}, log_{&log}, store_{&store}, base_{base}, comp_{&comp}, def_map_{&def_map}, gcomp_{&gcomp},
          var_map_{&var_map}, body_{&body}, state_{&state} {}

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
        if (test(comp_->type, Input::ComponentType::single_pass)) {
            return true;
        }
        if (auto const *slit = std::get_if<Input::LitSymbolic>(&lit); slit != nullptr) {
            return slit->sign() != Sign::none || index(*slit) == Ground::stratified_index;
        }
        return true;
    }

    //! Check if the (current) body can be grounded in a single pass.
    [[nodiscard]] auto single_pass_body() const -> bool {
        return test(comp_->type, Input::ComponentType::single_pass) ||
               std::all_of(body_->begin(), body_->end(), [](auto const &lit) { return lit->single_pass(); });
    }

    [[nodiscard]] auto next_index() -> size_t { return comp_->incomplete.size() + index_++; }

    //! Analyze the given conditional literal and return the required indices for grounding.
    [[nodiscard]] auto analyze(Input::CondLit const &lit) -> std::tuple<bool, bool, bool, size_t, size_t, size_t> {
        assert(!Input::is_fixed(lit.lit()).value_or(false));

        auto has_conclusion = !Input::is_fixed(lit.lit()).has_value();
        auto sp_body = single_pass_body();
        auto sp_premise =
            test(comp_->type, Input::ComponentType::single_pass) ||
            std::all_of(lit.cond().begin(), lit.cond().end(), [this](auto const &lit) { return single_pass(lit); });
        bool sp_conclusion = single_pass(lit.lit());

        auto empty_index = Ground::stratified_index;
        auto premise_index = Ground::stratified_index;
        auto lit_index = Ground::stratified_index;

        if (!sp_premise || !sp_conclusion) {
            if (!sp_body) {
                empty_index = next_index();
            }
            if (!sp_body || !sp_premise) {
                premise_index = next_index();
            }
            lit_index = has_conclusion ? next_index() : premise_index;
        }

        return {has_conclusion, sp_conclusion, sp_premise, empty_index, premise_index, lit_index};
    }

    //! Get the logger.
    [[nodiscard]] auto logger() const -> Logger & { return *log_; }

    //! Get the symbol store.
    [[nodiscard]] auto store() const -> SymbolStore & { return *store_; }

    //! Add a base for a projection.
    auto add_project(Ground::UTerm const &term,
                     Ground::Base &base) -> std::pair<Ground::UTerm, Ground::LitProject::State *> {
        return base_.add_project(*store_, term, base);
    }

    //! Add an atom base for the given signature.
    auto add_base(std::tuple<String, size_t, bool> sig) -> BaseMap::iterator { return base_.add_base(sig); }

    //! Get the monotonic allocator for the component.
    [[nodiscard]] auto mbr() const -> std::pmr::monotonic_buffer_resource & { return *mbr_; }

    //! Get the definition map.
    [[nodiscard]] auto def_map() const -> DefMap & { return *def_map_; }
    //! Get the variable map.
    [[nodiscard]] auto var_map() const -> Util::unordered_map<String, size_t> & { return *var_map_; }

    //! Get the current component.
    [[nodiscard]] auto gcomp() const -> Ground::Component & { return *gcomp_; }
    //! Get the current statement body.
    [[nodiscard]] auto body() const -> Ground::ULitVec & { return *body_; }

    //! Add a new state object for a body aggregate literal.
    template <class T, class... Args> [[nodiscard]] auto state(Args &&...args) -> T & {
        state_->emplace_front(std::in_place_type<T>, std::forward<Args>(args)...);
        return std::get<T>(state_->front());
    }

    //! Increment the priority and return its previous value.
    auto inc_priority() -> size_t { return priority++; }

  private:
    std::pmr::monotonic_buffer_resource *mbr_;
    Logger *log_;
    SymbolStore *store_;
    BaseHelper base_;
    Input::Component const *comp_;
    Util::unordered_map<Input::Term const *, std::vector<size_t>> *def_map_;
    Ground::Component *gcomp_;
    Util::unordered_map<String, size_t> *var_map_;
    Ground::ULitVec *body_;
    StateList *state_;
    size_t priority = 0;
    size_t index_ = 0;
};

} // namespace Gringo::Grounder
