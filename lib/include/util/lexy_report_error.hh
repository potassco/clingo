#pragma once

#include <lexy_ext/report_error.hpp>

namespace Gringo::Util {

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
//!
//! @todo Move into input/parser module.
template <typename OutputIt, typename Input, typename Reader, typename Tag>
auto write_error(OutputIt out, const lexy::error_context<Input> &context, const lexy::error<Reader, Tag> &error,
                 lexy::visualization_options opts, const char *path) -> OutputIt {
    lexy_ext::diagnostic_writer<Input> writer(context.input(), opts);

    using Counting = typename get_counting<Input>::type;

    // Convert the context location and error location into line/column
    // information.
    auto context_location =
        lexy::get_input_location<Counting>(context.input(), context.position(), get_anchor(context.input(), 0));
    auto location = lexy::get_input_location<Counting>(context.input(), error.position(), context_location.anchor());

    // Write the main error headline.
    out = writer.write_message(out, lexy_ext::diagnostic_kind::error, [&](OutputIt out, lexy::visualization_options) {
        out = lexy::_detail::write_str(out, "while parsing ");
        out = lexy::_detail::write_str(out, context.production());
        return out;
    });
    if (path != nullptr) {
        out = writer.write_path(out, path);
    }
    out = writer.write_empty_annotation(out);

    // Write an annotation for the context.
    if (location.line_nr() != context_location.line_nr()) {
        out = writer.write_annotation(
            out, lexy_ext::annotation_kind::secondary, context_location, lexy::_detail::next(context.position()),
            [&](OutputIt out, lexy::visualization_options) { return lexy::_detail::write_str(out, "beginning here"); });
        out = writer.write_empty_annotation(out);
    }

    // Write the main annotation.
    if constexpr (std::is_same_v<Tag, lexy::expected_literal>) {
        auto string = lexy::_detail::make_literal_lexeme<typename Reader::encoding>(error.string(), error.length());

        out = writer.write_annotation(out, lexy_ext::annotation_kind::primary, location, error.index() + 1,
                                      [&](OutputIt out, lexy::visualization_options opts) {
                                          out = lexy::_detail::write_str(out, "expected '");
                                          out = lexy::visualize_to(out, string, opts);
                                          out = lexy::_detail::write_str(out, "'");
                                          return out;
                                      });
    } else if constexpr (std::is_same_v<Tag, lexy::expected_keyword>) {
        auto string = lexy::_detail::make_literal_lexeme<typename Reader::encoding>(error.string(), error.length());

        out = writer.write_annotation(out, lexy_ext::annotation_kind::primary, location, error.end(),
                                      [&](OutputIt out, lexy::visualization_options opts) {
                                          out = lexy::_detail::write_str(out, "expected keyword '");
                                          out = lexy::visualize_to(out, string, opts);
                                          out = lexy::_detail::write_str(out, "'");
                                          return out;
                                      });
    } else if constexpr (std::is_same_v<Tag, lexy::expected_char_class>) {
        out = writer.write_annotation(out, lexy_ext::annotation_kind::primary, location, 1U,
                                      [&](OutputIt out, lexy::visualization_options) {
                                          out = lexy::_detail::write_str(out, "expected ");
                                          out = lexy::_detail::write_str(out, error.name());
                                          return out;
                                      });
    } else {
        out = writer.write_annotation(
            out, lexy_ext::annotation_kind::primary, location, error.end(),
            [&](OutputIt out, lexy::visualization_options) { return lexy::_detail::write_str(out, error.message()); });
    }

    return out;
}

//! An error reporter outputting to the given iterator.
template <typename OutputIterator = int> class report_error_ {
  public:
    //! Construct a reporter for errors.
    constexpr report_error_(OutputIterator iter = {}, char const *path = nullptr) : iter_{iter}, path_{path} {}

    //! Get the corresponding error sink.
    [[nodiscard]] constexpr auto sink() const { return sink_{iter_, opts_, path_, 0}; }

    //! Specifies a path that will be printed alongside the diagnostic.
    constexpr auto path(const char *path) const -> report_error_ { return {iter_, opts_, path}; }

    //! Specifies an output iterator where the errors are written to.
    template <typename OI> constexpr auto to(OI out) const -> report_error_<OI> { return {out, opts_, path_}; }

    //! Overrides visualization options.
    [[nodiscard]] constexpr auto opts(lexy::visualization_options opts) const -> report_error_ {
        return {iter_, opts, path_};
    }

  private:
    OutputIterator iter_;
    lexy::visualization_options opts_;
    const char *path_ = nullptr;

    struct sink_ {
        OutputIterator iter_;
        lexy::visualization_options opts_;
        const char *path_;
        std::size_t _count;

        using return_type = std::size_t;

        template <typename Input, typename Reader, typename Tag>
        void operator()(const lexy::error_context<Input> &context, const lexy::error<Reader, Tag> &error) {
            if constexpr (std::is_same_v<OutputIterator, int>) {
                write_error(lexy::cfile_output_iterator{stderr}, context, error, opts_, path_);
            } else {
                iter_ = write_error(iter_, context, error, opts_, path_);
            }
            ++_count;
        }

        auto finish() && -> std::size_t {
            if (_count != 0) {
                std::fputs("\n", stderr);
            }
            return _count;
        }
    };
};

//! An error callback that uses write_error() to print to stderr (by default).
//!
//! @related report_error_
constexpr auto report_error = report_error_{};

//! @}

} // namespace Gringo::Util
