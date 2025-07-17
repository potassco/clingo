#include <clingo/app.h>
#include <clingo/control.h>
#include <clingo/solve.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *program;
    size_t size;
} app_data_t;

static void program_name(void *data, clingo_string_t *string) {
    (void)data;
    string->data = "example";
    string->size = strlen(string->data);
}

static void version(void *data, clingo_string_t *string) {
    (void)data;
    string->data = "1.0.0";
    string->size = strlen(string->data);
}

static bool program_option_parser(char const *value, size_t size, void *data, bool *result) {
    char *str = NULL;
    str = (char *)malloc(size);
    if (str == NULL) {
        char const *msg = "failed to allocate memory for program option";
        clingo_set_error(clingo_result_bad_alloc, msg, strlen(msg));
        return false;
    }
    memcpy(str, value, size);

    app_data_t *app = (app_data_t *)data;
    free((void *)app->program);
    app->program = str;
    app->size = size;
    *result = true;
    return true;
}

static bool register_options(clingo_options_t *options, void *data) {
    char const *group = "Example";
    char const *desc = "Override the default program part to ground.";
    char const *option = "program";
    char const *argument = "NAME";
    return clingo_options_add(options, group, strlen(group), option, strlen(option), desc, strlen(desc),
                              program_option_parser, data, false, argument, strlen(argument));
}

static bool validate_options(void *data) {
    (void)data;
    // NOTE: Here compatibility checks can be performed. The example is valid
    // for all options.
    return true;
}

typedef bool (*clingo_model_printer_t)(clingo_model_t const *model, clingo_default_model_printer_t printer,
                                       void *printer_data, void *data);
static bool print_model(clingo_model_t const *model, clingo_default_model_printer_t printer, void *printer_data,
                        void *data) {
    (void)data;
    (void)model;
    // NOTE: Here model printing can be customized. We simply use the default
    // model printer here.
    return printer(printer_data);
}
static bool run(clingo_control_t *ctl, clingo_string_t const *files, size_t size, void *data) {
    app_data_t *app = (app_data_t *)data;
    char const *part = app->program ? app->program : "base";
    clingo_part_t part_obj = {part, app->program ? app->size : strlen(part), NULL, 0};

    if (!clingo_control_parse_files(ctl, files, size)) {
        return false;
    };
    if (!clingo_control_set_parts(ctl, &part_obj, 1, true)) {
        return false;
    }
    if (!clingo_control_main(ctl)) {
        return false;
    }

    return true;
}

int main(int argc, char *argv[]) {
    app_data_t app = {NULL, 0};
    clingo_lib_t *lib = NULL;
    clingo_application_t application = {program_name, version, run, print_model, register_options, validate_options};
    clingo_lib_flags_t flags = clingo_lib_flags_fast_release | clingo_lib_flags_slotted;
    clingo_log_level_t level = clingo_log_level_info;
    int code = EXIT_FAILURE;
    size_t const limit = 20;

    clingo_string_t *clingo_argv = malloc((argc - 1) * sizeof(clingo_string_t));
    if (clingo_argv == NULL) {
        fprintf(stderr, "failed to allocate memory for arguments\n");
        goto cleanup;
    }
    for (int i = 1; i < argc; ++i) {
        clingo_argv[i - 1].data = argv[i];
        clingo_argv[i - 1].size = strlen(argv[i]);
    }

    if (!clingo_lib_new(flags, level, NULL, NULL, limit, &lib)) {
        fprintf(stderr, "failed to create library\n");
        goto cleanup;
    };

    if (!clingo_main(lib, clingo_argv, argc - 1, &application, &app, &code)) {
        goto cleanup;
    }
cleanup:
    clingo_lib_release(lib);
    free(clingo_argv);
    free(app.program);
    return code;
}
