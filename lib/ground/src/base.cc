#include <clingo/ground/base.hh>

namespace Clingo::Ground {

void ProjectState::init(InitContext const &ctx, size_t gen) {
    base_->update(gen);
    for (size_t n = base_->end(MatcherType::all_atoms); imported_ != n; ++imported_) {
        auto atom = base_->nth(imported_);
        for (auto &sym : ass_) {
            sym = std::nullopt;
        }
        auto eval_ctx = EvalContext{ctx.log(), ctx.store(), ass_};
        if (p_body_->match(eval_ctx, atom->first)) {
            if (auto sym = p_head_->eval(eval_ctx); sym) {
                p_base_.add(*sym, atom->second.state, [id = atom->second.id]() { return id; });
            }
        }
    }
    p_base_.update(gen);
}

//! Add an atom base.
//!
//! Names starting with a `#` are added as auxiliary bases.
auto Base::add_base(std::tuple<String, size_t, bool> sig) -> Ground::AtomBase & {
    auto aux = std::get<0>(sig).starts_with("#");
    auto dom_it = (aux ? aux_base_ : atom_base_).try_emplace(std::move(sig), nullptr).first;
    if (dom_it->second == nullptr) {
        dom_it.value() = std::make_unique<Ground::AtomBase>();
    }
    return *dom_it.value();
}

//! Add a base for a projected atom.
auto Base::add_project(SymbolStore &store, Ground::UTerm const &term, Ground::AtomBase &base)
    -> std::pair<Ground::UTerm, ProjectState *> {
    size_t vars = 0;
    auto [it, ins] = project_base_.try_emplace(term->rename(store, Ground::RenameMode::rename_vars, nullptr, &vars));
    auto const &p_key = *it->first;
    auto &state = it.value();
    if (ins) {
        auto p_name = store.string_ref("#p_" + std::to_string(project_base_.size()));
        auto p_head = p_key.rename(store, Ground::RenameMode::drop_projection, &p_name, nullptr);
        auto p_body = p_key.rename(store, Ground::RenameMode::rename_projection, nullptr, &vars);
        state = std::make_unique<ProjectState>(p_name, vars, base, std::move(p_head), std::move(p_body));
    }
    return {term->rename(store, Ground::RenameMode::drop_projection, &state->name(), nullptr), state.get()};
}

auto Base::get_base(std::tuple<String, size_t, bool> sig) const -> Ground::AtomBase * {
    auto aux = std::get<0>(sig).starts_with("#");
    auto const &dom = aux ? aux_base_ : atom_base_;
    auto it = dom.find(sig);
    return it != dom.end() ? it.value().get() : nullptr;
}

void Base::clear_aux() { aux_base_.clear(); }

} // namespace Clingo::Ground
