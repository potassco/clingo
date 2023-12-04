#pragma once

#include <sstream>

#include <logger.hh>

#include <lexy_ext/report_error.hpp>

namespace Gringo::Input {

//! @addtogroup util
//! @{

//! Get the default counting strategy if the input does not define one.
//!
//! @related report_error_
template <class Input, class Void = void> struct get_counting {
    //! The counting strategy.
    using type = lexy::_default_location_counting<Input>;
};

//! Get the counting strategy if the input defines one.
//!
//! @related report_error_
template <class Input> struct get_counting<Input, std::void_t<typename Input::counting>> {
    //! The counting strategy.
    using type = typename Input::counting;
};

//! Get an anchor if the input does not define one.
//!
//! @related report_error_
template <class Input> auto get_anchor(Input const &input, long /*unused*/) {
    return lexy::input_location_anchor{input};
}

//! Get the current anchor if the input defines one.
//!
//! @related report_error_
template <class Input> auto get_anchor(Input const &input, int /*unused*/) -> decltype(input.anchor()) {
    return input.anchor();
}

//! Write out an error message.
//!
//! @related report_error_
template <typename Input, typename Reader, typename Tag>
void write_error(Gringo::Logger &log, const lexy::error_context<Input> &context, const lexy::error<Reader, Tag> &error,
                 lexy::visualization_options opts, const char *path) {
    lexy_ext::diagnostic_writer<Input> writer(context.input(), opts);

    using Counting = typename get_counting<Input>::type;

    // Convert the context location and error location into line/column
    // information.
    auto context_location =
        lexy::get_input_location<Counting>(context.input(), context.position(), get_anchor(context.input(), 0));
    auto location = lexy::get_input_location<Counting>(context.input(), error.position(), context_location.anchor());

    std::ostringstream oss;
    auto out = std::ostreambuf_iterator{oss};

    // Write the main error headline.
    out = lexy::_detail::write_str(out, "while parsing ");
    out = lexy::_detail::write_str(out, context.production());
    *out++ = ':';
    *out++ = '\n';
    if (path != nullptr) {
        out = writer.write_path(out, path);
    }
    out = writer.write_empty_annotation(out);

    // Write an annotation for the context.
    if (location.line_nr() != context_location.line_nr()) {
        out = writer.write_annotation(
            out, lexy_ext::annotation_kind::secondary, context_location, lexy::_detail::next(context.position()),
            [&](auto out, lexy::visualization_options) { return lexy::_detail::write_str(out, "beginning here"); });
        out = writer.write_empty_annotation(out);
    }

    // Write the main annotation.
    if constexpr (std::is_same_v<Tag, lexy::expected_literal>) {
        auto string = lexy::_detail::make_literal_lexeme<typename Reader::encoding>(error.string(), error.length());

        out = writer.write_annotation(out, lexy_ext::annotation_kind::primary, location, error.index() + 1,
                                      [&](auto out, lexy::visualization_options opts) {
                                          out = lexy::_detail::write_str(out, "expected '");
                                          out = lexy::visualize_to(out, string, opts);
                                          out = lexy::_detail::write_str(out, "'");
                                          return out;
                                      });
    } else if constexpr (std::is_same_v<Tag, lexy::expected_keyword>) {
        auto string = lexy::_detail::make_literal_lexeme<typename Reader::encoding>(error.string(), error.length());

        out = writer.write_annotation(out, lexy_ext::annotation_kind::primary, location, error.end(),
                                      [&](auto out, lexy::visualization_options opts) {
                                          out = lexy::_detail::write_str(out, "expected keyword '");
                                          out = lexy::visualize_to(out, string, opts);
                                          out = lexy::_detail::write_str(out, "'");
                                          return out;
                                      });
    } else if constexpr (std::is_same_v<Tag, lexy::expected_char_class>) {
        out = writer.write_annotation(out, lexy_ext::annotation_kind::primary, location, 1U,
                                      [&](auto out, lexy::visualization_options) {
                                          out = lexy::_detail::write_str(out, "expected ");
                                          out = lexy::_detail::write_str(out, error.name());
                                          return out;
                                      });
    } else {
        out = writer.write_annotation(
            out, lexy_ext::annotation_kind::primary, location, error.end(),
            [&](auto out, lexy::visualization_options) { return lexy::_detail::write_str(out, error.message()); });
    }
    GRINGO_REPORT(log, error) << oss.str();
}

//! An error reporter outputting to the given iterator.
class report_error {
  public:
    //! Construct a reporter for errors.
    constexpr report_error(Gringo::Logger &log, char const *path = nullptr) : log_{log}, path_{path} {}

    //! Get the corresponding error sink.
    [[nodiscard]] constexpr auto sink() const { return sink_{log_, opts_, path_, 0}; }

  private:
    Gringo::Logger &log_;
    lexy::visualization_options opts_;
    const char *path_ = nullptr;

    struct sink_ {
        Gringo::Logger &log_;
        lexy::visualization_options opts_;
        const char *path_;
        std::size_t _count;

        using return_type = std::size_t;

        template <typename Input, typename Reader, typename Tag>
        void operator()(const lexy::error_context<Input> &context, const lexy::error<Reader, Tag> &error) {
            write_error(log_, context, error, opts_, path_);
            ++_count;
        }

        [[nodiscard]] auto finish() const && -> std::size_t {
            if (_count != 0) {
                std::fputs("\n", stderr);
            }
            return _count;
        }
    };
};

//! @}

} // namespace Gringo::Input
