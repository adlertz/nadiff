#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "types.h"
#include "parse.h"
#include "render.h"
#include "error.h"

int
main(int argc, char * argv[])
{
    struct diff_array da = {0};

    if (!parse_stdin(&da))
        return EXIT_FAILURE;

    FILE * tty = fopen("/dev/tty", "r");
    if (!tty) {
        na_printf("Unable to open /dev/tty. Needed when re-setting stdin\n");
        return EXIT_FAILURE;
    }

    int fd = fileno(tty);
    if (fd < 0) {
        na_printf("Unable to get file descriptor for /dev/tty\n");
        fclose(tty);
        return EXIT_FAILURE;
    }

    if (!render(fd, &da)) {
        fclose(tty);
        return EXIT_FAILURE;
    }

    fclose(tty);
    return EXIT_SUCCESS;
}
