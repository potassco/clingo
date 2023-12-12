#include <util/algorithm.hh>

#include <input/algo/analyze.hh>
#include <input/algo/unpool_relations.hh>

#include "unpool.hh"

namespace Gringo::Input {

namespace {

struct NegateLiteral {
    auto operator()(Literal const &lit) const -> Literal { return std::visit(*this, lit); }
    auto operator()(LiteralBoolean const &lit) const -> Literal {
        return LiteralBoolean{lit.loc, lit.sign, !lit.value};
    }
    auto operator()(LiteralRelation const &lit) const -> Literal {
        assert(lit.rhs.size() == 1);
        auto const &[rel, rhs] = lit.rhs.front();
        return LiteralRelation{lit.loc, lit.sign, lit.lhs, Util::make_vec<Guard>(Guard{complement(rel), rhs})};
    }
    auto operator()(LiteralSymbolic const &lit) const -> Literal {
        return LiteralSymbolic{lit.loc, lit.sign + Sign::once, lit.term};
    }
};

//! Helper to update a vector of elements.
//!
//! @todo: this is rather generic and a candidate for Util.
template <class T> class ResultVec {
  public:
    ResultVec(std::vector<T> const &source) : source_{source}, current_{source.begin()} {}

    //! Keep the current element.
    void keep() {
        if (result_) {
            result_->emplace_back(*current_);
        }
        ++current_;
    }
    //! Remove the current element.
    void remove() {
        if (!result_) {
            result_ = Util::copy_n(source_, std::distance(source_.begin(), current_));
        }
        ++current_;
    }
    //! Replace the current element.
    template <class... Args> void replace(Args &&...args) {
        if (!result_) {
            result_ = Util::copy_n(source_, std::distance(source_.begin(), current_));
        }
        result_->emplace_back(std::forward<Args>(args)...);
        ++current_;
    }
    //! Append fresh elements.
    template <class... Args> void append(Args &&...args) {
        if (!result_) {
            result_ = Util::copy_n(source_, std::distance(source_.begin(), current_));
        }
        result_->emplace_back(std::forward<Args>(args)...);
    }
    //! Get a const reference to the current vector.
    //!
    //! This returns a reference to the old vector if it does not have a new one.
    [[nodiscard]] auto value() const & -> std::vector<T> const & { return result_ ? result_.value() : source_; }
    //! Move out the new vector or return a copy of the old one.
    [[nodiscard]] auto value() && -> std::vector<T> { return std::move(result_).value_or(source_); }
    //! Check if the old vector has been updated.
    [[nodiscard]] auto has_value() const -> bool { return result_.has_value(); }

    //! Get a const reference to the current vector.
    //!
    //! This returns a reference to the old vector if it does not have a new one.
    [[nodiscard]] auto operator*() const & -> std::vector<T> const & { value(); }
    //! Move out the new vector or return a copy of the old one.
    [[nodiscard]] auto operator*() && -> std::vector<T> { return value(); }
    //! Arrow operator based on (const ref) value.
    auto operator->() const -> std::vector<T> const * { return result_ ? &result_.value() : &source_; }
    //! Check if the old vector has been updated.
    explicit operator bool() const { return has_value(); }

  private:
    std::vector<T> const &source_;
    std::optional<std::vector<T>> result_;
    std::vector<T>::const_iterator current_;
};

auto rewrite_head(HeadLiteral const &head, ResultVec<BodyLiteral> &body) -> std::optional<HeadLiteral> {
    std::optional<HeadLiteral> res_head;
    auto res = std::optional<std::pair<HeadLiteral, BodyLiteralVec>>{};
    if (auto const *lit = std::get_if<SimpleHeadLiteral>(&head); lit != nullptr) {
        if (!is_atom(lit->lit) && !is_boolean(lit->lit)) {
            body.append(NegateLiteral{}(lit->lit));
            res_head = SimpleHeadLiteral{LiteralBoolean{location(lit->lit), Sign::none, false}};
        }
    } else if (auto const *disj = std::get_if<Disjunction>(&head); disj != nullptr) {
        auto res_elems = ResultVec{disj->elems};
        for (auto const &elem : disj->elems) {
            if (elem.cond.empty()) {
                auto res_lits = ResultVec{elem.lits};
                for (auto const &lit : elem.lits) {
                    // Note: we should never get a boolean literal
                    // here. False literals are removed from the
                    // set of literals and true literals with an
                    // empty conditions would make the whole
                    // disjunction true.
                    if (!is_atom(lit)) {
                        res_lits.remove();
                        body.append(NegateLiteral{}(lit));
                    } else {
                        res_lits.keep();
                    }
                }
                if (res_lits->empty()) {
                    res_elems.remove();
                } else if (res_lits) {
                    res_elems.replace(ConditionalLiteral{elem.loc, *std::move(res_lits), {}});
                } else {
                    res_elems.keep();
                }
            } else {
                res_elems.keep();
            }
        }
        if (res_elems) {
            if (res_elems->empty()) {
                res_head = SimpleHeadLiteral{LiteralBoolean{disj->loc, Sign::none, false}};
            } else {
                res_head = Disjunction{disj->loc, *std::move(res_elems)};
            }
        }
    }
    return res_head;
}

auto rewrite_body(BodyLiteralVec const &body) -> ResultVec<BodyLiteral> {
    auto res_body = ResultVec{body};
    for (auto const &blit : body) {
        if (auto const *conj = std::get_if<Conjunction>(&blit)) {
            auto res_elems = ResultVec{conj->elems};
            for (auto const &elem : conj->elems) {
                if (elem.cond.empty()) {
                    res_elems.remove();
                    for (auto const &lit : elem.lits) {
                        res_body.append(SimpleBodyLiteral{lit});
                    }
                } else {
                    res_elems.keep();
                }
            }
            // construct conjunctions from the modified elements
            if (res_elems) {
                res_body.remove();
                for (auto &elem : *std::move(res_elems)) {
                    res_body.append(Conjunction{conj->loc, Util::make_vec<ConditionalLiteral>(std::move(elem))});
                }
            }
            // construct conjunctions from the existing elements
            else if (conj->elems.size() != 1) {
                res_body.remove();
                for (auto const &elem : conj->elems) {
                    res_body.append(Conjunction{conj->loc, Util::make_vec<ConditionalLiteral>(elem)});
                }
            }
            // keep existing literal
            else {
                res_body.keep();
            }
        }
        // keep existing literal
        else {
            res_body.keep();
        }
    }
    return res_body;
}

template <class F, class... Args>
auto rewrite_statement(std::optional<StatementVec> res, F &&fun, Args &&...args) -> std::optional<StatementVec> {
    if (!res) {
        auto res_stm = std::invoke(std::forward<F>(fun), std::forward<Args>(args)...);
        if (res_stm) {
            return Util::make_vec<Statement>(*std::move(res_stm));
        }
    }
    return res;
}

struct UnpoolRelations {

