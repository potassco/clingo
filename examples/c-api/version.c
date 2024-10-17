#include <clingo.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    int major = 0;
    int minor = 0;
    int revision = 0;

    clingo_version(&major, &minor, &revision);

    printf("Hello, this is clingo version %d.%d.%d.\n", major, minor, revision);

    return 0;
}
