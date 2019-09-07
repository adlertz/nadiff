#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "types.h"
#include "parse.h"
#include "render.h"
#include "error.h"

static int
get_tty_fd()
{
    FILE * tty = fopen("/dev/tty", "r");
    if (!tty) {
        na_printf("Unable to open /dev/tty. Needed when re-setting stdin. \n");
        return -1;
    }

    return fileno(tty);
}

static void
cleanup(struct diff_array * da)
{
    // free everything..
}

int
main(int argc, char * argv[])
{
    struct diff_array da = {0};

    if (!parse_stdin(&da)) {
        cleanup(&da);
        return EXIT_FAILURE;
    }

    int fd = get_tty_fd();
    if (fd < 0 ) {
        cleanup(&da);
        return EXIT_FAILURE;
    }

    render(fd, &da);

    cleanup(&da);

    return EXIT_SUCCESS;
}
