#include <clingo.h>
#include <stdlib.h>
#include <stdio.h>

bool print_model(clingo_model_t *model) {
  bool ret = true;
  clingo_symbol_t *atoms = NULL;
  size_t atoms_n;
  clingo_symbol_t const *it, *ie;
  char *str = NULL;
  size_t str_n = 0;

  // determine the number of (shown) symbols in the model
  if (!clingo_model_symbols_size(model, clingo_show_type_shown, &atoms_n)) { goto error; }

  // allocate required memory to hold all the symbols
  if (!(atoms = (clingo_symbol_t*)malloc(sizeof(*atoms) * atoms_n))) {
    clingo_set_error(clingo_error_bad_alloc, "could not allocate memory for atoms");
    goto error;
  }

  // retrieve the symbols in the model
  if (!clingo_model_symbols(model, clingo_show_type_shown, atoms, atoms_n)) { goto error; }

  printf("Model:");

  for (it = atoms, ie = atoms + atoms_n; it != ie; ++it) {
    size_t n;
    char *str_new;

    // determine size of the string representation of the next symbol in the model
    if (!clingo_symbol_to_string_size(*it, &n)) { goto error; }

    if (str_n < n) {
      // allocate required memory to hold the symbol's string
      if (!(str_new = (char*)realloc(str, sizeof(*str) * n))) {
        clingo_set_error(clingo_error_bad_alloc, "could not allocate memory for symbol's string");
        goto error;
      }

      str = str_new;
      str_n = n;
    }

    // retrieve the symbol's string
    if (!clingo_symbol_to_string(*it, str, n)) { goto error; }

    printf(" %s", str);
  }

  printf("\n");
  goto out;

error:
  ret = false;

out:
  if (atoms) { free(atoms); }
  if (str)   { free(str); }

  return ret;
}

bool solve(clingo_control_t *ctl, clingo_solve_result_bitset_t *result) {
  bool ret = true;
  clingo_solve_handle_t *handle;
  clingo_model_t *model;

  // get a solve handle
  if (!clingo_control_solve(ctl, clingo_solve_mode_yield, NULL, 0, NULL, NULL, &handle)) { goto error; }
  // loop over all models
  while (true) {
    if (!clingo_solve_handle_resume(handle)) { goto error; }
    if (!clingo_solve_handle_model(handle, &model)) { goto error; }
    // print the model
    if (model) { print_model(model); }
    // stop if there are no more models
    else       { break; }
  }
  // close the solve handle
  if (!clingo_solve_handle_get(handle, result)) { goto error; }

  goto out;

error:
  ret = false;

out:
  // free the solve handle
  return clingo_solve_handle_close(handle) && ret;
}

void print_prefix(int depth) {
  for (int i = 0; i < depth; ++i) {
    printf("  ");
  }
}

bool userstats(clingo_user_statistics_t* stats, void* data) {
  size_t root;
  size_t map;
  size_t array;
  size_t c;
  size_t value;
  if (!clingo_user_statistics_root(stats, &root)) { goto error; } 
  if (!clingo_user_statistics_map_get(stats, root, "information", clingo_statistics_type_map, &map)) { goto error; }
  if (!clingo_user_statistics_map_get(stats, map, "Animals", clingo_statistics_type_array, &array)) { goto error; }
  if (!clingo_user_statistics_array_get(stats, array, 0, clingo_statistics_type_map, &c)) { goto error; }
  if (!clingo_user_statistics_map_get(stats, c, "Age", clingo_statistics_type_value, &value)) { goto error; }
  if (!clingo_user_statistics_value_set(stats, value, 42)) { goto error; }
  if (!clingo_user_statistics_map_get(stats, c, "Legs", clingo_statistics_type_value, &value)) { goto error; }
  if (!clingo_user_statistics_value_set(stats, value, 13)) { goto error; }
  if (!clingo_user_statistics_map_get(stats, map, "DeathCounter", clingo_statistics_type_value, &value)) { goto error; }
  if (!clingo_user_statistics_value_set(stats, value, 42)) { goto error; }
  if (!clingo_user_statistics_array_get(stats, array, 1, clingo_statistics_type_map, &c)) { goto error; }
  if (!clingo_user_statistics_map_get(stats, c, "Age", clingo_statistics_type_value, &value)) { goto error; }
  if (!clingo_user_statistics_value_set(stats, value, 2)) { goto error; }
  if (!clingo_user_statistics_map_get(stats, c, "Legs", clingo_statistics_type_value, &value)) { goto error; }
  if (!clingo_user_statistics_value_set(stats, value, 7)) { goto error; }
  if (!clingo_user_statistics_array_get(stats, array, 3, clingo_statistics_type_map, &c)) { goto error; }
  if (!clingo_user_statistics_map_get(stats, c, "Age", clingo_statistics_type_value, &value)) { goto error; }
  if (!clingo_user_statistics_value_set(stats, value, 423)) { goto error; }
  if (!clingo_user_statistics_map_get(stats, c, "Legs", clingo_statistics_type_value, &value)) { goto error; }
  if (!clingo_user_statistics_value_set(stats, value, 0)) { goto error; }
  if (!clingo_user_statistics_array_get(stats, array, 4, clingo_statistics_type_value, &value)) { goto error; }
  if (!clingo_user_statistics_value_set(stats, value, 42)) { goto error; }

  return true;
error:
  return false;
}

