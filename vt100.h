#ifndef _NADIFF_VT100_H_
#define _NADIFF_VT100_H_

#include <stdbool.h>

enum vt100_key_type {
    KEY_TYPE_NONE,
    KEY_TYPE_UNKNOWN, /* a key which is not mapped to anything */
    KEY_TYPE_EXIT, /* exit nadiff */
    KEY_TYPE_ERROR, /* something went wrong, exit */
    KEY_TYPE_NEXT_DIFF,
    KEY_TYPE_PREV_DIFF,
    KEY_TYPE_NEXT_CHANGE,
    KEY_TYPE_PREV_CHANGE,
    KEY_TYPE_MOVE_DIFFS_UP,
    KEY_TYPE_MOVE_DIFFS_DOWN,
    KEY_TYPE_MOVE_DIFFS_LEFT,
    KEY_TYPE_MOVE_DIFFS_RIGHT,
};

struct vt100_dims {
    int rows;
    int cols;
};

void
vt100_enable_raw_mode(int fd);

enum vt100_key_type
vt100_read_key(int fd);

void
vt100_disable_raw_mode(int fd);

void
vt100_clear_screen(void);

void
vt100_goto_top_left(void);

bool
vt100_set_pos(int x, int y);

void
vt100_write(char const * data, unsigned len, unsigned max);

bool
vt100_get_window_size(struct vt100_dims * d);

void
vt100_hide_cursor(void);

void
vt100_show_cursor(void);

void
vt100_set_inverted_colors(void);

void
vt100_set_default_colors(void);

void
vt100_set_green_foreground(void);

void
vt100_set_green_background(void);

void
vt100_set_red_foreground(void);

void
vt100_set_red_background(void);

void
vt100_set_yellow_foreground(void);

void
vt100_set_underline(void);

void
vt100_leave_alternate_screen_buffer(void);

void
vt100_enter_alternate_screen_buffer(void);

#endif
