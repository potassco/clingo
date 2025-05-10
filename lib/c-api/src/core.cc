#include "core.hh"
#include "lib.hh"

#include <clingo/core/location.hh>

#include <cstring>
#include <mutex>

extern "C" void clingo_version(int *major, int *minor, int *revision) {
    *major = CLINGO_VERSION_MAJOR;
    *minor = CLINGO_VERSION_MINOR;
    *revision = CLINGO_VERSION_REVISION;
}

extern "C" void clingo_result_string(clingo_result_t code, clingo_string_t *value) {
    auto str = [code]() -> std::string_view {
        switch (static_cast<clingo_result_e>(code)) {
            case clingo_result_success: {
                return "success";
            }
            case clingo_result_runtime: {
                return "runtime error";
            }
            case clingo_result_bad_alloc: {
                return "bad allocation";
            }
            case clingo_result_logic: {
                return "logic error";
            }
            case clingo_result_invalid: {
                return "invalid arguments";
            }
            case clingo_result_range: {
                return "range error";
            }
        }
        return "unknown error";
    }();
    value->data = str.data();
    value->size = str.size();
}

static_assert(static_cast<int>(Clingo::MessageCode::trace) == clingo_message_trace);
static_assert(static_cast<int>(Clingo::MessageCode::debug) == clingo_message_debug);
static_assert(static_cast<int>(Clingo::MessageCode::info) == clingo_message_info);
static_assert(static_cast<int>(Clingo::MessageCode::info_operation_undefined) == clingo_message_operation_undefined);
static_assert(static_cast<int>(Clingo::MessageCode::info_atom_undefined) == clingo_message_atom_undefined);
static_assert(static_cast<int>(Clingo::MessageCode::info_file_included) == clingo_message_file_included);
static_assert(static_cast<int>(Clingo::MessageCode::info_global_variable) == clingo_message_global_variable);
static_assert(static_cast<int>(Clingo::MessageCode::warn) == clingo_message_warn);
static_assert(static_cast<int>(Clingo::MessageCode::error) == clingo_message_error);

extern "C" void clingo_message_string(clingo_message_t code, clingo_string_t *value) {
    auto str = [code]() -> std::string_view {
        switch (static_cast<clingo_message_e>(code)) {
            case clingo_message_trace: {
                return "trace";
            }
            case clingo_message_debug: {
                return "debug";
            }
            case clingo_message_info: {
                return "info";
            }
            case clingo_message_operation_undefined: {
                return "operation undefined";
            }
            case clingo_message_atom_undefined: {
                return "atom undefined";
            }
            case clingo_message_file_included: {
                return "file included";
            }
            case clingo_message_global_variable: {
                return "global variable";
            }
            case clingo_message_warn: {
                return "warning";
            }
            case clingo_message_error: {
                return "error";
            }
        }
        return "unknown message code";
    }();
    value->data = str.data();
    value->size = str.size();
}

extern "C" auto clingo_lib_new(clingo_lib_flags_t flags, clingo_log_level_t level, clingo_logger_t logger, void *data,
                               size_t limit, clingo_lib_t **lib) -> bool {
    try {
        *lib = nullptr;
        Clingo::Logger::Printer prt = nullptr;
        if (logger != nullptr) {
            prt = [logger, data](Clingo::MessageCode code, std::string_view msg) {
                logger(static_cast<clingo_message_t>(code), msg.data(), msg.size(), data);
            };
        }
        *lib = std::make_unique<clingo_lib>(Clingo::Logger{prt, limit},
                                            Clingo::make_symbol_store((flags & clingo_lib_flags_slotted) != 0,
                                                                      (flags & clingo_lib_flags_shared) != 0),
                                            data, (flags & clingo_lib_flags_fast_release) != 0)
                   .release();
        (*lib)->log.set_level(static_cast<Clingo::LogLevel>(level));
    }
    CLINGO_CATCH;
}

extern "C" void clingo_lib_acquire(clingo_lib_t *lib) {
    if (lib != nullptr) {
        ++lib->ref_count;
    }
}

