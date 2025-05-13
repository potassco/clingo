#include <clingo/ground/base.hh>

namespace CppClingo::Ground {

// #define DEBUG_PROJECTION

void ProjectState::init(InstantiationContext const &ctx, size_t gen) {
    base_->update(gen);
    for (size_t n = base_->end(MatcherType::all_atoms), m = base_->end(MatcherType::old_atoms); imported_ != n;
         ++imported_) {
        if (imported_ == m && gen > 0) {
            // import as new from here onward
            // (generation 0 can be ignored because there will be a reset later)
            p_base_.update(gen - 1);
        }
        auto atom = base_->nth(imported_);
        for (auto &sym : ass_) {
            sym = std::nullopt;
        }
        auto eval_ctx = EvalContext{ctx.log(), ctx.store(), ctx.out(), ass_};
        if (p_body_->match(eval_ctx, atom->first)) {
            if (auto sym = p_head_->eval(eval_ctx); sym) {
                auto [it, res] = p_base_.add(*sym, atom->second.state, [&]() {
                    // if the atom is a fact we can simply use its uid here
                    return atom.value().state == StateAtom::fact ? atom.value().id : ctx.out().uid();
                });
                // add projection rules (if not previously derived as a fact)
                if (res == AtomUpdate::changed || it.value().state != StateAtom::fact) {
                    ctx.out().project_atom(it.value().id, atom.value().id);
                }
            }
        }
    }
#ifdef DEBUG_PROJECTION
    p_base_.update(gen);
    Util::unordered_set<size_t> old;
    for (size_t i = 0, n = base_->end(MatcherType::all_atoms); i != n; ++i) {
        auto atom = base_->nth(i);
        for (auto &sym : ass_) {
            sym = std::nullopt;
        }
        auto eval_ctx = EvalContext{ctx.log(), ctx.store(), ctx.out(), ass_};
        if (p_body_->match(eval_ctx, atom->first)) {
            if (auto sym = p_head_->eval(eval_ctx); sym) {
                auto j = p_base_.index(*sym);
                if (j == p_base_.end(MatcherType::all_atoms)) {
                    printf("atom not found\n");
                } else if (i < base_->end(MatcherType::old_atoms)) {
                    if (j < p_base_.end(MatcherType::old_atoms)) {
                        old.emplace(j);
                    } else {
                        printf("unexpected new atom\n");
                    }
                } else if (j < p_base_.end(MatcherType::old_atoms) && !old.contains(j)) {
                    printf("unexpected old atom\n");
                }
            }
        }
    }
#endif
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

void Bases::clear_aux() {
    aux_.clear();
}

} // namespace CppClingo::Ground
