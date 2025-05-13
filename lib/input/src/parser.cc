#include <clingo/input/parser.hh>

#include <clingo/input/rewrite/check_syntax.hh>

#include "parse/parser_state.hh"

namespace CppClingo::Input {

namespace {

template <class C> auto check_true([[maybe_unused]] Logger &log, [[maybe_unused]] C const &expr) -> bool {
    return true;
}

template <class P, class C>
auto parse_expr(Parse::ParserState &state, Parse::Condition cond, P parse, C check)
    -> std::invoke_result_t<P, Parse::ParserState &> {
    auto lock = GCLock{state.store()};
    state.condition(cond);
    state.consume();
    if (auto lit = std::invoke(parse, state); lit && std::invoke(check, state.log(), *lit)) {
        if (!state.branch(Parse::TokenType::end)) {
            return state.expected<std::nullopt>(Parse::TokenType::end);
        }
        return lit;
    }
    return std::nullopt;
}

} // namespace

Parser::Parser(Logger &log, SymbolStore &store, ProgramBackend *prg_backend, TheoryBackend *thy_backend)
    : impl_{std::make_unique<Parse::ParserState>(log, store, prg_backend, thy_backend)} {
}

Parser::Parser(Parser &&other) noexcept = default;

auto Parser::operator=(Parser &&other) noexcept -> Parser & = default;

Parser::~Parser() noexcept = default;

void Parser::init(std::istream &in, String file) {
    impl_->init(in, file);
}

void Parser::init(std::string_view in, String file) {
    impl_->init(in, file);
}

auto Parser::parse_symbol() -> std::optional<SharedSymbol> {
    return parse_expr(*impl_, Parse::Condition::normal, Parse::parse_symbol, check_true<SharedSymbol>);
}

auto Parser::parse_const_def() -> std::optional<std::pair<SharedString, SharedSymbol>> {
    return parse_expr(*impl_, Parse::Condition::normal, Parse::parse_const_def,
                      check_true<std::pair<SharedString, SharedSymbol>>);
}

auto Parser::parse_program_parts() -> std::optional<ProgramParamVec> {
    return parse_expr(
        *impl_, Parse::Condition::normal,
        [](Parse::ParserState &state) { return Parse::parse_program_parts(state, Parse::TokenType::end); },
        check_true<ProgramParamVec>);
}

auto Parser::parse_term() -> std::optional<Term> {
    return parse_expr(*impl_, Parse::Condition::normal, Parse::parse_term, check_term);
}

auto Parser::parse_theory_term() -> std::optional<TheoryTerm> {
    return parse_expr(*impl_, Parse::Condition::theory, Parse::parse_theory_term, check_true<TheoryTerm>);
}

auto Parser::parse_literal() -> std::optional<Lit> {
    return parse_expr(*impl_, Parse::Condition::normal, Parse::parse_literal, check_literal);
}

auto Parser::parse_body_literal() -> std::optional<BdLit> {
    return parse_expr(*impl_, Parse::Condition::normal, Parse::parse_body_literal, check_body_literal);
}

auto Parser::parse_head_literal() -> std::optional<HdLit> {
    return parse_expr(*impl_, Parse::Condition::normal, Parse::parse_head_literal, check_head_literal);
}

auto Parser::parse_statement() -> std::optional<Stm> {
    return parse_expr(*impl_, Parse::Condition::normal, Parse::parse_statement, check_statement);
}

auto Parser::scan() -> std::pair<std::optional<Stm>, bool> {
    if (impl_->token() == Parse::TokenType::begin) {
        impl_->consume();
        if (impl_->token() == Parse::TokenType::aspif) {
            return {std::nullopt, parse_aspif(*impl_)};
        }
    }
    if (impl_->has_stms()) {
        return {impl_->pop_stm(), true};
    }
    auto [stm, res] = scan_statement(*impl_);
    while (true) {
        // ensure stored statements are reported
        if (!stm) {
            impl_->mark_stms();
            return {impl_->pop_stm(), res};
        }
        // discard illformed statements
        if (!check_statement(impl_->log(), *stm)) {
            std::tie(stm, std::ignore) = scan_statement(*impl_);
            res = false;
            continue;
        }
        // report stored statements first
        if (impl_->has_stms()) {
            impl_->push_stm(*std::move(stm));
            return {impl_->pop_stm(), res};
        }
        // report statement just parsed
        return {std::move(stm), res};
    }
}

} // namespace CppClingo::Input
