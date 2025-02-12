#include <emscripten.h>
#include <stdio.h>

EMSCRIPTEN_KEEPALIVE
const char *hello_world() {
    printf("This is printed to stdout\n");
    return "Hello, World from WebAssembly!";
}
