#pragma once

//! @file
//! This file contains helpers to write out error messages during parsing.

#include <lexy_ext/report_error.hpp>

namespace Gringo::Util {

template <class Input, class Void = void> struct get_counting {
    using type = lexy::_default_location_counting<Input>;
};

template <class Input> struct get_counting<Input, std::void_t<typename Input::counting>> {
    using type = typename Input::counting;
};

template <class Input> auto get_anchor(Input const &input, int /*unused*/) -> decltype(input.anchor()) {
    return input.anchor();
}

template <class Input> auto get_anchor(Input const &input, long /*unused*/) {
    return lexy::input_location_anchor{input};
}

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

template <typename OutputIterator = int> struct _report_error {
    OutputIterator _iter;
    lexy::visualization_options _opts;
    const char *_path;

    struct _sink {
        OutputIterator _iter;
        lexy::visualization_options _opts;
        const char *_path;
        std::size_t _count;

        using return_type = std::size_t;

        template <typename Input, typename Reader, typename Tag>
        void operator()(const lexy::error_context<Input> &context, const lexy::error<Reader, Tag> &error) {
            if constexpr (std::is_same_v<OutputIterator, int>) {
                write_error(lexy::cfile_output_iterator{stderr}, context, error, _opts, _path);
            } else {
                _iter = write_error(_iter, context, error, _opts, _path);
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
    [[nodiscard]] constexpr auto sink() const { return _sink{_iter, _opts, _path, 0}; }

    /// Specifies a path that will be printed alongside the diagnostic.
    constexpr auto path(const char *path) const -> _report_error { return {_iter, _opts, path}; }

    /// Specifies an output iterator where the errors are written to.
    template <typename OI> constexpr auto to(OI out) const -> _report_error<OI> { return {out, _opts, _path}; }

    /// Overrides visualization options.
    [[nodiscard]] constexpr auto opts(lexy::visualization_options opts) const -> _report_error {
        return {_iter, opts, _path};
    }
};

/// An error callback that uses diagnostic_writer to print to stderr (by default).
constexpr auto report_error = _report_error{};

} // namespace Gringo::Util
