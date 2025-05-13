#include "transform.hh"

#include <clingo/input/rewrite/analyze.hh>
#include <clingo/input/rewrite/rewrite_theory.hh>

namespace CppClingo::Input {

namespace {

class ParseTheory : public Transformer<ParseTheory> {
  public:
    ParseTheory(Logger &log, TheoryAtomParser const &parser) : log_{&log}, parser_{&parser} {}

    template <bool has_sign>
    auto accept(TheoryAtom<has_sign> const &atom) const -> std::optional<TheoryAtom<has_sign>> {
        return parser_->parse(*log_, atom, fact_);
    }

    auto accept(StmRule const &stm) const -> std::optional<Stm> {
        fact_ = stm.body().empty();
        return rewrite(stm, a_head, a_body);
    }

  private:
    Logger *log_;
    TheoryAtomParser const *parser_;
    mutable bool fact_ = false;
};

} // namespace

auto rewrite_theory(RewriteContext &ctx, Stm const &stm) -> std::optional<Stm> {
    return ParseTheory{ctx.logger(), ctx.parser()}.transform(stm);
}

} // namespace CppClingo::Input
