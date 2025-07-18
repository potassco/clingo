#include <clingo/core.h>
#include <stdio.h>

int main(void) {
    int major = 0;
    int minor = 0;
    int revision = 0;

    clingo_version(&major, &minor, &revision);

    printf("Hello, this is clingo version %d.%d.%d.\n", major, minor, revision);

    return 0;
}
