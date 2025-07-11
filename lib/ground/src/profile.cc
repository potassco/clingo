#include <clingo/ground/profile.hh>

#include <chrono>
#include <iomanip>

namespace CppClingo::Ground {

void ProfileStats::print(std::ostream &out, ProfileIndent indent) const {
    auto time = [](uint64_t time) { return std::chrono::duration<double>(std::chrono::nanoseconds(time)).count(); };
    out << indent << "matches:     " << matches << "\n"
        << indent << "instances:   " << instances << "\n"
        << std::fixed << std::setprecision(3) //
        << indent << "instantiate: " << time(time_instantiate) << "s\n"
        << indent << "propagate:   " << time(time_propagate) << "s\n";
}

} // namespace CppClingo::Ground
