#include "io.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_LINE_SIZE 4096

static char buf[MAX_LINE_SIZE];
static struct line l;
static bool use_prev_line = false;
static unsigned row = 1;

struct line *
stdin_read_line(void)
{
    if (use_prev_line) {
        use_prev_line = false;
        return &l;
    }

    l.row = row++;
    l.len = 0;
    l.data = NULL;

    if (fgets(buf, sizeof(buf), stdin) == NULL)
        return &l;

    /* in case we can't read complete line */
    buf[MAX_LINE_SIZE - 1] = '\0';

    /* get the length of the buffer, i.e. how much we've read */
    size_t buf_len = strlen(buf);

    /* replace '\n' with '\0' */
    if (buf[buf_len - 1] == '\n')
        buf[buf_len - 1] = '\0';

    if (buf[buf_len - 1] == '\0') {
        l.len = buf_len;
        l.data = buf;
    }

    return &l;
}

void
stdin_reset_cur_line(void)
{
    use_prev_line = true;
}
