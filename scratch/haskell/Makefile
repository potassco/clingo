test: Main.o Clingo/FFI.o ../../build/release/bin/libclingo.so
	lib=$$(cd ../../build/release/bin; pwd -P) && ghc -o $@ $^ -optl-Wl,-rpath=$${lib}

Main.o: Clingo/FFI.o
Main.o: Main.hs
	ghc -c $<

Clingo/FFI.o: Clingo/FFI.hs
	ghc -c $<

Clingo/FFI.hs: Clingo/FFI.hsc
	hsc2hs -I/usr/lib64/bindings-dsl-1.0.23/ghc-8.0.1/include -I. $<

Clingo/FFI.hsc: ../../libclingo/clingo.h Clingo
	c2hsc --prefix Clingo.FFI $< -s > $@
	sed -i \
	  -e 's/^module Clingo.FFI.Clingo where/module Clingo.FFI where/g' \
	  -e '/^#opaque_t struct clingo_ast_unary_operation/d' \
	  -e '/^#opaque_t struct clingo_ast_binary_operation/d' \
	  -e '/^#opaque_t struct clingo_ast_interval/d' \
	  -e '/^#opaque_t struct clingo_ast_function/d' \
	  -e '/^#opaque_t struct clingo_ast_pool/d' \
	  -e '/^#opaque_t struct clingo_ast_theory_term_array/d' \
	  -e '/^#opaque_t struct clingo_ast_theory_function/d' \
	  -e '/^#opaque_t struct clingo_ast_theory_unparsed_term/d' \
	  $@

Clingo:
	mkdir Clingo

.PHONY: clean
clean:
	rm -rf test Main.o Clingo