extern "C" void clingo_lib_release(clingo_lib_t *lib) {
    if (lib == nullptr || --lib->ref_count > 0) {
        return;
    }
    if (lib->fast_release) {
        // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
        delete lib;
        return;
    }
    static std::mutex gc_mut;
    static auto *lst = static_cast<clingo_lib_t *>(nullptr);
    if (lib != nullptr) {
        // reset logger and scripts in case they are holding symbols
        // the store might be kept alive if it is still holding symbols
        lib->log = Clingo::Logger{};
        lib->scripts = Clingo::Control::Scripts{};
        auto res = lib->store->gc();
        if (get<0>(res) > 0 || get<1>(res) > 0) {
            auto lck = std::unique_lock(gc_mut);
            lib->next_ = lst;
            lst = lib;
        } else {
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
            delete lib;
        }
    }
    // Note that running the gc for the lib object two times is intended.
    // The current implementation needs two passes to free all symbols.
    auto lck = std::unique_lock(gc_mut);
    auto *cur = std::exchange(lst, nullptr);
    while (cur != nullptr) {
        auto *nxt = cur->next_;
        auto res = cur->store->gc();
        if (get<0>(res) > 0 || get<1>(res) > 0) {
            cur->next_ = lst;
            lst = cur;
        } else {
            // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
            delete cur;
        }
        cur = nxt;
    }
#ifdef CLINGO_DEBUG
    if (lst != nullptr) {
        fprintf(stderr, "warning: not all symbols have been freed before the library was deleted\n");
        fflush(stderr);
    }
#endif
}

extern "C" auto clingo_user_data_slot() -> size_t {
    static size_t slots = 0;
    return slots++;
}

