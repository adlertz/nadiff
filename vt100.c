#include "vt100.h"
#include "error.h"
#include "compare.h"

#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

struct termios org;

#define MAX_STR_SIZE 4096

char str[MAX_STR_SIZE];

void
vt100_enable_raw_mode(int fd)
{
    tcgetattr(fd, &org);

    struct termios raw = org;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    tcsetattr(fd, TCSAFLUSH, &raw);
}

enum vt100_key_type
vt100_read_key(int fd)
{
    char c;
    int ret = read(fd, &c, 1);

    if (ret == -1)
        return KEY_TYPE_ERROR;

    if (ret == 0)
        return KEY_TYPE_NONE;

    if (ret != 1)
        return KEY_TYPE_ERROR;

    switch (c) {
        case 'q':
            return KEY_TYPE_EXIT;
        case 'N':
            return KEY_TYPE_PREV_DIFF;
        case 'n':
            return KEY_TYPE_NEXT_DIFF;
        case 'a':
            return KEY_TYPE_MOVE_DIFF0_UP;
        case 'z':
            return KEY_TYPE_MOVE_DIFF0_DOWN;
        case 's':
            return KEY_TYPE_MOVE_DIFF1_UP;
        case 'x':
            return KEY_TYPE_MOVE_DIFF1_DOWN;
        case 'd':
            return KEY_TYPE_MOVE_DIFFS_UP;
        case 'c':
            return KEY_TYPE_MOVE_DIFFS_DOWN;
        default:
            return KEY_TYPE_UNKNOWN;
    }
}

void
vt100_disable_raw_mode(int fd)
{
  tcsetattr(fd, TCSAFLUSH, &org);
}

void
vt100_clear_screen()
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
}

void
vt100_hide_cursor()
{
    write(STDOUT_FILENO, "\x1b[?25l", 6);
}

void
vt100_show_cursor()
{
    write(STDOUT_FILENO, "\x1b[?25h", 6);
}

void
vt100_goto_top_left()
{
    write(STDOUT_FILENO, "\x1b[H", 3);
}

void
vt100_set_inverted_colors()
{
    /* 30-37 foreground, 40-47 background */
    /* 30-37 foreground, 40-47 background
     * 30 = black
     * 47 = white
     */
    //write(STDOUT_FILENO, "\x1b[30m\x1b[47m", 10);
    //write(STDOUT_FILENO, "\x1b[?5h", 5);
    /* turn on reverse mode */
    write(STDOUT_FILENO, "\x1b[7m", 4);
}

void
vt100_set_default_colors()
{
    /* turn off character attributes (such as reverse mode) */
    write(STDOUT_FILENO, "\x1b[m", 3);
}

void
vt100_set_green_foreground()
{
    write(STDOUT_FILENO, "\x1b[32m", 5);
}

void
vt100_set_red_foreground()
{
    write(STDOUT_FILENO, "\x1b[31m", 5);
}

void
vt100_set_green_background()
{
    write(STDOUT_FILENO, "\x1b[m\x1b[42m", 8);
}

void
vt100_set_red_background()
{
    write(STDOUT_FILENO, "\x1b[m\x1b[41m", 8);
}

void
vt100_set_yellow_foreground()
{
    write(STDOUT_FILENO, "\x1b[33m", 5);
}

void
vt100_set_underline()
{
    write(STDOUT_FILENO, "\x1b[4m", 4);
}

bool
vt100_set_pos(int x, int y)
{
    /* max length: \x1b[150;150H<NULL> */
    char a[14];
    sprintf(a, "\x1b[%d;%dH", y, x);
    int len = strlen(a);
    write(STDOUT_FILENO, a, len);

    return true;
}

void
vt100_write(char const * data, unsigned len, unsigned max, unsigned horizontal_offset)
{
    if (horizontal_offset >= len)
        return;

    int si = 0;
    for (unsigned i = horizontal_offset; i < len; i++) {
        if (i >= MAX_STR_SIZE)
            break;

        char c = data[i];
        if (c == '\t') {
            str[si++] = ' ';
            str[si++] = ' ';
            str[si++] = ' ';
            str[si++] = ' ';
         } else {
            str[si++] = c;
         }
    }

    write(STDOUT_FILENO, str, MIN(si, max));
}

bool
vt100_get_window_size(struct vt100_dims * d)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        na_printf("Could not get terminal size\n");
        return false;
    } else {
        d->cols = ws.ws_col;
        d->rows = ws.ws_row;
        return true;
    }
}

void
vt100_leave_alternate_screen_buffer(void)
{
    write(STDOUT_FILENO, "\x1b[?1049l", 8);
}

void
vt100_enter_alternate_screen_buffer(void)
{
    write(STDOUT_FILENO, "\x1b[?1049h", 8);
}
