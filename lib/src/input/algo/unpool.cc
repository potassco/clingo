/*
auto TermSymbol::unpool() const -> std::optional<STermVec> { return std::nullopt; }

auto TermTuple::unpool() const -> std::optional<STermVec> {
    // unpool the elements
    auto elems = unpool_union(pool_, [](Element const &tuple_or_term) {
        return Util::visit_variant(
            tuple_or_term,
            [](STerm const &term) -> std::optional<ElementVec> {
                return map_opt_vec(term->unpool(), [](auto term) { return Element{std::move(term)}; });
            },
            [](TupleVec const &tuple) -> std::optional<ElementVec> {
                return map_opt_vec(unpool_crossproduct(tuple,
                                                       [](TupleElem const &elem) {
                                                           return Util::visit_variant(
                                                               elem,
                                                               [](STerm const &term) -> std::optional<TupleVec> {
                                                                   return map_opt_vec(term->unpool(), [](auto term) {
                                                                       return TupleElem{std::move(term)};
                                                                   });
                                                               },
                                                               [](std::monostate x) -> std::optional<TupleVec> {
                                                                   static_cast<void>(x);
                                                                   return std::nullopt;
                                                               });
                                                       }),
                                   [](auto tuple) { return Element{std::move(tuple)}; });
            });
    });

    // turn the elements into individual tuple terms or terms
    if (!elems.has_value() && (pool_.size() != 1 || std::holds_alternative<STerm>(pool_.front()))) {
        elems = pool_;
    }
    return map_opt_vec(std::move(elems), [](auto elem) -> STerm {
        return Util::visit_variant(
            std::move(elem), [](STerm term) { return term; },
            [](TupleVec tuple) { return Util::construct_shared<TermTuple, Term>(ElementVec{std::move(tuple)}); });
    });
}

auto TermVariable::unpool() const -> std::optional<STermVec> { return std::nullopt; }

auto TermAbs::unpool() const -> std::optional<STermVec> {
    auto unpooled = unpool_union(pool_);
    if (!unpooled.has_value() && pool_.size() != 1) {
        unpooled = pool_;
    }
    return map_opt_vec(std::move(unpooled),
                       [](auto term) { return Util::construct_shared<TermAbs, Term>(STermVec{std::move(term)}); });
}

auto TermFunction::unpool() const -> std::optional<STermVec> {
    auto elems = unpool_union(pool_, [](TupleVec const &tuple) {
        // unpool the elements
        return unpool_crossproduct(tuple, [](TupleElem const &elem) {
            return Util::visit_variant(
                elem,
                [](STerm const &term) -> std::optional<TupleVec> {
                    return map_opt_vec(term->unpool(), [](auto term) { return TupleElem{std::move(term)}; });
                },
                [](std::monostate x) -> std::optional<TupleVec> {
                    static_cast<void>(x);
                    return std::nullopt;
                });
        });
    });

    if (!elems.has_value() && pool_.size() != 1) {
        elems = pool_;
    }

    return map_opt_vec(std::move(elems), [this](auto elem) {
        // turn individual elements into function terms
        return Util::construct_shared<TermFunction, Term>(name_, PoolVec{std::move(elem)}, external_);
    });
}

auto TermUnary::unpool() const -> std::optional<STermVec> {
    return map_opt_vec(rhs_->unpool(),
                       [this](auto term) { return Util::construct_shared<TermUnary, Term>(op_, std::move(term)); });
}

auto TermBinary::unpool() const -> std::optional<STermVec> {
    return unpool_crossproducts(
        [this](STerm lhs, STerm rhs) {
            return Util::construct_shared<TermBinary, Term>(std::move(lhs), op_, std::move(rhs));
        },
        [](STerm const &term) { return term->unpool(); }, lhs_, rhs_);
}

*/
