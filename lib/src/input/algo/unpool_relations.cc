#include <input/algo/unpool_relations.hh>

namespace Gringo::Input {

namespace {

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
        if (lit.rhs.size() > 1) {
            static_cast<void>(head);
            throw std::runtime_error("implement me!!!");
        }
        return std::nullopt;
    }

    auto operator()(LiteralSymbolic const &lit, bool head) const -> std::optional<LiteralVecVec> {
        static_cast<void>(head);
        static_cast<void>(lit);
        return std::nullopt;
    }

    auto operator()(HeadLiteral const &lit) const -> std::optional<HeadLiteralVecVec> {
        static_cast<void>(lit);
        throw std::logic_error("implement me!!!");
        // return std::visit(*this, lit);
    }

    auto operator()(BodyLiteral const &lit) const -> std::optional<BodyLiteralVecVec> {
        static_cast<void>(lit);
        throw std::logic_error("implement me!!!");
        // return std::visit(*this, lit);
    }

    auto operator()(Statement const &stm) const -> std::optional<StatementVec> {
        static_cast<void>(stm);
        throw std::logic_error("implement me!!!");
        // return std::visit(*this, stm);
    }

    RewriteContext const &ctx;
};

} // namespace

[[nodiscard]] auto unpool(RewriteContext &ctx, Literal const &lit, bool head) -> std::optional<LiteralVecVec> {
    return UnpoolRelations{ctx}(lit, head);
}

[[nodiscard]] auto unpool(RewriteContext &ctx, HeadLiteral const &lit) -> std::optional<HeadLiteralVecVec> {
    return UnpoolRelations{ctx}(lit);
}

[[nodiscard]] auto unpool(RewriteContext &ctx, BodyLiteral const &lit) -> std::optional<BodyLiteralVecVec> {
    return UnpoolRelations{ctx}(lit);
}

[[nodiscard]] auto unpool(RewriteContext &ctx, Statement const &stm) -> std::optional<StatementVec> {
    return UnpoolRelations{ctx}(stm);
}

} // namespace Gringo::Input
