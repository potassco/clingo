#include <clingo.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

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

typedef struct {
    // assignment of pigeons to holes
    // (hole number -> pigeon placement literal or zero)
    clingo_literal_t *holes;
    size_t size;
} state_t;

typedef struct {
    // mapping from aspif literals capturing pigeon placements to hole numbers
    // (aspif literal -> hole number or zero)
    int *pigeons;
    size_t pigeons_size;
    // array of states
    state_t *states;
    size_t states_size;
} pigeon_propagator_t;

bool get_arg(clingo_symbol_t sym, int offset, int *num) {
    clingo_symbol_t const *args;
    size_t args_size;
    if (!clingo_symbol_arguments(sym, &args, &args_size)) { return false; }
    if (!clingo_symbol_number(args[offset], num)) { return false; }
    return true;
}

bool init(clingo_propagate_init_t *init, pigeon_propagator_t *data) {
    int holes = 0;
    size_t threads = clingo_propagate_init_number_of_threads(init);
    clingo_literal_t max = 0;
    clingo_symbolic_atoms_t *atoms;
    clingo_signature_t sig;
    clingo_symbolic_atom_iterator_t atoms_it, atoms_ie;
    if (data->states != NULL) {
        clingo_set_error(clingo_error_runtime, "multi-shot solving is not supported");
        return false;
    }
    if (!(data->states = (state_t*)malloc(sizeof(*data->states) * threads))) {
        clingo_set_error(clingo_error_bad_alloc, "allocation failed");
        return false;
    }
    memset(data->states, 0, sizeof(*data->states) * threads);
    data->states_size = threads;

    if (!clingo_propagate_init_symbolic_atoms(init, &atoms)) { return false; }
    if (!clingo_signature_create("place", 2, true, &sig)) { return false; }
    if (!clingo_symbolic_atoms_end(atoms, &atoms_ie)) { return false; }
    for (int pass = 0; pass < 2; ++pass) {
        if (!clingo_symbolic_atoms_begin(atoms, &sig, &atoms_it)) { return false; }
        if (pass == 1) {
            if (!(data->pigeons = (int*)malloc(sizeof(*data->pigeons) * (max + 1)))) {
                clingo_set_error(clingo_error_bad_alloc, "allocation failed");
                return false;
            }
            data->pigeons_size = max + 1;
        }
        while (true) {
            int h;
            bool equal;
            clingo_literal_t lit;
            clingo_symbol_t sym;
            if (!clingo_symbolic_atoms_iterator_is_equal_to(atoms, atoms_it, atoms_ie, &equal)) { return false; }
            if (equal) { break; }
            if (!clingo_symbolic_atoms_literal(atoms, atoms_it, &lit)) { return false; }
            if (!clingo_propagate_init_solver_literal(init, lit, &lit)) { return false; }
            if (pass == 0) {
                assert(lit > 0);
                if (lit > max) { max = lit; }
            }
            else {
                if (!clingo_symbolic_atoms_symbol(atoms, atoms_it, &sym)) { return false; }
                if (!get_arg(sym, 1, &h)) { return false; }
                data->pigeons[lit] = h;
                if (!clingo_propagate_init_add_watch(init, lit)) { return false; }
                if (h > holes)   { holes = h; }
            }
            if (!clingo_symbolic_atoms_next(atoms, atoms_it, &atoms_it)) { return false; }
        }
    }
    for (size_t i = 0; i < threads; ++i) {
        if (!(data->states[i].holes = (clingo_literal_t*)malloc(sizeof(*data->states[i].holes) * (holes + 1)))) {
            clingo_set_error(clingo_error_bad_alloc, "allocation failed");
            return false;
        }
        memset(data->states[i].holes, 0, sizeof(*data->states[i].holes) * (holes + 1));
        data->states[i].size = holes + 1;
    }
    return true;
}

bool propagate(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size, pigeon_propagator_t *data) {
    state_t state = data->states[clingo_propagate_control_thread_id(control)];
    for (size_t i = 0; i < size; ++i) {
        clingo_literal_t lit = changes[i];
        clingo_literal_t *prev = state.holes + data->pigeons[lit];
        if (*prev == 0) { *prev = lit; }
        else {
            clingo_literal_t clause[] = { -lit, -*prev };
            bool result;
            if (!clingo_propagate_control_add_clause(control, clause, sizeof(clause)/sizeof(*clause), clingo_clause_type_learnt, &result)) { return false; }
            if (!result) { return true; }
            if (!clingo_propagate_control_propagate(control, &result)) { return false; }
            if (!result) { return true; }
            assert(false);
        }
    }
    return true;
}

bool undo(clingo_propagate_control_t *control, const clingo_literal_t *changes, size_t size, pigeon_propagator_t *data) {
    state_t state = data->states[clingo_propagate_control_thread_id(control)];
    for (size_t i = 0; i < size; ++i) {
        clingo_literal_t lit = changes[i];
        int hole = data->pigeons[lit];
        if (state.holes[hole] == lit) {
            state.holes[hole] = 0;
        }
    }
    return true;
}

int main(int argc, char const **argv) {
  char const *error_message;
  int ret = 0;
  clingo_solve_result_bitset_t solve_ret;
  clingo_control_t *ctl = NULL;
  clingo_symbol_t args[2];
  clingo_part_t parts[] = {{ "base", args, sizeof(args)/sizeof(*args) }};
  char const *params[] = {"h", "p"};
  pigeon_propagator_t prop_data = { NULL, 0, NULL, 0 };
  clingo_propagator_t prop = {
      (bool (*) (clingo_propagate_init_t *, void *))init,
      (bool (*) (clingo_propagate_control_t *, clingo_literal_t const *, size_t, void *))propagate,
      (bool (*) (clingo_propagate_control_t *, clingo_literal_t const *, size_t, void *))undo,
      NULL
  };

  clingo_symbol_create_number(8, &args[0]);
  clingo_symbol_create_number(9, &args[1]);

  // create a control object and pass command line arguments
  if (!clingo_control_new(argv+1, argc-1, NULL, NULL, 20, &ctl) != 0) { goto error; }

  // register the propagator
  if (!clingo_control_register_propagator(ctl, prop, &prop_data, false)) { goto error; }

  // add a logic program to the base part
  if (!clingo_control_add(ctl, "base", params, sizeof(params)/sizeof(*params), "1 { place(P,H) : H = 1..h } 1 :- P = 1..p.")) { goto error; }

  // ground the base part
  if (!clingo_control_ground(ctl, parts, 1, NULL, NULL)) { goto error; }

  // solve using a model callback
  if (!clingo_control_solve(ctl, on_model, NULL, NULL, 0, &solve_ret)) { goto error; }

  goto out;

error:
  if (!(error_message = clingo_error_message())) { error_message = "error"; }

  printf("%s\n", error_message);
  ret = clingo_error_code();

out:
  if (prop_data.pigeons) { free(prop_data.pigeons); }
  if (prop_data.states_size > 0) {
      for (size_t i = 0; i < prop_data.states_size; ++i) {
          if (prop_data.states[i].holes) {
              free(prop_data.states[i].holes);
          }
      }
      free(prop_data.states);
  }
  if (ctl) { clingo_control_free(ctl); }

  return ret;
}

