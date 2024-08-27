#include <gringo/ground/assignment_aggregate.hh>

#include <gringo/util/type_traits.hh>

namespace Gringo::Ground {

// definition of AtomAssignAggr

auto AtomAssignAggr::init_(AggregateFunction fun) -> Values {
    return std::visit(
        []<class T>(T &&x) -> Values {
            Util::small_vector<std::remove_cvref_t<T>> ret;
            ret.emplace_back(std::forward<T>(x));
            return ret;
        },
        neutral_num(fun));
}

auto AtomAssignAggr::is_fact() const {
    return std::visit([](auto const &x) { return x.size() == 1; }, values_);
}

void AtomAssignAggr::accumulate(AggregateFunction fun, SymbolSpan tup, bool fact) {
    if (tup.empty()) {
        return;
    }
    auto const &val = tup.front();
    switch (fun) {
        case AggregateFunction::min:
        case AggregateFunction::max: {
            auto &vals = std::get<Util::small_vector<Symbol>>(values_);
            auto cmp = [lt = fun == AggregateFunction::min](auto const &a, auto const &b) {
                return lt ? a < b : b < a;
            };
            if (fact) {
                if (auto *it = std::lower_bound(vals.begin(), vals.end(), val, cmp); it != vals.end()) {
                    vals.erase(it, vals.end());
                    vals.emplace_back(val);
                }
            } else {
                if (auto *it = std::lower_bound(vals.begin(), vals.end(), val, cmp); it != vals.end() && *it != val) {
                    vals.emplace(it, val);
                }
            }
            break;
        }
        default: {
            if (tup.front().type() != SymbolType::number) {
                return;
            }
            auto const &num = tup.front().num();
            if (num == 0 || (fun == AggregateFunction::sump && num < 0)) {
                return;
            }
            auto &vals = std::get<Util::small_vector<Number>>(values_);
            if (fact) {
                propagated_vals_ = 0;
                for (auto &val : vals) {
                    val += num;
                }
            } else {
                auto m = std::ssize(vals);
                auto n = std::ssize(vals);
                auto p = static_cast<ssize_t>(propagated_vals_);
                for (auto i = ssize_t{0}; i < n; ++i) {
                    if (i == p) {
                        m = std::ssize(vals);
                    }
                    // the value to insert
                    auto iv = vals[i] + num;
                    // there are 4 sorted ranges:
                    // - [ib, ip) : values already propagated
                    // - [ip, in) : values previously inserted
                    // - [in, im) : fresh values from propagated ones
                    // - [im, ie) : fresh values from not yet propagated ones
                    auto *ib = vals.begin();
                    auto *ip = std::next(ib, p);
                    auto *in = std::next(ib, n);
                    auto *im = std::next(ib, m);
                    if (!std::binary_search(ib, ib, iv) && !std::binary_search(ip, in, iv) &&
                        !std::binary_search(in, im, iv) && vals.back() != num) {
                        vals.emplace_back(num);
                    }
                }
                std::sort(std::next(vals.begin(), p), vals.end());
                /*
                // alternative with better time complexity but allocation
                auto ie = vals.end();
                std::inplace_merge(ib + n, ib + m, ib + m, ie);
                std::inplace_merge(ib + p, ib + m, ib + m, ie);
                */
            }
        }
    }
}

auto AtomAssignAggr::enqueue_vals() -> bool {
    if (!enqueued_vals_ && propagated_vals_ < std::visit([](auto const &x) { return x.size(); }, values_)) {
        enqueued_vals_ = true;
        return true;
    }
    return false;
}

void AtomAssignAggr::dequeue_vals() {
    assert(enqueued_vals_);
    std::visit([](auto &x) { std::sort(x.begin(), x.end()); }, values_);
    enqueued_vals_ = false;
    propagated_vals_ = std::visit([](auto const &x) { return x.size(); }, values_);
}

auto AtomAssignAggr::todo_values() -> std::variant<NumberSpan, SymbolSpan> {
    return std::visit(
        [p = static_cast<ssize_t>(propagated_)](auto const &x) -> std::variant<NumberSpan, SymbolSpan> {
            return std::span{std::next(x.begin(), p), x.end()};
        },
        values_);
}

void AtomAssignAggr::add_elem(size_t idx) { elems_.emplace_back(idx); }

auto AtomAssignAggr::elems() const -> std::span<size_t const> { return std::span{elems_.begin(), elems_.end()}; }

auto AtomAssignAggr::enqueue() -> bool {
    if (!enqueued_ && propagated_ < elems_.size()) {
        enqueued_ = true;
        return true;
    }
    return false;
}

void AtomAssignAggr::dequeue() {
    assert(enqueued_);
    propagated_ = elems_.size();
    enqueued_ = false;
}

auto AtomAssignAggr::todo() -> std::span<size_t const> {
    return std::span{elems_.begin() + static_cast<ssize_t>(propagated_vals_), elems_.end()};
}

// definition of BaseAssignAggr

auto BaseAssignAggr::is_fact(Key sym) const -> bool {
    // the derived.contains check might be unnecessary
    return single_pass_elems_ && atoms_.nth(sym.first).value().is_fact() && derived_.contains(sym);
}

void BaseAssignAggr::add(size_t idx, Symbol val) { derived_.emplace(idx, val); }

auto BaseAssignAggr::size() const -> size_t { return derived_.size(); }

auto BaseAssignAggr::index(Key sym) const -> size_t { return derived_.find(sym) - derived_.begin(); }

auto BaseAssignAggr::nth(size_t i) const -> AtomSet::const_iterator { return derived_.nth(i); }

auto BaseAssignAggr::nth(size_t i) -> AtomSet::iterator { return derived_.nth(i); }

auto BaseAssignAggr::atoms() -> AtomMap & { return atoms_; }

} // namespace Gringo::Ground
