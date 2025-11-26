#pragma once

#include <clingo/core.hh>

#include <clingo/profile.h>

#include <variant>
#include <vector>

namespace Clingo {

//! @addtogroup cpp_profile
//! Functions to profile the grounding process.
//!
//! @{

//! Enumeration of the types of profiling data.
enum class ProfileType : clingo_profile_type_t {
    step = clingo_profile_type_step, //!< Indicate per step profiling data.
    accu = clingo_profile_type_accu, //!< Indicate accumulated profiling data.
};

//! Class to hold profiling data for an expression in a logic program.
struct ProfileNodeLeaf {
    //! Constructs a profile leaf node with the given type and profiling data.
    //!
    //! @param type the type of the profiling data
    //! @param matches the number of matches for the expression
    //! @param instances the number of instances of the expression
    //! @param time_instantiate the time spent instantiating the expression
    //! @param time_propagate the time spent propagating the expression
    ProfileNodeLeaf(ProfileType type, uint64_t matches = 0, uint64_t instances = 0, uint64_t time_instantiate = 0,
                    uint64_t time_propagate = 0)
        : type{type}, matches{matches}, instances{instances}, time_instantiate{time_instantiate},
          time_propagate{time_propagate} {}

    ProfileType type;          //!< The type of the profiling data.
    uint64_t matches;          //!< The number of matches for the expression.
    uint64_t instances;        //!< The number of instances of the expression.
    uint64_t time_instantiate; //!< The time spent instantiating the expression.
    uint64_t time_propagate;   //!< The time spent propagating the expression.
};

struct ProfileNodeInternal;
//! A profile node that can be either an internal node or a leaf node.
using ProfileNode = std::variant<ProfileNodeInternal, ProfileNodeLeaf>;

//! Class to hold profiling data for an expression in a logic program.
struct ProfileNodeInternal {
    //! Constructs a profile internal node with the given key.
    //!
    //! @param key the key of the profile node
    //! @param nested whether times are included in the parent
    ProfileNodeInternal(std::string key, bool nested) : key{std::move(key)}, nested{nested} {}

    //! The key of the profile node.
    std::string key;
    //! Whether times are included in the parent.
    bool nested;
    //! The children of the profile node.
    std::vector<ProfileNode> children;
};

//! @}

} // namespace Clingo
