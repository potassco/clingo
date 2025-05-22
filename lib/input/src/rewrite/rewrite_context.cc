#include "transform.hh"

#include <clingo/input/print.hh>

#include <clingo/input/rewrite/analyze.hh>
#include <clingo/input/rewrite/rewrite_context.hh>

namespace CppClingo::Input {

namespace {

//! Parser for theory terms.
class ParseTheoryTerm : public Transformer<ParseTheoryTerm> {
  public:
    ParseTheoryTerm(Logger &log, TheoryTermParser const &parser) : log_{&log}, parser_{&parser} {}

    [[nodiscard]] auto accept(TheoryTermFunction const &term) const -> std::optional<TheoryTerm> {
        auto arity = std::optional<Arity>{};
        if (term.args().size() == 1) {
            arity = Arity::unary;
        } else if (term.args().size() == 2) {
            arity = Arity::binary;
        }
        if (arity && is_theory_operator(term.name().view())) {
            parser_->check_operator(*log_, term.name(), *arity, term.loc());
        }

        return rewrite(term, a_args);
    }

    [[nodiscard]] auto accept(TheoryTermUnparsed const &term) const -> std::optional<TheoryTerm> {
        auto res = parser_->parse(*log_, term);
        auto ret = transform(res);
        return ret ? std::move(ret) : std::make_optional(std::move(res));
    }

