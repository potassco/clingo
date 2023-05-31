#include <util/print.hh>

#include <aggregate.hh>

auto operator<<(std::ostream &out, AggregateFunction fun) -> std::ostream & {
    switch (fun) {
        case AggregateFunction::count: {
            out << "#count";
            break;
        }
        case AggregateFunction::sum: {
            out << "#sum";
            break;
        }
        case AggregateFunction::sump: {
            out << "#sum+";
            break;
        }
        case AggregateFunction::min: {
            out << "#min";
            break;
        }
        case AggregateFunction::max: {
            out << "#max";
            break;
        }
    }
    return out;
}

void SetAggregate::set_rhs(STerm lhs, Relation rel) { lhs_ = std::make_pair(std::move(lhs), rel); }

auto operator<<(std::ostream &out, SetAggregate const &aggr) -> std::ostream & {
    if (aggr.lhs_) {
        out << *aggr.lhs_->first << " " << aggr.lhs_->second << " ";
    }
    out << "{ " << p_range_with(aggr.elements_, "; ", [](std::ostream &out, auto const &elem) {
        out << *std::get<0>(elem);
        if (!std::get<1>(elem).empty()) {
            out << ": " << p_range{std::get<1>(elem), ", "};
        }
    }) << (aggr.elements_.empty() ? "}" : " }");
    if (aggr.rhs_) {
        out << " " << aggr.rhs_->first << " " << *aggr.rhs_->second;
    }
    return out;
}
