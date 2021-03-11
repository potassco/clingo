%module _clingo

%immutable clingo_location::begin_file;
%immutable clingo_location::end_file;
%immutable clingo_ast_constructor::name;
%immutable clingo_part::name;

%rename("enum_%s", %$isenum) "";

%include "clingo.h"

%extend clingo_location {
    bool begin_file_set(char const *value);
    bool end_file_set(char const *value);
}

%extend clingo_part {
    bool name_set(char const *value);
}

%{
#include <clingo.h>

bool clingo_location_begin_file_set(clingo_location_t *loc, char const *value) {
    return clingo_add_string(value, &loc->begin_file);
}

bool clingo_location_end_file_set(clingo_location_t *loc, char const *value) {
    return clingo_add_string(value, &loc->end_file);
}

bool clingo_part_name_set(clingo_part_t *part, char const *value) {
    return clingo_add_string(value, &part->name);
}
%}
