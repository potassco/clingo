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

typedef struct {
    // assignment of pigeons to holes
    // (hole number -> pigeon placement literal or zero)
    clingo_literal_t *holes;
    size_t size;
} state_t;

typedef struct {
    // mapping from aspif literals capturing pigeon placements to hole numbers
    // (aspif literal -> hole number or zero)
    clingo_literal_t *pigeons;
    size_t pigeons_size;
    // array of states
    clingo_literal_t **states;
    size_t states_size;
} pigeon_propagator_t;

bool init(clingo_propagate_init_t *init, pigeon_propagator_t *data) {
    printf("%s\n", "init!!!!");
    clingo_set_error(clingo_error_logic, "implement me!!!");
    return false;
}

bool propagate(clingo_propagate_control_t *init, const clingo_literal_t *changes, size_t size, pigeon_propagator_t *data) {
    clingo_set_error(clingo_error_logic, "implement me!!!");
    return false;
}

bool undo(clingo_propagate_control_t *init, const clingo_literal_t *changes, size_t size, pigeon_propagator_t *data) {
    clingo_set_error(clingo_error_logic, "implement me!!!");
    return false;
}
/*
class PigeonPropagator : public Propagator {
public:
    void init(PropagateInit &init) override {
        unsigned nHole = 0, nPig = 0, nWatch = 0, p, h;
        state_.clear();
        p2h_[0] = 0;
        for (auto it = init.symbolic_atoms().begin(Signature("place", 2)), ie = init.symbolic_atoms().end(); it != ie; ++it) {
            literal_t lit = init.solver_literal(it->literal());
            p = it->symbol().arguments()[0].number();
            h = it->symbol().arguments()[1].number();
            p2h_[lit] = h;
            init.add_watch(lit);
            nHole = std::max(h, nHole);
            nPig  = std::max(p, nPig);
            ++nWatch;
        }
        assert(p2h_[0] == 0);
        for (unsigned i = 0, end = init.number_of_threads(); i != end; ++i) {
            state_.emplace_back(nHole + 1, 0);
            assert(state_.back().size() == nHole + 1);
        }
    }
    void propagate(PropagateControl &ctl, LiteralSpan changes) override {
        assert(ctl.thread_id() < state_.size());
        Hole2Lit& holes = state_[ctl.thread_id()];
        for (literal_t lit : changes) {
            literal_t& prev = holes[ p2h_[lit] ];
            if (prev == 0) { prev = lit; }
            else {
                if (!ctl.add_clause({-lit, -prev}) || !ctl.propagate()) {
                    return;
                }
                assert(false);
            }
        }
    }
    void undo(PropagateControl const &ctl, LiteralSpan undo) override {
        assert(ctl.thread_id() < state_.size());
        Hole2Lit& holes = state_[ctl.thread_id()];
        for (literal_t lit : undo) {
            unsigned hole = p2h_[lit];
            if (holes[hole] == lit) {
                holes[hole] = 0;
            }
        }
    }
private:
    using Lit2Hole = std::unordered_map<literal_t, unsigned>;
    using Hole2Lit = std::vector<literal_t>;
    using State    = std::vector<Hole2Lit>;

    Lit2Hole p2h_;
    State    state_;
};

*/

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
  // TODO: free prop_data
  if (ctl) { clingo_control_free(ctl); }

  return ret;
}

