#include "transform.hh"

#include <gringo/input/algo/rewrite_context.hh>

#include <gringo/input/algo/analyze.hh>
#include <gringo/input/algo/print.hh>

namespace Gringo::Input {

namespace {

//! Parser for theory terms.
class ParseTheoryTerm : public Transformer<ParseTheoryTerm> {
  public:
    ParseTheoryTerm(Logger &log, TheoryTermParser const &parser) : log_{log}, parser_{parser} {}

    [[nodiscard]] auto accept(TheoryTermFunction const &term) const -> std::optional<TheoryTerm> {
        auto arity = std::optional<Arity>{};
        if (term.args().size() == 1) {
            arity = Arity::unary;
        } else if (term.args().size() == 2) {
            arity = Arity::binary;
        }
        if (arity && is_theory_operator(term.name().view())) {
            parser_.check_operator(log_, term.name(), *arity, term.loc());
        }

        return rewrite(term, a_args);
    }

    [[nodiscard]] auto accept(TheoryTermUnparsed const &term) const -> std::optional<TheoryTerm> {
        return parser_.parse(log_, term);
    }

  private:
    Logger &log_;
    TheoryTermParser const &parser_;
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
    if (!table_.try_emplace(std::pair(def.op(), arity), def.prio(), assoc).second) {
        GRINGO_REPORT_LOC(log, error, def.loc()) << "duplicate operator definition `" << def.op() << "`";
    }
}

void TheoryTermParser::check_operator(Logger &log, String op, Arity arity, Location loc) const {
    if (!table_.contains(std::pair(op, arity))) {
        GRINGO_REPORT_LOC(log, error, loc) << "cannot parse operator `" << op << "`";
    }
}

auto TheoryTermParser::parse(Logger &log, TheoryTermUnparsed const &term) const -> TheoryTerm {
    stack_.clear();
    terms_.clear();

    auto arity = Arity::unary;
    for (auto const &elem : term.elems()) {
        for (auto const &op : elem.first) {
            check_operator(log, op, arity, term.loc());

            while (arity == Arity::binary && check_(op)) {
                reduce_();
            }

            stack_.emplace_back(op, arity);
            arity = Arity::unary;
        }

        terms_.emplace_back(elem.second);
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
    auto previous_priority = priority_(stack_.back().first, stack_.back().second);
    return previous_priority > priority || (previous_priority == priority && associativity == Associativity::left);
}

void TheoryTermParser::reduce_() const {
    auto b = std::move(terms_.back());
    auto loc = location(b);
    terms_.pop_back();
    auto [op, arity] = std::move(stack_.back());
    stack_.pop_back();
    if (arity == Arity::unary) {
        terms_.emplace_back(TheoryTermFunction{loc, op, Util::make_immutable_array<TheoryTerm>(std::move(b))});
    } else {
        auto a = std::move(terms_.back());
        loc = location(a) + loc;
        terms_.pop_back();
        terms_.emplace_back(
            TheoryTermFunction{loc, op, Util::make_immutable_array<TheoryTerm>(std::move(a), std::move(b))});
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
            GRINGO_REPORT_LOC(log, error, term_def.loc()) << "duplicate term definition `" << term_def.name() << "`";
        }
    }
    atom_table_.reserve(atom_table_.size() + stm.atom_defs().size());
    for (auto const &atom_def : stm.atom_defs()) {
        auto guard = std::optional<GuardTable>{};
        if (atom_def.rhs()) {
            auto const &[ops, term] = *atom_def.rhs();
            if (auto it = term_defs.find(term); it != term_defs.end()) {
                guard.emplace(StringSet{ops.begin(), ops.end()}, std::distance(term_defs.begin(), it));
            } else {
                GRINGO_REPORT_LOC(log, error, atom_def.loc()) << "term definition not found `" << term << "`";
            }
        }
        if (auto it = term_defs.find(atom_def.term()); it != term_defs.end()) {
            if (!atom_table_
                     .try_emplace(std::pair{atom_def.name(), atom_def.arity()}, atom_def.type(),
                                  std::distance(term_defs.begin(), it), std::move(guard))
                     .second) {
                GRINGO_REPORT_LOC(log, error, atom_def.loc())
                    << "duplicate atom definition `" << atom_def.name() << "/" << atom_def.arity() << "`";
            }
        } else {
            GRINGO_REPORT_LOC(log, error, atom_def.loc()) << "term definition not found `" << atom_def.term() << "`";
        }
    }
}

//! Parse the given theory atom.
template <bool has_sign>
auto TheoryAtomParser::parse(Logger &log, TheoryAtom<has_sign> const &atom, bool fact) const
    -> std::optional<TheoryAtom<has_sign>> {
    auto [name, arity, sign] = signature(atom.name()).value();
    static_cast<void>(fact);
    if (sign) {
        throw std::runtime_error("invalid theory atom");
    }
    auto it = atom_table_.find(std::pair{name, arity});
    if (it == atom_table_.end()) {
        GRINGO_REPORT_LOC(log, error, atom.loc()) << "atom definition not found `" << name << "/" << arity << "`";
        // maybe clear guard and elems...
        return std::nullopt;
    }
    auto const &[type, index, guard] = it->second;
    if constexpr (has_sign) {
        if (type == TheoryAtomType::head || type == TheoryAtomType::directive) {
            GRINGO_REPORT_LOC(log, error, atom.loc()) << "theory atom may only occur in head";
            // maybe clear guard and elems...
            return std::nullopt;
        }
    } else {
        if (type == TheoryAtomType::body) {
            GRINGO_REPORT_LOC(log, error, atom.loc()) << "theory atom may only occur in body";
            // maybe clear guard and elems...
            return std::nullopt;
        }
        if (type == TheoryAtomType::directive && !fact) {
            GRINGO_REPORT_LOC(log, error, atom.loc()) << "theory atom must be a directive";
            // maybe clear guard and elems...
            return std::nullopt;
        }
    }
    auto elems = ParseTheoryTerm{log, term_parsers_[index]}.transform(atom.elems());
    auto rhs = std::optional<TheoryRGuard>{};
    if (atom.rhs()) {
        if (!guard) {
            GRINGO_REPORT_LOC(log, error, atom.loc()) << "unexpected guard in theory atom";
            // maybe clear guard and elems...
            return std::nullopt;
        }
        auto const &[guard_set, guard_index] = *guard;
        if (!guard_set.contains(atom.rhs()->first)) {
            GRINGO_REPORT_LOC(log, error, atom.loc()) << "unexpected guard in theory atom";
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

} // namespace Gringo::Input
