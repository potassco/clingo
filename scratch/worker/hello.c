#include <emscripten.h>
#include <stdio.h>

EMSCRIPTEN_KEEPALIVE
const char *hello_world() {
    return "Hello, World from WebAssembly!";
}