extern "C" auto clingo_lib_set_user_data(clingo_lib_t *lib, size_t slot, void *data, void (*deleter)(void *data))
    -> bool {
    CLINGO_TRY {
        lib->user_data.resize(slot + 1);
        lib->user_data[slot] = std::unique_ptr<void, user_data_deleter>(data, user_data_deleter{deleter});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_lib_get_user_data(clingo_lib_t *lib, size_t slot) -> void * {
    if (lib->user_data.size() > slot) {
        return lib->user_data[slot].get();
    }
    return nullptr;
}

extern "C" void clingo_lib_report(clingo_lib_t *lib, clingo_message_t code, char const *message, size_t size) {
    auto c = static_cast<Clingo::MessageCode>(code);
    if (lib != nullptr && lib->log.check(c)) {
        Clingo::Report(lib->log, c).out() << std::string_view{message, size};
    }
}

// definition of position

extern "C" auto clingo_string_builder_new(clingo_string_builder_t **bld) -> bool {
    CLINGO_TRY {
        // NOLINTNEXTLINE
        *bld = c_cast(new Clingo::Util::OutputBuffer{});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_string_builder_copy(clingo_string_builder_t const *src, clingo_string_builder_t **dst) -> bool {
    CLINGO_TRY {
        auto oss = std::make_unique<Clingo::Util::OutputBuffer>();
        *oss << cpp_cast(src)->view();
        *dst = c_cast(oss.release());
    }
    CLINGO_CATCH;
}

extern "C" void clingo_string_builder_free(clingo_string_builder_t const *bld) {
    // NOLINTNEXTLINE
    delete cpp_cast(bld);
}

extern "C" auto clingo_string_builder_string(clingo_string_builder_t const *bld, clingo_string_t *value) -> bool {
    CLINGO_TRY {
        if (bld == nullptr || value == nullptr) {
            return fail_arguments();
        }
        auto *cpp_bld = cpp_cast(const_cast<clingo_string_builder_t *>(bld)); // NOLINT
        value->data = cpp_bld->c_str();
        value->size = cpp_bld->size();
    }
    CLINGO_CATCH;
}

extern "C" void clingo_string_builder_clear(clingo_string_builder_t *bld) {
    cpp_cast(bld)->reset();
}

// definition of position

static auto c_cast(Clingo::Position const *pos) -> clingo_position_t const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<clingo_position_t const *>(pos);
}

static auto cpp_cast(clingo_position_t const *pos) -> Clingo::Position const * {
    // NOLINTNEXTLINE
    return reinterpret_cast<Clingo::Position const *>(pos);
}

extern "C" auto clingo_position_new(clingo_lib_t *lib, char const *file, size_t size, size_t line, size_t column,
                                    clingo_position_t const **pos) -> bool {
    CLINGO_TRY {
        // NOLINTNEXTLINE
        *pos = c_cast(new Clingo::Position{*lib->store->string(std::string_view{file, size}), line, column});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_position_copy(clingo_position_t const *src, clingo_position_t const **dst) -> bool {
    CLINGO_TRY {
        // NOLINTNEXTLINE
        *dst = src != nullptr ? c_cast(new Clingo::Position{*cpp_cast(src)}) : nullptr;
    }
    CLINGO_CATCH;
}

extern "C" void clingo_position_free(clingo_position_t const *pos) {
    // NOLINTNEXTLINE
    delete cpp_cast(pos);
}

extern "C" void clingo_position_file(clingo_position_t const *pos, clingo_string_t *value) {
    auto str = cpp_cast(pos)->file().view();
    value->data = str.data();
    value->size = str.size();
}

extern "C" auto clingo_position_line(clingo_position_t const *pos) -> size_t {
    return cpp_cast(pos)->line();
}

extern "C" auto clingo_position_column(clingo_position_t const *pos) -> size_t {
    return cpp_cast(pos)->column();
}

extern "C" auto clingo_position_hash(clingo_position_t const *pos) -> size_t {
    const auto *p = cpp_cast(pos);
    return Clingo::Util::value_hash_record<Clingo::Position>(p->file(), p->line(), p->column());
}

extern "C" auto clingo_position_equal(clingo_position_t const *a, clingo_position_t const *b) -> bool {
    return *cpp_cast(a) == *cpp_cast(b);
}

extern "C" auto clingo_position_compare(clingo_position_t const *a, clingo_position_t const *b) -> int {
    return c_cast(*cpp_cast(a) <=> *cpp_cast(b));
}

extern "C" auto clingo_position_to_string(clingo_position_t const *pos, clingo_string_builder_t *str) -> bool {
    CLINGO_TRY {
        *cpp_cast(str) << *cpp_cast(pos);
    }
    CLINGO_CATCH;
}

// definition of location

extern "C" auto clingo_location_new(clingo_position_t const *begin, clingo_position_t const *end,
                                    clingo_location_t const **loc) -> bool {
    CLINGO_TRY {
        // NOLINTNEXTLINE
        *loc = c_cast(new Clingo::Location{*cpp_cast(begin), *cpp_cast(end)});
    }
    CLINGO_CATCH;
}

extern "C" auto clingo_location_copy(clingo_location_t const *src, clingo_location_t const **dst) -> bool {
    CLINGO_TRY {
        // NOLINTNEXTLINE
        *dst = src != nullptr ? c_cast(new Clingo::Location{*cpp_cast(src)}) : nullptr;
    }
    CLINGO_CATCH;
}

extern "C" void clingo_location_free(clingo_location_t const *loc) {
    // NOLINTNEXTLINE
    delete cpp_cast(loc);
}

extern "C" auto clingo_location_begin(clingo_location_t const *loc) -> clingo_position_t const * {
    return c_cast(&cpp_cast(loc)->begin());
}

extern "C" auto clingo_location_end(clingo_location_t const *loc) -> clingo_position_t const * {
    return c_cast(&cpp_cast(loc)->end());
}

extern "C" auto clingo_location_hash(clingo_location_t const *loc) -> size_t {
    auto const *l = cpp_cast(loc);
    return Clingo::Util::value_hash_record<Clingo::Location>(clingo_position_hash(c_cast(&l->begin())),
                                                             clingo_position_hash(c_cast(&l->end())));
}

extern "C" auto clingo_location_equal(clingo_location_t const *a, clingo_location_t const *b) -> bool {
    return *cpp_cast(a) == *cpp_cast(b);
}

extern "C" auto clingo_location_compare(clingo_location_t const *a, clingo_location_t const *b) -> int {
    return c_cast(*cpp_cast(a) <=> *cpp_cast(b));
}

extern "C" auto clingo_location_to_string(clingo_location_t const *loc, clingo_string_builder_t *str) -> bool {
    CLINGO_TRY {
        *cpp_cast(str) << *cpp_cast(loc);
    }
    CLINGO_CATCH;
}