// recursively print the statistics object
bool print_statistics(clingo_statistics_t *stats, uint64_t key, int depth) {
  bool ret = true;
  clingo_statistics_type_t type;

  // get the type of an entry and switch over its various values
  if (!clingo_statistics_type(stats, key, &type)) { goto error; }
  switch ((enum clingo_statistics_type)type) {
    // print values
    case clingo_statistics_type_value: {
      double value;

      // print value (with prefix for readability)
      print_prefix(depth);
      if (!clingo_statistics_value_get(stats, key, &value)) { goto error; }
      printf("%g\n", value);

      break;
    }

    // print arrays
    case clingo_statistics_type_array: {
      size_t size;

      // loop over array elements
      if (!clingo_statistics_array_size(stats, key, &size)) { goto error; }
      for (size_t i = 0; i < size; ++i) {
        uint64_t subkey;

        // print array offset (with prefix for readability)
        if (!clingo_statistics_array_at(stats, key, i, &subkey)) { goto error; }
        print_prefix(depth);
        printf("%zu:\n", i);

        // recursively print subentry
        if (!print_statistics(stats, subkey, depth+1)) { goto error; }
      }
      break;
    }

    // print maps
    case clingo_statistics_type_map: {
      size_t size;

      // loop over map elements
      if (!clingo_statistics_map_size(stats, key, &size)) { goto error; }
      for (size_t i = 0; i < size; ++i) {
        char const *name;
        uint64_t subkey;

        // get and print map name (with prefix for readability)
        if (!clingo_statistics_map_subkey_name(stats, key, i, &name)) { goto error; }
        if (!clingo_statistics_map_at(stats, key, name, &subkey)) { goto error; }
        print_prefix(depth);
        printf("%s:\n", name);

        // recursively print subentry
        if (!print_statistics(stats, subkey, depth+1)) { goto error; }
      }
    }

    // this case won't occur if the statistics are traversed like this
    case clingo_statistics_type_empty: { goto out; }
  }

  goto out;
error:
  ret = false;
out:
  return ret;
}

int main(int argc, char const **argv) {
  char const *error_message;
  int ret = 0;
  clingo_solve_result_bitset_t solve_ret;
  clingo_control_t *ctl = NULL;
  clingo_part_t parts[] = {{ "base", NULL, 0 }};
  clingo_configuration_t *conf;
  clingo_id_t conf_root, conf_sub;
  clingo_statistics_t *stats;
  uint64_t stats_key;

  // create a control object and pass command line arguments
  if (!clingo_control_new(argv+1, argc-1, NULL, NULL, 20, &ctl) != 0) { goto error; }

  // get the configuration object and its root key
  if (!clingo_control_configuration(ctl, &conf)) { goto error; }
  if (!clingo_configuration_root(conf, &conf_root)) { goto error; }
  // and set the statistics level to one to get more statistics
  if (!clingo_configuration_map_at(conf, conf_root, "stats", &conf_sub)) { goto error; }
  if (!clingo_configuration_value_set(conf, conf_sub, "1")) { goto error; }

  // add a logic program to the base part
  if (!clingo_control_add(ctl, "base", NULL, 0, "a :- not b. b :- not a.")) { goto error; }

  // add a callback to set userdefined statistics
  if (!clingo_control_add_user_statistics(ctl, userstats, 0)) { goto error; }

  // ground the base part
  if (!clingo_control_ground(ctl, parts, 1, NULL, NULL)) { goto error; }

  // solve
  if (!solve(ctl, &solve_ret)) { goto error; }

  // get the statistics object, get the root key, then print the statistics recursively
  if (!clingo_control_statistics(ctl, &stats)) { goto error; }
  if (!clingo_statistics_root(stats, &stats_key)) { goto error; }
  if (!print_statistics(stats, stats_key, 0)) { goto error; }

  goto out;

error:
  if (!(error_message = clingo_error_message())) { error_message = "error"; }

  printf("%s\n", error_message);
  ret = clingo_error_code();

out:
  if (ctl) { clingo_control_free(ctl); }

  return ret;
}

