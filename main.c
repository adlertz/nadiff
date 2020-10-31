#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h> // isatty()

#include "types.h"
#include "parse.h"
#include "render.h"
#include "error.h"

const char * semantic_version = "1.1.0";

static void print_help()
{
    printf("Usage:\n");
    printf("    git diff | nadiff\n");
    printf("    git diff --staged | nadiff\n");
    printf("    git diff HEAD~1..HEAD | nadiff\n");
    printf("    You get the point.\n");
    printf("    You can also use 'git-nadiff' which is installed together wih nadiff:\n");
    printf("    git-nadiff HEAD~1..HEAD\n");
    printf("\n");
    printf("NOTE: Tabs are displayed as tilde with 3 spaces; '~   '.\n");
    printf("\n");
    printf("Navigation:\n");
    printf("    n           Next diff.\n");
    printf("    N           Previous diff.\n");
    printf("    k/d         Scroll up in both views.\n");
    printf("    j/c         Scroll down in both views.\n");
    printf("    h/w         Scroll left in both views.\n");
    printf("    l/e         Scroll right in both views.\n");
    printf("    q           Quit.\n");
    printf("\n");
    printf("Options:\n");
    printf("    --help      Display this information.\n");
    printf("    --version   Display version information.\n");
}

static void print_version()
{
    printf("nadiff %s\n", semantic_version);
}

int
main(int argc, char * argv[])
{
    if (argc >= 2) {
        const char * option = argv[1];
        if (strcmp(option, "--version") == 0 || strcmp(option, "-v") == 0) {
            print_version();
        } else if (strcmp(option, "--help") == 0 || strcmp(option, "-h") == 0) {
            print_help();
        } else { /* Unknown option */
            printf("Unknown command line option: '%s'\n", option);
            print_help();
        }

        return EXIT_SUCCESS;
    }

    if (isatty(fileno(stdin))) {
        print_help();
        return EXIT_SUCCESS;
    }

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
