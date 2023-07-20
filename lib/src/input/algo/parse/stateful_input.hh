#pragma once

#include <cassert>
#include <queue>
#include <string>
#include <utility>

namespace Gringo::Input {

//! @defgroup parser Parser
//! @ingroup language
//!
//! Data structures and functions for parsing.
//!
//! @{

//! Helper class to attach state information to a reader.
//!
//! @todo Move into separate header.
template <typename Reader, typename State> class StatefulReader : public Reader {
  public:
    //! Construct a stateful reader.
    explicit StatefulReader(Reader reader, State &state) : Reader{std::move(reader)}, state_{&state} {};

    //! Return the state associated with the reader.
    [[nodiscard]] auto state() const -> State & { return *state_; }

  private:
    State *state_;
};

//! Helper class to attach state information to an input.
//!
//! @todo Move into separate header.
template <typename Input, typename State> class StatefulInput {
  public:
    //! Construct a stateful input.
    StatefulInput(Input &input, State &state) : input_{input}, state_{&state} {}

    //! Return the state associated with the input.
    [[nodiscard]] auto reader() const & { return StatefulReader(input_.reader(), *state_); }

  private:
    Input &input_;
    State *state_;
};

//! Helper class to track comments.
class Comments {
  public:
    //! Add a comment.
    void push(std::string comment) { comments_.push(std::move(comment)); }

    //! Mark all currently available comments for popping.
    void mark() { mark_ = comments_.size(); }

    //! Check if a comment is available.
    [[nodiscard]] auto empty() const -> bool { return mark_ == 0; }

    //! Pop the last comment.
    auto pop() -> std::string {
        assert(mark_ > 0);
        auto ret = std::move(comments_.front());
        comments_.pop();
        --mark_;
        return ret;
    }

  private:
    std::queue<std::string> comments_;
    size_t mark_ = 0;
};

//! @}

} // namespace Gringo::Input