    // protect ourselves -> no unintended overloads

    template <class T>
    auto operator()(T const &x, bool head) const -> std::optional<std::vector<std::vector<T>>> = delete;

    // literal

    auto operator()(Literal const &lit, bool head) const -> std::optional<LiteralVecVec> {
        return std::visit(*this, lit, std::variant<bool>{head});
    }

    auto operator()(LiteralBoolean const &lit, bool head) const -> std::optional<LiteralVecVec> {
        static_cast<void>(head);
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(LiteralRelation const &lit, bool head) const -> std::optional<LiteralVecVec> {
        // TODO: the literal still has to be simplified
        if (lit.rhs.size() > 1) {
            auto conjunctive = head == (lit.sign == Sign::once);
            auto const *lhs = &lit.lhs;
            LiteralVecVec res;
            if (conjunctive) {
                res.emplace_back();
                res.back().reserve(lit.rhs.size());
            }
            for (auto const &rhs : lit.rhs) {
                if (!conjunctive) {
                    res.emplace_back();
                }
                auto rel = lit.sign == Sign::none ? rhs.first : complement(rhs.first);
                res.back().emplace_back(
                    LiteralRelation{lit.loc, Sign::none, *lhs, Util::make_vec<Guard>(Guard{rel, rhs.second})});
                lhs = &rhs.second;
            }
            return res;
        }
        return std::nullopt;
    }

    auto operator()(LiteralSymbolic const &lit, bool head) const -> std::optional<LiteralVecVec> {
        static_cast<void>(head);
        static_cast<void>(lit);
        return std::nullopt;
    }

    // conditional literal
    template <bool head> using HBLitVecVec = std::conditional_t<head, HeadLiteralVec, BodyLiteralVec>;

    template <bool Conjunctive>
    auto operator()(Junction<Conjunctive> const &lit) const -> std::optional<HBLitVecVec<!Conjunctive>> {
        static_cast<void>(lit);
        throw std::runtime_error("implement me!!!");
    }

    // aggregate

    template <bool HasSign>
    auto operator()(SetAggregate<HasSign> const &lit) const -> std::optional<HBLitVecVec<!HasSign>> {
        static_cast<void>(lit);
        throw std::runtime_error("implement me!!!");
    }

    // theory

    template <bool HasSign>
    auto operator()(TheoryAtom<HasSign> const &atom) const -> std::optional<HBLitVecVec<!HasSign>> {
        static_cast<void>(atom);
        throw std::runtime_error("implement me!!!");
    }

    // head literal

    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteralVec> { return std::visit(*this, lit); }

    auto operator()(SimpleHeadLiteral const &lit) const -> std::optional<HeadLiteralVec> {
        if (auto res_lits = operator()(lit.lit, true); res_lits) {
            HeadLiteralVec head_lits;
            head_lits.reserve(res_lits->size());
            for (auto &lits : *res_lits) {
                assert(!lits.empty());
                if (lits.size() == 1) {
                    head_lits.emplace_back(SimpleHeadLiteral{std::move(lits.back())});
                } else {
                    ConditionalLiteralVec elems;
                    elems.reserve(lits.size());
                    for (auto &lit : lits) {
                        elems.emplace_back(location(lit), Util::make_vec<Literal>(std::move(lit)), LiteralVec{});
                    }
                    head_lits.emplace_back(Disjunction{location(lit.lit), std::move(elems)});
                }
            }
            return head_lits;
        }
        return std::nullopt;
    }

    auto operator()(HeadAggregate const &lit) const -> std::optional<HeadLiteralVec> {
        static_cast<void>(lit);
        throw std::runtime_error("implement me!!!");
    }

    // body literal

    auto operator()(BodyLiteral const &lit) const -> std::optional<BodyLiteralVec> { return std::visit(*this, lit); }

    auto operator()(SimpleBodyLiteral const &lit) const -> std::optional<BodyLiteralVec> {
        if (auto res_lits = operator()(lit.lit, false); res_lits) {
            BodyLiteralVec body_lits;
            body_lits.reserve(res_lits->size());
            for (auto &lits : *res_lits) {
                assert(!lits.empty());
                if (lits.size() == 1) {
                    body_lits.emplace_back(SimpleBodyLiteral{std::move(lits.back())});
                } else {
                    ConditionalLiteralVec elems;
                    elems.reserve(lits.size());
                    for (auto &lit : lits) {
                        elems.emplace_back(location(lit), Util::make_vec<Literal>(std::move(lit)), LiteralVec{});
                    }
                    body_lits.emplace_back(Conjunction{location(lit.lit), std::move(elems)});
                }
            }
            return body_lits;
        }
        return std::nullopt;
    }

    auto operator()(BodyAggregate const &lit) const -> std::optional<BodyLiteralVec> {
        // we now need the crossproduct here!!!
        static_cast<void>(lit);
        throw std::runtime_error("implement me!!!");
    }

    // statement

    auto operator()(Statement const &stm) const -> std::optional<StatementVec> { return std::visit(*this, stm); }

    auto operator()(BodyLiteralVec const &lits) const -> std::optional<std::vector<BodyLiteralVec>> {
        return unpool_crossproduct(lits, *this);
    }

    auto operator()(Rule const &stm) const -> std::optional<StatementVec> {
        auto rewrite_rule = [&](auto const &rule, bool not_null) -> std::optional<Statement> {
            auto body = rewrite_body(rule.body);
            auto head = rewrite_head(rule.head, body);
            if (head || body || not_null) {
                return Rule{stm.loc, std::move(head).value_or(rule.head), *std::move(body)};
            }
            return std::nullopt;
        };
        return rewrite_statement(unpool_crossproducts(
                                     [&](auto head, auto body) -> Statement {
                                         return *rewrite_rule(Rule{stm.loc, std::move(head), std::move(body)}, true);
                                     },
                                     *this, stm.head, stm.body),
                                 rewrite_rule, stm, false);
    }

    auto operator()(TheoryDefinition const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementOptimize const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementWeakConstraint const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementShow const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementShowSig const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementProject const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementProjectSig const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementDefined const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementExternal const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementEdge const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementHeuristic const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementScript const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementInclude const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementProgram const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(StatementConst const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    auto operator()(Comment const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
    }

    RewriteContext const &ctx;
};

} // namespace

[[nodiscard]] auto unpool_relations(RewriteContext &ctx, Literal const &lit, bool head)
    -> std::optional<LiteralVecVec> {
    return UnpoolRelations{ctx}(lit, head);
}

[[nodiscard]] auto unpool_relations(RewriteContext &ctx, HeadLiteral const &lit) -> std::optional<HeadLiteralVec> {
    // 0 < x < 9 :- B.
    // corresponds to:
    // 0 < x :- B.
    // x < 9 :- B.
    // corresponds to:
    // :- B, not 0 < x.
    // :- B, not x < 9.
    // corresponds to:
    // :- B, 0 >= x.
    // :- B, x >= 9.
    //
    // not 0 < x < 9 :- B.
    // corresponds to:
    // not 0 < x | not x < 9 :- B.
    // corresponds to:
    // :- B, 0 < x, x < 9.
    //
    // 0 < x < 9: C :- B.
    // corresponds to:
    // 0 < x: C :- B.
    // x < 9: C :- B.
    // looks like it is not possible to do more
    //
    // not 0 < x < 9: C :- B.
    // corresponds to:
    // (not 0 < x | not x < 9): C :- B.
    // corresponds to:
    // (0 >= x | x >= 9): C :- B.
    // looks like it is not possible to do more
    return UnpoolRelations{ctx}(lit);
}

[[nodiscard]] auto unpool_relations(RewriteContext &ctx, BodyLiteral const &lit) -> std::optional<BodyLiteralVec> {
    return UnpoolRelations{ctx}(lit);
}

[[nodiscard]] auto unpool_relations(RewriteContext &ctx, Statement const &stm) -> std::optional<StatementVec> {
    return UnpoolRelations{ctx}(stm);
}

} // namespace Gringo::Input
