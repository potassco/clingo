#include <clingo/control/term.hh>

#include <clingo/input/rewrite/analyze.hh>

#include <clingo/util/type_traits.hh>

namespace CppClingo::Control {

namespace {

//! Maps a binary input operator to the corresponding ground one.
//!
//! @note: The input interval operator has no matching ground version.
//! @note: Candidate for core.
inline auto map_binary_op(Input::BinaryOperator op) -> Ground::BinaryOperator {
    switch (op) {
        case Input::BinaryOperator::and_: {
            return Ground::BinaryOperator::and_;
        }
        case Input::BinaryOperator::div: {
            return Ground::BinaryOperator::div;
        }
        case Input::BinaryOperator::dots: {
            break;
        }
        case Input::BinaryOperator::minus: {
            return Ground::BinaryOperator::minus;
        }
        case Input::BinaryOperator::mod: {
            return Ground::BinaryOperator::mod;
        }
        case Input::BinaryOperator::or_: {
            return Ground::BinaryOperator::or_;
        }
        case Input::BinaryOperator::plus: {
            return Ground::BinaryOperator::plus;
        }
        case Input::BinaryOperator::pow: {
            return Ground::BinaryOperator::pow;
        }
        case Input::BinaryOperator::times: {
            return Ground::BinaryOperator::times;
        }
        case Input::BinaryOperator::xor_: {
            return Ground::BinaryOperator::xor_;
        }
    }
    throw std::runtime_error("unsupported binary operator");
}

//! Translates input terms to their ground representation.
//!
//! Input terms must be rewritten before translation.
class BuilderTerm {
  public:
    //! Construct a term builder.
    //!
    //! The reference has_projection is used to track, whether a projection
    //! star occurred in the term. The var_map dictionary is used to map
    //! variables to integers. Each name must have been assigned an integer
    //! beforehand.
    BuilderTerm(bool &has_projection, Util::unordered_map<String, size_t> const &var_map)
        : has_projection_{&has_projection}, var_map_{&var_map} {}

    //! Translate a variable.
    auto operator()(Input::TermVariable const &term) const -> Ground::UTerm {
        assert(var_map_->contains(term.name()));
        return std::make_unique<Ground::TermVariable>(var_map_->find(term.name())->second);
    }
    //! Translate a symbol.
    auto operator()(Input::TermSymbol const &term) const -> Ground::UTerm {
        return std::make_unique<Ground::TermSymbol>(term.value());
    }
    auto operator()(Input::TermFormatString const &term) const -> Ground::UTerm {
        // FIXME: add implementation
        std::ignore = term;
        throw std::logic_error("implement me: ground representation for format string");
    }
    //! Translate arguments of tuples and functions.
    [[nodiscard]] auto handle_args(Input::ArgumentArray const &args) const -> Ground::UTermVec {
        Ground::UTermVec g_args;
        g_args.reserve(args.size());
        for (auto const &arg : args) {
            g_args.emplace_back(std::visit(
                [this]<class T>(T const &arg) -> Ground::UTerm {
                    if constexpr (Util::matches<T, Input::Projection>) {
                        *has_projection_ = true;
                        return std::make_unique<Ground::TermProjection>();
                    } else {
                        return std::visit(*this, arg);
                    }
                },
                arg));
        }
        return g_args;
    }
    //! Translate tuples.
    //!
    //! Assumes that the arguments consists of a single pool.
    auto operator()(Input::TermTuple const &term) const -> Ground::UTerm {
        assert(term.pool().size() == 1 && std::holds_alternative<Input::ArgumentTuple>(term.pool().front()));
        return std::make_unique<Ground::TermTuple>(
            handle_args(std::get<Input::ArgumentTuple>(term.pool().front()).elems()));
    }
    //! Translate a function.
    //!
    //! Assumes that the arguments consist of a single pool.
    auto operator()(Input::TermFunction const &term) const -> Ground::UTerm {
        assert(!term.external() && term.pool().size() == 1);
        return std::make_unique<Ground::TermFunction>(term.name(), handle_args(term.pool().front().elems()));
    }
    //! Translate a function.
    //!
    //! Assumes that there is a single argument.
    auto operator()(Input::TermAbs const &term) const -> Ground::UTerm {
        assert(term.pool().size() == 1);
        auto const &rhs = term.pool().front();
        return std::make_unique<Ground::TermUnary>(Ground::UnaryOperator::abs, location(rhs), std::visit(*this, rhs));
    }
    //! Translate a unary term.
    auto operator()(Input::TermUnary const &term) const -> Ground::UTerm {
        Ground::UnaryOperator op =
            term.op() == Input::UnaryOperator::minus ? Ground::UnaryOperator::minus : Ground::UnaryOperator::negate;
        return std::make_unique<Ground::TermUnary>(op, location(*term.rhs()), std::visit(*this, *term.rhs()));
    }
    //! Translate a unary term.
    //!
    //! Intervals must be removed by rewriting beforehand.
    auto operator()(Input::TermBinary const &term) const -> Ground::UTerm {
        assert(term.op() != Input::BinaryOperator::dots);
        if (auto lin = Input::check_linear(term); lin) {
            assert(var_map_->contains(lin->x()));
            return std::make_unique<Ground::TermLinear>(term.loc(), lin->m(), var_map_->find(lin->x())->second,
                                                        lin->n());
        }
        return std::make_unique<Ground::TermBinary>(location(*term.lhs()), std::visit(*this, *term.lhs()),
                                                    map_binary_op(term.op()), location(*term.rhs()),
                                                    std::visit(*this, *term.rhs()));
    }

