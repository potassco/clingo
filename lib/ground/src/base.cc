#include <clingo/ground/base.hh>

namespace Clingo::Ground {

void ProjectState::init(InstantiationContext const &ctx, size_t gen) {
    base_->update(gen);
    if (gen == 0) {
        // reset at generation zero
        p_base_.update(0);
    }
    if (size_t n = base_->end(MatcherType::all_atoms); imported_ != n) {
        // auto old = imported_;
        for (size_t m = base_->end(MatcherType::old_atoms); imported_ != n; ++imported_) {
            if (imported_ == m && gen > 0) {
                // import as new from here onward
                // (noting that gen > 0 can only )
                p_base_.update(gen - 1);
            }
            auto atom = base_->nth(imported_);
            for (auto &sym : ass_) {
                sym = std::nullopt;
            }
            auto eval_ctx = EvalContext{ctx.log(), ctx.store(), ctx.out(), ass_};
            if (p_body_->match(eval_ctx, atom->first)) {
                if (auto sym = p_head_->eval(eval_ctx); sym) {
                    // FIXME: This is obviously not correct, there need to be rules!!
                    // - we need the output here
                    // - introduce a fresh literal
                    // - derive the literal from the atom (via informing the output)
                    p_base_.add(*sym, atom->second.state, [id = atom->second.id]() { return id; });
                }
            }
        }
    }
}

//! Add an atom base.
//!
//! Names starting with a `#` are added as auxiliary bases.
auto Bases::add_base(std::tuple<String, size_t, bool> sig) -> Ground::AtomBase & {
    auto aux = std::get<0>(sig).starts_with("#");
    auto dom_it = (aux ? aux_ : atoms_).try_emplace(std::move(sig), nullptr).first;
    if (dom_it->second == nullptr) {
        dom_it.value() = std::make_unique<Ground::AtomBase>();
    }
    return *dom_it.value();
}

//! Add a base for a projected atom.
auto Bases::add_project(SymbolStore &store, Ground::UTerm const &term, Ground::AtomBase &base)
    -> std::pair<Ground::UTerm, ProjectState *> {
    size_t vars = 0;
    auto [it, ins] = projected_.try_emplace(term->rename(store, Ground::RenameMode::rename_vars, nullptr, &vars));
    auto const &p_key = *it->first;
    auto &state = it.value();
    if (ins) {
        auto p_name = store.string_ref("#p_" + std::to_string(projected_.size()));
        auto p_head = p_key.rename(store, Ground::RenameMode::drop_projection, &p_name, nullptr);
        auto p_body = p_key.rename(store, Ground::RenameMode::rename_projection, nullptr, &vars);
        state = std::make_unique<ProjectState>(p_name, vars, base, std::move(p_head), std::move(p_body));
    }
    return {term->rename(store, Ground::RenameMode::drop_projection, &state->name(), nullptr), state.get()};
}

auto Bases::get_base(std::tuple<String, size_t, bool> sig) const -> Ground::AtomBase * {
    auto aux = std::get<0>(sig).starts_with("#");
    auto const &dom = aux ? aux_ : atoms_;
    auto it = dom.find(sig);
    return it != dom.end() ? it.value().get() : nullptr;
}

void Bases::clear_aux() { aux_.clear(); }

} // namespace Clingo::Ground