  private:
    Logger *log_;
    TheoryTermParser const *parser_;
};

} // namespace

void TheoryTermParser::add(Logger &log, TheoryOpDefinition const &def) {
    auto arity = Arity::binary;
    auto assoc = Associativity::non_assoc;
    if (def.type() == TheoryOpType::binary_left) {
        assoc = Associativity::left;
    } else if (def.type() == TheoryOpType::binary_right) {
        assoc = Associativity::right;
    } else {
        arity = Arity::unary;
    }
    if (!table_.try_emplace(std::pair(SharedString(def.op()), arity), def.prio(), assoc).second) {
        CLINGO_REPORT_LOC(log, error, def.loc()) << "duplicate operator definition `" << def.op() << "`";
        has_error_ = true;
    }
}

void TheoryTermParser::check_operator(Logger &log, String op, Arity arity, Location const &loc) const {
    if (!table_.contains(std::pair(op, arity))) {
        CLINGO_REPORT_LOC(log, error, loc) << "cannot parse operator `" << op << "`";
        has_error_ = true;
    }
}

auto TheoryTermParser::parse(Logger &log, TheoryTermUnparsed const &term) const -> TheoryTerm {
    stack_.clear();
    terms_.clear();

    auto arity = Arity::unary;
    for (auto const &elem : term.elems()) {
        for (auto const &op : elem.ops()) {
            check_operator(log, op, arity, term.loc());

            while (arity == Arity::binary && check_(op)) {
                reduce_();
            }

            stack_.emplace_back(op, arity);
            arity = Arity::unary;
        }

        terms_.emplace_back(elem.term());
        arity = Arity::binary;
    }

    while (!stack_.empty()) {
        reduce_();
    }

    return std::move(terms_.back());
}

auto TheoryTermParser::priority_and_associativity_(String op) const -> std::pair<int, Associativity> {
    if (auto it = table_.find(std::pair(op, Arity::binary)); it != table_.end()) {
        return it->second;
    }
    return {0, Associativity::left};
}

auto TheoryTermParser::priority_(String op, Arity arity) const -> int {
    if (auto it = table_.find(std::pair(op, arity)); it != table_.end()) {
        return it->second.first;
    }
    return 0;
}

auto TheoryTermParser::check_(String op) const -> bool {
    if (stack_.empty()) {
        return false;
    }
    auto [priority, associativity] = priority_and_associativity_(op);
    auto previous_priority = priority_(*stack_.back().first, stack_.back().second);
    return previous_priority > priority || (previous_priority == priority && associativity == Associativity::left);
}

void TheoryTermParser::reduce_() const {
    auto b = std::move(terms_.back());
    auto loc = location(b);
    terms_.pop_back();
    auto [op, arity] = std::move(stack_.back());
    stack_.pop_back();
    if (arity == Arity::unary) {
        terms_.emplace_back(TheoryTermFunction{loc, *op, Util::make_immutable_array<TheoryTerm>(std::move(b))});
    } else {
        auto a = std::move(terms_.back());
        loc = location(a) + loc;
        terms_.pop_back();
        terms_.emplace_back(
            TheoryTermFunction{loc, *op, Util::make_immutable_array<TheoryTerm>(std::move(a), std::move(b))});
    }
}

void TheoryAtomParser::add_theory(Logger &log, StmTheory const &stm) {
    Util::ordered_set<String> term_defs;
    term_defs.reserve(stm.term_defs().size());
    term_parsers_.reserve(stm.term_defs().size());
    for (auto const &term_def : stm.term_defs()) {
        if (term_defs.insert(term_def.name()).second) {
            term_parsers_.emplace_back();
            for (auto const &op_def : term_def.op_defs()) {
                term_parsers_.back().add(log, op_def);
            }
        } else {
            CLINGO_REPORT_LOC(log, error, term_def.loc()) << "duplicate term definition `" << term_def.name() << "`";
            has_error_ = true;
        }
    }
    atom_table_.reserve(atom_table_.size() + stm.atom_defs().size());
    for (auto const &atom_def : stm.atom_defs()) {
        auto guard = std::optional<GuardTable>{};
        if (auto const &rhs = atom_def.rhs(); rhs) {
            if (auto it = term_defs.find(rhs->term()); it != term_defs.end()) {
                guard.emplace(SharedStringSet(rhs->ops().begin(), rhs->ops().end()),
                              std::distance(term_defs.begin(), it));
            } else {
                CLINGO_REPORT_LOC(log, error, atom_def.loc()) << "term definition not found `" << rhs->term() << "`";
                has_error_ = true;
            }
        }
        if (auto it = term_defs.find(atom_def.term()); it != term_defs.end()) {
            if (!atom_table_
                     .try_emplace(std::pair{atom_def.name(), static_cast<size_t>(atom_def.arity())}, atom_def.type(),
                                  std::distance(term_defs.begin(), it), std::move(guard))
                     .second) {
                CLINGO_REPORT_LOC(log, error, atom_def.loc())
                    << "duplicate atom definition `" << atom_def.name() << "/" << atom_def.arity() << "`";
                has_error_ = true;
            }
        } else {
            CLINGO_REPORT_LOC(log, error, atom_def.loc()) << "term definition not found `" << atom_def.term() << "`";
            has_error_ = true;
        }
    }
}

//! Parse the given theory atom.
template <bool has_sign>
auto TheoryAtomParser::parse(Logger &log, TheoryAtom<has_sign> const &atom, bool fact) const
    -> std::optional<TheoryAtom<has_sign>> {
    // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
    auto [name, arity, sign] = signature(atom.name()).value();
    static_cast<void>(fact);
    if (sign) {
        throw std::runtime_error("invalid theory atom");
    }
    auto it = atom_table_.find(std::pair{name, arity});
    if (it == atom_table_.end()) {
        CLINGO_REPORT_LOC(log, error, atom.loc()) << "atom definition not found `" << name << "/" << arity << "`";
        has_error_ = true;
        // maybe clear guard and elems...
        return std::nullopt;
    }
    auto const &[type, index, guard] = it->second;
    if constexpr (has_sign) {
        if (type == TheoryAtomType::head || type == TheoryAtomType::directive) {
            CLINGO_REPORT_LOC(log, error, atom.loc()) << "theory atom may only occur in head";
            has_error_ = true;
            // maybe clear guard and elems...
            return std::nullopt;
        }
    } else {
        if (type == TheoryAtomType::body) {
            CLINGO_REPORT_LOC(log, error, atom.loc()) << "theory atom may only occur in body";
            has_error_ = true;
            // maybe clear guard and elems...
            return std::nullopt;
        }
        if (type == TheoryAtomType::directive && !fact) {
            CLINGO_REPORT_LOC(log, error, atom.loc()) << "theory atom must be a directive";
            has_error_ = true;
            // maybe clear guard and elems...
            return std::nullopt;
        }
    }
    auto elems = ParseTheoryTerm{log, term_parsers_[index]}.transform(atom.elems());
    auto rhs = std::optional<TheoryRGuard>{};
    if (atom.rhs()) {
        if (!guard) {
            CLINGO_REPORT_LOC(log, error, atom.loc()) << "unexpected guard in theory atom";
            has_error_ = true;
            // maybe clear guard and elems...
            return std::nullopt;
        }
        auto const &[guard_set, guard_index] = *guard;
        if (!guard_set.contains(atom.rhs()->op())) {
            CLINGO_REPORT_LOC(log, error, atom.loc()) << "unexpected guard in theory atom";
            has_error_ = true;
            // maybe clear guard and elems...
            return std::nullopt;
        }
        rhs = ParseTheoryTerm{log, term_parsers_[guard_index]}.transform(*atom.rhs());
    }
    return atom.rewrite(a_elems = std::move(elems), a_rhs = std::move(rhs));
}
template auto TheoryAtomParser::parse(Logger &, TheoryAtom<true> const &, bool) const
    -> std::optional<TheoryAtom<true>>;
template auto TheoryAtomParser::parse(Logger &, TheoryAtom<false> const &, bool) const
    -> std::optional<TheoryAtom<false>>;

auto TheoryAtomParser::has_error() const -> bool {
    for (auto const &parser : term_parsers_) {
        if (parser.has_error()) {
            return true;
        }
    }
    return has_error_;
}

} // namespace CppClingo::Input
