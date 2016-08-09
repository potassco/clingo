#include <clingo.h>
#include <stdlib.h>
#include <stdio.h>

bool on_model(clingo_model_t *model, void *data, bool *goon) {
  (void)data;
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
  *goon = true;
  goto out;

error:
  ret = false;

out:
  if (atoms) { free(atoms); }
  if (str)   { free(str); }

  return ret;
}

bool on_finish(clingo_solve_result_bitset_t result, bool *finished) {
  (void)result;
  *finished = true;
  return true;
}

int main(int argc, char const **argv) {
  char const *error_message;
  int ret = 0;
  bool finished = false;
  uint64_t samples = 0;
  uint64_t incircle = 0;
  uint64_t x, y;
  clingo_solve_result_bitset_t solve_ret;
  clingo_control_t *ctl = NULL;
  clingo_solve_async_t *async = NULL;
  clingo_part_t parts[] = {{ "base", NULL, 0 }};

  // create a control object and pass command line arguments
  if (!clingo_control_new(argv+1, argc-1, NULL, NULL, 20, &ctl) != 0) { goto error; }

  // add a logic program to the base part
  if (!clingo_control_add(ctl, "base", NULL, 0, "#const n = 17."
                                                "1 { p(X); q(X) } 1 :- X = 1..n."
                                                ":- not n+1 { p(1..n); q(1..n) }.")) { goto error; }

  // ground the base part
  if (!clingo_control_ground(ctl, parts, 1, NULL, NULL)) { goto error; }

  // solve using a model callback
  if (!clingo_control_solve_async(ctl, on_model, NULL, (clingo_finish_callback_t*)on_finish, &finished, NULL, 0, &async)) { goto error; }

  // let's approximate pi
  do {
    ++samples;
    x = rand();
    y = rand();
    if (x * x + y * y <= (uint64_t)RAND_MAX * RAND_MAX) { incircle+= 1; }
  }

  while (!finished);

  printf("pi = %g\n", 4.0*incircle/samples);

  // get the result (and make sure the search is finished because calling the finish handler is still part of the search)
  if (!clingo_solve_async_get(async, &solve_ret)) { goto error; }

  goto out;

error:
  if (!(error_message = clingo_error_message())) { error_message = "error"; }

  printf("%s\n", error_message);
  ret = clingo_error_code();

out:
  if (ctl) { clingo_control_free(ctl); }

  return ret;
}

