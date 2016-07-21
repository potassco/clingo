#include <clingo.h>
#include <stdlib.h>
#include <stdio.h>

clingo_error_t on_model(clingo_model_t *model, void *data, bool *goon) {
  (void)data;
  clingo_error_t ret = 0;
  clingo_symbol_t *atoms = NULL;
  size_t atoms_n;
  clingo_symbol_t const *it, *ie;
  char *str = NULL;
  size_t str_n = 0;
  // determine the number of (shown) symbols in the model
  if ((ret = clingo_model_symbols_size(model, clingo_show_type_shown, &atoms_n))) { goto error; }
  // allocate required memory to hold all the symbols
  if (!(atoms = (clingo_symbol_t*)malloc(sizeof(*atoms) * atoms_n))) { goto error; }
  // retrieve the symbols in the model
  if ((ret = clingo_model_symbols(model, clingo_show_type_shown, atoms, atoms_n))) { goto error; }
  printf("Model:");
  for (it = atoms, ie = atoms + atoms_n; it != ie; ++it) {
    size_t n;
    char *str_new;
    // determine size of the string representation of the next symbol in the model
    if ((ret = clingo_symbol_to_string_size(*it, &n))) { goto error; }
    if (str_n < n) {
      // allocate required memory to hold the symbol's string
      if (!(str_new = (char*)realloc(str, sizeof(*str) * n))) { goto error; }
      str = str_new;
      str_n = n;
    }
    // retrieve the symbol's string
    if (clingo_symbol_to_string(*it, str, n)) { goto error; }
    printf(" %s", str);
  }
  printf("\n");
  *goon = true;
  goto out;
error:
  if (!ret) { ret = clingo_error_unknown; }
out:
  if (atoms) { free(atoms); }
  if (str)   { free(str); }
  return ret;
}

int main(int argc, char const **argv) {
  int ret = 0;
  clingo_solve_result_bitset_t solve_ret;
  clingo_control_t *ctl = NULL;
  clingo_part_t parts[] = {{ "base", NULL, 0 }};
  // create a control object and pass command line arguments
  if (clingo_control_new(argv+1, argc-1, NULL, NULL, 20, &ctl) != 0) { goto error; }
  // add a logic program to the base part
  if (clingo_control_add(ctl, "base", NULL, 0, "a :- not b. b :- not a.")) { goto error; }
  // ground the base part
  if (clingo_control_ground(ctl, parts, 1, NULL, NULL)) { goto error; }
  // solve using a model callback
  if (clingo_control_solve(ctl, on_model, NULL, NULL, 0, &solve_ret)) { goto error; }
  goto out;
error:
  ret = 1;
out:
  if (ctl) { clingo_control_free(ctl); }
  return ret;
}

