#include "util.hh"

namespace Clingo::Python {

auto string_builder() -> clingo_string_builder_t * {
    struct free_builder {
        void operator()(clingo_string_builder_t const *bld) { clingo_string_builder_free(bld); }
    };
    thread_local static std::unique_ptr<clingo_string_builder_t, free_builder> builder;
    if (builder == nullptr) {
        clingo_string_builder_t *bld = nullptr;
        handle_error(clingo_string_builder_new(&bld));
        builder.reset(bld);
    } else {
        clingo_string_builder_clear(builder.get());
    }
    return builder.get();
}

} // namespace Clingo::Python
