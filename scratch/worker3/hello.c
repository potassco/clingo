#include <emscripten.h>
#include <stdio.h>

EMSCRIPTEN_KEEPALIVE
void process_input() {
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), stdin) != NULL) {
        printf("Received: %s", buffer);
    }
    printf("Processing complete.\n");
}
