#pragma once

#include <gringo/input/statement.hh>

namespace Gringo::Input {

//! Helper class to attach state information to a reader.
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

} // namespace Gringo::Input