  private:
    bool *has_projection_;
    Util::unordered_map<String, size_t> const *var_map_;
};

//! Translates input theory terms to their ground representation.
//!
//! Input terms must be rewritten before translation.
class BuilderTheoryTerm {
  public:
    //! Construct a theory term builder.
    //!
    //! The var_map dictionary is used to map variables to integers. Each name
    //! must have been assigned an integer beforehand.
    BuilderTheoryTerm(Util::unordered_map<String, size_t> const &var_map) : var_map_{&var_map} {}

    //! Translate unparsed terms.
    auto operator()([[maybe_unused]] Input::TheoryTermUnparsed const &term) const -> Ground::UTheoryTerm {
        throw std::logic_error("unexpected unparsed theory term");
    }
    //! Translate a variable.
    auto operator()(Input::TheoryTermVariable const &term) const -> Ground::UTheoryTerm {
        assert(var_map_->contains(term.name()));
        return std::make_unique<Ground::TheoryTermVariable>(var_map_->find(term.name())->second);
    }
    //! Translate a symbol.
    auto operator()(Input::TheoryTermSymbol const &term) const -> Ground::UTheoryTerm {
        return std::make_unique<Ground::TheoryTermSymbol>(term.value());
    }
    //! Translate arguments of tuples and functions.
    [[nodiscard]] auto handle_args(Input::TheoryTermArray const &args) const -> Ground::UTheoryTermVec {
        Ground::UTheoryTermVec g_args;
        g_args.reserve(args.size());
        for (auto const &arg : args) {
            g_args.emplace_back(std::visit(*this, arg));
        }
        return g_args;
    }
    //! Translate tuples.
    auto operator()(Input::TheoryTermTuple const &term) const -> Ground::UTheoryTerm {
        return std::make_unique<Ground::TheoryTermTuple>(term.type(), handle_args(term.elems()));
    }
    //! Translate a function.
    auto operator()(Input::TheoryTermFunction const &term) const -> Ground::UTheoryTerm {
        return std::make_unique<Ground::TheoryTermFunction>(term.name(), handle_args(term.args()));
    }

  private:
    Util::unordered_map<String, size_t> const *var_map_;
};

} // namespace

//! Translates input theory terms to their ground representation.
auto build_term(Util::unordered_map<String, size_t> const &var_map, Input::Term const &term, bool &has_projection)
    -> Ground::UTerm {
    return std::visit(BuilderTerm{has_projection, var_map}, term);
}

//! Translates input theory terms to their ground representation.
auto build_theory_term(Util::unordered_map<String, size_t> const &var_map, Input::TheoryTerm const &term)
    -> Ground::UTheoryTerm {
    return std::visit(BuilderTheoryTerm{var_map}, term);
}

} // namespace CppClingo::Control
